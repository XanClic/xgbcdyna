#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "xgbc.h"

// #define MMAP_ECHO

int romfd, ramfd = -1, vramfd, wramfd;
#ifdef UNSAVE_RAM_MAPPING
int hmemfd;
#endif
static int cramb = 0, cromb = 1;
static bool ram_enabled = false, ram_mapped;
static time_t latched_time;

extern unsigned mbc;

uint8_t *oam, *hram, *full_vidram;

#ifdef UNSAVE_RAM_MAPPING
extern struct io io_state;
extern uint8_t ie;
#endif

#ifdef MMAP_ECHO
char path_buf[PATH_MAX];
#endif

static void unlink_shms(int status, void *arg)
{
    (void)status;
    (void)arg;

    printf("Dumpe Speicher nach /tmp/xgbcdyna-memory...\n");
    FILE *md = fopen("/tmp/xgbcdyna-memory", "wb");
    fwrite((void *)(uintptr_t)MEM_BASE, 0xA000, 1, md);
    if (ram_mapped)
        fwrite((void *)((uintptr_t)MEM_BASE + 0xA000), 0x2000, 1, md);
    else
        fseek(md, 0x2000, SEEK_CUR);
    fwrite((void *)((uintptr_t)MEM_BASE + 0xC000), 0x3000, 1, md);
    fwrite((void *)((uintptr_t)MEM_BASE + 0xD000), 0x0E00, 1, md);
    for (size_t i = 0x0E00; i < 0x1000; i++)
        fputc(hmem_read8(i), md);
    fclose(md);

    if (ramfd >= 0)
        close(ramfd);

    close(romfd);
    close(vramfd);
    close(wramfd);

#ifdef UNSAVE_RAM_MAPPING
    close(hmemfd);
    shm_unlink("/xgbcdyna-hmem");
#endif

    shm_unlink("/xgbcdyna-vram");
    shm_unlink("/xgbcdyna-wram");
}

#ifdef MMAP_ECHO
static void fd_name(int fd, char *buffer, size_t sz)
{
    char tbuf[32];
    sprintf(tbuf, "/proc/self/fd/%i", fd);

    ssize_t ret = readlink(tbuf, buffer, sz - 1);
    if (ret < 0)
        ret = 0;

    buffer[ret] = 0;
}
#endif

static void mmap_file(uint16_t vaddr, uint16_t sz, int prot, int fd, off_t offset)
{
    if (mmap((void *)(MEM_BASE + vaddr), sz, prot, MAP_SHARED | MAP_FIXED, fd, offset) != (void *)(MEM_BASE + vaddr))
    {
        fprintf(stderr, "[mmap_file] Konnte Adresse 0x%04X nicht mappen: %s\n", (unsigned)vaddr, strerror(errno));
        exit(1);
    }

#ifdef MMAP_ECHO
    fd_name(fd, path_buf, PATH_MAX);
    printf("[%04X – %04X]: %c%c %s:%08X\n", (unsigned)vaddr, (unsigned)vaddr + sz - 1, (prot & PROT_READ) ? 'r' : '-', (prot & PROT_WRITE) ? 'w' : '-', path_buf, (unsigned)offset);
#endif
}

static void munmap_stuff(uint16_t vaddr, uint16_t sz)
{
    munmap((void *)(MEM_BASE + vaddr), sz);
#ifdef MMAP_ECHO
    printf("[%04X – %04X]: -- (none):00000000\n", (unsigned)vaddr, (unsigned)vaddr + sz - 1);
#endif
}

void init_memory(void)
{
    vramfd = shm_open("/xgbcdyna-vram", O_RDWR | O_CREAT | O_TRUNC, 0644);
    wramfd = shm_open("/xgbcdyna-wram", O_RDWR | O_CREAT | O_TRUNC, 0644);

    ftruncate(vramfd, 0x4000);
    ftruncate(wramfd, 0x8000);

    on_exit(&unlink_shms, NULL);

    mmap_file(0x0000, 0x4000, PROT_READ,              romfd,  0x0000);
    mmap_file(0x4000, 0x4000, PROT_READ,              romfd,  0x4000);
    mmap_file(0x8000, 0x2000, PROT_READ | PROT_WRITE, vramfd, 0x0000);
    mmap_file(0xC000, 0x1000, PROT_READ | PROT_WRITE, wramfd, 0x0000);
    mmap_file(0xD000, 0x1000, PROT_READ | PROT_WRITE, wramfd, 0x1000);
    mmap_file(0xE000, 0x1000, PROT_READ | PROT_WRITE, wramfd, 0x0000);

    if (ramfd == -1)
        ram_mapped = false;
    else
    {
        mmap_file(0xA000, 0x2000, PROT_READ | PROT_WRITE, ramfd, 0x0000);
        ram_mapped = true;
    }

#ifndef UNSAVE_RAM_MAPPING
    oam = malloc(256);
    hram = malloc(128);
    munmap_stuff(0xF000, 0x1000);
#else
    hmemfd = shm_open("/xgbdyna-hmem", O_RDWR | O_CREAT | O_TRUNC, 0644);
    ftruncate(hmemfd, 0x1000);
    mmap_file(0xF000, 0x1000, PROT_READ, hmemfd, 0x0000);
    void *hmapping = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, hmemfd, 0x0000);
    oam = (void *)((uintptr_t)hmapping + 0xE00);
