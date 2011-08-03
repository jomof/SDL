/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

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

#include "SDL_bframebuffer.h"

#include <AppKit.h>
#include <InterfaceKit.h>
#include "SDL_bmodes.h"
#include "SDL_BWin.h"

#include "../../main/beos/SDL_BApp.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline SDL_BWin *_ToBeWin(SDL_Window *window) {
	return ((SDL_BWin*)(window->driverdata));
}

static inline SDL_BApp *_GetBeApp() {
	return ((SDL_BApp*)be_app);
}

int BE_CreateWindowFramebuffer(_THIS, SDL_Window * window,
                                       Uint32 * format,
                                       void ** pixels, int *pitch) {
	SDL_BWin *bwin = _ToBeWin(window);
	BScreen bscreen;
	if(!bscreen.IsValid()) {
		return -1;
	}

	while(!bwin->Connected()) { snooze(100); }
	
	/* Make sure we have exclusive access to frame buffer data */
	bwin->LockBuffer();

	/* format */
	display_mode bmode;
	bscreen.GetMode(&bmode);
	int32 bpp = ColorSpaceToBitsPerPixel(bmode.space);
	*format = BPPToSDLPxFormat(bpp);

	/* pitch = width of screen, in bytes */
	*pitch = bwin->GetFbWidth() * bwin->GetBytesPerPx();

	/* Create a copy of the pixel buffer if it doesn't recycle */
	*pixels = bwin->GetWindowFramebuffer();
	if( (*pixels) != NULL ) {
		SDL_free(*pixels);
	}
	*pixels = SDL_calloc((*pitch) * bwin->GetFbHeight() * 
		bwin->GetBytesPerPx(), sizeof(uint8));
	bwin->SetWindowFramebuffer((uint8*)(*pixels));

	bwin->SetBufferExists(true);
	bwin->SetTrashBuffer(false);
	bwin->UnlockBuffer();
	return 0;
}



int BE_UpdateWindowFramebuffer(_THIS, SDL_Window * window,
                                      SDL_Rect * rects, int numrects) {
	if(!window)
		return 0;

	SDL_BWin *bwin = _ToBeWin(window);
	
	bwin->LockBuffer();
	bwin->SetBufferDirty(true);
	bwin->UnlockBuffer();

	return 0;
}

int32 BE_DrawThread(void *data) {
	SDL_BWin *bwin = (SDL_BWin*)data;
	SDL_Window *window = _GetBeApp()->GetSDLWindow(bwin->GetID());
	
	BScreen bscreen;
	if(!bscreen.IsValid()) {
		return -1;
	}

	while(bwin->ConnectionEnabled()) {
		if( bwin->Connected() && bwin->BufferExists() && bwin->BufferIsDirty() ) {
			bwin->LockBuffer();
			int32 windowPitch = window->surface->pitch;
			int32 bufferPitch = bwin->GetRowBytes();
			uint8 *windowpx;
			uint8 *bufferpx;

			int32 BPP = bwin->GetBytesPerPx();
			uint8 *windowBaseAddress = (uint8*)window->surface->pixels;
			int32 windowSub = bwin->GetFbX() * BPP +
						  bwin->GetFbY() * windowPitch;
			clipping_rect *clips = bwin->GetClips();
			int32 numClips = bwin->GetNumClips();
			int i, y;

			/* Blit each clipping rectangle */
			bscreen.WaitForRetrace();
			for(i = 0; i < numClips; ++i) {
				clipping_rect rc = clips[i];
				/* Get addresses of the start of each clipping rectangle */
				int32 width = clips[i].right - clips[i].left + 1;
				int32 height = clips[i].bottom - clips[i].top + 1;
				bufferpx = bwin->GetBufferPx() + 
					clips[i].top * bufferPitch + clips[i].left * BPP;
				windowpx = windowBaseAddress + 
					clips[i].top * windowPitch + clips[i].left * BPP - windowSub;

				/* Copy each row of pixels from the window buffer into the frame
				   buffer */
				for(y = 0; y < height; ++y)
				{
					if(bwin->CanTrashWindowBuffer()) {
						goto escape;	/* Break out before the buffer is killed */
					}
					memcpy(bufferpx, windowpx, width * BPP);
					bufferpx += bufferPitch;
					windowpx += windowPitch;
				}
			}
			bwin->SetBufferDirty(false);
escape:
			bwin->UnlockBuffer();
		} else {
			snooze(16000);
		}
	}
	
	return B_OK;
}

void BE_DestroyWindowFramebuffer(_THIS, SDL_Window * window) {
	SDL_BWin *bwin = _ToBeWin(window);
	
	bwin->LockBuffer();
	
	/* Free and clear the window buffer */
	uint8* winBuffer = bwin->GetWindowFramebuffer();
	SDL_free(winBuffer);
	bwin->SetWindowFramebuffer(NULL);
	bwin->SetBufferExists(false);
	bwin->UnlockBuffer();
}

#ifdef __cplusplus
}
#endif
