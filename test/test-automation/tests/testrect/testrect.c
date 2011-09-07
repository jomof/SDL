/**
 * Original code: automated SDL rect test written by Edgar Simo "bobbens"
 */

#include <stdio.h>

#include <SDL/SDL.h>

#include "../../include/SDL_test.h"

/* Test cases */
static const TestCaseReference test1 =
		(TestCaseReference){ "rect_testIntersectRectAndLine", "Tests SDL_IntersectRectAndLine clipping cases", TEST_ENABLED, 0, 0 };

static const TestCaseReference test2 =
		(TestCaseReference){ "rect_testIntersectRectAndLineInside", "Tests SDL_IntersectRectAndLine with line fully contained in rect", TEST_ENABLED, 0, 0 };

static const TestCaseReference test3 =
		(TestCaseReference){ "rect_testIntersectRectAndLineOutside", "Tests SDL_IntersectRectAndLine with line fully contained in rect", TEST_ENABLED, 0, 0 };

static const TestCaseReference test4 =
		(TestCaseReference){ "rect_testIntersectRectAndLineParam", "Negative tests against SDL_IntersectRectAndLine with invalid parameters", TEST_ENABLED, 0, 0 };

static const TestCaseReference test5 =
		(TestCaseReference){ "rect_testIntersectRectInside", "Tests SDL_IntersectRect with B fully contained in A", TEST_ENABLED, 0, 0 };

static const TestCaseReference test6 =
		(TestCaseReference){ "rect_testIntersectRectOutside", "Tests SDL_IntersectRect with B fully outside of A", TEST_ENABLED, 0, 0 };

static const TestCaseReference test7 =
		(TestCaseReference){ "rect_testIntersectRectPartial", "Tests SDL_IntersectRect with B partially intersecting A", TEST_ENABLED, 0, 0 };

static const TestCaseReference test8 =
		(TestCaseReference){ "rect_testIntersectRectPoint", "Tests SDL_IntersectRect with 1x1 sized rectangles", TEST_ENABLED, 0, 0 };

static const TestCaseReference test9 =
		(TestCaseReference){ "rect_testIntersectRectParam", "Negative tests against SDL_IntersectRect with invalid parameters", TEST_ENABLED, 0, 0 };

static const TestCaseReference test10 =
		(TestCaseReference){ "rect_testHasIntersectionInside", "Tests SDL_HasIntersection with B fully contained in A", TEST_ENABLED, 0, 0 };

static const TestCaseReference test11 =
		(TestCaseReference){ "rect_testHasIntersectionOutside", "Tests SDL_HasIntersection with B fully outside of A", TEST_ENABLED, 0, 0 };

static const TestCaseReference test12 =
		(TestCaseReference){ "rect_testHasIntersectionPartial", "Tests SDL_HasIntersection with B partially intersecting A", TEST_ENABLED, 0, 0 };

static const TestCaseReference test13 =
		(TestCaseReference){ "rect_testHasIntersectionPoint", "Tests SDL_HasIntersection with 1x1 sized rectangles", TEST_ENABLED, 0, 0 };

static const TestCaseReference test14 =
		(TestCaseReference){ "rect_testHasIntersectionParam", "Negative tests against SDL_HasIntersection with invalid parameters", TEST_ENABLED, 0, 0 };


/* Test suite */
extern const TestCaseReference *testSuite[] =  {
	&test1, &test2, &test3, &test4, &test5, &test6, &test7, &test8, &test9, &test10, &test11, &test12, &test13, &test14, NULL
};

TestCaseReference **QueryTestSuite() {
	return (TestCaseReference **)testSuite;
}

/*!
 * \brief Private helper to check SDL_IntersectRectAndLine results
 */
