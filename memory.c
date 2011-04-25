#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "xgbc.h"

int romfd, vramfd, wramfd;

uint8_t *oam, *hram;

char path_buf[PATH_MAX];

static void unlink_shms(int status, void *arg)
{
    (void)status;
    (void)arg;

    close(romfd);
    close(vramfd);
    close(wramfd);

    shm_unlink("/xgbcdyna-vram");
    shm_unlink("/xgbcdyna-wram");
}

static void fd_name(int fd, char *buffer, size_t sz)
{
    char tbuf[32];
    sprintf(tbuf, "/proc/self/fd/%i", fd);

    ssize_t ret = readlink(tbuf, buffer, sz - 1);
    if (ret < 0)
        ret = 0;

    buffer[ret] = 0;
}

static void mmap_file(uint16_t vaddr, uint16_t sz, int prot, int fd, off_t offset)
{
    if (mmap((void *)(MEM_BASE + vaddr), sz, prot, MAP_SHARED | MAP_FIXED, fd, offset) != (void *)(MEM_BASE + vaddr))
    {
        fprintf(stderr, "[mmap_file] Konnte Adresse 0x%04X nicht mappen: %s\n", (unsigned)vaddr, strerror(errno));
        exit(1);
    }

    fd_name(fd, path_buf, PATH_MAX);
    printf("[%04X – %04X]: %c%c %s:%08X\n", (unsigned)vaddr, (unsigned)vaddr + sz - 1, (prot & PROT_READ) ? 'r' : '-', (prot & PROT_WRITE) ? 'w' : '-', path_buf, (unsigned)offset);
}

static void mmap_anon(uint16_t vaddr, uint16_t sz, int prot)
{
    if (mmap((void *)(MEM_BASE + vaddr), sz, prot, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0) != (void *)(MEM_BASE + vaddr))
    {
        fprintf(stderr, "[mmap_anon] Konnte Adresse 0x%04X nicht mappen: %s\n", (unsigned)vaddr, strerror(errno));
        exit(1);
    }

    printf("[%04X – %04X]: %c%c (anon):00000000\n", (unsigned)vaddr, (unsigned)vaddr + sz - 1, (prot & PROT_READ) ? 'r' : '-', (prot & PROT_WRITE) ? 'w' : '-');
}

static void munmap_stuff(uint16_t vaddr, uint16_t sz)
{
    munmap((void *)(MEM_BASE + vaddr), sz);
    printf("[%04X – %04X]: -- (none):00000000\n", (unsigned)vaddr, (unsigned)vaddr + sz - 1);
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
    mmap_anon(0xA000, 0x2000, PROT_READ | PROT_WRITE);
    mmap_file(0xC000, 0x1000, PROT_READ | PROT_WRITE, wramfd, 0x0000);
    mmap_file(0xD000, 0x1000, PROT_READ | PROT_WRITE, wramfd, 0x1000);
    mmap_file(0xE000, 0x1000, PROT_READ | PROT_WRITE, wramfd, 0x0000);
    munmap_stuff(0xF000, 0x1000);

    oam = malloc(256);
    hram = malloc(128);
}

void mem_select_wram_bank(unsigned bank)
{
    mmap_file(0xD000, 0x1000, PROT_READ | PROT_WRITE, wramfd, bank * 0x1000);
}

void mem_select_vram_bank(unsigned bank)
{
    mmap_file(0x8000, 0x2000, PROT_READ | PROT_WRITE, vramfd, bank * 0x2000);
}

void hmem_write8(unsigned off, uint8_t val)
{
    if (unlikely(off < 0xE00))
        MEM8(0xD000 + off) = val;
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
        return MEM8(0xD000 + off);
    else if (off < 0xF00)
        return oam[off & 0xFF];
    else if ((off < 0xF80) || (off == 0xFFF))
        return io_read(off & 0xFF);
    else
        return hram[off & 0x7F];
}
