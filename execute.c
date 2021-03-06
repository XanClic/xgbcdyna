#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "xgbc.h"

// #define DUMP_BLOCKS



unsigned ltu_counter = 0;

volatile uint32_t vm_fa, vm_bc, vm_de, vm_hl, vm_sp, vm_ip, cycles_gone = 0;
extern volatile int ime;
extern bool interrupt_issued;

#ifdef CACHE_STATS
unsigned cache_hits = 0, cache_misses = 0, uncached = 0;
#endif
#ifdef SEGV_STATS
extern unsigned segfaults;
#endif
#ifdef CYCLE_STATS
uintmax_t dyna_cycles = 0, update_cycles = 0, exec_cycles = 0, evade_cycles = 0, halt_cycles = 0;
unsigned long long vhalt_cycles = 0;
#endif

extern const uint8_t flag_table[256], reverse_flag_table[256];
extern unsigned rom_size;
extern struct io io_state;

#define vm_af (((vm_fa & 0xFF) << 8) | ((vm_fa & 0x4000) ? 0x80 : 0x00) | ((vm_fa & 0x1000) ? 0x20 : 0x00) | ((vm_fa & 0x0100) ? 0x10 : 0x00))

typedef void (*exec_unit_t)(void);

struct dynarec_cache
{
    bool in_use;

    uint16_t ip;
    exec_unit_t code;

    unsigned bank;
};

static struct dynarec_cache base_rom[CACHES_SIZE];
static struct dynarec_cache banked_rom[CACHES_SIZE];
#ifdef UNSAVE_RAM_CACHING
static struct dynarec_cache ram_cache[CACHES_SIZE];
#endif

// EU für Code im RAM (darf nicht gecachet werden)
// FIXME: Wenn sich der Code selbst modifziert, siehts trotzdem eng aus.
static exec_unit_t variable_eu;

#define vmapp1(b, c, v) (b)[(c)++] = (v)
#define vmapp2(b, c, v) *((uint16_t *)&((b)[(c)])) = (v); (c) += 2
#define vmapp4(b, c, v) *((uint32_t *)&((b)[(c)])) = (v); (c) += 4

static inline void vmappn(uint8_t *b, size_t *c, uint8_t *va, size_t size)
{
    memcpy(b + *c, va, size);
    *c += size;
}

#include "dynarec/op-info.pc"
#include "dynarec/vm-exit.pc"
#include "dynarec/ops.pc"

#define evmappn(...) vmappn(drc, &ei, __VA_ARGS__, sizeof(__VA_ARGS__))

static void insert_const_insn(const struct DynarecConstInsn dci, int drip_offset)
{
    if (unlikely(dci.length == 0)) {
        printf("%04X: Unbekannter Opcode %02X\n", drip - drip_offset, drc[drip - drip_offset]);
        FILE *d = fopen("/tmp/xgbcdyna-coredump", "w");
        fwrite(drc, 1, CODE_BUFSZ, d);
        fclose(d);
        exit(1);
    }

    memcpy(drc + dri, dci.data, dci.length);
    if (unlikely(dci.load_length)) {
        if (dci.load_length == sizeof(uint8_t)) {
            drc[dri + dci.load_offset] = MEM8(drip++);
        } else if (likely(dci.load_length == sizeof(uint16_t))) {
            *(uint16_t *)&drc[dri + dci.load_offset] = MEM16(drip);
            drip += 2;
        } else {
            for (int i = 0; i < dci.load_length; i++) {
                drc[dri + dci.load_offset + i] = MEM8(drip++);
            }
        }
    }
    dri += dci.length;
}