void _validateIntersectRectAndLineResults(
    SDL_bool intersection, SDL_bool expectedIntersection,
    SDL_Rect *rect, SDL_Rect * refRect,
    int x1, int y1, int x2, int y2,
    int x1Ref, int y1Ref, int x2Ref, int y2Ref)
{
    AssertTrue(intersection == expectedIntersection, 
        "Incorrect intersection result: expected %s, got %s intersecting rect (%d,%d,%d,%d) with line (%d,%d - %d,%d)\n",
        (expectedIntersection == SDL_TRUE) ? "true" : "false",
        (intersection == SDL_TRUE) ? "true" : "false",
        refRect->x, refRect->y, refRect->w, refRect->h,
        x1Ref, y1Ref, x2Ref, y2Ref);
    AssertTrue(rect->x == refRect->x && rect->y == refRect->y && rect->w == refRect->w && rect->h == refRect->h,
        "Source rectangle was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rect->x, rect->y, rect->w, rect->h,
        refRect->x, refRect->y, refRect->w, refRect->h);
    AssertTrue(x1 == x1Ref && y1 == y1Ref && x2 == x2Ref && y2 == y2Ref,
        "Line was incorrectly clipped or modified: got (%d,%d - %d,%d) expected (%d,%d - %d,%d)",
        x1, y1, x2, y2,
        x1Ref, y1Ref, x2Ref, y2Ref);
}

/*!
 * \brief Tests SDL_IntersectRectAndLine() clipping cases
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRectAndLine
 */
int rect_testIntersectRectAndLine (void *arg)
{
    SDL_Rect refRect = { 0, 0, 32, 32 };
    SDL_Rect rect;
    int x1, y1;
    int x2, y2;
    SDL_bool intersected;

    int xLeft = -RandomIntegerInRange(1, refRect.w);
    int xRight = refRect.w + RandomIntegerInRange(1, refRect.w);
    int yTop = -RandomIntegerInRange(1, refRect.h);
    int yBottom = refRect.h + RandomIntegerInRange(1, refRect.h);

    x1 = xLeft;
    y1 = 15;
    x2 = xRight;
    y2 = 15;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_TRUE, &rect, &refRect, x1, y1, x2, y2, 0, 15, 31, 15);

    x1 = 15;
    y1 = yTop;
    x2 = 15;
    y2 = yBottom;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_TRUE, &rect, &refRect, x1, y1, x2, y2, 15, 0, 15, 31);

    x1 = -refRect.w;
    y1 = -refRect.h;
    x2 = 2*refRect.w;
    y2 = 2*refRect.h;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
     _validateIntersectRectAndLineResults(intersected, SDL_TRUE, &rect, &refRect, x1, y1, x2, y2, 0, 0, 31, 31);

    x1 = 2*refRect.w;
    y1 = 2*refRect.h;
    x2 = -refRect.w;
    y2 = -refRect.h;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_TRUE, &rect, &refRect, x1, y1, x2, y2, 31, 31, 0, 0);

    x1 = -1;
    y1 = 32;
    x2 = 32;
    y2 = -1;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_TRUE, &rect, &refRect, x1, y1, x2, y2, 0, 31, 31, 0);

    x1 = 32;
    y1 = -1;
    x2 = -1;
    y2 = 32;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_TRUE, &rect, &refRect, x1, y1, x2, y2, 31, 0, 0, 31);
}

/*!
 * \brief Tests SDL_IntersectRectAndLine() non-clipping case line inside
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRectAndLine
 */
