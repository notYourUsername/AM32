/* =========================================================================
    Unity - A Test Framework for C
    ThrowTheSwitch.org
    Copyright (c) 2007-23 Mike Karlesky, Mark VanderVoord, & Greg Williams
    SPDX-License-Identifier: MIT
   ========================================================================= */

#include "unity.h"
#include <stdio.h>
#include <string.h>

/* Silence warnings when UNITY_OUTPUT_CHAR is not defined */
#ifndef UNITY_OUTPUT_CHAR
#define UNITY_OUTPUT_CHAR(a) putchar(a)
#endif

struct UNITY_STORAGE_T Unity;

/* ------------------------------------------------------------------ */
/* Internal helpers                                                    */
/* ------------------------------------------------------------------ */

static void UnityPrintChar(const char* pch)
{
    if ((*pch <= 126) && (*pch >= 32))
        UNITY_OUTPUT_CHAR(*pch);
    else
        UNITY_OUTPUT_CHAR('?');
}

void UnityPrint(const char* string)
{
    const char* pch = string;
    if (pch != NULL)
        while (*pch) { UnityPrintChar(pch); pch++; }
}

void UnityPrintLen(const char* string, const UNITY_UINT32 length)
{
    const char* pch = string;
    if (pch != NULL)
    {
        while (*pch && (UNITY_UINT32)(pch - string) < length)
        {
            UnityPrintChar(pch);
            pch++;
        }
    }
}

void UnityPrintNumberByFormat(const UNITY_INT number, const UNITY_DISPLAY_STYLE_T style)
{
    char output[UNITY_DISPLAY_RANGE_INT + 1];
    char* p = output + sizeof(output) - 1;
    UNITY_UINT abs_num = (UNITY_UINT)number;

    *p = '\0';

    if ((style == UNITY_DISPLAY_STYLE_INT) && (number < 0))
        abs_num = (UNITY_UINT)(0 - (UNITY_UINT)number);

    do {
        *--p = (char)('0' + (abs_num % 10));
        abs_num /= 10;
    } while (abs_num > 0);

    if ((style == UNITY_DISPLAY_STYLE_INT) && (number < 0))
        *--p = '-';

    UnityPrint(p);
}

void UnityPrintNumber(const UNITY_INT number)
{
    UnityPrintNumberByFormat(number, UNITY_DISPLAY_STYLE_INT);
}

void UnityPrintNumberUnsigned(const UNITY_UINT number)
{
    char output[UNITY_DISPLAY_RANGE_UINT + 1];
    char* p = output + sizeof(output) - 1;
    UNITY_UINT n = number;
    *p = '\0';
    do { *--p = (char)('0' + (n % 10)); n /= 10; } while (n > 0);
    UnityPrint(p);
}

void UnityPrintNumberHex(const UNITY_UINT number, const char nibbles)
{
    static const char hex[] = "0123456789ABCDEF";
    char output[9];
    char* p = output + 8;
    UNITY_UINT n = number;
    int i;
    *p = '\0';
    for (i = 0; i < 8; i++) { *--p = hex[n & 0xF]; n >>= 4; }
    /* Print only requested nibbles */
    UnityPrint(p + (8 - nibbles));
}

/* ------------------------------------------------------------------ */
/* Framework entry / exit                                              */
/* ------------------------------------------------------------------ */

void UnityBegin(const char* filename)
{
    Unity.TestFile = filename;
    Unity.CurrentTestName = NULL;
    Unity.CurrentTestLineNumber = 0;
    Unity.NumberOfTests = 0;
    Unity.TestFailures = 0;
    Unity.TestIgnores = 0;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
}

int UnityEnd(void)
{
    UnityPrint("\n-----------------------\n");
    UnityPrintNumber((UNITY_INT)Unity.NumberOfTests);
    UnityPrint(" Tests ");
    UnityPrintNumber((UNITY_INT)Unity.TestFailures);
    UnityPrint(" Failures ");
    UnityPrintNumber((UNITY_INT)Unity.TestIgnores);
    UnityPrint(" Ignored\n");
    if (Unity.TestFailures == 0U)
        UnityPrint("OK\n");
    else
        UnityPrint("FAIL\n");
    return (int)(Unity.TestFailures);
}