#undef io_state
    io_state = (struct io *)((uintptr_t)hmapping + 0xF00);
    hram = (void *)((uintptr_t)hmapping + 0xF80);
#undef ie
    ie = (uint8_t *)((uintptr_t)hmapping + 0xFFF);
#endif

    full_vidram = mmap(NULL, 0x4000, PROT_READ | PROT_WRITE, MAP_SHARED, vramfd, 0x0000);
}

void mem_select_wram_bank(unsigned bank)
{
    mmap_file(0xD000, 0x1000, PROT_READ | PROT_WRITE, wramfd, bank * 0x1000);
}

void mem_select_vram_bank(unsigned bank)
{
    mmap_file(0x8000, 0x2000, PROT_READ | PROT_WRITE, vramfd, bank * 0x2000);
}

void mem_select_rom_bank(unsigned bank)
{
    mmap_file(0x4000, 0x4000, PROT_READ, romfd, bank * 0x4000);
}

void mem_select_ram_bank(unsigned bank)
{
    mmap_file(0xA000, 0x2000, PROT_READ | PROT_WRITE, ramfd, bank * 0x2000);
    ram_mapped = true;
}

void unselect_ram(void)
{
    if (ram_mapped)
    {
        munmap_stuff(0xA000, 0x2000);
        ram_mapped = false;
    }
}

void hmem_write8(unsigned off, uint8_t val)
{
    if (unlikely(off < 0xE00))
        ((uint8_t *)(uintptr_t)MEM_BASE)[0xD000 + off] = val;
    else if (off < 0xF00)
        oam[off & 0xFF] = val;
    else if ((off < 0xF80) || (off == 0xFFF))
        io_write(off & 0xFF, val);
    else
        hram[off & 0x7F] = val;
}

uint8_t hmem_read8(unsigned off)
{
    if (unlikely(off < 0xE00))
        return ((uint8_t *)(uintptr_t)MEM_BASE)[0xD000 + off];
    else if (off < 0xF00)
        return oam[off & 0xFF];
    else if ((off < 0xF80) || (off == 0xFFF))
        return io_read(off & 0xFF);
    else
        return hram[off & 0x7F];
}

static void mbc3_rom_write(unsigned off, uint8_t val)
{
    static unsigned latched_state = 1;

    switch (off & 0x6000)
    {
        case 0x0000:
            if (!val)
            {
                unselect_ram();
                ram_enabled = false;
            }
            else if (likely(val == 0x0A))
            {
                ram_enabled = true;
                if (cramb >= 4)
                    unselect_ram();
                else if (!ram_mapped)
                    mem_select_ram_bank(cramb);
            }
            break;
        case 0x2000:
            cromb = val ? val : 1;
            mem_select_rom_bank(cromb);
            change_banked_rom(cromb);
            break;
        case 0x4000:
            if (val >= 8)
            {
                cramb = val;
                unselect_ram();
            }
            else if (likely(val < 4) && likely(cramb != val))
            {
                cramb = val;
                if (ram_enabled)
                    mem_select_ram_bank(cramb);
                else
                    unselect_ram();
            }
            break;
        case 0x6000:
            if (!val)
                latched_state = 0;
            else if (likely(val == 1) && likely(!latched_state))
            {
                latched_state = 1;
                latched_time = time(NULL);
            }
            break;
    }
}

