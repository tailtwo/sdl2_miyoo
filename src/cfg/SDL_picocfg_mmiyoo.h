#ifndef __SDL_PICOCFG_MMIYOO_H__
#define __SDL_PICOCFG_MMIYOO_H__

#include <SDL2/SDL.h>
#include "../../cfg/SDL_picocfg_mmiyoo.h"

#ifndef MAX_PATH
    #define MAX_PATH 255
#endif

#define PICO_CFG_PATH                    "cfg/onioncfg.json"

#define MMIYOO_DEFAULT_KEY_L2 SDLK_d
#define MMIYOO_DEFAULT_KEY_L1 SDLK_d
#define MMIYOO_DEFAULT_KEY_UpDpad SDLK_UP
#define MMIYOO_DEFAULT_KEY_DownDpad SDLK_DOWN
#define MMIYOO_DEFAULT_KEY_LeftDpad SDLK_LEFT
#define MMIYOO_DEFAULT_KEY_RightDpad SDLK_RIGHT
#define MMIYOO_DEFAULT_KEY_R2 SDLK_d
#define MMIYOO_DEFAULT_KEY_R1 SDLK_d
#define MMIYOO_DEFAULT_KEY_A SDLK_z
#define MMIYOO_DEFAULT_KEY_B SDLK_x
#define MMIYOO_DEFAULT_KEY_X SDLK_ESCAPE
#define MMIYOO_DEFAULT_KEY_Y SDLK_d
#define MMIYOO_DEFAULT_KEY_Select SDLK_m
#define MMIYOO_DEFAULT_KEY_Start SDLK_RETURN
#define MMIYOO_DEFAULT_KEY_MENU SDLK_ESCAPE
#define MMIYOO_DEFAULT_CPU_CLOCK 1300
#define MMIYOO_MAX_CPU_CLOCK 1700
#define MMIYOO_MIN_CPU_CLOCK 600
#define MMIYOO_DEFAULT_CPU_CLOCK_INCREMENT 25
#define MMIYOO_MAX_CPU_CLOCK_INCREMENT 100
#define MMIYOO_DEFAULT_SCALE_FACTOR 2
#define MMIYOO_DEFAULT_ACCELERATION 2.0
#define MMIYOO_DEFAULT_ACCELERATION_RATE 1.5
#define MMIYOO_DEFAULT_MAX_ACCELERATION 5
#define MMIYOO_DEFAULT_INCREMENT_MODIFIER 0.1
#define DIGIT_PATH "res/digit"
#define BORDER_PATH "res/border"
#define MAX_BORDERS 256
#define DEFAULT_BORDER_IMAGE "res/border/def_border.png"
#define DEFAULT_BORDER_ID 0
#define PICO_WINDOW_W 320
#define PICO_WINDOW_H 240

SDL_Keycode stringToKeycode(const char *keyString);
int picoConfigRead(void);
int picoConfigWrite(void);

typedef struct _CUSTKEY {
    SDL_Keycode A, B, X, Y, L1, L2, R1, R2, LeftDpad, RightDpad, UpDpad, DownDpad, Start, Select, Menu;
} CUSTKEY;

typedef struct _MOUSE {
    int scaleFactor;
    float acceleration;
    float accelerationRate;
    float maxAcceleration;
    float incrementModifier;
} MOUSE;

typedef struct _STATE {
    int oc_changed;
    int oc_decay;
    int push_update;
    int refresh_border;
} STATE;

typedef struct _PICO {
    int cpuclock;
    int cpuclockincrement;
    char cfg_path[MAX_PATH];
    CUSTKEY customkey;
    MOUSE mouse;
    STATE state;
    SDL_Surface *digit[10];
    SDL_Surface *border[MAX_BORDERS];
    int current_border_id;
    int total_borders_loaded;
} PICO;


#endif