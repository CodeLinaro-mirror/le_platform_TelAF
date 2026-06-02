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

/*
 *  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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
using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Covert RF band bitmask.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_BandBitMask_t taf_radio_CovertRFBand
(
    std::shared_ptr<telux::tel::IRFBandList> list ///< [IN] RF band list.
)
{
    taf_radio_BandBitMask_t bitmask = 0;
    std::vector<telux::tel::GsmRFBand> gsmBands = list->getGsmBands();
    if(!gsmBands.empty())
    {
        for (auto gsmBand : gsmBands)
        {
            switch (gsmBand)
            {
                case telux::tel::GsmRFBand::GSM_450:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_GSM_BAND_450;
                    break;
                case telux::tel::GsmRFBand::GSM_480:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_GSM_BAND_480;
                    break;
                case telux::tel::GsmRFBand::GSM_750:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_GSM_BAND_750;
                    break;
                case telux::tel::GsmRFBand::GSM_850:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_GSM_BAND_850;
                    break;
                case telux::tel::GsmRFBand::GSM_900_EXTENDED:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_CLASS_E_GSM_900_BAND;
                    break;
                case telux::tel::GsmRFBand::GSM_900_PRIMARY:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_CLASS_P_GSM_900_BAND;
                    break;
                case telux::tel::GsmRFBand::GSM_900_RAILWAYS:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_GSM_BAND_RAILWAYS_900_BAND;
                    break;
                case telux::tel::GsmRFBand::GSM_1800:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_CLASS_GSM_DCS_1800_BAND;
                    break;
                case telux::tel::GsmRFBand::GSM_1900:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_GSM_PCS_1900_BAND;
                    break;
                default:
                    LE_ERROR("Invalid GSM band.");
                    break;
            }
        }
    }

    std::vector<telux::tel::WcdmaRFBand> wcdmaBands = list->getWcdmaBands();
    if(!wcdmaBands.empty())
    {
        for (auto wcdmaBand : wcdmaBands)
        {
            switch (wcdmaBand)
            {
                case telux::tel::WcdmaRFBand::WCDMA_2100:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_J_CH_IMT_2100_BAND;
                    break;
                case telux::tel::WcdmaRFBand::WCDMA_PCS_1900:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_WCDMA_US_PCS_1900_BAND;
                    break;
                case telux::tel::WcdmaRFBand::WCDMA_DCS_1800:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_EU_CH_DCS_1800_BAND;
                    break;
                case telux::tel::WcdmaRFBand::WCDMA_1700_US:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_WCDMA_US_1700_BAND;
                    break;
                case telux::tel::WcdmaRFBand::WCDMA_850:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_WCDMA_US_850_BAND;
                    break;
                case telux::tel::WcdmaRFBand::WCDMA_800:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_WCDMA_JAPAN_800_BAND;
                    break;
                case telux::tel::WcdmaRFBand::WCDMA_2600:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_2600_BAND;
                    break;
                case telux::tel::WcdmaRFBand::WCDMA_900:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_J_900_BAND;
                    break;
                case telux::tel::WcdmaRFBand::WCDMA_1700_JAPAN:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_J_1700_BAND;
                    break;
                case telux::tel::WcdmaRFBand::WCDMA_1500_JAPAN:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_WCDMA_JAPAN_1500_BAND;
                    break;
                case telux::tel::WcdmaRFBand::WCDMA_850_JAPAN:
                    bitmask |= TAF_RADIO_BAND_BIT_MASK_WCDMA_JAPAN_850_BAND;
                    break;
                default:
                    LE_ERROR("Invalid WCDMA band.");
                    break;
            }
        }
    }

    return bitmask;
}

//--------------------------------------------------------------------------------------------------
/**
 * Covert RF bandwidth.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_RFBandWidth_t taf_radio_CovertRFBandwidth
(
    telux::tel::RFBandWidth bandwidth ///< [IN] RF bandwidth.
)
{
    switch (bandwidth)
    {
        case telux::tel::RFBandWidth::GSM_BW_NRB_2:
            return TAF_RADIO_RF_BANDWIDTH_GSM_BW_0_2;
        case telux::tel::RFBandWidth::WCDMA_BW_NRB_5:
            return TAF_RADIO_RF_BANDWIDTH_WCDMA_BW_5;
        case telux::tel::RFBandWidth::WCDMA_BW_NRB_10:
            return TAF_RADIO_RF_BANDWIDTH_WCDMA_BW_10;
        case telux::tel::RFBandWidth::TDSCDMA_BW_NRB_2:
            return TAF_RADIO_RF_BANDWIDTH_TDSCDMA_BW_1_6;
        case telux::tel::RFBandWidth::LTE_BW_NRB_6:
            return TAF_RADIO_RF_BANDWIDTH_LTE_BW_1_4;
        case telux::tel::RFBandWidth::LTE_BW_NRB_15:
            return TAF_RADIO_RF_BANDWIDTH_LTE_BW_3;
        case telux::tel::RFBandWidth::LTE_BW_NRB_25:
            return TAF_RADIO_RF_BANDWIDTH_LTE_BW_5;
        case telux::tel::RFBandWidth::LTE_BW_NRB_50:
            return TAF_RADIO_RF_BANDWIDTH_LTE_BW_10;
        case telux::tel::RFBandWidth::LTE_BW_NRB_75:
            return TAF_RADIO_RF_BANDWIDTH_LTE_BW_15;
        case telux::tel::RFBandWidth::LTE_BW_NRB_100:
            return TAF_RADIO_RF_BANDWIDTH_LTE_BW_20;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_5:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_5;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_10:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_10;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_15:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_15;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_20:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_20;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_25:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_25;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_30:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_30;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_40:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_40;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_50:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_50;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_60:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_60;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_70:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_70;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_80:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_80;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_90:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_90;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_100:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_100;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_200:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_200;
        case telux::tel::RFBandWidth::NR5G_BW_NRB_400:
            return TAF_RADIO_RF_BANDWIDTH_NR5G_BW_400;
        default:
            LE_DEBUG("Invalid bandwidth.");
    }

    return TAF_RADIO_RF_BANDWIDTH_INVALID;
}

//--------------------------------------------------------------------------------------------------
/**
 * Covert 2G/3G RF active band.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_BandBitMask_t taf_radio_CovertRFBand
(
    telux::tel::RFBand band ///< [IN] 2G/3G active band
)
{
    switch (band)
    {
        case telux::tel::RFBand::GSM_450:
            return TAF_RADIO_BAND_BIT_MASK_GSM_BAND_450;
        case telux::tel::RFBand::GSM_480:
            return TAF_RADIO_BAND_BIT_MASK_GSM_BAND_480;
        case telux::tel::RFBand::GSM_750:
            return TAF_RADIO_BAND_BIT_MASK_GSM_BAND_750;
        case telux::tel::RFBand::GSM_850:
            return TAF_RADIO_BAND_BIT_MASK_GSM_BAND_850;
        case telux::tel::RFBand::GSM_900_EXTENDED:
            return TAF_RADIO_BAND_BIT_MASK_CLASS_E_GSM_900_BAND;
        case telux::tel::RFBand::GSM_900_PRIMARY:
            return TAF_RADIO_BAND_BIT_MASK_CLASS_P_GSM_900_BAND;
        case telux::tel::RFBand::GSM_900_RAILWAYS:
            return TAF_RADIO_BAND_BIT_MASK_GSM_BAND_RAILWAYS_900_BAND;
        case telux::tel::RFBand::GSM_1800:
            return TAF_RADIO_BAND_BIT_MASK_CLASS_GSM_DCS_1800_BAND;
        case telux::tel::RFBand::GSM_1900:
            return TAF_RADIO_BAND_BIT_MASK_GSM_PCS_1900_BAND;
        case telux::tel::RFBand::WCDMA_2100:
            return TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_J_CH_IMT_2100_BAND;
        case telux::tel::RFBand::WCDMA_PCS_1900:
            return TAF_RADIO_BAND_BIT_MASK_WCDMA_US_PCS_1900_BAND;
        case telux::tel::RFBand::WCDMA_DCS_1800:
            return TAF_RADIO_BAND_BIT_MASK_EU_CH_DCS_1800_BAND;
        case telux::tel::RFBand::WCDMA_1700_US:
            return TAF_RADIO_BAND_BIT_MASK_WCDMA_US_1700_BAND;
        case telux::tel::RFBand::WCDMA_850:
            return TAF_RADIO_BAND_BIT_MASK_WCDMA_US_850_BAND;
        case telux::tel::RFBand::WCDMA_800:
            return TAF_RADIO_BAND_BIT_MASK_WCDMA_JAPAN_800_BAND;
        case telux::tel::RFBand::WCDMA_2600:
            return TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_2600_BAND;
        case telux::tel::RFBand::WCDMA_900:
            return TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_J_900_BAND;
        case telux::tel::RFBand::WCDMA_1700_JAPAN:
            return TAF_RADIO_BAND_BIT_MASK_WCDMA_EU_J_1700_BAND;
        case telux::tel::RFBand::WCDMA_1500_JAPAN:
            return TAF_RADIO_BAND_BIT_MASK_WCDMA_JAPAN_1500_BAND;
        case telux::tel::RFBand::WCDMA_850_JAPAN:
            return TAF_RADIO_BAND_BIT_MASK_WCDMA_JAPAN_850_BAND;
        case telux::tel::RFBand::TDSCDMA_BAND_A:
            return TAF_RADIO_BAND_BIT_MASK_TDSCDMA_BAND_A;
        case telux::tel::RFBand::TDSCDMA_BAND_B:
            return TAF_RADIO_BAND_BIT_MASK_TDSCDMA_BAND_B;
        case telux::tel::RFBand::TDSCDMA_BAND_C:
            return TAF_RADIO_BAND_BIT_MASK_TDSCDMA_BAND_C;
        case telux::tel::RFBand::TDSCDMA_BAND_D:
            return TAF_RADIO_BAND_BIT_MASK_TDSCDMA_BAND_D;
        case telux::tel::RFBand::TDSCDMA_BAND_E:
            return TAF_RADIO_BAND_BIT_MASK_TDSCDMA_BAND_E;
        case telux::tel::RFBand::TDSCDMA_BAND_F:
            return TAF_RADIO_BAND_BIT_MASK_TDSCDMA_BAND_F;
        default:
            LE_DEBUG("Invalid active band.");
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Covert LTE active band.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_CovertLteActiveBand
(
    telux::tel::RFBand band ///< [IN] LTE active band
)
{
    switch (band)
    {
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_1:
            return 1;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_2:
            return 2;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_3:
            return 3;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_4:
            return 4;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_5:
            return 5;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_6:
            return 6;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_7:
            return 7;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_8:
            return 8;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_9:
            return 9;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_10:
            return 10;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_11:
            return 11;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_12:
            return 12;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_13:
            return 13;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_14:
            return 14;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_17:
            return 17;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_18:
            return 18;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_19:
            return 19;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_20:
            return 20;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_21:
            return 21;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_23:
            return 23;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_24:
            return 24;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_25:
            return 25;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_26:
            return 26;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_27:
            return 27;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_28:
            return 28;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_29:
            return 29;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_30:
            return 30;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_31:
            return 31;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_32:
            return 32;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_33:
            return 33;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_34:
            return 34;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_35:
            return 35;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_36:
            return 36;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_37:
            return 37;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_38:
            return 38;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_39:
            return 39;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_40:
            return 40;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_41:
            return 41;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_42:
            return 42;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_43:
            return 43;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_46:
            return 46;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_47:
            return 47;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_48:
            return 48;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_49:
            return 49;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_53:
            return 53;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_66:
            return 66;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_67:
            return 67;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_68:
            return 68;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_70:
            return 70;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_71:
            return 71;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_72:
            return 72;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_73:
            return 73;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_85:
            return 85;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_86:
            return 86;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_87:
            return 87;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_88:
            return 88;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_125:
            return 125;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_126:
            return 126;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_127:
            return 127;
        case telux::tel::RFBand::E_UTRA_OPERATING_BAND_250:
            return 250;
        default:
            LE_DEBUG("Invalid LTE active band.");
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Covert NR5G active band.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_CovertNrActiveBand
(
    telux::tel::RFBand band ///< [IN] NR5G active band
)
{
    switch (band)
    {
        case telux::tel::RFBand::NR5G_BAND_1:
            return 1;
        case telux::tel::RFBand::NR5G_BAND_2:
            return 2;
        case telux::tel::RFBand::NR5G_BAND_3:
            return 3;
        case telux::tel::RFBand::NR5G_BAND_5:
            return 5;
        case telux::tel::RFBand::NR5G_BAND_7:
            return 7;
        case telux::tel::RFBand::NR5G_BAND_8:
            return 8;
        case telux::tel::RFBand::NR5G_BAND_12:
            return 12;
        case telux::tel::RFBand::NR5G_BAND_13:
            return 13;
        case telux::tel::RFBand::NR5G_BAND_14:
            return 14;
        case telux::tel::RFBand::NR5G_BAND_18:
            return 18;
        case telux::tel::RFBand::NR5G_BAND_20:
            return 20;
        case telux::tel::RFBand::NR5G_BAND_25:
            return 25;
        case telux::tel::RFBand::NR5G_BAND_26:
            return 26;
        case telux::tel::RFBand::NR5G_BAND_28:
            return 28;
        case telux::tel::RFBand::NR5G_BAND_29:
            return 29;
        case telux::tel::RFBand::NR5G_BAND_30:
            return 30;
        case telux::tel::RFBand::NR5G_BAND_34:
            return 34;
        case telux::tel::RFBand::NR5G_BAND_38:
            return 38;
        case telux::tel::RFBand::NR5G_BAND_39:
            return 39;
        case telux::tel::RFBand::NR5G_BAND_40:
            return 40;
        case telux::tel::RFBand::NR5G_BAND_41:
            return 41;
        case telux::tel::RFBand::NR5G_BAND_46:
            return 46;
        case telux::tel::RFBand::NR5G_BAND_48:
            return 48;
        case telux::tel::RFBand::NR5G_BAND_50:
            return 50;
        case telux::tel::RFBand::NR5G_BAND_51:
            return 51;
        case telux::tel::RFBand::NR5G_BAND_53:
            return 53;
        case telux::tel::RFBand::NR5G_BAND_65:
            return 65;
        case telux::tel::RFBand::NR5G_BAND_66:
            return 66;
        case telux::tel::RFBand::NR5G_BAND_70:
            return 70;
        case telux::tel::RFBand::NR5G_BAND_71:
            return 71;
        case telux::tel::RFBand::NR5G_BAND_74:
            return 74;
        case telux::tel::RFBand::NR5G_BAND_75:
            return 75;
        case telux::tel::RFBand::NR5G_BAND_76:
            return 76;
        case telux::tel::RFBand::NR5G_BAND_77:
            return 77;
        case telux::tel::RFBand::NR5G_BAND_78:
            return 78;
        case telux::tel::RFBand::NR5G_BAND_79:
            return 79;
        case telux::tel::RFBand::NR5G_BAND_80:
            return 80;
        case telux::tel::RFBand::NR5G_BAND_81:
            return 81;
        case telux::tel::RFBand::NR5G_BAND_82:
            return 82;
        case telux::tel::RFBand::NR5G_BAND_83:
            return 83;
        case telux::tel::RFBand::NR5G_BAND_84:
            return 84;
        case telux::tel::RFBand::NR5G_BAND_85:
            return 85;
        case telux::tel::RFBand::NR5G_BAND_86:
            return 86;
        case telux::tel::RFBand::NR5G_BAND_91:
            return 91;
        case telux::tel::RFBand::NR5G_BAND_92:
            return 92;
        case telux::tel::RFBand::NR5G_BAND_93:
            return 93;
        case telux::tel::RFBand::NR5G_BAND_94:
            return 94;
        case telux::tel::RFBand::NR5G_BAND_257:
            return 257;
        case telux::tel::RFBand::NR5G_BAND_258:
            return 258;
        case telux::tel::RFBand::NR5G_BAND_259:
            return 259;
        case telux::tel::RFBand::NR5G_BAND_260:
            return 260;
        case telux::tel::RFBand::NR5G_BAND_261:
            return 261;
        default:
            LE_DEBUG("Invalid NR5G active band.");
    }

    return 0;
}

static taf_radio_Rat_t ConvertPaRatToSvcRat(taf_pa_radio_Rat_t paRat)
{
    switch (paRat)
    {
        case TAF_PA_RADIO_RAT_GSM:    return TAF_RADIO_RAT_GSM;
        case TAF_PA_RADIO_RAT_WCDMA:  return TAF_RADIO_RAT_UMTS;
        case TAF_PA_RADIO_RAT_LTE:    return TAF_RADIO_RAT_LTE;
        case TAF_PA_RADIO_RAT_NR5G:   return TAF_RADIO_RAT_NR5G;
        case TAF_PA_RADIO_RAT_UNKNOWN:
        default:
            LE_WARN("ConvertPaRatToSvcRat: unmapped PA RAT=%d, defaulting to UNKNOWN", paRat);
            return TAF_RADIO_RAT_UNKNOWN;
    }
}

static taf_radio_RatSvcStatus_t ConvertPaSvcStatusToSvcStatus(taf_pa_radio_ServiceStatus_t paStatus)
{
    switch (paStatus)
    {
        case TAF_PA_RADIO_SRV_STATUS_NO_SRV:           return TAF_RADIO_RAT_SVC_STATUS_NO_SERVICE;
        case TAF_PA_RADIO_SRV_STATUS_LIMITED:          return TAF_RADIO_RAT_SVC_STATUS_LIMITED;
        case TAF_PA_RADIO_SRV_STATUS_SRV:              return TAF_RADIO_RAT_SVC_STATUS_SERVICE;
        case TAF_PA_RADIO_SRV_STATUS_LIMITED_REGIONAL: return TAF_RADIO_RAT_SVC_STATUS_LIMITED_REGIONAL;
        case TAF_PA_RADIO_SRV_STATUS_PWR_SAVE:         return TAF_RADIO_RAT_SVC_STATUS_POWER_SAVE;
        case TAF_PA_RADIO_SRV_STATUS_UNKNOWN:
        default:
            LE_WARN("ConvertPaSvcStatusToSvcStatus: unmapped PA status=%d, defaulting to UNKNOWN", paStatus);
            return TAF_RADIO_RAT_SVC_STATUS_UNKNOWN;
    }
}

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
 * Initiate listener for telephony serving system.
 */
