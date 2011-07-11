/**
 * Original code: automated SDL surface test written by Edgar Simo "bobbens"
 */

#include <stdio.h>

#include <SDL/SDL.h>

#include "../SDL_test.h"

/* Test case references */
static const TestCaseReference test1 =
		(TestCaseReference){ "surface_testLoad", "Tests sprite loading.", TEST_ENABLED, 0, 0};

static const TestCaseReference test2 =
		(TestCaseReference){ "surface_testBlit", "Tests some blitting routines.", TEST_ENABLED, 0, 0};

static const TestCaseReference test3 =
		(TestCaseReference){ "surface_testBlitBlend", "Tests some more blitting routines.", TEST_ENABLED, 0, 0};

static const TestCaseReference test4 =
		(TestCaseReference){ "surface_testLoadFailure", "Tests sprite loading. A failure case.", TEST_ENABLED, 0, 0};


/* Test suite */
extern const TestCaseReference *testSuite[] =  {
	&test1, &test2, &test3, &test4, NULL
};


TestCaseReference **QueryTestSuite() {
	return (TestCaseReference **)testSuite;
}

/* Helper functions for the test cases */

#define TEST_SURFACE_WIDTH 80
#define TEST_SURFACE_HEIGHT 60

/*!
 * Creates test surface
 */
SDL_Surface *
_CreateTestSurface()
{
	SDL_Surface *testsur = NULL;

	/* Create the test surface. */
	testsur = SDL_CreateRGBSurface( 0,
			TEST_SURFACE_WIDTH, TEST_SURFACE_HEIGHT, 32,
			RMASK, GMASK, BMASK, AMASK );

	if(testsur->w != TEST_SURFACE_WIDTH) {
		AssertFail("Test surface width doesn't match");
	}

	if(testsur->h != TEST_SURFACE_HEIGHT) {
		AssertFail("Test surface height doesn't match");
	}

	AssertTrue(testsur != NULL, "SDL_CreateRGBSurface");

	return testsur;
}

/**
 * @brief Tests a blend mode.
 */
int _testBlitBlendMode(SDL_Surface *testsur, SDL_Surface *face, int mode)
{
	int ret;
	int i, j, ni, nj;
	SDL_Rect rect;

	AssertTrue(testsur != NULL, "testsur != NULL");
	AssertTrue(face != NULL, "face != NULL");

	/* Clear surface. */
	ret = SDL_FillRect( testsur, NULL,
		 SDL_MapRGB( testsur->format, 0, 0, 0 ) );
	 AssertTrue(ret != 0, "SDL_FillRect");

	/* Steps to take. */
	ni     = testsur->w - face->w;
	nj     = testsur->h - face->h;

	/* Constant values. */
	rect.w = face->w;
	rect.h = face->h;

	/* Test blend mode. */
	for (j=0; j <= nj; j+=4) {
	  for (i=0; i <= ni; i+=4) {
		 /* Set blend mode. */
		 ret = SDL_SetSurfaceBlendMode( face, mode );
		 AssertTrue(ret != 0, "SDL_SetSurfaceBlendMode");

		 /* Blitting. */
		 rect.x = i;
		 rect.y = j;
		 // TODO Add pixel level validation, SDL_BlitSurface might be no-op
		 ret = SDL_BlitSurface( face, NULL, testsur, &rect );
		 AssertTrue(ret != 0, "SDL_BlitSurface");	  }
	}

	return 0;
}

/* Test case functions */
/**
 * @brief Tests sprite loading.
 */
void surface_testLoad(void *arg)
{
	int ret;
    SDL_Surface *face, *rface;

	ret = SDL_Init(SDL_INIT_VIDEO);
	AssertTrue(ret == 0, "SDL_Init(SDL_INIT_VIDEO)");

	SDL_Surface *testsur = _CreateTestSurface();

   /* Clear surface. */
   ret = SDL_FillRect( testsur, NULL,
         SDL_MapRGB( testsur->format, 0, 0, 0 ) );
	AssertTrue(ret == 0,  "SDL_FillRect");

   /* Create the blit surface. */
#ifdef __APPLE__
	face = SDL_LoadBMP("icon.bmp");
#else
	face = SDL_LoadBMP("../icon.bmp");
#endif

	AssertTrue(face != NULL, "SDL_CreateLoadBmp");

   /* Set transparent pixel as the pixel at (0,0) */
   if (face->format->palette) {
      ret = SDL_SetColorKey(face, SDL_RLEACCEL, *(Uint8 *) face->pixels);
      AssertTrue(ret == 0, "SDL_SetColorKey");
   }

   /* Convert to 32 bit to compare. */
   rface = SDL_ConvertSurface( face, testsur->format, 0 );
   AssertTrue(rface != NULL, "SDL_ConvertSurface");

   /* See if it's the same. */
   AssertTrue(surface_compare( rface, &img_face, 0 ) == 0,
		   "Comparing primitives output.");

   /* Clean up. */
   SDL_FreeSurface( rface );
   SDL_FreeSurface( face );

   SDL_FreeSurface( testsur );

   SDL_Quit();
}


