/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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