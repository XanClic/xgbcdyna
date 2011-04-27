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


#ifdef STATS
unsigned segfaults = 0;
#endif

static void unhandled_segv(ucontext_t *ac, const char *reason) __attribute__((noreturn));
static void unhandled_segv(ucontext_t *ac, const char *reason)
{
    fprintf(stderr, "*** Unbehandelter Speicherzugriffsfehler. ***\n");
    fprintf(stderr, "%s\n", reason);

    fprintf(stderr, "EIP=0x%08X CR2=0x%08X\n", ac->uc_mcontext.gregs[REG_EIP], (unsigned)ac->uc_mcontext.cr2);
    fprintf(stderr, "EAX=0x%08X EBX=0x%08X ECX=0x%08X EDX=0x%08X\n", ac->uc_mcontext.gregs[REG_EAX], ac->uc_mcontext.gregs[REG_EBX], ac->uc_mcontext.gregs[REG_ECX], ac->uc_mcontext.gregs[REG_EDX]);
    fprintf(stderr, "ESI=0x%08X EDI=0x%08X EBP=0x%08X ESP=0x%08X\n", ac->uc_mcontext.gregs[REG_ESI], ac->uc_mcontext.gregs[REG_EDI], ac->uc_mcontext.gregs[REG_EBP], ac->uc_mcontext.gregs[REG_ESP]);

    exit(1);
}

// Prüft, ob der Zugriff auf die Adresse überhaupt faulten darf
static void unsafe_mem_test(ucontext_t *ac, uintptr_t addr)
{
    if (unlikely(addr < MEM_BASE) || unlikely(addr > MEM_BASE + 0xFFFF))
        unhandled_segv(ac, "Programminterner Fehler (unbeabsichtigt aka Bug).");

    if (unlikely((addr >= 0x8000) && (addr < 0xA000)) || unlikely((addr >= 0xC000) && (addr < 0xF000)))
        unhandled_segv(ac, "Zugriff sollte eigentlich funktionieren (Bug)...");
}

// Liest aus anscheinend ungemapptem Speicher
static uint8_t unsafe_mem_read8(uint16_t addr)
{
    if (likely(addr >= 0xF000)) // HMEM (WRAM/OAM/IO/HRAM)
        return hmem_read8(addr - 0xF000);
    else if (likely(addr < 0x8000)) // ROM
        return *(uint8_t *)(MEM_BASE + addr);
    else // externer RAM
        return eram_read8(addr - 0xA000);
}

// Schreibt in anscheinend ungemappten oder RO-Speicher
static void unsafe_mem_write8(uint16_t addr, uint8_t val)
{
    if (likely(addr >= 0xF000)) // HMEM (WRAM/OAM/IO/HRAM)
        hmem_write8(addr - 0xF000, val);
    else if (addr < 0x8000) // ROM
        rom_write8(addr, val);
    else // externer RAM
        eram_write8(addr - 0xA000, val);
}

static void unsafe_mem_write16(uint16_t addr, uint16_t val)
{
    if (likely(addr >= 0xF000))
    {
        hmem_write8(addr - 0xF000, val);
        hmem_write8(addr - 0xEFFF, val >> 8);
    }
    else if (addr < 0x8000)
    {
        rom_write8(addr + 0, val);
        rom_write8(addr + 1, val >> 8);
    }
    else
    {
        eram_write8(addr - 0xA000, val);
        eram_write8(addr - 0x9FFF, val >> 8);
    }
}

