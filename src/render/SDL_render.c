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

/* The SDL 2D rendering system */

#include "SDL_render.h"
#include "SDL_sysrender.h"
#include "../video/SDL_pixels_c.h"


#define CHECK_RENDERER_MAGIC(renderer, retval) \
    if (!renderer || renderer->magic != &renderer_magic) { \
        SDL_SetError("Invalid renderer"); \
        return retval; \
    }

#define CHECK_TEXTURE_MAGIC(texture, retval) \
    if (!texture || texture->magic != &texture_magic) { \
        SDL_SetError("Invalid texture"); \
        return retval; \
    }


static const SDL_RenderDriver *render_drivers[] = {
#if SDL_VIDEO_RENDER_D3D
    &D3D_RenderDriver,
#endif
#if SDL_VIDEO_RENDER_OGL
    &GL_RenderDriver,
#endif
#if SDL_VIDEO_RENDER_OGL_ES
    &GL_ES_RenderDriver,
#endif
    &SW_RenderDriver
};
static char renderer_magic;
static char texture_magic;

int
SDL_GetNumRenderDrivers(void)
{
    return SDL_arraysize(render_drivers);
}

int
SDL_GetRenderDriverInfo(int index, SDL_RendererInfo * info)
{
    if (index < 0 || index >= SDL_GetNumRenderDrivers()) {
        SDL_SetError("index must be in the range of 0 - %d",
                     SDL_GetNumRenderDrivers() - 1);
        return -1;
    }
    *info = render_drivers[index]->info;
    return 0;
}

static int
SDL_RendererEventWatch(void *userdata, SDL_Event *event)
{
    SDL_Renderer *renderer = (SDL_Renderer *)userdata;

    if (event->type == SDL_WINDOWEVENT && renderer->WindowEvent) {
        SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
        if (window == renderer->window) {
            renderer->WindowEvent(renderer, &event->window);
        }
    }
    return 0;
}

SDL_Renderer *
SDL_CreateRenderer(SDL_Window * window, int index, Uint32 flags)
{
    SDL_Renderer *renderer = NULL;
    int n = SDL_GetNumRenderDrivers();

    if (index < 0) {
        char *override = SDL_getenv("SDL_VIDEO_RENDERER");

        if (override) {
            for (index = 0; index < n; ++index) {
                const SDL_RenderDriver *driver = render_drivers[index];

                if (SDL_strcasecmp(override, driver->info.name) == 0) {
                    /* Create a new renderer instance */
                    renderer = driver->CreateRenderer(window, flags);
                    break;
                }
            }
        } else {
            for (index = 0; index < n; ++index) {
                const SDL_RenderDriver *driver = render_drivers[index];

                if ((driver->info.flags & flags) == flags) {
                    /* Create a new renderer instance */
                    renderer = driver->CreateRenderer(window, flags);
                    if (renderer) {
                        /* Yay, we got one! */
                        break;
                    }
                }
            }
        }
        if (index == n) {
            SDL_SetError("Couldn't find matching render driver");
            return NULL;
        }
    } else {
        if (index >= SDL_GetNumRenderDrivers()) {
            SDL_SetError("index must be -1 or in the range of 0 - %d",
                         SDL_GetNumRenderDrivers() - 1);
            return NULL;
        }
        /* Create a new renderer instance */
        renderer = render_drivers[index]->CreateRenderer(window, flags);
    }

    if (renderer) {
        renderer->magic = &renderer_magic;

        SDL_AddEventWatch(SDL_RendererEventWatch, renderer);
    }
    return renderer;
}

int
SDL_GetRendererInfo(SDL_Renderer * renderer, SDL_RendererInfo * info)
{
    CHECK_RENDERER_MAGIC(renderer, -1);

    *info = renderer->info;
    return 0;
}

