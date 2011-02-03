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

#include "../SDL_sysrender.h"
#include "../../video/SDL_pixels_c.h"


/* SDL surface based renderer implementation */

static SDL_Renderer *SW_CreateRenderer(SDL_Window * window, Uint32 flags);
static void SW_WindowEvent(SDL_Renderer * renderer,
                           const SDL_WindowEvent *event);
static int SW_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int SW_SetTextureColorMod(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static int SW_SetTextureAlphaMod(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static int SW_SetTextureBlendMode(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static int SW_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                            const SDL_Rect * rect, const void *pixels,
                            int pitch);
static int SW_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * rect, void **pixels, int *pitch);
static void SW_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int SW_RenderDrawPoints(SDL_Renderer * renderer,
                               const SDL_Point * points, int count);
static int SW_RenderDrawLines(SDL_Renderer * renderer,
                              const SDL_Point * points, int count);
static int SW_RenderFillRects(SDL_Renderer * renderer,
                              const SDL_Rect ** rects, int count);
static int SW_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                         const SDL_Rect * srcrect, const SDL_Rect * dstrect);
static int SW_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                               Uint32 format, void * pixels, int pitch);
static void SW_RenderPresent(SDL_Renderer * renderer);
static void SW_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static void SW_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver SW_RenderDriver = {
    SW_CreateRenderer,
    {
     "software",
     (SDL_RENDERER_PRESENTVSYNC),
     8,
     {
      SDL_PIXELFORMAT_RGB555,
      SDL_PIXELFORMAT_RGB565,
      SDL_PIXELFORMAT_RGB888,
      SDL_PIXELFORMAT_BGR888,
      SDL_PIXELFORMAT_ARGB8888,
      SDL_PIXELFORMAT_RGBA8888,
      SDL_PIXELFORMAT_ABGR8888,
      SDL_PIXELFORMAT_BGRA8888
     },
     0,
     0}
};

typedef struct
{
    Uint32 format;
    SDL_bool updateSize;
    SDL_Texture *texture;
    SDL_Surface surface;
    SDL_Renderer *renderer;
} SW_RenderData;

static SDL_Texture *
CreateTexture(SDL_Renderer * renderer, Uint32 format, int w, int h)
{
    SDL_Texture *texture;

    texture = (SDL_Texture *) SDL_calloc(1, sizeof(*texture));
    if (!texture) {
        SDL_OutOfMemory();
        return NULL;
    }

    texture->format = format;
    texture->access = SDL_TEXTUREACCESS_STREAMING;
    texture->w = w;
    texture->h = h;
    texture->renderer = renderer;

    if (renderer->CreateTexture(renderer, texture) < 0) {
        SDL_free(texture);
        return NULL;
    }
    return texture;
}

static void
DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    renderer->DestroyTexture(renderer, texture);
    SDL_free(texture);
}

