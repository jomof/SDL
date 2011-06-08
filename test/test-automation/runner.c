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

#include "SDL_test.h"

//!< Function pointer to a test case function
typedef void (*TestCase)(void *arg);
//!< Function pointer to a test case init function
typedef void (*TestCaseInit)(void);
//!< Function pointer to a test case quit function
typedef int  (*TestCaseQuit)(void);

//!< Flag for executing tests in-process
static int execute_inproc = 0;

//!< Flag for executing only test with selected name
static int only_selected_test  = 0;
//!< Flag for executing only the selected test suite
static int only_selected_suite = 0;

//!< Size of the test and suite name buffers
#define NAME_BUFFER_SIZE 1024
//!< Name of the selected test
char selected_test_name[NAME_BUFFER_SIZE];
//!< Name of the selected suite
char selected_suite_name[NAME_BUFFER_SIZE];


//! Default directory of the test suites
#define DEFAULT_TEST_DIRECTORY "tests/"

/*!
 * Holds information about test suite. Implemented as
 * linked list. \todo write better doc
 */
typedef struct TestSuiteReference {
	char *name; //<! test suite name
	struct TestSuiteReference *next; //!< Pointer to next item in the list
} TestSuiteReference;


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
 *
 * \return Pointer to TestSuiteReference which holds all the info about suites
 */
TestSuiteReference *
ScanForTestSuites(char *directoryName, char *extension) {
	typedef struct dirent Entry;
	DIR *directory = opendir(directoryName);

	TestSuiteReference *suites = NULL;

	Entry *entry = NULL;
	if(directory) {
		while(entry = readdir(directory)) {
			if(entry->d_namlen > 2) { // discards . and ..
				char buffer[NAME_BUFFER_SIZE];
				memset(buffer, 0, NAME_BUFFER_SIZE);

				char *name = strtok(entry->d_name, ".");
				char *ext = strtok(NULL, ".");

				// filter out all other suites but the selected test suite
				int ok = 1;
				if(only_selected_suite) {
					ok = SDL_strncmp(selected_suite_name, name, NAME_BUFFER_SIZE) == 0;
				}

				if(ok && SDL_strcmp(ext, extension)  == 0) {
					strcat(buffer, directoryName);
					strcat(buffer, name);
					strcat(buffer, ".");
					strcat(buffer, ext);

					// create tes suite reference
					TestSuiteReference *reference = (TestSuiteReference *) SDL_malloc(sizeof(TestSuiteReference));
					memset(reference, 0, sizeof(TestSuiteReference));

					int length = strlen(buffer) + 1; // + 1 for '\0'?
					reference->name = SDL_malloc(length * sizeof(char));

					strcpy(reference->name, buffer);

					reference->next = suites;

					suites = reference;

					printf("Reference added to: %s\n", buffer);
				}
			}
		}

		closedir(directory);
	} else {
		perror("Couldn't open directory: tests/");
	}

	return suites;
}


/*!
 * Loads test suite which is implemented as dynamic library.
 *
 * \param testSuiteName Name of the test suite which will be loaded
 *
 * \return Pointer to loaded test suite, or NULL if library could not be loaded
 */