/**
 * @brief Tests sprite loading. A failure case.
 */
void surface_testLoadFailure(void *arg)
{
	int ret = SDL_Init(SDL_INIT_VIDEO);
	AssertTrue(ret == 0, "SDL_Init(SDL_INIT_VIDEO)");

	SDL_Surface *face = SDL_LoadBMP("nonexistant.bmp");

	AssertTrue(face == NULL, "SDL_CreateLoadBmp");

	SDL_Quit();
}


/**
 * @brief Tests some blitting routines.
 */
void surface_testBlit(void *arg)
{
   int ret;
   SDL_Rect rect;
   SDL_Surface *face;
   int i, j, ni, nj;

	ret = SDL_Init(SDL_INIT_VIDEO);
	AssertTrue(ret == 0, "SDL_Init(SDL_INIT_VIDEO)");

   SDL_Surface *testsur = _CreateTestSurface();

   /* Clear surface. */
   ret = SDL_FillRect( testsur, NULL,
         SDL_MapRGB( testsur->format, 0, 0, 0 ) );

   AssertTrue(ret == 0, "SDL_FillRect");

   /* Create face surface. */
   face = SDL_CreateRGBSurfaceFrom( (void*)img_face.pixel_data,
         img_face.width, img_face.height, 32, img_face.width*4,
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
         0xff000000, /* Red bit mask. */
         0x00ff0000, /* Green bit mask. */
         0x0000ff00, /* Blue bit mask. */
         0x000000ff /* Alpha bit mask. */
#else
         0x000000ff, /* Red bit mask. */
         0x0000ff00, /* Green bit mask. */
         0x00ff0000, /* Blue bit mask. */
         0xff000000 /* Alpha bit mask. */
#endif
         );
   AssertTrue(face != NULL, "SDL_CreateRGBSurfaceFrom");

   /* Constant values. */
   rect.w = face->w;
   rect.h = face->h;
   ni     = testsur->w - face->w;
   nj     = testsur->h - face->h;

   /* Loop blit. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Blitting. */
         rect.x = i;
         rect.y = j;
         // TODO Add pixel level validation, SDL_BlitSurface might be no-op
         ret = SDL_BlitSurface( face, NULL, testsur, &rect );

         AssertTrue(ret == 0, "SDL_BlitSurface");
      }
   }

   /* See if it's the same. */
   AssertTrue(surface_compare( testsur, &img_blit, 0 ) == 0,
		   "Comparing blitting output (normal blit).");

   /* Clear surface. */
   ret = SDL_FillRect( testsur, NULL,
         SDL_MapRGB( testsur->format, 0, 0, 0 ) );
   AssertTrue(ret == 0, "SDL_FillRect");

   /* Test blitting with colour mod. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Set colour mod. */
         ret = SDL_SetSurfaceColorMod( face, (255/nj)*j, (255/ni)*i, (255/nj)*j );
         AssertTrue(ret == 0, "SDL_SetSurfaceColorMod");

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         // TODO Add pixel level validation, SDL_BlitSurface might be no-op
         ret = SDL_BlitSurface( face, NULL, testsur, &rect );

         AssertTrue(ret == 0, "SDL_BlitSurface");
      }
   }

   /* See if it's the same. */
   AssertTrue(surface_compare( testsur, &img_blitColour, 0 ) == 0,
		   "Comparing blitting output (using SDL_SetSurfaceColorMod).");

   /* Clear surface. */
   ret = SDL_FillRect( testsur, NULL,
         SDL_MapRGB( testsur->format, 0, 0, 0 ) );
   AssertTrue(ret == 0, "SDL_FillRect");

   /* Restore colour. */
   ret = SDL_SetSurfaceColorMod( face, 255, 255, 255 );
   AssertTrue(ret == 0, "SDL_SetSurfaceColorMod");

   /* Test blitting with colour mod. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {
         /* Set alpha mod. */
         ret = SDL_SetSurfaceAlphaMod( face, (255/ni)*i );
         AssertTrue(ret == 0, "SDL_SetSurfaceAlphaMod");

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         // TODO Add pixel level validation, SDL_BlitSurface might be no-op
         ret = SDL_BlitSurface( face, NULL, testsur, &rect );
         AssertTrue(ret == 0, "SDL_BlitSurface");
      }
   }

   /* See if it's the same. */
   AssertTrue(surface_compare( testsur, &img_blitAlpha, 0 ) == 0,
		   "Comparing blitting output (using SDL_SetSurfaceAlphaMod).");

   /* Clean up. */
   SDL_FreeSurface( face );
   SDL_FreeSurface( testsur );

   SDL_Quit();
}

/**
 * @brief Tests some more blitting routines.
 */
