/*
  Copyright (C) 2011 Markus Kauppila <markus.kauppila@gmail.com>

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

#include <stdio.h>

#include <SDL/SDL.h>

#include "../SDL_test.h"

/* Test casess */
static const TestCaseReference test1 =
		(TestCaseReference){ "audio_printOutputDevices", "Checks available output (non-capture) device names.", TEST_ENABLED, 0, 0};

static const TestCaseReference test2 =
		(TestCaseReference){ "audio_printInputDevices", "Checks available input (capture) device names.", TEST_ENABLED, 0, 0};

static const TestCaseReference test3 =
		(TestCaseReference){ "audio_printAudioDrivers", "Checks available audio driver names.", TEST_ENABLED, 0, 0};

static const TestCaseReference test4 =
		(TestCaseReference){ "audio_printCurrentAudioDriver", "Checks current audio driver name with initialized audio.", TEST_ENABLED, 0, 0};

/* Test suite */
extern const TestCaseReference *testSuite[] =  {
	&test1, &test2, &test3, &test4, NULL
};


TestCaseReference **QueryTestSuite() {
	return (TestCaseReference **)testSuite;
}

/* Test case functions */

/**
 * @brief Checks available output (non-capture) device names.
 */
int audio_printOutputDevices()
{
   int ret;
   int i, n;
   char *name;

   /* Start SDL. */
   ret = SDL_Init( SDL_INIT_AUDIO );
   AssertTrue(ret==0, "SDL_Init(SDL_INIT_AUDIO): %s", SDL_GetError());

   /* Get number of devices. */
   n = SDL_GetNumAudioDevices(0);
   AssertTrue(n>=0, "Number of output devices < 0, reported as %i", n);
   
   /* List devices. */
   if (n>0)
   {
      for (i=0; i<n; i++) {
         name = SDL_GetAudioDeviceName(i, 0);
         AssertTrue(name != NULL, "name != NULL");
         AssertTrue(strlen(name)>0, "name blank");
      }
   }

   /* Quit SDL. */
   SDL_Quit();
}

/**
 * @brief Checks available input (capture) device names.
 */
int audio_printInputDevices()
{
   int ret;
   int i, n;
   char *name;

   /* Start SDL. */
   ret = SDL_Init( SDL_INIT_AUDIO );
   AssertTrue(ret==0, "SDL_Init(SDL_INIT_AUDIO): %s", SDL_GetError());

   /* Get number of devices. */
   n = SDL_GetNumAudioDevices(1);
   AssertTrue(n>=0, "Number of input devices < 0, reported as %i", n);
   
   /* List devices. */
   if (n>0)
   {
      for (i=0; i<n; i++) {
         name = SDL_GetAudioDeviceName(i, 1);
         AssertTrue(name != NULL, "name != NULL");
         AssertTrue(strlen(name)>0, "name empty");
      }
   }

   /* Quit SDL. */
   SDL_Quit();
}

/**
 * @brief Checks available audio driver names.
 */
int audio_printAudioDrivers()
{
   int i, n;
   char *name;

   /* Get number of drivers */
   n = SDL_GetNumAudioDrivers();
   AssertTrue(n>=0, "Number of audio drivers >= 0");
   
   /* List drivers. */
   if (n>0)
   {
      for (i=0; i<n; i++) {
         name = SDL_GetAudioDriver(i);
         AssertTrue(name != NULL, "name != NULL");
         AssertTrue(strlen(name)>0, "name empty");
      }
   }
}

/**
 * @brief Checks current audio driver name with initialized audio.
 */
int audio_printCurrentAudioDriver()
{
   int ret;
   char *name;

   /* Start SDL. */
   ret = SDL_Init(SDL_INIT_AUDIO);
   AssertTrue(ret==0, "SDL_Init(SDL_INIT_AUDIO): %s", SDL_GetError());

   /* Check current audio driver */
   name = SDL_GetCurrentAudioDriver();
   AssertTrue(name != NULL, "name != NULL");
   AssertTrue(strlen(name)>0, "name empty");
   
   /* Quit SDL. */
   SDL_Quit();   
}
