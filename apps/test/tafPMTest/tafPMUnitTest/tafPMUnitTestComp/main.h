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

#include "legato.h"
#include "interfaces.h"

taf_pm_StateChangeHandlerRef_t StateChangeRefHandler;
char* tafStateToString(taf_pm_State_t tafState);
void TestStateChangeHandler(taf_pm_State_t state, void* contextPtr);
taf_pm_WakeupSourceRef_t tafPowerMgrTest_create(const char* tag);
void tafPMTest_acquire(taf_pm_WakeupSourceRef_t ref);
void tafPMTest_release(taf_pm_WakeupSourceRef_t ref);
void tafPMTest_getState();
void tafPMTest_registerStateChangeListener();
void tafPMTest_deregisterStateChangeListener();
void tafPMTest_deregisterListenerTest();
/*
 * To test suspend testcase when wakelock is acquired, and observe
 * device will not suspend when wakelock is acquired
 */
void tafPMTest_test3();
/*
 * To test remote proc suspend testcase when wakelock is acquired, and
 * observe device will not suspend when wakelock is acquired
 */
void tafPMTest_test4();
/*
 * To test suspend test case when WL is released, device suspends when no wakelock is held
 */
void tafPMTest_test5();
/*
 * To test suspend test case when app exits with wakelock acquired
 */
void tafPMTest_test6();
/*
 * To test acquire and release multiple times WL with reference
 */
void tafPMTest_test8();
