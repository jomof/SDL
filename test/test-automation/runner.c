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

#include "SDL/SDL.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#include <sys/types.h>

#include "fuzzer/fuzzer.h"


#include "config.h"

#include "SDL_test.h"

#include "plain_logger.h"
#include "xml_logger.h"
#include "logger.h"
#include "support.h"

//!< Function pointer to a test case function
typedef void (*TestCaseFp)(void *arg);
//!< Function pointer to a test case init function
typedef void (*InitTestInvironmentFp)(void);
//!< Function pointer to a test case quit function
typedef int  (*QuitTestInvironmentFp)(void);
//!< Function pointer to a test case set up function
typedef void (*TestCaseSetUpFp)(void *arg);
//!< Function pointer to a test case tear down function
typedef void  (*TestCaseTearDownFp)(void *arg);
//!< Function pointer to a function which returns the failed assert count
typedef int (*CountFailedAssertsFp)(void);


//!< Flag for executing tests in-process
static int execute_inproc = 0;
//!< Flag for only printing out the test names
static int only_print_tests = 0;
//!< Flag for executing only test with selected name
static int only_selected_test  = 0;
//!< Flag for executing only the selected test suite
static int only_selected_suite = 0;
//!< Flag for executing only tests that contain certain string in their name
static int only_tests_with_string = 0;
//!< Flag for enabling XML logging
static int xml_enabled = 0;
//! Flag for enabling user-supplied style sheet for XML test report
static int custom_xsl_enabled = 0;
//! Flag for disabling xsl-style from xml report
static int xsl_enabled = 0;
//! Flag for enabling universal timeout for tests
static int universal_timeout_enabled = 0;
//! Flag for enabling verbose logging
static int enable_verbose_logger = 0;
//! Flag for using user supplied run seed
static int userRunSeed = 0;


//!< Size of the test and suite name buffers
#define NAME_BUFFER_SIZE 1024
//!< Name of the selected test
char selected_test_name[NAME_BUFFER_SIZE];
//!< Name of the selected suite
char selected_suite_name[NAME_BUFFER_SIZE];

//!< substring of test case name
char testcase_name_substring[NAME_BUFFER_SIZE];

//! Name for user-supplied XSL style sheet name
char xsl_stylesheet_name[NAME_BUFFER_SIZE];
//! User-suppled timeout value for tests
int universal_timeout = -1;

//! Default directory of the test suites
#define DEFAULT_TEST_DIRECTORY "tests/"

//! Fuzzer seed for the harness
char *runSeed = NULL;


//! Variable is used to pass the generated execution key to a test
char *globalExecKey = NULL;

//! Execution key that user supplied via command options
char *userExecKey = NULL;

//! How man time a test will be invocated
int testInvocationCount = 1;

// \todo add comments
int totalTestFailureCount = 0, totalTestPassCount = 0, totalTestSkipCount = 0;
int testFailureCount = 0, testPassCount = 0, testSkipCount = 0;


/*!
 * Holds information about test suite such as it's name
 * and pointer to dynamic library. Implemented as linked list.
 */
typedef struct TestSuiteReference {
	char *name; //!< test suite name
	char *directoryPath; //!< test suites path (eg. tests/libtestsuite)
	void *library; //!< pointer to shared/dynamic library implementing the suite

	struct TestSuiteReference *next; //!< Pointer to next item in the list
} TestSuiteReference;


/*!
 * Holds information about the tests that will be executed.
 *
 * Implemented as linked list.
 */
typedef struct TestCaseItem {
	char *testName;
	char *suiteName;

	char *description;
	long requirements;
	long timeout;

	InitTestInvironmentFp initTestEnvironment;
	TestCaseSetUpFp testSetUp;
	TestCaseFp testCase;
	TestCaseTearDownFp testTearDown;
 	QuitTestInvironmentFp quitTestEnvironment;

 	CountFailedAssertsFp countFailedAsserts;

	struct TestCaseItem *next;
} TestCase;


/*! Some function prototypes. Add the rest of functions and move to runner.h */
TestCaseFp LoadTestCaseFunction(void *suite, char *testName);
InitTestInvironmentFp LoadInitTestInvironmentFunction(void *suite);
QuitTestInvironmentFp LoadQuitTestInvironmentFunction(void *suite);
TestCaseReference **QueryTestCaseReferences(void *library);
TestCaseSetUpFp LoadTestSetUpFunction(void *suite);
TestCaseTearDownFp LoadTestTearDownFunction(void *suite);
CountFailedAssertsFp LoadCountFailedAssertsFunction(void *suite);
void KillHungTestInChildProcess(int signum);


