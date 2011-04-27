#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "xgbc.h"

static SDL_Surface *screen, *vscreen;

extern struct io io_state;
extern bool has_cgb, lcd_on;

extern uint8_t *full_vidram, *oam;

// Background palette, object palette
uint16_t bpalette[32], opalette[32];
// Internal background palette, internal object palette
uint32_t ibpalette[32], iopalette[32];
uint8_t *btm[2], *bwtd[2], *wtm[2];

#define vpb ((uint32_t *)vscreen->pixels)

#define WIDTH 160
#define HEIGHT 144

extern unsigned keystates;

void init_video(unsigned mult)
{
    (void)mult;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)
    {
        fprintf(stderr, "Konnte SDL nicht initialisieren: %s\n", SDL_GetError());
        exit(1);
    }

    atexit(SDL_Quit);

    screen = SDL_SetVideoMode(WIDTH, HEIGHT, 32, SDL_HWSURFACE | SDL_ASYNCBLIT);
    if (screen == NULL)
    {
        fprintf(stderr, "Konnte keine SDL-Ausgabe öffnen: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_PixelFormat vscr_format = {
        .BitsPerPixel = 32,
        .BytesPerPixel = 4,
        .Rshift = 16,
        .Gshift = 8,
        .Bshift = 0,
        .Rmask = 0xFF0000,
        .Gmask = 0x00FF00,
        .Bmask = 0x0000FF,
    };
    vscreen = SDL_ConvertSurface(screen, &vscr_format, SDL_HWSURFACE | SDL_ASYNCBLIT);
    if (vscreen == NULL)
    {
        fprintf(stderr, "Konnte vscreen nicht erstellen: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_WM_SetCaption("xgbcdyna", NULL);


    btm[0] = &full_vidram[0x1800];
    btm[1] = &full_vidram[0x3800];
    bwtd[0] = &full_vidram[0x0000];
    bwtd[1] = &full_vidram[0x2000];
    wtm[0] = &full_vidram[0x1800];
    wtm[1] = &full_vidram[0x3800];

    for(int i = 0; i < 32; i++)
    {
        bpalette[i] = 0x011F;
        opalette[i] = 0x011F;
        ibpalette[i] = 0xFF4000;
        iopalette[i] = 0xFF4000;
    }
}

static void draw_bg_line(unsigned line, unsigned bit7val, unsigned window)
{
    unsigned ay = io_state.scy + line;
    int by = ay & 0xF8, ry = ay & 0x07;

    line *= 160;

    int cb1 = 0, cb2 = 0, cflags = 0, vbank = 0;
    uint32_t *cpal = ibpalette;
    uint8_t ax = io_state.scx;

    for (int sx = 0; sx < 160; sx++, ax++)
    {
        if (unlikely(!sx) || unlikely(!(ax & 7)))
        {
            if (window && (io_state.wx <= ax))
                break;

            int tile = (by << 2) + (ax >> 3);

            if (has_cgb)
                cflags = btm[1][tile];

            if ((cflags & (1 << 7)) != bit7val)
            {
                ax = ((ax + 9) & ~7) - 1;
                sx = ax - io_state.scx;
                if (sx < 0)
                    sx += 256;
                continue;
            }

            uint8_t *tdat;
            if (has_cgb)
            {
                vbank = !!(cflags & (1 << 3));
                cpal = &ibpalette[(cflags & 7) * 4];
            }

            if (bwtd[0] == &full_vidram[0x0000])
                tdat = &bwtd[vbank][btm[0][tile] * 16];
            else
                tdat = &bwtd[vbank][(int8_t)btm[0][tile] * 16];

            if (cflags & (1 << 6))
            {
                cb1 = tdat[(7 - ry) * 2];
                cb2 = tdat[(7 - ry) * 2 + 1];
            }
            else
            {
                cb1 = tdat[ry * 2];
                cb2 = tdat[ry * 2 + 1];
            }
        }

        int val, mask;

        if (cflags & (1 << 5))
            mask = 1 << (ax & 7);
        else
            mask = 1 << (7 - (ax & 7));

        val = !!(cb1 & mask) + !!(cb2 & mask) * 2;
        vpb[line + sx] = cpal[val] | (val << 24);
    }
}

void draw_line(unsigned line)
{
    if (!lcd_on)
    {
        if (line == 143)
            handle_input_events();
        return;
    }

    struct
    {
        uint8_t y, x;
        uint8_t num;
        uint8_t flags;
    } __attribute__((packed)) *soam = (void *)oam;
    int window_active = ((io_state.lcdc & (1 << 5)) && (io_state.wx >= 7) && (io_state.wx <= 166) && (io_state.wy <= line));

    unsigned line_o = line * 160;

    if (!(io_state.lcdc & (1 << 0)))
        memset(vpb + line_o, 0, 160 * sizeof(uint32_t));
    else
        draw_bg_line(line, 0 << 7, window_active);

    if (window_active)
    {
        int wx = io_state.wx - 7, wy = io_state.wy;
        int yoff = line - wy;
        int by = yoff & 0xF8, ry = yoff & 0x07;
        int tile = by * 4;

        for (int bx = 0; bx < 160; bx += 8)
        {
            int flags;
            if (has_cgb)
                flags = wtm[1][tile];
            else
                flags = 0;

            uint8_t *tdat;
            int vbank;
            uint32_t *pal;
            if (has_cgb)
            {
                vbank = !!(flags & (1 << 3));
                pal = &ibpalette[(flags & 7) * 4];
            }
            else
            {
                vbank = 0;
                pal = ibpalette;
            }

            if (bwtd[0] == &full_vidram[0x0000])
                tdat = &bwtd[vbank][(unsigned)wtm[0][tile] * 16];
            else
                tdat = &bwtd[vbank][(int)(int8_t)wtm[0][tile] * 16];

            for (int rx = 0; rx < 8; rx++)
            {
                int val;

                switch (flags & (3 << 5))
                {
                    case (0 << 5):
                        val = !!(tdat[ry * 2] & (1 << (7 - rx)));
                        val += !!(tdat[ry * 2 + 1] & (1 << (7 - rx))) << 1;
                        break;
                    case (1 << 5):
                        val = !!(tdat[ry * 2] & (1 << rx));
                        val += !!(tdat[ry * 2 + 1] & (1 << rx)) << 1;
                        break;
                    case (2 << 5):
                        val = !!(tdat[(7 - ry) * 2] & (1 << (7 - rx)));
                        val += !!(tdat[(7 - ry) * 2 + 1] & (1 << (7 - rx))) << 1;
                        break;
                    default:
                        val = !!(tdat[(7 - ry) * 2] & (1 << rx));
                        val += !!(tdat[(7 - ry) * 2 + 1] & (1 << rx)) << 1;
                }

                vpb[line_o + bx + rx + wx] = pal[val];
            }

            tile++;
        }
    }

    if (io_state.lcdc & (1 << 1))
    {
        int obj_height = (io_state.lcdc & (1 << 2)) ? 16 : 8;
        int count = 0;

        for (int sprite = 40; sprite >= 0; sprite--)
        {
            uint8_t *tdat;
            int bx = soam[sprite].x, by = soam[sprite].y, flags = soam[sprite].flags;
            uint32_t *pal;
            if (has_cgb)
                pal = &iopalette[(flags & 7) * 4];
            else
                pal = &iopalette[(flags & (1 << 4)) >> 2];

            if (!has_cgb)
                flags &= 0xF0;

            bx -= 8;
            by -= 16;

            if ((by > (int)line) || (by + obj_height <= (int)line))
                continue;

            if (count++ >= 10)
                break;

            if ((bx <= -8) || (bx >= 160))
                continue;

            int ry = line - by;

            if (flags & (1 << 6))
                ry = (obj_height - 1) - ry;

            if (obj_height == 8)
            {
                if (!(flags & (1 << 3)))
                    tdat = &full_vidram[0x0000 + soam[sprite].num * 16];
                else
                    tdat = &full_vidram[0x2000 + soam[sprite].num * 16];
            }
            else
            {
                if (!(flags & (1 << 3)))
                    tdat = &full_vidram[0x0000 + (soam[sprite].num & 0xFE) * 16];
                else
                    tdat = &full_vidram[0x2000 + (soam[sprite].num & 0xFE) * 16];
            }

            for (int rx = 0; rx < 8; rx++)
            {
                int val, bmask;

                if (flags & (1 << 5))
                    bmask = 1 << rx;
                else
                    bmask = 1 << (7 - rx);

                val = !!(tdat[ry * 2] & bmask);
                val += !!(tdat[ry * 2 + 1] & bmask) << 1;

                if (val && !(flags & (1 << 7)))
                    vpb[line_o + bx + rx] = pal[val];
                else if (val)
                {
                    if (!(vpb[line_o + bx + rx] & 0xFF000000))
                        vpb[line_o + bx + rx] = pal[val];
                }
            }
        }
    }

    if ((io_state.lcdc & (1 << 0)) && has_cgb)
        draw_bg_line(line, 1 << 7, window_active);

    if (line == 143)
    {
        handle_input_events();

        SDL_BlitSurface(vscreen, NULL, screen, NULL);
        SDL_UpdateRect(screen, 0, 0, 0, 0);
    }
}
