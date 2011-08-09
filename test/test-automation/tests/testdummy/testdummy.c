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

/*! \file
 * Dummy test suite for test runner. This can be used as a base for
 * writing new tests. Dummy suite also works as reference to using
 * various asserts and other (possible) utilities.
 */

#include <stdio.h>
#include <stdint.h>

#include <SDL/SDL.h>

#include "../../include/SDL_test.h"

#include <limits.h>

/* Test case references */
static const TestCaseReference test1 =
		(TestCaseReference){ "dummycase1", "description", TEST_ENABLED, 0, 4};

static const TestCaseReference test2 =
		(TestCaseReference){ "dummycase2", "description", TEST_ENABLED, 0, 0};

static const TestCaseReference test3 =
		(TestCaseReference){ "dummycase3", "description", TEST_ENABLED, 0, 2};

/* Test suite */
extern const TestCaseReference *testSuite[] =  {
	&test1, &test2, &test3, NULL
};


TestCaseReference **QueryTestSuite() {
	return (TestCaseReference **)testSuite;
}

/* Create test fixture */

/*!
 * SetUp function can be used to create a test fixture for test cases.
 * The function will be called right before executing the test case.
 *
 * Note: If any assert in the function fails then the test will be skipped.
 * In practice, the entire suite will be skipped if assert failure happens.
 *
 * Note: this function is optional.
 *
 * \param arg parameters given to test. Usually NULL
 */
void
SetUp(void *arg)
{
	// create test fixture,
	// for example, set up static variables used by test cases here
}

/*!
 * TearDown function can be used to destroy a test fixture for test cases.
 * The function will be called right after executing the test case.
 *
 * Note: this function is optional.
 *
 * \param arg parameters given to test. Usually NULL
 */
void
TearDown(void *arg)
{
	// destroy test fixture
}

/* Test case functions */
void
dummycase1(void *arg)
{
	AssertEquals(5, 5, "Assert message");

	/*
	for( ; 1 ; )
		Log(0, "uint8 (same value): %u", RandomPositiveInteger());
	// */

	//Log(0, "uint8 (same value): %d", RandomUint8BoundaryValue(200, 200, SDL_TRUE));


	for( ; 1 ; ) {
		//Log(0, "sint8: %d", RandomSint8BoundaryValue(-11, 10, SDL_TRUE));
		//Log(0, "sint16: %d", RandomSint16BoundaryValue(SHRT_MIN, SHRT_MAX, SDL_FALSE));
		Log(0, "sint32: %d", RandomSint32BoundaryValue(INT_MIN, 3000, SDL_FALSE));
		//Log(0, "sint64: %lld", RandomSint64BoundaryValue(-34, -34, SDL_FALSE));
	}

	for(; 0 ;) {
		//Log(0, "int8: %u", RandomUint8BoundaryValue(0, 255, SDL_FALSE));
		//Log(0, "uint16: %u", RandomUint16BoundaryValue(0, UINT16_MAX, SDL_FALSE));
		//Log(0, "int32: %u", RandomUint32BoundaryValue(0, 0xFFFFFFFE, SDL_FALSE));
		Log(0, "int64: %llu", RandomUint64BoundaryValue(2, 0xFFFFFFFFFFFFFFFE, SDL_FALSE));
	}

	for(; 0 ;) {
		int min = -5;
		int max = 5;
		int random =  RandomIntegerInRange(min, max);
		if(random < min || random > max ) {
			AssertFail("Generated incorrect integer");
		}
		Log(0, "%d", random);
	}

	//Log(0, "Random: %s", RandomAsciiString());
}

void
dummycase2(void *arg)
{
	char *msg = "eello";
	//msg[0] = 'H';
	AssertTrue(1, "Assert message");
}

void
dummycase3(void *arg)
{
	while(0);
	//AssertTrue(1, "Assert message");
}