/*! Pointers to selected logger implementation */
RunStartedFp RunStarted = NULL;
RunEndedFp RunEnded = NULL;
SuiteStartedFp SuiteStarted = NULL;
SuiteEndedFp SuiteEnded = NULL;
TestStartedFp TestStarted = NULL;
TestEndedFp TestEnded = NULL;
AssertFp Assert = NULL;
AssertWithValuesFp AssertWithValues = NULL;
AssertSummaryFp AssertSummary = NULL;
LogFp Log = NULL;


/*!
 * Scans the tests/ directory and returns the names
 * of the dynamic libraries implementing the test suites.
 *
 * Note: currently function assumes that test suites names
 * are in following format: libtestsuite.dylib or libtestsuite.so.
 *
 * Note: if only_selected_suite flags is non-zero, only the selected
 * test will be loaded.
 *
 * \param directoryName Name of the directory which will be scanned
 * \param extension What file extension is used with dynamic objects
 *
 * \return Pointer to TestSuiteReference which holds all the info about suites
 */
TestSuiteReference *
ScanForTestSuites(char *directoryName, char *extension)
{
	typedef struct dirent Entry;
	DIR *directory = opendir(directoryName);
	TestSuiteReference *suites = NULL;
	Entry *entry = NULL;

	if(!directory) {
		fprintf(stderr, "Failed to open test suite directory: %s\n", directoryName);
		perror("Error message");
		exit(1);
	}

	while(entry = readdir(directory)) {
		if(strlen(entry->d_name) > 2) { // discards . and ..
			const char *delimiters = ".";
			char *name = strtok(entry->d_name, delimiters);
			char *ext = strtok(NULL, delimiters);

			// filter out all other suites but the selected test suite
			int ok = 1;
			if(only_selected_suite) {
				ok = SDL_strncmp(selected_suite_name, name, NAME_BUFFER_SIZE) == 0;
			}

			if(ok && SDL_strcmp(ext, extension)  == 0) {
				// create test suite reference
				TestSuiteReference *reference = (TestSuiteReference *) SDL_malloc(sizeof(TestSuiteReference));
				if(reference == NULL) {
					fprintf(stderr, "Allocating TestSuiteReference failed\n");
				}

				memset(reference, 0, sizeof(TestSuiteReference));

				const int dirSize = SDL_strlen(directoryName);
				const int extSize = SDL_strlen(ext);
				const int nameSize = SDL_strlen(name) + 1;

				// copy the name
				reference->name = SDL_malloc(nameSize * sizeof(char));
				if(reference->name == NULL) {
					SDL_free(reference);
					return NULL;
				}

				SDL_snprintf(reference->name, nameSize, "%s", name);

				// copy the directory path
				const int dpSize = dirSize + nameSize + 1 + extSize + 1;
				reference->directoryPath = SDL_malloc(dpSize * sizeof(char));
				if(reference->directoryPath == NULL) {
					SDL_free(reference->name);
					SDL_free(reference);
					return NULL;
				}
				SDL_snprintf(reference->directoryPath, dpSize, "%s%s.%s",
						directoryName, name, ext);

				reference->next = suites;
				suites = reference;
			}
		}
	}

	closedir(directory);

	return suites;
}


/*!
 * Loads test suite which is implemented as dynamic library.
 *
 * \param suite Reference to test suite that'll be loaded
 *
 * \return Pointer to loaded test suite, or NULL if library could not be loaded
 */
