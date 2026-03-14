/* =========================================================================
    Unity - A Test Framework for C  (minimal public header)
    ThrowTheSwitch.org
    Copyright (c) 2007-23 Mike Karlesky, Mark VanderVoord, & Greg Williams
    SPDX-License-Identifier: MIT
   ========================================================================= */

#ifndef UNITY_FRAMEWORK_H
#define UNITY_FRAMEWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Portable types                                                       */
/* ------------------------------------------------------------------ */
typedef unsigned int  UNITY_UINT;
typedef   signed int  UNITY_INT;
typedef unsigned long UNITY_UINT32;
typedef unsigned int  UNITY_LINE_TYPE;
typedef unsigned int  UNITY_FLAGS_T;
/* UNITY_PTR_ATTRIBUTE is intentionally empty — pointer attributes are
 * compiler-specific and not needed for host-compiled tests. */
#define UNITY_PTR_ATTRIBUTE

typedef enum {
    UNITY_DISPLAY_STYLE_INT  = 0,
    UNITY_DISPLAY_STYLE_UINT = 1,
    UNITY_DISPLAY_STYLE_HEX8,
    UNITY_DISPLAY_STYLE_HEX16,
    UNITY_DISPLAY_STYLE_HEX32
} UNITY_DISPLAY_STYLE_T;

/* Display range for output buffers */
#define UNITY_DISPLAY_RANGE_INT   16
#define UNITY_DISPLAY_RANGE_UINT  16

/* ------------------------------------------------------------------ */
/* Framework state                                                      */
/* ------------------------------------------------------------------ */
struct UNITY_STORAGE_T {
    const char* TestFile;
    const char* CurrentTestName;
    UNITY_LINE_TYPE CurrentTestLineNumber;
    unsigned int NumberOfTests;
    unsigned int TestFailures;
    unsigned int TestIgnores;
    unsigned int CurrentTestFailed;
    unsigned int CurrentTestIgnored;
};

extern struct UNITY_STORAGE_T Unity;

typedef void (*UnityTestFunction)(void);

/* ------------------------------------------------------------------ */
/* Framework API                                                        */
/* ------------------------------------------------------------------ */
void UnityBegin(const char* filename);
int  UnityEnd(void);
void UnityDefaultTestRun(UnityTestFunction Func, const char* FuncName, int FuncLineNum);

void UnityFail_   (const char* msg, UNITY_LINE_TYPE line);
void UnityIgnore_ (const char* msg, UNITY_LINE_TYPE line);

void UnityAssertEqualNumber(UNITY_INT expected, UNITY_INT actual,
        const char* msg, UNITY_LINE_TYPE line, UNITY_DISPLAY_STYLE_T style);
void UnityAssertEqualUnsignedNumber(UNITY_UINT expected, UNITY_UINT actual,
        const char* msg, UNITY_LINE_TYPE line, UNITY_DISPLAY_STYLE_T style);
void UnityAssertBits(UNITY_INT mask, UNITY_INT expected, UNITY_INT actual,
        const char* msg, UNITY_LINE_TYPE line);
void UnityAssertEqualString(const char* expected, const char* actual,
        const char* msg, UNITY_LINE_TYPE line);
void UnityAssertEqualMemory(const void* expected,
        const void* actual, UNITY_UINT32 length,
        UNITY_UINT32 num_elements, const char* msg, UNITY_LINE_TYPE line,
        UNITY_FLAGS_T flags);
void UnityAssertNumbersWithin(UNITY_UINT delta, UNITY_INT expected, UNITY_INT actual,
        const char* msg, UNITY_LINE_TYPE line, UNITY_DISPLAY_STYLE_T style);

void UnityPrint(const char* string);
void UnityPrintNumber(UNITY_INT number);
void UnityPrintNumberUnsigned(UNITY_UINT number);
void UnityPrintNumberHex(UNITY_UINT number, char nibbles);

/* ------------------------------------------------------------------ */
/* Assertion macros                                                     */
/* ------------------------------------------------------------------ */
#define TEST_LINE_NUM    (__LINE__)
#define TEST_FILE        (__FILE__)

#define TEST_FAIL_MESSAGE(msg)    UnityFail_((msg), TEST_LINE_NUM)
#define TEST_FAIL()               UnityFail_(NULL,  TEST_LINE_NUM)
#define TEST_IGNORE_MESSAGE(msg)  UnityIgnore_((msg), TEST_LINE_NUM)
#define TEST_IGNORE()             UnityIgnore_(NULL,   TEST_LINE_NUM)

#define TEST_ASSERT(cond) \
    do { if (!(cond)) UnityFail_("Expected TRUE was FALSE", TEST_LINE_NUM); } while (0)

#define TEST_ASSERT_TRUE(cond)    TEST_ASSERT(cond)
#define TEST_ASSERT_FALSE(cond)   TEST_ASSERT(!(cond))
#define TEST_ASSERT_NULL(p)       TEST_ASSERT((p) == NULL)
#define TEST_ASSERT_NOT_NULL(p)   TEST_ASSERT((p) != NULL)

#define TEST_ASSERT_EQUAL_INT(e, a) \
    UnityAssertEqualNumber((UNITY_INT)(e),(UNITY_INT)(a),NULL,TEST_LINE_NUM,UNITY_DISPLAY_STYLE_INT)
#define TEST_ASSERT_EQUAL_UINT(e, a) \
    UnityAssertEqualUnsignedNumber((UNITY_UINT)(e),(UNITY_UINT)(a),NULL,TEST_LINE_NUM,UNITY_DISPLAY_STYLE_UINT)
#define TEST_ASSERT_EQUAL_HEX8(e, a) \
    UnityAssertEqualUnsignedNumber((UNITY_UINT)(e),(UNITY_UINT)(a),NULL,TEST_LINE_NUM,UNITY_DISPLAY_STYLE_HEX8)
#define TEST_ASSERT_EQUAL_HEX16(e, a) \
    UnityAssertEqualUnsignedNumber((UNITY_UINT)(e),(UNITY_UINT)(a),NULL,TEST_LINE_NUM,UNITY_DISPLAY_STYLE_HEX16)
#define TEST_ASSERT_EQUAL_HEX32(e, a) \
    UnityAssertEqualUnsignedNumber((UNITY_UINT)(e),(UNITY_UINT)(a),NULL,TEST_LINE_NUM,UNITY_DISPLAY_STYLE_HEX32)

#define TEST_ASSERT_EQUAL_STRING(e, a) \
    UnityAssertEqualString((e),(a),NULL,TEST_LINE_NUM)

#define TEST_ASSERT_EQUAL_MEMORY(e, a, len) \
    UnityAssertEqualMemory((e),(a),(UNITY_UINT32)(len),1,NULL,TEST_LINE_NUM,0)

#define TEST_ASSERT_INT_WITHIN(d, e, a) \
    UnityAssertNumbersWithin((UNITY_UINT)(d),(UNITY_INT)(e),(UNITY_INT)(a),NULL,TEST_LINE_NUM,UNITY_DISPLAY_STYLE_INT)

#define TEST_ASSERT_EQUAL(e, a) \
    UnityAssertEqualNumber((UNITY_INT)(e),(UNITY_INT)(a),NULL,TEST_LINE_NUM,UNITY_DISPLAY_STYLE_INT)

/* Convenience runner macro */
#define RUN_TEST(func) \
    UnityDefaultTestRun(func, #func, __LINE__)

#ifdef __cplusplus
}
#endif

#endif /* UNITY_FRAMEWORK_H */
