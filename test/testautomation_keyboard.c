/**
 * Keyboard test suite
 */

#include <stdio.h>

#include "SDL.h"
#include "SDL_test.h"

/* ================= Test Case Implementation ================== */

/* Test case functions */

/**
 * @brief Check call to SDL_GetKeyboardState with and without numkeys reference.
 * 
 * @sa http://wiki.libsdl.org/moin.cgi/SDL_GetKeyboardState
 */
int
keyboard_getKeyboardState(void *arg)
{
   int numkeys;
   Uint8 *state;

   /* Case where numkeys pointer is NULL */    
   state = SDL_GetKeyboardState(NULL);
   SDLTest_AssertPass("Call to SDL_GetKeyboardState(NULL)");
   SDLTest_AssertCheck(state != NULL, "Validate that return value from SDL_GetKeyboardState is not NULL");

   /* Case where numkeys pointer is not NULL */
   numkeys = -1;
   state = SDL_GetKeyboardState(&numkeys);
   SDLTest_AssertPass("Call to SDL_GetKeyboardState(&numkeys)");
   SDLTest_AssertCheck(state != NULL, "Validate that return value from SDL_GetKeyboardState is not NULL");
   SDLTest_AssertCheck(numkeys >= 0, "Validate that value of numkeys is >= 0, got: %i", numkeys);
   
   return TEST_COMPLETED;
}

/**
 * @brief Check call to SDL_GetKeyboardFocus
 * 
 * @sa http://wiki.libsdl.org/moin.cgi/SDL_GetKeyboardFocus
 */
int
keyboard_getKeyboardFocus(void *arg)
{
   SDL_Window* window;

   /* Call, but ignore return value */
   window = SDL_GetKeyboardFocus();
   SDLTest_AssertPass("Call to SDL_GetKeyboardFocus()");

   return TEST_COMPLETED;
}

/**
 * @brief Check call to SDL_GetKeyFromName for known, unknown and invalid name.
 * 
 * @sa http://wiki.libsdl.org/moin.cgi/SDL_GetKeyFromName
 */
int
keyboard_getKeyFromName(void *arg)
{
   SDL_Keycode result;

   /* Case where Key is known, 1 character input */
   result = SDL_GetKeyFromName("A");
   SDLTest_AssertPass("Call to SDL_GetKeyFromName(known/single)");
   SDLTest_AssertCheck(result == SDLK_a, "Verify result from call, expected: %i, got: %i", SDLK_a, result);

   /* Case where Key is known, 2 character input */
   result = SDL_GetKeyFromName("F1");
   SDLTest_AssertPass("Call to SDL_GetKeyFromName(known/double)");
   SDLTest_AssertCheck(result == SDLK_F1, "Verify result from call, expected: %i, got: %i", SDLK_F1, result);

   /* Case where Key is known, 3 character input */
   result = SDL_GetKeyFromName("End");
   SDLTest_AssertPass("Call to SDL_GetKeyFromName(known/triple)");
   SDLTest_AssertCheck(result == SDLK_END, "Verify result from call, expected: %i, got: %i", SDLK_END, result);

   /* Case where Key is known, 4 character input */
   result = SDL_GetKeyFromName("Find");
   SDLTest_AssertPass("Call to SDL_GetKeyFromName(known/quad)");
   SDLTest_AssertCheck(result == SDLK_FIND, "Verify result from call, expected: %i, got: %i", SDLK_FIND, result);

   /* Case where Key is known, multiple character input */
   result = SDL_GetKeyFromName("AudioStop");
   SDLTest_AssertPass("Call to SDL_GetKeyFromName(known/multi)");
   SDLTest_AssertCheck(result == SDLK_AUDIOSTOP, "Verify result from call, expected: %i, got: %i", SDLK_AUDIOSTOP, result);

   /* Case where Key is unknown */
   result = SDL_GetKeyFromName("NotThere");
   SDLTest_AssertPass("Call to SDL_GetKeyFromName(unknown)");
   SDLTest_AssertCheck(result == SDLK_UNKNOWN, "Verify result from call is UNKNOWN, expected: %i, got: %i", SDLK_UNKNOWN, result);

   /* Case where input is NULL/invalid */
   result = SDL_GetKeyFromName(NULL);
   SDLTest_AssertPass("Call to SDL_GetKeyFromName(NULL)");
   SDLTest_AssertCheck(result == SDLK_UNKNOWN, "Verify result from call is UNKNOWN, expected: %i, got: %i", SDLK_UNKNOWN, result);

   return TEST_COMPLETED;
}

/**
 * @brief Check call to SDL_GetKeyFromScancode
 * 
 * @sa http://wiki.libsdl.org/moin.cgi/SDL_GetKeyFromScancode
 */