int rect_testIntersectRectAndLineInside (void *arg)
{
    SDL_Rect refRect = { 0, 0, 32, 32 };
    SDL_Rect rect;
    int x1, y1;
    int x2, y2;
    SDL_bool intersected;

    int xmin = refRect.x;
    int xmax = refRect.x + refRect.w - 1;
    int ymin = refRect.y;
    int ymax = refRect.y + refRect.h - 1;
    int x1Ref = RandomIntegerInRange(xmin + 1, xmax - 1);
    int y1Ref = RandomIntegerInRange(ymin + 1, ymax - 1);
    int x2Ref = RandomIntegerInRange(xmin + 1, xmax - 1);
    int y2Ref = RandomIntegerInRange(ymin + 1, ymax - 1);

    x1 = x1Ref;
    y1 = y1Ref;
    x2 = x2Ref;
    y2 = y2Ref;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_TRUE, &rect, &refRect, x1, y1, x2, y2, x1Ref, y1Ref, x2Ref, y2Ref);

    x1 = x1Ref;
    y1 = y1Ref;
    x2 = xmax;
    y2 = ymax;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_TRUE, &rect, &refRect, x1, y1, x2, y2, x1Ref, y1Ref, xmax, ymax);

    x1 = xmin;
    y1 = ymin;
    x2 = x2Ref;
    y2 = y2Ref;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_TRUE, &rect, &refRect, x1, y1, x2, y2, xmin, ymin, x2Ref, y2Ref);

    x1 = xmin;
    y1 = ymin;
    x2 = xmax;
    y2 = ymax;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_TRUE, &rect, &refRect, x1, y1, x2, y2, xmin, ymin, xmax, ymax);

    x1 = xmin;
    y1 = ymax;
    x2 = xmax;
    y2 = ymin;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_TRUE, &rect, &refRect, x1, y1, x2, y2, xmin, ymax, xmax, ymin);
}


/*!
 * \brief Tests SDL_IntersectRectAndLine() non-clipping cases outside
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRectAndLine
 */
int rect_testIntersectRectAndLineOutside (void *arg)
{
    SDL_Rect refRect = { 0, 0, 32, 32 };
    SDL_Rect rect;
    int x1, y1;
    int x2, y2;
    SDL_bool intersected;

    int xLeft = -RandomIntegerInRange(1, refRect.w);
    int xRight = refRect.w + RandomIntegerInRange(1, refRect.w);
    int yTop = -RandomIntegerInRange(1, refRect.h);
    int yBottom = refRect.h + RandomIntegerInRange(1, refRect.h);

    x1 = xLeft;
    y1 = 0;
    x2 = xLeft;
    y2 = 31;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_FALSE, &rect, &refRect, x1, y1, x2, y2, xLeft, 0, xLeft, 31);

    x1 = xRight;
    y1 = 0;
    x2 = xRight;
    y2 = 31;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_FALSE, &rect, &refRect, x1, y1, x2, y2, xRight, 0, xRight, 31);

    x1 = 0;
    y1 = yTop;
    x2 = 31;
    y2 = yTop;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_FALSE, &rect, &refRect, x1, y1, x2, y2, 0, yTop, 31, yTop);

    x1 = 0;
    y1 = yBottom;
    x2 = 31;
    y2 = yBottom;
    rect = refRect;
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    _validateIntersectRectAndLineResults(intersected, SDL_FALSE, &rect, &refRect, x1, y1, x2, y2, 0, yBottom, 31, yBottom);
}

/*!
 * \brief Negative tests against SDL_IntersectRectAndLine() with invalid parameters
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRectAndLine
 */
int rect_testIntersectRectAndLineParam (void *arg)
{
    SDL_Rect rect = { 0, 0, 32, 32 };
    int x1 = rect.w / 2;
    int y1 = rect.h / 2;
    int x2 = x1;
    int y2 = 2 * rect.h;
    SDL_bool intersected;
    
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2);
    AssertTrue(intersected == SDL_TRUE, "Test variables not intersected as expected");
    
    intersected = SDL_IntersectRectAndLine((SDL_Rect *)NULL, &x1, &y1, &x2, &y2);
    AssertTrue(intersected == SDL_FALSE, "Incorrect intersected result when 1st parameter is NULL");
    intersected = SDL_IntersectRectAndLine(&rect, (int *)NULL, &y1, &x2, &y2);
    AssertTrue(intersected == SDL_FALSE, "Incorrect intersected result when 2nd parameter is NULL");
    intersected = SDL_IntersectRectAndLine(&rect, &x1, (int *)NULL, &x2, &y2);
    AssertTrue(intersected == SDL_FALSE, "Incorrect intersected result when 3rd parameter is NULL");
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, (int *)NULL, &y2);
    AssertTrue(intersected == SDL_FALSE, "Incorrect intersected result when 4th parameter is NULL");
    intersected = SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, (int *)NULL);
    AssertTrue(intersected == SDL_FALSE, "Incorrect intersected result when 5th parameter is NULL");
    intersected = SDL_IntersectRectAndLine((SDL_Rect *)NULL, (int *)NULL, (int *)NULL, (int *)NULL, (int *)NULL);
    AssertTrue(intersected == SDL_FALSE, "Incorrect intersected result when all parameters are NULL");
}