SDL_Texture *
SDL_CreateTexture(SDL_Renderer * renderer, Uint32 format, int access, int w, int h)
{
    SDL_Texture *texture;

    CHECK_RENDERER_MAGIC(renderer, NULL);

    if (w <= 0 || h <= 0) {
        SDL_SetError("Texture dimensions can't be 0");
        return 0;
    }
    texture = (SDL_Texture *) SDL_calloc(1, sizeof(*texture));
    if (!texture) {
        SDL_OutOfMemory();
        return 0;
    }
    texture->magic = &texture_magic;
    texture->format = format;
    texture->access = access;
    texture->w = w;
    texture->h = h;
    texture->r = 255;
    texture->g = 255;
    texture->b = 255;
    texture->a = 255;
    texture->renderer = renderer;
    texture->next = renderer->textures;
    if (renderer->textures) {
        renderer->textures->prev = texture;
    }
    renderer->textures = texture;

    if (renderer->CreateTexture(renderer, texture) < 0) {
        SDL_DestroyTexture(texture);
        return 0;
    }
    return texture;
}

SDL_Texture *
SDL_CreateTextureFromSurface(SDL_Renderer * renderer, Uint32 format, SDL_Surface * surface)
{
    SDL_Texture *texture;
    Uint32 requested_format = format;
    SDL_PixelFormat *fmt;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    CHECK_RENDERER_MAGIC(renderer, NULL);

    if (!surface) {
        SDL_SetError("SDL_CreateTextureFromSurface() passed NULL surface");
        return NULL;
    }
    fmt = surface->format;

    if (format) {
        if (!SDL_PixelFormatEnumToMasks
            (format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
            SDL_SetError("Unknown pixel format");
            return 0;
        }
    } else {
        SDL_bool hasColorkey;
        SDL_BlendMode blendMode;
        SDL_bool hasBlending;

        hasColorkey = (SDL_GetColorKey(surface, NULL) == 0);

        SDL_GetSurfaceBlendMode(surface, &blendMode);
        hasBlending = (blendMode == SDL_BLENDMODE_BLEND);

        if (surface->format->Amask || (!hasColorkey && !hasBlending)) {
            Uint32 it;
            int pfmt;

            /* Pixel formats, sorted by best first */
            static const Uint32 sdl_pformats[] = {
                SDL_PIXELFORMAT_ARGB8888,
                SDL_PIXELFORMAT_RGBA8888,
                SDL_PIXELFORMAT_ABGR8888,
                SDL_PIXELFORMAT_BGRA8888,
                SDL_PIXELFORMAT_RGB888,
                SDL_PIXELFORMAT_BGR888,
                SDL_PIXELFORMAT_RGB24,
                SDL_PIXELFORMAT_BGR24,
                SDL_PIXELFORMAT_RGB565,
                SDL_PIXELFORMAT_BGR565,
                SDL_PIXELFORMAT_ARGB1555,
                SDL_PIXELFORMAT_RGBA5551,
                SDL_PIXELFORMAT_ABGR1555,
                SDL_PIXELFORMAT_BGRA5551,
                SDL_PIXELFORMAT_RGB555,
                SDL_PIXELFORMAT_BGR555,
                SDL_PIXELFORMAT_ARGB4444,
                SDL_PIXELFORMAT_RGBA4444,
                SDL_PIXELFORMAT_ABGR4444,
                SDL_PIXELFORMAT_BGRA4444,
                SDL_PIXELFORMAT_RGB444,
                SDL_PIXELFORMAT_ARGB2101010,
                SDL_PIXELFORMAT_RGB332,
                SDL_PIXELFORMAT_UNKNOWN
            };

            bpp = fmt->BitsPerPixel;
            Rmask = fmt->Rmask;
            Gmask = fmt->Gmask;
            Bmask = fmt->Bmask;
            Amask = fmt->Amask;

            format =
                SDL_MasksToPixelFormatEnum(bpp, Rmask, Gmask, Bmask, Amask);
            if (!format) {
                SDL_SetError("Unknown pixel format");
                return 0;
            }

            /* Search requested format in the supported texture */
            /* formats by current renderer                      */
            for (it = 0; it < renderer->info.num_texture_formats; it++) {
                if (renderer->info.texture_formats[it] == format) {
                    break;
                }
            }

            /* If requested format can't be found, search any best */
            /* format which renderer provides                      */
            if (it == renderer->info.num_texture_formats) {
                pfmt = 0;
                for (;;) {
                    if (sdl_pformats[pfmt] == SDL_PIXELFORMAT_UNKNOWN) {
                        break;
                    }

                    for (it = 0; it < renderer->info.num_texture_formats;
                         it++) {
                        if (renderer->info.texture_formats[it] ==
                            sdl_pformats[pfmt]) {
                            break;
                        }
                    }

                    if (it != renderer->info.num_texture_formats) {
                        /* The best format has been found */
                        break;
                    }
                    pfmt++;
                }

                /* If any format can't be found, then return an error */
                if (it == renderer->info.num_texture_formats) {
                    SDL_SetError
                        ("Any of the supported pixel formats can't be found");
                    return 0;
                }

                /* Convert found pixel format back to color masks */
                if (SDL_PixelFormatEnumToMasks
                    (renderer->info.texture_formats[it], &bpp, &Rmask, &Gmask,
                     &Bmask, &Amask) != SDL_TRUE) {
                    SDL_SetError("Unknown pixel format");
                    return 0;
                }
            }
        } else {
            /* Need a format with alpha */
            Uint32 it;
            int apfmt;

            /* Pixel formats with alpha, sorted by best first */
            static const Uint32 sdl_alpha_pformats[] = {
                SDL_PIXELFORMAT_ARGB8888,
                SDL_PIXELFORMAT_RGBA8888,
                SDL_PIXELFORMAT_ABGR8888,
                SDL_PIXELFORMAT_BGRA8888,
                SDL_PIXELFORMAT_ARGB1555,
                SDL_PIXELFORMAT_RGBA5551,
                SDL_PIXELFORMAT_ABGR1555,
                SDL_PIXELFORMAT_BGRA5551,
                SDL_PIXELFORMAT_ARGB4444,
                SDL_PIXELFORMAT_RGBA4444,
                SDL_PIXELFORMAT_ABGR4444,
                SDL_PIXELFORMAT_BGRA4444,
                SDL_PIXELFORMAT_ARGB2101010,
                SDL_PIXELFORMAT_UNKNOWN
            };

            if (surface->format->Amask) {
                /* If surface already has alpha, then try an original */
                /* surface format first                               */
                bpp = fmt->BitsPerPixel;
                Rmask = fmt->Rmask;
                Gmask = fmt->Gmask;
                Bmask = fmt->Bmask;
                Amask = fmt->Amask;
            } else {
                bpp = 32;
                Rmask = 0x00FF0000;
                Gmask = 0x0000FF00;
                Bmask = 0x000000FF;
                Amask = 0xFF000000;
            }

            format =
                SDL_MasksToPixelFormatEnum(bpp, Rmask, Gmask, Bmask, Amask);
            if (!format) {
                SDL_SetError("Unknown pixel format");
                return 0;
            }

            /* Search this format in the supported texture formats */
            /* by current renderer                                 */
            for (it = 0; it < renderer->info.num_texture_formats; it++) {
                if (renderer->info.texture_formats[it] == format) {
                    break;
                }
            }

            /* If this format can't be found, search any best       */
            /* compatible format with alpha which renderer provides */
            if (it == renderer->info.num_texture_formats) {
                apfmt = 0;
                for (;;) {
                    if (sdl_alpha_pformats[apfmt] == SDL_PIXELFORMAT_UNKNOWN) {
                        break;
                    }

                    for (it = 0; it < renderer->info.num_texture_formats;
                         it++) {
                        if (renderer->info.texture_formats[it] ==
                            sdl_alpha_pformats[apfmt]) {
                            break;
                        }
                    }

                    if (it != renderer->info.num_texture_formats) {
                        /* Compatible format has been found */
                        break;
                    }
                    apfmt++;
                }

                /* If compatible format can't be found, then return an error */
                if (it == renderer->info.num_texture_formats) {
                    SDL_SetError("Compatible pixel format can't be found");
                    return 0;
                }

                /* Convert found pixel format back to color masks */
                if (SDL_PixelFormatEnumToMasks
                    (renderer->info.texture_formats[it], &bpp, &Rmask, &Gmask,
                     &Bmask, &Amask) != SDL_TRUE) {
                    SDL_SetError("Unknown pixel format");
                    return 0;
                }
            }
        }

        format = SDL_MasksToPixelFormatEnum(bpp, Rmask, Gmask, Bmask, Amask);
        if (!format) {
            SDL_SetError("Unknown pixel format");
            return 0;
        }
    }

    texture =
        SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STATIC,
                          surface->w, surface->h);
    if (!texture && !requested_format) {
        SDL_DisplayMode desktop_mode;
        SDL_GetDesktopDisplayMode(&desktop_mode);
        format = desktop_mode.format;
        texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STATIC,
                                    surface->w, surface->h);
    }
    if (!texture) {
        return 0;
    }
    if (bpp == fmt->BitsPerPixel && Rmask == fmt->Rmask && Gmask == fmt->Gmask
        && Bmask == fmt->Bmask && Amask == fmt->Amask) {
        if (SDL_MUSTLOCK(surface)) {
            SDL_LockSurface(surface);
            SDL_UpdateTexture(texture, NULL, surface->pixels,
                              surface->pitch);
            SDL_UnlockSurface(surface);
        } else {
            SDL_UpdateTexture(texture, NULL, surface->pixels,
                              surface->pitch);
        }
    } else {
        SDL_PixelFormat dst_fmt;
        SDL_Surface *dst = NULL;

        /* Set up a destination surface for the texture update */
        SDL_InitFormat(&dst_fmt, bpp, Rmask, Gmask, Bmask, Amask);
        dst = SDL_ConvertSurface(surface, &dst_fmt, 0);
        if (dst) {
            SDL_UpdateTexture(texture, NULL, dst->pixels, dst->pitch);
            SDL_FreeSurface(dst);
        }
        if (!dst) {
            SDL_DestroyTexture(texture);
            return 0;
        }
    }

    {
        Uint8 r, g, b, a;
        SDL_BlendMode blendMode;

        SDL_GetSurfaceColorMod(surface, &r, &g, &b);
        SDL_SetTextureColorMod(texture, r, g, b);

        SDL_GetSurfaceAlphaMod(surface, &a);
        SDL_SetTextureAlphaMod(texture, a);

        if (SDL_GetColorKey(surface, NULL) == 0) {
            /* We converted to a texture with alpha format */
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        } else {
            SDL_GetSurfaceBlendMode(surface, &blendMode);
            SDL_SetTextureBlendMode(texture, blendMode);
        }
    }
    return texture;
}

