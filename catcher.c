#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L
#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#define _XOPEN_SOURCE 700
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "xgbc.h"

#define PLEN (sizeof(uintptr_t) * 2)

static void unhandled_segv(siginfo_t *info, ucontext_t *ac, const char *reason) __attribute__((noreturn));
static void unhandled_segv(siginfo_t *info, ucontext_t *ac, const char *reason)
{
    fprintf(stderr, "*** Unbehandelter Speicherzugriffsfehler. ***\n");
    fprintf(stderr, "%s\n", reason);

    uintptr_t ip = ac->uc_mcontext.gregs[REG_EIP];

    fprintf(stderr, "IP=0x%0*X FA=0x%0*X\n", PLEN, ip, PLEN, (uintptr_t)info->si_addr);

    exit(1);
}

static bool x86_disassemble(uint8_t *i, bool *rw, size_t *width, unsigned *reg, unsigned *reg_ofs, size_t *instr_len)
{
    if ((i[0] == 0x88) && (i[1] == 0x02)) // mov [edx],al
    {
        *rw = 1;
        *width = sizeof(uint8_t);
        *reg = REG_EAX;
        *reg_ofs = 0;
        *instr_len = 2;
    }
    else if (i[0] == 0xA2) // mov byte [],al
    {
        *rw = 1;
        *width = sizeof(uint8_t);
        *reg = REG_EAX;
        *reg_ofs = 0;
        *instr_len = 2;
    }
    else
    {
        fprintf(stderr, "Unbekannter x86-Befehl: %02X %02X %02X %02X %02X %02X\n", i[0], i[1], i[2], i[3], i[4], i[5]);
        return false;
    }

    return true;
}

static void segv_handler(int signo, siginfo_t *info, void *context)
{
    if (signo != SIGSEGV)
        return;

    ucontext_t *ac = context;

    uintptr_t addr = (uintptr_t)info->si_addr;
    uint8_t *instr = (uint8_t *)ac->uc_mcontext.gregs[REG_EIP];

    if ((addr < MEM_BASE) || (addr > MEM_BASE + 0xFFFF))
        unhandled_segv(info, ac, "Programminterner Fehler (unbeabsichtigt aka Bug).");

    bool rw;
    size_t wdt, ilen;
    unsigned reg, reg_shift;

    if (!x86_disassemble(instr, &rw, &wdt, &reg, &reg_shift, &ilen))
        unhandled_segv(info, ac, "Unbekannte Instruktion.");

    addr -= MEM_BASE;

    if (addr >= 0xF000)
    {
        switch (wdt)
        {
            case sizeof(uint8_t):
                if (rw)
                    hmem_write8(addr, ac->uc_mcontext.gregs[reg] >> reg_shift);
                else
                {
                    ac->uc_mcontext.gregs[reg] &= ~(0xFF << reg_shift);
                    ac->uc_mcontext.gregs[reg] |= hmem_read8(addr) << reg_shift;
                }
                break;
            case sizeof(uint16_t):
                if (rw)
                {
                    hmem_write8(addr + 0, ac->uc_mcontext.gregs[reg] >> (reg_shift + 0));
                    hmem_write8(addr + 1, ac->uc_mcontext.gregs[reg] >> (reg_shift + 8));
                }
                else
                {
                    ac->uc_mcontext.gregs[reg] &= ~(0xFFFF << reg_shift);
                    ac->uc_mcontext.gregs[reg] |= hmem_read8(addr + 0) << (reg_shift + 0);
                    ac->uc_mcontext.gregs[reg] |= hmem_read8(addr + 1) << (reg_shift + 8);
                }
                break;
            default:
                unhandled_segv(info, ac, "Ungültige Zugriffsgröße.");
        }
    }
    else if (addr < 0x8000)
    {
        if (!rw)
            unhandled_segv(info, ac, "RO-Zugriff auf ROM sollte eigentlich funktionieren (Bug)...");

        unhandled_segv(info, ac, "Schreibzugriff auf ROM.");
    }
    else
        unhandled_segv(info, ac, "Zugriff sollte eigentlich funktionieren (Bug)...");

    ac->uc_mcontext.gregs[REG_EIP] += ilen;
}

void install_segv_handler(void)
{
    sigaction(SIGSEGV, &(struct sigaction){ .sa_flags = SA_SIGINFO, .sa_sigaction = &segv_handler }, NULL);
}