static void mbc1_rom_write(unsigned off, uint8_t val)
{
    static int mode = 0;

    switch (off & 0x6000)
    {
        case 0x0000:
            if (!val)
            {
                unselect_ram();
                ram_enabled = false;
            }
            else if (likely(val == 0x0A))
            {
                ram_enabled = true;
                if (!ram_mapped)
                    mem_select_ram_bank(cramb);
            }
            break;
        case 0x2000:
            cromb = val & 0x1F;
            mem_select_rom_bank(cromb);
            change_banked_rom(cromb);
            break;
        case 0x4000:
            if (mode)
            {
                if ((val & 0x03) != cramb)
                {
                    cramb = val & 0x03;
                    if (ram_enabled && !ram_mapped)
                        mem_select_ram_bank(cramb);
                }
            }
            else
            {
                cromb = (cromb & 0x1F) | ((val & 0x03) << 5);
                mem_select_rom_bank(cromb);
                change_banked_rom(cromb);
            }
            break;
        case 0x6000:
            mode = val & 1;
            break;
    }
}

static void mbc3_ram_write(unsigned off, uint8_t val)
{
    if (unlikely(cramb < 8) || unlikely(cramb > 12))
    {
        fprintf(stderr, "[mbc3] Fehlgeschlagener RAM-Schreibzugriff (0x%02X nach 0x%04X) auf Bank %i.\n", val, off, cramb);
        exit(1);
    }

    if (unlikely(!ram_enabled))
    {
        fprintf(stderr, "[mbc3] Versuch, nicht aktivierten RAM zu beschreiben.\n");
        return;
    }

#if 0
    fprintf(stderr, "[mbc3] Ignoriere geschriebenen Zeitwert 0x%02X ins Register %i.\n", val, cramb);
#endif
}

static uint8_t mbc3_ram_read(unsigned off)
{
    if (unlikely(!ram_enabled))
    {
        fprintf(stderr, "[mbc3] Versuch, von nicht aktiviertem RAM zu lesen.\n");
        return 0;
    }

    struct tm *tm = gmtime(&latched_time);

    switch (cramb)
    {
        case 8:
            return tm->tm_sec;
        case 9:
            return tm->tm_min;
        case 10:
            return tm->tm_hour;
        case 11:
            return tm->tm_yday; // FIXME oder so
        case 12:
            return tm->tm_yday >> 8; // ebenso
        default:
            fprintf(stderr, "[mbc3] Fehlgeschlagener RAM-Lesezugriff von 0x%04X auf Bank %i.\n", off, cramb);
            exit(1);
    }
}

static void mbc5_rom_write(unsigned off, uint8_t val)
{
    switch (off & 0x6000)
    {
        case 0x0000:
            if (!val)
            {
                unselect_ram();
                ram_enabled = false;
            }
            else if (likely(val == 0x0A))
            {
                ram_enabled = true;
                if (!ram_mapped)
                    mem_select_ram_bank(cramb);
            }
            break;

        case 0x2000:
            cromb = (off & 0x1000)
                  ? (cromb & 0x0FF) | ((val & 1) << 8)
                  : (cromb & 0x100) | val;

            mem_select_rom_bank(cromb);
            change_banked_rom(cromb);
            break;

        case 0x4000:
            cramb = val & 0xF;
            if (ram_enabled)
                mem_select_ram_bank(cramb);
            else
                unselect_ram();
            break;

#if 0
        default:
            fprintf(stderr, "[mbc5] Fehlgeschlagener ROM-Schreibzugriff 0x%02X auf 0x%04X.\n", val, off);
            exit(1);
#endif
    }
}

static void (*const rom_write[6])(unsigned off, uint8_t val) = {
    [1] = &mbc1_rom_write,
    [3] = &mbc3_rom_write,
    [5] = &mbc5_rom_write
};

static uint8_t (*const ram_read[6])(unsigned off) = {
    [3] = &mbc3_ram_read
};

static void (*const ram_write[6])(unsigned off, uint8_t val) = {
    [3] = &mbc3_ram_write
};

void rom_write8(unsigned off, uint8_t val)
{
    if (unlikely(rom_write[mbc] == NULL))
    {
        fprintf(stderr, "[mem] ROM-Schreibzugriff (0x%02X) auf 0x%04X.\n", val, off);
        exit(1);
    }

    rom_write[mbc](off, val);
}

uint8_t eram_read8(unsigned off)
{
    if (unlikely(ram_read[mbc] == NULL))
    {
        fprintf(stderr, "[mem] RAM-Lesezugriff ohne RAM von 0x%04X.\n", off);
        exit(1);
    }

    return ram_read[mbc](off);
}

void eram_write8(unsigned off, uint8_t val)
{
    if (unlikely(ram_write[mbc] == NULL))
    {
        fprintf(stderr, "[mem] RAM-Schreibzugriff ohne RAM von 0x%02X auf 0x%04X.\n", val, off);
        exit(1);
    }

    ram_write[mbc](off, val);
}
