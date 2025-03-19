/*
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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

//-----------------------------------------------------------

/*! \page tafMngdConn_Common Managed Connectivity Service Common Header
 * Provides common definitions and values used by the Managed Connectivity service.
 *
*/
/**
 * \file tafMngdConn_Common.hpp
 * tafMngdConn_Common.hpp provides common definitions and values used by the Managed Connectivity
 * service.
 *
 */
//-----------------------------------------------------------
#include <string>

#pragma once
namespace telux {
namespace tafsvc {
    /**
     * \brief Invalid index
     *
     */
    const signed int MCS_INVALID_INDEX = -1;

    /**
     * \brief Maximum string length for APN.
     *
     * Max APN length is 64 characters, including null terminator.
     *
     */
    const unsigned int MCS_MAX_APN_LEN = 64;

    /**
     * \brief Maximum supported Data objects
     *
     * Number of Data objects in the Configuration JSON that the Connectivity service can manage.
     *
     */
    const unsigned int MCS_MAX_DATA_OBJECT_COUNT = 4;

    /**
     * \brief Maximum supported Data Connection objects
     *
     * Number of Data Connection objects in the Policy JSON that the Connectivity
     * service can manage.
     *
     */
    const unsigned int MCS_MAX_DATA_CONNECION_OBJECT_COUNT = 2;

    /**
     * \brief Maximum string length for file names, including path.
     *
     * Max length is 256 characters, including null terminator.
     *
     */
    const unsigned int MCS_MAX_FILE_NAME_LEN = 256;

    /**
     * \brief Maximum string length for IPv4 address.
     *
     * Max length is 16 characters, including null terminator.
     * 111.222.333.444
     */
    const unsigned int MCS_MAX_IPV4_LEN = 16;

    /**
     * \brief Maximum string length for IPv6 address.
     *
     * Max length is 40 characters, including null terminator.
     * 1111:2222:3333:4444:5555:6666:7777:8888
     * 8*4+7+1 = 40
     *
     */
    const unsigned int MCS_MAX_IPV6_LEN = 40;

    /**
     * \brief Maximum string length for names, including file names.
     *
     * Max name length is 32 characters, including null terminator.
     *
     */
    const unsigned int MCS_MAX_NAME_LEN = TAF_MNGDCONN_MAX_NAME_LEN;

    /**
     * \brief Maximum string length for Configuration JSON network registartion data type.
     *
     * Max name length is 8 characters, including null terminator.
     *
     */
    const unsigned int MCS_MAX_NW_REGISTRATION_TYPE_LEN = 8;

    /**
     * \brief Maximum string length for URL for connection test.
     *
     * Max URL length is 256 characters, including null terminator.
     *
     */
    const unsigned int MCS_MAX_CONNECTION_URL_LEN = 256;

    /**
     * \brief Maximum supported Nework objects
     *
     * Number of Network objects in the Configuration JSON that the Connectivity service can manage.
     *
     */
    const unsigned int MCS_MAX_NETWORK_OBJECT_COUNT = 2;

    /**
     * \brief Maximum string length for Registration values. The valid values are:
     * - Auto
     * - Manual
     *
     * Max name length is 8 characters, including null terminator.
     *
     */
    const unsigned int MCS_MAX_REGISTRATION_STRING_LEN = 8;

    /**
     * \brief Maximum supported Sim objects
     *
     * Number of SIM objects in the Configuration JSON that the Connectivity service can manage.
     *
     */
    const unsigned int MCS_MAX_SIM_OBJECT_COUNT = 2;

    /**
     * \brief Maximum supported simultaneous data sessions.
     *
     * Maximum supported simultaneous data sessions. It is 1 for Alpha 1 release.
     *
     */
    const unsigned int MCS_MAX_SIMULTANEOUS_DATA_SESSION_COUNT = 1;

    /**
     * \brief Maximum string length of Yes/No values
     *
     * Yes/No string is 4 characters in length, including null terminator.
     *
     */
    const unsigned int MCS_MAX_YES_NO_LEN = 4;

    typedef enum
    {
        MCS_NO = 0, /**<  Value of No. 0 */
        MCS_YES = 1 /**<  Value of Yes. 1 */
    } mcs_Yes_No_t;

    typedef enum
    {
        MCS_NW_REGISTRATION_TYPE_AUTO   = 0, /**<  Auto value. 0 */
        MCS_NW_REGISTRATION_TYPE_MANUAL = 1  /**<  Manual Value. 1 */
    } mcs_NW_Registration_Type_t;

    typedef enum
    {
        MCS_CONNECTIONRECOVERY_LEVEL_NONE = 0, /**<  DISABLED value. 0 */
        MCS_CONNECTIONRECOVERY_LEVEL_L1 = 1,   /**<  L1 Value. 1 */
        MCS_CONNECTIONRECOVERY_LEVEL_L2 = 2,    /**<  L2 Value. 2 */
        MCS_CONNECTIONRECOVERY_LEVEL_L3 = 3    /**<  L3 Value. 3 */
    } mcs_Policy_ConnRecoveryLevel_t;

    /**
     * \brief Enumeration for valid JSON versions
     *
     * This enum will be mapped to a version string when mngdConnectivity.json is parsed.
     * The valid versions strings are listed in tafMngdConnSvcJSONParser.hpp
     *
     * The enum uses numbers from the version string for easier readbility. Examples:
     * TAF_23_07_00 = 230700
     * TAF_23_10_00 = 231000
     * TAF_24_01_00 = 240100
     *
     */
    typedef enum
    {
        MCS_JSON_VERSION_23_07_00 = 230700, //"TAF_23.07.00"
        MCS_JSON_VERSION_23_11_00 = 231100, //"TAF_23.11.00"
        MCS_JSON_VERSION_24_03_00 = 240300, //"TAF_24.03.00"
        MCS_JSON_VERSION_24_06_00 = 240600, //"TAF_24.06.00"
        MCS_JSON_VERSION_24_07_00 = 240700,  //"TAF_24.07.00"
        MCS_JSON_VERSION_24_09_00 = 240900,  //"TAF_24.09.00"
        MCS_JSON_VERSION_24_12_00 = 241200,  //"TAF_24.12.00"
        MCS_JSON_VERSION_25_02_00 = 250200,  //"TAF_25.02.00"
        MCS_JSON_VERSION_25_03_00 = 250300  //"TAF_25.03.00"
    } mcs_JSON_Version_t;

    // Constant Strings

    /**
     * \brief Value for Product in mngdConnectivity.json should be TelAF
     *
     */
    const std::string MCS_Default_Product_Value("TelAF");

    /**
     * \brief Maximum level of supported RecoveryLevel
     *
     * Maximum RecoveryLevel is 4 for Alpha2 Release
     *
     */
    const int MCS_MAX_RECOVERY_LEVEL = 4;
}
}