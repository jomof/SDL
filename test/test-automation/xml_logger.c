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

#ifndef _XML_LOGGER_C
#define _XML_LOGGER_C

#include "xml.h"
#include "logger.h"

#include "xml_logger.h"

void
XMLRunStarted(LogOutputFp outputFn, const char *runnerParameters, time_t eventTime)
{
	XMLOpenDocument("testlog", outputFn);

	XMLOpenElement("parameters");
	XMLAddContent(runnerParameters);
	XMLCloseElement("parameters");
}

void
XMLRunEnded(time_t endTime, time_t totalRuntime)
{
	XMLCloseDocument("testlog");
}

void
XMLSuiteStarted(const char *suiteName, time_t eventTime)
{
	XMLOpenElement("suite");

	XMLOpenElement("eventTime");
	//XMLAddContent(evenTime);
	XMLCloseElement("eventTime");
}

void
XMLSuiteEnded(int testsPassed, int testsFailed, int testsSkipped,
           double endTime, time_t totalRuntime)
{
	XMLCloseElement("suite");
}

void
XMLTestStarted(const char *testName, const char *testDescription, time_t startTime)
{
	XMLOpenElement("test");

	XMLOpenElement("name");
	XMLAddContent(testName);
	XMLCloseElement("name");

	XMLOpenElement("description");
	XMLAddContent(testDescription);
	XMLCloseElement("description");

	XMLOpenElement("starttime");
	//XMLAddContent(startTime);
	XMLCloseElement("starttime");
}

void
XMLTestEnded(const char *testName, const char *testDescription,
          int testResult, int numAsserts, time_t endTime, time_t totalRuntime)
{
	XMLCloseElement("test");
}

void
XMLAssert(const char *assertName, int assertResult, const char *assertMessage,
       time_t eventTime)
{
	XMLOpenElement("assert");

	XMLOpenElement("result");
	XMLAddContent((assertResult) ? "pass" : "failure");
	XMLOpenElement("result");


	XMLCloseElement("assert");
}

void
XMLLog(const char *logMessage, time_t eventTime)
{
	XMLOpenElement("log");

	XMLAddContent(logMessage);

	XMLCloseElement("log");
}

#endif
