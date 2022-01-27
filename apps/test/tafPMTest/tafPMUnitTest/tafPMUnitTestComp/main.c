/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "main.h"

void DisplayAppUsage(void) {
    printf("Usage of the 'tafPMUnitTest' application is:\n");
    printf("Please follow the instructions mentioned with ACTION\n");
    printf("Test State change listener: app runProc tafPMUnitTest --exe=tafPMUnitTest -- test1\n");
    printf("Test remove state change listener: app runProc tafPMUnitTest --exe=tafPMUnitTest -- test2\n");
    printf("Suspend testcase when wakelock is acquired, and observe device will not suspend when wakelock is acquired\n"
            "\t: app runProc tafPMUnitTest --exe=tafPMUnitTest -- test3\n");
    printf("Remote proc suspend testcase when wakelock is acquired, and observe device will not suspend when wakelock is acquired\n"
            "\t: app runProc tafPMUnitTest --exe=tafPMUnitTest -- test4\n");
    printf("Suspend test case when WL is released, device suspends when no wakelock is held\n"
            "\t: app runProc tafPMUnitTest --exe=tafPMUnitTest -- test5\n");
    printf("Suspend test case when app exits with wakelock acquired\n"
            "\t: app runProc tafPMUnitTest --exe=tafPMUnitTest -- test6\n");
    printf("Test getState : app runProc tafPMUnitTest --exe=tafPMUnitTest -- test7\n");
    printf("Test acquire and release multiple times WL with reference : app runProc tafPMUnitTest --exe=tafPMUnitTest -- test8\n");
}

COMPONENT_INIT
{
    bool exitApplication = false;
    const char* testType = "";

    int NumberOfArgs = le_arg_NumArgs();
    if (NumberOfArgs >= 1) {
        testType = le_arg_GetArg(0);
        if (NULL == testType) {
            LE_ERROR("testType is NULL");
            DisplayAppUsage();
            exit(EXIT_FAILURE);
        }
    }

    if(strcmp(testType,"test1") == 0) {
        tafPMTest_registerStateChangeListener();
    } else if(strcmp(testType,"test2") == 0) {
        tafPMTest_deregisterListenerTest();
    } else if(strcmp(testType,"test3") == 0) {
        tafPMTest_test3();
    } else if(strcmp(testType,"test4") == 0) {
        tafPMTest_test4();
    } else if(strcmp(testType,"test5") == 0) {
        tafPMTest_test5();
    } else if(strcmp(testType,"test6") == 0) {
        tafPMTest_test6();
        exitApplication = true;
        exit(EXIT_SUCCESS);
    } else if(strcmp(testType,"test7") == 0) {
        tafPMTest_getState();
        exitApplication = true;
    } else if(strcmp(testType,"test8") == 0) {
        tafPMTest_test8();
        exitApplication = true;
    } else {
        DisplayAppUsage();
        exit(EXIT_FAILURE);
    }

    if (exitApplication)
    {
        LE_INFO("Exit tafPMUnitTest App");
        printf("\nApplication exits\n");
        exit(EXIT_SUCCESS);
    }
}