int
SDL_QueryTexture(SDL_Texture * texture, Uint32 * format, int *access,
                 int *w, int *h)
{
    CHECK_TEXTURE_MAGIC(texture, -1);

    if (format) {
        *format = texture->format;
    }
    if (access) {
        *access = texture->access;
    }
    if (w) {
        *w = texture->w;
    }
    if (h) {
        *h = texture->h;
    }
    return 0;
}

int
SDL_QueryTexturePixels(SDL_Texture * texture, void **pixels, int *pitch)
{
    SDL_Renderer *renderer;

    CHECK_TEXTURE_MAGIC(texture, -1);

    renderer = texture->renderer;
    if (!renderer->QueryTexturePixels) {
        SDL_Unsupported();
        return -1;
    }
    return renderer->QueryTexturePixels(renderer, texture, pixels, pitch);
}

int
SDL_SetTextureColorMod(SDL_Texture * texture, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_Renderer *renderer;

    CHECK_TEXTURE_MAGIC(texture, -1);

    renderer = texture->renderer;
    if (r < 255 || g < 255 || b < 255) {
        texture->modMode |= SDL_TEXTUREMODULATE_COLOR;
    } else {
        texture->modMode &= ~SDL_TEXTUREMODULATE_COLOR;
    }
    texture->r = r;
    texture->g = g;
    texture->b = b;
    if (renderer->SetTextureColorMod) {
        return renderer->SetTextureColorMod(renderer, texture);
    } else {
        return 0;
    }
}

