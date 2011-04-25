#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xgbc.h"

extern int romfd;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "ROM-Imagename erwartet.\n");
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL)
    {
        perror("Konnte ROM-Image nicht öffnen");
        return 1;
    }


    romfd = fileno(fp);
    init_memory();
    install_segv_handler();


    begin_execution();

    return 0;
}
