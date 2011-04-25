#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "xgbc.h"

unsigned ltu_counter = 0;

uint32_t vm_fa, vm_bc, vm_de, vm_hl, vm_sp, vm_ip, cycles_gone = 0;
bool ime = false;

extern const uint8_t flag_table[256], reverse_flag_table[256];

#define vm_af (((vm_fa & 0xFF) << 8) | ((vm_fa & 0x4000) ? 0x80 : 0x00) | ((vm_fa & 0x1000) ? 0x20 : 0x00) | ((vm_fa & 0x0100) ? 0x10 : 0x00))

typedef void (*exec_unit_t)(void);

struct dynarec_cache
{
    bool in_use;

    uint16_t ip;
    exec_unit_t code;
};

static struct dynarec_cache base_rom[0x100];
static struct dynarec_cache banked_rom[0x100];

const unsigned cycles[256] = {
    1, 3, 2, 2, 1, 1, 2, 1, 5, 2, 2, 2, 1, 1, 2, 1,
    1, 3, 2, 2, 1, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 1,
    2, 3, 2, 2, 1, 1, 2, 1, 2, 2, 2, 2, 1, 1, 2, 1,
    2, 3, 2, 2, 3, 3, 3, 1, 2, 2, 2, 2, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    2, 3, 3, 4, 3, 4, 2, 4, 2, 4, 3, 2, 3, 6, 2, 4,
    2, 3, 3, 0, 3, 4, 2, 4, 2, 4, 3, 0, 3, 0, 2, 4,
    3, 3, 2, 0, 0, 4, 2, 4, 4, 1, 4, 0, 0, 0, 2, 4,
    3, 3, 2, 1, 0, 4, 2, 4, 3, 2, 4, 1, 0, 0, 2, 4
};

const unsigned cycles0xCB[256] = {
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2
};

const unsigned regs[256] = {
 // 0x00  0x01  0x02  0x03  0x04  0x05  0x06  0x07  0x08  0x09  0x0A  0x0B  0x0C  0x0D  0x0E  0x0F
    0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x10, 0x0A, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, // 0x00
    0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, // 0x10
    0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, // 0x20
    0x00, 0x10, 0x08, 0x10, 0x08, 0x08, 0x08, 0x00, 0x00, 0x18, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, // 0x30
    0x02, 0x02, 0x06, 0x06, 0x0A, 0x0A, 0x0A, 0x02, 0x02, 0x02, 0x06, 0x06, 0x0A, 0x0A, 0x0A, 0x02, // 0x40
    0x06, 0x06, 0x04, 0x04, 0x0C, 0x0C, 0x0C, 0x04, 0x06, 0x06, 0x04, 0x04, 0x0C, 0x0C, 0x0C, 0x04, // 0x50
    0x0A, 0x0A, 0x0C, 0x0C, 0x08, 0x08, 0x08, 0x08, 0x0A, 0x0A, 0x0C, 0x0C, 0x08, 0x08, 0x08, 0x08, // 0x60
    0x0A, 0x0A, 0x0C, 0x0C, 0x08, 0x08, 0x00, 0x08, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0x70
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0x80
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0x90
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0xA0
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0xB0
    0x20, 0x12, 0x20, 0x20, 0x30, 0x12, 0x00, 0x30, 0x30, 0x30, 0x20, 0x3E, 0x30, 0x30, 0x00, 0x30, // 0xC0
    0x30, 0x14, 0x20, 0x00, 0x30, 0x14, 0x00, 0x30, 0x30, 0x30, 0x20, 0x00, 0x30, 0x00, 0x00, 0x30, // 0xD0
    0x00, 0x18, 0x02, 0x00, 0x00, 0x18, 0x00, 0x30, 0x10, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, // 0xE0
    0x00, 0x10, 0x02, 0x00, 0x00, 0x10, 0x00, 0x30, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30  // 0xF0
};

const unsigned regs0xCB[256] = {
 // 0x00  0x01  0x02  0x03  0x04  0x05  0x06  0x07  0x08  0x09  0x0A  0x0B  0x0C  0x0D  0x0E  0x0F
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0x00
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0x10
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0x20
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0x30
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0x40
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0x50
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0x60
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0x70
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0x80
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0x90
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0xA0
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0xB0
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0xC0
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0xD0
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 0xE0
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00  // 0xF0
};