int
SDL_GetTextureColorMod(SDL_Texture * texture, Uint8 * r, Uint8 * g,
                       Uint8 * b)
{
    SDL_Renderer *renderer;

    CHECK_TEXTURE_MAGIC(texture, -1);

    renderer = texture->renderer;
    if (r) {
        *r = texture->r;
    }
    if (g) {
        *g = texture->g;
    }
    if (b) {
        *b = texture->b;
    }
    return 0;
}

int
SDL_SetTextureAlphaMod(SDL_Texture * texture, Uint8 alpha)
{
    SDL_Renderer *renderer;

    CHECK_TEXTURE_MAGIC(texture, -1);

    renderer = texture->renderer;
    if (alpha < 255) {
        texture->modMode |= SDL_TEXTUREMODULATE_ALPHA;
    } else {
        texture->modMode &= ~SDL_TEXTUREMODULATE_ALPHA;
    }
    texture->a = alpha;
    if (renderer->SetTextureAlphaMod) {
        return renderer->SetTextureAlphaMod(renderer, texture);
    } else {
        return 0;
    }
}

int
SDL_GetTextureAlphaMod(SDL_Texture * texture, Uint8 * alpha)
{
    CHECK_TEXTURE_MAGIC(texture, -1);

    if (alpha) {
        *alpha = texture->a;
    }
    return 0;
}