void *
LoadTestSuite(const TestSuiteReference *suite)
{
	void *library = SDL_LoadObject(suite->directoryPath);
	if(library == NULL) {
		fprintf(stderr, "Loading %s failed\n", suite->name);
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return library;
}


/*!
 * Goes through all the given TestSuiteReferences
 * and loads the dynamic libraries. Updates the suites
 * parameter on-the-fly and returns it.
 *
 * \param suites Suites that will be loaded
 *
 * \return Updated TestSuiteReferences with pointer to loaded libraries
 */
TestSuiteReference *
LoadTestSuites(TestSuiteReference *suites)
{
	TestSuiteReference *reference = NULL;
	for(reference = suites; reference; reference = reference->next) {
		reference->library = LoadTestSuite(reference);
	}

	return suites;
}


/*!
 * Unloads the given TestSuiteReferences. Frees all
 * the allocated resources including the dynamic libraries.
 *
 * \param suites TestSuiteReferences for deallocation process
 */
void
UnloadTestSuites(TestSuiteReference *suites)
{
	TestSuiteReference *ref = suites;
	while(ref) {
		SDL_free(ref->name);
		SDL_free(ref->directoryPath);
		SDL_UnloadObject(ref->library);

		TestSuiteReference *temp = ref->next;
		SDL_free(ref);
		ref = temp;
	}

	suites = NULL;
}


/*!
 * Goes through the previously loaded test suites and
 * loads test cases from them. Test cases are filtered
 * during the process. Function will only return the
 * test cases which aren't filtered out.
 *
 * \param suites previously loaded test suites
 *
 * \return Test cases that survived filtering process.
 */
TestCase *
LoadTestCases(TestSuiteReference *suites)
{
	TestCase *testCases = NULL;

	TestSuiteReference *suiteReference = NULL;
	for(suiteReference = suites; suiteReference; suiteReference = suiteReference->next) {
		TestCaseReference **tests = QueryTestCaseReferences(suiteReference->library);

		TestCaseReference *testReference = NULL;
		int counter = 0;
		for(testReference = tests[counter]; testReference; testReference = tests[++counter]) {

			void *suite = suiteReference->library;

			// Load test case functions
			InitTestInvironmentFp initTestEnvironment = LoadInitTestInvironmentFunction(suiteReference->library);
			QuitTestInvironmentFp quitTestEnvironment = LoadQuitTestInvironmentFunction(suiteReference->library);

			TestCaseSetUpFp testSetUp = LoadTestSetUpFunction(suiteReference->library);
			TestCaseTearDownFp testTearDown = LoadTestTearDownFunction(suiteReference->library);

			TestCaseFp testCase = LoadTestCaseFunction(suiteReference->library, testReference->name);

			CountFailedAssertsFp countFailedAsserts = LoadCountFailedAssertsFunction(suiteReference->library);

			// Do the filtering
			if(FilterTestCase(testReference)) {
				TestCase *item = SDL_malloc(sizeof(TestCase));
				memset(item, 0, sizeof(TestCase));

				item->initTestEnvironment = initTestEnvironment;
				item->quitTestEnvironment = quitTestEnvironment;

				item->testSetUp = testSetUp;
				item->testTearDown = testTearDown;

				item->testCase = testCase;

				item->countFailedAsserts = countFailedAsserts;

				// copy suite name
				int length = SDL_strlen(suiteReference->name) + 1;
				item->suiteName = SDL_malloc(length);
				strncpy(item->suiteName, suiteReference->name, length);

				// copy test name
				length = SDL_strlen(testReference->name) + 1;
				item->testName = SDL_malloc(length);
				strncpy(item->testName, testReference->name, length);

				// copy test description
				length = SDL_strlen(testReference->description) + 1;
				item->description = SDL_malloc(length);
				strncpy(item->description, testReference->description, length);

				item->requirements = testReference->requirements;
				item->timeout = testReference->timeout;

				// prepend the list
				item->next = testCases;
				testCases = item;

				//printf("Added test: %s\n", testReference->name);
			}
		}
	}

	return testCases;
}


/*!
 * Unloads the given TestCases. Frees all the resources
 * allocated for test cases.
 *
 * \param testCases Test cases to be deallocated
 */
void
UnloadTestCases(TestCase *testCases)
{
	TestCase *ref = testCases;
	while(ref) {
		SDL_free(ref->testName);
		SDL_free(ref->suiteName);
		SDL_free(ref->description);

		TestCase *temp = ref->next;
		SDL_free(ref);
		ref = temp;
	}

	testCases = NULL;
}


/*!
 * Filters a test case based on its properties in TestCaseReference and user
 * preference.
 *
 * \return Non-zero means test will be added to execution list, zero means opposite
 */
int
FilterTestCase(TestCaseReference *testReference)
{
	int retVal = 1;

	if(testReference->enabled == TEST_DISABLED) {
		retVal = 0;
	}

	if(only_selected_test) {
		if(SDL_strncmp(testReference->name, selected_test_name, NAME_BUFFER_SIZE) == 0) {
			retVal = 1;
		} else {
			retVal = 0;
		}
	}

	if(only_tests_with_string) {
		if(strstr(testReference->name, testcase_name_substring) != NULL) {
			retVal = 1;
		} else {
			retVal = 0;
		}
	}

	return retVal;
}


/*!
 * Loads the test case references from the given test suite.

 * \param library Previously loaded dynamic library AKA test suite
 * \return Pointer to array of TestCaseReferences or NULL if function failed
 */
TestCaseReference **
QueryTestCaseReferences(void *library)
{
	TestCaseReference **(*suite)(void);

	suite = (TestCaseReference **(*)(void)) SDL_LoadFunction(library, "QueryTestSuite");
	if(suite == NULL) {
		fprintf(stderr, "Loading QueryTestCaseReferences() failed.\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	TestCaseReference **tests = suite();
	if(tests == NULL) {
		fprintf(stderr, "Failed to load test references.\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return tests;
}


/*!
 * Loads test case from a test suite
 *
 * \param suite a test suite
 * \param testName Name of the test that is going to be loaded
 *
 * \return Function Pointer (TestCase) to loaded test case, NULL if function failed
 */
TestCaseFp
LoadTestCaseFunction(void *suite, char *testName)
{
	TestCaseFp test = (TestCaseFp) SDL_LoadFunction(suite, testName);
	if(test == NULL) {
		fprintf(stderr, "Loading test failed, tests == NULL\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return test;
}


/*!
 * Loads function that sets up a fixture for a test case. Note: if there's
 * no SetUp function present in the suite the function will return NULL.
 *
 * \param suite Used test suite
 *
 * \return Function pointer to test case's set up function
 */
TestCaseSetUpFp
LoadTestSetUpFunction(void *suite) {
	return (TestCaseSetUpFp) SDL_LoadFunction(suite, "SetUp");
}


/*!
 * Loads function that tears down a fixture for a test case. Note: if there's
 * no TearDown function present in the suite the function will return NULL.
 *
 * \param suite Used test suite
 *
 * \return Function pointer to test case's tear down function
 */
TestCaseTearDownFp
LoadTestTearDownFunction(void *suite) {
	return (TestCaseTearDownFp) SDL_LoadFunction(suite, "TearDown");
}


/*!
 * Loads function that initialises the test environment for
 * a test case in the given suite.
 *
 * \param suite Used test suite
 *
 * \return Function pointer (InitTestInvironmentFp) which points to loaded init function. NULL if function fails.
 */
InitTestInvironmentFp
LoadInitTestInvironmentFunction(void *suite) {
	InitTestInvironmentFp testEnvInit = (InitTestInvironmentFp) SDL_LoadFunction(suite, "_InitTestEnvironment");
	if(testEnvInit == NULL) {
		fprintf(stderr, "Loading _InitTestInvironment function failed, testEnvInit == NULL\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return testEnvInit;
}


/*!
 * Loads function that deinitialises the test environment (and returns
 * the test case's result) created for the test case in the given suite.
 *
 * \param suite Used test suite
 *
 * \return Function pointer (QuitTestInvironmentFp) which points to loaded init function. NULL if function fails.
 */
QuitTestInvironmentFp
LoadQuitTestInvironmentFunction(void *suite) {
	QuitTestInvironmentFp testEnvQuit = (QuitTestInvironmentFp) SDL_LoadFunction(suite, "_QuitTestEnvironment");
	if(testEnvQuit == NULL) {
		fprintf(stderr, "Loading _QuitTestEnvironment function failed, testEnvQuit == NULL\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return testEnvQuit;
}


/*!
 * Loads function that returns failed assert count in the current
 * test environment
 *
 * \param suite Used test suite
 *
 * \return Function pointer to _CountFailedAsserts function
 */
CountFailedAssertsFp
LoadCountFailedAssertsFunction(void *suite) {
	CountFailedAssertsFp countFailedAssert = (CountFailedAssertsFp) SDL_LoadFunction(suite, "_CountFailedAsserts");
	if(countFailedAssert == NULL) {
		fprintf(stderr, "Loading _CountFailedAsserts function failed, countFailedAssert == NULL\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return countFailedAssert;
}


/*!
 * Set timeout for test.
 *
 * \param timeout Timeout interval in seconds!
 * \param callback Function that will be called after timeout has elapsed
 */
void
SetTestTimeout(int timeout, void (*callback)(int))
{
	if(callback == NULL) {
		fprintf(stderr, "Error: timeout callback can't be NULL");
	}

	if(timeout < 0) {
		fprintf(stderr, "Error: timeout value must be bigger than zero.");
	}

	int tm = (timeout > universal_timeout ? timeout : universal_timeout);

#if 1
	/* Init SDL timer if not initialized before */
	if(SDL_WasInit(SDL_INIT_TIMER) == 0) {
		if(SDL_InitSubSystem(SDL_INIT_TIMER)) {
			fprintf(stderr, "Error: Failed to init timer subsystem");
			fprintf(stderr, "%s\n", SDL_GetError());
		}
	}

	/* Note:
	 * SDL_Init(SDL_INIT_TIMER) should be successfully called before using this
	 */
	int timeoutInMilliseconds = tm * 1000;

	SDL_TimerID timerID = SDL_AddTimer(timeoutInMilliseconds, callback, 0x0);
	if(timerID == NULL) {
		fprintf(stderr, "Error: Creation of SDL timer failed.\n");
		fprintf(stderr, "Error: %s\n", SDL_GetError());
	}
#else
	signal(SIGALRM, callback);
	alarm((unsigned int) tm);
#endif
}


/*!
 * Kills test that hungs. Test hungs when its execution
 * takes longer than timeout specified for it.
 *
 * When test will be killed SIG_ALRM will be triggered and
 * it'll call this function which kills the test process.
 *
 * Note: if runner is executed with --in-proc then hung tests
 * can't be killed
 *
 * \param signum
 */
void
KillHungTestInChildProcess(int signum)
{
	exit(TEST_RESULT_KILLED);
}

/*!
 * Checks if given test case can be executed on the current platform.
 *
 * \param testCase Test to be checked
 * \returns 1 if test is runnable, otherwise 0. On error returns -1
 */
int
CheckTestRequirements(TestCase *testCase)
{
	int retVal = 1;

	if(testCase == NULL) {
		fprintf(stderr, "TestCase parameter can't be NULL");
		return -1;
	}

	if(testCase->requirements & TEST_REQUIRES_AUDIO) {
		retVal = PlatformSupportsAudio();
	}

	return retVal;
}


/*
 * Execute a test. Loads the test, executes it and
 * returns the tests return value to the caller.
 *
 * \param testItem Test to be executed
 * \param test result
 */
int
RunTest(TestCase *testCase, char *execKey)
{
	if(!testCase) {
		return -1;
	}

	int runnable = CheckTestRequirements(testCase);
	if(runnable != 1) {
		return TEST_RESULT_SKIPPED;
	}

	if(testCase->timeout > 0 || universal_timeout > 0) {
		if(execute_inproc) {
			Log(time(0), "Test asked for timeout which is not supported.");
		}
		else {
			SetTestTimeout(testCase->timeout, KillHungTestInChildProcess);
		}
	}

	testCase->initTestEnvironment();

	if(testCase->testSetUp) {
		testCase->testSetUp(0x0);
	}

	int cntFailedAsserts = testCase->countFailedAsserts();
	if(cntFailedAsserts != 0) {
		return TEST_RESULT_SETUP_FAILURE;
	}

	testCase->testCase(0x0);

	if(testCase->testTearDown) {
		testCase->testTearDown(0x0);
	}

	return testCase->quitTestEnvironment();
}


/*!
 * Sets up a test case. Decideds wheter the test will
 * be executed in-proc or out-of-proc.
 *
 * \param testItem The test case that will be executed
 * \return The return value of the test. Zero means success, non-zero failure.
 */
int
ExecuteTest(TestCase *testItem, char *execKey) {
	int retVal = -1;

	if(execute_inproc) {
		retVal = RunTest(testItem, execKey);
	} else {
		int childpid = fork();
		if(childpid == 0) {
			exit(RunTest(testItem, execKey));
		} else {
			int stat_lock = -1;
			int child = wait(&stat_lock);

			retVal = HandleChildProcessReturnValue(stat_lock);
		}
	}

	if(retVal == TEST_RESULT_SKIPPED) {
		testSkipCount++;
		totalTestSkipCount++;
	}
	else if(retVal) {
		totalTestFailureCount++;
		testFailureCount++;
	}
	else {
		totalTestPassCount++;
		testPassCount++;
	}

	// return the value for logger
	return retVal;
}


/*!
 * If using out-of-proc execution of tests. This function
 * will handle the return value of the child process
 * and interprets it to the runner. Also prints warnings
 * if child was aborted by a signela.
 *
 * \param stat_lock information about the exited child process
 *
 * \return 0 if test case succeeded, 1 otherwise
 */
int
HandleChildProcessReturnValue(int stat_lock)
{
	int returnValue = -1;

	if(WIFEXITED(stat_lock)) {
		returnValue = WEXITSTATUS(stat_lock);
	} else if(WIFSIGNALED(stat_lock)) {
		int signal = WTERMSIG(stat_lock);
		// \todo add this to logger (add signal number)
		Log(time(0), "FAILURE: test was aborted due to  %d\n", signal);
		returnValue = 1;
	}

	return returnValue;
}


/*!
 * Generates a random run seed for the harness.
 *
 * \param length The length of the generated seed
 *
 * \returns The generated seed
 */
char *
GenerateRunSeed(const int length)
{
	if(length <= 0) {
		fprintf(stderr, "Error: lenght of harness seed can't be less than zero\n");
		return NULL;
	}

	char *seed = SDL_malloc(length * sizeof(8));
	if(seed == NULL) {
		fprintf(stderr, "Error: malloc for run seed failed\n");
		return NULL;
	}

	RND_CTX randomContext;

	utl_randomInitTime(&randomContext);

	int counter = 0;
	for( ; counter < length; ++counter) {
		int number = abs(utl_random(&randomContext));
		seed[counter] = (char) (number % (127-34)) + 34;
	}

	return seed;
}

/*!
 * Sets up the logger.
 *
 * \return Logger data structure (that needs be deallocated)
 */
void *
SetUpLogger()
{
	LoggerData *loggerData = SDL_malloc(sizeof(loggerData));
	if(loggerData == NULL) {
		fprintf(stderr, "Error: Logger data structure not allocated.");
		return NULL;
	}

	loggerData->level = (enable_verbose_logger ? VERBOSE : STANDARD);

	if(xml_enabled) {
		RunStarted = XMLRunStarted;
		RunEnded = XMLRunEnded;

		SuiteStarted = XMLSuiteStarted;
		SuiteEnded = XMLSuiteEnded;

		TestStarted = XMLTestStarted;
		TestEnded = XMLTestEnded;

		Assert = XMLAssert;
		AssertWithValues = XMLAssertWithValues;
		AssertSummary = XMLAssertSummary;

		Log = XMLLog;

		char *sheet = NULL;
		if(xsl_enabled) {
			sheet = "style.xsl"; // default style sheet;
		}

		if(custom_xsl_enabled) {
			sheet = xsl_stylesheet_name;
		}

		loggerData->custom = sheet;
	} else {
		RunStarted = PlainRunStarted;
		RunEnded = PlainRunEnded;

		SuiteStarted = PlainSuiteStarted;
		SuiteEnded = PlainSuiteEnded;

		TestStarted = PlainTestStarted;
		TestEnded = PlainTestEnded;

		Assert = PlainAssert;
		AssertWithValues = PlainAssertWithValues;
		AssertSummary = PlainAssertSummary;

		Log = PlainLog;
	}

	return loggerData;
}


/*!
 * Prints usage information
 */
void
PrintUsage() {
	  printf("Usage: ./runner [--in-proc] [--show-tests] [--verbose] [--xml]\n");
	  printf("                [--xsl [STYLESHEET]] [--seed VALUE] [--iterations VALUE]\n");
	  printf("                [--exec-key KEY] [--timeout VALUE] [--test TEST]\n");
	  printf("                [--name-contains SUBSTR] [--suite SUITE]\n");
	  printf("                [--version] [--help]\n");
	  printf("Options:\n");
	  printf("     --in-proc                Executes tests in-process\n");
	  printf("     --show-tests             Prints out all the executable tests\n");
	  printf(" -v  --verbose                Enables verbose logging\n");
	  printf("     --xml                    Enables XML logger\n");
	  printf("     --xsl [STYLESHEET]       Adds XSL stylesheet to the XML test reports for\n");
	  printf("                              browser viewing. Optionally uses the specified XSL\n");
	  printf("                              file or URL instead of the default one\n");
	  printf("     --seed VALUE             Specify fuzzing seed for the harness\n");
	  printf("     --iterations VALUE       Specify how many times a test will be executed\n");
	  printf("     --exec-key KEY           Run test(s) with specific execution key\n");
	  printf(" -tm --timeout VALUE          Specify common timeout value for all tests\n");
	  printf("                              Timeout is given in seconds and it'll override\n");
	  printf("                              test specific timeout value only if the given\n");
	  printf("                              value is greater than the test specific value\n");
	  printf("                              note: doesn't work with --in-proc option.\n");
	  printf(" -t  --test TEST              Executes only tests with given name\n");
	  printf(" -ts --name-contains SUBSTR   Executes only tests that have given\n");
	  printf("                              substring in test name\n");
	  printf(" -s  --suite SUITE            Executes only the given test suite\n");

	  printf("     --version                Print version information\n");
	  printf(" -h  --help                   Print this help\n");
}


/*!
 * Parse command line arguments
 *
 * \param argc Count of command line arguments
 * \param argv Array of commond lines arguments
 */
void
ParseOptions(int argc, char *argv[])
{
   int i;

   for (i = 1; i < argc; ++i) {
      const char *arg = argv[i];
      if(SDL_strcmp(arg, "--in-proc") == 0) {
         execute_inproc = 1;
      }
      else if(SDL_strcmp(arg, "--show-tests") == 0) {
    	  only_print_tests = 1;
      }
      else if(SDL_strcmp(arg, "--xml") == 0) {
    	  xml_enabled = 1;
      }
      else if(SDL_strcmp(arg, "--verbose") == 0 || SDL_strcmp(arg, "-v") == 0) {
    	  enable_verbose_logger = 1;
      }
      else if(SDL_strcmp(arg, "--timeout") == 0 || SDL_strcmp(arg, "-tm") == 0) {
    	  universal_timeout_enabled = 1;
    	  char *timeoutString = NULL;

    	  if( (i + 1) < argc)  {
    		  timeoutString = argv[++i];
    	  }  else {
    		  printf("runner: timeout is missing\n");
    		  PrintUsage();
    		  exit(1);
    	  }

    	  universal_timeout = atoi(timeoutString);
      }
      else if(SDL_strcmp(arg, "--seed") == 0) {
    	  userRunSeed = 1;

    	  if( (i + 1) < argc)  {
    		  runSeed = argv[++i];
    	  }  else {
    		  printf("runner: seed value is missing\n");
    		  PrintUsage();
    		  exit(1);
    	  }
    	  //!�\todo should the seed be copied to a buffer?
      }
      else if(SDL_strcmp(arg, "--iterations") == 0) {
    	  char *iterationsString = NULL;
    	  if( (i + 1) < argc)  {
    		  iterationsString = argv[++i];
    	  }  else {
    		  printf("runner: iterations value is missing\n");
    		  PrintUsage();
    		  exit(1);
    	  }

    	  testInvocationCount = atoi(iterationsString);
    	  if(testInvocationCount < 1) {
    		  printf("Iteration value has to bigger than 0.\n");
    		  exit(1);
    	  }
      }
      else if(SDL_strcmp(arg, "--exec-key") == 0) {
    	  char *execKeyString = NULL;
    	  if( (i + 1) < argc)  {
    		  execKeyString = argv[++i];
    	  }  else {
    		  printf("runner: execkey value is missing\n");
    		  PrintUsage();
    		  exit(1);
    	  }

    	  userExecKey = execKeyString;
      }
      else if(SDL_strcmp(arg, "--test") == 0 || SDL_strcmp(arg, "-t") == 0) {
    	  only_selected_test = 1;
    	  char *testName = NULL;

    	  if( (i + 1) < argc)  {
    		  testName = argv[++i];
    	  }  else {
    		  printf("runner: test name is missing\n");
    		  PrintUsage();
    		  exit(1);
    	  }

    	  memset(selected_test_name, 0, NAME_BUFFER_SIZE);
    	  strcpy(selected_test_name, testName);
      }
      else if(SDL_strcmp(arg, "--xsl") == 0) {
    	  xsl_enabled = 1;

    	  if( (i + 1) < argc)  {
    		  char *stylesheet = argv[++i];
    		  if(stylesheet[0] != '-') {
    	    	  custom_xsl_enabled = 1;

    	    	  memset(xsl_stylesheet_name, 0, NAME_BUFFER_SIZE);
    	    	  strncpy(xsl_stylesheet_name, stylesheet, NAME_BUFFER_SIZE);
    		  }
    	  }
      }
      else if(SDL_strcmp(arg, "--name-contains") == 0 || SDL_strcmp(arg, "-ts") == 0) {
    	  only_tests_with_string = 1;
    	  char *substring = NULL;

    	  if( (i + 1) < argc)  {
    		  substring = argv[++i];
    	  }  else {
    		  printf("runner: substring of test name is missing\n");
    		  PrintUsage();
    		  exit(1);
    	  }

    	  memset(testcase_name_substring, 0, NAME_BUFFER_SIZE);
    	  strcpy(testcase_name_substring, substring);
      }
      else if(SDL_strcmp(arg, "--suite") == 0 || SDL_strcmp(arg, "-s") == 0) {
    	  only_selected_suite = 1;

    	  char *suiteName = NULL;
    	  if( (i + 1) < argc)  {
    		  suiteName = argv[++i];
    	  }  else {
    		  printf("runner: suite name is missing\n");
    		  PrintUsage();
    		  exit(1);
    	  }

    	  memset(selected_suite_name, 0, NAME_BUFFER_SIZE);
    	  strcpy(selected_suite_name, suiteName);
      }
      else if(SDL_strcmp(arg, "--version") == 0) {
    	  fprintf(stdout, "SDL test harness (version %s)\n", PACKAGE_VERSION);
    	  exit(0);
      }
      else if(SDL_strcmp(arg, "--help") == 0 || SDL_strcmp(arg, "-h") == 0) {
    	  PrintUsage();
    	  exit(0);
      }
      else {
    	  printf("runner: unknown command '%s'\n", arg);
    	  PrintUsage();
    	  exit(0);
      }
   }
}


/*!
 * Entry point for test runner
 *
 * \param argc Count of command line arguments
 * \param argv Array of commond lines arguments
 */
int
main(int argc, char *argv[])
{
	ParseOptions(argc, argv);

	// print: Testing against SDL version fuu (rev: bar) if verbose == true

	char *testSuiteName = NULL;
	int suiteCounter = 0;

#if defined(linux) || defined( __linux)
	char *extension = "so";
#else
	char *extension = "dylib";
#endif

	if(userRunSeed == 0) {
		runSeed = GenerateRunSeed(16);
		if(runSeed == NULL) {
			fprintf(stderr, "Error: Generating harness seed failed\n");
			return 1;
		}
	}

	LoggerData *loggerData = SetUpLogger();

	const Uint32 startTicks = SDL_GetTicks();

	TestSuiteReference *suites = ScanForTestSuites(DEFAULT_TEST_DIRECTORY, extension);
	suites = LoadTestSuites(suites);

	TestCase *testCases = LoadTestCases(suites);

	// if --show-tests option is given, only print tests and exit
	if(only_print_tests) {
		TestCase *testItem = NULL;
		for(testItem = testCases; testItem; testItem = testItem->next) {
			printf("%s (in %s)\n", testItem->testName, testItem->suiteName);
		}

		return 0;
	}

	RunStarted(argc, argv, runSeed, time(0), loggerData);

	// logger data is no longer used
	SDL_free(loggerData);

	if(execute_inproc && universal_timeout_enabled) {
		Log(time(0), "Test timeout is not supported with in-proc execution.");
		Log(time(0), "Timeout will be disabled...");

		universal_timeout_enabled = 0;
		universal_timeout = -1;
	}

	char *currentSuiteName = NULL;
	int suiteStartTime = SDL_GetTicks();

	int notFirstSuite = 0;
	int startNewSuite = 1;
	TestCase *testItem = NULL;
	for(testItem = testCases; testItem; testItem = testItem->next) {
		if(currentSuiteName && strncmp(currentSuiteName, testItem->suiteName, NAME_BUFFER_SIZE) != 0) {
			startNewSuite = 1;
		}

		if(startNewSuite) {
			if(notFirstSuite) {
				const double suiteRuntime = (SDL_GetTicks() - suiteStartTime) / 1000.0f;

				SuiteEnded(testPassCount, testFailureCount, testSkipCount, time(0),
							suiteRuntime);
			}

			suiteStartTime = SDL_GetTicks();

			currentSuiteName = testItem->suiteName;
			SuiteStarted(currentSuiteName, time(0));
			testFailureCount = testPassCount = testSkipCount = 0;
			suiteCounter++;

			startNewSuite = 0;
			notFirstSuite = 1;
		}

		int currentIteration = testInvocationCount;
		while(currentIteration > 0) {
			if(userExecKey != NULL) {
				globalExecKey = userExecKey;
			} else {
				char *execKey = GenerateExecKey(runSeed, testItem->suiteName,
											  testItem->testName, currentIteration);
				globalExecKey = execKey;
			}

			TestStarted(testItem->testName, testItem->suiteName,
						testItem->description, globalExecKey, time(0));

			const Uint32 testTimeStart = SDL_GetTicks();

			int retVal = ExecuteTest(testItem, globalExecKey);

			const double testTotalRuntime = (SDL_GetTicks() - testTimeStart) / 1000.0f;

			TestEnded(testItem->testName, testItem->suiteName, retVal, time(0), testTotalRuntime);

			currentIteration--;

			SDL_free(globalExecKey);
			globalExecKey = NULL;
		}
	}

	if(currentSuiteName) {
		SuiteEnded(testPassCount, testFailureCount, testSkipCount, time(0),
					(SDL_GetTicks() - suiteStartTime) / 1000.0f);
	}

	UnloadTestCases(testCases);
	UnloadTestSuites(suites);

	const Uint32 endTicks = SDL_GetTicks();
	const double totalRunTime = (endTicks - startTicks) / 1000.0f;

	RunEnded(totalTestPassCount + totalTestFailureCount + totalTestSkipCount, suiteCounter,
			 totalTestPassCount, totalTestFailureCount, totalTestSkipCount, time(0), totalRunTime);

	// Some SDL subsystem might be init'ed so shut them down
	SDL_Quit();

	return (totalTestFailureCount ? 1 : 0);
}