const unsigned break_on[256] = {
 // 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 2
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 3
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6
    0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 7
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B
    0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, // C
    0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, // D
    1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, // E
    1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1  // F
};

const unsigned compilability[256] = {
 // 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 1
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 2
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 3
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 4
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 5
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 6
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 7
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 8
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 9
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // A
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // B
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // C
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // D
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // E
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1  // F
};

static unsigned op_ops[256] = {
 // 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    0, 2, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 1, 0, // 0
    0, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, // 1
    1, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, // 2
    1, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, // 3
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 7
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B
    0, 0, 2, 2, 2, 0, 1, 0, 0, 0, 2, 0, 2, 2, 1, 0, // C
    0, 0, 2, 0, 2, 0, 1, 0, 0, 0, 2, 0, 2, 0, 1, 0, // D
    1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 2, 0, 0, 0, 1, 0, // E
    1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 2, 0, 0, 0, 1, 0  // F
};

static void exit_vm(uint8_t *opbuf, size_t *i, size_t ei, uint16_t exit_ip, uint8_t exit_cycles)
{
    opbuf[(*i)++] = 0x66; // mov word [],ip
    opbuf[(*i)++] = 0xC7;
    opbuf[(*i)++] = 0x05;
    *((uint32_t *)&opbuf[*i]) = (uintptr_t)&vm_ip;
    *i += 4;
    opbuf[(*i)++] = exit_ip & 0xFF;
    opbuf[(*i)++] = exit_ip >> 8;
    opbuf[(*i)++] = 0xC6; // mov byte [],exit_cycles
    opbuf[(*i)++] = 0x05;
    *((uint32_t *)&opbuf[*i]) = (uintptr_t)&cycles_gone;
    *i += 4;
    opbuf[(*i)++] = exit_cycles;
    opbuf[(*i)++] = 0xE9; // jmp dword
    *((uint32_t *)&opbuf[*i]) = (uint32_t)(ei - (*i + 4));
    *i += 4;
}

static void exit_vm_by_ret(uint8_t *opbuf, size_t *i, size_t ei, uint8_t exit_cycles)
{
    opbuf[(*i)++] = 0x9F; // lahf
    opbuf[(*i)++] = 0x50; // push eax
    opbuf[(*i)++] = 0x66; // mov ax,[ebp]
    opbuf[(*i)++] = 0x8B;
    opbuf[(*i)++] = 0x45;
    opbuf[(*i)++] = 0x00;
    opbuf[(*i)++] = 0x66; // add bp,2
    opbuf[(*i)++] = 0x83;
    opbuf[(*i)++] = 0xC5;
    opbuf[(*i)++] = 0x02;
    opbuf[(*i)++] = 0x66; // mov word [],ax
    opbuf[(*i)++] = 0xA3;
    *((uint32_t *)&opbuf[*i]) = (uintptr_t)&vm_ip;
    *i += 4;
    opbuf[(*i)++] = 0x58; // pop eax
    opbuf[(*i)++] = 0x9E; // sahf
    opbuf[(*i)++] = 0xC6; // mov byte [],exit_cycles
    opbuf[(*i)++] = 0x05;
    *((uint32_t *)&opbuf[*i]) = (uintptr_t)&cycles_gone;
    *i += 4;
    opbuf[(*i)++] = exit_cycles;
    opbuf[(*i)++] = 0xE9; // jmp dword
    *((uint32_t *)&opbuf[*i]) = (uint32_t)(ei - (*i + 4));
    *i += 4;
}

