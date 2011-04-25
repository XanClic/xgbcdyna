#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "xgbc.h"

#define CODE_BUFSZ 4096

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

#define vmapp1(b, c, v) (b)[(c)++] = (v)
#define vmapp2(b, c, v) *((uint16_t *)&((b)[(c)])) = (v); (c) += 2
#define vmapp4(b, c, v) *((uint32_t *)&((b)[(c)])) = (v); (c) += 4

#define vmpre1(b, c, v) (b)[--(c)] = (v)
#define vmpre2(b, c, v) (c) -= 2; *((uint16_t *)&((b)[(c)])) = (v)
#define vmpre4(b, c, v) (c) -= 4; *((uint32_t *)&((b)[(c)])) = (v)

// Reserviert Speicher der Größe CODE_BUFSZ, in dem Code ausgeführt werden kann
static exec_unit_t allocate_eu(void)
{
    return (exec_unit_t)(uintptr_t)mmap(NULL, CODE_BUFSZ, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

// Gibt solch einen Speicherbereich wieder frei
static void free_eu(exec_unit_t ptr)
{
    munmap((void *)(uintptr_t)ptr, CODE_BUFSZ);
}

#include "dynarec/op-info.c"
#include "dynarec/vm-exit.c"
#include "dynarec/ops.c"

static exec_unit_t dynarec(uint16_t ip)
{
    drip = ip;

    exec_unit_t eu = allocate_eu();
    drc = (uint8_t *)(uintptr_t)eu;

    unsigned cycle_counter = 0;
    dr_cycle_counter = 0;

    dri = 0;
    drei = CODE_BUFSZ;

    // push ebx; push ebp
    drvmapp2(0x5553);

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
        // mov eax,[vm_fa]
        drvmapp1(0xA1);
        drvmapp4((uintptr_t)&vm_fa);
        // sahf
        drvmapp1(0x9E);
    }
    if (need_regs & REG_BC)
    {
        // mov ebx,[vm_bc]
        drvmapp2(0x1D8B);
        drvmapp4((uintptr_t)&vm_bc);
    }
    if (need_regs & REG_DE)
    {
        // mov ecx,[vm_de]
        drvmapp2(0x0D8B);
        drvmapp4((uintptr_t)&vm_de);
    }
    if (need_regs & REG_HL)
    {
        // mov edx,[vm_hl]
        drvmapp2(0x158B);
        drvmapp4((uintptr_t)&vm_hl);
    }
    if (need_regs & REG_SP)
    {
        // mov ebp,[vm_sp]
        drvmapp2(0x2D8B);
        drvmapp4((uintptr_t)&vm_sp);
    }


    // ret
    vmpre1(drc, drei, 0xC3);
    // pop ebx; pop ebp
    vmpre2(drc, drei, 0x5D5B);

    if (need_regs & REG_AF)
    {
        // mov [vm_fa],eax
        vmpre4(drc, drei, (uintptr_t)&vm_fa);
        // lahf
        vmpre2(drc, drei, 0xA39F);
    }
    if (need_regs & REG_BC)
    {
        // mov [vm_bc],ebx
        vmpre4(drc, drei, (uintptr_t)&vm_bc);
        vmpre2(drc, drei, 0x1D89);
    }
    if (need_regs & REG_DE)
    {
        // mov [vm_de],ecx
        vmpre4(drc, drei, (uintptr_t)&vm_de);
        vmpre2(drc, drei, 0x0D89);
    }
    if (need_regs & REG_HL)
    {
        // mov [vm_hl],edx
        vmpre4(drc, drei, (uintptr_t)&vm_hl);
        vmpre2(drc, drei, 0x1589);
    }
    if (need_regs & REG_SP)
    {
        // mov [vm_sp],ebp
        vmpre4(drc, drei, (uintptr_t)&vm_sp);
        vmpre2(drc, drei, 0x2D89);
    }


    do_break = 0;

    while (!do_break)
    {
        op = MEM8(drip++);

        if (likely(op != 0xCB))
        {
            if (compilability[op])
                dr_cycle_counter += cycles[op];
            do_break = break_on[op];

            if (dynarec_table[op] != NULL)
                dynarec_table[op]();
            else
            {
                unsigned type = dynarec_const_length[op];

                if (unlikely(type & DYNAREC_LOAD))
                {
                    if (likely(type == (2 | DYNAREC_LOAD)))
                    {
                        drvmapp1(dynarec_const[op]);
                        drvmapp1(MEM8(drip++));
                    }
                    else if (likely(type == (4 | DYNAREC_LOAD)))
                    {
                        drvmapp2(dynarec_const[op]);
                        drvmapp2(MEM16(drip));
                        drip += 2;
                    }
                    else
                    {
                        printf("%04X: Unknown opcode %02X\n", drip - 1, op);
                        FILE *d = fopen("/tmp/xgbcdyna-coredump", "w");
                        fwrite(drc, 1, CODE_BUFSZ, d);
                        fclose(d);
                        exit(1);
                    }
                }
                else
                {
                    if (likely(dynarec_const_length[op] == (2 | DYNAREC_CNST)))
                    {
                        drvmapp2(dynarec_const[op]);
                    }
                    else if (likely(dynarec_const_length[op] == (4 | DYNAREC_CNST)))
                    {
                        drvmapp4(dynarec_const[op]);
                    }
                    else
                    {
                        printf("%04X: Unknown opcode %02X\n", drip - 1, op);
                        FILE *d = fopen("/tmp/xgbcdyna-coredump", "w");
                        fwrite(drc, 1, CODE_BUFSZ, d);
                        fclose(d);
                        exit(1);
                    }
                }
            }
        }
        else
        {
            pop = MEM8(drip++);
            dr_cycle_counter += cycles0xCB[pop];

            printf("%04X: Unknown opcode CB %02X\n", drip - 1, pop);
            exit(1);
        }

        if (cycle_counter >= 128) // Wahllos...
            do_break = 1;
    }

    /*
    printf("block 0x???? - 0x%04X dumped.\n", ip);
    FILE *d = fopen("/tmp/dump", "w");
    fwrite(drc, 1, CODE_BUFSZ, d);
    fclose(d);
    */

    return eu;
}

// Sucht eine Ausführungseinheit aus dem angegebenen Cache, wenn sie da nicht
// gefunden wird, dann wird eine neue erstellt und in den Cache geladen
static inline exec_unit_t get_from_cache(struct dynarec_cache *c, uint16_t ip)
{
    if (!c[ip & 0xFF].in_use)
        c[ip & 0xFF].in_use = true;
    else
    {
        if (c[ip & 0xFF].ip == ip)
            return c[ip & 0xFF].code;

        free_eu(c[ip & 0xFF].code);
    }

    c[ip & 0xFF].ip = ip;

    return (c[ip & 0xFF].code = dynarec(ip));
}

// Sucht eine Ausführungseinheit in ihrem zugehörigen Cache, wenn sie nicht
// vorhanden ist, wird sie neu generiert und in den Cache geladen
static exec_unit_t cache_get(uint16_t ip)
{
    if (likely(ip < 0x4000))
        return get_from_cache(base_rom, ip & 0x3FFF);
    else if (likely(ip < 0x8000))
        return get_from_cache(banked_rom, ip & 0x3FFF);
    else
    {
        fprintf(stderr, "Kein Cache für IP=0x%04X.\n", (unsigned)ip);
        exit(1);
    }
}

// Leert den Cache für den ROM-Bereich zwischen 0x4000 und 0x7FFF
static void flush_banked_rom(void)
{
    for (size_t i = 0; i < 0x100; i++)
    {
        free_eu(banked_rom[i].code);
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

            // Diese OPs segfaulten in jedem Fall, sodass es schön dämlich wäre,
            // sie auszuführen
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
