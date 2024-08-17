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

le_result_t taf_pa_radio_GetServingCellRoutingAreaCode
(
    uint8_t* rac,   ///< [OUT] Routing area code.
    uint8_t phoneId ///< [IN] Phone id.
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
