/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_DID_DATA_HANDLER_HPP
#define TAF_DID_DATA_HANDLER_HPP

#include "legato.h"
#include "interfaces.h"

#include "tafDIDDataAccessComp.h"
#include "tafBaseDAO.hpp"
#include "tafDataStatement.hpp"
#include "tafIOHandler.hpp"
#include "tafDIDEntityDAO.hpp"
#include <map>

namespace taf{
namespace dataAccess{

    #define DID_DATABASE_DIR    "/data/diag/"
    #define DID_DATABASE_NAME   DID_DATABASE_DIR"did.db"
    #define DID_DATABASE_CONTEXT    "system_u:system_r:telaf_sys_t:s0-s15"

    constexpr int DID_DB_VERSION = 1;

    class DidDataHandler {
        public:
            DidDataHandler();
            ~DidDataHandler();
            static DidDataHandler &GetInstance();

            void Init();
            le_result_t Load();

            le_result_t ReadDID(uint16_t did, uint8_t *value, size_t *len);
            le_result_t WriteDID(uint16_t did, const uint8_t *value, size_t len);

        private:

    };
}
}
#endif
