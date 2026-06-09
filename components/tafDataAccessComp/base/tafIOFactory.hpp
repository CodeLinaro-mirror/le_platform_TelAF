/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_IO_FACTORY_HPP
#define TAF_IO_FACTORY_HPP

#include <vector>
#include <memory>

#include "legato.h"
#include "interfaces.h"

#include "tafBaseDAO.hpp"
#include "tafIOHandler.hpp"
#include "tafSQLite3Handler.hpp"

namespace taf{
namespace dataAccess{
    // Data access nvm type
    typedef enum
    {
        NVM_TYPE_CONFIGTREE = 0,
        NVM_TYPE_SQLITE3,
        NVM_TYPE_UNKNOWN
    }taf_DataAcsNVMType_t;

    template <typename T, typename K>
    class SQLite3Handler;

    class IOFactory{
        public:
            IOFactory() {}
            ~IOFactory() {}

            static IOFactory &GetInstance();
            void Init();

            template <typename T, typename K>
            std::shared_ptr<IOHandler<T, K>> getHandler(taf_DataAcsNVMType_t nvmType,
                                                            const char *nvmName)
            {
                if (nvmType == NVM_TYPE_SQLITE3)
                {
                    return std::make_shared<SQLite3Handler<T, K>>(nvmName);
                }
                else
                {
                    LE_ERROR("IO handler for nvm type%d is not implemented.", nvmType);
                    return nullptr;
                }
            }
        private:

    };
}
}
#endif