/*!
 * \brief Private helper to check SDL_HasIntersection results
 */
void _validateHasIntersectionResults(
    SDL_bool intersection, SDL_bool expectedIntersection, 
    SDL_Rect *rectA, SDL_Rect *rectB, SDL_Rect *refRectA, SDL_Rect *refRectB)
{
    AssertTrue(intersection == expectedIntersection, 
        "Incorrect intersection result: expected %s, got %s intersecting A (%d,%d,%d,%d) with B (%d,%d,%d,%d)\n",
        (expectedIntersection == SDL_TRUE) ? "true" : "false",
        (intersection == SDL_TRUE) ? "true" : "false",
        rectA->x, rectA->y, rectA->w, rectA->h, 
        rectB->x, rectB->y, rectB->w, rectB->h);
    AssertTrue(rectA->x == refRectA->x && rectA->y == refRectA->y && rectA->w == refRectA->w && rectA->h == refRectA->h,
        "Source rectangle A was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectA->x, rectA->y, rectA->w, rectA->h,
        refRectA->x, refRectA->y, refRectA->w, refRectA->h);
    AssertTrue(rectB->x == refRectB->x && rectB->y == refRectB->y && rectB->w == refRectB->w && rectB->h == refRectB->h,
        "Source rectangle B was modified: got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
        rectB->x, rectB->y, rectB->w, rectB->h,
        refRectB->x, refRectB->y, refRectB->w, refRectB->h);
}

/*!
 * \brief Private helper to check SDL_IntersectRect results
 */
void _validateIntersectRectResults(
    SDL_bool intersection, SDL_bool expectedIntersection, 
    SDL_Rect *rectA, SDL_Rect *rectB, SDL_Rect *refRectA, SDL_Rect *refRectB, 
    SDL_Rect *result, SDL_Rect *expectedResult)
{
    _validateHasIntersectionResults(intersection, expectedIntersection, rectA, rectB, refRectA, refRectB);
    if (result && expectedResult) {
        AssertTrue(result->x == expectedResult->x && result->y == expectedResult->y && result->w == expectedResult->w && result->h == expectedResult->h,
            "Intersection of rectangles A (%d,%d,%d,%d) and B (%d,%d,%d,%d) was incorrectly calculated, got (%d,%d,%d,%d) expected (%d,%d,%d,%d)",
            rectA->x, rectA->y, rectA->w, rectA->h, 
            rectB->x, rectB->y, rectB->w, rectB->h,
            result->x, result->y, result->w, result->h,
            expectedResult->x, expectedResult->y, expectedResult->w, expectedResult->h);
    }
}

/*!
 * \brief Tests SDL_IntersectRect() with B fully inside A
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRect
 */
int rect_testIntersectRectInside (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 32, 32 };
    SDL_Rect refRectB;
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_Rect result;
    SDL_bool intersection;

    // rectB fully contained in rectA
    refRectB.x = 0;
    refRectB.y = 0;
    refRectB.w = RandomIntegerInRange(refRectA.x + 1, refRectA.x + refRectA.w - 1);
    refRectB.h = RandomIntegerInRange(refRectA.y + 1, refRectA.y + refRectA.h - 1);
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    _validateIntersectRectResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB, &result, &refRectB);
}

/*!
 * \brief Tests SDL_IntersectRect() with B fully outside A
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRect
 */