void surface_testBlitBlend(void *arg)
{
   int ret;
   SDL_Rect rect;
   SDL_Surface *face;
   int i, j, ni, nj;
   int mode;

	ret = SDL_Init(SDL_INIT_VIDEO);
	AssertTrue(ret == 0, "SDL_Init(SDL_INIT_VIDEO)");

   SDL_Surface *testsur = _CreateTestSurface();

   /* Clear surface. */
   ret = SDL_FillRect( testsur, NULL,
         SDL_MapRGB( testsur->format, 0, 0, 0 ) );

   AssertTrue(ret == 0, "SDL_FillRect");

   /* Create the blit surface. */
   face = SDL_CreateRGBSurfaceFrom( (void*)img_face.pixel_data,
         img_face.width, img_face.height, 32, img_face.width*4,
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
         0xff000000, /* Red bit mask. */
         0x00ff0000, /* Green bit mask. */
         0x0000ff00, /* Blue bit mask. */
         0x000000ff /* Alpha bit mask. */
#else
         0x000000ff, /* Red bit mask. */
         0x0000ff00, /* Green bit mask. */
         0x00ff0000, /* Blue bit mask. */
         0xff000000 /* Alpha bit mask. */
#endif
         );
   AssertTrue(face != NULL, "SDL_CreateRGBSurfaceFrom");

   /* Set alpha mod. */
   // TODO alpha value could be generated by fuzzer
   ret = SDL_SetSurfaceAlphaMod( face, 100 );
   AssertTrue(ret == 0, "SDL_SetSurfaceAlphaMod");

   /* Steps to take. */
   ni     = testsur->w - face->w;
   nj     = testsur->h - face->h;

   AssertTrue(ni != 0, "ni != 0");
   AssertTrue(nj != 0, "nj != 0");

   /* Constant values. */
   rect.w = face->w;
   rect.h = face->h;

   /* Test None. */
  _testBlitBlendMode( testsur, face, SDL_BLENDMODE_NONE );

   AssertTrue(surface_compare( testsur, &img_blendNone, 0 ) == 0,
   		   "Comparing blitting blending output (using SDL_BLENDMODE_NONE).");

   /* Test Blend. */
   _testBlitBlendMode( testsur, face, SDL_BLENDMODE_BLEND );
   AssertTrue(surface_compare( testsur, &img_blendBlend, 0 ) == 0,
   		   "Comparing blitting blending output (using SDL_BLENDMODE_BLEND).");

   /* Test Add. */
   _testBlitBlendMode( testsur, face, SDL_BLENDMODE_ADD );
   AssertTrue(surface_compare( testsur, &img_blendAdd, 0 ) == 0,
   		      "Comparing blitting blending output (using SDL_BLENDMODE_ADD).");

   /* Test Mod. */
   _testBlitBlendMode( testsur, face, SDL_BLENDMODE_MOD );
   AssertTrue(surface_compare( testsur, &img_blendMod, 0 ) == 0,
   		      "Comparing blitting blending output not the same (using SDL_BLENDMODE_MOD).");

   /* Clear surface. */
   ret = SDL_FillRect( testsur, NULL,
         SDL_MapRGB( testsur->format, 0, 0, 0 ) );

   AssertTrue(ret == 0, "SDL_FillRect");

   /* Loop blit. */
   for (j=0; j <= nj; j+=4) {
      for (i=0; i <= ni; i+=4) {

         /* Set colour mod. */
         ret = SDL_SetSurfaceColorMod( face, (255/nj)*j, (255/ni)*i, (255/nj)*j );
         AssertTrue(ret == 0, "SDL_SetSurfaceColorMod");

         /* Set alpha mod. */
         ret = SDL_SetSurfaceAlphaMod( face, (100/ni)*i );
         AssertTrue(ret == 0, "SDL_SetSurfaceAlphaMod");

         /* Crazy blending mode magic. */
         mode = (i/4*j/4) % 4;
         if (mode==0) mode = SDL_BLENDMODE_NONE;
         else if (mode==1) mode = SDL_BLENDMODE_BLEND;
         else if (mode==2) mode = SDL_BLENDMODE_ADD;
         else if (mode==3) mode = SDL_BLENDMODE_MOD;
         ret = SDL_SetSurfaceBlendMode( face, mode );

         AssertTrue(ret == 0, "SDL_SetSurfaceBlendMode");

         /* Blitting. */
         rect.x = i;
         rect.y = j;
         ret = SDL_BlitSurface( face, NULL, testsur, &rect );
         AssertTrue(ret == 0, "SDL_BlitSurface");
      }
   }

   /* Check to see if matches. */
   AssertTrue(surface_compare( testsur, &img_blendAll, 0 ) == 0,
		      "Surface comparison (surface_compare).");

   /* Clean up. */
   SDL_FreeSurface( face );
   SDL_FreeSurface( testsur );

   SDL_Quit();
}
