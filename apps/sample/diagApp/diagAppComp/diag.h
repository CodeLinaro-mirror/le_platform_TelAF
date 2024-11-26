/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DIAG_H
#define DIAG_H

#include "diagPrivate.h"

le_result_t diagReadWriteDid_Init(void);
le_result_t diagSecurityAccess_Init(void);
le_result_t diagRequestFileTransfer_Init(void);
le_result_t diagReset_Init(void);
le_result_t diagRoutineControl_Init(void);
le_result_t diagIOControl_Init(void);
le_result_t diagCommon_Init(void);
le_result_t diagAuth_Init(void);

#ifndef LE_CONFIG_DIAG_VSTACK
le_result_t diagDoIP_Init(void);

// Multi-Vlan sample code
le_result_t diagVlanReadWriteDid_Init(void);
le_result_t diagVlanSecurityAccess_Init(void);
le_result_t diagVlanRequestFileTransfer_Init(void);
le_result_t diagVlanReset_Init(void);
le_result_t diagVlanRoutineControl_Init(void);
le_result_t diagVlanIOControl_Init(void);
le_result_t diagVlanDoIP_Init(void);
le_result_t diagVlanAuth_Init(void);
le_result_t diagVlanDiag_Init(void);

#endif

#endif /* DIAG_H */
