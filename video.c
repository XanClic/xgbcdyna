#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "xgbc.h"

// #define DISPLAY_ALL

int frameskip_skip = 0, frameskip_draw = 16;
static int frameskip_draw_i = 0, frameskip_skip_i = 0, skip_this = 0;

static SDL_Surface *screen;

extern struct io io_state;
extern bool has_cgb, lcd_on;

extern uint8_t *full_vidram, *oam;

uint16_t bpalette[32], opalette[32];
uint8_t *btm[2], *bwtd[2], *wtm[2];

uint32_t *vidmem;

#ifdef DISPLAY_ALL
#define WIDTH 256
#define HEIGHT 256
#else
#define WIDTH 160
#define HEIGHT 144
#endif

extern unsigned keystates;

static inline int pal2rgb(int pal)
{
    return ((pal & 0x1F) << 19) | (((pal >> 5) & 0x1F) << 11) | ((pal >> 10) << 3);
}

void init_video(unsigned mult)
{
    (void)mult;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)
    {
        fprintf(stderr, "Konnte SDL nicht initialisieren: %s\n", SDL_GetError());
        exit(1);
    }

    atexit(SDL_Quit);

    screen = SDL_SetVideoMode(WIDTH, HEIGHT, 32, SDL_HWSURFACE);
    if (screen == NULL)
    {
        fprintf(stderr, "Konnte keine SDL-Ausgabe öffnen: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_WM_SetCaption("xgbcdyna", NULL);


    vidmem = calloc(sizeof(uint32_t), 256 * 256);

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
    }
}

static void draw_bg_line(unsigned line, unsigned bit7val, unsigned window)
{
    int by = line & 0xF8, ry = line & 0x07;
    int tile = by * 4;

    line *= 256;

    for (int bx = 0; bx < 256; bx += 8)
    {
        if (window && (io_state.wx <= bx))
            break;

        int flags;
        if (has_cgb)
            flags = btm[1][tile];
        else
            flags = 0;

        if ((flags & (1 << 7)) != bit7val)
        {
            tile++;
            continue;
        }

        uint8_t *tdat;
        int vbank;
        uint16_t *pal;
        if (has_cgb)
        {
            vbank = !!(flags & (1 << 3));
            pal = &bpalette[(flags & 7) * 4];
        }
        else
        {
            vbank = 0;
            pal = bpalette;
        }

        if (bwtd[0] == &full_vidram[0x0000])
            tdat = &bwtd[vbank][(unsigned)btm[0][tile] * 16];
        else
            tdat = &bwtd[vbank][(int)(int8_t)btm[0][tile] * 16];

        int b1, b2;

        if (flags & (1 << 6))
        {
            b1 = tdat[(7 - ry) * 2];
            b2 = tdat[(7 - ry) * 2 + 1];
        }
        else
        {
            b1 = tdat[ry * 2];
            b2 = tdat[ry * 2 + 1];
        }

        for (int rx = 0; rx < 8; rx++)
        {
            int val, mask;

            if (flags & (1 << 5))
                mask = 1 << rx;
            else
                mask = 1 << (7 - rx);

            val = !!(b1 & mask) + !!(b2 & mask) * 2;

            vidmem[line + bx + rx] = pal2rgb(pal[val]);
        }

        tile++;
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

    if (skip_this && (line < 143))
        return;
    else if (line == 143)
    {
        if (skip_this)
        {
            if (++frameskip_skip_i >= frameskip_skip)
            {
                frameskip_skip_i = 0;
                skip_this = 0;
                return;
            }
        }
        else
        {
            if (++frameskip_draw_i >= frameskip_draw)
            {
                frameskip_draw_i = 0;
                if (frameskip_skip)
                    skip_this = 1;
            }
        }
    }

    struct
    {
        uint8_t y, x;
        uint8_t num;
        uint8_t flags;
    } __attribute__((packed)) *soam = (void *)oam;
    int sx = io_state.scx, sy = io_state.scy;
    int abs_line = (line + sy) & 0xFF;
    int window_active = ((io_state.lcdc & (1 << 5)) && (io_state.wx >= 7) && (io_state.wx <= 166) && (io_state.wy <= line));

    if (!(io_state.lcdc & (1 << 0)))
        memset(vidmem + abs_line * 256, 0, 256 * 4);
    else
        draw_bg_line(abs_line, 0 << 7, window_active);

    if (window_active)
    {
        int wx = io_state.wx + sx - 7, wy = io_state.wy;
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
            uint16_t *pal;
            if (has_cgb)
            {
                vbank = !!(flags & (1 << 3));
                pal = &bpalette[(flags & 7) * 4];
            }
            else
            {
                vbank = 0;
                pal = bpalette;
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

                vidmem[abs_line * 256 + ((bx + rx + wx) & 0xFF)] = pal2rgb(pal[val]);
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
            uint16_t *pal;
            if (has_cgb)
                pal = &opalette[(flags & 7) * 4];
            else
                pal = &opalette[(flags & (1 << 4)) >> 2];

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

            bx += sx;
            by += sy;

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
                    vidmem[abs_line * 256 + ((bx + rx) & 0xFF)] = pal2rgb(pal[val]);
                else if (val)
                {
                    if (!(vidmem[abs_line * 256 + ((bx + rx) & 0xFF)] & 0xFF000000))
                        vidmem[abs_line * 256 + ((bx + rx) & 0xFF)] = pal2rgb(pal[val]);
                }
            }
        }
    }

    if ((io_state.lcdc & (1 << 0)) && has_cgb)
        draw_bg_line(abs_line, 1 << 7, window_active);

    if (line == 143)
    {
        handle_input_events();

        #ifdef DISPLAY_ALL
        for (int rem = 144; rem < 256; rem++)
            draw_line(rem);
        #endif
    }


    //void os_draw_line(int offx, int offy, int line)
    int py = ((line + sy) & 0xFF) << 8;
    int rline = line;

    line *= WIDTH;

    SDL_LockSurface(screen);

    for (int x = 0; x < WIDTH; x++)
    {
        int px = (x + sx) & 0xFF;
        uint32_t col = vidmem[py + px];
        ((uint32_t *)screen->pixels)[line + x] = SDL_MapRGB(screen->format, (col & 0xFF0000) >> 16, (col & 0xFF00) >> 8, col & 0xFF);
    }

    SDL_UnlockSurface(screen);

    if (rline == HEIGHT - 1)
        SDL_UpdateRect(screen, 0, 0, 0, 0);
}

void increase_frameskip(void)
{
    if (!frameskip_skip)
        frameskip_skip = 1;
    else if (frameskip_draw > 1)
        frameskip_draw /= 2;
    else if (frameskip_skip < 5)
        frameskip_skip++;
    else
    {
        frameskip_skip = 0;
        frameskip_draw = 16;
    }
}

void decrease_frameskip(void)
{
    if (!frameskip_skip)
    {
        frameskip_skip = 5;
        frameskip_draw = 1;
    }
    else if (frameskip_skip > 1)
        frameskip_skip--;
    else if (frameskip_draw < 16)
        frameskip_draw *= 2;
    else
        frameskip_skip = 0;
}
