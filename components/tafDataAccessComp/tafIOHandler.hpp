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
#ifndef TAF_IO_HANDLER_HPP
#define TAF_IO_HANDLER_HPP

#include <string>
#include "legato.h"
#include "interfaces.h"

#include "tafDataStatement.hpp"

namespace taf{
namespace dataAccess{

    template <typename T, typename K>
    class BaseDao;

    template <typename T, typename K>  // Template for entity type and key type
    class IOHandler {
        public:
            BaseDao<T, K> *mpDao = nullptr;    // Belong to the specific dao

            IOHandler() {};
            ~IOHandler() {};

            void AttachDao(BaseDao<T, K> *dao)
            {
                if ((mpDao != nullptr) && (mpDao != dao))
                {
                    // Throw a exception.
                    LE_ERROR("IOHandler belongs to a another DAO");
                    return;
                }

                mpDao = dao;
            }

            void DetachDao()
            {
                mpDao = nullptr;
            }

            virtual le_result_t GetVersion(int &ver) = 0;   // Get storage(e.g. database) version.
            virtual le_result_t SetVersion(int ver) = 0;  // Set storage(e.g. database) version.
            virtual le_result_t Add(T &entity) = 0;
            virtual le_result_t Remove(T &entity) = 0;
            virtual le_result_t Update(T &entity) = 0;
            virtual le_result_t QueryByKey(T &entity) = 0;

            // For DAO to execute raw-style SQL or other Data Manipulation Language.
            virtual le_result_t ExecRaw(std::string &cmd) = 0;

            // For query by multiple selection.
            virtual DataStatement Query() = 0;
            virtual DataStatement Query(std::string &where) = 0;
            virtual DataStatement QueryCount(std::string &where) = 0;
            virtual bool CheckTableExist() = 0;
            virtual bool CheckTableEmpty() = 0;
    };
}
}
#endif