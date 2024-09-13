/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "diag.h"

COMPONENT_INIT
{
    LE_INFO("%s [start]", __FUNCTION__);
#ifndef DIAG_MULTIVLAN_TEST
    LE_ERROR_IF(diagReadWriteDid_Init() != LE_OK, "diagReadWriteDid_Init -> init failed");
    LE_FATAL_IF(diagSecurityAccess_Init() != LE_OK, "diagSecurityAccess_Init -> init failed");
    LE_FATAL_IF(diagRequestFileTransfer_Init() != LE_OK,
                "diagRequestFileTransfer_Init -> init failed");

#ifndef LE_CONFIG_DIAG_VSTACK
    LE_FATAL_IF(diagReset_Init() != LE_OK, "diagReset_Init -> init failed");
    LE_FATAL_IF(diagRoutineControl_Init() != LE_OK, "diagRoutineControl_Init -> init failed");
    LE_FATAL_IF(diagIOControl_Init() != LE_OK, "diagIOControl_Init -> init failed");
    LE_FATAL_IF(diagDoIP_Init() != LE_OK, "diagDoIP_Init -> init failed");
#endif

#else
// Multi-VLAN test
#ifndef LE_CONFIG_DIAG_VSTACK
    LE_INFO("=====> Enter multi-VLAN sample.");
    LE_FATAL_IF(diagVlanDoIP_Init() != LE_OK, "diagDoIP_Init -> init failed");
    LE_ERROR_IF(diagVlanReadWriteDid_Init() != LE_OK, "diagReadWriteDid_Init -> init failed");
    LE_FATAL_IF(diagVlanSecurityAccess_Init() != LE_OK, "diagSecurityAccess_Init -> init failed");
    LE_FATAL_IF(diagVlanRequestFileTransfer_Init() != LE_OK,
                "diagRequestFileTransfer_Init -> init failed");
    LE_FATAL_IF(diagVlanReset_Init() != LE_OK, "diagReset_Init -> init failed");
    LE_FATAL_IF(diagVlanRoutineControl_Init() != LE_OK, "diagRoutineControl_Init -> init failed");
    LE_FATAL_IF(diagVlanIOControl_Init() != LE_OK, "diagIOControl_Init -> init failed");
#endif
#endif
    LE_INFO("%s [done]", __FUNCTION__);
}