int
keyboard_getKeyFromScancode(void *arg)
{
   SDL_Keycode result;

   /* Case where input is valid */
   result = SDL_GetKeyFromScancode(SDL_SCANCODE_A);
   SDLTest_AssertPass("Call to SDL_GetKeyFromScancode(valid)");
   SDLTest_AssertCheck(result == SDLK_a, "Verify result from call, expected: %i, got: %i", SDLK_a, result);

   /* Case where input is zero */
   result = SDL_GetKeyFromScancode(0);
   SDLTest_AssertPass("Call to SDL_GetKeyFromScancode(zero)");
   SDLTest_AssertCheck(result == SDLK_UNKNOWN, "Verify result from call is UNKNOWN, expected: %i, got: %i", SDLK_UNKNOWN, result);

   /* Case where input is invalid */
   result = SDL_GetKeyFromScancode(-999);
   SDLTest_AssertPass("Call to SDL_GetKeyFromScancode(invalid)");
   SDLTest_AssertCheck(result == SDLK_UNKNOWN, "Verify result from call is UNKNOWN, expected: %i, got: %i", SDLK_UNKNOWN, result);

   return TEST_COMPLETED;
}

/**
 * @brief Check call to SDL_GetKeyName
 * 
 * @sa http://wiki.libsdl.org/moin.cgi/SDL_GetKeyName
 */
int
keyboard_getKeyName(void *arg)
{   
   char *result;
   char *expected;

   /* Case where key has a 1 character name */
   expected = "3";
   result = (char *)SDL_GetKeyName(SDLK_3);
   SDLTest_AssertPass("Call to SDL_GetKeyName()");
   SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
   SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: %s, got: %s", expected, result);

   /* Case where key has a 2 character name */
   expected = "F1";
   result = (char *)SDL_GetKeyName(SDLK_F1);
   SDLTest_AssertPass("Call to SDL_GetKeyName()");
   SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
   SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: %s, got: %s", expected, result);

   /* Case where key has a 3 character name */
   expected = "Cut";
   result = (char *)SDL_GetKeyName(SDLK_CUT);
   SDLTest_AssertPass("Call to SDL_GetKeyName()");
   SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
   SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: %s, got: %s", expected, result);

   /* Case where key has a 4 character name */
   expected = "Down";
   result = (char *)SDL_GetKeyName(SDLK_DOWN);
   SDLTest_AssertPass("Call to SDL_GetKeyName()");
   SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
   SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: %s, got: %s", expected, result);

   /* Case where key has a N character name */
   expected = "BrightnessUp";
   result = (char *)SDL_GetKeyName(SDLK_BRIGHTNESSUP);
   SDLTest_AssertPass("Call to SDL_GetKeyName()");
   SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
   SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: %s, got: %s", expected, result);

   /* Case where key has a N character name with space */
   expected = "Keypad MemStore";
   result = (char *)SDL_GetKeyName(SDLK_KP_MEMSTORE);
   SDLTest_AssertPass("Call to SDL_GetKeyName()");
   SDLTest_AssertCheck(result != NULL, "Verify result from call is not NULL");
   SDLTest_AssertCheck(SDL_strcmp(result, expected) == 0, "Verify result from call is valid, expected: %s, got: %s", expected, result);

   return TEST_COMPLETED;
}

/**
 * @brief Check call to SDL_GetModState and SDL_SetModState
 * 
 * @sa http://wiki.libsdl.org/moin.cgi/SDL_GetModState
 * @sa http://wiki.libsdl.org/moin.cgi/SDL_SetModState
 */
