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

/*! \page tafMngdSvcJSONParser_Helper Managed Service JSON Helper
 * This module provdies helper functions to help parse JSONs.
 *
*/
/**
 * \file tafMngdSvcJSONParser_Helper.hpp
 * This module provdies helper functions to help parse JSONs.
 *
 */
//-----------------------------------------------------------

#pragma once

#include "tafMngdConn_Common.hpp"

namespace telux {
namespace tafsvc {
    typedef enum
    {
        /**
         * \brief Unknown data type
         *
         */
        TAF_MNGD_JSON_DATA_TYPE_UNKNOWN = -1,

        /**
         * \brief String data type
         *
         */
        TAF_MNGD_JSON_DATA_TYPE_STRING,

        /**
         * \brief Number data type
         *
         */
        TAF_MNGD_JSON_DATA_TYPE_NUMBER,

        /**
         * \brief Yes/No data type, a.k.a boolean
         *
         */
        TAF_MNGD_JSON_DATA_TYPE_YES_NO,

        /**
         * \brief Network Registration data type.
         *
         * Valid values:
         * - Auto
         * - Manual
         *
         */
        TAF_MNGD_JSON_DATA_TYPE_NW_REGISTRATION,

        /**
         * \brief NULL data type
         *
         */
        TAF_MNGD_JSON_DATA_TYPE_NULL,

        /**
         * \brief Object data type is not used
         *
         */
        TAF_MNGD_JSON_DATA_TYPE_OBJECT,

        /**
         * \brief Array data type is not used
         *
         */
        TAF_MNGD_JSON_DATA_TYPE_ARRAY,

        /**
         * \brief None Data type when ConnectivityRecovery is disabled
         *
         */
        TAF_MNGD_JSON_DATA_TYPE_NONE,

        /**
         * \brief ConnectivityRecovery Level data type.
         *
         */
        TAF_MNGD_JSON_DATA_TYPE_CONNRECOVERY_LEVEL

    } taf_mngd_JSON_Data_Types_t;

    /*
     * This function will return the data type of the value passed to it
     *
     * @param [in] Value       Value (in string format) read from JSON
     */
    taf_mngd_JSON_Data_Types_t tafMngd_GetDataType(std::string Value);

    /*
     * This function will return taf_mngd_Yes_No_t enum from Yes/No string
     *
     * @param [in] Value       Yes/No (in string format) read from JSON
     */
    taf_mngd_Yes_No_t tafMngd_Convert_to_Yes_No_enum(std::string Value);

    /*
     * This function will return taf_mngd_NW_Registration_Type_t enum from registration type string.
     *
     * @param [in] Value       Auto/Manual (in string format) read from JSON
     */
    taf_mngd_NW_Registration_Type_t tafMngd_Convert_to_NW_Registration_Type_enum(std::string Value);

    /*
     * This function will return taf_mngd_Conn_Policy_ConnRecoveryLevel_t enum from None/L1 string
     *
     * @param [in] Value       None/L1 (in string format) read from JSON
     */
    taf_mngd_Conn_Policy_ConnRecoveryLevel_t tafMngd_Convert_to_ConnRecovery_Level_Type_enum
                                            (std::string Value);

}
}