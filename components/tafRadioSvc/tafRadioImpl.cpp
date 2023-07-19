/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*  Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @file       tafRadioImpl.cpp
 * @brief      This file describes the implementation method that radio
 *             service is in use.
 */

#include <cstdlib>
#include <chrono>

#include "tafRadio.hpp"
#include "taf_pa_radio.hpp"

using namespace std;
using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Listener for network scan results.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioNetworkSelectionListener::onNetworkScanResults
(
    telux::tel::NetworkScanStatus scanStatus,           ///< [IN] Scan status.
    std::vector<telux::tel::OperatorInfo> operatorInfos ///< [IN] Operator information.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioNetworkSelectionListener --> onNetworkScanResults");
    if (scanStatus == telux::tel::NetworkScanStatus::FAILED)
    {
        LE_ERROR("Network scan failed.");
        le_sem_Post(semaphore);
    }
    else
    {
        for (auto info : operatorInfos)
        {
            opInfos.emplace_back(info);
        }

        if (scanStatus == telux::tel::NetworkScanStatus::COMPLETE)
        {
            LE_INFO("Network scan completed.");
            le_sem_Post(semaphore);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initiate listener for IMS serving system.
 */
//--------------------------------------------------------------------------------------------------
taf_RadioImsServSysListener::taf_RadioImsServSysListener
(
    SlotId slotId ///< [IN] Slot ID.
) : slot(slotId)
{
    auto &tafRadio = taf_Radio::GetInstance();
    if (tafRadio.phoneManager != nullptr)
    {
        phone = (uint8_t)(tafRadio.phoneManager->getPhoneIdFromSlotId((int)slotId));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for IMS registration status.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsServSysListener::onImsRegStatusChange
(
    telux::tel::ImsRegistrationInfo status ///< [IN] IMS registration status
)
{
    auto &tafRadio = taf_Radio::GetInstance();

    switch (status.imsRegStatus)
    {
        case telux::tel::RegistrationStatus::NOT_REGISTERED:
        {
            taf_RadioImsRegStatus_t* statusPtr =
               (taf_RadioImsRegStatus_t*)le_mem_ForceAlloc(tafRadio.imsRegStatusChangePool);
            statusPtr->status = TAF_RADIO_IMS_REG_STATUS_NOT_REGISTERED;
            statusPtr->phoneId = phone;
            le_event_ReportWithRefCounting(tafRadio.imsRegStatusChangeId, (void*)statusPtr);
            break;
        }
        case telux::tel::RegistrationStatus::REGISTERED:
        case telux::tel::RegistrationStatus::LIMITED_REGISTERED:
        {
            taf_RadioImsRegStatus_t* statusPtr =
               (taf_RadioImsRegStatus_t*)le_mem_ForceAlloc(tafRadio.imsRegStatusChangePool);
            statusPtr->status = TAF_RADIO_IMS_REG_STATUS_REGISTERED;
            statusPtr->phoneId = phone;
            le_event_ReportWithRefCounting(tafRadio.imsRegStatusChangeId, (void*)statusPtr);
            break;
        }
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for setting operating mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioSetOperatingModeCallback::setOperatingModeResponse
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioOperatingModeCallback --> setOperatingModeResponse");
    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting operating mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioGetOperatingModeCallback::operatingModeResponse
(
    telux::tel::OperatingMode operatingMode, ///< [IN] Operating mode.
    telux::common::ErrorCode error           ///< [IN] Error code.
)
{
    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        opMode = operatingMode;
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting signal strength.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioSignalStrengthCallback::signalStrengthResponse
(
    std::shared_ptr<telux::tel::SignalStrength> signalStrength, ///< [IN] Signal strength.
    telux::common::ErrorCode error                              ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioSignalStrengthCallback --> signalStrengthResponse");

    ssMetrics.ratMask = 0x0;

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        result = LE_OK;
    }

    if (signalStrength->getGsmSignalStrength() != nullptr &&
        signalStrength->getGsmSignalStrength()->getGsmSignalStrength() !=
        INVALID_SIGNAL_STRENGTH_VALUE)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_GSM;
        ssMetrics.gsm.sslv = (int8_t)signalStrength->getGsmSignalStrength()->getLevel() + 1;
        ssMetrics.gsm.ss = signalStrength->getGsmSignalStrength()->getDbm();
        ssMetrics.gsm.ber = signalStrength->getGsmSignalStrength()->getGsmBitErrorRate();
    }

    if (signalStrength->getCdmaSignalStrength() != nullptr &&
        signalStrength->getCdmaSignalStrength()->getDbm() != INVALID_SIGNAL_STRENGTH_VALUE)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_CDMA;
        ssMetrics.cdma.sslv = (int8_t)signalStrength->getCdmaSignalStrength()->getLevel() + 1;
        ssMetrics.cdma.ss = signalStrength->getCdmaSignalStrength()->getDbm();
        ssMetrics.cdma.ecio = signalStrength->getCdmaSignalStrength()->getCdmaEcio();
        ssMetrics.cdma.io = signalStrength->getCdmaSignalStrength()->getEvdoEcio();
        ssMetrics.cdma.snr = signalStrength->getCdmaSignalStrength()->getEvdoSignalNoiseRatio();
    }

    if (signalStrength->getWcdmaSignalStrength() != nullptr &&
        signalStrength->getWcdmaSignalStrength()->getSignalStrength() !=
        INVALID_SIGNAL_STRENGTH_VALUE)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_UMTS;
        ssMetrics.umts.sslv = (int8_t)signalStrength->getWcdmaSignalStrength()->getLevel() + 1;
        ssMetrics.umts.ss = signalStrength->getWcdmaSignalStrength()->getDbm();
        ssMetrics.umts.ber = signalStrength->getWcdmaSignalStrength()->getBitErrorRate();
    }

    if (signalStrength->getTdscdmaSignalStrength() != nullptr &&
        signalStrength->getTdscdmaSignalStrength()->getRscp() != INVALID_SIGNAL_STRENGTH_VALUE)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_TDSCDMA;
        ssMetrics.tdscdma.rscp = signalStrength->getTdscdmaSignalStrength()->getRscp();
    }

    if (signalStrength->getLteSignalStrength() != nullptr &&
        signalStrength->getLteSignalStrength()->getLteSignalStrength() !=
        INVALID_SIGNAL_STRENGTH_VALUE)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_LTE;
        ssMetrics.lte.sslv = (int8_t)signalStrength->getLteSignalStrength()->getLevel() + 1;
        ssMetrics.lte.ss = signalStrength->getLteSignalStrength()->getDbm();
        ssMetrics.lte.rsrq = signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality();
        ssMetrics.lte.rsrp = signalStrength->getLteSignalStrength()->getDbm();
        ssMetrics.lte.snr = signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr();
    }

    if (signalStrength->getNr5gSignalStrength() != nullptr &&
        signalStrength->getNr5gSignalStrength()->getDbm() != INVALID_SIGNAL_STRENGTH_VALUE)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_NR5G;
        ssMetrics.nr5g.sslv = (int8_t)signalStrength->getNr5gSignalStrength()->getLevel() + 1;
        ssMetrics.nr5g.rsrq = signalStrength->getNr5gSignalStrength()->getReferenceSignalReceiveQuality();
        ssMetrics.nr5g.rsrp = signalStrength->getNr5gSignalStrength()->getDbm();
        ssMetrics.nr5g.snr = signalStrength->getNr5gSignalStrength()->getReferenceSignalSnr();
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for setting selection mode.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioNetworkResponseCallback::selModeSem = NULL;
le_sem_Ref_t taf_RadioNetworkResponseCallback::prefNetSem = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of setting selection mode.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioNetworkResponseCallback::selModeRes = LE_OK;
le_result_t taf_RadioNetworkResponseCallback::prefNetRes = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Response for setting selection mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioNetworkResponseCallback --> setNetworkSelectionModeResponseCb");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        selModeRes = LE_FAULT;
    }
    else
    {
        selModeRes = LE_OK;
    }

    le_sem_Post(selModeSem);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for setting preferred network.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioNetworkResponseCallback::setPreferredNetworksResponseCb
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioNetworkResponseCallback --> setPreferredNetworksResponseCb");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        prefNetRes = LE_FAULT;
    }
    else
    {
        prefNetRes = LE_OK;
    }

    le_sem_Post(prefNetSem);
}

