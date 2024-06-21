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

static taf_doip_Ref_t  DoipEntityRef = NULL;

static le_result_t DoipServerInit()
{
    if (DoipEntityRef == NULL)
    {
        DoipEntityRef = CreateDoipEntityRef();
    }

    if (taf_doip_SetVin(DOIP_TEST_VIN) != LE_OK)
    {
        LE_ERROR("Failed to set vin");
        return LE_FAULT;
    }

    return LE_OK;
}

static le_result_t DoipServerDeInit()
{
    le_result_t ret;


    if (DoipEntityRef != NULL)
    {
        LE_INFO("DoIP Server is initialized.");
    }
    else
    {
        DoipServerInit();
        LE_INFO("DoIP Server is initialized.");
    }

    ret = taf_doip_Delete(DoipEntityRef);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to delete doip Entity(%d)", ret);
        return LE_FAULT;
    }
    DoipEntityRef = NULL;

    return LE_OK;
}

static le_result_t DoipServerStart()
{
    le_result_t ret;

    if (DoipEntityRef != NULL)
    {
        LE_INFO("DoIP Server is initialized.");
    }
    else
    {
        DoipServerInit();
        LE_INFO("DoIP Server is initialized.");
    }

    ret = taf_doip_Start(DoipEntityRef);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to start doip Entity(%d)", ret);
        return LE_FAULT;
    }

    return LE_OK;
}

static le_result_t DoipServerStop()
{
    le_result_t   ret;

    if (DoipEntityRef != NULL)
    {
        LE_INFO("DoIP Server is already initialized.");
    }
    else
    {
        DoipServerInit();
        LE_INFO("DoIP Server is initialized.");
    }

    ret = taf_doip_Stop(DoipEntityRef);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to stop doip Entity(%d)", ret);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Tests for DoIP APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void DoipInfTest_InitMain
(
    void
)
{
    LE_TEST_INFO("DoIP Init test");
    le_result_t ret;

    LE_INFO("DoipInfTest_InitMain Enter...");
    ret = DoipServerInit();
    LE_TEST_ASSERT(ret == LE_OK, "Test DoIP initialization");

    LE_INFO("DoipInfTest_InitMain Exit...");
}

__attribute__((unused)) void DoipInfTest_DeInitMain
(
    void
)
{
    LE_TEST_INFO("DoIP DeInit test");
    le_result_t ret;

    LE_INFO("DoipInfTest_DeInitMain Enter...");

    ret = DoipServerDeInit();
    LE_TEST_ASSERT(ret == LE_OK, "Test DoIP uninitialization");

    LE_INFO("DoipInfTest_DeInitMain Exit...");
}

__attribute__((unused)) void DoipInfTest_ReInitMain
(
    void
)
{
    LE_TEST_INFO("DoIP ReInit test");
    le_result_t ret;

    ret = DoipServerInit();
    LE_TEST_ASSERT(ret == LE_OK, "Test DoIP initialization");

    ret = DoipServerDeInit();
    LE_TEST_ASSERT(ret == LE_OK, "Test DoIP uninitialization");

    ret = DoipServerInit();
    LE_TEST_ASSERT(ret == LE_OK, "Test DoIP initialization");

    LE_INFO("DoipInfTest_ReInitMain Exit...");
}

__attribute__((unused)) void DoipInfTest_StartMain
(
    void
)
{
    LE_TEST_INFO("DoIP Start test");
    le_result_t ret;

    ret = DoipServerStart();
    LE_TEST_ASSERT(ret == LE_OK, "Test DoIP start");

    LE_INFO("DoipInfTest_StartMain Exit...");
}

__attribute__((unused)) void DoipInfTest_StopMain
(
    void
)
{
    LE_TEST_INFO("Doip Stopt test");
    le_result_t ret;

    ret = DoipServerStart();
    LE_TEST_ASSERT(ret == LE_OK, "Test DoIP start");

    ret = DoipServerStop();
    LE_TEST_ASSERT(ret == LE_OK, "Test DoIP stop");

    LE_INFO("DoipInfTest_StopMain Exit...");
}

__attribute__((unused)) void DoipInfTest_RestartMain
(
    void
)
{
    LE_TEST_INFO("Doip Restart test");
    le_result_t ret;

    ret = DoipServerStart();
    LE_TEST_ASSERT(ret == LE_OK, "Test DoIP start");

    ret = DoipServerStop();
    LE_TEST_ASSERT(ret == LE_OK, "Test DoIP stop");

    ret = DoipServerStart();
    LE_TEST_ASSERT(ret == LE_OK, "Test DoIP start");

    LE_INFO("DoipInfTest_RestartMain Exit...");
}