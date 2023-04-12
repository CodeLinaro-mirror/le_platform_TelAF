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

#ifndef TAF_PA_RADIO_HPP
#define TAF_PA_RADIO_HPP

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Get register mode
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetRegisterMode
(
    bool* isManualPtr,
    char* mccPtr,
    size_t mccPtrSize,
    char* mncPtr,
    size_t mncPtrSize,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Get platform specific registration error code
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int32_t taf_pa_radio_GetPlatformSpecificRegistrationErrorCode
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get network registration state
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetNetRegState
(
    taf_radio_NetRegState_t* statePtr,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Get packet switched state
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetPacketSwitchedState
(
    taf_radio_NetRegState_t* statePtr,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for network registration state
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_NetRegStateEventHandlerRef_t taf_pa_radio_AddNetRegStateEventHandler
(
    taf_radio_NetRegStateHandlerFunc_t handlerFuncPtr,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for network registration state
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_radio_RemoveNetRegStateEventHandler
(
    taf_radio_NetRegStateEventHandlerRef_t handlerRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for packet switched state
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_PacketSwitchedChangeHandlerRef_t taf_pa_radio_AddPacketSwitchedChangeHandler
(
    taf_radio_PacketSwitchedChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for packet switched state
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_radio_RemovePacketSwitchedChangeHandler
(
    taf_radio_PacketSwitchedChangeHandlerRef_t handlerRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for network registration rejection
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_NetRegRejectHandlerRef_t taf_pa_radio_AddNetRegRejectHandler
(
    taf_radio_NetRegRejectHandlerFunc_t handlerFuncPtr,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for network registration rejection
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_radio_RemoveNetRegRejectHandler
(
    taf_radio_NetRegRejectHandlerRef_t handlerRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for radio access technology change
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_RatChangeHandlerRef_t taf_pa_radio_AddRatChangeHandler
(
    taf_radio_RatChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for radio access technology change
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_radio_RemoveRatChangeHandler
(
    taf_radio_RatChangeHandlerRef_t handlerRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get radio access technology in use
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetRadioAccessTechInUse
(
    taf_radio_Rat_t* ratPtr,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for signal strength change
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_SignalStrengthChangeHandlerRef_t taf_pa_radio_AddSignalStrengthChangeHandler
(
    taf_radio_Rat_t rat,
    taf_radio_SignalStrengthChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for signal strength change
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void taf_pa_radio_RemoveSignalStrengthChangeHandler
(
    taf_radio_SignalStrengthChangeHandlerRef_t handlerRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get current network name
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetCurrentNetworkName
(
    char* nameStr,
    size_t nameStrSize,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Get current network Mobile Country Code and Mobile Network Code
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetCurrentNetworkMccMnc
(
    char* mccStr,
    size_t mccStrNumElements,
    char* mncStr,
    size_t mncStrNumElements,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Enable indication
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_EnableIndication
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Disable indication
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_DisableIndication
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get 2G/3G band capabilities
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetBandCapabilities
(
    taf_radio_BandBitMask_t* bandMaskPtr,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Get LTE band capabilities
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetLteBandCapabilities
(
    uint64_t* bandMaskPtr,
    size_t* bandMaskSizePtr,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Set 2G/3G band preferences
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_SetBandPreferences
(
    taf_radio_BandBitMask_t bandMask,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Get 2G/3G band preferences
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetBandPreferences
(
    taf_radio_BandBitMask_t* bandMaskPtr,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Set LTE band preferences
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_SetLteBandPreferences
(
    const uint64_t* bandMask,
    size_t bandMaskSize,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Get LTE band preferences
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetLteBandPreferences
(
    uint64_t* bandMaskPtr,
    size_t* bandMaskSizePtr,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Set signal strength indication thresholds
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_SetSignalStrengthIndThresholds
(
    taf_radio_SigType_t sigType,
    int32_t lowerRangeThreshold,
    int32_t upperRangeThreshold,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Set signal strength indication delta
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_SetSignalStrengthIndDelta
(
    taf_radio_SigType_t sigType,
    uint16_t delta,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Perform network scan with Pysical Cell ID
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_PciScanInformationListRef_t taf_pa_radio_PerformPciNetworkScan
(
    taf_radio_RatBitMask_t ratMask,
    uint8_t phoneId
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the first PCI network scan information reference
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_PciScanInformationRef_t taf_pa_radio_GetFirstPciScanInfo
(
    taf_radio_PciScanInformationListRef_t pciScanInformationListRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the next PCI network scan information reference
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_PciScanInformationRef_t taf_pa_radio_GetNextPciScanInfo
(
    taf_radio_PciScanInformationListRef_t pciScanInformationListRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the first PLMN network information reference
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_PlmnInformationRef_t taf_pa_radio_GetFirstPlmnInfo
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the next PLMN network information reference
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED taf_radio_PlmnInformationRef_t taf_pa_radio_GetNextPlmnInfo
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get PCI network scan Cell ID
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint16_t taf_pa_radio_GetPciScanCellId
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get PCI network scan Global Cell ID
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint32_t taf_pa_radio_GetPciScanGlobalCellId
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get MCC and MNC of PLMN information from PCI network scan
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_GetPciScanMccMnc
(
    taf_radio_PlmnInformationRef_t plmnRef,
    char* mccPtr,
    size_t mccPtrSize,
    char* mncPtr,
    size_t mncPtrSize
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete PCI network scan
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_pa_radio_DeletePciNetworkScan
(
    taf_radio_PciScanInformationListRef_t scanInformationListRef
);

#endif /* TAF_PA_RADIO_HPP */