static void dynarec(exec_unit_t eu, uint16_t ip)
{
#ifdef DUMP_BLOCKS
    uint16_t ipwas = ip;
#endif

    drip = ip;

    drc = (uint8_t *)(uintptr_t)eu;

#ifndef REGISTER_EQUIVALENCE
    unsigned cycle_counter = 0;
#endif
    dr_cycle_counter = 0;

    dri = 0;

    uint8_t op, pop = 0;

#ifdef REGISTER_EQUIVALENCE
    unsigned need_regs = REG_AF | REG_BC | REG_DE | REG_HL | REG_SP, do_break = 0;
#else
    unsigned need_regs = REG_AF, do_break = 0;
#endif

    unsigned start_area = ip & 0xC000;

#ifndef REGISTER_EQUIVALENCE
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

        if (unlikely(cycle_counter >= MAX_CYCLES))
            break;

        if ((ip & 0xC000) != start_area)
            break;
    }
#endif

    if (need_regs & REG_AF) {
        drvmappn(@@asm("mov eax,[0x12345678] \n sahf", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_fa"));
    }
    if (need_regs & REG_BC) {
        drvmappn(@@asm("push ebx \n mov ebx,[0x12345678]", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_bc"));
    }
    if (need_regs & REG_DE) {
        drvmappn(@@asm("mov ecx,[0x12345678]", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_de"));
    }
    if (need_regs & REG_HL) {
        drvmappn(@@asm("mov edx,[0x12345678]", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_hl"));
    }
    if (need_regs & REG_SP) {
        drvmappn(@@asm("push ebp \n mov ebp,[0x12345678]", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_sp"));
    }


    size_t ei = CODE_EXITI;
    if (need_regs & REG_SP) {
        drvmappn(@@asm("mov ebp,[0x12345678]", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_sp"));
        evmappn(@@asm("mov [0x12345678],ebp \n pop ebp", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_sp"));
    }
    if (need_regs & REG_HL) {
        evmappn(@@asm("mov [0x12345678],edx", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_hl"));
    }
    if (need_regs & REG_DE) {
        evmappn(@@asm("mov [0x12345678],ecx", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_de"));
    }
    if (need_regs & REG_BC) {
        evmappn(@@asm("mov [0x12345678],ebx \n pop ebx", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_bc"));
    }
    if (need_regs & REG_AF) {
        evmappn(@@asm("lahf \n mov [0x12345678],eax", [0x78, 0x56, 0x34, 0x12], "(uintptr_t)&vm_fa"));
    }
    evmappn(@@asm("ret"));


#ifndef REGISTER_EQUIVALENCE
    do_break = 0;
#endif

    while (!do_break) {
        op = MEM8(drip++);

        if (likely(op != 0xCB)) {
            if (compilability[op]) {
                dr_cycle_counter += cycles[op];
            }
            do_break = break_on[op];

            if (dynarec_table[op] != NULL) {
                dynarec_table[op]();
            } else {
                insert_const_insn(dynarec_const_insns[op], 1);
            }
        } else {
            pop = MEM8(drip++);
            dr_cycle_counter += cycles0xCB[pop];

            insert_const_insn(dynarec_const_cb_insns[pop], 2);
        }

        if (unlikely(dr_cycle_counter >= MAX_CYCLES)) {
            exit_vm(drc, &dri, drip, dr_cycle_counter);
            break;
        }

        if ((drip & 0xC000) != start_area) {
            exit_vm(drc, &dri, drip, dr_cycle_counter);
            break;
        }
    }

#ifndef REGISTER_EQUIVALENCE
    assert(cycle_counter == dr_cycle_counter);
    assert(ip == drip);
#endif

#ifdef DUMP_BLOCKS
    printf("block 0x%04X - 0x%04X dumped.\n", ipwas, ip);
    FILE *d = fopen("/tmp/dump", "w");
    fwrite(drc, 1, CODE_BUFSZ, d);
    fclose(d);
#endif
}

// Sucht eine Ausführungseinheit aus dem angegebenen Cache, wenn sie da nicht
// gefunden wird, dann wird eine neue erstellt und in den Cache geladen
static inline exec_unit_t get_from_cache(struct dynarec_cache *c, unsigned bank, uint16_t ip)
{
    if (!c[ip & (CACHES_SIZE - 1)].in_use)
        c[ip & (CACHES_SIZE - 1)].in_use = true;
    else
    {
        if ((c[ip & (CACHES_SIZE - 1)].ip == ip) && (c[ip & (CACHES_SIZE - 1)].bank == bank))
        {
#ifdef CACHE_STATS
            cache_hits++;
#endif
            return c[ip & (CACHES_SIZE - 1)].code;
        }
    }

#ifdef CACHE_STATS
    cache_misses++;
#endif

    c[ip & (CACHES_SIZE - 1)].ip = ip;
    c[ip & (CACHES_SIZE - 1)].bank = bank;

    dynarec(c[ip & (CACHES_SIZE - 1)].code, ip);

    return c[ip & (CACHES_SIZE - 1)].code;
}

static unsigned current_rom_bank = 1;

// Sucht eine Ausführungseinheit in ihrem zugehörigen Cache, wenn sie nicht
// vorhanden ist, wird sie neu generiert und in den Cache geladen
static exec_unit_t cache_get(uint16_t ip)
{
    if (likely(ip < 0x4000))
        return get_from_cache(base_rom, 0, ip);
    else if (likely(ip < 0x8000))
        return get_from_cache(banked_rom, current_rom_bank, ip);
    else
    {
#ifndef UNSAVE_RAM_CACHING
#ifdef CACHE_STATS
        uncached++;
#endif

        dynarec(variable_eu, ip);
        return variable_eu;
#else
        return get_from_cache(ram_cache, 0, ip);
#endif
    }
}

// Zeigt an, dass sich der ROM zwischen 0x4000 und 0x7FFF geändert hat.
void change_banked_rom(unsigned new_bank)
{
    current_rom_bank = new_bank;
}

extern unsigned long long cumulative_cycles;

static struct timeval stp;

static void print_cum_cycles(int status, void *arg)
{
    (void)status;
    (void)arg;

    struct timeval etp;
    gettimeofday(&etp, NULL);

    printf("=== xgbcdyna beendet ===\n");

    printf("%llu vierfache CPU-Zyklen ausgeführt (%g s).\n", cumulative_cycles, (double)cumulative_cycles / 1048576.);

    uintmax_t start = stp.tv_sec * 1000000ULL + stp.tv_usec;
    uintmax_t end = etp.tv_sec * 1000000ULL + etp.tv_usec;

    printf("Benötigte Hostzeit: %g s.\n", (double)(end - start) / 1000000.);

    printf("Verhältnis: %g-fach.\n\n", (double)cumulative_cycles * 1. / (1.048576 * (double)(end - start)));


#ifdef CACHE_STATS
    unsigned total = cache_hits + cache_misses + uncached, cached = cache_hits + cache_misses;

    printf("Cache-Statistik:\n");
    printf("Insgesamt %i Rekompilierungsanfragen.\n", total);;
    printf("Davon %i (%g %%) echt rekompiliert, %i (%g %%) gecachet.\n", cache_misses + uncached, (double)(cache_misses + uncached) * 100. / (double)total, cache_hits, (double)cache_hits * 100. / (double)total);
    printf("%i (%g %%) durch den Cache, %i (%g %%) vorbei.\n", cached, (double)cached * 100. / (double)total, uncached, (double)uncached * 100. / (double)total);
    printf("%i (%g %%) Cache-Treffer, %i (%g %%) Cache-Verfehler.\n\n", cache_hits, (double)cache_hits * 100. / (double)cached, cache_misses, (double)cache_misses * 100. / (double)cached);
#endif

#ifdef SEGV_STATS
    printf("%i Speicherzugriffsfehler abgefangen (%i pro Sekunde).\n\n", segfaults, (int)((double)segfaults * 1000000. / (double)(end - start) + .5));
#endif

#ifdef CYCLE_STATS
    uint32_t itscres = determine_tsc_resolution();
    double tscres = (double)itscres * 1000.;
    double cycsum = dyna_cycles + update_cycles + exec_cycles + evade_cycles;

    printf("TSC läuft mit %i Zyklen pro Millisekunde (%g GHz).\n", itscres, tscres / 1000000000.);
    printf("%lli Zyklen im Übersetzer, %lli in der Ausgabe, %lli in der Ausführung, %lli im Ausweichemulator.\n", dyna_cycles, update_cycles, exec_cycles, evade_cycles);
    printf("== %g s (%g %%), %g s (%g %%), ", (double)dyna_cycles / tscres, (double)dyna_cycles * 100. / cycsum, (double)update_cycles / tscres, (double)update_cycles * 100. / cycsum);
    printf("%g s (%g %%), %g s (%g %%).\n", (double)exec_cycles / tscres, (double)exec_cycles * 100. / cycsum, (double)evade_cycles / tscres, (double)evade_cycles * 100. / cycsum);

    double time_sum = cycsum / tscres;
    printf("%g s nicht erfasst.\n\n", (double)(end - start) / 1000000. - time_sum);

    printf("%lli Zyklen (%g s, %g %%) im HALT, ", halt_cycles, (double)halt_cycles / tscres, (double)halt_cycles * 100. / cycsum);
    printf("virtuelle CPU-Auslastung: %llu von %llu Zyklen (%g %%).\n\n", cumulative_cycles - vhalt_cycles, cumulative_cycles, (double)(cumulative_cycles - vhalt_cycles) * 100. / (double)cumulative_cycles);
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

    io_state.p1     = 0x00;
    io_state.sb     = 0xFF;
    io_state.sc     = 0x00;
    io_state.div    = 0x00;
    io_state.tima   = 0x00;
    io_state.tma    = 0x00;
    io_state.tac    = 0x00;
    io_state.nr10   = 0x80;
    io_state.nr11   = 0xBF;
    io_state.nr12   = 0xF3;
    io_state.nr14   = 0xBF;
    io_state.nr21   = 0x3F;
    io_state.nr22   = 0x00;
    io_state.nr24   = 0xBF;
    io_state.nr30   = 0x7F;
    io_state.nr31   = 0xFF;
    io_state.nr32   = 0x9F;
    io_state.nr33   = 0xBF;
    io_state.nr41   = 0xFF;
    io_state.nr42   = 0x00;
    io_state.nr43   = 0x00;
    io_state.nr44   = 0xBF;
    io_state.nr50   = 0x77;
    io_state.nr51   = 0xF3;
    io_state.nr52   = 0xF1;
    io_state.lcdc   = 0x83;
    io_state.stat   = 0x01;
    io_state.scy    = 0x00;
    io_state.scx    = 0x00;
    io_state.lyc    = 0x00;
    io_state.bgp    = 0xFC;
    io_state.obp0   = 0xFF;
    io_state.obp1   = 0xFF;
    io_state.wy     = 0x00;
    io_state.wx     = 0x00;
    io_state.hdma5  = 0xFF;

    variable_eu = (exec_unit_t)(uintptr_t)mmap(NULL, CODE_BUFSZ, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    uintptr_t full_cache = (uintptr_t)mmap(NULL, 3 * CACHES_SIZE * CODE_BUFSZ, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    for (size_t e = 0; e < CACHES_SIZE; e++)
    {
        base_rom[e].code   = (exec_unit_t)(full_cache + 0 * CACHES_SIZE * CODE_BUFSZ);
        banked_rom[e].code = (exec_unit_t)(full_cache + 1 * CACHES_SIZE * CODE_BUFSZ);
#ifdef UNSAVE_RAM_CACHING
        ram_cache[e].code  = (exec_unit_t)(full_cache + 2 * CACHES_SIZE * CODE_BUFSZ);
#endif
        full_cache += CODE_BUFSZ;
    }

    on_exit(&print_cum_cycles, NULL);

    gettimeofday(&stp, NULL);

#ifdef CYCLE_STATS
    uintmax_t v1, v2, v3, v4;
#endif

    for (;;)
    {
#ifdef CYCLE_STATS
        __asm__ __volatile__ ("rdtsc" : "=A"(v1));
#endif
        uint8_t op;
        while (!compilability[(op = MEM8(vm_ip))])
        {
            vm_ip++;
            cycles_gone += cycles[op];

            // Diese OPs segfaulten in jedem Fall, sodass es schön dämlich wäre,
            // sie auszuführen
            switch (op)
            {
                case 0x76: // HALT
                    interrupt_issued = false;
#ifdef CYCLE_STATS
                    __asm__ __volatile__ ("rdtsc" : "=A"(v3));
                    evade_cycles += v3 - v1;
                    vhalt_cycles += cycles[0x76];
#endif
                    update_cpu(cycles_gone);
#ifdef CYCLE_STATS
                    __asm__ __volatile__ ("rdtsc" : "=A"(v4));
                    update_cycles += v4 - v3;
#endif
                    cycles_gone = 0;
                    while (!interrupt_issued)
                    {
#ifdef CYCLE_STATS
                        __asm__ __volatile__ ("rdtsc" : "=A"(v2));
#endif
#if HALT_CYCLES == -1
                        skip_until_hblank();
#else
#ifdef CYCLE_STATS
                        vhalt_cycles += HALT_CYCLES;
#endif
                        update_cpu(HALT_CYCLES);
#endif
#ifdef CYCLE_STATS
                        __asm__ __volatile__ ("rdtsc" : "=A"(v4));
                        update_cycles += v4 - v2;
#endif
                    }
#ifdef CYCLE_STATS
                    __asm__ __volatile__ ("rdtsc" : "=A"(v1));
                    halt_cycles += v1 - v3;
#endif
                    break;
                case 0xE0: // LD (0xFF00+n),A
                    hmem_write8(0x0F00 + MEM8(vm_ip++), vm_fa & 0xFF);
                    break;
                case 0xE2: // LD (0xFF00+C),A
                    hmem_write8(0x0F00 + (vm_bc & 0xFF), vm_fa & 0xFF);
                    break;
#ifndef UNSAVE_RAM_MAPPING
                case 0xF0: // LD A,(0xFF00+n)
                    vm_fa = (vm_fa & 0xFF00) | hmem_read8(0x0F00 + MEM8(vm_ip++));
                    break;
                case 0xF2: // LD A,(0xFF00+C)
                    vm_fa = (vm_fa & 0xFF00) | hmem_read8(0x0F00 + (vm_bc & 0xFF));
                    break;
#endif
                default:
                    vm_ip--;
                    printf("Unbekannter nicht kompilierbarer Opcode %02X (0x%04X).\n", op, vm_ip);
                    exit(1);
            }
        }
#ifdef CYCLE_STATS
        __asm__ __volatile__ ("rdtsc" : "=A"(v2));
        evade_cycles += v2 - v1;
#endif
        update_cpu(cycles_gone);
#ifdef CYCLE_STATS
        __asm__ __volatile__ ("rdtsc" : "=A"(v1));
        update_cycles += v1 - v2;
#endif
        cycles_gone = 0;

        exec_unit_t current_eu = cache_get(vm_ip);
#ifdef CYCLE_STATS
        __asm__ __volatile__ ("rdtsc" : "=A"(v2));
        dyna_cycles += v2 - v1;
#endif

        current_eu();
#ifdef CYCLE_STATS
        __asm__ __volatile__ ("rdtsc" : "=A"(v1));
        exec_cycles += v1 - v2;
#endif

        // printf("PC=%04x AF=%04x BC=%04x DE=%04x HL=%04x SP=%04x\n", vm_ip & 0xFFFF, vm_af & 0xFFFF, vm_bc & 0xFFFF, vm_de & 0xFFFF, vm_hl & 0xFFFF, vm_sp & 0xFFFF);
    }
}