int
SDL_SetTextureBlendMode(SDL_Texture * texture, SDL_BlendMode blendMode)
{
    SDL_Renderer *renderer;

    CHECK_TEXTURE_MAGIC(texture, -1);

    renderer = texture->renderer;
    texture->blendMode = blendMode;
    if (renderer->SetTextureBlendMode) {
        return renderer->SetTextureBlendMode(renderer, texture);
    } else {
        return 0;
    }
}

int
SDL_GetTextureBlendMode(SDL_Texture * texture, SDL_BlendMode *blendMode)
{
    CHECK_TEXTURE_MAGIC(texture, -1);

    if (blendMode) {
        *blendMode = texture->blendMode;
    }
    return 0;
}

int
SDL_UpdateTexture(SDL_Texture * texture, const SDL_Rect * rect,
                  const void *pixels, int pitch)
{
    SDL_Renderer *renderer;
    SDL_Rect full_rect;

    CHECK_TEXTURE_MAGIC(texture, -1);

    renderer = texture->renderer;
    if (!renderer->UpdateTexture) {
        SDL_Unsupported();
        return -1;
    }
    if (!rect) {
        full_rect.x = 0;
        full_rect.y = 0;
        full_rect.w = texture->w;
        full_rect.h = texture->h;
        rect = &full_rect;
    }
    return renderer->UpdateTexture(renderer, texture, rect, pixels, pitch);
}

int
SDL_LockTexture(SDL_Texture * texture, const SDL_Rect * rect, int markDirty,
                void **pixels, int *pitch)
{
    SDL_Renderer *renderer;
    SDL_Rect full_rect;

    CHECK_TEXTURE_MAGIC(texture, -1);

    if (texture->access != SDL_TEXTUREACCESS_STREAMING) {
        SDL_SetError("SDL_LockTexture(): texture must be streaming");
        return -1;
    }
    renderer = texture->renderer;
    if (!renderer->LockTexture) {
        SDL_Unsupported();
        return -1;
    }
    if (!rect) {
        full_rect.x = 0;
        full_rect.y = 0;
        full_rect.w = texture->w;
        full_rect.h = texture->h;
        rect = &full_rect;
    }
    return renderer->LockTexture(renderer, texture, rect, markDirty, pixels,
                                 pitch);
}

void
SDL_UnlockTexture(SDL_Texture * texture)
{
    SDL_Renderer *renderer;

    CHECK_TEXTURE_MAGIC(texture, );

    if (texture->access != SDL_TEXTUREACCESS_STREAMING) {
        return;
    }
    renderer = texture->renderer;
    if (!renderer->UnlockTexture) {
        return;
    }
    renderer->UnlockTexture(renderer, texture);
}

