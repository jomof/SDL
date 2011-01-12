/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* Android SDL video driver implementation
*/

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_androidvideo.h"
#include "SDL_androidevents.h"
#include "SDL_androidkeyboard.h"

#define ANDROID_VID_DRIVER_NAME "Android"

/* Initialization/Query functions */
static int Android_VideoInit(_THIS);
static void Android_VideoQuit(_THIS);

/* GL functions (SDL_androidgl.c) */
extern int Android_GL_LoadLibrary(_THIS, const char *path);
extern void *Android_GL_GetProcAddress(_THIS, const char *proc);
extern void Android_GL_UnloadLibrary(_THIS);
//extern int *Android_GL_GetVisual(_THIS, Display * display, int screen);
extern SDL_GLContext Android_GL_CreateContext(_THIS, SDL_Window * window);
extern int Android_GL_MakeCurrent(_THIS, SDL_Window * window,
                              SDL_GLContext context);
extern int Android_GL_SetSwapInterval(_THIS, int interval);
extern int Android_GL_GetSwapInterval(_THIS);
extern void Android_GL_SwapWindow(_THIS, SDL_Window * window);
extern void Android_GL_DeleteContext(_THIS, SDL_GLContext context);

/* Android driver bootstrap functions */


// These are filled in with real values in Android_SetScreenResolution on 
// init (before SDL_main())
static Uint32 iScreenFormat = SDL_PIXELFORMAT_UNKNOWN;
static int iScreenWidth = 0;
static int iScreenHeight = 0;


static int
Android_Available(void)
{
    return 1;
}

static void
Android_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_free(device);
}

static SDL_VideoDevice *
Android_CreateDevice(int devindex)
{
    printf("Creating video device\n");
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        if (device) {
            SDL_free(device);
        }
        return (0);
    }

    /* Set the function pointers */
    device->VideoInit = Android_VideoInit;
    device->VideoQuit = Android_VideoQuit;
    device->PumpEvents = Android_PumpEvents;

    device->free = Android_DeleteDevice;

    /* GL pointers */
    device->GL_LoadLibrary = Android_GL_LoadLibrary;
    device->GL_GetProcAddress = Android_GL_GetProcAddress;
    device->GL_UnloadLibrary = Android_GL_UnloadLibrary;
    device->GL_CreateContext = Android_GL_CreateContext;
    device->GL_MakeCurrent = Android_GL_MakeCurrent;
    device->GL_SetSwapInterval = Android_GL_SetSwapInterval;
    device->GL_GetSwapInterval = Android_GL_GetSwapInterval;
    device->GL_SwapWindow = Android_GL_SwapWindow;
    device->GL_DeleteContext = Android_GL_DeleteContext;

    return device;
}

VideoBootStrap Android_bootstrap = {
    ANDROID_VID_DRIVER_NAME, "SDL Android video driver",
    Android_Available, Android_CreateDevice
};


int
Android_VideoInit(_THIS)
{
    SDL_DisplayMode mode;

    mode.format = iScreenFormat;
    mode.w = iScreenWidth;
    mode.h = iScreenHeight;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;
    if (SDL_AddBasicVideoDisplay(&mode) < 0) {
        return -1;
    }

    SDL_zero(mode);
    SDL_AddDisplayMode(&_this->displays[0], &mode);

    Android_InitKeyboard();

    /* We're done! */
    return 0;
}

void
Android_VideoQuit(_THIS)
{
}

void
Android_SetScreenResolution(int width, int height, Uint32 format)
{
    iScreenWidth = width;
    iScreenHeight = height;   
    iScreenFormat = format;
}

/* vi: set ts=4 sw=4 expandtab: */