//--------------------------------------------------------------------------------------------------
/**
 * Preferred network information.
 */
//--------------------------------------------------------------------------------------------------
std::vector<telux::tel::PreferredNetworkInfo> taf_RadioPreferredNetworksResponseCallback::preferredNetworksInfo;

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for getting preferred network information.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioPreferredNetworksResponseCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of getting preferred network information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioPreferredNetworksResponseCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting preferred network information.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioPreferredNetworksResponseCallback::preferredNetworksResponse
(
    std::vector<telux::tel::PreferredNetworkInfo> preferredNetworks3gppInfo,
        ///< [IN] Preferred network.
    std::vector<telux::tel::PreferredNetworkInfo> staticPreferredNetworksInfo,
        ///< [IN] Static preferred network.
    telux::common::ErrorCode error
        ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioPreferredNetworksResponseCallback --> preferredNetworksResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        preferredNetworksInfo.assign(preferredNetworks3gppInfo.begin(), preferredNetworks3gppInfo.end());
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for peforming network scan.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioPerformNetworkScanCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of performing network scan.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioPerformNetworkScanCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Response for performing network scan.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioPerformNetworkScanCallback::performNetworkScanResponse
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioPerformNetworkScanCallback --> performNetworkScanResponse");
    if (error != telux::common::ErrorCode::SUCCESS) 
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for setting serving system.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioServingSystemResponseCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of setting serving system.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioServingSystemResponseCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Response for setting serving system.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioServingSystemResponseCallback::servingSystemResponse
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioServingSystemResponseCallback --> servingSystemResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for getting RAT preference.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioRatPreferenceResponseCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of getting RAT preference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioRatPreferenceResponseCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * RAT preference.
 */
//--------------------------------------------------------------------------------------------------
telux::tel::RatPreference taf_RadioRatPreferenceResponseCallback::ratPref = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting RAT preference.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioRatPreferenceResponseCallback::ratPreferenceResponse
(
    telux::tel::RatPreference preference, ///< [IN] RAT preference.
    telux::common::ErrorCode error        ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioRatPreferenceResponseCallback --> ratPreferenceResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        ratPref = preference;
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for getting cell list information.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioCellInfoCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of getting cell list information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioCellInfoCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Cell list information.
 */
//--------------------------------------------------------------------------------------------------
taf_RadioCellListInfo_t taf_RadioCellInfoCallback::cellListInfo;

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting cell list information.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioCellInfoCallback::cellInfoListResponse
(
    std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList, ///< [IN] Cell information list.
    telux::common::ErrorCode error                                   ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioCellInfoCallback --> cellInfoListResponse");

    cellListInfo.servingCell.clear();
    cellListInfo.neighborCell.clear();

    if (error == telux::common::ErrorCode::SUCCESS)
    {
        for (auto cellInfo : cellInfoList)
        {
            taf_RadioCellInfo_t cellIdInfo;
            if (cellInfo == NULL)
            {
                LE_ERROR("Cell information pointer is NULL.");
                break;
            }

            switch (cellInfo->getType())
            {
                case telux::tel::CellType::GSM:
                {
                    cellIdInfo.rat = TAF_RADIO_RAT_GSM;
                    auto gsmCellInfo = std::static_pointer_cast<telux::tel::GsmCellInfo>(cellInfo);
                    cellIdInfo.gsm.bsic = gsmCellInfo->getCellIdentity().getBaseStationIdentityCode();
                    cellIdInfo.gsm.cid = gsmCellInfo->getCellIdentity().getIdentity();
                    cellIdInfo.gsm.lac = gsmCellInfo->getCellIdentity().getLac();
                    cellIdInfo.gsm.ta = gsmCellInfo->getSignalStrengthInfo().getTimingAdvance();
                    cellIdInfo.ss = gsmCellInfo->getSignalStrengthInfo().getDbm();
                    if (gsmCellInfo->isRegistered())
                    {
                        cellListInfo.servingCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    else
                    {
                        cellListInfo.neighborCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    break;
                }
                case telux::tel::CellType::CDMA:
                {
                    cellIdInfo.rat = TAF_RADIO_RAT_CDMA;
                    auto cdmaCellInfo = std::static_pointer_cast<telux::tel::CdmaCellInfo>(cellInfo);
                    cellIdInfo.cdma.bsid = cdmaCellInfo->getCellIdentity().getBaseStationId();
                    cellIdInfo.cdma.ecio = cdmaCellInfo->getSignalStrengthInfo().getCdmaEcio();
                    cellIdInfo.ss = cdmaCellInfo->getSignalStrengthInfo().getDbm();
                    if (cdmaCellInfo->isRegistered())
                    {
                        cellListInfo.servingCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    else
                    {
                        cellListInfo.neighborCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    break;
                }
                case telux::tel::CellType::WCDMA:
                {
                    cellIdInfo.rat = TAF_RADIO_RAT_UMTS;
                    auto umtsCellInfo = std::static_pointer_cast<telux::tel::WcdmaCellInfo>(cellInfo);
                    cellIdInfo.umts.lac = umtsCellInfo->getCellIdentity().getLac();
                    cellIdInfo.umts.cid = umtsCellInfo->getCellIdentity().getIdentity();
                    cellIdInfo.umts.psc = umtsCellInfo->getCellIdentity().getPrimaryScramblingCode();
                    cellIdInfo.ss = umtsCellInfo->getSignalStrengthInfo().getDbm();
                    if (umtsCellInfo->isRegistered())
                    {
                        cellListInfo.servingCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    else
                    {
                        cellListInfo.neighborCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    break;
                }
                case telux::tel::CellType::TDSCDMA:
                {
                    cellIdInfo.rat = TAF_RADIO_RAT_TDSCDMA;
                    auto tdscdmaCellInfo = std::static_pointer_cast<telux::tel::TdscdmaCellInfo>(cellInfo);
                    cellIdInfo.tdscdma.cid = tdscdmaCellInfo->getCellIdentity().getIdentity();
                    cellIdInfo.tdscdma.lac = tdscdmaCellInfo->getCellIdentity().getLac();
                    cellIdInfo.ss = tdscdmaCellInfo->getSignalStrengthInfo().getRscp();
                    if (tdscdmaCellInfo->isRegistered())
                    {
                        cellListInfo.servingCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    else
                    {
                        cellListInfo.neighborCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    break;
                }
                case telux::tel::CellType::LTE:
                {
                    cellIdInfo.rat = TAF_RADIO_RAT_LTE;
                    auto lteCellInfo = std::static_pointer_cast<telux::tel::LteCellInfo>(cellInfo);
                    cellIdInfo.lte.cid = lteCellInfo->getCellIdentity().getIdentity();
                    cellIdInfo.lte.pcid = lteCellInfo->getCellIdentity().getPhysicalCellId();
                    cellIdInfo.lte.tac = lteCellInfo->getCellIdentity().getTrackingAreaCode();
                    cellIdInfo.lte.earfcn = lteCellInfo->getCellIdentity().getEarfcn();
                    cellIdInfo.lte.ta = lteCellInfo->getSignalStrengthInfo().getTimingAdvance();
                    cellIdInfo.ss = lteCellInfo->getSignalStrengthInfo().getDbm();
                    if (lteCellInfo->isRegistered())
                    {
                        cellListInfo.servingCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    else
                    {
                        cellListInfo.neighborCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    break;
                }
                case telux::tel::CellType::NR5G:
                {
                    cellIdInfo.rat = TAF_RADIO_RAT_NR5G;
                    auto nr5gCellInfo = std::static_pointer_cast<telux::tel::Nr5gCellInfo>(cellInfo);
                    cellIdInfo.nr5g.cid = nr5gCellInfo->getCellIdentity().getIdentity();
                    cellIdInfo.nr5g.pcid = nr5gCellInfo->getCellIdentity().getPhysicalCellId();
                    cellIdInfo.nr5g.tac = nr5gCellInfo->getCellIdentity().getTrackingAreaCode();
                    cellIdInfo.nr5g.arfcn = nr5gCellInfo->getCellIdentity().getArfcn();
                    cellIdInfo.ss = nr5gCellInfo->getSignalStrengthInfo().getDbm();
                    if (nr5gCellInfo->isRegistered())
                    {
                        cellListInfo.servingCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    else
                    {
                        cellListInfo.neighborCell.push_back(
                            std::make_shared<taf_RadioCellInfo_t>(cellIdInfo));
                    }
                    break;
                }
                default:
                {
                    LE_ERROR("Unknown RAT(%d)", (int)cellInfo->getType());
                    break;
                }
            }
        }

        result = LE_OK;
    }
    else 
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for getting IMS registration information.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioImsServSysCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of getting IMS registration information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioImsServSysCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * IMS registration information.
 */
//--------------------------------------------------------------------------------------------------
telux::tel::RegistrationStatus taf_RadioImsServSysCallback::status;

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting IMS registration information.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsServSysCallback::imsRegStateResponse
(
    telux::tel::ImsRegistrationInfo info, ///< [IN] IMS registration information.
    telux::common::ErrorCode error        ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioImsServSysCallback --> imsRegStateResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        status = info.imsRegStatus;
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

LE_MEM_DEFINE_STATIC_POOL(prefOpsListPool, TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM, sizeof(taf_RadioPrefOpList_t));

LE_MEM_DEFINE_STATIC_POOL(prefOpPool, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM, sizeof(taf_RadioPrefOp_t));

LE_MEM_DEFINE_STATIC_POOL(prefOpSafeRefPool, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM, sizeof(taf_RadioPrefOpSafeRef_t));

LE_MEM_DEFINE_STATIC_POOL(scanOpsListPool, TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM, sizeof(taf_RadioScanOpList_t));

LE_MEM_DEFINE_STATIC_POOL(scanOpPool, TAF_RADIO_SCAN_OPERATORS_MAX_NUM, sizeof(taf_RadioScanOp_t));

LE_MEM_DEFINE_STATIC_POOL(scanOpSafeRefPool, TAF_RADIO_SCAN_OPERATORS_MAX_NUM, sizeof(taf_RadioScanOpSafeRef_t));

LE_MEM_DEFINE_STATIC_POOL(metricsPool, TAF_RADIO_METRICS_MAX_NUM, sizeof(taf_RadioSignalMetrics_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for neighboring cells
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ngbrCellsPool, TAF_RADIO_NEIGHBOR_CELLS_MAX_NUM, sizeof(taf_RadioNgbrCells_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for neighboring cell information
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ngbrCellInfoPool, TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM,
    sizeof(taf_RadioNgbrCellInfo_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for neighboring cell information safe reference
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ngbrCellInfoSafeRefPool, TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM,
    sizeof(taf_RadioNgbrCellInfoSafeRef_t));

LE_REF_DEFINE_STATIC_MAP(prefOpListRefMap, TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM);

LE_REF_DEFINE_STATIC_MAP(prefOpSafeRefMap, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM);

LE_REF_DEFINE_STATIC_MAP(scanOpListRefMap, TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM);

LE_REF_DEFINE_STATIC_MAP(scanOpSafeRefMap, TAF_RADIO_SCAN_OPERATORS_MAX_NUM);

LE_REF_DEFINE_STATIC_MAP(metricsRefMap, TAF_RADIO_METRICS_MAX_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for neighboring cells
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(ngbrCellsRefMap, TAF_RADIO_NEIGHBOR_CELLS_MAX_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for neighboring cell information safe reference
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(ngbrCellInfoSafeRefMap, TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM);

le_event_Id_t taf_Radio::radioCmdEvId = nullptr;

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for IMS registration state.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerImsRegStateHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_ImsRegStatusChangeHandlerFunc_t handlerFunc =
        (taf_radio_ImsRegStatusChangeHandlerFunc_t)layerHandlerFunc;
    if (handlerFunc)
    {
        taf_RadioImsRegStatus_t* statusPtr = (taf_RadioImsRegStatus_t*)reportPtr;
        handlerFunc(statusPtr->status, statusPtr->phoneId, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

/*======================================================================

 FUNCTION        taf_Radio::GetInstance

 DESCRIPTION     Get the instance of radio.

 DEPENDENCIES    The initialization of Radio.

 PARAMETERS      None

 RETURN VALUE    taf_Radio&

 SIDE EFFECTS

======================================================================*/
taf_Radio &taf_Radio::GetInstance()
{
    static taf_Radio instance;
    return instance;
}

/*======================================================================

 FUNCTION        taf_Radio::RadioProcCmdHandler

 DESCRIPTION     Radio command handler.

 DEPENDENCIES    The initialization of radio command thread.

 PARAMETERS      [IN] void* cmdReqPtr: Command request pointer.

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Radio::RadioProcCmdHandler(void* cmdReqPtr)
{
    taf_RadioCmdReq_t* cmdReq = (taf_RadioCmdReq_t*)cmdReqPtr;
    uint8_t phoneId = cmdReq->phoneId;
    auto &tafRadio = taf_Radio::GetInstance();

    le_result_t res = LE_OK;
    le_clk_Time_t timeToWait = {1, 0};

    switch (cmdReq->cmdType)
    {
        case TAF_RADIO_CMD_TYPE_ASYNC_REG_MANUAL:
        {
            TAF_ERROR_IF_RET_NIL(phoneId > tafRadio.networkManagers.size(),
                "Invalid para(phoneId:%d > %" PRIuS ")", phoneId, tafRadio.networkManagers.size());

            auto networkManager = tafRadio.networkManagers[phoneId - 1];
            TAF_ERROR_IF_RET_NIL(networkManager == nullptr, "Invalid para(null ptr, phoneId:%d)", phoneId);

            std::string mcc(cmdReq->mccPtr);
            std::string mnc(cmdReq->mncPtr);

            if (networkManager->setNetworkSelectionMode(telux::tel::NetworkSelectionMode::MANUAL, mcc, mnc,
                &taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb) != telux::common::Status::SUCCESS)
            {
                LE_ERROR("setNetworkSelectionMode failed.");
                res = LE_FAULT;
            }

            res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::selModeSem, timeToWait);
            if (res != LE_OK)
            {
                LE_ERROR("Wait semaphore timeout.");
                res = LE_TIMEOUT;
            }

            if (taf_RadioNetworkResponseCallback::selModeRes != LE_OK)
            {
                LE_ERROR("Error response when setting network selection mode.");
                res = LE_FAULT;
            }

            taf_radio_ManualSelectionHandlerFunc_t handlerFunc = (taf_radio_ManualSelectionHandlerFunc_t)cmdReq->handlerFuncPtr;
            if (handlerFunc != nullptr)
            {
                LE_DEBUG("Handler function:%p, result:%d", handlerFunc, res);
                handlerFunc(res, cmdReq->contextPtr);
            }
            else
            {
                LE_WARN("No handler function, result:%d", res);
            }
            break;
        }
        case TAF_RADIO_CMD_TYPE_ASYNC_NETWORK_SCAN:
        {
            TAF_ERROR_IF_RET_NIL(phoneId > tafRadio.networkManagers.size(),
                "Invalid para(phoneId:%d > %" PRIuS ")", phoneId, tafRadio.networkManagers.size());

            auto networkManager = tafRadio.networkManagers[phoneId - 1];
            TAF_ERROR_IF_RET_NIL(networkManager == NULL,
                "Invalid network manager(null ptr, phoneId:%d)", phoneId);

            auto networkListener = tafRadio.networkListeners[phoneId - 1];
            TAF_ERROR_IF_RET_NIL(networkListener == NULL,
                "Invalid network listener(null ptr, phoneId:%d)", phoneId);
            networkListener->opInfos.clear();

            auto status = networkManager->registerListener(networkListener);
            TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS,
                "Fail to register listener with phoneId:%d)", phoneId);

            telux::tel::NetworkScanInfo info;
            info.scanType = telux::tel::NetworkScanType::ALL_RATS;
            if (networkManager->performNetworkScan(info,
                taf_RadioPerformNetworkScanCallback::performNetworkScanResponse) !=
                telux::common::Status::SUCCESS)
            {
                LE_ERROR("Call sdk function failed");
                status = networkManager->deregisterListener(networkListener);
                TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS,
                    "Fail to deregister listener with phoneId:%d)", phoneId);
            };

            res = le_sem_WaitWithTimeOut(taf_RadioPerformNetworkScanCallback::semaphore,
                timeToWait);
            if (res != LE_OK || taf_RadioPerformNetworkScanCallback::result != LE_OK)
            {
                LE_ERROR("Perform network scan failed.");
                status = networkManager->deregisterListener(networkListener);
                TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS,
                    "Fail to deregister listener with phoneId:%d)", phoneId);
            }

            le_clk_Time_t timeToScan = {TAF_RADIO_SCAN_INTERVAL, 0};
            res = le_sem_WaitWithTimeOut(networkListener->semaphore, timeToScan);
            if (res != LE_OK)
            {
                LE_ERROR("Network scan timeout");
                status = networkManager->deregisterListener(networkListener);
                TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS,
                    "Fail to deregister listener with phoneId:%d)", phoneId);
            };

            status = networkManager->deregisterListener(networkListener);
            TAF_ERROR_IF_RET_NIL(status != telux::common::Status::SUCCESS,
                "Fail to deregister listener with phoneId:%d)", phoneId);

            TAF_ERROR_IF_RET_NIL(networkListener->opInfos.size() == 0,
                "Phone%d has no operators after scanning", phoneId);

            taf_RadioScanOpList_t* opsList = (taf_RadioScanOpList_t*)le_mem_ForceAlloc(tafRadio.scanOpsListPool);
            TAF_ERROR_IF_RET_NIL(opsList == NULL, "Null ptr(opsList)");

            opsList->scanOpList = LE_SLS_LIST_INIT;
            opsList->safeRefList = LE_SLS_LIST_INIT;
            opsList->currPtr = NULL;

            taf_RadioScanOp_t* opPtr;
            for (auto info : networkListener->opInfos)
            {
                opPtr = (taf_RadioScanOp_t*)le_mem_ForceAlloc(tafRadio.scanOpPool);
                le_utf8_Copy(opPtr->name, info.getName().c_str(), TAF_RADIO_NETWORK_NAME_MAX_LEN, NULL);
                le_utf8_Copy(opPtr->mcc, info.getMcc().c_str(), TAF_RADIO_MCC_BYTES, NULL);
                le_utf8_Copy(opPtr->mnc, info.getMnc().c_str(), TAF_RADIO_MNC_BYTES, NULL);
                opPtr->status.inUse = info.getStatus().inUse;
                opPtr->status.roaming = info.getStatus().roaming;
                opPtr->status.forbidden = info.getStatus().forbidden;
                opPtr->status.preferred = info.getStatus().preferred;
                opPtr->rat = info.getRat();
                opPtr->link = LE_SLS_LINK_INIT;
                le_sls_Queue(&(opsList->scanOpList), &(opPtr->link));
            }

            taf_radio_ScanInformationListRef_t listRef = 
                (taf_radio_ScanInformationListRef_t)le_ref_CreateRef(tafRadio.scanOpListRefMap, (void*)opsList);
            taf_radio_CellularNetworkScanHandlerFunc_t handlerFunc =
                (taf_radio_CellularNetworkScanHandlerFunc_t)cmdReq->handlerFuncPtr;
            if (handlerFunc != nullptr)
            {
                LE_DEBUG("Handler function:%p listRef:%p", handlerFunc, listRef);
                handlerFunc(listRef, cmdReq->contextPtr);
            }
            else
            {
                LE_WARN("No handler function");
            }
            break;
        }
        case TAF_RADIO_CMD_TYPE_ASYNC_PCI_NETWORK_SCAN:
        {
            taf_radio_PciScanInformationListRef_t listRef =
                taf_pa_radio_PerformPciNetworkScan(cmdReq->ratMask, cmdReq->phoneId);
            taf_radio_PciNetworkScanHandlerFunc_t handlerFunc =
                (taf_radio_PciNetworkScanHandlerFunc_t)cmdReq->handlerFuncPtr;
            if (handlerFunc)
            {
                LE_DEBUG("Handler function:%p", handlerFunc);
                handlerFunc(listRef, phoneId, cmdReq->contextPtr);
            }
            else
            {
                LE_WARN("No handler function.");
            }
            break;
        }
    }
}

/*======================================================================

 FUNCTION        taf_Radio::RadioCmdThread

 DESCRIPTION     Radio command thread for handling asynchronous request.

 DEPENDENCIES    The initialization of Radio.

 PARAMETERS      [IN] void* contextPtr: Context pointer.

 RETURN VALUE    void*
                     NULL: Success.

 SIDE EFFECTS

======================================================================*/
void* taf_Radio::RadioCmdThread(void* contextPtr)
{
    le_event_AddHandler("RadioProcCmdHandler", radioCmdEvId, RadioProcCmdHandler);
    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register Listener.
 */
//--------------------------------------------------------------------------------------------------
void RegisterListener
(
    void
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    for (size_t i = 1; i <= tafRadio.slotCount; i++)
    {
        auto listener = std::make_shared<taf_RadioImsServSysListener>((SlotId)i);
        telux::common::Status status =
            tafRadio.imsServingSystemMgrs[(SlotId)i]->registerListener(listener);
        if (status != telux::common::Status::SUCCESS)
        {
            LE_ERROR("Failed to register IMS serving system listener.");
        }
        else
        {
            tafRadio.imsServSysListeners.emplace((SlotId)i, listener);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Deregister Listener.
 */
//--------------------------------------------------------------------------------------------------
void DeregisterListener
(
    void
)
{
    auto &tafRadio = taf_Radio::GetInstance();
    for (size_t i = 1; i <= tafRadio.slotCount; i++)
    {
        if (tafRadio.imsServingSystemMgrs[(SlotId)i] != nullptr &&
            tafRadio.imsServSysListeners[(SlotId)i] != nullptr)
        {
            tafRadio.imsServingSystemMgrs[(SlotId)i]->deregisterListener(
                tafRadio.imsServSysListeners[(SlotId)i]);
        }
    }
    tafRadio.imsServSysListeners.clear();
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for power state changes.
 */
//--------------------------------------------------------------------------------------------------
void PowerStateChangeHandler
(
    taf_pm_State_t state, ///< [IN] PM state.
    void* contextPtr      ///< [IN] Handler context.
)
{
    le_result_t result;
    if (state == TAF_PM_STATE_RESUME)
    {
        LE_INFO("Power state change to RESUME");
        RegisterListener();
        result = taf_pa_radio_EnableIndication();
        TAF_ERROR_IF_RET_NIL(result != LE_OK, "Enable indication falied.");
    }
    else if (state == TAF_PM_STATE_SUSPEND)
    {
        LE_INFO("Power state change to SUSPEND");
        DeregisterListener();
        result = taf_pa_radio_DisableIndication();
        TAF_ERROR_IF_RET_NIL(result != LE_OK, "Disable indication falied.");
    }
}

/*======================================================================

 FUNCTION        taf_Radio::Init

 DESCRIPTION     Initialization of the Radio Service

 DEPENDENCIES    The initialization of telaf.

 PARAMETERS      None

 RETURN VALUE    None

 SIDE EFFECTS

======================================================================*/
void taf_Radio::Init(void)
{
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    std::chrono::duration<double> elapsedTime;

    // 1. Initiate the semaphore
    taf_RadioNetworkResponseCallback::selModeSem = le_sem_Create("taf_RadioSelModeSem", 0);
    taf_RadioNetworkResponseCallback::prefNetSem = le_sem_Create("taf_RadioPrefNetSem", 0);
    taf_RadioPreferredNetworksResponseCallback::semaphore = le_sem_Create("taf_RadioPrefNetworkRespCbSem", 0);
    taf_RadioServingSystemResponseCallback::semaphore = le_sem_Create("taf_RadioSrvSysSem", 0);
    taf_RadioRatPreferenceResponseCallback::semaphore = le_sem_Create("taf_RadioRatPrefRespCbSem", 0);
    taf_RadioCellInfoCallback::semaphore = le_sem_Create("taf_RadioCellInfoCbSem", 0);
    taf_RadioPerformNetworkScanCallback::semaphore = le_sem_Create("taf_RadioPerfNetScanCbSem", 0);
    taf_RadioImsServSysCallback::semaphore = le_sem_Create("taf_RadioImsServSysCbSem", 0);

    imsRegStatusChangeId = le_event_CreateIdWithRefCounting("ImsRegStatus");

    // 2. Initiate the memory pool
    prefOpsListPool = le_mem_InitStaticPool(prefOpsListPool,
        TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM, sizeof(taf_RadioPrefOpList_t));
    prefOpPool = le_mem_InitStaticPool(prefOpPool, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM,
        sizeof(taf_RadioPrefOp_t));
    prefOpSafeRefPool = le_mem_InitStaticPool(prefOpSafeRefPool, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM,
        sizeof(taf_RadioPrefOpSafeRef_t));
    scanOpsListPool = le_mem_InitStaticPool(scanOpsListPool,
        TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM, sizeof(taf_RadioScanOpList_t));
    scanOpPool = le_mem_InitStaticPool(scanOpPool, TAF_RADIO_SCAN_OPERATORS_MAX_NUM,
        sizeof(taf_RadioScanOp_t));
    scanOpSafeRefPool = le_mem_InitStaticPool(scanOpSafeRefPool, TAF_RADIO_SCAN_OPERATORS_MAX_NUM,
        sizeof(taf_RadioScanOpSafeRef_t));
    metricsPool = le_mem_InitStaticPool(metricsPool, TAF_RADIO_METRICS_MAX_NUM,
        sizeof(taf_RadioSignalMetrics_t));
    ngbrCellsPool = le_mem_InitStaticPool(ngbrCellsPool, TAF_RADIO_NEIGHBOR_CELLS_MAX_NUM,
        sizeof(taf_RadioNgbrCells_t));
    ngbrCellInfoPool = le_mem_InitStaticPool(ngbrCellInfoPool, TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM,
        sizeof(taf_RadioNgbrCellInfo_t));
    ngbrCellInfoSafeRefPool = le_mem_InitStaticPool(ngbrCellInfoSafeRefPool,
        TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM, sizeof(taf_RadioNgbrCellInfoSafeRef_t));
    imsRegStatusChangePool = le_mem_CreatePool("imsRegStatusChangePool", sizeof(taf_RadioImsRegStatus_t));

    // 3. Initiate the reference map.
    prefOpListRefMap = le_ref_InitStaticMap(prefOpListRefMap, TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM);
    prefOpSafeRefMap = le_ref_InitStaticMap(prefOpSafeRefMap, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM);
    scanOpListRefMap = le_ref_InitStaticMap(scanOpListRefMap, TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM);
    scanOpSafeRefMap = le_ref_InitStaticMap(scanOpSafeRefMap, TAF_RADIO_SCAN_OPERATORS_MAX_NUM);
    metricsRefMap = le_ref_InitStaticMap(metricsRefMap, TAF_RADIO_METRICS_MAX_NUM);
    ngbrCellsRefMap = le_ref_InitStaticMap(ngbrCellsRefMap, TAF_RADIO_NEIGHBOR_CELLS_MAX_NUM);
    ngbrCellInfoSafeRefMap = le_ref_InitStaticMap(ngbrCellInfoSafeRefMap,
        TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM);

    startTime = std::chrono::system_clock::now();

    // 4. Get the PhoneFactory and PhoneManager instances
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    phoneManager = phoneFactory.getPhoneManager();

    // 5. Check if telephony subsystem is ready
    bool subSystemStatus = phoneManager->isSubsystemReady();
    if (!subSystemStatus) {
        LE_INFO("Telephony subsystem wait to be ready...");
        future<bool> f = phoneManager->onSubsystemReady();
        //  Wait until the subsystem is ready.
        subSystemStatus = f.get();
    }

    if (subSystemStatus) {
        endTime = std::chrono::system_clock::now();
        elapsedTime = endTime - startTime;
        LE_INFO("Elapsed time for telephony subsystem: %lfs", elapsedTime.count());

        // 6. Instantiate Phone
        std::vector<int> phoneIds;
        telux::common::Status status = phoneManager->getPhoneIds(phoneIds);
        if (status == telux::common::Status::SUCCESS) {
            for (size_t index = 1; index <= phoneIds.size(); index++) {
                auto phone = phoneManager->getPhone(index);
                if (phone != nullptr) {
                    phones.emplace_back(phone);
                }
                auto networkManager = telux::tel::PhoneFactory::getInstance().getNetworkSelectionManager(index);
                if (networkManager != nullptr) {
                    networkManagers.emplace_back(networkManager);
                    auto networkListener = std::make_shared<taf_RadioNetworkSelectionListener>();
                    networkListener->semaphore = le_sem_Create("networkListenerSem", 0);
                    networkListeners.emplace_back(networkListener);
                }
                auto servingSystemManager = telux::tel::PhoneFactory::getInstance().getServingSystemManager(index);
                if (servingSystemManager != nullptr) {
                    servingSystemManagers.emplace_back(servingSystemManager);
                }
            }
        }

        // 7. Instantiate RadioCallback
        signalStrengthCb = std::make_shared<taf_RadioSignalStrengthCallback>();
        setOperatingModeCb = std::make_shared<taf_RadioSetOperatingModeCallback>();
        getOperatingModeCb = std::make_shared<taf_RadioGetOperatingModeCallback>();
        signalStrengthCb->semaphore = le_sem_Create("taf_RadioSgnStrengthCbSem", 0);
        setOperatingModeCb->semaphore = le_sem_Create("taf_RadioSetOpModeCbSem", 0);
        getOperatingModeCb->semaphore = le_sem_Create("taf_RadioGetOpModeCbSem", 0);

        for (size_t index = 0; index < networkManagers.size(); index++) {
            startTime = std::chrono::system_clock::now();

            // 8. Check if network subsystem is ready
            bool networkSystemStatus = networkManagers[index]->isSubsystemReady();
            if (!networkSystemStatus) {
                LE_INFO("Network subsystem wait to be ready...");
                std::future<bool> f = networkManagers[index]->onSubsystemReady();
                //  Wait until the subsystem is ready.
                networkSystemStatus = f.get();
            }

            if (networkSystemStatus) {
                endTime = std::chrono::system_clock::now();
                elapsedTime = endTime - startTime;
                LE_INFO("Elapsed time for %" PRIuS " network subsystem: %lfs", index, elapsedTime.count());
            } else {
                LE_ERROR("Fail to init %" PRIuS " network subsystem", index);
            }
        }

        for (size_t index = 0; index < servingSystemManagers.size(); index++) {
            startTime = std::chrono::system_clock::now();

            // 9. Check if serving subsystem is ready
            bool servingSystemStatus = servingSystemManagers[index]->isSubsystemReady();
            if (!servingSystemStatus) {
                LE_INFO("Serving subsystem wait to be ready...");
                std::future<bool> f = servingSystemManagers[index]->onSubsystemReady();
                //  Wait until the subsystem is ready.
                servingSystemStatus = f.get();
            }

            if (servingSystemStatus) {
                endTime = std::chrono::system_clock::now();
                elapsedTime = endTime - startTime;
                LE_INFO("Elapsed time for %" PRIuS " serving subsystem: %lfs", index, elapsedTime.count());
            } else {
                LE_ERROR("Fail to init %" PRIuS " serving subsystem", index);
            }
        }
    } else {
        LE_ERROR("Fail to init telephony subsystem");
    }

    // 10. Initiate IMS serving system
    if (telux::common::DeviceConfig::isMultiSimSupported())
    {
        slotCount = TAF_RADIO_MULTI_SLOT_NUM;
    }
    for (size_t index = 1; index <= slotCount; index++)
    {
        startTime = std::chrono::system_clock::now();

        if (imsServingSystemMgrs.find((SlotId)index) != imsServingSystemMgrs.end())
        {
            LE_INFO("IMS Serving System manager is already initialized.");
        }
        else
        {
            std::promise<telux::common::ServiceStatus> prom;
            auto imsServingSystemMgr = phoneFactory.getImsServingSystemManager(
                (SlotId)index,[&](telux::common::ServiceStatus status)
                {
                    if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                    {
                        prom.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                    }
                    else
                    {
                        prom.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
                    }
                });
            if (!imsServingSystemMgr)
            {
                LE_ERROR("Failed to get IMS Serving System instance.");
            }
            else
            {
                telux::common::ServiceStatus imsServSysMgrStatus =
                    imsServingSystemMgr->getServiceStatus();
                if (imsServSysMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    LE_INFO("IMS serving subsystem wait to be ready...");
                    imsServSysMgrStatus = prom.get_future().get();
                }
                if (imsServSysMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    imsServingSystemMgrs.emplace((SlotId)index, imsServingSystemMgr);
                    endTime = std::chrono::system_clock::now();
                    elapsedTime = endTime - startTime;
                    LE_INFO("Elapsed time for %" PRIuS" IMS serving subsystem: %lfs",
                        index, elapsedTime.count());
                }
                else
                {
                    LE_ERROR("Fail to init IMS serving subsystem");
                }
            }
        }
    }

    // 11. Create and start command thread.
    le_sem_Ref_t radioCmdThreadSem = le_sem_Create("radioCmdThreadSem", 0);
    radioCmdEvId = le_event_CreateId("radioCmd", sizeof(taf_RadioCmdReq_t));
    le_thread_Ref_t radioCmdThreadRef = le_thread_Create("radioCmdThread", RadioCmdThread, (void*)radioCmdThreadSem);
    le_thread_SetStackSize(radioCmdThreadRef, TAF_RADIO_THREAD_STACK_SIZE);
    le_thread_Start(radioCmdThreadRef);
    le_sem_Wait(radioCmdThreadSem);

    // 12. Delete semaphore.
    le_sem_Delete(radioCmdThreadSem);

    // 13. Add power state change handle.
    taf_pm_AddStateChangeHandler(PowerStateChangeHandler, NULL);
    if (taf_pm_GetPowerState() != TAF_PM_STATE_SUSPEND)
    {
        RegisterListener();
        taf_pa_radio_EnableIndication();
    }
}
