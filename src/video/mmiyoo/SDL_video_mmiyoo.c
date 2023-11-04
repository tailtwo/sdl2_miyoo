/*
  Customized version for Miyoo-Mini handheld.
  Only tested under Miyoo-Mini stock OS (original firmware) with Parasyte compatible layer.

  Copyright (C) 1997-2022 Sam Lantinga <slouken@libsdl.org>
  Copyright (C) 2022-2022 Steward Fu <steward.fu@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
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

#include <dirent.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <json-c/json.h>

#include "../../cfg/SDL_picocfg_mmiyoo.h"
#include "../../events/SDL_events_c.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"

#include "SDL_image.h"
#include "SDL_version.h"
#include "SDL_syswm.h"
#include "SDL_loadso.h"
#include "SDL_events.h"
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "SDL_video_mmiyoo.h"
#include "SDL_event_mmiyoo.h"
#include "SDL_opengles_mmiyoo.h"
#include "SDL_framebuffer_mmiyoo.h"
#include "neon.h"

#include "hex_pen.h"

MMIYOO_VideoInfo MMiyooVideoInfo={0};
extern PICO pico;
extern MMIYOO_EventInfo MMiyooEventInfo;

static GFX gfx = {0};
static int MMIYOO_VideoInit(_THIS);
static int MMIYOO_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode);
static void MMIYOO_VideoQuit(_THIS);

int fileExists(const char *path) {
    FILE *file;
    if ((file = fopen(path, "r"))) {
        fclose(file);
        return 1;
    }
    return 0;
}

int preloadResources() {
    char buf[255] = {0};
    SDL_Surface *t = NULL;
    SDL_Surface *tmp = SDL_CreateRGBSurface(0, 32, 32, 32, 0, 0, 0, 0);
    
    if (tmp) {
        DIR *dir; 
        struct dirent *ent;
        int id = 0;
        char filepath[PATH_MAX];
        SDL_Surface *converted_surface;
        pico.total_borders_loaded = 0;

        if ((dir = opendir(BORDER_PATH)) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_type == DT_DIR) continue;
                snprintf(filepath, sizeof(filepath), "%s/%s", BORDER_PATH, ent->d_name);
                t = IMG_Load(filepath);
                if (t) {
                    converted_surface = SDL_ConvertSurface(t, tmp->format, 0);
                    SDL_FreeSurface(t);

                    if (!converted_surface) {
                        fprintf(stderr, "SDL_ConvertSurface failed: %s\n", SDL_GetError());
                        return -1;
                        continue;
                    }

                    if (strcmp(ent->d_name, "def_border.png") == 0) {
                        pico.border[0] = converted_surface;
                        printf("%s, default border loaded\n", __func__);
                    } else {
                        id++;
                        pico.border[id] = converted_surface;
                        printf("%s, %s loaded with ID %d\n", __func__, ent->d_name, id);
                        pico.total_borders_loaded++;
                    }
                } else {
                    fprintf(stderr, "IMG_Load failed: %s\n", SDL_GetError());
                    return -2;
                }
            }
            closedir(dir);
        } else {
            fprintf(stderr, "Could not open border directory: %s\n", BORDER_PATH);
            return -3;
        }

        for (int cc = 0; cc <= 9; ++cc) {
            sprintf(buf, "%s/%d.png", DIGIT_PATH, cc);
            if (fileExists(buf)) {
                t = IMG_Load(buf);
                if (t) {
                    pico.digit[cc] = SDL_ConvertSurface(t, tmp->format, 0);
                    SDL_FreeSurface(t);
                    printf("%s, %s loaded\n", __func__, buf);
                } else {
                    fprintf(stderr, "ERROR loading digit image: %s\n", buf);
                    pico.digit[cc] = NULL;
                    return -4;
                }
            } else {
                fprintf(stderr, "ERROR digit image file does not exist: %s\n", buf);
                pico.digit[cc] = NULL;
                return -5;
            }
        }
        SDL_FreeSurface(tmp);
    } else {
        fprintf(stderr, "ERROR creating RGB surface.\n");
    }
    return 0;
}

int drawBorderImage()
{
    SDL_Rect srt = {0};
    SDL_Rect drt = {0};
    SDL_Surface* border_surface;

    int border_id = pico.current_border_id;
    border_surface = pico.border[border_id];

    if (border_id < 0 || border_id >= MAX_BORDERS || !pico.border[border_id]) {
        printf("ERROR, no border image set for ID %d\n", border_id);
        pico.state.refresh_border = 0;
        return 1;
    }

    srt = (SDL_Rect){0, 0, border_surface->w, border_surface->h};
    drt = (SDL_Rect){0, 0, border_surface->w, border_surface->h};

    GFX_Copy(border_surface->pixels, srt, drt, border_surface->pitch, 0, E_MI_GFX_ROTATE_180);
    printf("%s, border %d drawn\n", __func__, border_id);

    return 0;
}

int drawCPUClock(int val, int num)
{
    SDL_Rect srt = {0};
    SDL_Rect drt = {0};
    SDL_Surface *p = NULL;
    
    if (num <= 0 || num > 10) {
        fprintf(stderr, "ERROR, invalid number of digits specified.\n");
        return 1;
    }

    for (int cc = 0; cc < num; cc++) {
        p = pico.digit[0];

        if (!p) {
            fprintf(stderr, "ERROR, digit image not set for value 0\n");
            pico.state.oc_changed = 0;
            return 1;
        }

        srt = (SDL_Rect){0, 0, p->w, p->h};
        drt = (SDL_Rect){640 - (p->w * (num - cc)), 0, p->w, p->h};

        GFX_Copy(p->pixels, srt, drt, p->pitch, 0, E_MI_GFX_ROTATE_180);
    }

    // Now draw the actual number, right to left
    for (int cc = num - 1; cc >= 0 && val > 0; cc--) {
        int digit = val % 10;
        val /= 10;

        p = pico.digit[digit];
        if (!p) {
            fprintf(stderr, "ERROR, digit image not set for value %d\n", digit);
            pico.state.oc_changed = 0;
            return 1;
        }

        srt = (SDL_Rect){0, 0, p->w, p->h};
        drt = (SDL_Rect){640 - (p->w * (cc + 1)), 0, p->w, p->h};

        GFX_Copy(p->pixels, srt, drt, p->pitch, 0, E_MI_GFX_ROTATE_180);
    }

    pico.state.oc_decay = 250;
    printf("%s, clock drawn\n", __func__);
    return 0;
}


int get_cpuclock(void)
{
    static const uint64_t divsrc = 432000000llu * 524288;

    int fd_mem = -1;
    void *pll_map = NULL;
    uint32_t rate = 0;
    uint32_t lpf_value = 0;
    uint32_t post_div = 0;

    fd_mem = open("/dev/mem", O_RDWR);
    if (fd_mem < 0) {
        return 0;
    }

    pll_map = mmap(0, PLL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, BASE_REG_MPLL_PA);
    if (pll_map) {
        volatile uint8_t* p8 = (uint8_t*)pll_map;
        volatile uint16_t* p16 = (uint16_t*)pll_map;

        lpf_value = p16[0x2a4] + (p16[0x2a6] << 16);
        post_div = p16[0x232] + 1;
        if (lpf_value == 0) {
            lpf_value = (p8[0x2c2 << 1] << 16) + (p8[0x2c1 << 1] << 8) + p8[0x2c0 << 1];
        }

        if (lpf_value && post_div) {
            rate = (divsrc / lpf_value * 2 / post_div * 16);
        }
        // printf("Current cpuclock=%u (lpf=%u, post_div=%u)\n", rate, lpf_value, post_div);
        munmap(pll_map, PLL_SIZE);
    }
    close(fd_mem);
    return rate / 1000000;
}

void write_file(const char* fname, char* str)
{
	int fd = open(fname, O_WRONLY);

	if (fd >= 0) {
        write(fd, str, strlen(str));
        close(fd);
    }
}

int set_cpuclock(uint32_t newclock)
{
    int fd_mem = -1;
    void *pll_map = NULL;
    uint32_t post_div = 0;
    char clockstr[16] = {0};
    const char fn_governor[] = "/sys/devices/system/cpu/cpufreq/policy0/scaling_governor";
    const char fn_setspeed[] = "/sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed";

    fd_mem = open("/dev/mem", O_RDWR);
    if (fd_mem < 0) {
        return -1;
    }

    pll_map = mmap(0, PLL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, BASE_REG_MPLL_PA);
    if (pll_map) {
        printf("Set cpuclock %dMHz\n", newclock);

        newclock*= 1000;
        sprintf(clockstr, "%d", newclock);
        write_file(fn_governor, "userspace");
        write_file(fn_setspeed, clockstr);

        if (newclock >= 800000) {
            post_div = 2;
        }
        else if (newclock >= 400000) {
            post_div = 4;
        }
        else if (newclock >= 200000) {
            post_div = 8;
        }
        else {
            post_div = 16;
        }

        if (1) {
            static const uint64_t divsrc = 432000000llu * 524288;
            uint32_t rate = (newclock * 1000) / 16 * post_div / 2;
            uint32_t lpf = (uint32_t)(divsrc / rate);
            volatile uint16_t* p16 = (uint16_t*)pll_map;
            uint32_t cur_post_div = (p16[0x232] & 0x0f) + 1;
            uint32_t tmp_post_div = cur_post_div;

            if (post_div > cur_post_div) {
                while (tmp_post_div != post_div) {
                    tmp_post_div <<= 1;
                    p16[0x232] = (p16[0x232] & 0xf0) | ((tmp_post_div - 1) & 0x0f);
                }
            }

            p16[0x2A8] = 0x0000;        // reg_lpf_enable = 0
            p16[0x2AE] = 0x000f;        // reg_lpf_update_cnt = 32
            p16[0x2A4] = lpf & 0xffff;  // set target freq to LPF high
            p16[0x2A6] = lpf >> 16;     // set target freq to LPF high
            p16[0x2B0] = 0x0001;        // switch to LPF control
            p16[0x2B2]|= 0x1000;        // from low to high
            p16[0x2A8] = 0x0001;        // reg_lpf_enable = 1
            while(!(p16[0x2ba] & 1));   // polling done
            p16[0x2A0] = lpf & 0xffff;  // store freq to LPF low
            p16[0x2A2] = lpf >> 16;     // store freq to LPF low

            if (post_div < cur_post_div) {
                while (tmp_post_div != post_div) {
                    tmp_post_div >>= 1;
                    p16[0x232] = (p16[0x232] & 0xf0) | ((tmp_post_div - 1) & 0x0f);
                }
            }
        }
        munmap(pll_map, PLL_SIZE);
    }
    close(fd_mem);
    return 0;
}

void GFX_Init(void)
{
    MI_SYS_Init();
    MI_GFX_Open();

    gfx.fd = open("/dev/fb0", O_RDWR);
    ioctl(gfx.fd, FBIOGET_FSCREENINFO, &gfx.finfo);
    gfx.fb.phyAddr = gfx.finfo.smem_start;
    ioctl(gfx.fd, FBIOGET_VSCREENINFO, &gfx.vinfo);
    gfx.vinfo.yoffset = 0;
    ioctl(gfx.fd, FBIOPUT_VSCREENINFO, &gfx.vinfo);
    MI_SYS_MemsetPa(gfx.fb.phyAddr, 0, FB_SIZE);
    MI_SYS_Mmap(gfx.fb.phyAddr, gfx.finfo.smem_len, &gfx.fb.virAddr, TRUE);
    memset(&gfx.hw.opt, 0, sizeof(gfx.hw.opt));

    MI_SYS_MMA_Alloc(NULL, TMP_SIZE, &gfx.tmp.phyAddr);
    MI_SYS_Mmap(gfx.tmp.phyAddr, TMP_SIZE, &gfx.tmp.virAddr, TRUE);

    MI_SYS_MMA_Alloc(NULL, TMP_SIZE, &gfx.overlay.phyAddr);
    MI_SYS_Mmap(gfx.overlay.phyAddr, TMP_SIZE, &gfx.overlay.virAddr, TRUE);
}

void GFX_Quit(void)
{
    gfx.vinfo.yoffset = 0;
    ioctl(gfx.fd, FBIOPUT_VSCREENINFO, &gfx.vinfo);
    close(gfx.fd);
    gfx.fd = 0;

    MI_SYS_Munmap(gfx.fb.virAddr, TMP_SIZE);
    MI_SYS_Munmap(gfx.tmp.virAddr, TMP_SIZE);
    MI_SYS_MMA_Free(gfx.tmp.phyAddr);
    MI_SYS_Munmap(gfx.overlay.virAddr, TMP_SIZE);
    MI_SYS_MMA_Free(gfx.overlay.phyAddr);
    MI_GFX_Close();
    MI_SYS_Exit();
}

void GFX_Clear(void)
{
    MI_SYS_MemsetPa(gfx.fb.phyAddr, 0, FB_SIZE);
    MI_SYS_MemsetPa(gfx.tmp.phyAddr, 0, TMP_SIZE);
}

int GFX_Copy(const void *pixels, SDL_Rect srcrect, SDL_Rect dstrect, int pitch, int alpha, int rotate)
{
    MI_U16 u16Fence = 0;

    if (pixels == NULL) {
        return -1;
    }
    neon_memcpy(gfx.tmp.virAddr, pixels, srcrect.h * pitch);
    
    gfx.hw.opt.u32GlobalSrcConstColor = 0;
    gfx.hw.opt.eRotate = rotate;
    gfx.hw.opt.eSrcDfbBldOp = E_MI_GFX_DFB_BLD_ONE;
    gfx.hw.opt.eDstDfbBldOp = 0;
    gfx.hw.opt.eDFBBlendFlag = 0;

    gfx.hw.src.rt.s32Xpos = srcrect.x;
    gfx.hw.src.rt.s32Ypos = srcrect.y;
    gfx.hw.src.rt.u32Width = srcrect.w;
    gfx.hw.src.rt.u32Height = srcrect.h;
    gfx.hw.src.surf.u32Width = srcrect.w;
    gfx.hw.src.surf.u32Height = srcrect.h;
    gfx.hw.src.surf.u32Stride = pitch;
    gfx.hw.src.surf.eColorFmt = (pitch / srcrect.w) == 2 ? E_MI_GFX_FMT_RGB565 : E_MI_GFX_FMT_ARGB8888;
    gfx.hw.src.surf.phyAddr = gfx.tmp.phyAddr;
    
    gfx.hw.dst.rt.s32Xpos = 0;
    gfx.hw.dst.rt.s32Ypos = 0;
    gfx.hw.dst.rt.u32Width = FB_W;
    gfx.hw.dst.rt.u32Height = FB_H;
    gfx.hw.dst.rt.s32Xpos = dstrect.x;
    gfx.hw.dst.rt.s32Ypos = dstrect.y;
    gfx.hw.dst.rt.u32Width = dstrect.w;
    gfx.hw.dst.rt.u32Height = dstrect.h;
    gfx.hw.dst.surf.u32Width = FB_W;
    gfx.hw.dst.surf.u32Height = FB_H;
    gfx.hw.dst.surf.u32Stride = FB_W * FB_BPP;
    gfx.hw.dst.surf.eColorFmt = E_MI_GFX_FMT_ARGB8888;
    gfx.hw.dst.surf.phyAddr = gfx.fb.phyAddr + (FB_W * gfx.vinfo.yoffset * FB_BPP);

    MI_SYS_FlushInvCache(gfx.tmp.virAddr, pitch * srcrect.h);
    MI_GFX_BitBlit(&gfx.hw.src.surf, &gfx.hw.src.rt, &gfx.hw.dst.surf, &gfx.hw.dst.rt, &gfx.hw.opt, &u16Fence);
    MI_GFX_WaitAllDone(FALSE, u16Fence);
    
    return 0;
}


void GFX_Flip(void)
{
    ioctl(gfx.fd, FBIOPAN_DISPLAY, &gfx.vinfo);
    gfx.vinfo.yoffset ^= FB_H;
}

static int MMIYOO_Available(void)
{
    const char *envr = SDL_getenv("SDL_VIDEODRIVER");
    if((envr) && (SDL_strcmp(envr, MMIYOO_DRIVER_NAME) == 0)) {
        return 1;
    }
    return 0;
}

static void MMIYOO_DeleteDevice(SDL_VideoDevice *device)
{
    SDL_free(device);
}

int MMIYOO_CreateWindow(_THIS, SDL_Window *window)
{
    SDL_SetMouseFocus(window);
    MMiyooVideoInfo.window = window;
    printf("%s, w:%d, h:%d\n", __func__, window->w, window->h);
    //glUpdateBufferSettings(fb_flip, &fb_idx, fb_vaddr);
    pico.state.push_update = 4;
    pico.state.refresh_border = 4;
    return 0;
}

int MMIYOO_CreateWindowFrom(_THIS, SDL_Window *window, const void *data)
{
    return SDL_Unsupported();
}

static SDL_VideoDevice *MMIYOO_CreateDevice(int devindex)
{
    SDL_VideoDevice *device=NULL;
    SDL_GLDriverData *gldata=NULL;

    if(!MMIYOO_Available()) {
        return (0);
    }

    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if(!device) {
        SDL_OutOfMemory();
        return (0);
    }
    device->is_dummy = SDL_TRUE;

    device->VideoInit = MMIYOO_VideoInit;
    device->VideoQuit = MMIYOO_VideoQuit;
    device->SetDisplayMode = MMIYOO_SetDisplayMode;
    device->PumpEvents = MMIYOO_PumpEvents;
    device->CreateSDLWindow = MMIYOO_CreateWindow;
    device->CreateSDLWindowFrom = MMIYOO_CreateWindowFrom;
    device->CreateWindowFramebuffer = MMIYOO_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = MMIYOO_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = MMIYOO_DestroyWindowFramebuffer;

    device->GL_LoadLibrary = glLoadLibrary;
    device->GL_GetProcAddress = glGetProcAddress;
    device->GL_CreateContext = glCreateContext;
    device->GL_SetSwapInterval = glSetSwapInterval;
    device->GL_SwapWindow = glSwapWindow;
    device->GL_MakeCurrent = glMakeCurrent;
    device->GL_DeleteContext = glDeleteContext;
    device->GL_UnloadLibrary = glUnloadLibrary;
    
    gldata = (SDL_GLDriverData*)SDL_calloc(1, sizeof(SDL_GLDriverData));
    if(gldata == NULL) {
        SDL_OutOfMemory();
        SDL_free(device);
        return NULL;
    }
    device->gl_data = gldata;

    device->free = MMIYOO_DeleteDevice;
    return device;
}

VideoBootStrap MMIYOO_bootstrap = {MMIYOO_DRIVER_NAME, "MMIYOO VIDEO DRIVER", MMIYOO_CreateDevice};

int MMIYOO_VideoInit(_THIS)
{

    int result;
    SDL_DisplayMode mode={0};
    SDL_VideoDisplay display={0};

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 640;
    mode.h = 480;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.w = 640;
    mode.h = 480;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 800;
    mode.h = 480;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.w = 800;
    mode.h = 480;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);
    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 800;
    mode.h = 600;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.w = 800;
    mode.h = 600;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 320;
    mode.h = 240;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.w = 320;
    mode.h = 240;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB565;
    mode.w = 480;
    mode.h = 272;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ARGB8888;
    mode.w = 480;
    mode.h = 272;
    mode.refresh_rate = 60;
    SDL_AddDisplayMode(&display, &mode);

    SDL_AddVideoDisplay(&display, SDL_FALSE);
    
    GFX_Init();
    picoConfigRead();
    get_cpuclock();
    set_cpuclock(pico.cpuclock);
    MMIYOO_EventInit();
        
    result = preloadResources();
    if (result != 0) {
        switch (result) {
            case -1:
                fprintf(stderr, "Error: Failed to create RGB surface.\n");
                break;
            case -2:
                fprintf(stderr, "Error: Could not open border directory.\n");
                break;
            case -3:
                fprintf(stderr, "Error: Failed to load an image.\n");
                break;
            case -4:
                fprintf(stderr, "ERROR loading digit image\n");
                break;
            case -5:
                fprintf(stderr, "ERROR digit image file does not exist\n");
                break;
            default:
                fprintf(stderr, "An unknown error occurred.\n");
        }
    }
    return 0;
}

static int MMIYOO_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    return 0;
}

void MMIYOO_VideoQuit(_THIS)
{
    int cc = 0;

    picoConfigWrite();
    
    for (cc=0; cc<=9; cc++) { // dealloc the digit icons
        if (pico.digit[cc]) {
            SDL_FreeSurface(pico.digit[cc]);
            pico.digit[cc] = NULL;
        }
    }
    
    for (int id = 0; id < pico.total_borders_loaded; id++) { // dealloc the borders
        if (pico.border[id]) {
            SDL_FreeSurface(pico.border[id]);
            pico.border[id] = NULL;
        }
    }
    
    GFX_Quit();
    MMIYOO_EventDeinit();
}

#endif

