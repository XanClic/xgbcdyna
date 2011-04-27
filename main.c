#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xgbc.h"

extern int romfd, ramfd;
extern unsigned ram_size;

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


    if (!check_cartridge(fp))
    {
        fprintf(stderr, "Diese Datei scheint kein ROM-Image zu sein.\n");
        return 1;
    }

    if (ram_size)
    {
        char *ramfn;
        if (argc >= 3)
            ramfn = argv[2];
        else
        {
            ramfn = malloc(strlen(argv[1]) + 5);
            strcpy(ramfn, argv[1]);
            char *sep = strrchr(ramfn, '.');
            if ((uintptr_t)sep > (uintptr_t)strrchr(ramfn, '/'))
                *sep = 0;
            strcat(ramfn, ".sav");
        }

        FILE *ramfp = fopen(ramfn, "rb+");
        if (ramfp == NULL)
        {
            ramfp = fopen(ramfn, "wb+");
            if (ramfp == NULL)
            {
                fprintf(stderr, "Konnte RAM-Image \"%s\" nicht öffnen: %s\n", ramfn, strerror(errno));
                return 1;
            }

            ftruncate(fileno(ramfp), ram_size * 0x2000);
        }

        ramfd = fileno(ramfp);
    }

    init_memory();
    install_segv_handler();
    init_video(1);


    begin_execution();

    return 0;
}
