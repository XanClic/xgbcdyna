#include <stdlib.h>

#include <SDL.h>

#include "xgbc.h"

extern struct io io_state;

static unsigned keys_were = 0;
uint8_t keystates = 0;

static void update_keyboard(void)
{
    unsigned diff_key = keystates ^ keys_were;
    unsigned value = io_state.p1 & (KEY_DIR | KEY_OTH);
    unsigned relevant = 0;

    if (!(value & KEY_DIR))
    {
        value |= ~((keystates & 0xF0) >> 4);
        relevant |= 0xF0;
    }
    if (!(value & KEY_OTH))
    {
        value |= ~(keystates & 0x0F);
        relevant |= 0x0F;
    }

    io_state.p1 = value;

    diff_key &= relevant;
    if (keystates & diff_key)
        io_state.int_flag |= INT_P10_P13;

    keys_were = keystates;
}

void handle_input_events(void)
{
    SDL_Event evt;
    unsigned new_keystate = keystates;

    while (SDL_PollEvent(&evt))
    {
        switch ((unsigned)evt.type)
        {
            case SDL_QUIT:
                exit(0);
                break;
            case SDL_KEYDOWN:
                switch ((unsigned)evt.key.keysym.sym)
                {
                    case SDLK_a:
                        new_keystate |= VK_A;
                        break;
                    case SDLK_b:
                        new_keystate |= VK_B;
                        break;
                    case SDLK_RETURN:
                        new_keystate |= VK_START;
                        break;
                    case SDLK_s:
                        new_keystate |= VK_SELECT;
                        break;
                    case SDLK_LEFT:
                        new_keystate |= VK_LEFT;
                        break;
                    case SDLK_RIGHT:
                        new_keystate |= VK_RIGHT;
                        break;
                    case SDLK_UP:
                        new_keystate |= VK_UP;
                        break;
                    case SDLK_DOWN:
                        new_keystate |= VK_DOWN;
                        break;
                }
                break;
            case SDL_KEYUP:
                switch ((unsigned)evt.key.keysym.sym)
                {
                    case SDLK_a:
                        new_keystate &= ~VK_A;
                        break;
                    case SDLK_b:
                        new_keystate &= ~VK_B;
                        break;
                    case SDLK_RETURN:
                        new_keystate &= ~VK_START;
                        break;
                    case SDLK_s:
                        new_keystate &= ~VK_SELECT;
                        break;
                    case SDLK_LEFT:
                        new_keystate &= ~VK_LEFT;
                        break;
                    case SDLK_RIGHT:
                        new_keystate &= ~VK_RIGHT;
                        break;
                    case SDLK_UP:
                        new_keystate &= ~VK_UP;
                        break;
                    case SDLK_DOWN:
                        new_keystate &= ~VK_DOWN;
                        break;
                }
        }
    }

    if (new_keystate != keystates)
    {
        keystates = new_keystate;
        update_keyboard();
    }
}
