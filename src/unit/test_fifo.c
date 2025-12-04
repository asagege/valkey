/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "../fifo.h"
#include "test_help.h"
#include <sys/wait.h>
#include <unistd.h>

/* Macro to detect if child process crashes as expected */
#define DETECT_CRASH                                                                         \
    pid_t pid = fork();                                                                      \
    if (pid < 0) {                                                                           \
        /* Fork failed */                                                                    \
        TEST_EXPECT(0);                                                                      \
    } else if (pid > 0) {                                                                    \
        /* Parent process - wait for child to crash */                                       \
        int status;                                                                          \
        waitpid(pid, &status, 0);                                                            \
        /* Verify child exited abnormally (assertion failure) */                             \
        TEST_EXPECT(WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status) != 0)); \
    } else /* macro is followed by the else clause */

static inline void *INT_TO_PTR(intptr_t i) {
    return (void *)i;
}

static inline intptr_t PTR_TO_INT(void *p) {
    return (intptr_t)p;
}

/* Helper functions */
static void push(fifo *q, intptr_t value) {
    int len = fifoLength(q);
    fifoPush(q, INT_TO_PTR(value));
    TEST_EXPECT(fifoLength(q) == len + 1);
}

static intptr_t popTest(fifo *q, intptr_t expected) {
    intptr_t peekValue = PTR_TO_INT(fifoPeek(q));
    TEST_EXPECT(peekValue == expected);
    int len = fifoLength(q);
    intptr_t value = PTR_TO_INT(fifoPop(q));
    TEST_EXPECT(fifoLength(q) == len - 1);
    TEST_EXPECT(value == expected);
    return value;
}

/* Test: emptyPop - verify that popping from empty queue triggers assertion */
int test_fifoEmptyPop(int argc, char *argv[], int flags) {
    UNUSED(argc);
    UNUSED(argv);
    UNUSED(flags);

    DETECT_CRASH {
        /* Child process - this should crash with assertion */
        fifo *q = fifoCreate();
        TEST_EXPECT(fifoLength(q) == 0);
        fifoPop(q); /* This should assert and crash */
        /* Should never reach here */
        TEST_ASSERT(0);
    }
    return 0;
}

/* Test: emptyPeek - verify that peeking at empty queue triggers assertion */
int test_fifoEmptyPeek(int argc, char *argv[], int flags) {
    UNUSED(argc);
    UNUSED(argv);
    UNUSED(flags);

    DETECT_CRASH {
        /* Child process - this should crash with assertion */
        fifo *q = fifoCreate();
        TEST_EXPECT(fifoLength(q) == 0);
        fifoPeek(q); /* This should assert and crash */
        /* Should never reach here */
        TEST_ASSERT(0);
    }
    return 0;
}

/* Test: simplePushPop */
int test_fifoSimplePushPop(int argc, char *argv[], int flags) {
    UNUSED(argc);
    UNUSED(argv);
    UNUSED(flags);

    fifo *q = fifoCreate();
    TEST_EXPECT(fifoLength(q) == 0);
    push(q, 1);
    popTest(q, 1);
    TEST_EXPECT(fifoLength(q) == 0);
    fifoDelete(q);
    return 0;
}

/* Test: tryVariousSizes */
int test_fifoTryVariousSizes(int argc, char *argv[], int flags) {
    UNUSED(argc);
    UNUSED(argv);
    UNUSED(flags);

    fifo *q = fifoCreate();
    for (int items = 1; items < 50; items++) {
        TEST_EXPECT(fifoLength(q) == 0);
        for (int value = 1; value <= items; value++) push(q, value);
        for (int value = 1; value <= items; value++) popTest(q, value);
        TEST_EXPECT(fifoLength(q) == 0);
    }
    fifoDelete(q);
    return 0;
}

/* Test: pushPopTest */
int test_fifoPushPopTest(int argc, char *argv[], int flags) {
    UNUSED(argc);
    UNUSED(argv);
    UNUSED(flags);

    fifo *q = fifoCreate();
    /* In this test, we repeatedly push 2 and pop 1. This hits the list differently than
     * other tests which push a bunch and then pop them all off. */
    int pushVal = 1;
    int popVal = 1;
    for (int i = 0; i < 200; i++) {
        if (i % 3 == 0 || i % 3 == 1) {
            push(q, pushVal++);
        } else {
            popTest(q, popVal++);
        }
    }
    fifoDelete(q);
    return 0;
}

