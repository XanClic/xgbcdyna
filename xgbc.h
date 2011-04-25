#ifndef XGBC_H
#define XGBC_H

#include <stdint.h>

#define MEM_BASE 0x40000000UL

#define MEM8(o) (((uint8_t *)(uintptr_t)MEM_BASE)[o])
#define MEM16(o) (*(uint16_t *)((uintptr_t)MEM_BASE + o))

#define likely(x)     __builtin_expect((x), 1)
#define unlikely(x)   __builtin_expect((x), 0)

struct io
{
    uint8_t p1;
    uint8_t sb;
    uint8_t sc;
    uint8_t rsvd1;
    uint8_t div;
    uint8_t tima;
    uint8_t tma;
    uint8_t tac;
    uint8_t rsvd2[7];
    uint8_t int_flag;
    uint8_t nr10;
    uint8_t nr11;
    uint8_t nr12;
    uint8_t nr13;
    uint8_t nr14;
    uint8_t rsvd3;
    uint8_t nr21;
    uint8_t nr22;
    uint8_t nr23;
    uint8_t nr24;
    uint8_t nr30;
    uint8_t nr31;
    uint8_t nr32;
    uint8_t nr33;
    uint8_t nr34;
    uint8_t rsvd4;
    uint8_t nr41;
    uint8_t nr42;
    uint8_t nr43;
    uint8_t nr44;
    uint8_t nr50;
    uint8_t nr51;
    uint8_t nr52;
    uint8_t rsvd5[9];
    uint8_t wave_pat[16];
    uint8_t lcdc;
    uint8_t stat;
    uint8_t scy;
    uint8_t scx;
    uint8_t ly;
    uint8_t lyc;
    uint8_t dma;
    uint8_t bgp;
    uint8_t obp0;
    uint8_t obp1;
    uint8_t wy;
    uint8_t wx;
    uint8_t rsvd6;
    uint8_t key1;
    uint8_t rsvd7;
    uint8_t vbk;
    uint8_t rsvd8;
    uint8_t hdma1, hdma2, hdma3, hdma4, hdma5;
    uint8_t rp;
    uint8_t rsvd9[17];
    uint8_t bcps;
    uint8_t bcpd;
    uint8_t ocps;
    uint8_t ocpd;
    uint8_t rsvd10[4];
    uint8_t svbk;
    uint8_t rsvd11[15];
} __attribute__((packed));

#define INT_P10_P13   (1 << 4)
#define INT_SERIAL    (1 << 3)
#define INT_TIMER     (1 << 2)
#define INT_LCDC_STAT (1 << 1)
#define INT_VBLANK    (1 << 0)

#define REG_AF 0x01
#define REG_BC 0x02
#define REG_DE 0x04
#define REG_HL 0x08
#define REG_SP 0x10
#define REG_IP 0x20

#define KEY_A      (1 << 0)
#define KEY_B      (1 << 1)
#define KEY_SELECT (1 << 2)
#define KEY_START  (1 << 3)

#define KEY_RIGHT  (1 << 0)
#define KEY_LEFT   (1 << 1)
#define KEY_UP     (1 << 2)
#define KEY_DOWN   (1 << 3)

#define KEY_DIR (1 << 4)
#define KEY_OTH (1 << 5)

void begin_execution(void);
void init_memory(void);
void install_segv_handler(void);

uint8_t hmem_read8(unsigned off);
void hmem_write8(unsigned off, uint8_t val);
uint8_t io_read(uint8_t reg);
void io_write(uint8_t reg, uint8_t val);
void inc_timer(unsigned cycles);

void mem_select_wram_bank(unsigned bank);
void mem_select_vram_bank(unsigned bank);

#endif