/* ------------------------------------------------------------------ */
/* Test execution                                                      */
/* ------------------------------------------------------------------ */

void UnityDefaultTestRun(UnityTestFunction Func, const char* FuncName, const int FuncLineNum)
{
    Unity.CurrentTestName = FuncName;
    Unity.CurrentTestLineNumber = (UNITY_LINE_TYPE)FuncLineNum;
    Unity.NumberOfTests++;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;

    Func();

    if (Unity.CurrentTestIgnored)
        Unity.TestIgnores++;
    else if (Unity.CurrentTestFailed)
        Unity.TestFailures++;
    else
    {
        UnityPrint(Unity.TestFile);
        UNITY_OUTPUT_CHAR(':');
        UnityPrintNumber((UNITY_INT)Unity.CurrentTestLineNumber);
        UNITY_OUTPUT_CHAR(':');
        UnityPrint(Unity.CurrentTestName);
        UnityPrint(":PASS\n");
    }
}

/* ------------------------------------------------------------------ */
/* Assertion internals                                                 */
/* ------------------------------------------------------------------ */

void UnityFail_(const char* msg, const UNITY_LINE_TYPE line)
{
    Unity.CurrentTestFailed = 1;
    UnityPrint(Unity.TestFile);
    UNITY_OUTPUT_CHAR(':');
    UnityPrintNumber((UNITY_INT)line);
    UNITY_OUTPUT_CHAR(':');
    UnityPrint(Unity.CurrentTestName ? Unity.CurrentTestName : "Unknown");
    UnityPrint(":FAIL");
    if (msg != NULL) { UNITY_OUTPUT_CHAR(':'); UnityPrint(msg); }
    UNITY_OUTPUT_CHAR('\n');
}

void UnityIgnore_(const char* msg, const UNITY_LINE_TYPE line)
{
    Unity.CurrentTestIgnored = 1;
    UnityPrint(Unity.TestFile);
    UNITY_OUTPUT_CHAR(':');
    UnityPrintNumber((UNITY_INT)line);
    UNITY_OUTPUT_CHAR(':');
    UnityPrint(Unity.CurrentTestName ? Unity.CurrentTestName : "Unknown");
    UnityPrint(":IGNORE");
    if (msg != NULL) { UNITY_OUTPUT_CHAR(':'); UnityPrint(msg); }
    UNITY_OUTPUT_CHAR('\n');
}

void UnityAssertEqualNumber(const UNITY_INT expected,
                            const UNITY_INT actual,
                            const char* msg,
                            const UNITY_LINE_TYPE lineNumber,
                            const UNITY_DISPLAY_STYLE_T style)
{
    if (expected != actual)
    {
        Unity.CurrentTestFailed = 1;
        UnityPrint(Unity.TestFile);
        UNITY_OUTPUT_CHAR(':');
        UnityPrintNumber((UNITY_INT)lineNumber);
        UNITY_OUTPUT_CHAR(':');
        UnityPrint(Unity.CurrentTestName);
        UnityPrint(":FAIL: Expected ");
        UnityPrintNumberByFormat(expected, style);
        UnityPrint(" Was ");
        UnityPrintNumberByFormat(actual, style);
        if (msg) { UnityPrint(" : "); UnityPrint(msg); }
        UNITY_OUTPUT_CHAR('\n');
    }
}

void UnityAssertEqualUnsignedNumber(const UNITY_UINT expected,
                                    const UNITY_UINT actual,
                                    const char* msg,
                                    const UNITY_LINE_TYPE lineNumber,
                                    const UNITY_DISPLAY_STYLE_T style)
{
    (void)style;
    if (expected != actual)
    {
        Unity.CurrentTestFailed = 1;
        UnityPrint(Unity.TestFile);
        UNITY_OUTPUT_CHAR(':');
        UnityPrintNumber((UNITY_INT)lineNumber);
        UNITY_OUTPUT_CHAR(':');
        UnityPrint(Unity.CurrentTestName);
        UnityPrint(":FAIL: Expected ");
        UnityPrintNumberUnsigned(expected);
        UnityPrint(" Was ");
        UnityPrintNumberUnsigned(actual);
        if (msg) { UnityPrint(" : "); UnityPrint(msg); }
        UNITY_OUTPUT_CHAR('\n');
    }
}

