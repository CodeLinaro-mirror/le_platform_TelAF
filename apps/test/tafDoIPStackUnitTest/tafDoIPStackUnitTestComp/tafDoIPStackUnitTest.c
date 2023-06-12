/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include "tafDoIPStack.h"

#include "tafDoIPStackUnitTest.h"
#include "tafDoIPInterfaceTest.h"
#include "tafDoIPNodeTest.h"

static taf_doip_Ref_t  DoipEntityRef = NULL;

taf_doip_Ref_t CreateDoipEntityRef
(
)
{
    LE_TEST_INFO("=== Create DoIP Entity reference ===");

    DoipEntityRef = taf_doip_Create(DOIP_CONFIG_PATH);
    LE_TEST_OK(DoipEntityRef != NULL, "DoipEntityRef created successfully");

    return DoipEntityRef;
}

static void SignalHandler
(
    int sigNum
)
{
    LE_TEST_INFO("=== telaf doip test END ===");

    if (DoipEntityRef != NULL)
    {
        taf_doip_Stop(DoipEntityRef);
        taf_doip_Delete(DoipEntityRef);
    }

    LE_TEST_EXIT;
}

/*======================================================================
 FUNCTION        COMPONENT_INIT
 DESCRIPTION     Component initialization
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("=== telaf doip test BEGIN ===");
    le_sig_SetEventHandler(SIGTERM, SignalHandler);

    DoipInfTest_InitMain();

    DoipInfTest_DeInitMain();

    DoipInfTest_ReInitMain();

    DoipInfTest_StartMain();

    DoipInfTest_StopMain();

    DoipInfTest_RestartMain();

    DoipNodeTest_DiagMain();  // This test case will enter diagnostic handler loop. put it last.
}