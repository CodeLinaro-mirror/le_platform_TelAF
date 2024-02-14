/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafDevInfo.hpp"

using namespace telux::tafsvc;

COMPONENT_INIT {
    LE_INFO("tafDevInfo Service Init...\n");
    auto& tafDevInfo = taf_info::GetInstance();
    tafDevInfo.Init();
    LE_INFO("tafDevInfo Service Ready...\n");
}

/*======================================================================
 FUNCTION        taf_info_GetImei
 DESCRIPTION     Get IMEI of the device
 PARAMETERS      [OUT]  imeiPtr: Firmware version
                 [IN]   numElements: IMEI size in bytes
 RETURN VALUE    void
======================================================================*/
le_result_t taf_info_GetImei(char* imeiPtr, size_t numElements) {
    TAF_ERROR_IF_RET_VAL(imeiPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(imeiPtr)");
    auto& tafDevInfo = taf_info::GetInstance();
    return tafDevInfo.GetIMEI(imeiPtr, numElements);
}

/*======================================================================
 FUNCTION        taf_info_GetModel
 DESCRIPTION     Get Model of the device
 PARAMETERS      [OUT]  modelPtr
                 [IN]   numElements: Device Model size in bytes
 RETURN VALUE    void
======================================================================*/
le_result_t taf_info_GetModel(char* modelPtr, size_t numElements) {
    auto& tafDevInfo = taf_info::GetInstance();
    return tafDevInfo.GetDeviceModel(modelPtr, numElements);
}

/*======================================================================
 FUNCTION        taf_info_GetKernelVersion
 DESCRIPTION     Get Kernel Version along with Boot version of device
 PARAMETERS      [OUT]  versionPtr
                 [IN]   numElements: Kernel and Boot version size in bytes
 RETURN VALUE    void
======================================================================*/
le_result_t taf_info_GetKernelVersion(char* versionPtr, size_t numElements) {
    auto& tafDevInfo = taf_info::GetInstance();
    return tafDevInfo.GetKernelVersion(versionPtr, numElements);
}

/*======================================================================
 FUNCTION        taf_info_GetModemVersion
 DESCRIPTION     Get Modem Version of the device
 PARAMETERS      [OUT]  modemPtr
                 [IN]   numElements: Modem version size in bytes
 RETURN VALUE    void
======================================================================*/
le_result_t taf_info_GetModemVersion(char* modemPtr, size_t numElements) {
    auto& tafDevInfo = taf_info::GetInstance();
    return tafDevInfo.GetModemVersion(modemPtr, numElements);
}

/*======================================================================
 FUNCTION        taf_info_GetTZVersion
 DESCRIPTION     Get TZ Version of the device
 PARAMETERS      [OUT]  tzPtr
                 [IN]   numElements: TZ version size in bytes
 RETURN VALUE    void
======================================================================*/
le_result_t taf_info_GetTZVersion(char* tzPtr, size_t numElements) {
    auto& tafDevInfo = taf_info::GetInstance();
    return tafDevInfo.GetTzVersion(tzPtr, numElements);
}