void UnityAssertBits(const UNITY_INT mask,
                     const UNITY_INT expected,
                     const UNITY_INT actual,
                     const char* msg,
                     const UNITY_LINE_TYPE lineNumber)
{
    if ((mask & expected) != (mask & actual))
    {
        Unity.CurrentTestFailed = 1;
        UnityPrint(Unity.TestFile);
        UNITY_OUTPUT_CHAR(':');
        UnityPrintNumber((UNITY_INT)lineNumber);
        UNITY_OUTPUT_CHAR(':');
        UnityPrint(Unity.CurrentTestName);
        UnityPrint(":FAIL: Expected ");
        UnityPrintNumberHex((UNITY_UINT)expected, 8);
        UnityPrint(" Was ");
        UnityPrintNumberHex((UNITY_UINT)actual, 8);
        if (msg) { UnityPrint(" : "); UnityPrint(msg); }
        UNITY_OUTPUT_CHAR('\n');
    }
}

void UnityAssertEqualString(const char* expected,
                            const char* actual,
                            const char* msg,
                            const UNITY_LINE_TYPE lineNumber)
{
    if ((expected == NULL) || (actual == NULL) || (strcmp(expected, actual) != 0))
    {
        Unity.CurrentTestFailed = 1;
        UnityPrint(Unity.TestFile);
        UNITY_OUTPUT_CHAR(':');
        UnityPrintNumber((UNITY_INT)lineNumber);
        UNITY_OUTPUT_CHAR(':');
        UnityPrint(Unity.CurrentTestName);
        UnityPrint(":FAIL: Expected '");
        UnityPrint(expected ? expected : "NULL");
        UnityPrint("' Was '");
        UnityPrint(actual ? actual : "NULL");
        UNITY_OUTPUT_CHAR('\'');
        if (msg) { UnityPrint(" : "); UnityPrint(msg); }
        UNITY_OUTPUT_CHAR('\n');
    }
}

void UnityAssertEqualMemory(const void* expected,
                            const void* actual,
                            const UNITY_UINT32 length,
                            const UNITY_UINT32 num_elements,
                            const char* msg,
                            const UNITY_LINE_TYPE lineNumber,
                            const UNITY_FLAGS_T flags)
{
    (void)flags;
    if (memcmp(expected, actual, (size_t)(length * num_elements)) != 0)
    {
        Unity.CurrentTestFailed = 1;
        UnityPrint(Unity.TestFile);
        UNITY_OUTPUT_CHAR(':');
        UnityPrintNumber((UNITY_INT)lineNumber);
        UNITY_OUTPUT_CHAR(':');
        UnityPrint(Unity.CurrentTestName);
        UnityPrint(":FAIL: Memory mismatch");
        if (msg) { UnityPrint(" : "); UnityPrint(msg); }
        UNITY_OUTPUT_CHAR('\n');
    }
}

void UnityAssertNumbersWithin(const UNITY_UINT delta,
                              const UNITY_INT expected,
                              const UNITY_INT actual,
                              const char* msg,
                              const UNITY_LINE_TYPE lineNumber,
                              const UNITY_DISPLAY_STYLE_T style)
{
    (void)style;
    UNITY_UINT diff = (expected > actual)
                      ? (UNITY_UINT)(expected - actual)
                      : (UNITY_UINT)(actual - expected);
    if (diff > delta)
    {
        Unity.CurrentTestFailed = 1;
        UnityPrint(Unity.TestFile);
        UNITY_OUTPUT_CHAR(':');
        UnityPrintNumber((UNITY_INT)lineNumber);
        UNITY_OUTPUT_CHAR(':');
        UnityPrint(Unity.CurrentTestName);
        UnityPrint(":FAIL: Values not within delta ");
        UnityPrintNumberByFormat((UNITY_INT)delta, style);
        if (msg) { UnityPrint(" : "); UnityPrint(msg); }
        UNITY_OUTPUT_CHAR('\n');
    }
}
