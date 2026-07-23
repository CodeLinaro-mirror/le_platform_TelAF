/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafDevInfo.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * SIGTERM signal event handler.
 *
 * Invoked by the Legato signal event framework when the process receives SIGTERM.
 */
//--------------------------------------------------------------------------------------------------
static void SigTermEventHandler
(
    int sigNum ///< [IN] Signal number received (expected: SIGTERM).
)
{
    LE_INFO("SigTermEventHandler signal : %d", sigNum);

#ifdef LE_CONFIG_GET_IMEI_SUPPORT
    taf_pa_result_t result = taf_pa_deviceinfo_Deinit();
    if (result != TAF_PA_OK)
        LE_ERROR("taf_pa_deviceinfo_Deinit failed, result: %d", result);
    else
        LE_INFO("taf_pa_deviceinfo_Deinit succeeded.");
#endif

    exit(EXIT_SUCCESS);
}

COMPONENT_INIT {
    LE_INFO("tafDevInfo Service Init...\n");
    auto& tafDevInfo = taf_devInfo::GetInstance();
    tafDevInfo.Init();
    LE_INFO("tafDevInfo Service Ready...\n");

    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, SigTermEventHandler);
}

/*======================================================================
 FUNCTION        taf_devInfo_GetImei
 DESCRIPTION     Get IMEI of the device
 PARAMETERS      [OUT]  imeiPtr: Firmware version
                 [IN]   numElements: IMEI size in bytes
 RETURN VALUE    void
======================================================================*/
le_result_t taf_devInfo_GetImei(char* imeiPtr, size_t numElements) {
    TAF_ERROR_IF_RET_VAL(imeiPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(imeiPtr)");
#ifdef LE_CONFIG_GET_IMEI_SUPPORT
    auto& tafDevInfo = taf_devInfo::GetInstance();
    return tafDevInfo.GetIMEI(imeiPtr, numElements);
#endif
    return LE_UNSUPPORTED;
}

/*======================================================================
 FUNCTION        taf_devInfo_GetModel
 DESCRIPTION     Get Model of the device
 PARAMETERS      [OUT]  modelPtr
                 [IN]   numElements: Device Model size in bytes
 RETURN VALUE    void
======================================================================*/
le_result_t taf_devInfo_GetModel(char* modelPtr, size_t numElements) {
    auto& tafDevInfo = taf_devInfo::GetInstance();
    return tafDevInfo.GetDeviceModel(modelPtr, numElements);
}
