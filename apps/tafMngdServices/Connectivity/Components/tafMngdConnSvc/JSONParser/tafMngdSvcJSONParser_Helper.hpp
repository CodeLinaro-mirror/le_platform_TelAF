/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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

#include "interfaces.h"
#include "tafMngdConn_Common.hpp"

    namespace tafsvc {
        typedef enum
        {
            /**
             * \brief Unknown data type
             *
             */
            MCS_JSON_DATA_TYPE_UNKNOWN = -1,

            /**
             * \brief String data type
             *
             */
            MCS_JSON_DATA_TYPE_STRING,

            /**
             * \brief Number data type
             *
             */
            MCS_JSON_DATA_TYPE_NUMBER,

            /**
             * \brief Yes/No data type, a.k.a boolean
             *
             */
            MCS_JSON_DATA_TYPE_YES_NO,

            /**
             * \brief Network Registration data type.
             *
             * Valid values:
             * - Auto
             * - Manual
             *
             */
            MCS_JSON_DATA_TYPE_NW_REGISTRATION,

            /**
             * \brief NULL data type
             *
             */
            MCS_JSON_DATA_TYPE_NULL,

            /**
             * \brief Object data type is not used
             *
             */
            MCS_JSON_DATA_TYPE_OBJECT,

            /**
             * \brief Array data type is not used
             *
             */
            MCS_JSON_DATA_TYPE_ARRAY,

            /**
             * \brief None Data type when ConnectivityRecovery is disabled
             *
             */
            MCS_JSON_DATA_TYPE_NONE,

            /**
             * \brief ConnectivityRecovery Level data type.
             *
             */
            MCS_JSON_DATA_TYPE_CONNRECOVERY_LEVEL

        } mcs_JSON_Data_Types_t;

        /*
        * This function will return the data type of the value passed to it
        *
        * @param [in] Value       Value (in string format) read from JSON
        */
        mcs_JSON_Data_Types_t mcs_GetDataType(std::string Value);

        /*
        * This function will return mcs_Yes_No_t enum from Yes/No string
        *
        * @param [in] Value       Yes/No (in string format) read from JSON
        */
        mcs_Yes_No_t mcs_Convert_to_Yes_No_enum(std::string Value);

        /*
        * This function will return mcs_NW_Registration_Type_t enum from registration type string.
        *
        * @param [in] Value       Auto/Manual (in string format) read from JSON
        */
        mcs_NW_Registration_Type_t mcs_Convert_to_NW_Registration_Type_enum(std::string Value);

        /*
        * This function will return mcs_Policy_ConnRecoveryLevel_t enum from None/L1/L2/L3 string
        *
        * @param [in] Value   None/L1/L2/l3 (in string format) read from JSON
        */
        mcs_Policy_ConnRecoveryLevel_t mcs_Convert_to_ConnRecovery_Level_Type_enum
                                                (std::string Value);
    }
