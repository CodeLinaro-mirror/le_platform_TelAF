/*
 *  Copyright (c) 2022, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafMrc.hpp"

using namespace tafsvc;

COMPONENT_INIT
{
    LE_INFO("tafMrc Service Init...\n");
    auto &tafMrc = taf_Mrc::GetInstance();
    tafMrc.Init();
    LE_INFO("tafMrc Service Ready...\n");
}

le_result_t taf_mrc_SendOtaStartMsg()
{
    auto &tafMrc = taf_Mrc::GetInstance();
    return tafMrc.SendOtaMsg(TAF_MRC_OTA_MSG_TYPE_START);
}

le_result_t taf_mrc_SendOtaResumeMsg()
{
    auto &tafMrc = taf_Mrc::GetInstance();
    return tafMrc.SendOtaMsg(TAF_MRC_OTA_MSG_TYPE_RESUME);
}

le_result_t taf_mrc_SendOtaEndMsg(taf_mrc_OtaOperationStatus_t otaStatus)
{
    auto &tafMrc = taf_Mrc::GetInstance();
    if (otaStatus == TAF_MRC_OTA_OP_STATUS_SUCCESS) {
        return tafMrc.SendOtaMsg(TAF_MRC_OTA_MSG_TYPE_END_SUCCESS);
    } else if (otaStatus == TAF_MRC_OTA_OP_STATUS_FAILURE) {
        return tafMrc.SendOtaMsg(TAF_MRC_OTA_MSG_TYPE_END_FAILURE);
    }

    return LE_BAD_PARAMETER;
}

le_result_t taf_mrc_SendOtaAbsyncMsg()
{
    auto &tafMrc = taf_Mrc::GetInstance();
    return tafMrc.SendOtaMsg(TAF_MRC_OTA_MSG_TYPE_ABSYNC);
}