static exec_unit_t dynarec(uint16_t ip)
{
    uint16_t ipwas = ip;

    uint8_t *c = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    size_t i = 0, cycle_counter = 0;

    c[i++] = 0x53; // push ebx
    c[i++] = 0x55; // push ebp

    uint8_t op, pop = 0;

    unsigned need_regs = REG_AF, do_break = 0;

    while (!do_break)
    {
        op = MEM8(ip++);

        if (likely(op != 0xCB))
        {
            need_regs |= regs[op];
            if (compilability[op])
                cycle_counter += cycles[op];
            do_break = break_on[op];

            switch (op)
            {
                case 0x18: // JR
                    ip += (int8_t)MEM8(ip) + 1;
                    break;
                case 0xC3: // JP
                    ip = MEM16(ip);
                    break;
                case 0xCD: // CALL
                    ip = MEM16(ip);
                    break;
                default:
                    ip += op_ops[op];
            }
        }
        else
        {
            pop = MEM8(ip++);
            need_regs |= regs0xCB[pop];
            cycle_counter += cycles0xCB[pop];
        }

        if (cycle_counter >= 128) // Wahllos...
            do_break = 1;
    }

    if (need_regs & REG_AF)
    {
        c[i++] = 0xA1; // mov eax,[]
        *((uint32_t *)&c[i]) = (uintptr_t)&vm_fa;
        i += 4;
        c[i++] = 0x9E; // sahf
    }
    if (need_regs & REG_BC)
    {
        c[i++] = 0x8B; // mov ebx,[]
        c[i++] = 0x1D;
        *((uint32_t *)&c[i]) = (uintptr_t)&vm_bc;
        i += 4;
    }
    if (need_regs & REG_DE)
    {
        c[i++] = 0x8B; // mov ecx,[]
        c[i++] = 0x0D;
        *((uint32_t *)&c[i]) = (uintptr_t)&vm_de;
        i += 4;
    }
    if (need_regs & REG_HL)
    {
        c[i++] = 0x8B; // mov edx,[]
        c[i++] = 0x15;
        *((uint32_t *)&c[i]) = (uintptr_t)&vm_hl;
        i += 4;
    }
    if (need_regs & REG_SP)
    {
        c[i++] = 0x8B; // mov ebp,[]
        c[i++] = 0x2D;
        *((uint32_t *)&c[i]) = (uintptr_t)&vm_sp;
        i += 4;
    }


    size_t ei = 4096;
    c[--ei] = 0xC3; // ret
    c[--ei] = 0x5B; // pop ebx
    c[--ei] = 0x5D; // pop ebp

    if (need_regs & REG_AF)
    {
        ei -= 4;
        *((uint32_t *)&c[ei]) = (uintptr_t)&vm_fa;
        c[--ei] = 0xA3; // mov [],eax
        c[--ei] = 0x9F; // lahf
    }
    if (need_regs & REG_BC)
    {
        ei -= 4;
        *((uint32_t *)&c[ei]) = (uintptr_t)&vm_bc;
        c[--ei] = 0x1D; // mov [],ebx
        c[--ei] = 0x89;
    }
    if (need_regs & REG_DE)
    {
        ei -= 4;
        *((uint32_t *)&c[ei]) = (uintptr_t)&vm_de;
        c[--ei] = 0x0D; // mov [],ecx
        c[--ei] = 0x89;
    }
    if (need_regs & REG_HL)
    {
        ei -= 4;
        *((uint32_t *)&c[ei]) = (uintptr_t)&vm_hl;
        c[--ei] = 0x15; // mov [],edx
        c[--ei] = 0x89;
    }
    if (need_regs & REG_SP)
    {
        ei -= 4;
        *((uint32_t *)&c[ei]) = (uintptr_t)&vm_sp;
        c[--ei] = 0x2D; // mov [],ebp
        c[--ei] = 0x89;
    }


    ip = ipwas;

    cycle_counter = 0;
    do_break = 0;

    while (!do_break)
    {
        op = MEM8(ip++);

        if (likely(op != 0xCB))
        {
            if (compilability[op])
                cycle_counter += cycles[op];
            do_break = break_on[op];

            switch (op)
            {
                case 0x00: // NOP
                    break;
                case 0x01: // LD BC,nn
                    c[i++] = 0x66; // mov bx,nn
                    c[i++] = 0xBB;
                    c[i++] = MEM8(ip++);
                    c[i++] = MEM8(ip++);
                    break;
                case 0x04: // INC B
                    c[i++] = 0xFE; // inc bh
                    c[i++] = 0xC7;
                    break;
                case 0x05: // DEC B
                    c[i++] = 0xFE; // dec bh
                    c[i++] = 0xCF;
                    break;
                case 0x06: // LD B,n
                    c[i++] = 0xB7; // mov bh,n
                    c[i++] = MEM8(ip++);
                    break;
                case 0x0B: // DEC BC
                    c[i++] = 0x9F; // lahf
                    c[i++] = 0x66; // dec bx
                    c[i++] = 0x4B;
                    c[i++] = 0x9E; // sahf
                    break;
                case 0x0C: // INC C
                    c[i++] = 0xFE; // inc bl
                    c[i++] = 0xC3;
                    break;
                case 0x0D: // DEC C
                    c[i++] = 0xFE; // dec bl
                    c[i++] = 0xCB;
                    break;
                case 0x18: // JR
                    ip += (int8_t)MEM8(ip) + 1;
                    break;
                case 0x20: // JRNZ
                    c[i++] = 0x74; // jz byte
                    c[i++] = 0x15;
                    exit_vm(c, &i, ei, ip + (int8_t)MEM8(ip) + 1, cycle_counter);
                    ip++;
                    break;
                case 0x21: // LD HL,nn
                    c[i++] = 0x66; // mov dx,nn
                    c[i++] = 0xBA;
                    c[i++] = MEM8(ip++);
                    c[i++] = MEM8(ip++);
                    break;
                case 0x22: // LDI (HL),A
                    c[i++] = 0x88; // mov [edx],al
                    c[i++] = 0x02;
                    c[i++] = 0x9F; // lahf
                    c[i++] = 0x66; // inc dx
                    c[i++] = 0x42;
                    c[i++] = 0x9E; // sahf
                    break;
                case 0x23: // INC HL
                    c[i++] = 0x9F; // lahf
                    c[i++] = 0x66; // inc dx
                    c[i++] = 0x42;
                    c[i++] = 0x9E; // sahf
                    break;
                case 0x28: // JRZ
                    c[i++] = 0x75; // jnz byte
                    c[i++] = 0x15;
                    exit_vm(c, &i, ei, ip + (int8_t)MEM8(ip) + 1, cycle_counter);
                    ip++;
                    break;
                case 0x30: // JRNC
                    c[i++] = 0x72; // jc byte
                    c[i++] = 0x15;
                    exit_vm(c, &i, ei, ip + (int8_t)MEM8(ip) + 1, cycle_counter);
                    ip++;
                    break;
                case 0x31: // LD SP,nn
                    c[i++] = 0x66; // mov bp,nn
                    c[i++] = 0xBD;
                    c[i++] = MEM8(ip++);
                    c[i++] = MEM8(ip++);
                    break;
                case 0x36: // LD (HL),nn
                    c[i++] = 0xC6; // mov byte [edx],nn
                    c[i++] = 0x02;
                    c[i++] = MEM8(ip++);
                    break;
                case 0x38: // JRC
                    c[i++] = 0x73; // jnc byte
                    c[i++] = 0x15;
                    exit_vm(c, &i, ei, ip + (int8_t)MEM8(ip) + 1, cycle_counter);
                    ip++;
                    break;
                case 0x3C: // INC A
                    c[i++] = 0xFE; // inc al
                    c[i++] = 0xC0;
                    break;
                case 0x3D: // DEC A
                    c[i++] = 0xFE; // dec al
                    c[i++] = 0xC8;
                    break;
                case 0x3E: // LD A,n
                    c[i++] = 0xB0; // mov al,n
                    c[i++] = MEM8(ip++);
                    break;
                case 0x78: // LD A,B
                    c[i++] = 0x88; // mov al,bh
                    c[i++] = 0xF8;
                    break;
                case 0xAF: // XOR A,A
                    c[i++] = 0x30; // xor al,al
                    c[i++] = 0xC0;
                    c[i++] = 0x9F; // lahf
                    c[i++] = 0x80; // and ah,0x40 -- nur das ZF behalten
                    c[i++] = 0xE4;
                    c[i++] = 0x40;
                    c[i++] = 0x9E; // sahf
                    break;
                case 0xB1: // OR A,C
                    c[i++] = 0x08; // or al,bl
                    c[i++] = 0xD8;
                    break;
                case 0xC3: // JP
                    ip = MEM16(ip);
                    break;
                case 0xC9: // RET
                    exit_vm_by_ret(c, &i, ei, cycle_counter);
                    break;
                case 0xCD: // CALL
                    c[i++] = 0x9F; // lahf
                    c[i++] = 0x66; // sub bp,2
                    c[i++] = 0x83;
                    c[i++] = 0xED;
                    c[i++] = 0x02;
                    c[i++] = 0x9E; // sahf
                    c[i++] = 0x66; // mov word [ebp],ip+2
                    c[i++] = 0xC7;
                    c[i++] = 0x45;
                    c[i++] = 0x00;
                    c[i++] = (ip + 2) & 0xFF;
                    c[i++] = (ip + 2) >> 8;
                    ip = MEM16(ip);
                    break;
                case 0xE0: // LD (0xFF00+n),A
                case 0xF0: // LD A,(0xFF00+n)
                    exit_vm(c, &i, ei, ip - 1, cycle_counter);
                    break;
                case 0xEA: // LD (nn),A
                    c[i++] = 0xA2; // mov byte [],al
                    *((uint32_t *)&c[i]) = 0x40000000 + MEM16(ip);
                    ip += 2;
                    i += 4;
                    break;
                case 0xF1: // POP AF
                    // So fies wie PUSH AF.
                    c[i++] = 0x0F; // movzx eax,byte [ebp]
                    c[i++] = 0xB6;
                    c[i++] = 0x45;
                    c[i++] = 0x00;
                    c[i++] = 0x8A; // mov ah,[reverse_flag_table+eax]
                    c[i++] = 0xA0;
                    *((uint32_t *)&c[i]) = (uintptr_t)&reverse_flag_table;
                    i += 4;
                    c[i++] = 0x8A; // mov al,[ebp+1]
                    c[i++] = 0x45;
                    c[i++] = 0x01;
                    c[i++] = 0x66; // add bp,2
                    c[i++] = 0x83;
                    c[i++] = 0xC5;
                    c[i++] = 0x02;
                    c[i++] = 0x0D; // or eax,0x40000000
                    c[i++] = 0x00;
                    c[i++] = 0x00;
                    c[i++] = 0x00;
                    c[i++] = 0x40;
                    c[i++] = 0x9E; // sahf
                    break;
                case 0xF3: // DI
                    c[i++] = 0xC6; // mov byte [],0
                    c[i++] = 0x05;
                    *((uint32_t *)&c[i]) = (uintptr_t)&ime;
                    i += 4;
                    c[i++] = 0x00;
                    break;
                case 0xF5: // PUSH AF
                    // Das ist ganz schön fies, weil wir AF einfach umgekehrt
                    // verwendet und zudem ein ganz anderes F-Register haben...
                    c[i++] = 0x50; // push eax
                    c[i++] = 0x9F; // lahf
                    c[i++] = 0x50; // push eax
                    c[i++] = 0x0F; // movzx eax,ah
                    c[i++] = 0xB6;
                    c[i++] = 0xC4;
                    c[i++] = 0x8A; // mov al,[flag_table+eax]
                    c[i++] = 0x80;
                    *((uint32_t *)&c[i]) = (uintptr_t)&flag_table;
                    i += 4;
                    c[i++] = 0x8A; // mov ah,[esp]
                    c[i++] = 0x24;
                    c[i++] = 0x24;
                    c[i++] = 0x66; // sub bp,2
                    c[i++] = 0x83;
                    c[i++] = 0xED;
                    c[i++] = 0x02;
                    c[i++] = 0x66; // mov [ebp],ax
                    c[i++] = 0x89;
                    c[i++] = 0x45;
                    c[i++] = 0x00;
                    c[i++] = 0x58; // pop eax
                    c[i++] = 0x9E; // sahf
                    c[i++] = 0x58; // pop eax
                    break;
                case 0xFE: // CP A,n
                    c[i++] = 0x3C;
                    c[i++] = MEM8(ip++);
                    break;
                default:
                    printf("%04X: Unknown opcode %02X\n", ip - 1, op);
                    FILE *d = fopen("/tmp/xgbcdyna-coredump", "w");
                    fwrite(c, 1, 4096, d);
                    fclose(d);
                    exit(1);
            }
        }
        else
        {
            pop = MEM8(ip++);
            cycle_counter += cycles0xCB[pop];

            printf("Unknown opcode CB %02X\n", pop);
            exit(1);
        }

        if (cycle_counter >= 128) // Wahllos...
            do_break = 1;
    }

    /*
    printf("block 0x%04X - 0x%04X dumped.\n", ipwas, ip);
    FILE *d = fopen("/tmp/dump", "w");
    fwrite(c, 1, 4096, d);
    fclose(d);
    */

    return (exec_unit_t)(uintptr_t)c;
}

