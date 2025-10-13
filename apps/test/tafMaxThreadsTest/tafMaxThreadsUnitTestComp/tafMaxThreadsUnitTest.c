/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

static void* ThreadSleep(void * contextPtr)
{
    uintptr_t ind = (uintptr_t)contextPtr;

    LE_TEST_INFO("[thread %" PRIuPTR "] Started", ind);
    while(1)
    {
        le_thread_Sleep(10000);
    }

    return NULL;
}

static le_thread_Ref_t CreateThreads(uintptr_t index)
{
    le_thread_Ref_t threadRef;
    char thread_name[100];
    snprintf(thread_name, sizeof(thread_name), "tlimit%" PRIuPTR, index);

    LE_TEST_INFO("[thread %" PRIuPTR "] Create", index);

    threadRef = le_thread_Create(thread_name, ThreadSleep, (void*)index);
    if (!threadRef)
    {
        LE_TEST_INFO("Failed to create thread %" PRIuPTR, index);
        return NULL;
    }
    LE_TEST_INFO("MaxThreads ref => %p", threadRef);

    LE_TEST_INFO("[thread %" PRIuPTR "] Start", index);

    le_thread_Start(threadRef);
    return threadRef;
}

COMPONENT_INIT
{
    LE_TEST_INFO("---------- Started testing in:---");
    int index;
    int maxThreads = 1000; // Set a reasonable maximum

    //Create threads as long the telaf system allows or the maximum threads limit reached by the app
    for(index=0; index < maxThreads; index++)
    {
        le_thread_Ref_t threadRef = CreateThreads(index);
        if (!threadRef)
        {
            LE_TEST_INFO("Maximum threads reached: %d", index);
            break;
        }
        LE_TEST_INFO("MaxThreads number => %d", index);
        le_thread_Sleep(2);
    }

    LE_TEST_INFO("---------- All Tests Completed in: ----");
    exit(EXIT_SUCCESS);
}