int rect_testIntersectRectOutside (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 32, 32 };
    SDL_Rect refRectB;
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_Rect result;
    SDL_bool intersection;

    // rectB fully outside of rectA
    refRectB.x = refRectA.x + refRectA.w + RandomIntegerInRange(1, 10);
    refRectB.y = refRectA.y + refRectA.h + RandomIntegerInRange(1, 10);
    refRectB.w = refRectA.w;
    refRectB.h = refRectA.h;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    _validateIntersectRectResults(intersection, SDL_FALSE, &rectA, &rectB, &refRectA, &refRectB, (SDL_Rect *)NULL, (SDL_Rect *)NULL);    
}

/*!
 * \brief Tests SDL_IntersectRect() with B partially intersecting A
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRect
 */
int rect_testIntersectRectPartial (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 32, 32 };
    SDL_Rect refRectB;
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_Rect result;
    SDL_Rect expectedResult;
    SDL_bool intersection;

    // rectB partially contained in rectA
    refRectB.x = RandomIntegerInRange(refRectA.x + 1, refRectA.x + refRectA.w - 1);
    refRectB.y = RandomIntegerInRange(refRectA.y + 1, refRectA.y + refRectA.h - 1);
    refRectB.w = refRectA.w;
    refRectB.h = refRectA.h;
    rectA = refRectA;
    rectB = refRectB;
    expectedResult.x = refRectB.x;
    expectedResult.y = refRectB.y;
    expectedResult.w = refRectA.w - refRectB.x;
    expectedResult.h = refRectA.h - refRectB.y;    
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    _validateIntersectRectResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB, &result, &expectedResult);

    // rectB right edge
    refRectB.x = rectA.w - 1;
    refRectB.y = rectA.y;
    refRectB.w = RandomIntegerInRange(1, refRectA.w - 1);
    refRectB.h = RandomIntegerInRange(1, refRectA.h - 1);
    rectA = refRectA;
    rectB = refRectB;
    expectedResult.x = refRectB.x;
    expectedResult.y = refRectB.y;
    expectedResult.w = 1;
    expectedResult.h = refRectB.h;    
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    _validateIntersectRectResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB, &result, &expectedResult);

    // rectB left edge
    refRectB.x = 1 - rectA.w;
    refRectB.y = rectA.y;
    refRectB.w = refRectA.w;
    refRectB.h = RandomIntegerInRange(1, refRectA.h - 1);
    rectA = refRectA;
    rectB = refRectB;
    expectedResult.x = 0;
    expectedResult.y = refRectB.y;
    expectedResult.w = 1;
    expectedResult.h = refRectB.h;    
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    _validateIntersectRectResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB, &result, &expectedResult);

    // rectB bottom edge
    refRectB.x = rectA.x;
    refRectB.y = rectA.h - 1;
    refRectB.w = RandomIntegerInRange(1, refRectA.w - 1);
    refRectB.h = RandomIntegerInRange(1, refRectA.h - 1);
    rectA = refRectA;
    rectB = refRectB;
    expectedResult.x = refRectB.x;
    expectedResult.y = refRectB.y;
    expectedResult.w = refRectB.w;
    expectedResult.h = 1;    
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    _validateIntersectRectResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB, &result, &expectedResult);

    // rectB top edge
    refRectB.x = rectA.x;
    refRectB.y = 1 - rectA.h;
    refRectB.w = RandomIntegerInRange(1, refRectA.w - 1);
    refRectB.h = rectA.h;
    rectA = refRectA;
    rectB = refRectB;
    expectedResult.x = refRectB.x;
    expectedResult.y = 0;
    expectedResult.w = refRectB.w;
    expectedResult.h = 1;    
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    _validateIntersectRectResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB, &result, &expectedResult);
}

/*!
 * \brief Tests SDL_IntersectRect() with 1x1 pixel sized rectangles
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRect
 */
