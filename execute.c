#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "xgbc.h"

#define UNSAVE_FLAG_OPTIMIZATIONS
#define UNSAVE_RAM_CACHING



// ough to be enough
#define CODE_BUFSZ 0x1000
#define CODE_EXITI (CODE_BUFSZ - 0x0100)

#define CACHES_SIZE 0x400

// #define DUMP_BLOCKS
#define STATS



unsigned ltu_counter = 0;

volatile uint32_t vm_fa, vm_bc, vm_de, vm_hl, vm_sp, vm_ip, cycles_gone = 0;
extern volatile int ime;
extern bool interrupt_issued;

#ifdef STATS
unsigned cache_hits = 0, cache_misses = 0, cache_frees = 0, uncached = 0;
#endif

extern const uint8_t flag_table[256], reverse_flag_table[256];
extern unsigned rom_size;

#define vm_af (((vm_fa & 0xFF) << 8) | ((vm_fa & 0x4000) ? 0x80 : 0x00) | ((vm_fa & 0x1000) ? 0x20 : 0x00) | ((vm_fa & 0x0100) ? 0x10 : 0x00))

typedef void (*exec_unit_t)(void);

struct dynarec_cache
{
    bool in_use;

    uint16_t ip;
    exec_unit_t code;
};

static struct dynarec_cache base_rom[CACHES_SIZE];
static struct dynarec_cache **banked_rom_caches, *banked_rom;
#ifdef UNSAVE_RAM_CACHING
static struct dynarec_cache ram_cache[CACHES_SIZE];
#endif

// EU für Code im RAM (darf nicht gecachet werden)
// FIXME: Wenn sich der Code selbst modifziert, siehts trotzdem eng aus.
static exec_unit_t variable_eu;

#define vmapp1(b, c, v) (b)[(c)++] = (v)
#define vmapp2(b, c, v) *((uint16_t *)&((b)[(c)])) = (v); (c) += 2
#define vmapp4(b, c, v) *((uint32_t *)&((b)[(c)])) = (v); (c) += 4

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

static void dynarec(exec_unit_t eu, uint16_t ip)
{
#ifdef DUMP_BLOCKS
    uint16_t ipwas = ip;
#endif

    drip = ip;

    drc = (uint8_t *)(uintptr_t)eu;

    unsigned cycle_counter = 0;
    dr_cycle_counter = 0;

    dri = 0;

    uint8_t op, pop = 0;

    unsigned need_regs = REG_AF, do_break = 0;

    unsigned start_area = ip & 0xC000;

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
                case 0xC7: // RST
                case 0xCF:
                case 0xD7:
                case 0xDF:
                case 0xE7:
                case 0xEF:
                case 0xF7:
                case 0xFF:
                    ip = op - 0xC7;
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

        if (unlikely(cycle_counter >= 128)) // Wahllos...
            break;

        if ((ip & 0xC000) != start_area)
            break;
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
        // push ebx
        drvmapp1(0x53);
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
        // push ebp
        drvmapp1(0x55);
        // mov ebp,[vm_sp]
        drvmapp2(0x2D8B);
        drvmapp4((uintptr_t)&vm_sp);
    }


    size_t ei = CODE_EXITI;
    if (need_regs & REG_SP)
    {
        // mov [vm_sp],ebp
        vmapp2(drc, ei, 0x2D89);
        vmapp4(drc, ei, (uintptr_t)&vm_sp);
        // pop ebp
        vmapp1(drc, ei, 0x5D);
    }
    if (need_regs & REG_HL)
    {
        // mov [vm_hl],edx
        vmapp2(drc, ei, 0x1589);
        vmapp4(drc, ei, (uintptr_t)&vm_hl);
    }
    if (need_regs & REG_DE)
    {
        // mov [vm_de],ecx
        vmapp2(drc, ei, 0x0D89);
        vmapp4(drc, ei, (uintptr_t)&vm_de);
    }
    if (need_regs & REG_BC)
    {
        // mov [vm_bc],ebx
        vmapp2(drc, ei, 0x1D89);
        vmapp4(drc, ei, (uintptr_t)&vm_bc);
        // pop ebx
        vmapp1(drc, ei, 0x5B);
    }
    if (need_regs & REG_AF)
    {
        // lahf
        vmapp2(drc, ei, 0xA39F);
        // mov [vm_fa],eax
        vmapp4(drc, ei, (uintptr_t)&vm_fa);
    }
    // ret
    vmapp1(drc, ei, 0xC3);


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
                        printf("%04X: Unbekannter Opcode %02X\n", drip - 1, op);
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
                        printf("%04X: Unbekannter Opcode %02X\n", drip - 1, op);
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

            unsigned bval = 1 << ((pop & 0x38) >> 3);
            unsigned reg = pop & 0x07;

            switch ((pop & 0xC0) >> 6)
            {
                case 0x01: // BIT
                    // lahf
                    drvmapp1(0x9F);
                    // test r,n
                    if (reg == 7) // A aka al
                        drvmapp1(0xA8);
                    else
                    {
                        drvmapp1(0xF6);
                        drvmapp1(dynarec_table_CBbit[reg]);
                    }
                    drvmapp1(bval);
#ifndef UNSAVE_FLAG_OPTIMIZATIONS
                    // push eax; mov al,ah; lahf
                    drvmapp4(0x9FE08850);
                    // and al,0x01 (CF behalten); and ah,0xFE (CF löschen)
                    drvmapp4(0xE4800124);
                    // ...
                    drvmapp1(0xFE);
                    // or al,0x10 (AF setzen); or ah,al
                    drvmapp4(0xC408100C);
                    // sahf; pop eax
                    drvmapp2(0x589E);
#endif
                    break;
                case 0x02: // RES
                    // lahf
                    drvmapp1(0x9F);
                    // and r,n
                    if (reg == 7) // A aka al
                        drvmapp1(0x24);
                    else
                    {
                        drvmapp1(0x80);
                        drvmapp1(dynarec_table_CBbit[reg] | 0x20);
                    }
                    drvmapp1(~bval);
                    // sahf
                    drvmapp1(0x9E);
                    break;
                case 0x03: // SET
                    // lahf
                    drvmapp1(0x9F);
                    // or r,n
                    if (reg == 7) // A aka al
                        drvmapp1(0x0C);
                    else
                    {
                        drvmapp1(0x80);
                        drvmapp1(dynarec_table_CBbit[reg] | 0x08);
                    }
                    drvmapp1(bval);
                    // sahf
                    drvmapp1(0x9E);
                    break;
                default:
#ifdef UNSAVE_FLAG_OPTIMIZATIONS
                    if (unlikely(dynarec_table_CB[pop] != NULL))
                        dynarec_table_CB[pop]();
                    else if (likely(dynarec_const_CB[pop]))
                    {
                        drvmapp2(dynarec_const_CB[pop]);
                    }
#else
                    if (likely(dynarec_table_CB[pop] != NULL))
                        dynarec_table_CB[pop]();
#endif
                    else
                    {
                        printf("%04X: Unbekannter Opcode CB %02X\n", drip - 2, pop);
                        exit(1);
                    }
            }
        }

        if (unlikely(cycle_counter >= 128))
        {
            exit_vm(drc, &dri, drip, dr_cycle_counter);
            break;
        }

        if ((drip & 0xC000) != start_area)
        {
            exit_vm(drc, &dri, drip, dr_cycle_counter);
            break;
        }
    }