static inline exec_unit_t get_from_cache(struct dynarec_cache *c, uint16_t ip)
{
    if (!c[ip & 0xFF].in_use)
        c[ip & 0xFF].in_use = true;
    else
    {
        if (c[ip & 0xFF].ip == ip)
            return c[ip & 0xFF].code;

        munmap((void *)(uintptr_t)c[ip & 0xFF].code, 4096);
    }

    c[ip & 0xFF].ip = ip;

    return (c[ip & 0xFF].code = dynarec(ip));
}

static exec_unit_t cache_get(uint16_t ip)
{
    if (ip < 0x4000)
        return get_from_cache(base_rom, ip & 0x3FFF);
    else if (ip < 0x8000)
        return get_from_cache(banked_rom, ip & 0x3FFF);
    else
    {
        fprintf(stderr, "Kein Cache für IP=0x%04X.\n", (unsigned)ip);
        exit(1);
    }
}

static void flush_banked_rom(void)
{
    for (size_t i = 0; i < 0x100; i++)
    {
        munmap((void *)(uintptr_t)banked_rom[i].code, 4096);
        banked_rom[i].in_use = false;
    }
}

extern unsigned cumulative_cycles;

static struct timeval stp;

static void print_cum_cycles(int status, void *arg)
{
    (void)status;
    (void)arg;

    struct timeval etp;
    gettimeofday(&etp, NULL);

    printf("%i vierfache CPU-Zyklen ausgeführt (%g s).\n", cumulative_cycles, (double)cumulative_cycles / 1048576.);

    uintmax_t start = stp.tv_sec * 1000000ULL + stp.tv_usec;
    uintmax_t end = etp.tv_sec * 1000000ULL + etp.tv_usec;

    printf("Benötigte Hostzeit: %g s.\n", (double)(end - start) / 1000000.);
}

