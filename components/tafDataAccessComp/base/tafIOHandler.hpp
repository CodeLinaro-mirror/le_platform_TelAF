/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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
            virtual le_result_t EnableWAL() = 0;
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