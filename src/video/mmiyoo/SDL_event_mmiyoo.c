/*
  Customized version for Miyoo-Mini handheld.
  Only tested under Miyoo-Mini stock OS (original firmware) with Parasyte compatible layer.

  Copyright (C) 1997-2022 Sam Lantinga <slouken@libsdl.org>
  Copyright (C) 2022-2022 Steward Fu <steward.fu@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute itqpte
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_MMIYOO

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include "../../events/SDL_events_c.h"
#include "../../core/linux/SDL_evdev.h"
#include "../../thread/SDL_systhread.h"
#include <stdbool.h>

#include "../../cfg/SDL_picocfg_mmiyoo.h"
#include "SDL_video_mmiyoo.h"
#include "SDL_event_mmiyoo.h"

MMIYOO_EventInfo MMiyooEventInfo = {0};

extern PICO pico;
extern MMIYOO_VideoInfo MMiyooVideoInfo;

static int running = 0;
static int event_fd = -1;
static int is_stock_system = 0;
static SDL_sem *event_sem = NULL;
static SDL_Thread *thread = NULL;
static uint32_t pre_keypad_bitmaps = 0;
int volume_inc(void);
int volume_dec(void);

int specialKey = 0;
int r2Hold = 0;

uint32_t hotkey = 0;

static SDL_Scancode code[KEY_COUNT];

void drawStateHandler(int action) {
    switch (action) {
        case 1:
            pico.state.refresh_bezel = 8;
            
            if (MMiyooEventInfo.mode == MMIYOO_MOUSE_MODE) {
                pico.state.draw_mouse = 8;
            }
            
            break;
        case 2:
            pico.state.oc_changed = 8;
            break;
        case 3:
            pico.state.draw_mouse = 8;
            break;
        case 4:
            break;
        default:
            printf("error in drawStateHandler");
            break;
    }
    pico.state.alpha_draw = 30;
    pico.state.wait_frame = 30;
    pico.state.push_update = 8;
}

void initializeKeyCodeArray() {
    code[0] = pico.customkey.UpDpad;
    code[1] = pico.customkey.DownDpad;
    code[2] = pico.customkey.LeftDpad;
    code[3] = pico.customkey.RightDpad;
    code[4] = pico.customkey.A;
    code[5] = pico.customkey.B;
    code[6] = pico.customkey.X;
    code[7] = pico.customkey.Y;
    code[8] = pico.customkey.L1;
    code[9] = pico.customkey.R1;
    code[10] = pico.customkey.L2;
    code[11] = pico.customkey.R2;
    code[12] = pico.customkey.Select;
    code[13] = pico.customkey.Start;
    code[14] = pico.customkey.Menu;
    code[15] = SDLK_HOME;
    code[16] = SDLK_0;
    code[17] = SDLK_1;
}

void resetInputStates() {
    
    uint32_t bitmap;
   
    bitmap = MMiyooEventInfo.keypad.bitmaps;
    for (int key = 0; key <= MYKEY_LAST_BITS; key++) {
        if (bitmap & (1 << key)) {
            SDL_SendKeyboardKey(SDL_RELEASED, SDL_GetScancodeFromKey(code[key]));
        }
    }

    MMiyooEventInfo.keypad.bitmaps = 0;

    if (specialKey) {
        SDL_SendKeyboardKey(SDL_RELEASED, specialKey);
        specialKey = 0;
    }
}

void updateClockOnEvent(int adjust) {
    int currentClock = get_cpuclock();
    int newclock = currentClock + adjust;

    if (newclock > pico.perf.maxcpu) {
        // printf("Maximum Clock of %d MHz reached. Not increasing further.\n", pico.perf.maxcpu);
        newclock = pico.perf.maxcpu;
    }

    if (newclock < pico.perf.mincpu) {
        // printf("Minimum Clock of %d MHz reached. Not decreasing further.\n", pico.perf.mincpu);
        newclock = pico.perf.mincpu;
    }

    // printf("Current Clock: %d MHz\n", currentClock);

    if (currentClock != newclock) {
        // printf("Updating Clock to %d MHz\n", newclock);
        set_cpuclock(newclock);
        pico.perf.cpuclock = newclock;
        drawStateHandler(2);
    }
}

void updateBezelOnEvent(int direction) {
    if (pico.state.integer_bezel) {
        if (direction > 0) {
            pico.res.current_integer_bezel_id = (pico.res.current_integer_bezel_id + 1) % pico.res.total_integer_bezels_loaded;
        } else {
            pico.res.current_integer_bezel_id = (pico.res.current_integer_bezel_id - 1 + pico.res.total_integer_bezels_loaded) % pico.res.total_integer_bezels_loaded;
        }
    } else {
        if (direction > 0) {
            pico.res.current_bezel_id = (pico.res.current_bezel_id + 1) % pico.res.total_bezels_loaded;
        } else {
            pico.res.current_bezel_id = (pico.res.current_bezel_id - 1 + pico.res.total_bezels_loaded) % pico.res.total_bezels_loaded;
        }
    }
    drawStateHandler(1);
}

void sendSpecial() { // manage sending CTRL etc
    if (specialKey) {
        SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_LCTRL);
        SDL_SendKeyboardKey(SDL_PRESSED, specialKey);
        
        SDL_SendKeyboardKey(SDL_RELEASED, specialKey);

        SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_LCTRL);       
        specialKey = 0;
    }
}

void modeSwitch() { // toggle func for mouse/keypad mode
    MMiyooEventInfo.mode = (MMiyooEventInfo.mode == MMIYOO_MOUSE_MODE) ? MMIYOO_KEYPAD_MODE : MMIYOO_MOUSE_MODE;
    if (MMiyooEventInfo.mode == MMIYOO_MOUSE_MODE) {
        resetInputStates();
        MMiyooEventInfo.mouse.x = pico.state.lastMouseX;
        MMiyooEventInfo.mouse.y = pico.state.lastMouseY;
        drawStateHandler(3);
        printf("Mode switched to MOUSE\n");
    } else {
        resetInputStates();
        pico.state.lastMouseX = MMiyooEventInfo.mouse.x;
        pico.state.lastMouseY = MMiyooEventInfo.mouse.y;
        drawStateHandler(1);
        printf("Mode switched to KEYPAD\n");
    }
}

void handleR2Event(struct input_event ev) { // R2 for speeding up the mouse
    static int copy = 0;
    static int isCopied = 0;
    
    if(ev.value == 1) {
        r2Hold = 1;
        if (!isCopied && pico.customkey.R2 != 0) {
            copy = pico.customkey.R2;
            pico.customkey.R2 = 0;
            isCopied = 1;
        }
    } else if(ev.value == 0) {
        r2Hold = 0;
        if (isCopied) {
            pico.customkey.R2 = copy;
            isCopied = 0; 
        }
    }
}

void updateMousePosition(int xDirection, int yDirection) {
    int xIncrement = (int)(pico.mouse.incrementModifier * pico.mouse.scaleFactor * pico.mouse.acceleration);
    int yIncrement = (int)(pico.mouse.incrementModifier * pico.mouse.scaleFactor * pico.mouse.acceleration);

    if (r2Hold == 1) {
        xIncrement *= 2;
        yIncrement *= 2;
    }

    pico.mouse.acceleration *= pico.mouse.accelerationRate;
    if (pico.mouse.acceleration > pico.mouse.maxAcceleration) {
        pico.mouse.acceleration = pico.mouse.maxAcceleration;
    }

    if (xDirection < 0) {
        MMiyooEventInfo.mouse.x -= xIncrement;
    } else if (xDirection > 0) {
        MMiyooEventInfo.mouse.x += xIncrement;
    }

    if (yDirection < 0) {
        MMiyooEventInfo.mouse.y -= yIncrement;
    } else if (yDirection > 0) {
        MMiyooEventInfo.mouse.y += yIncrement;
    }

    if (MMiyooEventInfo.mouse.y < MMiyooEventInfo.mouse.miny) {
        MMiyooEventInfo.mouse.y = MMiyooEventInfo.mouse.miny;
    }
    if (MMiyooEventInfo.mouse.y > MMiyooEventInfo.mouse.maxy) {
        MMiyooEventInfo.mouse.y = MMiyooEventInfo.mouse.maxy;
    }
    if (MMiyooEventInfo.mouse.x < MMiyooEventInfo.mouse.minx) {
        MMiyooEventInfo.mouse.x = MMiyooEventInfo.mouse.minx;
    }
    if (MMiyooEventInfo.mouse.x >= MMiyooEventInfo.mouse.maxx) {
        MMiyooEventInfo.mouse.x = MMiyooEventInfo.mouse.maxx;
    }
        
    // printf("Updated X-coordinate: %d, Y-coordinate: %d\n", MMiyooEventInfo.mouse.x, MMiyooEventInfo.mouse.y);
}

// void processHotkeys(uint32_t keyBitmaps) {
    // if (!(keyBitmaps & (1 << MYKEY_SELECT))) {
        // return;
    // }

    // if (keyBitmaps & (1 << MYKEY_MENU)) {
        // specialKey = SDL_SCANCODE_Q; 
        // running = 0; // Quit
    // } else if (keyBitmaps & (1 << MYKEY_L1)) {
        // specialKey = SDL_SCANCODE_R; // Reload the game
    // } else if (keyBitmaps & (1 << MYKEY_UP)) {
        // updateClockOnEvent(pico.perf.cpuclockincrement); // Overclock increase
    // } else if (keyBitmaps & (1 << MYKEY_DOWN)) {
        // updateClockOnEvent(-pico.perf.cpuclockincrement); // Overclock decrease
    // } else if (keyBitmaps & (1 << MYKEY_LEFT)) {
        // updateBezelOnEvent(-1); // Bezel next
    // } else if (keyBitmaps & (1 << MYKEY_RIGHT)) {
        // updateBezelOnEvent(1); // Bezel previous
    // } else if (keyBitmaps & (1 << MYKEY_R1)) {
        // pico.state.screen_scaling = (pico.state.screen_scaling % 3) + 1;
        // pico.state.integer_bezel = (pico.state.screen_scaling == 2) ? 1 : 0;
        // drawStateHandler(1); // Change scaling
    // }

    // Clear the processed hotkey bits
    // keyBitmaps &= ~((1 << MYKEY_MENU) | (1 << MYKEY_L1) | (1 << MYKEY_UP) | 
                    // (1 << MYKEY_DOWN) | (1 << MYKEY_LEFT) | (1 << MYKEY_RIGHT) | 
                    // (1 << MYKEY_R1));
// }

void flushEvents(void) {
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
}

int EventUpdate(void *data);

void MMIYOO_EventInit(void)
{
    DIR *dir = NULL;

    pre_keypad_bitmaps = 0;
    memset(&MMiyooEventInfo, 0, sizeof(MMiyooEventInfo));
    MMiyooEventInfo.mouse.minx = 38;
    MMiyooEventInfo.mouse.miny = 0;
    MMiyooEventInfo.mouse.maxx = 278;
    MMiyooEventInfo.mouse.maxy = 240;
    MMiyooEventInfo.mouse.x = pico.state.lastMouseX = 160;
    MMiyooEventInfo.mouse.y = pico.state.lastMouseY = 120;
    MMiyooEventInfo.mode = MMIYOO_KEYPAD_MODE;

#if defined(MMIYOO)
    event_fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if(event_fd < 0){
        printf("failed to open /dev/input/event0\n");
    }
#endif

    if((event_sem =  SDL_CreateSemaphore(1)) == NULL) {
        SDL_SetError("Can't create input semaphore");
        return;
    }

    running = 1;
    if((thread = SDL_CreateThreadInternal(EventUpdate, "MMIYOOInputThread", 4096, NULL)) == NULL) {
        SDL_SetError("Can't create input thread");
        return;
    }

    initializeKeyCodeArray();

    dir = opendir("/mnt/SDCARD/.tmp_update");
    if (dir) {
        closedir(dir);
    }
    else {
        is_stock_system = 1;
        printf("Run on stock system\n");
    }
}

void MMIYOO_EventDeinit(void)
{
    running = 0;
    SDL_WaitThread(thread, NULL);
    SDL_DestroySemaphore(event_sem);
    if(event_fd > 0) {
        close(event_fd);
        event_fd = -1;
    }
}

int EventUpdate(void *data)
{

    struct input_event ev = {0};
    uint32_t bit = 0;

    while (running) { 
    
        static int bezelRedrawn = 0;  

        if (pico.state.oc_decay > 0) {
            pico.state.oc_decay--;
            bezelRedrawn = 0; 
        } else if (!bezelRedrawn) {
            drawStateHandler(1);
            bezelRedrawn = 1;
        }
        
        SDL_SemWait(event_sem);
        if (event_fd > 0) {
            if (read(event_fd, &ev, sizeof(struct input_event))) {
                if ((ev.type == EV_KEY) && (ev.value != 2)) {
                    //printf("%s, code:%d\n", __func__, ev.code);

                    switch (ev.code) {
                        case 18:  bit = (1 << MYKEY_L1);      break;
                        case 15:  
                            bit = (1 << MYKEY_L2);
                            if (ev.value == 1) { 
                                modeSwitch();
                            }
                            break;
                        case 20:  bit = (1 << MYKEY_R1);      break;     
                        case 14:
                            bit = (1 << MYKEY_R2);
                            if (MMiyooEventInfo.mode == MMIYOO_MOUSE_MODE) {
                                handleR2Event(ev);
                            }
                            break;
                        case 103: bit = (1 << MYKEY_UP);      
                            break;
                        case 108: bit = (1 << MYKEY_DOWN);    
                            break;
                        case 105: bit = (1 << MYKEY_LEFT);    
                            break;
                        case 106: bit = (1 << MYKEY_RIGHT);   
                            break;
                        case 57:  bit = (1 << MYKEY_A);       break;
                        case 29:  bit = (1 << MYKEY_B);       break;
                        case 42:  bit = (1 << MYKEY_X);       break;
                        case 56:  bit = (1 << MYKEY_Y);       break;
                        case 28:  bit = (1 << MYKEY_START);   break;
                        case 97:  bit = (1 << MYKEY_SELECT);  break;
                        case 1:   bit = (1 << MYKEY_MENU);    break;
                        case 116: bit = (1 << MYKEY_POWER);   break;
                        case 114: bit = (1 << MYKEY_VOLDOWN); break;
                        case 115: bit = (1 << MYKEY_VOLUP);
                    }
                    
                    if(bit){
                        if(ev.value){
                            MMiyooEventInfo.keypad.bitmaps|= bit;
                        }
                        else{
                            MMiyooEventInfo.keypad.bitmaps&= ~bit;
                        }
                    }
                                                            
                    hotkey = MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_SELECT);
                    // processHotkeys(MMiyooEventInfo.keypad.bitmaps);
                    
                    // refactor this, it's ugly

                    if (hotkey && (MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_MENU))) { // Quits
                        specialKey = SDL_SCANCODE_Q; 
                        MMiyooEventInfo.keypad.bitmaps &= ~(1 << MYKEY_MENU);
                        hotkey = 0;
                        running = 0;
                    } else if (hotkey && (MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_L1))) { // reload the game
                        specialKey = SDL_SCANCODE_R; 
                        MMiyooEventInfo.keypad.bitmaps &= ~(1 << MYKEY_L1);
                    } else if (hotkey && (MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_UP))) { // overclock increase
                        updateClockOnEvent(pico.perf.cpuclockincrement);
                        MMiyooEventInfo.keypad.bitmaps &= ~(1 << MYKEY_UP);
                    } else if (hotkey && (MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_DOWN))) { // overclock decrease
                        updateClockOnEvent(-pico.perf.cpuclockincrement);
                        MMiyooEventInfo.keypad.bitmaps &= ~(1 << MYKEY_DOWN);
                    } else if (hotkey && (MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_LEFT))) { // bezel next
                        updateBezelOnEvent(-1);
                        MMiyooEventInfo.keypad.bitmaps &= ~(1 << MYKEY_LEFT);
                    } else if (hotkey && (MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_RIGHT))) { // bezel previous
                        updateBezelOnEvent(1);
                        MMiyooEventInfo.keypad.bitmaps &= ~(1 << MYKEY_RIGHT);
                    } else if (hotkey && (MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_R1))) { // change scaling
                        pico.state.screen_scaling = (pico.state.screen_scaling % 3) + 1;

                        if (pico.state.screen_scaling == 2) {
                            pico.state.integer_bezel = 1;
                        } else {
                            pico.state.integer_bezel = 0;
                        }

                        drawStateHandler(1);
                        MMiyooEventInfo.keypad.bitmaps &= ~(1 << MYKEY_R1);
                    }

                }
            }
        }
        SDL_SemPost(event_sem);
        usleep(1000000 / 120);
    }
    return 0;
}

void MMIYOO_PumpEvents(_THIS)

// Square = Z / C / N 
// X = X / V / M            pico keybinds default
// Pause = Enter

{
   
    int updated = 0;
    int xDirection, yDirection;
    
    SDL_SemWait(event_sem);
    
    if (specialKey) { 
        sendSpecial();
    }
                
    if (MMiyooEventInfo.mode == MMIYOO_KEYPAD_MODE) {
        if (pre_keypad_bitmaps != MMiyooEventInfo.keypad.bitmaps) {
            int cc = 0;
            uint32_t v0 = pre_keypad_bitmaps;
            uint32_t v1 = MMiyooEventInfo.keypad.bitmaps;

            for (cc=0; cc<=MYKEY_LAST_BITS; cc++) {              
                if ((v0 & 1) != (v1 & 1)) {
                    // printf("Key %d: %s\n", cc, (v1 & 1) ? "Pressed" : "Released");
                    SDL_SendKeyboardKey((v1 & 1) ? SDL_PRESSED : SDL_RELEASED, SDL_GetScancodeFromKey(code[cc]));
                }
                v0>>= 1;
                v1>>= 1;
            }
            
            pre_keypad_bitmaps = MMiyooEventInfo.keypad.bitmaps;
        }
    } else {
        if (pre_keypad_bitmaps != MMiyooEventInfo.keypad.bitmaps) {
            uint32_t cc = 0;
            uint32_t v0 = pre_keypad_bitmaps;
            uint32_t v1 = MMiyooEventInfo.keypad.bitmaps;
                       
            if ((v0 & (1 << MYKEY_A)) != (v1 & (1 << MYKEY_A))) {
                SDL_SendMouseButton(MMiyooVideoInfo.window, 0, (v1 & (1 << MYKEY_A)) ? SDL_PRESSED : SDL_RELEASED, SDL_BUTTON_LEFT);
            }
            
            if ((v0 & (1 << MYKEY_B)) != (v1 & (1 << MYKEY_B))) {
                SDL_SendMouseButton(MMiyooVideoInfo.window, 0, (v1 & (1 << MYKEY_B)) ? SDL_PRESSED : SDL_RELEASED, SDL_BUTTON_RIGHT);
            }
            
            for (cc=0; cc<=MYKEY_LAST_BITS; cc++) {
                if ((cc == MYKEY_SELECT) || (cc == MYKEY_R2)) {
                    if ((v0 & 1) != (v1 & 1)) {
                        // printf("Special Key %d: %s\n", cc, (v1 & 1) ? "Pressed" : "Released");
                        SDL_SendKeyboardKey((v1 & 1) ? SDL_PRESSED : SDL_RELEASED, SDL_GetScancodeFromKey(code[cc]));
                        pico.state.wait_frame = 30;
                    }
                }
                v0>>= 1;
                v1>>= 1;
            }
        }
        
        xDirection = 0;
        yDirection = 0;

        if (MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_UP)) {
            yDirection -= 1;
        }
        if (MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_DOWN)) {
            yDirection += 1;
        }
        if (MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_LEFT)) {
            xDirection -= 1;
        }
        if (MMiyooEventInfo.keypad.bitmaps & (1 << MYKEY_RIGHT)) {
            xDirection += 1;
        }

        if (xDirection != 0 || yDirection != 0) {
            updated = 1;
            updateMousePosition(xDirection, yDirection);
        }
        
        if (updated) {
            SDL_SendMouseMotion(MMiyooVideoInfo.window, 0, 0, MMiyooEventInfo.mouse.x, MMiyooEventInfo.mouse.y);
        } 
        
        pre_keypad_bitmaps = MMiyooEventInfo.keypad.bitmaps;
    }
    
    SDL_SemPost(event_sem);
}


#endif

