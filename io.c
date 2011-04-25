#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "xgbc.h"

unsigned cumulative_cycles = 0;

extern void update_cpu(unsigned cycles);

static uint8_t keystates = 0;

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
    .wx     = 0x00
};

uint8_t ie = 0;

static bool timer_running = false, lcd_on = true;
static unsigned timer_div = 0;

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
                    // Müsste was zeichnen...
                    redrawed = true;
                }
            }
        }

        while (lcd_cycles >= 114)
        {
            lcd_cycles -= 114;

            hblank_start = true;
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
            break;
        case 0x4F: // VBK -- select VRAM bank
            io_state.vbk = val & 1;
            mem_select_vram_bank(io_state.vbk);
            break;
        case 0x70: // SVBK -- select WRAM bank
            val &= 7;
            io_state.svbk = val ? val : 1;
            mem_select_wram_bank(io_state.svbk);
            break;
        case 0xFF: // IE -- interrupt enable
            ie = val & 0x1F;
            update_cpu(0);
            break;
        // Einfach reinschreiben
        case 0x02: // SC -- serial control
        case 0x06: // TMA -- timer modulo
        case 0x42: // SCY -- scroll Y
        case 0x43: // SCX -- scroll X
        case 0x47: // BGP -- background palette
        case 0x48: // OBP0 -- object palette 0
        case 0x49: // OBP1 -- object palette 1
        case 0x4A: // WY -- window Y
        case 0x4B: // WX -- window X
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