void
SDL_DirtyTexture(SDL_Texture * texture, int numrects,
                 const SDL_Rect * rects)
{
    SDL_Renderer *renderer;

    CHECK_TEXTURE_MAGIC(texture, );

    if (texture->access != SDL_TEXTUREACCESS_STREAMING) {
        return;
    }
    renderer = texture->renderer;
    if (!renderer->DirtyTexture) {
        return;
    }
    renderer->DirtyTexture(renderer, texture, numrects, rects);
}

int
SDL_SetRenderDrawColor(SDL_Renderer * renderer,
                       Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    CHECK_RENDERER_MAGIC(renderer, -1);

    renderer->r = r;
    renderer->g = g;
    renderer->b = b;
    renderer->a = a;
    return 0;
}

int
SDL_GetRenderDrawColor(SDL_Renderer * renderer,
                       Uint8 * r, Uint8 * g, Uint8 * b, Uint8 * a)
{
    CHECK_RENDERER_MAGIC(renderer, -1);

    if (r) {
        *r = renderer->r;
    }
    if (g) {
        *g = renderer->g;
    }
    if (b) {
        *b = renderer->b;
    }
    if (a) {
        *a = renderer->a;
    }
    return 0;
}

int
SDL_SetRenderDrawBlendMode(SDL_Renderer * renderer, SDL_BlendMode blendMode)
{
    CHECK_RENDERER_MAGIC(renderer, -1);

    renderer->blendMode = blendMode;
    return 0;
}

int
SDL_GetRenderDrawBlendMode(SDL_Renderer * renderer, SDL_BlendMode *blendMode)
{
    CHECK_RENDERER_MAGIC(renderer, -1);

    *blendMode = renderer->blendMode;
    return 0;
}

int
SDL_RenderClear(SDL_Renderer * renderer)
{
    CHECK_RENDERER_MAGIC(renderer, -1);

    if (!renderer->RenderClear) {
        SDL_BlendMode blendMode = renderer->blendMode;
        int status;

        if (blendMode >= SDL_BLENDMODE_BLEND) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }

        status = SDL_RenderFillRect(renderer, NULL);

        if (blendMode >= SDL_BLENDMODE_BLEND) {
            SDL_SetRenderDrawBlendMode(renderer, blendMode);
        }
        return status;
    }
    return renderer->RenderClear(renderer);
}

int
SDL_RenderDrawPoint(SDL_Renderer * renderer, int x, int y)
{
    SDL_Point point;

    point.x = x;
    point.y = y;
    return SDL_RenderDrawPoints(renderer, &point, 1);
}

int
SDL_RenderDrawPoints(SDL_Renderer * renderer,
                     const SDL_Point * points, int count)
{
    CHECK_RENDERER_MAGIC(renderer, -1);

    if (!points) {
        SDL_SetError("SDL_RenderDrawPoints(): Passed NULL points");
        return -1;
    }
    if (count < 1) {
        return 0;
    }
    return renderer->RenderDrawPoints(renderer, points, count);
}

int
SDL_RenderDrawLine(SDL_Renderer * renderer, int x1, int y1, int x2, int y2)
{
    SDL_Point points[2];

    points[0].x = x1;
    points[0].y = y1;
    points[1].x = x2;
    points[1].y = y2;
    return SDL_RenderDrawLines(renderer, points, 2);
}

int
SDL_RenderDrawLines(SDL_Renderer * renderer,
                    const SDL_Point * points, int count)
{
    CHECK_RENDERER_MAGIC(renderer, -1);

    if (!points) {
        SDL_SetError("SDL_RenderDrawLines(): Passed NULL points");
        return -1;
    }
    if (count < 2) {
        return 0;
    }
    return renderer->RenderDrawLines(renderer, points, count);
}