SDL_Renderer *
SW_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_Renderer *renderer;
    SW_RenderData *data;
    int i;
    int w, h;
    Uint32 format;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;
    Uint32 renderer_flags;
    const char *desired_driver;

    format = SDL_GetWindowPixelFormat(window);
    if (!SDL_PixelFormatEnumToMasks
        (format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
        SDL_SetError("Unknown display format");
        return NULL;
    }

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (SW_RenderData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        SW_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    renderer->WindowEvent = SW_WindowEvent;
    renderer->CreateTexture = SW_CreateTexture;
    renderer->SetTextureColorMod = SW_SetTextureColorMod;
    renderer->SetTextureAlphaMod = SW_SetTextureAlphaMod;
    renderer->SetTextureBlendMode = SW_SetTextureBlendMode;
    renderer->UpdateTexture = SW_UpdateTexture;
    renderer->LockTexture = SW_LockTexture;
    renderer->UnlockTexture = SW_UnlockTexture;
    renderer->DestroyTexture = SW_DestroyTexture;
    renderer->RenderDrawPoints = SW_RenderDrawPoints;
    renderer->RenderDrawLines = SW_RenderDrawLines;
    renderer->RenderFillRects = SW_RenderFillRects;
    renderer->RenderCopy = SW_RenderCopy;
    renderer->RenderReadPixels = SW_RenderReadPixels;
    renderer->RenderPresent = SW_RenderPresent;
    renderer->DestroyRenderer = SW_DestroyRenderer;
    renderer->info = SW_RenderDriver.info;
    renderer->info.flags = 0;
    renderer->window = window;
    renderer->driverdata = data;

    data->format = format;

    /* Find a render driver that we can use to display data */
    renderer_flags = 0;
    if (flags & SDL_RENDERER_PRESENTVSYNC) {
        renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
    }
    desired_driver = SDL_getenv("SDL_VIDEO_RENDERER_SWDRIVER");
    for (i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
        SDL_RendererInfo info;
        SDL_GetRenderDriverInfo(i, &info);
        if (SDL_strcmp(info.name, SW_RenderDriver.info.name) == 0) {
            continue;
        }
        if (desired_driver
            && SDL_strcasecmp(desired_driver, info.name) != 0) {
            continue;
        }
        data->renderer = SDL_CreateRenderer(window, i, renderer_flags);
        if (data->renderer) {
            break;
        }
    }
    if (i == SDL_GetNumRenderDrivers()) {
        SW_DestroyRenderer(renderer);
        SDL_SetError("Couldn't find display render driver");
        return NULL;
    }
    if (data->renderer->info.flags & SDL_RENDERER_PRESENTVSYNC) {
        renderer->info.flags |= SDL_RENDERER_PRESENTVSYNC;
    }

    /* Create the textures we'll use for display */
    SDL_GetWindowSize(window, &w, &h);
    data->texture = CreateTexture(data->renderer, data->format, w, h);
    if (!data->texture) {
        SW_DestroyRenderer(renderer);
        return NULL;
    }

    /* Create a surface we'll use for rendering */
    data->surface.flags = SDL_PREALLOC;
    data->surface.format = SDL_AllocFormat(bpp, Rmask, Gmask, Bmask, Amask);
    if (!data->surface.format) {
        SW_DestroyRenderer(renderer);
        return NULL;
    }

    return renderer;
}

static SDL_Texture *
SW_ActivateRenderer(SDL_Renderer * renderer)
{
    SW_RenderData *data = (SW_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;

    if (data->updateSize) {
        /* Recreate the textures for the new window size */
        int w, h;
        if (data->texture) {
            DestroyTexture(data->renderer, data->texture);
        }
        SDL_GetWindowSize(window, &w, &h);
        data->texture = CreateTexture(data->renderer, data->format, w, h);
        if (data->texture) {
            data->updateSize = SDL_FALSE;
        }
    }
    return data->texture;
}

static void
SW_WindowEvent(SDL_Renderer * renderer, const SDL_WindowEvent *event)
{
    SW_RenderData *data = (SW_RenderData *) renderer->driverdata;

    if (event->event == SDL_WINDOWEVENT_RESIZED) {
        data->updateSize = SDL_TRUE;
    }
}

static int
SW_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    if (!SDL_PixelFormatEnumToMasks
        (texture->format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
        SDL_SetError("Unknown texture format");
        return -1;
    }

    texture->driverdata =
        SDL_CreateRGBSurface(0, texture->w, texture->h, bpp, Rmask, Gmask,
                             Bmask, Amask);
    SDL_SetSurfaceColorMod(texture->driverdata, texture->r, texture->g,
                           texture->b);
    SDL_SetSurfaceAlphaMod(texture->driverdata, texture->a);
    SDL_SetSurfaceBlendMode(texture->driverdata, texture->blendMode);

    if (texture->access == SDL_TEXTUREACCESS_STATIC) {
        SDL_SetSurfaceRLE(texture->driverdata, 1);
    }

    if (!texture->driverdata) {
        return -1;
    }
    return 0;
}

static int
SW_SetTextureColorMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    return SDL_SetSurfaceColorMod(surface, texture->r, texture->g,
                                  texture->b);
}

static int
SW_SetTextureAlphaMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    return SDL_SetSurfaceAlphaMod(surface, texture->a);
}

static int
SW_SetTextureBlendMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    return SDL_SetSurfaceBlendMode(surface, texture->blendMode);
}

static int
SW_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                 const SDL_Rect * rect, const void *pixels, int pitch)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    Uint8 *src, *dst;
    int row;
    size_t length;

    src = (Uint8 *) pixels;
    dst = (Uint8 *) surface->pixels +
                        rect->y * surface->pitch +
                        rect->x * surface->format->BytesPerPixel;
    length = rect->w * surface->format->BytesPerPixel;
    for (row = 0; row < rect->h; ++row) {
        SDL_memcpy(dst, src, length);
        src += pitch;
        dst += surface->pitch;
    }
    return 0;
}

