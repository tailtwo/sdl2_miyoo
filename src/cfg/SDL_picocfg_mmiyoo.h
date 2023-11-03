#ifndef __SDL_PICOCFG_MMIYOO_H__
#define __SDL_PICOCFG_MMIYOO_H__

#include <SDL2/SDL.h>
#include "../../cfg/SDL_picocfg_mmiyoo.h"

#ifndef MAX_PATH
    #define MAX_PATH 255
#endif

#define PICO_CFG_PATH                    "cfg/onioncfg.json"

#define MMIYOO_DEFAULT_KEY_L2 SDLK_q
#define MMIYOO_DEFAULT_KEY_L1 SDLK_e
#define MMIYOO_DEFAULT_KEY_UpDpad SDLK_UP
#define MMIYOO_DEFAULT_KEY_DownDpad SDLK_DOWN
#define MMIYOO_DEFAULT_KEY_LeftDpad SDLK_LEFT
#define MMIYOO_DEFAULT_KEY_RightDpad SDLK_RIGHT
#define MMIYOO_DEFAULT_KEY_R2 SDLK_p
#define MMIYOO_DEFAULT_KEY_R1 SDLK_t
#define MMIYOO_DEFAULT_KEY_A SDLK_SPACE
#define MMIYOO_DEFAULT_KEY_B SDLK_BACKSPACE
#define MMIYOO_DEFAULT_KEY_X SDLK_x
#define MMIYOO_DEFAULT_KEY_Y SDLK_y
#define MMIYOO_DEFAULT_KEY_Select SDLK_m
#define MMIYOO_DEFAULT_KEY_Start SDLK_RETURN
#define MMIYOO_DEFAULT_KEY_MENU SDLK_ESCAPE
#define MMIYOO_DEFAULT_CPU_CLOCK 1300
#define MMIYOO_MAX_CPU_CLOCK 1800
#define MMIYOO_MIN_CPU_CLOCK 600
#define MMIYOO_DEFAULT_CPU_CLOCK_INCREMENT 25
#define MMIYOO_MAX_CPU_CLOCK_INCREMENT 100
#define MMIYOO_DEFAULT_SCALE_FACTOR 2
#define MMIYOO_DEFAULT_ACCELERATION 2.0
#define MMIYOO_DEFAULT_ACCELERATION_RATE 1.5
#define MMIYOO_DEFAULT_MAX_ACCELERATION 5
#define MMIYOO_DEFAULT_INCREMENT_MODIFIER 0.1

SDL_Keycode stringToKeycode(const char *keyString);
int read_pico_config(void);
int write_pico_config(void);

typedef struct _CUSTKEY {
    SDL_Keycode A;
    SDL_Keycode B;
    SDL_Keycode X;
    SDL_Keycode Y;
    SDL_Keycode L1;
    SDL_Keycode L2;
    SDL_Keycode R1;
    SDL_Keycode R2;
    SDL_Keycode LeftDpad;
    SDL_Keycode RightDpad;
    SDL_Keycode UpDpad;
    SDL_Keycode DownDpad;
    SDL_Keycode Start;
    SDL_Keycode Select;
    SDL_Keycode Menu;
} CUSTKEY;

typedef struct _MOUSE {
    int scaleFactor;
    float acceleration;
    float accelerationRate;
    float maxAcceleration;
    float incrementModifier;
} MOUSE;

typedef struct _PICO {
    int cpuclock;
    int cpuclockincrement;
    char cfg_path[MAX_PATH];
    CUSTKEY customkey;
    MOUSE mouse;
} PICO;

#endif