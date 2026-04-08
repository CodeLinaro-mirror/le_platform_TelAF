/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EcallMgrNetwork.hpp"
#include "ApiRadio.hpp"

extern "C"
{
#include "legato.h"
}

namespace ecall
{

namespace
{

constexpr const char* LOG_NAME = "EcallMgrNetwork";

const char* RatToStr(taf_radio_Rat_t rat)
{
    switch (rat)
    {
        case TAF_RADIO_RAT_GSM:  return "GSM";
        case TAF_RADIO_RAT_UMTS: return "UMTS";
        case TAF_RADIO_RAT_LTE:  return "LTE";
#ifdef TAF_RADIO_RAT_NR5G
        case TAF_RADIO_RAT_NR5G: return "NR5G";
#endif
        default:                 return "UNKNOWN";
    }
}

} // anonymous namespace

// =============================================================================
// Singleton
// =============================================================================
EcallMgrNetwork& EcallMgrNetwork::GetInstance()
{
    static EcallMgrNetwork instance;
    return instance;
}

// =============================================================================
// GetNetworkStatus
// =============================================================================
le_result_t EcallMgrNetwork::GetNetworkStatus(uint8_t          phoneId,
                                              EcNetworkStatus& outStatus)
{
    auto& radio = ApiRadio::GetInstance();

    outStatus = EcNetworkStatus{};

    taf_radio_Rat_t rat = TAF_RADIO_RAT_UNKNOWN;
    le_result_t rc = radio.GetRatInUse(&rat, phoneId);
    if (rc == LE_OK)
    {
        outStatus.rat = rat;
    }
    else
    {
        LE_WARN("%s: GetRatInUse failed rc=%d (phoneId=%u)",
                LOG_NAME, rc, phoneId);
    }

    le_result_t plmnRc = radio.GetCurrentPlmn(
        outStatus.mcc, sizeof(outStatus.mcc),
        outStatus.mnc, sizeof(outStatus.mnc),
        phoneId);
    if (plmnRc != LE_OK)
    {
        LE_WARN("%s: GetCurrentPlmn failed rc=%d (phoneId=%u)",
                LOG_NAME, plmnRc, phoneId);
    }

    LE_INFO("%s: GetNetworkStatus rat=%s mcc=%s mnc=%s",
            LOG_NAME,
            RatToStr(outStatus.rat),
            outStatus.mcc,
            outStatus.mnc);

    // Return LE_OK even if individual queries failed; caller can inspect
    // the fields (rat == UNKNOWN or mcc/mnc empty) to detect partial data.
    return LE_OK;
}

// =============================================================================
// GetSignalStrength
// =============================================================================
le_result_t EcallMgrNetwork::GetSignalStrength(uint8_t           phoneId,
                                               EcSignalStrength& outSig)
{
    auto& radio = ApiRadio::GetInstance();

    outSig = EcSignalStrength{};

    taf_radio_Rat_t rat = TAF_RADIO_RAT_UNKNOWN;
    le_result_t rc = radio.GetRatInUse(&rat, phoneId);
    if (rc != LE_OK)
    {
        LE_ERROR("%s: GetSignalStrength: GetRatInUse failed rc=%d (phoneId=%u)",
                 LOG_NAME, rc, phoneId);
        return rc;
    }

    outSig.rat = rat;

    switch (rat)
    {
        case TAF_RADIO_RAT_GSM:
        {
            int32_t rssi = 0;
            rc = radio.GetGsmSignalMetrics(&rssi, phoneId);
            if (rc != LE_OK)
            {
                LE_ERROR("%s: GetGsmSignalMetrics failed rc=%d", LOG_NAME, rc);
                return rc;
            }
            outSig.rawRssi = rssi;
            break;
        }

        case TAF_RADIO_RAT_UMTS:
        {
            int32_t ss   = 0;
            int32_t rscp = 0;
            rc = radio.GetUmtsSignalMetrics(&ss, &rscp, phoneId);
            if (rc != LE_OK)
            {
                LE_ERROR("%s: GetUmtsSignalMetrics failed rc=%d", LOG_NAME, rc);
                return rc;
            }
            outSig.rawRssi  = ss;
            outSig.umtsRscp = rscp;
            break;
        }

        case TAF_RADIO_RAT_LTE:
        {
            int32_t ss   = 0;
            int32_t rsrq = 0;
            int32_t rsrp = 0;
            int32_t snr  = 0;
            rc = radio.GetLteSignalMetrics(&ss, &rsrq, &rsrp, &snr, phoneId);
            if (rc != LE_OK)
            {
                LE_ERROR("%s: GetLteSignalMetrics failed rc=%d", LOG_NAME, rc);
                return rc;
            }
            outSig.rawRssi = INT32_MIN;  // LTE RSSI not meaningful for eCall logging
            outSig.rsrp    = rsrp;
            outSig.rsrq    = rsrq;
            outSig.sinr    = snr;
            break;
        }

#ifdef TAF_RADIO_RAT_NR5G
        case TAF_RADIO_RAT_NR5G:
        {
            int32_t rsrq = 0;
            int32_t rsrp = 0;
            int32_t snr  = 0;
            rc = radio.GetNr5gSignalMetrics(&rsrq, &rsrp, &snr, phoneId);
            if (rc != LE_OK)
            {
                LE_ERROR("%s: GetNr5gSignalMetrics failed rc=%d", LOG_NAME, rc);
                return rc;
            }
            outSig.rawRssi = INT32_MIN;  // NR5G RSSI not meaningful for eCall logging
            outSig.rsrp    = rsrp;
            outSig.rsrq    = rsrq;
            outSig.sinr    = snr;
            break;
        }
#endif

        default:
            LE_INFO("%s: GetSignalStrength: unsupported RAT=%d",
                    LOG_NAME, static_cast<int>(rat));
            return LE_UNSUPPORTED;
    }

    LE_INFO("%s: GetSignalStrength rat=%s rawRssi=%d rsrp=%d rsrq=%d "
            "sinr=%d umtsRscp=%d",
            LOG_NAME,
            RatToStr(outSig.rat),
            outSig.rawRssi,
            outSig.rsrp,
            outSig.rsrq,
            outSig.sinr,
            outSig.umtsRscp);

    return LE_OK;
}

} // namespace ecall

