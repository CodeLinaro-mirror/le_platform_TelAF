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
#include "interfaces.h"

#include "taf_pa_radio.hpp"

//--------------------------------------------------------------------------------------------------
/**
 * Get register mode
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetRegisterMode
(
    bool* isManualPtr,
    char* mccPtr,
    size_t mccPtrSize,
    char* mncPtr,
    size_t mncPtrSize,
    uint8_t phoneId
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get platform specific registration error code
 */
//--------------------------------------------------------------------------------------------------
int32_t taf_pa_radio_GetPlatformSpecificRegistrationErrorCode
(
    void
)
{
    return INT32_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for network registration rejection
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NetRegRejectHandlerRef_t taf_pa_radio_AddNetRegRejectHandler
(
    taf_radio_NetRegRejectHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for network registration rejection
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_radio_RemoveNetRegRejectHandler
(
    taf_radio_NetRegRejectHandlerRef_t handlerRef
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for radio access technology change
 */
//--------------------------------------------------------------------------------------------------
taf_radio_RatChangeHandlerRef_t taf_pa_radio_AddRatChangeHandler
(
    taf_radio_RatChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for radio access technology change
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_radio_RemoveRatChangeHandler
(
    taf_radio_RatChangeHandlerRef_t handlerRef
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get radio access technology in use
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetRadioAccessTechInUse
(
    taf_radio_Rat_t* ratPtr,
    uint8_t phoneId
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get current network name
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetCurrentNetworkName
(
    char* nameStr,
    size_t nameStrSize,
    uint8_t phoneId
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get current network Mobile Country Code and Mobile Network Code
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetCurrentNetworkMccMnc
(
    char* mccStr,
    size_t mccStrNumElements,
    char* mncStr,
    size_t mncStrNumElements,
    uint8_t phoneId
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable indication
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_EnableIndication
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable indication
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_DisableIndication
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get 2G/3G band capabilities
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetBandCapabilities
(
    taf_radio_BandBitMask_t* bandMaskPtr,
    uint8_t phoneId
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get LTE band capabilities
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetLteBandCapabilities
(
    uint64_t* bandMaskPtr,
    size_t* bandMaskSizePtr,
    uint8_t phoneId
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set 2G/3G band preferences
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_SetBandPreferences
(
    taf_radio_BandBitMask_t bandMask,
    uint8_t phoneId
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get 2G/3G band preferences
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetBandPreferences
(
    taf_radio_BandBitMask_t* bandMaskPtr,
    uint8_t phoneId
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set LTE band preferences
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_SetLteBandPreferences
(
    const uint64_t* bandMask,
    size_t bandMaskSize,
    uint8_t phoneId
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get LTE band preferences
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetLteBandPreferences
(
    uint64_t* bandMaskPtr,
    size_t* bandMaskSizePtr,
    uint8_t phoneId
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Perform network scan with Pysical Cell ID
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationListRef_t taf_pa_radio_PerformPciNetworkScan
(
    taf_radio_RatBitMask_t ratMask,
    uint8_t phoneId
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the first PCI network scan information reference
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationRef_t taf_pa_radio_GetFirstPciScanInfo
(
    taf_radio_PciScanInformationListRef_t pciScanInformationListRef
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next PCI network scan information reference
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationRef_t taf_pa_radio_GetNextPciScanInfo
(
    taf_radio_PciScanInformationListRef_t pciScanInformationListRef
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the first PLMN network information reference
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PlmnInformationRef_t taf_pa_radio_GetFirstPlmnInfo
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next PLMN network information reference
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PlmnInformationRef_t taf_pa_radio_GetNextPlmnInfo
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get PCI network scan Cell ID
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_pa_radio_GetPciScanCellId
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
)
{
    return UINT16_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get PCI network scan Global Cell ID
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_pa_radio_GetPciScanGlobalCellId
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
)
{
    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get MCC and MNC of PLMN information from PCI network scan
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetPciScanMccMnc
(
    taf_radio_PlmnInformationRef_t plmnRef,
    char* mccPtr,
    size_t mccPtrSize,
    char* mncPtr,
    size_t mncPtrSize
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete PCI network scan
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_DeletePciNetworkScan
(
    taf_radio_PciScanInformationListRef_t pciScanInformationListRef
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set reference.
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_radio_SetReference
(
    uint8_t phoneId,
    taf_radio_NetStatusRef_t netStatusRef
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for network status.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NetStatusChangeHandlerRef_t taf_pa_radio_AddNetStatusChangeHandler
(
    taf_radio_NetStatusHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for network status.
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_radio_RemoveNetStatusChangeHandler
(
    taf_radio_NetStatusChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Get LTE CS capabilitiy.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetLteCsCap
(
    uint8_t phoneId,
    taf_radio_CsCap_t* capabilitiy
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Get RAT service status.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pa_radio_GetRatSvcStatus
(
    uint8_t phoneId,
    taf_radio_RatSvcStatus_t* status
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Init this component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("taf_pa_radio stub");
}