int
SDL_RenderDrawRect(SDL_Renderer * renderer, const SDL_Rect * rect)
{
    SDL_Rect full_rect;
    SDL_Point points[5];

    CHECK_RENDERER_MAGIC(renderer, -1);

    /* If 'rect' == NULL, then outline the whole surface */
    if (!rect) {
        SDL_Window *window = renderer->window;

        full_rect.x = 0;
        full_rect.y = 0;
        SDL_GetWindowSize(window, &full_rect.w, &full_rect.h);
        rect = &full_rect;
    }

    points[0].x = rect->x;
    points[0].y = rect->y;
    points[1].x = rect->x+rect->w-1;
    points[1].y = rect->y;
    points[2].x = rect->x+rect->w-1;
    points[2].y = rect->y+rect->h-1;
    points[3].x = rect->x;
    points[3].y = rect->y+rect->h-1;
    points[4].x = rect->x;
    points[4].y = rect->y;
    return SDL_RenderDrawLines(renderer, points, 5);
}

int
SDL_RenderDrawRects(SDL_Renderer * renderer,
                    const SDL_Rect ** rects, int count)
{
    int i;

    CHECK_RENDERER_MAGIC(renderer, -1);

    if (!rects) {
        SDL_SetError("SDL_RenderDrawRects(): Passed NULL rects");
        return -1;
    }
    if (count < 1) {
        return 0;
    }

    /* Check for NULL rect, which means fill entire window */
    for (i = 0; i < count; ++i) {
        if (SDL_RenderDrawRect(renderer, rects[i]) < 0) {
            return -1;
        }
    }
    return 0;
}

int
SDL_RenderFillRect(SDL_Renderer * renderer, const SDL_Rect * rect)
{
    return SDL_RenderFillRects(renderer, &rect, 1);
}

int
SDL_RenderFillRects(SDL_Renderer * renderer,
                    const SDL_Rect ** rects, int count)
{
    int i;

    CHECK_RENDERER_MAGIC(renderer, -1);

    if (!rects) {
        SDL_SetError("SDL_RenderFillRects(): Passed NULL rects");
        return -1;
    }
    if (count < 1) {
        return 0;
    }

    /* Check for NULL rect, which means fill entire window */
    for (i = 0; i < count; ++i) {
        if (rects[i] == NULL) {
            SDL_Window *window = renderer->window;
            SDL_Rect full_rect;
            const SDL_Rect *rect;

            full_rect.x = 0;
            full_rect.y = 0;
            SDL_GetWindowSize(window, &full_rect.w, &full_rect.h);
            rect = &full_rect;
            return renderer->RenderFillRects(renderer, &rect, 1);
        }
    }
    return renderer->RenderFillRects(renderer, rects, count);
}

int
SDL_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    SDL_Window *window;
    SDL_Rect real_srcrect;
    SDL_Rect real_dstrect;

    CHECK_RENDERER_MAGIC(renderer, -1);
    CHECK_TEXTURE_MAGIC(texture, -1);

    if (renderer != texture->renderer) {
        SDL_SetError("Texture was not created with this renderer");
        return -1;
    }
    window = renderer->window;

    real_srcrect.x = 0;
    real_srcrect.y = 0;
    real_srcrect.w = texture->w;
    real_srcrect.h = texture->h;
    if (srcrect) {
        if (!SDL_IntersectRect(srcrect, &real_srcrect, &real_srcrect)) {
            return 0;
        }
    }

    real_dstrect.x = 0;
    real_dstrect.y = 0;
    SDL_GetWindowSize(window, &real_dstrect.w, &real_dstrect.h);
    if (dstrect) {
        if (!SDL_IntersectRect(dstrect, &real_dstrect, &real_dstrect)) {
            return 0;
        }
        /* Clip srcrect by the same amount as dstrect was clipped */
        if (dstrect->w != real_dstrect.w) {
            int deltax = (real_dstrect.x - dstrect->x);
            int deltaw = (real_dstrect.w - dstrect->w);
            real_srcrect.x += (deltax * real_srcrect.w) / dstrect->w;
            real_srcrect.w += (deltaw * real_srcrect.w) / dstrect->w;
        }
        if (dstrect->h != real_dstrect.h) {
            int deltay = (real_dstrect.y - dstrect->y);
            int deltah = (real_dstrect.h - dstrect->h);
            real_srcrect.y += (deltay * real_srcrect.h) / dstrect->h;
            real_srcrect.h += (deltah * real_srcrect.h) / dstrect->h;
        }
    }

    return renderer->RenderCopy(renderer, texture, &real_srcrect,
                                &real_dstrect);
}

