/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafDiagSvc.cpp
 * @brief      This file provides the telaf enbale condition service as interfaces described in
 *             taf_diag.api. The Diag service will be started automatically.
 */

#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include "tafDiagSvr.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Sets an enable condition.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diag_SetEnableCondition
(
    uint8_t enableConditionID,
        ///< [IN] Enable condition ID.
    bool conditionFulfilled
        ///< [IN] Enable condition status.
)
{
    LE_DEBUG("taf_diag_SetEnableCondition");
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.SetEnableCondition(enableConditionID, conditionFulfilled);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets an enable condition status.
 *
 * @return
 *     - TRUE -- Succeeded.
 *     - FALSE -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
bool taf_diag_GetEnableConditionStatus
(
    uint8_t enableConditionID
        ///< [IN] Enable condition ID.
)
{
    LE_DEBUG("taf_diag_GetEnableCondition");
    auto &diag = taf_DiagSvr::GetInstance();
    return diag.GetEnableConditionStatus(enableConditionID);
}
