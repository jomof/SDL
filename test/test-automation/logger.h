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

#ifndef _LOGGER_H
#define _LOGGER_H

#include <time.h>

/*!
 * Generic logger interface
 *
 */
typedef	void (*RunStartedFp)(int parameterCount, char *runnerParameters[], time_t eventTime);
typedef	void (*RunEndedFp)(int testCount, int suiteCount, int testPassCount, int testFailCount,
                           time_t endTime, double totalRuntime);

typedef	void (*SuiteStartedFp)(const char *suiteName, time_t eventTime);
typedef	void (*SuiteEndedFp)(int testsPassed, int testsFailed, int testsSkipped,
		                time_t endTime, double totalRuntime);

typedef	void (*TestStartedFp)(const char *testName, const char *suiteName,
                              const char *testDescription, time_t startTime);
typedef	void (*TestEndedFp)(const char *testName, const char *suiteName, int testResult,
                            time_t endTime, double totalRuntime);

/*!
 * Note: for assertResult, non-zero == pass, zero == failure
 *
 */
typedef	void (*AssertFp)(const char *assertName, int assertResult,
						 const char *assertMessage, time_t eventTime);
typedef	void (*AssertSummaryFp)(int numAsserts, int numAssertsFailed,
								int numAssertsPass, time_t eventTime);


typedef	void (*LogFp)(const char *logMessage, time_t eventTime);


extern RunStartedFp RunStarted;
extern RunEndedFp RunEnded;
extern SuiteStartedFp SuiteStarted;
extern SuiteEndedFp SuiteEnded;
extern TestStartedFp TestStarted;
extern TestEndedFp TestEnded;
extern AssertFp Assert;
extern AssertSummaryFp AssertSummary;
extern LogFp Log;

#endif
