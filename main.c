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
        FILE *ramfp = fopen("/tmp/xgbcdyna.sav", "rb+");
        if (ramfp == NULL)
        {
            ramfp = fopen("/tmp/xgbcdyna.sav", "wb+");
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