static int
SW_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * rect, void **pixels, int *pitch)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;

    *pixels =
        (void *) ((Uint8 *) surface->pixels + rect->y * surface->pitch +
                  rect->x * surface->format->BytesPerPixel);
    *pitch = surface->pitch;
    return 0;
}

static void
SW_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
}

static int
SW_RenderDrawPoints(SDL_Renderer * renderer, const SDL_Point * points,
                    int count)
{
    SW_RenderData *data = (SW_RenderData *) renderer->driverdata;
    SDL_Texture *texture = SW_ActivateRenderer(renderer);
    SDL_Rect rect;
    int i;
    int x, y;
    int status = 0;

    if (!texture) {
        return -1;
    }

    /* Get the smallest rectangle that contains everything */
    rect.x = 0;
    rect.y = 0;
    rect.w = texture->w;
    rect.h = texture->h;
    if (!SDL_EnclosePoints(points, count, &rect, &rect)) {
        /* Nothing to draw */
        return 0;
    }

    if (data->renderer->LockTexture(data->renderer, texture, &rect,
                                    &data->surface.pixels,
                                    &data->surface.pitch) < 0) {
        return -1;
    }

    data->surface.clip_rect.w = data->surface.w = rect.w;
    data->surface.clip_rect.h = data->surface.h = rect.h;

    /* Draw the points! */
    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
        Uint32 color = SDL_MapRGBA(data->surface.format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);

        for (i = 0; i < count; ++i) {
            x = points[i].x - rect.x;
            y = points[i].y - rect.y;

            status = SDL_DrawPoint(&data->surface, x, y, color);
        }
    } else {
        for (i = 0; i < count; ++i) {
            x = points[i].x - rect.x;
            y = points[i].y - rect.y;

            status = SDL_BlendPoint(&data->surface, x, y,
                                    renderer->blendMode,
                                    renderer->r, renderer->g, renderer->b,
                                    renderer->a);
        }
    }

    data->renderer->UnlockTexture(data->renderer, texture);

    return status;
}

static int
SW_RenderDrawLines(SDL_Renderer * renderer, const SDL_Point * points,
                   int count)
{
    SW_RenderData *data = (SW_RenderData *) renderer->driverdata;
    SDL_Texture *texture = SW_ActivateRenderer(renderer);
    SDL_Rect clip, rect;
    int i;
    int x1, y1, x2, y2;
    int status = 0;

    if (!texture) {
        return -1;
    }

    /* Get the smallest rectangle that contains everything */
    clip.x = 0;
    clip.y = 0;
    clip.w = texture->w;
    clip.h = texture->h;
    SDL_EnclosePoints(points, count, NULL, &rect);
    if (!SDL_IntersectRect(&rect, &clip, &rect)) {
        /* Nothing to draw */
        return 0;
    }

    if (data->renderer->LockTexture(data->renderer, texture, &rect,
                                    &data->surface.pixels,
                                    &data->surface.pitch) < 0) {
        return -1;
    }

    data->surface.clip_rect.w = data->surface.w = rect.w;
    data->surface.clip_rect.h = data->surface.h = rect.h;

    /* Draw the points! */
    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
        Uint32 color = SDL_MapRGBA(data->surface.format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);

        for (i = 1; i < count; ++i) {
            x1 = points[i-1].x - rect.x;
            y1 = points[i-1].y - rect.y;
            x2 = points[i].x - rect.x;
            y2 = points[i].y - rect.y;

            status = SDL_DrawLine(&data->surface, x1, y1, x2, y2, color);
        }
    } else {
        for (i = 1; i < count; ++i) {
            x1 = points[i-1].x - rect.x;
            y1 = points[i-1].y - rect.y;
            x2 = points[i].x - rect.x;
            y2 = points[i].y - rect.y;

            status = SDL_BlendLine(&data->surface, x1, y1, x2, y2,
                                   renderer->blendMode,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);
        }
    }

    data->renderer->UnlockTexture(data->renderer, texture);

    return status;
}