/* Test: joinTest */
int test_fifoJoinTest(int argc, char *argv[], int flags) {
    UNUSED(argc);
    UNUSED(argv);
    UNUSED(flags);

    fifo *q = fifoCreate();
    fifo *q2 = fifoCreate();

    /* In this test, there are 2 queues Q and Q2.
     * Various sizes are tested. For each size, various amounts are popped off the front first.
     * Q2 is appended to Q. */
    for (int qLen = 0; qLen <= 21; qLen++) {
        for (int qPop = 0; qPop < 6 && qPop <= qLen; qPop++) {
            for (int q2Len = 0; q2Len <= 21; q2Len++) {
                for (int q2Pop = 0; q2Pop < 6 && q2Pop <= q2Len; q2Pop++) {
                    intptr_t pushValue = 1;
                    intptr_t popQValue = 1;

                    for (int i = 0; i < qLen; i++) fifoPush(q, INT_TO_PTR(pushValue++));
                    for (int i = 0; i < qPop; i++) TEST_EXPECT(PTR_TO_INT(fifoPop(q)) == popQValue++);

                    intptr_t popQ2Value = pushValue;
                    for (int i = 0; i < q2Len; i++) fifoPush(q2, INT_TO_PTR(pushValue++));
                    for (int i = 0; i < q2Pop; i++) TEST_EXPECT(PTR_TO_INT(fifoPop(q2)) == popQ2Value++);

                    fifoJoin(q, q2);
                    TEST_EXPECT(fifoLength(q) == (qLen - qPop) + (q2Len - q2Pop));
                    TEST_EXPECT(fifoLength(q2) == 0);

                    fifo *temp = fifoPopAll(q); /* Exercise fifoPopAll also */
                    TEST_EXPECT(fifoLength(temp) == (qLen - qPop) + (q2Len - q2Pop));
                    TEST_EXPECT(fifoLength(q) == 0);

                    for (int i = 0; i < (qLen - qPop); i++) TEST_EXPECT(PTR_TO_INT(fifoPop(temp)) == popQValue++);
                    for (int i = 0; i < (q2Len - q2Pop); i++) TEST_EXPECT(PTR_TO_INT(fifoPop(temp)) == popQ2Value++);
                    TEST_EXPECT(fifoLength(temp) == 0);

                    fifoDelete(temp);
                }
            }
        }
    }

    fifoDelete(q);
    fifoDelete(q2);
    return 0;
}

/* Set to 1 to prove that FIFO outperforms ADLIST. This test will exercise both.
 * The test will (intentionally) fail, printing the results as failed assertions. */
#define COMPARE_PERFORMANCE_TO_ADLIST 0

#include "../adlist.h"
#include "../monotonic.h"

const int LIST_ITEMS = 10000;

static void exerciseList(void) {
    list *q = listCreate();
    for (intptr_t i = 0; i < LIST_ITEMS; i++) {
        listAddNodeTail(q, INT_TO_PTR(i));
    }
    TEST_EXPECT(listLength(q) == (unsigned)LIST_ITEMS);
    for (intptr_t i = 0; i < LIST_ITEMS; i++) {
        listNode *node = listFirst(q);
        listDelNode(q, node);
        TEST_EXPECT(listNodeValue(node) == INT_TO_PTR(i));
    }
    TEST_EXPECT(listLength(q) == 0u);
    listRelease(q);
}

static void exerciseFifo(void) {
    fifo *q = fifoCreate();
    for (intptr_t i = 0; i < LIST_ITEMS; i++) {
        fifoPush(q, INT_TO_PTR(i));
    }
    TEST_EXPECT(fifoLength(q) == LIST_ITEMS);
    for (intptr_t i = 0; i < LIST_ITEMS; i++) {
        TEST_EXPECT(fifoPop(q) == INT_TO_PTR(i));
    }
    TEST_EXPECT(fifoLength(q) == 0);
    fifoDelete(q);
}

int test_fifoComparePerformance(int argc, char *argv[], int flags) {
    UNUSED(argc);
    UNUSED(argv);
    UNUSED(flags);

    if (COMPARE_PERFORMANCE_TO_ADLIST) {
        monotonicInit();
        monotime timer;
        const int iterations = 500;

        exerciseList(); /* Warm up the list before timing */
        elapsedStart(&timer);
        for (int i = 0; i < iterations; i++) exerciseList();
        long listMs = elapsedMs(timer);

        exerciseFifo(); /* Warm up the fifo before timing */
        elapsedStart(&timer);
        for (int i = 0; i < iterations; i++) exerciseFifo();
        long fifoMs = elapsedMs(timer);

        TEST_EXPECT(listMs == fifoMs); /* This will fail, printing result */
        double percentImprovement = (double)(listMs - fifoMs) * 100.0 / listMs;
        TEST_PRINT_INFO("List: %ld ms, FIFO: %ld ms, Improvement: %.2f%%", listMs, fifoMs, percentImprovement);
        TEST_EXPECT(percentImprovement == 0.0); /* This will fail, printing result */
    }

    return 0;
}