int rect_testIntersectRectPoint (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 1, 1 };
    SDL_Rect refRectB = { 0, 0, 1, 1 };
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_Rect result;
    SDL_bool intersection;
    int offsetX, offsetY;

    // intersecting pixels
    refRectA.x = RandomIntegerInRange(1, 100);
    refRectA.y = RandomIntegerInRange(1, 100);
    refRectB.x = refRectA.x;
    refRectB.y = refRectA.y;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_IntersectRect(&rectA, &rectB, &result);
    _validateIntersectRectResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB, &result, &refRectA);

    // non-intersecting pixels cases
    for (offsetX = -1; offsetX <= 1; offsetX++) {
        for (offsetY = -1; offsetY <= 1; offsetY++) {
            if (offsetX != 0 || offsetY != 0) {
                refRectA.x = RandomIntegerInRange(1, 100);
                refRectA.y = RandomIntegerInRange(1, 100);
                refRectB.x = refRectA.x;
                refRectB.y = refRectA.y;    
                refRectB.x += offsetX;
                refRectB.y += offsetY;
                rectA = refRectA;
                rectB = refRectB;
                intersection = SDL_IntersectRect(&rectA, &rectB, &result);
                _validateIntersectRectResults(intersection, SDL_FALSE, &rectA, &rectB, &refRectA, &refRectB, (SDL_Rect *)NULL, (SDL_Rect *)NULL);
            }
        }
    }
}

/*!
 * \brief Negative tests against SDL_IntersectRect() with invalid parameters
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_IntersectRect
 */
int rect_testIntersectRectParam(void *arg)
{
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_Rect result;
    SDL_bool intersection;

    // invalid parameter combinations
    intersection = SDL_IntersectRect((SDL_Rect *)NULL, &rectB, &result);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 1st parameter was NULL"); 
    intersection = SDL_IntersectRect(&rectA, (SDL_Rect *)NULL, &result);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 2st parameter was NULL"); 
    intersection = SDL_IntersectRect(&rectA, &rectB, (SDL_Rect *)NULL);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 3st parameter was NULL"); 
    intersection = SDL_IntersectRect((SDL_Rect *)NULL, (SDL_Rect *)NULL, &result);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 1st and 2nd parameters were NULL"); 
    intersection = SDL_IntersectRect((SDL_Rect *)NULL, &rectB, (SDL_Rect *)NULL);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 1st and 3rd parameters were NULL "); 
    intersection = SDL_IntersectRect((SDL_Rect *)NULL, (SDL_Rect *)NULL, (SDL_Rect *)NULL);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when all parameters were NULL");     
}

/*!
 * \brief Tests SDL_HasIntersection() with B fully inside A
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_HasIntersection
 */
int rect_testHasIntersectionInside (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 32, 32 };
    SDL_Rect refRectB;
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_bool intersection;

    // rectB fully contained in rectA
    refRectB.x = 0;
    refRectB.y = 0;
    refRectB.w = RandomIntegerInRange(refRectA.x + 1, refRectA.x + refRectA.w - 1);
    refRectB.h = RandomIntegerInRange(refRectA.y + 1, refRectA.y + refRectA.h - 1);
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_HasIntersection(&rectA, &rectB);
    _validateHasIntersectionResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB);
}

/*!
 * \brief Tests SDL_HasIntersection() with B fully outside A
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_HasIntersection
 */
int rect_testHasIntersectionOutside (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 32, 32 };
    SDL_Rect refRectB;
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_bool intersection;

    // rectB fully outside of rectA
    refRectB.x = refRectA.x + refRectA.w + RandomIntegerInRange(1, 10);
    refRectB.y = refRectA.y + refRectA.h + RandomIntegerInRange(1, 10);
    refRectB.w = refRectA.w;
    refRectB.h = refRectA.h;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_HasIntersection(&rectA, &rectB);
    _validateHasIntersectionResults(intersection, SDL_FALSE, &rectA, &rectB, &refRectA, &refRectB);
}

/*!
 * \brief Tests SDL_HasIntersection() with B partially intersecting A
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_HasIntersection
 */
