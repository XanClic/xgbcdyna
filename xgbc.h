#ifndef XGBC_H
#define XGBC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Schnelle und unsichere Version, ohne Statistiken
// #define FAST_VERSION
// Langsamere und sichere Version, ohne Statistiken
// #define SECURE_RELEASE_VERSION


#define MEM_BASE 0x40000000UL

// ought to be enough
#define CODE_BUFSZ 0x1000
#define CODE_EXITI (CODE_BUFSZ - 0x0100)

// Maximale Anzahl von Zyklen, die in einem übersetzten Block ausgeführt werden
// sollen
#define MAX_CYCLES 32

// Anzahl der Einträge pro Cache
#define CACHES_SIZE 0x400

// Flags nur aktualisieren, soweit dies sinnvoll ist -- dies betrifft vor allem
// das AF sowie bei einigen Operationen (wie SWAP und Rotationen) das ZF.
#define UNSAVE_FLAG_OPTIMIZATIONS
// Auch Code cachen, der sich außerhalb des ROMs befindet -- unsicher, aber da
// die meisten Spiele nur einen kleinen festen Code für OAM-DMA in den HRAM
// kopieren dürften, eine wirkungsvolle Optimierung.
#define UNSAVE_RAM_CACHING
// Den Speicher ab 0xF000 als lesbar mappen. Normalerweise befindet sich
// zwischen 0xF000 und 0xFDFF der gleiche Speicher wie zwischen 0xD000 und
// 0xDDFF, mit dieser Option nicht mehr. Offiziell darf auf diese Adressen
// (0xF000 bis 0xFDFF) sowieso nicht zugegriffen werden, deshalb sollte dies
// nicht schaden.
#define UNSAVE_RAM_MAPPING
// Wenn diese Option ausgeschaltet ist, werden die Registerwerte in der VM nur
// soweit nötig geladen und gespeichert. Bringt merkwürdigerweise aber
// Instabilität. Mit dieser Option werden immer alle Register (AF/EAX, BC/EBX,
// DE/ECX, HL/EDX und BP/ESP) geladen und gesichert.
#define REGISTER_EQUIVALENCE

// Anzahl von Zyklen, die pro HALT-Schritt simuliert werden sollen.
// 1: Sicher, korrekte Implementierung. Andererseits aber Verschwendung, da
//    letzten Endes sowieso bis zum nächsten IRQ gewartet werden muss.
// 32: Unsicherer.
// -1: (entsprechend ca. 80, maximal sinnvolle Anzahl): Nicht mehr korrekt,
//     geht bis zur nächsten LCD-Zeile. Dadurch sollten die IRQs immer noch zum
//     korrekten Zeitpunkt ausgelöst werden.
#define HALT_CYCLES -1

// Gibt nach Beendigung Informationen zum DBT-Cache (Dynamic Binary Translation)
// aus, betreffend Hits, Misses, etc.
#define CACHE_STATS
// Gibt nach Beendigung die Anzahl der abgefangenen Speicherzugriffsfehler aus.
#define SEGV_STATS
// Gibt nach Beendigung detaillierte Informationen darüber aus, wie viel Zeit
// welcher Teil des Programms in Anspruch genommen hat.
#define CYCLE_STATS


#ifdef FAST_VERSION
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
#define UNSAVE_FLAG_OPTIMIZATIONS
#endif
#ifndef UNSAVE_RAM_CACHING
#define UNSAVE_RAM_CACHING
#endif
#ifndef UNSAVE_RAM_MAPPING
#define UNSAVE_RAM_MAPPING
#endif
#ifndef REGISTER_EQUIVALENCE
#define REGISTER_EQUIVALENCE
#endif
#undef HALT_CYCLES
#define HALT_CYCLES -1
#ifdef CACHE_STATS
#undef CACHE_STATS
#endif
#ifdef SEGV_STATS
#undef SEGV_STATS
#endif
#ifdef CYCLE_STATS
#undef CYCLE_STATS
#endif
#endif

#ifdef SECURE_RELEASE_VERSION
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
#undef UNSAVE_FLAG_OPTIMIZATIONS
#endif
#ifdef UNSAVE_RAM_CACHING
#undef UNSAVE_RAM_CACHING
#endif
#ifdef UNSAVE_RAM_MAPPING
#undef UNSAVE_RAM_MAPPING
#endif
#ifndef REGISTER_EQUIVALENCE
#define REGISTER_EQUIVALENCE
#endif
#undef HALT_CYCLES
#define HALT_CYCLES 1
#ifdef CACHE_STATS
#undef CACHE_STATS
#endif
#ifdef SEGV_STATS
#undef SEGV_STATS
#endif
#ifdef CYCLE_STATS
#undef CYCLE_STATS
#endif
#endif


#define likely(x)   __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

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

#ifdef UNSAVE_RAM_MAPPING
#define io_state (*io_state)
#endif

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

#ifdef VK_A
#undef VK_A
#endif
#ifdef VK_B
#undef VK_B
#endif
#ifdef VK_SELECT
#undef VK_SELECT
#endif
#ifdef VK_START
#undef VK_START
#endif
#ifdef VK_RIGHT
#undef VK_RIGHT
#endif
#ifdef VK_LEFT
#undef VK_LEFT
#endif
#ifdef VK_UP
#undef VK_UP
#endif
#ifdef VK_DOWN
#undef VK_DOWN
#endif

#define VK_A      KEY_A
#define VK_B      KEY_B
#define VK_SELECT KEY_SELECT
#define VK_START  KEY_START
#define VK_RIGHT  (KEY_RIGHT << 4)
#define VK_LEFT   (KEY_LEFT  << 4)
#define VK_UP     (KEY_UP    << 4)
#define VK_DOWN   (KEY_DOWN  << 4)

void begin_execution(void);
bool check_cartridge(FILE *fp);
void init_memory(void);
void install_segv_handler(void);
void init_video(unsigned multiplier);
uint32_t determine_tsc_resolution(void);

void change_banked_rom(unsigned new_bank);

uint8_t hmem_read8(unsigned off);
void hmem_write8(unsigned off, uint8_t val);
void rom_write8(unsigned off, uint8_t val);
uint8_t io_read(uint8_t reg);
void io_write(uint8_t reg, uint8_t val);
uint8_t eram_read8(unsigned off);
void eram_write8(unsigned off, uint8_t val);
void update_cpu(unsigned cycles);
void skip_until_hblank(void);
void inc_timer(unsigned cycles);
void handle_input_events(void);
void draw_line(unsigned line);

void mem_select_wram_bank(unsigned bank);
void mem_select_vram_bank(unsigned bank);

static inline uint8_t MEM8(uint16_t addr)
{
#ifndef UNSAVE_RAM_MAPPING
    if (likely(addr < 0xF000))
        return ((uint8_t *)(uintptr_t)MEM_BASE)[addr];
    return hmem_read8(addr & 0x0FFF);
#else
    return ((uint8_t *)(uintptr_t)MEM_BASE)[addr];
#endif
}

static inline uint16_t MEM16(uint16_t addr)
{
    if (likely(addr < 0xF000))
        return *(uint16_t *)((uintptr_t)MEM_BASE + addr);
    return hmem_read8(addr & 0x0FFF) | ((unsigned)hmem_read8((addr & 0x0FFF) + 1) << 8);
}

static inline void mem_write16(uint16_t addr, uint16_t val)
{
    if (likely(addr < 0xF000))
        *(uint16_t *)((uintptr_t)MEM_BASE + addr) = val;
    else
    {
        hmem_write8(addr - 0xF000, val);
        hmem_write8(addr - 0xEFFF, val >> 8);
    }
}

#endif
