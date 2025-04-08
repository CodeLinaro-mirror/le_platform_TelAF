/*
 *  Copyright (c) 2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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
