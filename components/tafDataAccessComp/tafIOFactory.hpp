/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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