int
keyboard_getSetModState(void *arg)
{   
   SDL_Keymod result;
   SDL_Keymod currentState;
   SDL_Keymod newState;
   SDL_Keymod allStates =     
    KMOD_NONE |
    KMOD_LSHIFT |
    KMOD_RSHIFT |
    KMOD_LCTRL |
    KMOD_RCTRL |
    KMOD_LALT |
    KMOD_RALT |
    KMOD_LGUI |
    KMOD_RGUI |
    KMOD_NUM |
    KMOD_CAPS |
    KMOD_MODE |
    KMOD_RESERVED;

   /* Get state, cache for later reset */                                                   
   result = SDL_GetModState();
   SDLTest_AssertPass("Call to SDL_GetModState()");
   SDLTest_AssertCheck(result >=0 && result <= allStates, "Verify result from call is valid, expected: 0 <= result <= %i, got: %i", allStates, result);   
   currentState = result;

   /* Set random state */   
   newState = SDLTest_RandomIntegerInRange(0, allStates);
   SDL_SetModState(newState);
   SDLTest_AssertPass("Call to SDL_SetModState(%i)", newState);
   result = SDL_GetModState();
   SDLTest_AssertPass("Call to SDL_GetModState()");
   SDLTest_AssertCheck(result == newState, "Verify result from call is valid, expected: %i, got: %i", newState, result);

   /* Set zero state */
   SDL_SetModState(0);
   SDLTest_AssertPass("Call to SDL_SetModState(0)");
   result = SDL_GetModState();
   SDLTest_AssertPass("Call to SDL_GetModState()");
   SDLTest_AssertCheck(result == 0, "Verify result from call is valid, expected: 0, got: %i", result);

   /* Revert back to cached current state if needed */
   if (currentState != 0) {
     SDL_SetModState(currentState);
     SDLTest_AssertPass("Call to SDL_SetModState(%i)", currentState);
     result = SDL_GetModState();
     SDLTest_AssertPass("Call to SDL_GetModState()");
     SDLTest_AssertCheck(result == currentState, "Verify result from call is valid, expected: %i, got: %i", currentState, result);
   }

   return TEST_COMPLETED;
}


/**
 * @brief Check call to SDL_StartTextInput and SDL_StopTextInput
 * 
 * @sa http://wiki.libsdl.org/moin.cgi/SDL_StartTextInput
 * @sa http://wiki.libsdl.org/moin.cgi/SDL_StopTextInput
 */
int
keyboard_startStopTextInput(void *arg)
{   
   /* Start-Stop */
   SDL_StartTextInput();
   SDLTest_AssertPass("Call to SDL_StartTextInput()");
   SDL_StopTextInput();
   SDLTest_AssertPass("Call to SDL_StopTextInput()");

   /* Stop-Start */
   SDL_StartTextInput();
   SDLTest_AssertPass("Call to SDL_StartTextInput()");
   
   /* Start-Start */
   SDL_StartTextInput();
   SDLTest_AssertPass("Call to SDL_StartTextInput()");

   /* Stop-Stop */   
   SDL_StopTextInput();
   SDLTest_AssertPass("Call to SDL_StopTextInput()");
   SDL_StopTextInput();
   SDLTest_AssertPass("Call to SDL_StopTextInput()");

   return TEST_COMPLETED;
}



/* ================= Test References ================== */

/* Keyboard test cases */
static const SDLTest_TestCaseReference keyboardTest1 =
		{ (SDLTest_TestCaseFp)keyboard_getKeyboardState, "keyboard_getKeyboardState", "Check call to SDL_GetKeyboardState with and without numkeys reference", TEST_ENABLED };

static const SDLTest_TestCaseReference keyboardTest2 =
		{ (SDLTest_TestCaseFp)keyboard_getKeyboardFocus, "keyboard_getKeyboardFocus", "Check call to SDL_GetKeyboardFocus", TEST_ENABLED };

static const SDLTest_TestCaseReference keyboardTest3 =
		{ (SDLTest_TestCaseFp)keyboard_getKeyFromName, "keyboard_getKeyFromName", "Check call to SDL_GetKeyFromName for known, unknown and invalid name", TEST_ENABLED };

static const SDLTest_TestCaseReference keyboardTest4 =
		{ (SDLTest_TestCaseFp)keyboard_getKeyFromScancode, "keyboard_getKeyFromScancode", "Check call to SDL_GetKeyFromScancode", TEST_ENABLED };

static const SDLTest_TestCaseReference keyboardTest5 =
		{ (SDLTest_TestCaseFp)keyboard_getKeyName, "keyboard_getKeyName", "Check call to SDL_GetKeyName", TEST_ENABLED };

static const SDLTest_TestCaseReference keyboardTest6 =
		{ (SDLTest_TestCaseFp)keyboard_getSetModState, "keyboard_getSetModState", "Check call to SDL_GetModState and SDL_SetModState", TEST_ENABLED };

static const SDLTest_TestCaseReference keyboardTest7 =
		{ (SDLTest_TestCaseFp)keyboard_startStopTextInput, "keyboard_startStopTextInput", "Check call to SDL_StartTextInput and SDL_StopTextInput", TEST_ENABLED };

/* Sequence of Keyboard test cases */
static const SDLTest_TestCaseReference *keyboardTests[] =  {
	&keyboardTest1, &keyboardTest2, &keyboardTest3, &keyboardTest4, &keyboardTest5, &keyboardTest6, &keyboardTest7, NULL
};

/* Keyboard test suite (global) */
SDLTest_TestSuiteReference keyboardTestSuite = {
	"Keyboard",
	NULL,
	keyboardTests,
	NULL
};