void *
LoadTestSuite(char *testSuiteName)
{
	void *library = SDL_LoadObject(testSuiteName);
	if(library == NULL) {
		fprintf(stderr, "Loading %s failed\n", testSuiteName);
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return library;
}


/*!
 * Loads the test case references from the given test suite.

 * \param library Previously loaded dynamic library AKA test suite
 * \return Pointer to array of TestCaseReferences or NULL if function failed
 */
TestCaseReference **
QueryTestCases(void *library)
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
TestCase
LoadTestCase(void *suite, char *testName)
{
	TestCase test = (TestCase) SDL_LoadFunction(suite, testName);
	if(test == NULL) {
		fprintf(stderr, "Loading test failed, tests == NULL\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return test;
}


/*!
 * Loads function that initialises the test case from the
 * given test suite.
 *
 * \param suite Used test suite
 *
 * \return Function pointer (TestCaseInit) which points to loaded init function. NULL if function fails.
 */
TestCaseInit
LoadTestCaseInit(void *suite) {
	TestCaseInit testCaseInit = (TestCaseInit) SDL_LoadFunction(suite, "_TestCaseInit");
	if(testCaseInit == NULL) {
		fprintf(stderr, "Loading TestCaseInit function failed, testCaseInit == NULL\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return testCaseInit;
}


/*!
 * Loads function that deinitialises the executed test case from the
 * given test suite.
 *
 * \param suite Used test suite
 *
 * \return Function pointer (TestCaseInit) which points to loaded init function. NULL if function fails.
 */
TestCaseQuit
LoadTestCaseQuit(void *suite) {
	TestCaseQuit testCaseQuit = (TestCaseQuit) SDL_LoadFunction(suite, "_TestCaseQuit");
	if(testCaseQuit == NULL) {
		fprintf(stderr, "Loading TestCaseQuit function failed, testCaseQuit == NULL\n");
		fprintf(stderr, "%s\n", SDL_GetError());
	}

	return testCaseQuit;
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
HandleTestReturnValue(int stat_lock)
{
	//! \todo rename to: HandleChildProcessReturnValue?
	int returnValue = -1;

	if(WIFEXITED(stat_lock)) {
		returnValue = WEXITSTATUS(stat_lock);
	} else if(WIFSIGNALED(stat_lock)) {
		int signal = WTERMSIG(stat_lock);
		fprintf(stderr, "FAILURE: test was aborted due to signal no %d\n", signal);
		returnValue = 1;
	}

	return returnValue;
}


/*!
 * Executes a test case. Loads the test, executes it and
 * returns the tests return value to the caller.
 *
 * \param suite The suite from which the test will be loaded
 * \param testReference TestCaseReference of the test under execution
 * \return The return value of the test. Zero means success, non-zero failure.
 */
int
ExecuteTest(void *suite, TestCaseReference *testReference) {
	TestCaseInit testCaseInit = LoadTestCaseInit(suite);
	TestCaseQuit testCaseQuit = LoadTestCaseQuit(suite);
	TestCase test = (TestCase) LoadTestCase(suite, testReference->name);

	int retVal = 1;
	if(execute_inproc) {
		testCaseInit();

		test(0x0);

		retVal = testCaseQuit();
	} else {
		int childpid = fork();
		if(childpid == 0) {
			testCaseInit();

			test(0x0);

			exit(testCaseQuit());
		} else {
			int stat_lock = -1;
			int child = wait(&stat_lock);

			retVal = HandleTestReturnValue(stat_lock);
		}
	}

	return retVal;
}


/*!
 * Prints usage information
 */
void
printUsage() {
	  printf("Usage: ./runner [--in-proc] [--suite SUITE] [--test TEST] [--help]\n");
	  printf("Options:\n");
	  printf("    --in-proc        Executes tests in-process\n");
	  printf(" -t --test TEST      Executes only tests with given name\n");
	  printf(" -s --suite SUITE    Executes only the given test suite\n");

	  printf(" -h --help           Print this help\n");
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
      else if(SDL_strcmp(arg, "--help") == 0 || SDL_strcmp(arg, "-h") == 0) {
    	  printUsage();
    	  exit(0);
      }
      else if(SDL_strcmp(arg, "--test") == 0 || SDL_strcmp(arg, "-t") == 0) {
    	  only_selected_test = 1;
    	  char *testName = NULL;

    	  if( (i + 1) < argc)  {
    		  testName = argv[++i];
    	  }  else {
    		  printf("runner: test name is missing\n");
    		  printUsage();
    		  exit(1);
    	  }

    	  memset(selected_test_name, 0, NAME_BUFFER_SIZE);
    	  strcpy(selected_test_name, testName);
      }
      else if(SDL_strcmp(arg, "--suite") == 0 || SDL_strcmp(arg, "-s") == 0) {
    	  only_selected_suite = 1;

    	  char *suiteName = NULL;
    	  if( (i + 1) < argc)  {
    		  suiteName = argv[++i];
    	  }  else {
    		  printf("runner: suite name is missing\n");
    		  printUsage();
    		  exit(1);
    	  }

    	  memset(selected_suite_name, 0, NAME_BUFFER_SIZE);
    	  strcpy(selected_suite_name, suiteName);
      }
      else {
    	  printf("runner: unknown command '%s'\n", arg);
    	  printUsage();
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

	int failureCount = 0, passCount = 0;
	char *testSuiteName = NULL;
	int suiteCounter = 0;

#if defined(linux) || defined( __linux)
	char *extension = "so";
#else
	char *extension = "dylib";
#endif

	const Uint32 startTicks = SDL_GetTicks();
	TestSuiteReference *suites = ScanForTestSuites(DEFAULT_TEST_DIRECTORY, extension);

	// load the suites
	// load tests and filter them
	// end result: list of tests to run

	TestSuiteReference *suiteReference = NULL;
	for(suiteReference = suites; suiteReference; suiteReference = suiteReference->next) {
		char *testSuiteName = suiteReference->name;

		// if the current suite isn't selected, go to next suite
		void *suite = LoadTestSuite(testSuiteName);
		TestCaseReference **tests = QueryTestCases(suite);

		TestCaseReference *reference = NULL;
		int counter = 0;
		for(reference = tests[counter]; reference; reference = tests[++counter]) {
			if(only_selected_test && SDL_strncmp(selected_test_name, reference->name, NAME_BUFFER_SIZE) != 0) {
				continue;
			}

			if(reference->enabled == TEST_DISABLED) {
				printf("Test %s (in %s) disabled. Omitting...\n", reference->name, testSuiteName);
			} else {
				printf("Executing %s (in %s):\n", reference->name, testSuiteName);

				int retVal = ExecuteTest(suite, reference);

				if(retVal) {
					failureCount++;
					if(retVal == 2) {
						printf("%s (in %s): FAILED -> No asserts\n", reference->name, testSuiteName);
					} else {
						printf("%s (in %s): FAILED\n", reference->name, testSuiteName);
					}
				} else {
					passCount++;
					printf("%s (in %s): ok\n", reference->name, testSuiteName);
				}
			}

			printf("\n");
		}

		SDL_UnloadObject(suite);
	}

	const Uint32 endTicks = SDL_GetTicks();

	printf("Ran %d tests in %0.5f seconds.\n", (passCount + failureCount), (endTicks-startTicks)/1000.0f);

	printf("%d tests passed\n", passCount);
	printf("%d tests failed\n", failureCount);

	// Deallocate the memory used by test suites
	TestSuiteReference *ref = suites;
	while(ref) {
		SDL_free(ref->name);

		TestSuiteReference *temp = ref->next;
		SDL_free(ref);
		ref = temp;
	}

	return 0;
}