static int
SW_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect ** rects,
                   int count)
{
    SW_RenderData *data = (SW_RenderData *) renderer->driverdata;
    SDL_Texture *texture = SW_ActivateRenderer(renderer);
    SDL_Rect clip, rect;
    Uint32 color = 0;
    int i;
    int status = 0;

    if (!texture) {
        return -1;
    }

    clip.x = 0;
    clip.y = 0;
    clip.w = texture->w;
    clip.h = texture->h;

    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
        color = SDL_MapRGBA(data->surface.format,
                            renderer->r, renderer->g, renderer->b,
                            renderer->a);
    }

    for (i = 0; i < count; ++i) {
        if (!SDL_IntersectRect(rects[i], &clip, &rect)) {
            /* Nothing to draw */
            continue;
        }

        if (data->renderer->LockTexture(data->renderer, texture, &rect,
                                        &data->surface.pixels,
                                        &data->surface.pitch) < 0) {
            return -1;
        }

        data->surface.clip_rect.w = data->surface.w = rect.w;
        data->surface.clip_rect.h = data->surface.h = rect.h;

        if (renderer->blendMode == SDL_BLENDMODE_NONE) {
            status = SDL_FillRect(&data->surface, NULL, color);
        } else {
            status = SDL_BlendFillRect(&data->surface, NULL,
                                       renderer->blendMode,
                                       renderer->r, renderer->g, renderer->b,
                                       renderer->a);
        }

        data->renderer->UnlockTexture(data->renderer, texture);
    }
    return status;
}

static int
SW_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
              const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    SW_RenderData *data = (SW_RenderData *) renderer->driverdata;
    SDL_Surface *surface;
    SDL_Rect real_srcrect;
    SDL_Rect real_dstrect;
    int status;

    if (!SW_ActivateRenderer(renderer)) {
        return -1;
    }

    if (data->renderer->LockTexture(data->renderer, data->texture, dstrect,
                                    &data->surface.pixels,
                                    &data->surface.pitch) < 0) {
        return -1;
    }

    surface = (SDL_Surface *) texture->driverdata;
    real_srcrect = *srcrect;

    data->surface.w = dstrect->w;
    data->surface.h = dstrect->h;
    data->surface.clip_rect.w = dstrect->w;
    data->surface.clip_rect.h = dstrect->h;
    real_dstrect = data->surface.clip_rect;

    status = SDL_LowerBlit(surface, &real_srcrect, &data->surface, &real_dstrect);
    data->renderer->UnlockTexture(data->renderer, data->texture);
    return status;
}

static int
SW_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                    Uint32 format, void * pixels, int pitch)
{
    SW_RenderData *data = (SW_RenderData *) renderer->driverdata;

    if (!SW_ActivateRenderer(renderer)) {
        return -1;
    }

    if (data->renderer->LockTexture(data->renderer, data->texture, rect,
                                    &data->surface.pixels,
                                    &data->surface.pitch) < 0) {
        return -1;
    }

    SDL_ConvertPixels(rect->w, rect->h,
                      data->format, data->surface.pixels, data->surface.pitch,
                      format, pixels, pitch);

    data->renderer->UnlockTexture(data->renderer, data->texture);
    return 0;
}

static void
SW_RenderPresent(SDL_Renderer * renderer)
{
    SW_RenderData *data = (SW_RenderData *) renderer->driverdata;
    SDL_Texture *texture = SW_ActivateRenderer(renderer);
    SDL_Rect rect;

    if (!texture) {
        return;
    }

    /* Send the data to the display */
    rect.x = 0;
    rect.y = 0;
    rect.w = texture->w;
    rect.h = texture->h;
    data->renderer->RenderCopy(data->renderer, texture, &rect, &rect);
    data->renderer->RenderPresent(data->renderer);
}

static void
SW_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;

    SDL_FreeSurface(surface);
}

static void
SW_DestroyRenderer(SDL_Renderer * renderer)
{
    SW_RenderData *data = (SW_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;

    if (data) {
        if (data->texture) {
            DestroyTexture(data->renderer, data->texture);
        }
        if (data->surface.format) {
            SDL_FreeFormat(data->surface.format);
        }
        if (data->renderer) {
            data->renderer->DestroyRenderer(data->renderer);
        }
        SDL_free(data);
    }
    SDL_free(renderer);
}

/* vi: set ts=4 sw=4 expandtab: */
