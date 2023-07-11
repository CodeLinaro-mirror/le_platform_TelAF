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
    const signed int TAF_MNGD_CONN_INVALID_INDEX = -1;

    /**
     * \brief Maximum string length for APN.
     *
     * Max APN length is 128 characters, including null terminator.
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_APN_LEN = 128;

    /**
     * \brief Maximum supported Data objects
     *
     * Number of Data objects in the Configuration JSON that the Connectivity service can manage.
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_DATA_OBJECT_COUNT = 2;

    /**
     * \brief Maximum supported Data Connection objects
     *
     * Number of Data Connection objects in the Policy JSON that the Connectivity service can manage.
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_DATA_CONNECION_OBJECT_COUNT = 2;

    /**
     * \brief Maximum string length for file names, including path.
     *
     * Max length is 256 characters, including null terminator.
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_FILE_NAME_LEN = 256;

    /**
     * \brief Maximum string length for IPv4 address.
     *
     * Max length is 16 characters, including null terminator.
     * 111.222.333.444
     */
    const unsigned int TAF_MNGD_CONN_MAX_IPV4_LEN = 16;

    /**
     * \brief Maximum string length for IPv6 address.
     *
     * Max length is 40 characters, including null terminator.
     * 1111:2222:3333:4444:5555:6666:7777:8888
     * 8*4+7+1 = 40
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_IPV6_LEN = 40;

    /**
     * \brief Maximum string length for names, including file names.
     *
     * Max name length is 32 characters, including null terminator.
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_NAME_LEN = 32;

    /**
     * \brief Maximum string length for Configuration JSON network registartion data type.
     *
     * Max name length is 8 characters, including null terminator.
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_NW_REGISTRATION_TYPE_LEN = 8;

    /**
     * \brief Maximum string length for URL for ping test.
     *
     * Max URL length is 256 characters, including null terminator.
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_PING_URL_LEN = 256;

    /**
     * \brief Maximum string length for profile names.
     *
     * Max profile name length is 16 characters, including null terminator.
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_PROFILE_NAME_LEN = 16;

    /**
     * \brief Maximum supported Nework objects
     *
     * Number of Network objects in the Configuration JSON that the Connectivity service can manage.
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_NETWORK_OBJECT_COUNT = 2;

    /**
     * \brief Maximum string length for Registration values. The valid values are:
     * - Auto
     * - Manual
     *
     * Max name length is 8 characters, including null terminator.
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_REGISTRATION_STRING_LEN = 8;

    /**
     * \brief Maximum supported Sim objects
     *
     * Number of SIM objects in the Configuration JSON that the Connectivity service can manage.
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_SIM_OBJECT_COUNT = 2;

    /**
     * \brief Maximum supported simultaneous data sessions.
     *
     * Maximum supported simultaneous data sessions. It is 1 for Alpha 1 release.
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_SIMULTANEOUS_DATA_SESSION_COUNT = 1;

    /**
     * \brief Maximum string length of Yes/No values
     *
     * Yes/No string is 4 characters in length, including null terminator.
     *
     */
    const unsigned int TAF_MNGD_CONN_MAX_YES_NO_LEN = 4;

    typedef enum
    {
        TAF_MNGD_CONN_NO = 0, /**<  Value of No. 0 */
        TAF_MNGD_CONN_YES = 1 /**<  Value of Yes. 1 */
    } taf_mngd_Yes_No_t;

    typedef enum
    {
        TAF_MNGD_CONN_MAX_NW_REGISTRATION_TYPE_AUTO   = 0, /**<  Auto value. 0 */
        TAF_MNGD_CONN_MAX_NW_REGISTRATION_TYPE_MANUAL = 1  /**<  Manual Value. 1 */
    } taf_mngd_NW_Registration_Type_t;

    // Constant Strings


    /**
     * \brief Default Location for Configuration JSONs
     *
     */
    const std::string TAF_MNGD_DefaultLocation_Configuration("/data/ManagedServices");

}
}