void begin_execution(void)
{
    vm_fa = MEM_BASE + 0x5311; // 0x53 entspricht 0xB0
    vm_bc = MEM_BASE + 0x0000;
    vm_de = MEM_BASE + 0xFF56;
    vm_hl = MEM_BASE + 0x000D;
    vm_sp = MEM_BASE + 0xFFFE;
    vm_ip =            0x0100;

    exec_unit_t recompiled;

    gettimeofday(&stp, NULL);

    on_exit(&print_cum_cycles, NULL);

    for (;;)
    {
        while (!compilability[MEM8(vm_ip)])
        {
            uint8_t op = MEM8(vm_ip++);
            cycles_gone += cycles[op];

            switch (op)
            {
                case 0xE0: // LD (0xFF00+n),A
                    hmem_write8(0x0F00 + MEM8(vm_ip++), vm_fa & 0xFF);
                    break;
                case 0xF0: // LD A,(0xFF00+n)
                    vm_fa = (vm_fa & 0xFF00) | hmem_read8(0x0F00 + MEM8(vm_ip++));
                    break;
                default:
                    vm_ip--;
                    printf("Unknown uncompilable opcode %02X (0x%04X).\n", op, vm_ip);
                    exit(1);
            }
        }

        inc_timer(cycles_gone);
        cycles_gone = 0;

        recompiled = cache_get(vm_ip);

        recompiled();

        // printf("PC=%04x AF=%04x BC=%04x DE=%04x HL=%04x SP=%04x\n", vm_ip & 0xFFFF, vm_af & 0xFFFF, vm_bc & 0xFFFF, vm_de & 0xFFFF, vm_hl & 0xFFFF, vm_sp & 0xFFFF);
    }
}