int rect_testHasIntersectionPartial (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 32, 32 };
    SDL_Rect refRectB;
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_bool intersection;

    // rectB partially contained in rectA
    refRectB.x = RandomIntegerInRange(refRectA.x + 1, refRectA.x + refRectA.w - 1);
    refRectB.y = RandomIntegerInRange(refRectA.y + 1, refRectA.y + refRectA.h - 1);
    refRectB.w = refRectA.w;
    refRectB.h = refRectA.h;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_HasIntersection(&rectA, &rectB);
    _validateHasIntersectionResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB);

    // rectB right edge
    refRectB.x = rectA.w - 1;
    refRectB.y = rectA.y;
    refRectB.w = RandomIntegerInRange(1, refRectA.w - 1);
    refRectB.h = RandomIntegerInRange(1, refRectA.h - 1);
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_HasIntersection(&rectA, &rectB);
    _validateHasIntersectionResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB);

    // rectB left edge
    refRectB.x = 1 - rectA.w;
    refRectB.y = rectA.y;
    refRectB.w = refRectA.w;
    refRectB.h = RandomIntegerInRange(1, refRectA.h - 1);
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_HasIntersection(&rectA, &rectB);
    _validateHasIntersectionResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB);

    // rectB bottom edge
    refRectB.x = rectA.x;
    refRectB.y = rectA.h - 1;
    refRectB.w = RandomIntegerInRange(1, refRectA.w - 1);
    refRectB.h = RandomIntegerInRange(1, refRectA.h - 1);
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_HasIntersection(&rectA, &rectB);
    _validateHasIntersectionResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB);

    // rectB top edge
    refRectB.x = rectA.x;
    refRectB.y = 1 - rectA.h;
    refRectB.w = RandomIntegerInRange(1, refRectA.w - 1);
    refRectB.h = rectA.h;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_HasIntersection(&rectA, &rectB);
    _validateHasIntersectionResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB);
}

/*!
 * \brief Tests SDL_HasIntersection() with 1x1 pixel sized rectangles
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_HasIntersection
 */
int rect_testHasIntersectionPoint (void *arg)
{
    SDL_Rect refRectA = { 0, 0, 1, 1 };
    SDL_Rect refRectB = { 0, 0, 1, 1 };
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_Rect result;
    SDL_bool intersection;
    int offsetX, offsetY;

    // intersecting pixels
    refRectA.x = RandomIntegerInRange(1, 100);
    refRectA.y = RandomIntegerInRange(1, 100);
    refRectB.x = refRectA.x;
    refRectB.y = refRectA.y;
    rectA = refRectA;
    rectB = refRectB;
    intersection = SDL_HasIntersection(&rectA, &rectB);
    _validateHasIntersectionResults(intersection, SDL_TRUE, &rectA, &rectB, &refRectA, &refRectB);

    // non-intersecting pixels cases
    for (offsetX = -1; offsetX <= 1; offsetX++) {
        for (offsetY = -1; offsetY <= 1; offsetY++) {
            if (offsetX != 0 || offsetY != 0) {
                refRectA.x = RandomIntegerInRange(1, 100);
                refRectA.y = RandomIntegerInRange(1, 100);
                refRectB.x = refRectA.x;
                refRectB.y = refRectA.y;    
                refRectB.x += offsetX;
                refRectB.y += offsetY;
                rectA = refRectA;
                rectB = refRectB;
                intersection = SDL_HasIntersection(&rectA, &rectB);
                _validateHasIntersectionResults(intersection, SDL_FALSE, &rectA, &rectB, &refRectA, &refRectB);
            }
        }
    }
}

/*!
 * \brief Negative tests against SDL_HasIntersection() with invalid parameters
 *
 * \sa
 * http://wiki.libsdl.org/moin.cgi/SDL_HasIntersection
 */
int rect_testHasIntersectionParam(void *arg)
{
    SDL_Rect rectA;
    SDL_Rect rectB;
    SDL_bool intersection;

    // invalid parameter combinations
    intersection = SDL_HasIntersection((SDL_Rect *)NULL, &rectB);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 1st parameter was NULL"); 
    intersection = SDL_HasIntersection(&rectA, (SDL_Rect *)NULL);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when 2st parameter was NULL"); 
    intersection = SDL_HasIntersection((SDL_Rect *)NULL, (SDL_Rect *)NULL);
    AssertTrue(intersection == SDL_FALSE, "Function did not return false when all parameters were NULL");     
}