#ifdef DUMP_BLOCKS
    printf("block 0x%04X - 0x%04X dumped.\n", ipwas, ip);
    FILE *d = fopen("/tmp/dump", "w");
    fwrite(drc, 1, CODE_BUFSZ, d);
    fclose(d);
#endif
}

// Sucht eine Ausführungseinheit aus dem angegebenen Cache, wenn sie da nicht
// gefunden wird, dann wird eine neue erstellt und in den Cache geladen
static inline exec_unit_t get_from_cache(struct dynarec_cache *c, uint16_t ip)
{
    if (!c[ip & (CACHES_SIZE - 1)].in_use)
        c[ip & (CACHES_SIZE - 1)].in_use = true;
    else
    {
        if (c[ip & (CACHES_SIZE - 1)].ip == ip)
        {
#ifdef STATS
            cache_hits++;
#endif
            return c[ip & (CACHES_SIZE - 1)].code;
        }

#ifdef STATS
        cache_frees++;
#endif

        free_eu(c[ip & (CACHES_SIZE - 1)].code);
    }

#ifdef STATS
    cache_misses++;
#endif

    c[ip & (CACHES_SIZE - 1)].ip = ip;

    c[ip & (CACHES_SIZE - 1)].code = allocate_eu();
    dynarec((c[ip & (CACHES_SIZE - 1)].code = allocate_eu()), ip);

    return c[ip & (CACHES_SIZE - 1)].code;
}

// Sucht eine Ausführungseinheit in ihrem zugehörigen Cache, wenn sie nicht
// vorhanden ist, wird sie neu generiert und in den Cache geladen
static exec_unit_t cache_get(uint16_t ip)
{
    if (likely(ip < 0x4000))
        return get_from_cache(base_rom, ip);
    else if (likely(ip < 0x8000))
        return get_from_cache(banked_rom, ip);
    else
    {
#ifndef UNSAVE_RAM_CACHING
#ifdef STATS
        uncached++;
#endif

        dynarec(variable_eu, ip);
        return variable_eu;
#else
        return get_from_cache(ram_cache, ip);
#endif
    }
}

