#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "xgbc.h"

volatile int ime = 0;
bool interrupt_issued = false;
extern uint8_t ie;
extern struct io io_state;

extern uint32_t vm_ip, vm_sp;

void update_cpu(unsigned cycles)
{
    inc_timer(cycles);

    uint8_t issue = io_state.int_flag & ie & 0x1F;

    if (ime && issue)
    {
        ime = 0;
        vm_sp = ((vm_sp - 2) & 0xFFFF) | (vm_sp & 0xFFFF0000);
        mem_write16(vm_sp & 0xFFFF, vm_ip & 0xFFFF);

        uint16_t entry = 0x40;
        unsigned interrupt = 1;
        while (!(issue & interrupt))
        {
            entry += 0x08;
            interrupt <<= 1;
        }

        io_state.int_flag &= ~interrupt;
        vm_ip = (vm_ip & 0xFFFF0000) | entry;

        interrupt_issued = true;
    }
}
