#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xgbc.h"

unsigned cumulative_cycles = 0;

extern uint8_t *oam, *full_vidram;
extern uint16_t bpalette[32], opalette[32];
extern uint8_t *btm[2], *bwtd[2], *wtm[2];

extern void update_cpu(unsigned cycles);

extern uint8_t keystates;

struct io io_state = {
    .p1     = 0x00,
    .sb     = 0xFF,
    .sc     = 0x00,
    .div    = 0x00,
    .tima   = 0x00,
    .tma    = 0x00,
    .tac    = 0x00,
    .nr10   = 0x80,
    .nr11   = 0xBF,
    .nr12   = 0xF3,
    .nr14   = 0xBF,
    .nr21   = 0x3F,
    .nr22   = 0x00,
    .nr24   = 0xBF,
    .nr30   = 0x7F,
    .nr31   = 0xFF,
    .nr32   = 0x9F,
    .nr33   = 0xBF,
    .nr41   = 0xFF,
    .nr42   = 0x00,
    .nr43   = 0x00,
    .nr44   = 0xBF,
    .nr50   = 0x77,
    .nr51   = 0xF3,
    .nr52   = 0xF1,
    .lcdc   = 0x83,
    .stat   = 0x01,
    .scy    = 0x00,
    .scx    = 0x00,
    .lyc    = 0x00,
    .bgp    = 0xFC,
    .obp0   = 0xFF,
    .obp1   = 0xFF,
    .wy     = 0x00,
    .wx     = 0x00,
    .hdma5  = 0xFF
};

uint8_t ie = 0;

static bool timer_running = false, hdma_on = false;
bool lcd_on = true;
static unsigned timer_div = 0;

void hdma_copy_16b(void)
{
    uintptr_t src = ((uint16_t)io_state.hdma1 << 8) | io_state.hdma2;
    uintptr_t dst = ((uint16_t)io_state.hdma3 << 8) | io_state.hdma4;

    src &= 0xFFF0;
    src += MEM_BASE;
    dst &= 0x1FF0;
    dst += MEM_BASE | 0x8000;

    if (likely(src < 0xF000))
        memcpy((void *)dst, (void *)src, 16);
    else
        for (size_t i = 0; i < 16; i++)
            ((uint8_t *)dst)[i] = hmem_read8(src & 0xFFF);

    src += 16;
    dst += 16;

    io_state.hdma1 = src >> 8;
    io_state.hdma2 = src & 0xFF;
    io_state.hdma3 = dst >> 8;
    io_state.hdma4 = dst & 0xFF;

    if (--io_state.hdma5 & 0x80)
        hdma_on = false;

    // Achtung, Doublespeed: 16 Cycles
    update_cpu(8);
}

void update_timer(bool enable, unsigned mode)
{
    timer_running = enable;

    switch (mode)
    {
        case 0:
            timer_div = 256;
            break;
        case 1:
            timer_div = 4;
            break;
        case 2:
            timer_div = 16;
            break;
        case 3:
            timer_div = 64;
            break;
    }
}

void inc_timer(unsigned cycles)
{
    static unsigned cum_cycles = 0, cum_cycles_div = 0, lcd_cycles = 0;

    cumulative_cycles += cycles;

    if (timer_running)
    {
        cum_cycles += cycles;
        while (cum_cycles >= timer_div)
        {
            cum_cycles -= timer_div;

            if (!++io_state.tima)
            {
                io_state.tima = io_state.tma;
                io_state.int_flag |= INT_TIMER;
            }
        }
    }

    cum_cycles_div += cycles;
    while (cum_cycles_div >= 64)
    {
        cum_cycles_div -= 64;
        io_state.div++;
    }

    static bool redrawed = false;

    if (lcd_on)
    {
        bool hblank_start = false;

        lcd_cycles += cycles;

        io_state.stat &= ~3;
        if (io_state.ly >= 144)
            io_state.stat |= 1;
        else
        {
            if (lcd_cycles < 51)
                redrawed = false;
            else if (lcd_cycles < 71)
                io_state.stat |= 2;
            else
            {
                io_state.stat |= 3;
                if (!redrawed)
                {
                    draw_line(io_state.ly);
                    redrawed = true;
                }
            }
        }

        while (lcd_cycles >= 114)
        {
            lcd_cycles -= 114;

            hblank_start = true;
            if (hdma_on)
                hdma_copy_16b();
            if (++io_state.ly > 153)
                io_state.ly = 0;
            if (io_state.ly == 144)
            {
                io_state.stat = (io_state.stat & ~3) | 1;
                io_state.int_flag |= INT_VBLANK;
            }
            if (io_state.lyc != io_state.ly)
                io_state.stat &= ~(1 << 2);
            else
            {
                io_state.stat |= (1 << 2);
                if (io_state.stat & (1 << 6))
                    io_state.int_flag |= INT_LCDC_STAT;
            }
        }

        if ((io_state.stat & (1 << 5)) && ((io_state.stat & 3) == 2))
            io_state.int_flag |= INT_LCDC_STAT;
        else if ((io_state.stat & (1 << 4)) && ((io_state.stat & 3) == 1))
            io_state.int_flag |= INT_LCDC_STAT;
        else if ((io_state.stat & (1 << 3)) && hblank_start)
            io_state.int_flag |= INT_LCDC_STAT;
    }
}

