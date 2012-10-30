/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

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
#include "SDL_config.h"

#if SDL_VIDEO_DRIVER_WINRT

/* WinRT SDL video driver implementation

   Initial work on this was done by David Ludwig (dludwig@pobox.com), and
   was based off of SDL's "dummy" video driver.
 */

extern "C" {
#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"
}

#include "SDL_WinRTApp.h"
#include "SDL_winrtvideo.h"
#include "SDL_winrtevents_c.h"
#include "SDL_winrtframebuffer_c.h"

/* On Windows, windows.h defines CreateWindow */
#ifdef CreateWindow
#undef CreateWindow
#endif

extern SDL_WinRTApp ^ SDL_WinRTGlobalApp;

#define WINRTVID_DRIVER_NAME "dummy"

/* Initialization/Query functions */
static int WINRT_VideoInit(_THIS);
static int WINRT_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
static void WINRT_VideoQuit(_THIS);

/* Window functions */
static int WINRT_CreateWindow(_THIS, SDL_Window * window);
static void WINRT_DestroyWindow(_THIS, SDL_Window * window);

/* WinRT driver bootstrap functions */

static int
WINRT_Available(void)
{
    return (1);
}

static void
WINRT_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_free(device);
}

static SDL_VideoDevice *
WINRT_CreateDevice(int devindex)
{
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
    device->VideoInit = WINRT_VideoInit;
    device->VideoQuit = WINRT_VideoQuit;
    device->CreateWindow = WINRT_CreateWindow;
    device->DestroyWindow = WINRT_DestroyWindow;
    device->SetDisplayMode = WINRT_SetDisplayMode;
    device->PumpEvents = WINRT_PumpEvents;
    device->CreateWindowFramebuffer = SDL_WINRT_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = SDL_WINRT_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = SDL_WINRT_DestroyWindowFramebuffer;

    device->free = WINRT_DeleteDevice;

    return device;
}

VideoBootStrap WINRT_bootstrap = {
    WINRTVID_DRIVER_NAME, "SDL Windows RT video driver",
    WINRT_Available, WINRT_CreateDevice
};

int
WINRT_VideoInit(_THIS)
{
    SDL_DisplayMode mode = SDL_WinRTGlobalApp->GetMainDisplayMode();
    if (SDL_AddBasicVideoDisplay(&mode) < 0) {
        return -1;
    }

    SDL_AddDisplayMode(&_this->displays[0], &mode);

    /* We're done! */
    return 0;
}

static int
WINRT_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    return 0;
}

void
WINRT_VideoQuit(_THIS)
{
}

int
WINRT_CreateWindow(_THIS, SDL_Window * window)
{
    // Make sure that only one window gets created, at least until multimonitor
    // support is added.
    if (SDL_WinRTGlobalApp->HasSDLWindowData())
    {
        SDL_SetError("WinRT only supports one window");
        return -1;
    }

    SDL_WindowData *data;
    data = (SDL_WindowData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }
    SDL_zerop(data);
    data->sdlWindow = window;

    /* Adjust the window data to match the screen */
    window->x = 0;
    window->y = 0;
    window->w = _this->displays->desktop_mode.w;
    window->h = _this->displays->desktop_mode.h;

    SDL_WinRTGlobalApp->SetSDLWindowData(data);

    return 0;
}

void
WINRT_DestroyWindow(_THIS, SDL_Window * window)
{
    if (SDL_WinRTGlobalApp->HasSDLWindowData() &&
        SDL_WinRTGlobalApp->GetSDLWindowData()->sdlWindow == window)
    {
        SDL_WinRTGlobalApp->SetSDLWindowData(NULL);
    }
}


#endif /* SDL_VIDEO_DRIVER_WINRT */

/* vi: set ts=4 sw=4 expandtab: */