// Zeigt an, dass sich der ROM zwischen 0x4000 und 0x7FFF geändert hat.
void change_banked_rom(unsigned new_bank)
{
    banked_rom = banked_rom_caches[new_bank];
}

extern unsigned cumulative_cycles;

static struct timeval stp;

static void print_cum_cycles(int status, void *arg)
{
    (void)status;
    (void)arg;

    struct timeval etp;
    gettimeofday(&etp, NULL);

    printf("=== xgbcdyna beendet ===\n");

    printf("%i vierfache CPU-Zyklen ausgeführt (%g s).\n", cumulative_cycles, (double)cumulative_cycles / 1048576.);

    uintmax_t start = stp.tv_sec * 1000000ULL + stp.tv_usec;
    uintmax_t end = etp.tv_sec * 1000000ULL + etp.tv_usec;

    printf("Benötigte Hostzeit: %g s.\n", (double)(end - start) / 1000000.);

    printf("Verhältnis: %g-fach.\n\n", (double)cumulative_cycles * 1. / (1.048576 * (double)(end - start)));


#ifdef STATS
    unsigned total = cache_hits + cache_misses + uncached, cached = cache_hits + cache_misses;

    printf("Cache-Statistik:\n");
    printf("Insgesamt %i Rekompilierungsanfragen.\n", total);;
    printf("Davon %i (%g %%) echt rekompiliert, %i (%g %%) gecachet.\n", cache_misses + uncached, (double)(cache_misses + uncached) * 100. / (double)total, cache_hits, (double)cache_hits * 100. / (double)total);
    printf("%i (%g %%) durch den Cache, %i (%g %%) vorbei.\n", cached, (double)cached * 100. / (double)total, uncached, (double)uncached * 100. / (double)total);
    printf("%i (%g %%) Cache-Treffer, %i (%g %%) Cache-Verfehler.\n", cache_hits, (double)cache_hits * 100. / (double)cached, cache_misses, (double)cache_misses * 100. / (double)cached);
    printf("%i Blöcke gelöscht.\n\n", cache_frees);
#endif


    printf("Letzter bekannter VM-Status:\n");
    printf("PC=%04X AF=%04X BC=%04X DE=%04X HL=%04X SP=%04X\n", vm_ip & 0XFFFF, vm_af & 0XFFFF, vm_bc & 0XFFFF, vm_de & 0XFFFF, vm_hl & 0XFFFF, vm_sp & 0XFFFF);
}

void begin_execution(void)
{
    vm_fa = MEM_BASE + 0x5311; // 0x53 entspricht 0xB0
    vm_bc = MEM_BASE + 0x0000;
    vm_de = MEM_BASE + 0xFF56;
    vm_hl = MEM_BASE + 0x000D;
    vm_sp = MEM_BASE + 0xFFFE;
    vm_ip =            0x0100;

    variable_eu = allocate_eu();

    banked_rom_caches = calloc(rom_size, sizeof(banked_rom_caches[0]));
    for (size_t b = 0; b < rom_size; b++)
        banked_rom_caches[b] = calloc(CACHES_SIZE, sizeof(banked_rom_caches[b][0]));

    on_exit(&print_cum_cycles, NULL);

    gettimeofday(&stp, NULL);

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
                case 0x76: // HALT
                    interrupt_issued = false;
                    update_cpu(cycles_gone);
                    cycles_gone = 0;
                    while (!interrupt_issued)
                        update_cpu(1);
                    break;
                case 0xE0: // LD (0xFF00+n),A
                    hmem_write8(0x0F00 + MEM8(vm_ip++), vm_fa & 0xFF);
                    break;
                case 0xE2: // LD (0xFF00+C),A
                    hmem_write8(0x0F00 + (vm_bc & 0xFF), vm_fa & 0xFF);
                    break;
                case 0xF0: // LD A,(0xFF00+n)
                    vm_fa = (vm_fa & 0xFF00) | hmem_read8(0x0F00 + MEM8(vm_ip++));
                    break;
                case 0xF2: // LD A,(0xFF00+C)
                    vm_fa = (vm_fa & 0xFF00) | hmem_read8(0x0F00 + (vm_bc & 0xFF));
                    break;
                default:
                    vm_ip--;
                    printf("Unbekannter nicht kompilierbarer Opcode %02X (0x%04X).\n", op, vm_ip);
                    exit(1);
            }
        }

        update_cpu(cycles_gone);
        cycles_gone = 0;

        cache_get(vm_ip)();

        // printf("PC=%04x AF=%04x BC=%04x DE=%04x HL=%04x SP=%04x\n", vm_ip & 0xFFFF, vm_af & 0xFFFF, vm_bc & 0xFFFF, vm_de & 0xFFFF, vm_hl & 0xFFFF, vm_sp & 0xFFFF);
    }
}