int
SDL_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                     Uint32 format, void * pixels, int pitch)
{
    SDL_Window *window;
    SDL_Rect real_rect;

    CHECK_RENDERER_MAGIC(renderer, -1);

    if (!renderer->RenderReadPixels) {
        SDL_Unsupported();
        return -1;
    }
    window = renderer->window;

    if (!format) {
        format = SDL_GetWindowPixelFormat(window);
    }

    real_rect.x = 0;
    real_rect.y = 0;
    SDL_GetWindowSize(window, &real_rect.w, &real_rect.h);
    if (rect) {
        if (!SDL_IntersectRect(rect, &real_rect, &real_rect)) {
            return 0;
        }
        if (real_rect.y > rect->y) {
            pixels = (Uint8 *)pixels + pitch * (real_rect.y - rect->y);
        }
        if (real_rect.x > rect->x) {
            int bpp = SDL_BYTESPERPIXEL(SDL_GetWindowPixelFormat(window));
            pixels = (Uint8 *)pixels + bpp * (real_rect.x - rect->x);
        }
    }

    return renderer->RenderReadPixels(renderer, &real_rect,
                                      format, pixels, pitch);
}

int
SDL_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                      Uint32 format, const void * pixels, int pitch)
{
    SDL_Window *window;
    SDL_Rect real_rect;

    CHECK_RENDERER_MAGIC(renderer, -1);

    if (!renderer->RenderWritePixels) {
        SDL_Unsupported();
        return -1;
    }
    window = renderer->window;

    if (!format) {
        format = SDL_GetWindowPixelFormat(window);
    }

    real_rect.x = 0;
    real_rect.y = 0;
    SDL_GetWindowSize(window, &real_rect.w, &real_rect.h);
    if (rect) {
        if (!SDL_IntersectRect(rect, &real_rect, &real_rect)) {
            return 0;
        }
        if (real_rect.y > rect->y) {
            pixels = (const Uint8 *)pixels + pitch * (real_rect.y - rect->y);
        }
        if (real_rect.x > rect->x) {
            int bpp = SDL_BYTESPERPIXEL(SDL_GetWindowPixelFormat(window));
            pixels = (const Uint8 *)pixels + bpp * (real_rect.x - rect->x);
        }
    }

    return renderer->RenderWritePixels(renderer, &real_rect,
                                       format, pixels, pitch);
}

void
SDL_RenderPresent(SDL_Renderer * renderer)
{
    CHECK_RENDERER_MAGIC(renderer, );

    renderer->RenderPresent(renderer);
}

void
SDL_DestroyTexture(SDL_Texture * texture)
{
    SDL_Renderer *renderer;

    CHECK_TEXTURE_MAGIC(texture, );
    texture->magic = NULL;

    renderer = texture->renderer;
    if (texture->next) {
        texture->next->prev = texture->prev;
    }
    if (texture->prev) {
        texture->prev->next = texture->next;
    } else {
        renderer->textures = texture->next;
    }

    renderer->DestroyTexture(renderer, texture);
    SDL_free(texture);
}

void
SDL_DestroyRenderer(SDL_Renderer * renderer)
{
    CHECK_RENDERER_MAGIC(renderer, );

    SDL_DelEventWatch(SDL_RendererEventWatch, renderer);

    /* Free existing textures for this renderer */
    while (renderer->textures) {
        SDL_DestroyTexture(renderer->textures);
    }

    /* It's no longer magical... */
    renderer->magic = NULL;

    /* Free the renderer instance */
    renderer->DestroyRenderer(renderer);
}

/* vi: set ts=4 sw=4 expandtab: */