static size_t x86_execute(ucontext_t *ac)
{
    uint8_t *i = (uint8_t *)ac->uc_mcontext.gregs[REG_EIP];

    if ((i[0] == 0x88) && (i[1] == 0x02)) // mov [edx],al
    {
        unsafe_mem_write8(ac->uc_mcontext.gregs[REG_EDX] & 0xFFFF, ac->uc_mcontext.gregs[REG_EAX] & 0xFF);
        return 2;
    }
    else if ((i[0] == 0x8A) && (i[1] == 0x02)) // mov al,[edx]
    {
        ac->uc_mcontext.gregs[REG_EAX] &= ~0xFF;
        ac->uc_mcontext.gregs[REG_EAX] |= unsafe_mem_read8(ac->uc_mcontext.gregs[REG_EDX] & 0xFFFF);
        return 2;
    }
    else if (i[0] == 0xA2) // mov byte [nnnn],al
    {
        unsafe_mem_write8(*(uint16_t *)(i + 1), ac->uc_mcontext.gregs[REG_EAX] & 0xFF);
        return 5;
    }
    else if ((i[0] == 0xFE) && (i[1] == 0x02)) // inc byte [edx]
    {
        uint8_t result = unsafe_mem_read8(ac->uc_mcontext.gregs[REG_EDX] & 0xFFFF) + 1;
        unsafe_mem_write8(ac->uc_mcontext.gregs[REG_EDX] & 0xFFFF, result);
        ac->uc_mcontext.gregs[REG_EFL] &= ~0x08D4; // OF, SF, ZF, AF und PF löschen
        // ZF und AF entsprechend setzen, SF/OF/PF brauche ich nicht
        ac->uc_mcontext.gregs[REG_EFL] |= (!result << 6) /* ZF */ | (!(result & 15) << 4) /* AF */;
        return 2;
    }
    else if ((i[0] == 0xC6) && (i[1] == 0x02)) // mov byte [edx],n
    {
        unsafe_mem_write8(ac->uc_mcontext.gregs[REG_EDX] & 0xFFFF, i[2]);
        return 3;
    }
    else if ((i[0] == 0x8A) && (i[1] == 0x01)) // mov al,[ecx]
    {
        ac->uc_mcontext.gregs[REG_EAX] &= ~0xFF;
        ac->uc_mcontext.gregs[REG_EAX] |= unsafe_mem_read8(ac->uc_mcontext.gregs[REG_ECX] & 0xFFFF);
        return 2;
    }
    else if ((i[0] == 0x88) && (i[1] == 0x01)) // mov [ecx],al
    {
        unsafe_mem_write8(ac->uc_mcontext.gregs[REG_ECX] & 0xFFFF, ac->uc_mcontext.gregs[REG_EAX] & 0xFF);
        return 2;
    }
    else if ((i[0] == 0xF6) && (i[1] == 0x02)) // test byte [edx],n
    {
        uint8_t result = unsafe_mem_read8(ac->uc_mcontext.gregs[REG_EDX] & 0xFFFF) & i[2];
        ac->uc_mcontext.gregs[REG_EFL] &= ~0x08C5; // OF, SF, ZF, PF und CF löschen
        // ZF entsprechend setzen, SF/PF brauche ich nicht
        ac->uc_mcontext.gregs[REG_EFL] |= (!result << 6);
        return 3;
    }
    else if (i[0] == 0xA0) // mov al,[nnnn]
    {
        ac->uc_mcontext.gregs[REG_EAX] &= ~0xFF;
        ac->uc_mcontext.gregs[REG_EAX] |= unsafe_mem_read8(*(uint16_t *)(i + 1));
        return 5;
    }
    else if ((i[0] == 0x66) && (i[1] == 0x89) && (i[2] == 0x2D)) // mov [nnnn],bp
    {
        unsafe_mem_write16(*(uint16_t *)(i + 3), ac->uc_mcontext.gregs[REG_EBP] & 0xFFFF);
        return 7;
    }
    else if ((i[0] == 0x3A) && (i[1] == 0x02)) // cmp al,[edx]
    {
        uint8_t v1 = ac->uc_mcontext.gregs[REG_EAX] & 0xFF;
        uint8_t v2 = unsafe_mem_read8(ac->uc_mcontext.gregs[REG_EDX] & 0xFFFF);
        unsigned result = (unsigned)v1 - (unsigned)v2;
        ac->uc_mcontext.gregs[REG_EFL] &= ~0x08D5; // OF, SF, ZF, AF, PF und CF löschen
        // ZF, CF und AF setzen, den Rest brauche ich nicht
        ac->uc_mcontext.gregs[REG_EFL] |= (!result << 6) /* ZF */ | ((result & 0x100) >> 8) /* CF */ | (((v1 & 0xF) < (v2 & 0xF)) << 4) /* AF */;
        return 2;
    }
    else if ((i[0] == 0x2A) && (i[1] == 0x02)) // sub al,[edx]
    {
        uint8_t v1 = ac->uc_mcontext.gregs[REG_EAX] & 0xFF;
        uint8_t v2 = unsafe_mem_read8(ac->uc_mcontext.gregs[REG_EDX] & 0xFFFF);
        unsigned result = (unsigned)v1 - (unsigned)v2;
        ac->uc_mcontext.gregs[REG_EAX] &= ~0xFF;
        ac->uc_mcontext.gregs[REG_EAX] |= result & 0xFF;
        ac->uc_mcontext.gregs[REG_EFL] &= ~0x08D5; // OF, SF, ZF, AF, PF und CF löschen
        // ZF, CF und AF setzen, den Rest brauche ich nicht
        ac->uc_mcontext.gregs[REG_EFL] |= (!result << 6) /* ZF */ | ((result & 0x100) >> 8) /* CF */ | (((v1 & 0xF) < (v2 & 0xF)) << 4) /* AF */;
        return 2;
    }
    else if ((i[0] == 0x80) && (i[1] == 0x22)) // and byte [edx],n
    {
        uint8_t result = unsafe_mem_read8(ac->uc_mcontext.gregs[REG_EDX] & 0xFFFF) & i[2];
        unsafe_mem_write8(ac->uc_mcontext.gregs[REG_EDX] & 0xFFFF, result);
        ac->uc_mcontext.gregs[REG_EFL] &= ~0x08C5; // OF, SF, ZF, PF und CF löschen
        // ZF entsprechend setzen, SF/PF brauche ich nicht
        ac->uc_mcontext.gregs[REG_EFL] |= (!result << 6);
        return 3;
    }
    else
    {
        fprintf(stderr, "0x%08X: Unbekannter x86-Befehl: %02X %02X %02X %02X %02X %02X\n", (uintptr_t)i, i[0], i[1], i[2], i[3], i[4], i[5]);
        FILE *d = fopen("/tmp/xgbcdyna-coredump", "wb");
        fwrite((void *)((uintptr_t)i & ~0xFFF), 4096, 1, d);
        fclose(d);
        fprintf(stderr, "Page mit dem Befehl wurde nach /tmp/xgbcdyna-coredump gedumpt.\n");
        unhandled_segv(ac, "Unbekannter Befehl.");
    }
}

static void segv_handler(int signo, siginfo_t *info, void *context)
{
    if (signo != SIGSEGV)
        return;

#ifdef STATS
    segfaults++;
#endif

    ucontext_t *ac = context;

    unsafe_mem_test(ac, (uintptr_t)info->si_addr);

    ac->uc_mcontext.gregs[REG_EIP] += x86_execute(ac);
}

void install_segv_handler(void)
{
    sigaction(SIGSEGV, &(struct sigaction){ .sa_flags = SA_SIGINFO, .sa_sigaction = &segv_handler }, NULL);
}
