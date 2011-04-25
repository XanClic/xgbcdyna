#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "xgbc.h"

extern int ime;
extern uint8_t ie;
extern struct io io_state;

void update_cpu(unsigned cycles)
{
    inc_timer(cycles);

    if (ime && (io_state.int_flag & ie))
    {
        fprintf(stderr, "Müsste IRQ auslösen...\n");
        exit(1);
    }
}