uint8_t io_read(uint8_t reg)
{
    if (reg == 0xFF)
        return ie;

    return ((uint8_t *)&io_state)[reg];
}

void io_write(uint8_t reg, uint8_t val)
{
    switch (reg)
    {
        case 0x00: // P1 -- joypad
            val &= KEY_DIR | KEY_OTH;
            if (val == (KEY_DIR | KEY_OTH))
                val = 0xFF;
            if (!(val & KEY_DIR))
                val |= (~keystates & 0xF0) >> 4;
            if (!(val & KEY_OTH))
                val |= ~keystates & 0x0F;
            io_state.p1 = val;
            break;
        case 0x07: // TAC -- timer control
            io_state.tac = val & 0x07;
            update_timer((val & (1 << 2)) >> 2, val & 3);
            break;
        case 0x0F: // IF -- interrupt flag
            io_state.int_flag = val & 0x1F;
            update_cpu(0);
            break;
        case 0x40: // LCDC -- LCD control
            io_state.lcdc = val;
            lcd_on = val >> 7;
            if (val & (1 << 6))
            {
                wtm[0] = (uint8_t *)&full_vidram[0x1C00];
                wtm[1] = (uint8_t *)&full_vidram[0x3C00];
            }
            else
            {
                wtm[0] = (uint8_t *)&full_vidram[0x1800];
                wtm[1] = (uint8_t *)&full_vidram[0x3800];
            }
            if (val & (1 << 4))
            {
                bwtd[0] = (uint8_t *)&full_vidram[0x0000];
                bwtd[1] = (uint8_t *)&full_vidram[0x2000];
            }
            else
            {
                bwtd[0] = (uint8_t *)&full_vidram[0x1000];
                bwtd[1] = (uint8_t *)&full_vidram[0x3000];
            }
            if (val & (1 << 3))
            {
                btm[0] = (uint8_t *)&full_vidram[0x1C00];
                btm[1] = (uint8_t *)&full_vidram[0x3C00];
            }
            else
            {
                btm[0] = (uint8_t *)&full_vidram[0x1800];
                btm[1] = (uint8_t *)&full_vidram[0x3800];
            }
            break;
        case 0x41: // STAT -- LCD status
            io_state.stat &= 0x87;
            io_state.stat |= val & 0x78;
            break;
        case 0x46: // DMA -- DMA transfer to OAM
            if (val < 0xF0)
                memcpy(oam, (void *)(MEM_BASE + ((uintptr_t)val << 8)), 0xA0);
            else
            {
                uint16_t base = (uint16_t)(val & 0x0F) << 8;
                for (size_t a = 0; a < 0xA0; a++)
                    oam[a] = hmem_read8(base + a);
            }
            break;
        case 0x4F: // VBK -- select VRAM bank
            val &= 1;
            if (io_state.vbk != val)
                mem_select_vram_bank((io_state.vbk) = val);
            break;
        case 0x55: // HDMA5 -- DMA transfer length
            if (hdma_on)
            {
                if (val & 0x80)
                    io_state.hdma5 = val & 0x7F;
                else
                {
                    io_state.hdma5 = 0xFF;
                    hdma_on = false;
                }
                break;
            }
            io_state.hdma5 = val & 0x7F;
            hdma_on = true;
            if (!(val & 0x80)) // Sofort übertragen (nicht erst im HBlank)
                while (hdma_on)
                    hdma_copy_16b();
            break;
        case 0x68: // BGPI -- background palette index
            io_state.bcps = val & 0xBF;
            io_state.bcpd = (val & 1) ? (bpalette[(val >> 1) & 0x1F] >> 8) : (bpalette[(val >> 1) & 0x1F] & 0xFF);
            break;
        case 0x69: // BGPD -- background palette data
            io_state.bcpd = val;
            if (io_state.bcps & 1)
            {
                bpalette[(io_state.bcps >> 1) & 0x1F] &= 0x00FF;
                bpalette[(io_state.bcps >> 1) & 0x1F] |= (val << 8) & 0x7F00;
            }
            else
            {
                bpalette[(io_state.bcps >> 1) & 0x1F] &= 0xFF00;
                bpalette[(io_state.bcps >> 1) & 0x1F] |= val;
            }
            if (io_state.bcps & 0x80)
                io_write(0x68, io_state.bcps + 1);
            break;
        case 0x6A: // OBPI -- object palette index
            io_state.ocps = val & 0xBF;
            io_state.ocpd = (val & 1) ? (opalette[(val >> 1) & 0x1F] >> 8) : (opalette[(val >> 1) & 0x1F] & 0xFF);
            break;
        case 0x6B: // OBPD -- object palette data
            io_state.ocpd = val;
            if (io_state.ocps & 1)
            {
                opalette[(io_state.ocps >> 1) & 0x1F] &= 0x00FF;
                opalette[(io_state.ocps >> 1) & 0x1F] |= (val << 8) & 0x7F00;
            }
            else
            {
                opalette[(io_state.ocps >> 1) & 0x1F] &= 0xFF00;
                opalette[(io_state.ocps >> 1) & 0x1F] |= val;
            }
            if (io_state.ocps & 0x80)
                io_write(0x6A, io_state.ocps + 1);
            break;
        case 0x70: // SVBK -- select WRAM bank
            val &= 7;
            if (!val)
                val = 1;
            if (io_state.svbk != val)
                mem_select_wram_bank((io_state.svbk = val));
            break;
        case 0xFF: // IE -- interrupt enable
            ie = val & 0x1F;
            update_cpu(0);
            break;
        // Einfach reinschreiben
        case 0x02: // SC -- serial control
        case 0x06: // TMA -- timer modulo
        case 0x10: // NR10 -- channel 1 sweep register
        case 0x11: // NR11 -- channel 1 sound length/wave pattern
        case 0x12: // NR12 -- channel 1 volume envelope
        case 0x13: // NR13 -- channel 1 freq low
        case 0x14: // NR14 -- channel 1 freq high
        case 0x15: // (NR20)
        case 0x16: // NR21 -- channel 2 sound length/wave pattern
        case 0x17: // NR22 -- channel 2 volume envelope
        case 0x18: // NR23 -- channel 2 freq low
        case 0x19: // NR24 -- channel 2 freq high
        case 0x1A: // NR30 -- channel 3 on/off
        case 0x1B: // NR31 -- channel 3 sound length
        case 0x1C: // NR32 -- channel 3 output level
        case 0x1D: // NR33 -- channel 3 freq low
        case 0x1E: // NR34 -- channel 3 freq high
        case 0x1F: // (NR40)
        case 0x20: // NR41 -- channel 4 sound length
        case 0x21: // NR42 -- channel 4 volume envelope
        case 0x22: // NR43 -- channel 4 polynomical counter
        case 0x23: // NR44 -- channel 4 counter/consecutive
        case 0x24: // NR50 -- sound channel control
        case 0x25: // NR51 -- selection of sound output terminal
        case 0x26: // NR52 -- sound on/off
        case 0x30: // wave pattern RAM
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
        case 0x3A:
        case 0x3B:
        case 0x3C:
        case 0x3D:
        case 0x3E:
        case 0x3F:
        case 0x42: // SCY -- scroll Y
        case 0x43: // SCX -- scroll X
        case 0x47: // BGP -- background palette
        case 0x48: // OBP0 -- object palette 0
        case 0x49: // OBP1 -- object palette 1
        case 0x4A: // WY -- window Y
        case 0x4B: // WX -- window X
        case 0x51: // HDMA1 -- DMA source high
        case 0x52: // HDMA2 -- DMA source low
        case 0x53: // HDMA3 -- DMA destination high
        case 0x54: // HDMA4 -- DMA destination low
        case 0x56: // RP -- infrared communication
            ((uint8_t *)&io_state)[reg] = val;
            break;
        // Verwerfen
        case 0x01: // SB -- serial data (bus?)
            break;
        default:
            fprintf(stderr, "[io] Unbekanntes I/O-Register 0x%02X (möchte 0x%02X schreiben).\n", (int)reg, (int)val);
            exit(1);
    }
}