//--------------------------------------------------------------------------------------------------
taf_RadioServSysListener::taf_RadioServSysListener
(
    uint8_t phoneId ///< [IN] Slot ID.
)
{
    phone = phoneId;
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for serving system information change.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioServSysListener::onSystemInfoChanged
(
    telux::tel::ServingSystemInfo sysInfo ///< [IN] Serving system information.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioServSysListener --> onSystemInfoChanged");

    if (sysInfo.domain != serviceDomain)
    {
        serviceDomain = sysInfo.domain;
        auto &tafRadio = taf_Radio::GetInstance();
        taf_RadioNetStatusInd_t* statusPtr =
            (taf_RadioNetStatusInd_t*)le_mem_ForceAlloc(tafRadio.netStatusPool);
        statusPtr->phoneId = phone;
        statusPtr->bitmask = TAF_RADIO_NET_STATUS_IND_BIT_MASK_SVC_DOMAIN;
        statusPtr->netStatusRef = tafRadio.netStatusRefs[phone - 1];
        le_event_ReportWithRefCounting(tafRadio.netStatusEvId, (void*)statusPtr);
    }

    if (sysInfo.rat != rat)
    {
        rat = sysInfo.rat;
        auto &tafRadio = taf_Radio::GetInstance();
        taf_radio_RatChangeInd_t* ratChangeIndPtr =
            (taf_radio_RatChangeInd_t*)le_mem_ForceAlloc(tafRadio.ratChangePool);
        ratChangeIndPtr->rat = tafRadio.taf_radio_CovertRat(sysInfo.rat);
        ratChangeIndPtr->phoneId = phone;
        le_event_ReportWithRefCounting(tafRadio.ratChangeEvId, (void*)ratChangeIndPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for network rejection.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioServSysListener::onNetworkRejection
(
    telux::tel::NetworkRejectInfo rejectInfo ///< [IN] Network rejection information.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioServSysListener --> onNetworkRejection");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_radio_NetRegRejInd_t* netRegRejIndPtr =
        (taf_radio_NetRegRejInd_t*)le_mem_ForceAlloc(tafRadio.netRegRejPool);
    netRegRejIndPtr->rat = tafRadio.taf_radio_CovertRat(rejectInfo.rejectSrvInfo.rat);
    le_utf8_Copy(netRegRejIndPtr->mcc, rejectInfo.mcc.c_str(), TAF_RADIO_MCC_BYTES, NULL);
    le_utf8_Copy(netRegRejIndPtr->mnc, rejectInfo.mnc.c_str(), TAF_RADIO_MNC_BYTES, NULL);
    switch (rejectInfo.rejectSrvInfo.domain)
    {
        case telux::tel::ServiceDomain::NO_SRV:
            netRegRejIndPtr->domain = TAF_RADIO_SERVICE_DOMAIN_STATE_NO_SVC;
            break;
        case telux::tel::ServiceDomain::CS_ONLY:
            netRegRejIndPtr->domain = TAF_RADIO_SERVICE_DOMAIN_STATE_CS_ONLY;
            break;
        case telux::tel::ServiceDomain::PS_ONLY:
            netRegRejIndPtr->domain = TAF_RADIO_SERVICE_DOMAIN_STATE_PS_ONLY;
            break;
        case telux::tel::ServiceDomain::CS_PS:
            netRegRejIndPtr->domain = TAF_RADIO_SERVICE_DOMAIN_STATE_CS_AND_PS;
            break;
        case telux::tel::ServiceDomain::CAMPED:
            netRegRejIndPtr->domain = TAF_RADIO_SERVICE_DOMAIN_STATE_CAMPED;
            break;
        case telux::tel::ServiceDomain::UNKNOWN:
        default:
            netRegRejIndPtr->domain = TAF_RADIO_SERVICE_DOMAIN_STATE_UNKNOWN;
            break;
    }
    netRegRejIndPtr->phoneId = phone;
    netRegRejIndPtr->cause = (taf_radio_NetRejCause_t)rejectInfo.rejectCause;
    tafRadio.netRejectCause = (int32_t)rejectInfo.rejectCause;
    le_event_ReportWithRefCounting(tafRadio.netRegRejEvId, (void*)netRegRejIndPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for LTE CS capability change.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioServSysListener::onLteCsCapabilityChanged
(
    telux::tel::LteCsCapability lteCapability ///< [IN] LTE CS Capability.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioServSysListener --> onLteCsCapabilityChanged");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioNetStatusInd_t* statusPtr =
        (taf_RadioNetStatusInd_t*)le_mem_ForceAlloc(tafRadio.netStatusPool);
    statusPtr->phoneId = phone;
    statusPtr->bitmask = TAF_RADIO_NET_STATUS_IND_BIT_MASK_LTE_CS_CAP;
    statusPtr->netStatusRef = tafRadio.netStatusRefs[phone - 1];
    le_event_ReportWithRefCounting(tafRadio.netStatusEvId, (void*)statusPtr);
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
    LE_DEBUG("<SDK Listener> taf_RadioImsServSysListener --> onImsRegStatusChange");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioImsStatus_t* statusPtr =
        (taf_RadioImsStatus_t*)le_mem_ForceAlloc(tafRadio.imsStatusChangePool);
    statusPtr->phoneId = phone;
    statusPtr->status = (taf_radio_ImsRegStatus_t)status.imsRegStatus;
    statusPtr->imsRef = tafRadio.imsRefs[phone - 1];
    le_event_ReportWithRefCounting(tafRadio.imsRegStatusChangeId, (void*)statusPtr);
}

#ifdef LE_CONFIG_FEATURE_ENHANCED_IMS
//--------------------------------------------------------------------------------------------------
/**
 * Listener for IMS service information.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsServSysListener::onImsServiceInfoChange
(
    telux::tel::ImsServiceInfo service ///< [IN] IMS service information.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioImsServSysListener --> onImsServiceInfoChange");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioImsStatus_t* statusPtr =
        (taf_RadioImsStatus_t*)le_mem_ForceAlloc(tafRadio.imsStatusChangePool);
    statusPtr->bitmask = TAF_RADIO_IMS_IND_BIT_MASK_SVC_INFO;
    statusPtr->phoneId = phone;
    statusPtr->imsRef = tafRadio.imsRefs[phone - 1];
    le_event_ReportWithRefCounting(tafRadio.imsStatusChangeId, (void*)statusPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for IMS PDP status.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsServSysListener::onImsPdpStatusInfoChange
(
    telux::tel::ImsPdpStatusInfo status ///< [IN] IMS PDP status.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioImsServSysListener --> onImsPdpStatusInfoChange");

    auto &tafRadio = taf_Radio::GetInstance();
    taf_RadioImsStatus_t* statusPtr =
        (taf_RadioImsStatus_t*)le_mem_ForceAlloc(tafRadio.imsStatusChangePool);
    statusPtr->bitmask = TAF_RADIO_IMS_IND_BIT_MASK_PDP_ERROR;
    statusPtr->phoneId = phone;
    statusPtr->imsRef = tafRadio.imsRefs[phone - 1];
    le_event_ReportWithRefCounting(tafRadio.imsStatusChangeId, (void*)statusPtr);
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Listener for operating mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioPhoneListener::onOperatingModeChanged
(
    telux::tel::OperatingMode mode ///< [IN] Operating mode.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioPhoneListener --> onOperatingModeChanged");

    auto &tafRadio = taf_Radio::GetInstance();

    taf_radio_OpMode_t* modePtr =
        (taf_radio_OpMode_t*)le_mem_ForceAlloc(tafRadio.opModeChangePool);
	*modePtr = (taf_radio_OpMode_t)mode;
    le_event_ReportWithRefCounting(tafRadio.opModeChangeId, (void*)modePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for voice service state.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioPhoneListener::onVoiceServiceStateChanged
(
    int phoneId,
        ///< [IN] Phone ID.
    const std::shared_ptr<telux::tel::VoiceServiceInfo> &srvInfo
        ///< [IN] Voice service information.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioPhoneListener --> onVoiceServiceStateChanged");

    auto &tafRadio = taf_Radio::GetInstance();

    taf_radio_NetRegStateInd_t* indPtr =
        (taf_radio_NetRegStateInd_t*)le_mem_ForceAlloc(tafRadio.netRegStatePool);
    indPtr->phoneId = phoneId;
    if (srvInfo != nullptr)
    {
        switch (srvInfo->getVoiceServiceState())
        {
            case telux::tel::VoiceServiceState::NOT_REG_AND_NOT_SEARCHING:
                indPtr->state = TAF_RADIO_NET_REG_STATE_NONE;
                break;
            case telux::tel::VoiceServiceState::REG_HOME:
                indPtr->state = TAF_RADIO_NET_REG_STATE_HOME;
                break;
            case telux::tel::VoiceServiceState::NOT_REG_AND_SEARCHING:
                indPtr->state = TAF_RADIO_NET_REG_STATE_SEARCHING;
                break;
            case telux::tel::VoiceServiceState::REG_DENIED:
                indPtr->state = TAF_RADIO_NET_REG_STATE_DENIED;
                break;
            case telux::tel::VoiceServiceState::REG_ROAMING:
                indPtr->state = TAF_RADIO_NET_REG_STATE_ROAMING;
                break;
            case telux::tel::VoiceServiceState::NOT_REG_AND_EMERGENCY_AVAILABLE_AND_NOT_SEARCHING:
                indPtr->state = TAF_RADIO_NET_REG_STATE_NONE_AND_EMERGENCY_AVAILABLE;
                break;
            case telux::tel::VoiceServiceState::NOT_REG_AND_EMERGENCY_AVAILABLE_AND_SEARCHING:
                indPtr->state = TAF_RADIO_NET_REG_STATE_SEARCHING_AND_EMERGENCY_AVAILABLE;
                break;
            case telux::tel::VoiceServiceState::REG_DENIED_AND_EMERGENCY_AVAILABLE:
                indPtr->state = TAF_RADIO_NET_REG_STATE_DENIED_AND_EMERGENCY_AVAILABLE;
                break;
            case telux::tel::VoiceServiceState::UNKNOWN_AND_EMERGENCY_AVAILABLE:
                indPtr->state= TAF_RADIO_NET_REG_STATE_UNKNOWN_AND_EMERGENCY_AVAILABLE;
                break;
            default:
                indPtr->state = TAF_RADIO_NET_REG_STATE_UNKNOWN;
        }
    }
    else
    {
        indPtr->state = TAF_RADIO_NET_REG_STATE_UNKNOWN;
    }
    le_event_ReportWithRefCounting(tafRadio.netRegStateEvId, (void*)indPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for operating mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioPhoneListener::onSignalStrengthChanged
(
    int phoneId,                                               ///< [IN] Phone ID.
    std::shared_ptr<telux::tel::SignalStrength> signalStrength ///< [IN] Signal strength.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioPhoneListener --> onSignalStrengthChanged");

    auto &tafRadio = taf_Radio::GetInstance();

    if (signalStrength->getGsmSignalStrength() != nullptr &&
        signalStrength->getGsmSignalStrength()->getGsmSignalStrength() !=
        INVALID_SIGNAL_STRENGTH_VALUE)
    {
        taf_RadioSsInd_t* ssPtr = (taf_RadioSsInd_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
        ssPtr->phoneId = phoneId;
        ssPtr->rssi = signalStrength->getGsmSignalStrength()->getDbm();
        ssPtr->rsrp = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        le_event_ReportWithRefCounting(tafRadio.gsmSsChangeEvId, (void*)ssPtr);
    }

    if (signalStrength->getCdmaSignalStrength() != nullptr &&
        signalStrength->getCdmaSignalStrength()->getDbm() != INVALID_SIGNAL_STRENGTH_VALUE)
    {
        taf_RadioSsInd_t* ssPtr = (taf_RadioSsInd_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
        ssPtr->phoneId = phoneId;
        ssPtr->rssi = signalStrength->getCdmaSignalStrength()->getDbm();
        ssPtr->rsrp = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        le_event_ReportWithRefCounting(tafRadio.cdmaSsChangeEvId, (void*)ssPtr);
    }

    if (signalStrength->getWcdmaSignalStrength() != nullptr &&
        signalStrength->getWcdmaSignalStrength()->getSignalStrength() !=
        INVALID_SIGNAL_STRENGTH_VALUE)
    {
        taf_RadioSsInd_t* ssPtr = (taf_RadioSsInd_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
        ssPtr->phoneId = phoneId;
        ssPtr->rssi = signalStrength->getWcdmaSignalStrength()->getDbm();
        ssPtr->rsrp = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        le_event_ReportWithRefCounting(tafRadio.umtsSsChangeEvId, (void*)ssPtr);
    }

    if (signalStrength->getLteSignalStrength() != nullptr &&
        signalStrength->getLteSignalStrength()->getLteSignalStrength() !=
        INVALID_SIGNAL_STRENGTH_VALUE)
    {
        taf_RadioSsInd_t* ssPtr = (taf_RadioSsInd_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
        ssPtr->phoneId = phoneId;
        ssPtr->rssi = signalStrength->getLteSignalStrength()->getRssi();
        ssPtr->rsrp = signalStrength->getLteSignalStrength()->getDbm();
        le_event_ReportWithRefCounting(tafRadio.lteSsChangeEvId, (void*)ssPtr);
    }

    if (signalStrength->getNr5gSignalStrength() != nullptr &&
        signalStrength->getNr5gSignalStrength()->getDbm() != INVALID_SIGNAL_STRENGTH_VALUE)
    {
        taf_RadioSsInd_t* ssPtr = (taf_RadioSsInd_t*)le_mem_ForceAlloc(tafRadio.ssChangePool);
        ssPtr->phoneId = phoneId;
        ssPtr->rssi = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
        ssPtr->rsrp = signalStrength->getNr5gSignalStrength()->getDbm();
        le_event_ReportWithRefCounting(tafRadio.nr5gSsChangeEvId, (void*)ssPtr);
    }
}

void taf_RadioPhoneListener::onCellInfoListChanged
(
    int phoneId,                                                      ///< [IN] Phone ID.
    std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList   ///< [IN] Cell change info.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioPhoneListener --> onCellInfoListChanged");

    auto &tafRadio = taf_Radio::GetInstance();

    TAF_ERROR_IF_RET_NIL(tafRadio.phones[phoneId - 1] == NULL,
        "Invalid para(null ptr, phoneId:%d)", phoneId);

    TAF_ERROR_IF_RET_NIL(cellInfoList.size() == 0,
        "Invalid para(celllist size NULL, phoneId:%d)", phoneId);

    taf_RadioCellInfoInd_t* cellInfoPtr = (taf_RadioCellInfoInd_t*)le_mem_ForceAlloc(
                                                                   tafRadio.cellInfoChangePool);

    cellInfoPtr->phoneId = phoneId;
    bool registered = false;
    bool neighbor = false;
    for (auto cellInfo : cellInfoList)
    {
        if (cellInfo == NULL)
        {
            LE_ERROR("Cell information pointer is NULL.");
            break;
        }
        if(cellInfo->isRegistered())
        {
           registered = true;
           cellInfoPtr->cellInfoStatus = TAF_RADIO_CELL_SERVING_CHANGED;
        }
        else
        {
            neighbor = true;
            cellInfoPtr->cellInfoStatus = TAF_RADIO_CELL_NEIGHBOR_CHANGED;
        }
    }

    if(registered && neighbor)
    {
      cellInfoPtr->cellInfoStatus = TAF_RADIO_CELL_SERVING_AND_NEIGHBOR_CHANGED;
    }

    le_event_ReportWithRefCounting(tafRadio.cellInfoChangeEvId, (void*)cellInfoPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initiate listener for Data serving system.
 */
//--------------------------------------------------------------------------------------------------
taf_RadioDataServSysListener::taf_RadioDataServSysListener
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
 * Listener for Data service state.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioDataServSysListener::onServiceStateChanged
(
    telux::data::ServiceStatus status ///< [IN] Data service state.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioDataServSysListener --> onServiceStateChanged");

    auto &tafRadio = taf_Radio::GetInstance();

    taf_radio_NetRegStateInd_t* indPtr;
    taf_radio_NetRegState_t nextState = TAF_RADIO_NET_REG_STATE_UNKNOWN;

    switch (status.serviceState)
    {
        case telux::data::DataServiceState::IN_SERVICE:
            inService = true;
            if (isRoaming)
            {
                nextState = TAF_RADIO_NET_REG_STATE_ROAMING;
            }
            else
            {
                nextState = TAF_RADIO_NET_REG_STATE_HOME;
            }
            break;
        case telux::data::DataServiceState::OUT_OF_SERVICE:
            inService = false;
            nextState = TAF_RADIO_NET_REG_STATE_NONE;
            break;
        default:
            nextState = TAF_RADIO_NET_REG_STATE_UNKNOWN;
            break;
    }

    if (nextState != currState)
    {
        currState = nextState;
        indPtr = (taf_radio_NetRegStateInd_t*)le_mem_ForceAlloc(tafRadio.packSwStatePool);
        indPtr->phoneId = phone;
        indPtr->state = currState;
        le_event_ReportWithRefCounting(tafRadio.packSwStateEvId, (void*)indPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for Data roaming state.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioDataServSysListener::onRoamingStatusChanged
(
    telux::data::RoamingStatus status ///< [IN] Data roaming state.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioDataServSysListener --> onRoamingStatusChanged");

    auto &tafRadio = taf_Radio::GetInstance();

    taf_radio_NetRegStateInd_t* indPtr;
    taf_radio_NetRegState_t nextState = TAF_RADIO_NET_REG_STATE_UNKNOWN;

    isRoaming = status.isRoaming;
    if (isRoaming)
    {
        nextState = TAF_RADIO_NET_REG_STATE_ROAMING;
    }
    else
    {
        if (inService)
        {
            nextState = TAF_RADIO_NET_REG_STATE_HOME;
        }
        else
        {
            nextState = TAF_RADIO_NET_REG_STATE_NONE;
        }
    }

    if (nextState != currState)
    {
        currState = nextState;
        indPtr = (taf_radio_NetRegStateInd_t*)le_mem_ForceAlloc(tafRadio.packSwStatePool);
        indPtr->phoneId = phone;
        indPtr->state = currState;
        le_event_ReportWithRefCounting(tafRadio.packSwStateEvId, (void*)indPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Listener for NR icon type.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioDataServSysListener::onNrIconTypeChanged
(
    telux::data::NrIconType type ///< [IN] Data roaming state.
)
{
    LE_DEBUG("<SDK Listener> taf_RadioDataServSysListener --> onNrIconTypeChanged");

    auto &tafRadio = taf_Radio::GetInstance();

    taf_RadioNrIconTypeInd_t* indPtr = (taf_RadioNrIconTypeInd_t*)le_mem_ForceAlloc(
        tafRadio.nrIconTypePool);
    indPtr->phoneId = phone;
    indPtr->type = tafRadio.taf_radio_ConvertNrIconType(type);
    le_event_ReportWithRefCounting(tafRadio.nrIconTypeEvId, (void*)indPtr);
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
    LE_DEBUG("<SDK Callback> taf_RadioGetOperatingModeCallback --> operatingModeResponse");

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

    ssMetrics.gsm.ss = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    ssMetrics.gsm.ber = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;

    ssMetrics.cdma.ss = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    ssMetrics.cdma.ecio = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    ssMetrics.cdma.io = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    ssMetrics.cdma.snr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;

    ssMetrics.umts.ss = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    ssMetrics.umts.ber = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    ssMetrics.umts.rscp = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;

    ssMetrics.tdscdma.rscp = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;

    ssMetrics.lte.ss = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    ssMetrics.lte.rsrq = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    ssMetrics.lte.rsrp = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    ssMetrics.lte.snr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;

    ssMetrics.nr5g.rsrq = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    ssMetrics.nr5g.rsrp = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;
    ssMetrics.nr5g.snr = TAF_RADIO_INVALID_SIGNAL_STRENGTH_VALUE;

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        result = LE_OK;
    }

    if (signalStrength->getGsmSignalStrength() != nullptr)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_GSM;
        ssMetrics.gsm.sslv = (int8_t)signalStrength->getGsmSignalStrength()->getLevel() + 1;
        ssMetrics.gsm.ss = signalStrength->getGsmSignalStrength()->getDbm();
        ssMetrics.gsm.ber = signalStrength->getGsmSignalStrength()->getGsmBitErrorRate();
    }

    if (signalStrength->getCdmaSignalStrength() != nullptr)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_CDMA;
        ssMetrics.cdma.sslv = (int8_t)signalStrength->getCdmaSignalStrength()->getLevel() + 1;
        ssMetrics.cdma.ss = signalStrength->getCdmaSignalStrength()->getDbm();
        ssMetrics.cdma.ecio = signalStrength->getCdmaSignalStrength()->getCdmaEcio();
        ssMetrics.cdma.io = signalStrength->getCdmaSignalStrength()->getEvdoEcio();
        ssMetrics.cdma.snr = signalStrength->getCdmaSignalStrength()->getEvdoSignalNoiseRatio();
    }

    if (signalStrength->getWcdmaSignalStrength() != nullptr)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_UMTS;
        ssMetrics.umts.sslv = (int8_t)signalStrength->getWcdmaSignalStrength()->getLevel() + 1;
        ssMetrics.umts.ss = signalStrength->getWcdmaSignalStrength()->getDbm();
        ssMetrics.umts.ber = signalStrength->getWcdmaSignalStrength()->getBitErrorRate();
        ssMetrics.umts.rscp = signalStrength->getWcdmaSignalStrength()->getRscp();
    }

    if (signalStrength->getTdscdmaSignalStrength() != nullptr)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_TDSCDMA;
        ssMetrics.tdscdma.rscp = signalStrength->getTdscdmaSignalStrength()->getRscp();
    }

    if (signalStrength->getLteSignalStrength() != nullptr)
    {
        ssMetrics.ratMask |= TAF_RADIO_RAT_BIT_MASK_LTE;
        ssMetrics.lte.sslv = (int8_t)signalStrength->getLteSignalStrength()->getLevel() + 1;
        ssMetrics.lte.ss = signalStrength->getLteSignalStrength()->getDbm();
        ssMetrics.lte.rsrq = signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality();
        ssMetrics.lte.rsrp = signalStrength->getLteSignalStrength()->getDbm();
        ssMetrics.lte.snr = signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr();
    }

    if (signalStrength->getNr5gSignalStrength() != nullptr)
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
 * Response for getting cellular capability.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioCellularCapsCallback::cellularCapabilityResponse
(
     telux::tel::CellularCapabilityInfo   capabilityInfo,  ///< [IN] Cellular Capability.
     telux::common::ErrorCode error                        ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioCellularCapsCallback --> cellularCapabilityResponse");

    if (error == telux::common::ErrorCode::SUCCESS)
    {
        simCount = capabilityInfo.simCount;
        maxActiveSIM = capabilityInfo.maxActiveSims;

        for (auto simCaps : capabilityInfo.simRatCapabilities)
        {
            simRatCaps.emplace_back(simCaps);
        }
        for (auto deviceCaps : capabilityInfo.deviceRatCapability)
        {
            deviceRatCaps.emplace_back(deviceCaps);
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
 * Semaphore for configuring signal strength.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioConfigureSignalStrengthCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of configuring signal strength.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioConfigureSignalStrengthCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Response for configuring signal strength.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioConfigureSignalStrengthCallback::configureSignalStrengthResponse
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioConfigureSignalStrengthCallback --> \
        configureSignalStrengthResponse");

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
 * Response for getting voice service state.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioVoiceServiceStateCallback::voiceServiceStateResponse
(
    const std::shared_ptr<telux::tel::VoiceServiceInfo> &serviceInfo,
        ///< [IN] Voice service information.
    telux::common::ErrorCode error
        ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioVoiceServiceStateCallback --> voiceServiceStateResponse");

    if (error != telux::common::ErrorCode::SUCCESS || serviceInfo == nullptr)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        voiceSvcState = serviceInfo->getVoiceServiceState();
        result = LE_OK;
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
 * Semaphore for getting service domain preference.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioServiceDomainPreferenceResponseCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of getting service domain preference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioServiceDomainPreferenceResponseCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Service domain preference.
 */
//--------------------------------------------------------------------------------------------------
telux::tel::ServiceDomainPreference taf_RadioServiceDomainPreferenceResponseCallback::domainPref
    = telux::tel::ServiceDomainPreference::UNKNOWN;

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting service domain preference.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioServiceDomainPreferenceResponseCallback::serviceDomainPrefResponse
(
    telux::tel::ServiceDomainPreference preference, ///< [IN] Service domain preference.
    telux::common::ErrorCode error                  ///< [IN] Error code.
)
{
    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        domainPref = preference;
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
                    cellIdInfo.gsm.arfcn = gsmCellInfo->getCellIdentity().getArfcn();
                    cellIdInfo.ss = gsmCellInfo->getSignalStrengthInfo().getDbm();
                    le_utf8_Copy(cellIdInfo.mcc,
                        gsmCellInfo->getCellIdentity().getMobileCountryCode().c_str(),
                        TAF_RADIO_MCC_BYTES, NULL);
                    le_utf8_Copy(cellIdInfo.mnc,
                        gsmCellInfo->getCellIdentity().getMobileNetworkCode().c_str(),
                        TAF_RADIO_MNC_BYTES, NULL);
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
                    memset(cellIdInfo.mcc, 0, TAF_RADIO_MCC_BYTES);
                    memset(cellIdInfo.mnc, 0, TAF_RADIO_MNC_BYTES);
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
                    cellIdInfo.umts.uarfcn = umtsCellInfo->getCellIdentity().getUarfcn();
                    cellIdInfo.ss = umtsCellInfo->getSignalStrengthInfo().getDbm();
                    le_utf8_Copy(cellIdInfo.mcc,
                        umtsCellInfo->getCellIdentity().getMobileCountryCode().c_str(),
                        TAF_RADIO_MCC_BYTES, NULL);
                    le_utf8_Copy(cellIdInfo.mnc,
                        umtsCellInfo->getCellIdentity().getMobileNetworkCode().c_str(),
                        TAF_RADIO_MNC_BYTES, NULL);
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
                    le_utf8_Copy(cellIdInfo.mcc,
                        tdscdmaCellInfo->getCellIdentity().getMobileCountryCode().c_str(),
                        TAF_RADIO_MCC_BYTES, NULL);
                    le_utf8_Copy(cellIdInfo.mnc,
                        tdscdmaCellInfo->getCellIdentity().getMobileNetworkCode().c_str(),
                        TAF_RADIO_MNC_BYTES, NULL);
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
                    le_utf8_Copy(cellIdInfo.mcc,
                        lteCellInfo->getCellIdentity().getMobileCountryCode().c_str(),
                        TAF_RADIO_MCC_BYTES, NULL);
                    le_utf8_Copy(cellIdInfo.mnc,
                        lteCellInfo->getCellIdentity().getMobileNetworkCode().c_str(),
                        TAF_RADIO_MNC_BYTES, NULL);
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
                    le_utf8_Copy(cellIdInfo.mcc,
                        nr5gCellInfo->getCellIdentity().getMobileCountryCode().c_str(),
                        TAF_RADIO_MCC_BYTES, NULL);
                    le_utf8_Copy(cellIdInfo.mnc,
                        nr5gCellInfo->getCellIdentity().getMobileNetworkCode().c_str(),
                        TAF_RADIO_MNC_BYTES, NULL);
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

#ifdef LE_CONFIG_FEATURE_ENHANCED_IMS
//--------------------------------------------------------------------------------------------------
/**
 * IMS VoIP service status.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ImsSvcStatus_t taf_RadioImsServSysCallback::voip = TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;

//--------------------------------------------------------------------------------------------------
/**
 * IMS SMS service status.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ImsSvcStatus_t taf_RadioImsServSysCallback::sms = TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;

//--------------------------------------------------------------------------------------------------
/**
 * IMS PDP error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PdpError_t taf_RadioImsServSysCallback::pdpError = TAF_RADIO_PDP_ERROR_UNKNOWN;

//--------------------------------------------------------------------------------------------------
/**
 * Convert IMS service status.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ImsSvcStatus_t ConvertImsSvcStatus
(
    telux::tel::CellularServiceStatus status ///< [IN] IMS service status.
)
{
    switch (status)
    {
        case telux::tel::CellularServiceStatus::NO_SERVICE:
            return TAF_RADIO_IMS_SVC_STATUS_UNAVAILABLE;
        case telux::tel::CellularServiceStatus::LIMITED_SERVICE:
            return TAF_RADIO_IMS_SVC_STATUS_LIMITED;
        case telux::tel::CellularServiceStatus::FULL_SERVICE:
            return TAF_RADIO_IMS_SVC_STATUS_FULL_SERVICE;
        default:
            break;
    }

    return TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert IMS PDP error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PdpError_t ConvertImsPdpError
(
    telux::tel::PdpFailureCode error ///< [IN] IMS PDP error.
)
{
    switch (error)
    {
        case telux::tel::PdpFailureCode::OTHER_FAILURE:
            return TAF_RADIO_PDP_ERROR_GENERIC;
        case telux::tel::PdpFailureCode::OPTION_UNSUBSCRIBED:
            return TAF_RADIO_PDP_ERROR_OPTION_UNSUBSCRIBED;
        case telux::tel::PdpFailureCode::UNKNOWN_PDP:
            return TAF_RADIO_PDP_ERROR_UNKNOWN_PDP;
        case telux::tel::PdpFailureCode::REASON_NOT_SPECIFIED:
            return TAF_RADIO_PDP_ERROR_REASON_NOT_SPECIFIED;
        case telux::tel::PdpFailureCode::CONNECTION_BRINGUP_FAILURE:
            return TAF_RADIO_PDP_ERROR_CONNECTION_BRINGUP_FAILURE;
        case telux::tel::PdpFailureCode::CONNECTION_IKE_AUTH_FAILURE:
            return TAF_RADIO_PDP_ERROR_CONNECTION_IKE_AUTH_FAILURE;
        case telux::tel::PdpFailureCode::USER_AUTH_FAILED:
            return TAF_RADIO_PDP_ERROR_USER_AUTH_FAILURE;
        default:
            break;
    }

    return TAF_RADIO_PDP_ERROR_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting IMS service information.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsServSysCallback::imsSvcInfoResponse
(
    telux::tel::ImsServiceInfo info, ///< [IN] IMS service information.
    telux::common::ErrorCode error   ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioImsServSysCallback --> imsSvcInfoResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        sms = ConvertImsSvcStatus(info.sms);
        voip = ConvertImsSvcStatus(info.voice);
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting IMS PDP status.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsServSysCallback::imsPdpStatusResponse
(
    telux::tel::ImsPdpStatusInfo status,       ///< [IN] IMS PDP status.
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioImsServSysCallback --> imsPdpStatusResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        pdpError = ConvertImsPdpError(status.failureCode);
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for IMS settings.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioImsSettingCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of IMS settings.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioImsSettingCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * IMS service configurations.
 */
//--------------------------------------------------------------------------------------------------
telux::tel::ImsServiceConfig taf_RadioImsSettingCallback::config;

//--------------------------------------------------------------------------------------------------
/**
 * IMS sip user agent.
 */
//--------------------------------------------------------------------------------------------------
char taf_RadioImsSettingCallback::sipUserAgentPtr[TAF_RADIO_IMS_USER_AGENT_BYTES] = {0};

//--------------------------------------------------------------------------------------------------
/**
 * IMS Voice over NR.
 */
//--------------------------------------------------------------------------------------------------
bool taf_RadioImsSettingCallback::vonrConfig = true;
//--------------------------------------------------------------------------------------------------
/**
 * Response for setting IMS configurations.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsSettingCallback::onResponseCallback
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioImsSettingCallback --> onResponseCallback");

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
 * Response for getting IMS configurations.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsSettingCallback::onRequestImsServiceConfig
(
    SlotId slotId,                           ///< [IN] Slot ID.
    telux::tel::ImsServiceConfig configType, ///< [IN] IMS service configurations.
    telux::common::ErrorCode error           ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioImsSettingCallback --> onRequestImsServiceConfig");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        config = configType;
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting IMS sip user agent.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsSettingCallback::onRequestImsSipUserAgentConfig
(
    SlotId slotId,                 ///< [IN] Slot ID.
    std::string sipUserAgent,      ///< [IN] IMS service configurations.
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioImsSettingCallback --> onRequestImsSipUserAgentConfig");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        sipUserAgentPtr[0] = '\0';
        if (sipUserAgent.c_str() != NULL)
        {
            le_utf8_Copy(sipUserAgentPtr, sipUserAgent.c_str(),
                TAF_RADIO_IMS_USER_AGENT_BYTES, NULL);
        }
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting Voice over NR.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioImsSettingCallback::onRequestImsVonr
(
    SlotId slotId,                 ///< [IN] Slot ID.
    bool isEnable,                 ///< [IN] Voice over NR configurations.
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioImsSettingCallback --> onRequestImsVonr");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        vonrConfig = isEnable;
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for getting selection mode.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioSelectionModeResponseCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of getting selection mode.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioSelectionModeResponseCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Network selection mode information.
 */
//--------------------------------------------------------------------------------------------------
telux::tel::NetworkModeInfo taf_RadioSelectionModeResponseCallback::selectModeInfo;

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting selection mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioSelectionModeResponseCallback::selectionModeResponse
(
    telux::tel::NetworkModeInfo info, ///< [IN] Network mode information.
    telux::common::ErrorCode error    ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioSelectionModeResponseCallback --> selectionModeResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        result = LE_OK;
    }

    selectModeInfo = info;
    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for getting RF band capability.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioRFBandCapabilityResponseCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of getting RF band capability.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioRFBandCapabilityResponseCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * GSM and WCDMA RF band capability.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_BandBitMask_t taf_RadioRFBandCapabilityResponseCallback::bandCapability = 0;

//--------------------------------------------------------------------------------------------------
/**
 * LTE RF band capability.
 */
//--------------------------------------------------------------------------------------------------
uint64_t taf_RadioRFBandCapabilityResponseCallback::lteBandCapability[TAF_RADIO_LTE_BAND_GROUP_NUM];

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting RF band capability.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioRFBandCapabilityResponseCallback::rfBandCapabilityResponse
(
    std::shared_ptr<telux::tel::IRFBandList> capabilityList, ///< [IN] RF band capability list.
    telux::common::ErrorCode error                           ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioRFBandCapabilityResponseCallback --> rfBandCapabilityResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        for (uint8_t i = 0; i < TAF_RADIO_LTE_BAND_GROUP_NUM; i++)
        {
            lteBandCapability[i] = 0;
        }
        bandCapability = taf_radio_CovertRFBand(capabilityList);
        std::vector<telux::tel::LteRFBand> lteBands = capabilityList->getLteBands();
        for (auto lteBand : lteBands)
        {
            if (lteBand < telux::tel::LteRFBand::E_UTRA_BAND_1 ||
                lteBand > telux::tel::LteRFBand::E_UTRA_BAND_256)
            {
                LE_ERROR("Invalid LTE RF band.");
            }
            else
            {
                uint8_t bandIndex = static_cast<uint8_t>(lteBand) - 1;
                uint8_t groupIndex = bandIndex / TAF_RADIO_BAND_NUM_PER_GROUP;
                uint8_t bitIndex = bandIndex % TAF_RADIO_BAND_NUM_PER_GROUP;
                lteBandCapability[groupIndex] |= (uint64_t)0x1 << bitIndex;
            }
        }
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for RF band pereferences.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioRFBandPrefResponseCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of RF band pereferences.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioRFBandPrefResponseCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * GSM and WCDMA RF band pereferences.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_BandBitMask_t taf_RadioRFBandPrefResponseCallback::bandPreferences = 0;

//--------------------------------------------------------------------------------------------------
/**
 * LTE RF band pereferences.
 */
//--------------------------------------------------------------------------------------------------
uint64_t taf_RadioRFBandPrefResponseCallback::lteBandPreferences[TAF_RADIO_LTE_BAND_GROUP_NUM];

//--------------------------------------------------------------------------------------------------
/**
 * Response for setting RF band preferences.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioRFBandPrefResponseCallback::setRFBandPrefResponse
(
    telux::common::ErrorCode error ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioRFBandPrefResponseCallback --> setRFBandPrefResponse");

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
 * Response for getting RF band preferences.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioRFBandPrefResponseCallback::rfBandPrefResponse
(
    std::shared_ptr<telux::tel::IRFBandList> prefList, ///< [IN] RF band preferences list.
    telux::common::ErrorCode error                     ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioRFBandPrefResponseCallback --> rfBandPrefResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        for (uint8_t i = 0; i < TAF_RADIO_LTE_BAND_GROUP_NUM; i++)
        {
            lteBandPreferences[i] = 0;
        }
        bandPreferences = taf_radio_CovertRFBand(prefList);
        std::vector<telux::tel::LteRFBand> lteBands = prefList->getLteBands();
        for (auto lteBand : lteBands)
        {
            if (lteBand < telux::tel::LteRFBand::E_UTRA_BAND_1 ||
                lteBand > telux::tel::LteRFBand::E_UTRA_BAND_256)
            {
                LE_ERROR("Invalid LTE RF band.");
            }
            else
            {
                uint8_t bandIndex = static_cast<uint8_t>(lteBand) - 1;
                uint8_t groupIndex = bandIndex / TAF_RADIO_BAND_NUM_PER_GROUP;
                uint8_t bitIndex = bandIndex % TAF_RADIO_BAND_NUM_PER_GROUP;
                lteBandPreferences[groupIndex] |= (uint64_t)0x1 << bitIndex;
            }
        }
        result = LE_OK;
    }

    le_sem_Post(semaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore for RF band information.
 */
//--------------------------------------------------------------------------------------------------
le_sem_Ref_t taf_RadioRFBandInfoResponseCallback::semaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Result of RF band information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_RadioRFBandInfoResponseCallback::result = LE_OK;

//--------------------------------------------------------------------------------------------------
/**
 * GSM/WCDMA/TDSCDMA RF band information.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_BandBitMask_t taf_RadioRFBandInfoResponseCallback::band = 0;

//--------------------------------------------------------------------------------------------------
/**
 * LTE RF band information.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_RadioRFBandInfoResponseCallback::lteBand = 0;

//--------------------------------------------------------------------------------------------------
/**
 * NR5G RF band information.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_RadioRFBandInfoResponseCallback::nrBand = 0;

//--------------------------------------------------------------------------------------------------
/**
 * RF bandwidth information.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_RFBandWidth_t taf_RadioRFBandInfoResponseCallback::bandwidth =
    TAF_RADIO_RF_BANDWIDTH_INVALID;

//--------------------------------------------------------------------------------------------------
/**
 * Response for getting RF band information.
 */
//--------------------------------------------------------------------------------------------------
void taf_RadioRFBandInfoResponseCallback::rfBandInfoResponse
(
    telux::tel::RFBandInfo bandInfo, ///< [IN] RF band information.
    telux::common::ErrorCode error   ///< [IN] Error code.
)
{
    LE_DEBUG("<SDK Callback> taf_RadioRFBandInfoResponseCallback --> rfBandInfoResponse");

    if (error != telux::common::ErrorCode::SUCCESS)
    {
        LE_ERROR("Error(%d)", (int)error);
        result = LE_FAULT;
    }
    else
    {
        result = LE_OK;
        bandwidth = taf_radio_CovertRFBandwidth(bandInfo.bandWidth);
        band = taf_radio_CovertRFBand(bandInfo.band);
        lteBand = taf_radio_CovertLteActiveBand(bandInfo.band);
        nrBand = taf_radio_CovertNrActiveBand(bandInfo.band);
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
 * Static pool for IMS references.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(phonePool, TAF_RADIO_PHONE_NUM, sizeof(uint8_t));

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

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for CA information.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(caInfoPool, TAF_RADIO_CA_INFO_MAX_NUM, sizeof(taf_RadioCAInfo_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for connection status.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(connStatusPool, TAF_RADIO_CONN_STATUS_MAX_NUM,
    sizeof(taf_radio_NREndcAvailability_t));

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for IMS references.
 */
//--------------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------------
/**
 * Static map for IMS references.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(imsRefMap, TAF_RADIO_PHONE_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for ne references.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(netStatusRefMap, TAF_RADIO_PHONE_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for CA information.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(caInfoMap, TAF_RADIO_CA_INFO_MAX_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for connection status.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(connStatusMap, TAF_RADIO_CONN_STATUS_MAX_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Static map for service status.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(svcStatusRefMap, TAF_RADIO_SVC_STATUS_HANDLER_MAX_NUM);

le_event_Id_t taf_Radio::radioCmdEvId = nullptr;
le_event_Id_t taf_Radio::radioCmdCompleteEvId = nullptr;

//--------------------------------------------------------------------------------------------------
/**
 * Covert RAT enum.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_Rat_t taf_Radio::taf_radio_CovertRat
(
    telux::tel::RadioTechnology rat ///< [IN] Radio access technoloy.
)
{
    switch (rat)
    {
        case telux::tel::RadioTechnology::RADIO_TECH_GSM:
            return TAF_RADIO_RAT_GSM;
        case telux::tel::RadioTechnology::RADIO_TECH_GPRS:
            return TAF_RADIO_RAT_GPRS;
        case telux::tel::RadioTechnology::RADIO_TECH_EDGE:
            return TAF_RADIO_RAT_EDGE;
        case telux::tel::RadioTechnology::RADIO_TECH_UMTS:
            return TAF_RADIO_RAT_UMTS;
        case telux::tel::RadioTechnology::RADIO_TECH_IS95A:
            return TAF_RADIO_RAT_IS95A;
        case telux::tel::RadioTechnology::RADIO_TECH_IS95B:
            return TAF_RADIO_RAT_IS95B;
        case telux::tel::RadioTechnology::RADIO_TECH_1xRTT:
            return TAF_RADIO_RAT_1xRTT;
        case telux::tel::RadioTechnology::RADIO_TECH_EVDO_0:
            return TAF_RADIO_RAT_EVDO_0;
        case telux::tel::RadioTechnology::RADIO_TECH_EVDO_A:
            return TAF_RADIO_RAT_EVDO_A;
        case telux::tel::RadioTechnology::RADIO_TECH_EVDO_B:
            return TAF_RADIO_RAT_EVDO_B;
        case telux::tel::RadioTechnology::RADIO_TECH_EHRPD:
            return TAF_RADIO_RAT_EHRPD;
        case telux::tel::RadioTechnology::RADIO_TECH_HSDPA:
            return TAF_RADIO_RAT_HSDPA;
        case telux::tel::RadioTechnology::RADIO_TECH_HSUPA:
            return TAF_RADIO_RAT_HSUPA;
        case telux::tel::RadioTechnology::RADIO_TECH_HSPA:
            return TAF_RADIO_RAT_HSPA;
        case telux::tel::RadioTechnology::RADIO_TECH_HSPAP:
            return TAF_RADIO_RAT_HSPAP;
        case telux::tel::RadioTechnology::RADIO_TECH_TD_SCDMA:
            return TAF_RADIO_RAT_TDSCDMA;
        case telux::tel::RadioTechnology::RADIO_TECH_IWLAN:
            return TAF_RADIO_RAT_IWLAN;
        case telux::tel::RadioTechnology::RADIO_TECH_LTE:
            return TAF_RADIO_RAT_LTE;
        case telux::tel::RadioTechnology::RADIO_TECH_LTE_CA:
            return TAF_RADIO_RAT_LTE_CA;
        case telux::tel::RadioTechnology::RADIO_TECH_NR5G:
            return TAF_RADIO_RAT_NR5G;
        default:
            LE_WARN("Unknown RAT.");
    }

    return TAF_RADIO_RAT_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert NR icon type.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NrIconType_t taf_Radio::taf_radio_ConvertNrIconType
(
    telux::data::NrIconType type ///< [IN] NR icon type
)
{
    switch(type)
    {
        case telux::data::NrIconType::BASIC:
        case telux::data::NrIconType::UWB:
            LE_DEBUG("5G NR icon type.");
            return TAF_RADIO_NR_ICON_5G;
        default:
            LE_DEBUG("Unknown NR icon type.");
    }

    return TAF_RADIO_NR_ICON_TYPE_NONE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert ENDC status.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NREndcAvailability_t taf_Radio::taf_radio_ConvertEndcStatus
(
    taf_pa_radio_EndcStatus_t status
)
{
    switch (status)
    {
        case TAF_PA_RADIO_ENDC_STATUS_AVAILABLE:
            return TAF_RADIO_NR_ENDC_AVAILABLE;
        case TAF_PA_RADIO_ENDC_STATUS_UNAVAILABLE:
            return TAF_RADIO_NR_ENDC_UNAVAILABLE;
        default:
            LE_ERROR("Unknown status.");
    }

    return TAF_RADIO_NR_ENDC_UNKNOWN;
}

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
        taf_RadioImsStatus_t* statusPtr = (taf_RadioImsStatus_t*)reportPtr;
        handlerFunc(statusPtr->status, statusPtr->phoneId, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for operating mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerOpModeHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_OpModeChangeHandlerFunc_t handlerFunc =
        (taf_radio_OpModeChangeHandlerFunc_t)layerHandlerFunc;
    if (handlerFunc)
    {
        taf_radio_OpMode_t* modePtr = (taf_radio_OpMode_t*)reportPtr;
        handlerFunc(*modePtr, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for network registration state.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerNetRegStateHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_NetRegStateHandlerFunc_t handlerFunc =
        (taf_radio_NetRegStateHandlerFunc_t)layerHandlerFunc;
    if (handlerFunc)
    {
        handlerFunc((taf_radio_NetRegStateInd_t*)reportPtr, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for IMS state.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerImsStateHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_ImsStatusChangeHandlerFunc_t handlerFunc =
        (taf_radio_ImsStatusChangeHandlerFunc_t)layerHandlerFunc;

    taf_RadioImsStatus_t* statusPtr = (taf_RadioImsStatus_t*)reportPtr;
    if (handlerFunc)
    {
        handlerFunc(statusPtr->imsRef, statusPtr->bitmask, statusPtr->phoneId,
            le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for signal strength.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerSsHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_SignalStrengthChangeHandlerFunc_t handlerFunc =
        (taf_radio_SignalStrengthChangeHandlerFunc_t)layerHandlerFunc;

    taf_RadioSsInd_t* ssPtr = (taf_RadioSsInd_t*)reportPtr;
    if (handlerFunc)
    {
        handlerFunc(ssPtr->rssi, ssPtr->rsrp, ssPtr->phoneId, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for IMS registration state.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerCellInfoHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_CellInfoChangeHandlerFunc_t handlerFunc =
        (taf_radio_CellInfoChangeHandlerFunc_t)layerHandlerFunc;
    if (handlerFunc)
    {
        taf_RadioCellInfoInd_t* cellInfoPtr = (taf_RadioCellInfoInd_t*)reportPtr;
        handlerFunc(cellInfoPtr->cellInfoStatus, cellInfoPtr->phoneId, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for network status.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerNetStatusHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_NetStatusHandlerFunc_t handlerFunc =
        (taf_radio_NetStatusHandlerFunc_t)layerHandlerFunc;
    taf_RadioNetStatusInd_t* indPtr = (taf_RadioNetStatusInd_t*)reportPtr;
    if (handlerFunc)
    {
        handlerFunc(indPtr->netStatusRef, indPtr->bitmask, indPtr->phoneId,
            le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for service status change.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerServiceStatusHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_ServiceStatusChangeHandlerFunc_t handlerFunc =
        (taf_radio_ServiceStatusChangeHandlerFunc_t)layerHandlerFunc;
    if (handlerFunc)
    {
        taf_RadioServiceStatusInd_t* indPtr = (taf_RadioServiceStatusInd_t*)reportPtr;

        // ctxPtr holds both the phoneId filter and the original user contextPtr.
        taf_RadioServiceStatusHandlerCtx_t* ctxPtr =
            (taf_RadioServiceStatusHandlerCtx_t*)le_event_GetContextPtr();
        uint8_t filterPhoneId = (ctxPtr != NULL) ? ctxPtr->phoneId : 0;
        void*   userCtx       = (ctxPtr != NULL) ? ctxPtr->userCtx  : NULL;

        LE_DEBUG("LayerServiceStatusHandler: phone=%d status=%d filterPhoneId=%d ctxPtr=%p",
                indPtr->phone, indPtr->status, filterPhoneId, (void*)ctxPtr);

        // phoneId == 0 means "all phones"; otherwise only report when phone matches.
        if (filterPhoneId == 0 || filterPhoneId == indPtr->phone)
        {
            handlerFunc(indPtr->rat, indPtr->status, indPtr->phone, userCtx);
        }
        else
        {
            LE_DEBUG("ServiceStatusChange: skip phone=%d (filter=%d) ctxPtr=%p",
                    indPtr->phone, filterPhoneId, (void*)ctxPtr);
        }
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler registered via taf_pa_radio_AddNetStatusChangeHandler().
 * Filters service-status changes from NetStatus indications and retrieves the
 * current service status via taf_pa_radio_GetRatSvcStatus().
 */
//--------------------------------------------------------------------------------------------------
static void ServiceStatusChangeHandler
(
    uint8_t phoneId,
    taf_pa_radio_Rat_t rat,
    taf_pa_radio_ServiceStatus_t serviceStatus,
    void* contextPtr
)
{
    LE_UNUSED(contextPtr);

    TAF_ERROR_IF_RET_NIL(phoneId == 0 || phoneId > TAF_RADIO_PHONE_NUM,
        "ServiceStatusChangeHandler: invalid phoneId=%d", phoneId);

    auto &tafRadio = taf_Radio::GetInstance();

    taf_radio_RatSvcStatus_t svcStatus = ConvertPaSvcStatusToSvcStatus(serviceStatus);
    taf_radio_Rat_t          svcRat    = ConvertPaRatToSvcRat(rat);

    LE_DEBUG("ServiceStatusChangeHandler phone=%d oldStatus=%d newStatus=%d",
             phoneId, tafRadio.ratSvcState[phoneId - 1], svcStatus);

    if (svcStatus == tafRadio.ratSvcState[phoneId - 1])
    {
        LE_DEBUG("ServiceStatusChangeHandler phone=%d status=%d unchanged, skip",
                 phoneId, svcStatus);
        return;
    }

    le_event_Id_t svcEvId = nullptr;
    switch (svcStatus)
    {
        case TAF_RADIO_RAT_SVC_STATUS_NO_SERVICE:
            svcEvId = tafRadio.svcStatusNoServiceEvId;       break;
        case TAF_RADIO_RAT_SVC_STATUS_LIMITED:
            svcEvId = tafRadio.svcStatusLimitedEvId;         break;
        case TAF_RADIO_RAT_SVC_STATUS_SERVICE:
            svcEvId = tafRadio.svcStatusServiceEvId;         break;
        case TAF_RADIO_RAT_SVC_STATUS_LIMITED_REGIONAL:
            svcEvId = tafRadio.svcStatusLimitedRegionalEvId; break;
        case TAF_RADIO_RAT_SVC_STATUS_POWER_SAVE:
            svcEvId = tafRadio.svcStatusPowerSaveEvId;       break;
        default:
            LE_WARN("ServiceStatusChangeHandler: unexpected ratSrvStatus=%d phone=%d",
                    svcStatus, phoneId);
            return;
    }

    tafRadio.ratSvcState[phoneId - 1] = svcStatus;

    taf_RadioServiceStatusInd_t* indPtr =
        (taf_RadioServiceStatusInd_t*)le_mem_ForceAlloc(tafRadio.svcStatusIndPool);
    indPtr->phone  = phoneId;
    indPtr->status = svcStatus;
    indPtr->rat    = svcRat;
    le_event_ReportWithRefCounting(svcEvId, (void*)indPtr);
    LE_DEBUG("ServiceStatusChangeHandler phone=%d -> status=%d rat=%d",
             phoneId, svcStatus, svcRat);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for RAT change.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerRatChangeHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_RatChangeHandlerFunc_t handlerFunc =
        (taf_radio_RatChangeHandlerFunc_t)layerHandlerFunc;

    if (handlerFunc)
    {
        handlerFunc((taf_radio_RatChangeInd_t*)reportPtr, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for network registration rejection.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerNetRejectHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_NetRegRejectHandlerFunc_t handlerFunc =
        (taf_radio_NetRegRejectHandlerFunc_t)layerHandlerFunc;

    if (handlerFunc)
    {
        handlerFunc((taf_radio_NetRegRejInd_t*)reportPtr, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for NR icon type change.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerNrIconTypeHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_NrIconTypeHandlerFunc_t handlerFunc =
        (taf_radio_NrIconTypeHandlerFunc_t)layerHandlerFunc;

    taf_RadioNrIconTypeInd_t* indPtr = (taf_RadioNrIconTypeInd_t*)reportPtr;
    if (handlerFunc)
    {
        handlerFunc(indPtr->type, indPtr->phoneId, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for LTE CA information.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerLteCAHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_CAInfoHandlerFunc_t handlerFunc =
        (taf_radio_CAInfoHandlerFunc_t)layerHandlerFunc;

    if (handlerFunc)
    {
        taf_RadioCAInd_t* indPtr = (taf_RadioCAInd_t*)reportPtr;
        handlerFunc(indPtr->phone, indPtr->infoRef, le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Layered handler for connection status
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::taf_radio_LayerConnStatusHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_radio_ConnectionStatusHandlerFunc_t handlerFunc =
        (taf_radio_ConnectionStatusHandlerFunc_t)layerHandlerFunc;
    if (handlerFunc)
    {
        taf_RadioConnStatusInd_t* indPtr = (taf_RadioConnStatusInd_t*)reportPtr;
        handlerFunc(indPtr->phone, indPtr->bitmask, indPtr->statusRef,
            le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}

static void LteCAHandler
(
    uint8_t phone,
    taf_pa_radio_LteCaIndBitMask_t bitmask,
    taf_pa_radio_LteCphyCaInfoRef_t infoRef,
    void* contextPtr
)
{
    TAF_ERROR_IF_RET_NIL(!phone || phone > TAF_RADIO_PHONE_NUM,
        "Invalid para(phone:%d)", phone);

    auto &tafRadio = taf_Radio::GetInstance();
    LE_DEBUG("LteCAHandler phone %d bitmask 0x%lx", phone, bitmask);
    taf_RadioCAInfo_t lteInfo;
    lteInfo.status= TAF_RADIO_CA_STATUS_DEACTIVATED;
    lteInfo.cellCount = 0;

    if (bitmask & TAF_PA_RADIO_LTE_CA_IND_BIT_MASK_PCELL_INFO)
    {
        lteInfo.cellCount = 1;
        LE_DEBUG("Primary cell information.");
    }

    if (bitmask & TAF_PA_RADIO_LTE_CA_IND_BIT_MASK_SCELL_INFO)
    {
        uint32_t scellCount = 0;
        le_result_t result = taf_pa_radio_GetScellCount(infoRef, &scellCount);
        if (result != LE_OK)
            LE_ERROR("Fail to get secondary cell count.");
        if (scellCount > TAF_RADIO_SCELL_NUMBER)
        {
            LE_WARN("Scell number %d is more than limited number %d.", scellCount, TAF_RADIO_SCELL_NUMBER);
            scellCount = TAF_RADIO_SCELL_NUMBER;
        }
        LE_DEBUG("LteCAHandler scellCount %d", scellCount);

        for (uint32_t i = 0; i < scellCount; i++)
        {
            taf_pa_radio_ScellState_t state = TAF_PA_RADIO_SCELL_STATE_UNKNOWN;
            result = taf_pa_radio_GetScellState(infoRef, i, &state);
            if (result != LE_OK)
                LE_ERROR("Fail to get secondary cell state at %d.", i);

            if (state == TAF_PA_RADIO_SCELL_STATE_CONFIGURED_ACTIVATED)
            {
                lteInfo.status = TAF_RADIO_CA_STATUS_ACTIVATED;
                lteInfo.cellCount++;
            }
            LE_DEBUG("LteCAHandler state[%d] %d", i, state);
        }
    }

    taf_RadioCAInfo_t* infoPtr = (taf_RadioCAInfo_t*)le_ref_Lookup(tafRadio.caInfoMap, tafRadio.lteCAInfoRefs[phone -1]);
    if (infoPtr->status != lteInfo.status || infoPtr->cellCount != lteInfo.cellCount)
    {
        taf_RadioCAInd_t* indPtr = (taf_RadioCAInd_t*)le_mem_ForceAlloc(tafRadio.caIndPool);
        indPtr->phone = phone;
        infoPtr->status = lteInfo.status;
        infoPtr->cellCount = lteInfo.cellCount;
        indPtr->infoRef = tafRadio.lteCAInfoRefs[phone - 1];
        le_event_ReportWithRefCounting(tafRadio.lteCAIndEvId, (void*)indPtr);
    }
}

static void EndcStatusHandler
(
    uint8_t phone,
    taf_pa_radio_EndcStatus_t status,
    void* contextPtr
)
{
    TAF_ERROR_IF_RET_NIL(!phone || phone > TAF_RADIO_PHONE_NUM,
        "Invalid para(phone:%d)", phone);

    auto &tafRadio = taf_Radio::GetInstance();

    taf_radio_NREndcAvailability_t state = tafRadio.taf_radio_ConvertEndcStatus(status);
    taf_radio_NREndcAvailability_t* statusPtr =
        (taf_radio_NREndcAvailability_t*)le_ref_Lookup(tafRadio.connStatusMap,
        tafRadio.endcStatusRefs[phone - 1]);
    if (*statusPtr != state)
    {
        *statusPtr = state;

        taf_RadioConnStatusInd_t* indPtr =
            (taf_RadioConnStatusInd_t*)le_mem_ForceAlloc(tafRadio.connStatusIndPool);
        indPtr->phone = phone;
        indPtr->bitmask = TAF_RADIO_CONN_IND_BIT_MASK_ENDC;
        indPtr->statusRef = tafRadio.endcStatusRefs[phone -1];
        le_event_ReportWithRefCounting(tafRadio.connStatusEvId, (void*)indPtr);
    }
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
    auto &tafRadio = taf_Radio::GetInstance();

    le_result_t res = LE_OK;
    le_clk_Time_t timeToWait = {1, 0};
    taf_radio_ScanInformationListRef_t scanListRef = NULL;
    taf_radio_PciScanInformationListRef_t pciScanListRef = NULL;

    switch (cmdReq->cmdType)
    {
        case TAF_RADIO_CMD_TYPE_ASYNC_REG_MANUAL:
        {
            if (cmdReq->phoneId == 0 || cmdReq->phoneId > tafRadio.networkManagers.size())
            {
                LE_ERROR("Invalid para(phoneId:%d > %" PRIuS ")", cmdReq->phoneId,
                    tafRadio.networkManagers.size());
                res = LE_BAD_PARAMETER;
                break;
            }

            auto networkManager = tafRadio.networkManagers[cmdReq->phoneId - 1];
            if (networkManager == nullptr)
            {
                LE_ERROR("Invalid para(null ptr, phoneId:%d)", cmdReq->phoneId);
                res = LE_FAULT;
                break;
            }

            std::string mcc(cmdReq->mccPtr);
            std::string mnc(cmdReq->mncPtr);

            auto selModeStatus = networkManager->setNetworkSelectionMode(
                telux::tel::NetworkSelectionMode::MANUAL, mcc, mnc,
                &taf_RadioNetworkResponseCallback::setNetworkSelectionModeResponseCb);
            if (selModeStatus != telux::common::Status::SUCCESS)
            {
                LE_ERROR("setNetworkSelectionMode failed, status:%d", static_cast<int>(selModeStatus));
                res = LE_FAULT;
                break;
            }

            res = le_sem_WaitWithTimeOut(taf_RadioNetworkResponseCallback::selModeSem, timeToWait);
            if (res != LE_OK)
            {
                LE_ERROR("Wait semaphore timeout.");
                le_sem_TryWait(taf_RadioNetworkResponseCallback::selModeSem);
                res = LE_TIMEOUT;
                break;
            }

            if (taf_RadioNetworkResponseCallback::selModeRes != LE_OK)
            {
                LE_ERROR("Error response when setting network selection mode.");
                res = LE_FAULT;
            }
            break;
        }

        case TAF_RADIO_CMD_TYPE_ASYNC_NETWORK_SCAN:
        {
            if (cmdReq->phoneId == 0 || cmdReq->phoneId > tafRadio.networkManagers.size())
            {
                LE_ERROR("Invalid para(phoneId:%d > %" PRIuS ")", cmdReq->phoneId,
                    tafRadio.networkManagers.size());
                res = LE_BAD_PARAMETER;
                break;
            }

            auto networkManager = tafRadio.networkManagers[cmdReq->phoneId - 1];
            if (networkManager == nullptr)
            {
                LE_ERROR("Invalid network manager(null ptr, phoneId:%d)", cmdReq->phoneId);
                res = LE_FAULT;
                break;
            }

            auto networkListener = tafRadio.networkListeners[cmdReq->phoneId - 1];
            if (networkListener == nullptr)
            {
                LE_ERROR("Invalid network listener(null ptr, phoneId:%d)", cmdReq->phoneId);
                res = LE_FAULT;
                break;
            }
            networkListener->opInfos.clear();

            auto status = networkManager->registerListener(networkListener);
            if (status != telux::common::Status::SUCCESS)
            {
                LE_ERROR("Fail to register listener with phoneId:%d, status:%d",
                    cmdReq->phoneId, static_cast<int>(status));
                res = LE_FAULT;
                break;
            }

            telux::tel::NetworkScanInfo info;
            info.scanType = telux::tel::NetworkScanType::ALL_RATS;
            auto scanStatus = networkManager->performNetworkScan(info,
                taf_RadioPerformNetworkScanCallback::performNetworkScanResponse);
            if (scanStatus != telux::common::Status::SUCCESS)
            {
                LE_ERROR("Call sdk function failed, status:%d", static_cast<int>(scanStatus));
                status = networkManager->deregisterListener(networkListener);
                if (status != telux::common::Status::SUCCESS)
                {
                    LE_ERROR("Fail to deregister listener with phoneId:%d, status:%d",
                        cmdReq->phoneId, static_cast<int>(status));
                }
                res = LE_FAULT;
                break;
            }

            res = le_sem_WaitWithTimeOut(taf_RadioPerformNetworkScanCallback::semaphore, timeToWait);
            if (res != LE_OK)
            {
                LE_ERROR("Perform network scan submit timeout.");
                le_sem_TryWait(taf_RadioPerformNetworkScanCallback::semaphore);
                status = networkManager->deregisterListener(networkListener);
                if (status != telux::common::Status::SUCCESS)
                {
                    LE_ERROR("Fail to deregister listener with phoneId:%d, status:%d",
                        cmdReq->phoneId, static_cast<int>(status));
                }
                res = LE_TIMEOUT;
                break;
            }
            if (taf_RadioPerformNetworkScanCallback::result != LE_OK)
            {
                LE_ERROR("Perform network scan failed.");
                status = networkManager->deregisterListener(networkListener);
                if (status != telux::common::Status::SUCCESS)
                {
                    LE_ERROR("Fail to deregister listener with phoneId:%d, status:%d",
                        cmdReq->phoneId, static_cast<int>(status));
                }
                res = LE_FAULT;
                break;
            }

            le_clk_Time_t timeToScan = {TAF_RADIO_SCAN_INTERVAL, 0};
            res = le_sem_WaitWithTimeOut(networkListener->semaphore, timeToScan);
            if (res != LE_OK)
            {
                LE_ERROR("Network scan timeout");
                le_sem_TryWait(networkListener->semaphore);
                status = networkManager->deregisterListener(networkListener);
                if (status != telux::common::Status::SUCCESS)
                {
                    LE_ERROR("Fail to deregister listener with phoneId:%d, status:%d",
                        cmdReq->phoneId, static_cast<int>(status));
                }
                res = LE_TIMEOUT;
                break;
            }

            status = networkManager->deregisterListener(networkListener);
            if (status != telux::common::Status::SUCCESS)
            {
                LE_ERROR("Fail to deregister listener with phoneId:%d, status:%d",
                    cmdReq->phoneId, static_cast<int>(status));
                res = LE_FAULT;
                break;
            }

            if (networkListener->opInfos.size() == 0)
            {
                LE_ERROR("Phone%d has no operators after scanning", cmdReq->phoneId);
                res = LE_NOT_FOUND;
                break;
            }

            taf_RadioScanOpList_t* opsList =
                (taf_RadioScanOpList_t*)le_mem_ForceAlloc(tafRadio.scanOpsListPool);

            opsList->scanOpList = LE_SLS_LIST_INIT;
            opsList->safeRefList = LE_SLS_LIST_INIT;
            opsList->currPtr = NULL;
            scanListRef = (taf_radio_ScanInformationListRef_t)le_ref_CreateRef(
                tafRadio.scanOpListRefMap, (void*)opsList);

            for (auto opInfo : networkListener->opInfos)
            {
                taf_RadioScanOp_t* opPtr = (taf_RadioScanOp_t*)le_mem_ForceAlloc(tafRadio.scanOpPool);
                le_utf8_Copy(opPtr->name, opInfo.getName().c_str(), TAF_RADIO_NETWORK_NAME_MAX_LEN, NULL);
                le_utf8_Copy(opPtr->mcc, opInfo.getMcc().c_str(), TAF_RADIO_MCC_BYTES, NULL);
                le_utf8_Copy(opPtr->mnc, opInfo.getMnc().c_str(), TAF_RADIO_MNC_BYTES, NULL);
                opPtr->status.inUse = opInfo.getStatus().inUse;
                opPtr->status.roaming = opInfo.getStatus().roaming;
                opPtr->status.forbidden = opInfo.getStatus().forbidden;
                opPtr->status.preferred = opInfo.getStatus().preferred;
                opPtr->rat = opInfo.getRat();
                opPtr->link = LE_SLS_LINK_INIT;
                le_sls_Queue(&(opsList->scanOpList), &(opPtr->link));
            }
            break;
        }

        case TAF_RADIO_CMD_TYPE_ASYNC_PCI_NETWORK_SCAN:
        {
            pciScanListRef = taf_pa_radio_PerformPciNetworkScan(cmdReq->ratMask, cmdReq->phoneId);
            if (pciScanListRef == nullptr)
            {
                LE_ERROR("taf_pa_radio_PerformPciNetworkScan failed (phoneId:%d)", cmdReq->phoneId);
                res = LE_FAULT;
            }
            break;
        }

        default:
            LE_ERROR("Unsupported async radio cmd type: %d", cmdReq->cmdType);
            res = LE_UNSUPPORTED;
            break;
    }

    taf_RadioCmdComplete_t complete = {cmdReq, res, scanListRef, pciScanListRef};
    le_event_Report(taf_Radio::radioCmdCompleteEvId, &complete, sizeof(complete));
}

/*======================================================================

 FUNCTION        taf_Radio::RadioCmdCompleteHandler

 DESCRIPTION     async command completion events, executed in the MAIN thread's event loop.
                 Removes cmdReq from pendingCmdList, then calls the IPC client callback
                 if the session is still open (handlerFuncPtr != nullptr).

 DEPENDENCIES    The initialization of Radio.

 PARAMETERS      [IN] void* contextPtr: Context pointer.

 RETURN VALUE    void*
                     NULL: Success.

 SIDE EFFECTS

======================================================================*/
void taf_Radio::RadioCmdCompleteHandler(void* reportPtr)
{
    taf_RadioCmdComplete_t* complete = (taf_RadioCmdComplete_t*)reportPtr;
    taf_RadioCmdReq_t* cmdReq = complete->cmdPtr;

    auto &tafRadio = taf_Radio::GetInstance();
    le_dls_Remove(&tafRadio.pendingCmdList, &cmdReq->link);

    switch (cmdReq->cmdType)
    {
        case TAF_RADIO_CMD_TYPE_ASYNC_REG_MANUAL:
        {
            taf_radio_ManualSelectionHandlerFunc_t handlerFunc =
                (taf_radio_ManualSelectionHandlerFunc_t)cmdReq->handlerFuncPtr;
            if (handlerFunc != nullptr)
            {
                LE_DEBUG("Handler function:%p, result:%d", handlerFunc, complete->result);
                handlerFunc(complete->result, cmdReq->contextPtr);
            }
            else
            {
                LE_WARN("Session closed or no handler, skip callback for ASYNC_REG_MANUAL, result:%d",
                    complete->result);
            }
            break;
        }

        case TAF_RADIO_CMD_TYPE_ASYNC_NETWORK_SCAN:
        {
            taf_radio_CellularNetworkScanHandlerFunc_t handlerFunc =
                (taf_radio_CellularNetworkScanHandlerFunc_t)cmdReq->handlerFuncPtr;
            if (handlerFunc != nullptr)
            {
                LE_DEBUG("Handler function:%p listRef:%p", handlerFunc, complete->scanListRef);
                handlerFunc(complete->scanListRef, cmdReq->contextPtr);
                complete->scanListRef = NULL;
            }
            else
            {
                LE_WARN("Session closed or no handler, skip callback for ASYNC_NETWORK_SCAN");
            }
            if (complete->scanListRef != NULL)
            {
                taf_radio_DeleteCellularNetworkScan(complete->scanListRef);
            }
            break;
        }

        case TAF_RADIO_CMD_TYPE_ASYNC_PCI_NETWORK_SCAN:
        {
            taf_radio_PciNetworkScanHandlerFunc_t handlerFunc =
                (taf_radio_PciNetworkScanHandlerFunc_t)cmdReq->handlerFuncPtr;
            if (handlerFunc != nullptr)
            {
                LE_DEBUG("Handler function:%p", handlerFunc);
                handlerFunc(complete->pciScanListRef, cmdReq->phoneId, cmdReq->contextPtr);
                complete->pciScanListRef = NULL;
            }
            else
            {
                LE_WARN("Session closed or no handler, skip callback for ASYNC_PCI_NETWORK_SCAN");
            }
            if (complete->pciScanListRef != NULL)
            {
                taf_radio_DeletePciNetworkScan(complete->pciScanListRef);
            }
            break;
        }

        default:
            LE_ERROR("RadioCmdCompleteHandler: unknown cmdType %d, releasing cmdReq.",
                cmdReq->cmdType);
            break;
    }

    le_mem_Release(cmdReq);
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

    telux::common::Status status = tafRadio.phoneManager->registerListener(tafRadio.phoneListener);
    if (status != telux::common::Status::SUCCESS)
    {
        LE_ERROR("Failed to register phone listener.");
    }

    for (size_t i = 1; i <= tafRadio.phones.size(); i++)
    {
        int slot = tafRadio.phoneManager->getSlotIdFromPhoneId(i);
        if (tafRadio.imsServingSystemMgrs[(SlotId)slot] != nullptr &&
            tafRadio.imsServSysListeners[(SlotId)slot] != nullptr)
        {
            status = tafRadio.imsServingSystemMgrs[(SlotId)slot]->registerListener(
                tafRadio.imsServSysListeners[(SlotId)slot]);
            if (status != telux::common::Status::SUCCESS)
            {
                LE_ERROR("Failed to register IMS serving system listener.");
            }
        }

        tafRadio.dataServSysManagers[(SlotId)slot]->registerListener(
            tafRadio.dataServSysListeners[(SlotId)slot]);

        tafRadio.servingSystemManagers[i - 1]->registerListener(tafRadio.servSysListeners[i - 1]);
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

    tafRadio.phoneManager->removeListener(tafRadio.phoneListener);

    for (size_t i = 1; i <= tafRadio.phones.size(); i++)
    {
        int slot = (int)tafRadio.phoneManager->getSlotIdFromPhoneId(i);
        if (tafRadio.imsServingSystemMgrs[(SlotId)slot] != nullptr &&
            tafRadio.imsServSysListeners[(SlotId)slot] != nullptr)
        {
            tafRadio.imsServingSystemMgrs[(SlotId)slot]->deregisterListener(
                tafRadio.imsServSysListeners[(SlotId)slot]);
        }

        if (tafRadio.dataServSysManagers[(SlotId)slot] != nullptr &&
            tafRadio.dataServSysListeners[(SlotId)slot] != nullptr)
        {
            tafRadio.dataServSysManagers[(SlotId)slot]->deregisterListener(
                tafRadio.dataServSysListeners[(SlotId)slot]);
        }

        if (tafRadio.servingSystemManagers[i - 1] != nullptr &&
            tafRadio.servSysListeners[i -1] != nullptr)
        {
            tafRadio.servingSystemManagers[i - 1]->deregisterListener(
                tafRadio.servSysListeners[i - 1]);
        }
    }
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
    auto &tafRadio = taf_Radio::GetInstance();

    le_result_t result;
    if (state == TAF_PM_STATE_RESUME)
    {
        LE_INFO("Power state change to RESUME");
        RegisterListener();
        result = taf_pa_radio_EnableIndication();
        TAF_ERROR_IF_RET_NIL(result != LE_OK, "Enable indication falied.");

        for(auto i = 1; i <= TAF_RADIO_PHONE_NUM; i++)
        {
            le_result_t limitRes = taf_pa_radio_SetSysInfoIndLimit(i, TAF_PA_RADIO_SYS_INFO_IND_LIMIT_NONE);

            if (limitRes != LE_OK)
            {
                LE_ERROR("PowerStateChangeHandler: Failed to set SYS_INFO limit for phoneId %d "
                        "on RESUME [rc=%d]", i, limitRes);
            }
            else
            {
                LE_INFO("PowerStateChangeHandler: SYS_INFO limit (STATE_NONE) "
                        "set for %d phoneId", i);
            }
        }
    }
    else if (state == TAF_PM_STATE_SUSPEND)
    {
        LE_INFO("Power state change to SUSPEND");
        DeregisterListener();
        if (!le_dls_IsEmpty(&tafRadio.svcStatusCtxList))
        {
            // Clients are connected — disable indications AND limit SYS_INFO to state-toggle
            // so the modem only wakes the AP on SERVICE <-> OOS transitions.
            result = taf_pa_radio_DisableIndication(TAF_PA_RADIO_DISABLE_IND_MODE_SKIP_NAS_SYS_INFO_IND);
            if (result != LE_OK)
            {
                LE_ERROR("PowerStateChangeHandler: Disable indication failed [rc=%d]", result);
            }

            for(auto i = 1; i <= TAF_RADIO_PHONE_NUM; i++)
            {
                le_result_t limitRes = taf_pa_radio_SetSysInfoIndLimit(i, TAF_PA_RADIO_SYS_INFO_IND_LIMIT_BY_STATE_TOGGLE);

                if (limitRes != LE_OK)
                {
                    LE_ERROR("PowerStateChangeHandler: Failed to set SYS_INFO limit for phoneId %d "
                            "(STATE_TOGGLE) on SUSPEND [rc=%d]", i, limitRes);
                }
                else
                {
                    LE_INFO("PowerStateChangeHandler: SYS_INFO limit (STATE_TOGGLE) "
                            "set for %d PhoneId", i);
                }
            }
        }
        else
        {
            // No clients connected — only disable indications entirely.
            // No need to arm SYS_INFO state-toggle since no one is listening.
            result = taf_pa_radio_DisableIndication(TAF_PA_RADIO_DISABLE_IND_MODE_ALL);
            if (result != LE_OK)
            {
                LE_ERROR("PowerStateChangeHandler: Disable indication failed (no clients) [rc=%d]",
                        result);
            }
            else
            {
                LE_INFO("PowerStateChangeHandler: No clients connected - indications disabled directly");
            }
        }
    }
}

void taf_Radio::ApplyServiceStatusModemFiltering(void)
{
    auto &tafRadio = taf_Radio::GetInstance();

    if (taf_pm_GetPowerState() != TAF_PM_STATE_SUSPEND)
    {
        return;
    }

    bool haveClients = !le_dls_IsEmpty(&tafRadio.svcStatusCtxList);

    if (haveClients)
    {
        for(auto i = 1; i <= TAF_RADIO_PHONE_NUM; i++)
        {
            le_result_t limitRes = taf_pa_radio_SetSysInfoIndLimit(i,
                TAF_PA_RADIO_SYS_INFO_IND_LIMIT_BY_STATE_TOGGLE);
            if (limitRes != LE_OK)
            {
                LE_ERROR("ApplyServiceStatusModemFiltering: arm STATE_TOGGLE failed [rc=%d] for"
                    "phoneId=%d", limitRes, i);
            }
            else
            {
                LE_DEBUG("ApplyServiceStatusModemFiltering: SYS_INFO STATE_TOGGLE set phoneId=%d", i);
            }
        }
    }
    else
    {
        for(auto i = 1; i <= TAF_RADIO_PHONE_NUM; i++)
        {
            le_result_t limitRes = taf_pa_radio_SetSysInfoIndLimit(i,
                TAF_PA_RADIO_SYS_INFO_IND_LIMIT_NONE);
            if (limitRes != LE_OK)
            {
                LE_ERROR("ApplyServiceStatusModemFiltering:Disable SYS_INFO wakeup failed [rc=%d]"
                    "for phoneId:%d", limitRes, i);
            }
            else
            {
                LE_DEBUG("ApplyServiceStatusModemFiltering:SYS_INFO wakeup disabled phoneId:%d",i);
            }
        }
    }
}

static void ServiceStatusSessionCloseHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    LE_UNUSED(contextPtr);
    auto &tafRadio = taf_Radio::GetInstance();

    le_dls_Link_t* linkPtr = le_dls_Peek(&tafRadio.svcStatusCtxList);
    while (linkPtr != nullptr)
    {
        taf_RadioServiceStatusHandlerCtx_t* ctxPtr =
            CONTAINER_OF(linkPtr, taf_RadioServiceStatusHandlerCtx_t, link);

        le_dls_Link_t* nextPtr = le_dls_PeekNext(&tafRadio.svcStatusCtxList, linkPtr);

        if (ctxPtr->sessionRef == sessionRef)
        {
            LE_DEBUG("ServiceStatusSessionCloseHandler: cleaning up ctx for closed session");
            le_dls_Remove(&tafRadio.svcStatusCtxList, linkPtr);

            if (ctxPtr->safeRef != nullptr)
            {
                le_ref_DeleteRef(tafRadio.svcStatusRefMap, ctxPtr->safeRef);
                ctxPtr->safeRef = nullptr;
            }

            for (size_t i = 0; i < TAF_RADIO_SERVICE_STATUS_BIT_MASK_COUNT; i++)
            {
                if (ctxPtr->handlerRefs[i] != nullptr)
                {
                    le_event_RemoveHandler(ctxPtr->handlerRefs[i]);
                    ctxPtr->handlerRefs[i] = nullptr;
                }
            }
            le_mem_Release(ctxPtr);
        }

        linkPtr = nextPtr;
    }

    if (le_dls_IsEmpty(&tafRadio.svcStatusCtxList))
    {
        tafRadio.ApplyServiceStatusModemFiltering();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::Init(void)
{
    // 1. Initiate the semaphore
    taf_RadioNetworkResponseCallback::selModeSem = le_sem_Create("taf_RadioSelModeSem", 0);
    taf_RadioNetworkResponseCallback::prefNetSem = le_sem_Create("taf_RadioPrefNetSem", 0);
    taf_RadioPreferredNetworksResponseCallback::semaphore =
        le_sem_Create("taf_RadioPrefNetworkRespCbSem", 0);
    taf_RadioServingSystemResponseCallback::semaphore = le_sem_Create("taf_RadioSrvSysSem", 0);
    taf_RadioRatPreferenceResponseCallback::semaphore =
        le_sem_Create("taf_RadioRatPrefRespCbSem", 0);
    taf_RadioServiceDomainPreferenceResponseCallback::semaphore =
        le_sem_Create("taf_RadioServiceDomainPrefRespCbSem", 0);
    taf_RadioCellInfoCallback::semaphore = le_sem_Create("taf_RadioCellInfoCbSem", 0);
    taf_RadioPerformNetworkScanCallback::semaphore = le_sem_Create("taf_RadioPerfNetScanCbSem", 0);
    taf_RadioImsServSysCallback::semaphore = le_sem_Create("taf_RadioImsServSysCbSem", 0);
    taf_RadioImsSettingCallback::semaphore = le_sem_Create("taf_RadioImsSettingCbSem", 0);
    taf_RadioConfigureSignalStrengthCallback::semaphore = le_sem_Create("taf_RadioSigCfgCbSem", 0);
    taf_RadioSelectionModeResponseCallback::semaphore =
        le_sem_Create("taf_RadioSelectModeCbSem", 0);
    taf_RadioRFBandCapabilityResponseCallback::semaphore =
        le_sem_Create("taf_RadioBandCapCbSem", 0);
    taf_RadioRFBandPrefResponseCallback::semaphore =
        le_sem_Create("taf_RadioBandPrefCbSem", 0);
    taf_RadioRFBandInfoResponseCallback::semaphore =
        le_sem_Create("taf_RadioBandInfoCbSem", 0);

    imsRegStatusChangeId = le_event_CreateIdWithRefCounting("ImsRegStatus");
    opModeChangeId = le_event_CreateIdWithRefCounting("OpMode");
    netRegStateEvId = le_event_CreateIdWithRefCounting("NetRegState");
    packSwStateEvId = le_event_CreateIdWithRefCounting("PackSwState");
    imsStatusChangeId = le_event_CreateIdWithRefCounting("ImsStatus");
    gsmSsChangeEvId = le_event_CreateIdWithRefCounting("GsmSsChange");
    umtsSsChangeEvId = le_event_CreateIdWithRefCounting("UmtsSsChange");
    cdmaSsChangeEvId = le_event_CreateIdWithRefCounting("CdmaSsChange");
    lteSsChangeEvId = le_event_CreateIdWithRefCounting("LteSsChange");
    nr5gSsChangeEvId = le_event_CreateIdWithRefCounting("Nr5gSsChange");
    cellInfoChangeEvId = le_event_CreateIdWithRefCounting("CellInfoChange");
    ratChangeEvId = le_event_CreateIdWithRefCounting("RatChange");
    netStatusEvId = le_event_CreateIdWithRefCounting("netStatus");
    netRegRejEvId = le_event_CreateIdWithRefCounting("NetRegRej");
    nrIconTypeEvId = le_event_CreateIdWithRefCounting("NrIconType");
    lteCAIndEvId = le_event_CreateIdWithRefCounting("LteCAInd");
    connStatusEvId = le_event_CreateIdWithRefCounting("ConnStatus");
    svcStatusNoServiceEvId       = le_event_CreateIdWithRefCounting("SvcStatusNoServiceChange");
    svcStatusLimitedEvId          = le_event_CreateIdWithRefCounting("SvcStatusLimitedChange");
    svcStatusServiceEvId          = le_event_CreateIdWithRefCounting("SvcStatusServiceChange");
    svcStatusLimitedRegionalEvId  = le_event_CreateIdWithRefCounting("SvcStatusLimitedRegionalChange");
    svcStatusPowerSaveEvId        = le_event_CreateIdWithRefCounting("SvcStatusPowerSaveChange");

    // 2. Initiate the memory pool
    prefOpsListPool = le_mem_InitStaticPool(prefOpsListPool,
        TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM, sizeof(taf_RadioPrefOpList_t));
    prefOpPool = le_mem_InitStaticPool(prefOpPool, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM,
        sizeof(taf_RadioPrefOp_t));
    prefOpSafeRefPool = le_mem_InitStaticPool(prefOpSafeRefPool,
        TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM, sizeof(taf_RadioPrefOpSafeRef_t));
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
    phonePool = le_mem_InitStaticPool(phonePool, TAF_RADIO_PHONE_NUM, sizeof(uint8_t));
    caInfoPool = le_mem_InitStaticPool(caInfoPool, TAF_RADIO_CA_INFO_MAX_NUM, sizeof(taf_RadioCAInfo_t));
    connStatusPool = le_mem_InitStaticPool(connStatusPool, TAF_RADIO_CONN_STATUS_MAX_NUM,
        sizeof(taf_radio_NREndcAvailability_t));

    opModeChangePool = le_mem_CreatePool("opModeChangePool", sizeof(taf_radio_OpMode_t));
    netRegStatePool = le_mem_CreatePool("netRegStatePool", sizeof(taf_radio_NetRegStateInd_t));
    packSwStatePool = le_mem_CreatePool("packSwStatePool", sizeof(taf_radio_NetRegStateInd_t));
    imsStatusChangePool = le_mem_CreatePool("imsStatusChangePool", sizeof(taf_RadioImsStatus_t));
    ssChangePool = le_mem_CreatePool("ssChangePool", sizeof(taf_RadioSsInd_t));
    cellInfoChangePool = le_mem_CreatePool("cellInfoPool", sizeof(taf_radio_NetRegStateInd_t));
    ratChangePool = le_mem_CreatePool("ratChangePool", sizeof(taf_radio_RatChangeInd_t));
    netStatusPool = le_mem_CreatePool("netStatusPool", sizeof(taf_RadioNetStatusInd_t));
    netRegRejPool = le_mem_CreatePool("netRegRejPool", sizeof(taf_radio_NetRegRejInd_t));
    nrIconTypePool = le_mem_CreatePool("nrIconTypePool", sizeof(taf_RadioNrIconTypeInd_t));
    caIndPool= le_mem_CreatePool("caIndPool", sizeof(taf_RadioCAInd_t));
    connStatusIndPool= le_mem_CreatePool("connStatusIndPool", sizeof(taf_RadioConnStatusInd_t));
    svcStatusIndPool = le_mem_CreatePool("svcStatusIndPool",
        sizeof(taf_RadioServiceStatusInd_t));
    svcStatusHandlerCtxPool = le_mem_CreatePool("svcStatusHandlerCtxPool",
        sizeof(taf_RadioServiceStatusHandlerCtx_t));

    // 3. Initiate the reference map.
    prefOpListRefMap = le_ref_InitStaticMap(prefOpListRefMap,TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM);
    prefOpSafeRefMap = le_ref_InitStaticMap(prefOpSafeRefMap, TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM);
    scanOpListRefMap = le_ref_InitStaticMap(scanOpListRefMap, TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM);
    scanOpSafeRefMap = le_ref_InitStaticMap(scanOpSafeRefMap, TAF_RADIO_SCAN_OPERATORS_MAX_NUM);
    metricsRefMap = le_ref_InitStaticMap(metricsRefMap, TAF_RADIO_METRICS_MAX_NUM);
    ngbrCellsRefMap = le_ref_InitStaticMap(ngbrCellsRefMap, TAF_RADIO_NEIGHBOR_CELLS_MAX_NUM);
    ngbrCellInfoSafeRefMap = le_ref_InitStaticMap(ngbrCellInfoSafeRefMap,
        TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM);
    imsRefMap = le_ref_InitStaticMap(imsRefMap, TAF_RADIO_PHONE_NUM);
    netStatusRefMap = le_ref_InitStaticMap(netStatusRefMap, TAF_RADIO_PHONE_NUM);
    caInfoMap = le_ref_InitStaticMap(caInfoMap, TAF_RADIO_CA_INFO_MAX_NUM);
    connStatusMap = le_ref_InitStaticMap(connStatusMap, TAF_RADIO_CONN_STATUS_MAX_NUM);
    svcStatusRefMap = le_ref_InitStaticMap(svcStatusRefMap, TAF_RADIO_SVC_STATUS_HANDLER_MAX_NUM);

    for (uint8_t phoneId = 1; phoneId <= TAF_RADIO_PHONE_NUM; phoneId++)
    {
        uint8_t* phonePtr = (uint8_t*)le_mem_ForceAlloc(phonePool);
        taf_RadioCAInfo_t* lteCAInfoPtr = (taf_RadioCAInfo_t*)le_mem_ForceAlloc(caInfoPool);
        lteCAInfoPtr->cellCount = 0;
        lteCAInfoPtr->status = TAF_RADIO_CA_STATUS_DEACTIVATED;
        taf_radio_NREndcAvailability_t* endcStatusPtr =
            (taf_radio_NREndcAvailability_t*)le_mem_ForceAlloc(connStatusPool);
        *endcStatusPtr = TAF_RADIO_NR_ENDC_UNKNOWN;
        *phonePtr = phoneId;
        imsRefs[phoneId - 1] = (taf_radio_ImsRef_t)le_ref_CreateRef(imsRefMap, (void*)phonePtr);
        netStatusRefs[phoneId - 1] =
            (taf_radio_NetStatusRef_t)le_ref_CreateRef(netStatusRefMap, (void*)phonePtr);
        lteCAInfoRefs[phoneId - 1] = (taf_radio_CAInfoRef_t)le_ref_CreateRef(
            caInfoMap, (void*)lteCAInfoPtr);
        endcStatusRefs[phoneId - 1] = (taf_radio_ConnStatusRef_t)le_ref_CreateRef(
             connStatusMap, (void*)endcStatusPtr);
        taf_pa_radio_SetReference(phoneId, netStatusRefs[phoneId - 1]);
    }
    taf_pa_radio_SetLteCaHandler(LteCAHandler, nullptr);
    taf_pa_radio_SetEndcStatusHandler(EndcStatusHandler, nullptr);

    svcStatusCtxList = LE_DLS_LIST_INIT;

    le_msg_AddServiceCloseHandler(taf_radio_GetServiceRef(), ServiceStatusSessionCloseHandler, nullptr);

    // Initialize ratSvcState cache with the current modem state before registering
    // the handler, so the first real status change is never silently dropped.
    for (uint8_t phoneId = 1; phoneId <= TAF_RADIO_PHONE_NUM; phoneId++)
    {
        taf_pa_radio_ServiceStatus_t initStatus = TAF_PA_RADIO_SRV_STATUS_UNKNOWN;
        taf_pa_radio_Rat_t rat = TAF_PA_RADIO_RAT_UNKNOWN;
        le_result_t result = taf_pa_radio_GetServiceStatus(phoneId, &rat, &initStatus);
        if (result == LE_OK)
        {
            ratSvcState[phoneId - 1] = ConvertPaSvcStatusToSvcStatus(initStatus);
            LE_INFO("Init: phone=%d ratSvcState initialized to %d", phoneId, initStatus);
        }
        else
        {
            LE_WARN("Init: phone=%d GetRatSvcStatus failed result=%d, ratSvcState stays UNKNOWN",
                    phoneId, result);
        }
    }

    (void)taf_pa_radio_AddServiceStatusChangeHandler(ServiceStatusChangeHandler, nullptr);

    // 4. Get the PhoneFactory, dataFactory and PhoneManager instances
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    auto &dataFactory = telux::data::DataFactory::getInstance();

    auto phoneProm = std::make_shared<std::promise<telux::common::ServiceStatus>>();
    auto cbPhone = [phoneProm](telux::common::ServiceStatus status) {
        try {
            phoneProm->set_value(status);
        }
        catch (const std::future_error& e) {
            LE_ERROR("Future error in callback: %s", e.what());
        }
        catch (const std::exception& e) {
            LE_ERROR("Exception in callback: %s", e.what());
        }
        catch (...) {
            LE_ERROR("Unknown error in callback.");
        }
    };

    phoneManager = phoneFactory.getPhoneManager(cbPhone);

    // 5. Check if telephony subsystem is ready
    bool isReady = false;
    if (!phoneManager)
    {
        LE_FATAL("Invalid phone manager.");
    }
    else
    {
        std::future<telux::common::ServiceStatus> initFuture = phoneProm->get_future();
        std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
            TAF_RADIO_SUBSYSTEM_TIMEOUT));
        telux::common::ServiceStatus serviceStatus;
        if (std::future_status::timeout == waitStatus)
        {
            LE_FATAL("Timeout waiting for telephony susbsytem.");
        }
        else
        {
            serviceStatus = initFuture.get();
            if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
            {
                LE_FATAL("Fail to initiate telephony susbsytem.");
            }
            else
            {
                LE_INFO("Telephony subsystem is ready.");
                isReady = true;
            }
        }

        phoneListener = std::make_shared<taf_RadioPhoneListener>();

        // 6. Instantiate Phone
        std::vector<int> phoneIds;
        telux::common::Status status = phoneManager->getPhoneIds(phoneIds);
        if (status == telux::common::Status::SUCCESS)
        {
            for (size_t index = 1; index <= phoneIds.size(); index++)
            {
                auto phone = phoneManager->getPhone(index);
                if (phone != nullptr)
                {
                    phones.emplace_back(phone);
                }

                // 7. Initialize network subsystem.
                isReady = false;
                auto netProm = std::make_shared<std::promise<telux::common::ServiceStatus>>();
                auto cbNet = [netProm](telux::common::ServiceStatus status) {
                    try {
                        netProm->set_value(status);
                    }
                    catch (const std::future_error& e) {
                        LE_ERROR("Future error in callback: %s", e.what());
                    }
                    catch (const std::exception& e) {
                        LE_ERROR("Exception in callback: %s", e.what());
                    }
                    catch (...) {
                        LE_ERROR("Unknown error in callback.");
                    }
                };

                auto networkManager =
                    telux::tel::PhoneFactory::getInstance().getNetworkSelectionManager(index,
                    cbNet);
                if (!networkManager)
                {
                    LE_FATAL("Invalid network manager.");
                }
                else
                {
                    std::future<telux::common::ServiceStatus> initFuture = netProm->get_future();
                    std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                        TAF_RADIO_SUBSYSTEM_TIMEOUT));
                    if (std::future_status::timeout == waitStatus)
                    {
                        LE_FATAL("Timeout waiting for network susbsytem.");
                    }
                    else
                    {
                        serviceStatus = initFuture.get();
                        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
                        {
                            LE_FATAL("Fail to initiate network susbsytem.");
                        }
                        else
                        {
                            isReady = true;
                            LE_INFO("%" PRIuS " network subsystem is ready.", index);
                            networkManagers.emplace_back(networkManager);
                            auto networkListener =
                                std::make_shared<taf_RadioNetworkSelectionListener>();
                            networkListener->semaphore = le_sem_Create("networkListenerSem", 0);
                            networkListeners.emplace_back(networkListener);
                        }
                    }
                }

                // 8. Initialize serving subsystem.
                isReady = false;
                auto telSrvProm = std::make_shared<std::promise<telux::common::ServiceStatus>>();
                auto cbTelSrv = [telSrvProm](telux::common::ServiceStatus status) {
                    try {
                        telSrvProm->set_value(status);
                    }
                    catch (const std::future_error& e) {
                        LE_ERROR("Future error in callback: %s", e.what());
                    }
                    catch (const std::exception& e) {
                        LE_ERROR("Exception in callback: %s", e.what());
                    }
                    catch (...) {
                        LE_ERROR("Unknown error in callback.");
                    }
                };

                auto servingSystemManager =
                    telux::tel::PhoneFactory::getInstance().getServingSystemManager(index,
                    cbTelSrv);
                if (!servingSystemManager)
                {
                    LE_FATAL("Invalid serving manager.");
                }
                if (servingSystemManager != nullptr)
                {
                    std::future<telux::common::ServiceStatus> initFuture =
                        telSrvProm->get_future();
                    std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                        TAF_RADIO_SUBSYSTEM_TIMEOUT));
                    if (std::future_status::timeout == waitStatus)
                    {
                        LE_FATAL("Timeout waiting for serving susbsytem.");
                    }
                    else
                    {
                        serviceStatus = initFuture.get();
                        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
                        {
                            LE_FATAL("Fail to initiate serving susbsytem.");
                        }
                        else
                        {
                            isReady = true;
                            LE_INFO("%" PRIuS " serving subsystem is ready.", index);
                            servingSystemManagers.emplace_back(servingSystemManager);
                            auto listener = std::make_shared<taf_RadioServSysListener>((uint8_t)index);
                            servSysListeners.emplace_back(listener);
                        }
                    }
                }
            }
        }

        // 9. Instantiate RadioCallback
        signalStrengthCb = std::make_shared<taf_RadioSignalStrengthCallback>();
        voiceSrvStateCb = std::make_shared<taf_RadioVoiceServiceStateCallback>();
        setOperatingModeCb = std::make_shared<taf_RadioSetOperatingModeCallback>();
        getOperatingModeCb = std::make_shared<taf_RadioGetOperatingModeCallback>();
        cellularCapsCb = std::make_shared<taf_RadioCellularCapsCallback>();
        voiceSrvStateCb->semaphore = le_sem_Create("taf_RadioVoiceSrvStateCbSem", 0);
        signalStrengthCb->semaphore = le_sem_Create("taf_RadioSgnStrengthCbSem", 0);
        setOperatingModeCb->semaphore = le_sem_Create("taf_RadioSetOpModeCbSem", 0);
        getOperatingModeCb->semaphore = le_sem_Create("taf_RadioGetOpModeCbSem", 0);
        cellularCapsCb->semaphore = le_sem_Create("CellCapsCbSem", 0);
        dataInfoCb.semaphore = le_sem_Create("dataInfoCbSem", 0);
        nrIconCb.semaphore = le_sem_Create("nrIconCbSem", 0);
        opNameCb.semaphore = le_sem_Create("OopNameCbSem", 0);
        memset(opNameCb.longOpNamePtr, 0, TAF_RADIO_NETWORK_NAME_MAX_LEN);
        memset(opNameCb.shortOpNamePtr, 0, TAF_RADIO_NETWORK_NAME_MAX_LEN);

        for (size_t index = 1; index <= phoneIds.size(); index++)
        {
            // 10. Initialize data serving subsystem.
            isReady = false;
            int slot = (int)phoneManager->getSlotIdFromPhoneId(index);
            auto dataSrvProm = std::make_shared<std::promise<telux::common::ServiceStatus>>();
            auto cbDataSrv = [dataSrvProm](telux::common::ServiceStatus status) {
                try {
                    dataSrvProm->set_value(status);
                }
                catch (const std::future_error& e) {
                    LE_ERROR("Future error in callback: %s", e.what());
                }
                catch (const std::exception& e) {
                    LE_ERROR("Exception in callback: %s", e.what());
                }
                catch (...) {
                    LE_ERROR("Unknown error in callback.");
                }
            };
            auto servingSystemMgr = dataFactory.getServingSystemManager(
                (SlotId)slot,cbDataSrv);
            if (!servingSystemMgr)
            {
                LE_ERROR("Failed to get Data Serving System instance.");
            }
            else
            {
                telux::common::ServiceStatus dataServSysMgrStatus =
                    servingSystemMgr->getServiceStatus();
                if (dataServSysMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    LE_INFO("Data serving subsystem wait to be ready...");
                    std::future<telux::common::ServiceStatus> initFuture =
                        dataSrvProm->get_future();
                    std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                        TAF_RADIO_SUBSYSTEM_TIMEOUT));
                    if (std::future_status::timeout == waitStatus)
                    {
                        LE_FATAL ("Timeout waiting for Data serving susbsytem");
                    }
                    else
                    {
                        dataServSysMgrStatus = initFuture.get();
                    }
                }
                if (dataServSysMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    isReady = true;
                    LE_INFO("%d data serving subsystem is ready.", slot);
                    dataServSysManagers.emplace((SlotId)slot, servingSystemMgr);
                    auto listener = std::make_shared<taf_RadioDataServSysListener>((SlotId)slot);
                    dataServSysListeners.emplace((SlotId)slot, listener);
                }
                else
                {
                    LE_FATAL("Fail to init Data Serving subsystem");
                }
            }

            // 11. Initialize IMS serving subsystem.
            isReady = false;
            auto imsSrvProm = std::make_shared<std::promise<telux::common::ServiceStatus>>();
            auto cbImsSrv = [imsSrvProm](telux::common::ServiceStatus status) {
                try {
                    imsSrvProm->set_value(status);
                }
                catch (const std::future_error& e) {
                    LE_ERROR("Future error in callback: %s", e.what());
                }
                catch (const std::exception& e) {
                    LE_ERROR("Exception in callback: %s", e.what());
                }
                catch (...) {
                    LE_ERROR("Unknown error in callback.");
                }
            };
            auto imsServingSystemMgr = phoneFactory.getImsServingSystemManager(
                (SlotId)slot,cbImsSrv);

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
                    std::future<telux::common::ServiceStatus> initFuture =
                        imsSrvProm->get_future();
                    std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                        TAF_RADIO_SUBSYSTEM_TIMEOUT));
                    if (std::future_status::timeout == waitStatus)
                    {
                        LE_FATAL ("Timeout waiting for IMS serving susbsytem");
                    }
                    else
                    {
                        imsServSysMgrStatus = initFuture.get();
                    }
                }
                if (imsServSysMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
                {
                    isReady = true;
                    imsServingSystemMgrs.emplace((SlotId)slot, imsServingSystemMgr);
                    LE_INFO("%d IMS serving subsystem is ready.", slot);

                    auto listener = std::make_shared<taf_RadioImsServSysListener>((SlotId)slot);
                    imsServSysListeners.emplace((SlotId)slot, listener);
                }
                else
                {
                    LE_FATAL("Fail to init IMS Serving subsystem");
                }
            }
        }
    }

    // 12. Initialize IMS settings subsystem.
    isReady = false;
    auto imsSetProm = std::make_shared<std::promise<telux::common::ServiceStatus>>();
    auto cbImsSet = [imsSetProm](telux::common::ServiceStatus status) {
        try {
            imsSetProm->set_value(status);
        }
        catch (const std::future_error& e) {
            LE_ERROR("Future error in callback: %s", e.what());
        }
        catch (const std::exception& e) {
            LE_ERROR("Exception in callback: %s", e.what());
        }
        catch (...) {
            LE_ERROR("Unknown error in callback.");
        }
    };

    imsSettingMgr = phoneFactory.getImsSettingsManager(cbImsSet);

    if (!imsSettingMgr)
    {
        LE_ERROR("Failed to get IMS Settings instance.");
    }
    else
    {
        telux::common::ServiceStatus imsSettingMgrStatus = imsSettingMgr->getServiceStatus();
        if (imsSettingMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            LE_INFO("IMS setting subsystem wait to be ready...");
            std::future<telux::common::ServiceStatus> initFuture = imsSetProm->get_future();
            std::future_status waitStatus = initFuture.wait_for(std::chrono::seconds(
                TAF_RADIO_SUBSYSTEM_TIMEOUT));
            if (std::future_status::timeout == waitStatus)
            {
                LE_FATAL ("Timeout waiting for IMS setting susbsytem");
            }
            else
            {
                imsSettingMgrStatus = initFuture.get();
            }
        }
        if (imsSettingMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)
        {
            isReady = true;
            LE_INFO("IMS setting subsystem is ready.");
        }
        else
        {
            LE_FATAL("Fail to init IMS Setting subsystem");
        }
    }

    if (isReady)
    {
        // 13. Create and start command thread.
        le_sem_Ref_t radioCmdThreadSem = le_sem_Create("radioCmdThreadSem", 0);
        radioCmdEvId = le_event_CreateIdWithRefCounting("radioCmd");
        radioCmdCompleteEvId = le_event_CreateId("radioCmdComplete", sizeof(taf_RadioCmdComplete_t));
        le_event_AddHandler("RadioCmdCompleteHandler", radioCmdCompleteEvId, RadioCmdCompleteHandler);
        pendingCmdList = LE_DLS_LIST_INIT;
        cmdReqPool = le_mem_CreatePool("cmdReqPool", sizeof(taf_RadioCmdReq_t));
        le_msg_AddServiceCloseHandler(taf_radio_GetServiceRef(), ClientSessionCloseHandler, NULL);
        le_thread_Ref_t radioCmdThreadRef = le_thread_Create("radioCmdThread", RadioCmdThread, (void*)radioCmdThreadSem);
        le_thread_SetStackSize(radioCmdThreadRef, TAF_RADIO_THREAD_STACK_SIZE);
        le_thread_Start(radioCmdThreadRef);
        le_sem_Wait(radioCmdThreadSem);

        // 14. Delete semaphore.
        le_sem_Delete(radioCmdThreadSem);

        // 15. Add power state change handle.
        taf_pm_AddStateChangeHandler(PowerStateChangeHandler, NULL);
        if (taf_pm_GetPowerState() != TAF_PM_STATE_SUSPEND)
        {
            RegisterListener();
            taf_pa_radio_EnableIndication();
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler called when a client session is closed.
 * Clears handlerFuncPtr of any pending async cmd belonging to the closed session,
 * preventing callbacks into freed client memory.
 */
//--------------------------------------------------------------------------------------------------
void taf_Radio::ClientSessionCloseHandler(le_msg_SessionRef_t sessionRef, void* contextPtr)
{
    LE_INFO("Client session %p closed, checking pending async cmds.", sessionRef);
    auto &tafRadio = taf_Radio::GetInstance();
    le_dls_Link_t* linkPtr = le_dls_Peek(&tafRadio.pendingCmdList);
    while (linkPtr != NULL)
    {
        taf_RadioCmdReq_t* cmdPtr = CONTAINER_OF(linkPtr, taf_RadioCmdReq_t, link);
        linkPtr = le_dls_PeekNext(&tafRadio.pendingCmdList, linkPtr);
        if (cmdPtr->sessionRef == sessionRef)
        {
            LE_WARN("Clearing handler for pending cmd type %d due to session %p close.",
                cmdPtr->cmdType, sessionRef);
            cmdPtr->handlerFuncPtr = NULL;
            cmdPtr->contextPtr = NULL;
        }
    }
}
