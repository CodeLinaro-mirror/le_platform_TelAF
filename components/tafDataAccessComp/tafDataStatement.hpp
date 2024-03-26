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
#ifndef TAF_DATA_STATEMENT_HPP
#define TAF_DATA_STATEMENT_HPP
#include <memory>
#include "legato.h"
#include "interfaces.h"
#include "sqlite3.h"

namespace taf{
namespace dataAccess{

    class DataStatement{
        public:
            DataStatement(sqlite3 * sqlitePtr, const char *query);

            // DataStatement is non-copyable.
            DataStatement(const DataStatement&) = delete;
            DataStatement& operator=(const DataStatement&) = delete;

            // DataStatement is movable.
            DataStatement(DataStatement&& stmt) noexcept;
            DataStatement& operator=(DataStatement&& stmt) = default;

            // Set to default. So the share_ptr can be released
            ~DataStatement() = default;

            // Notice: Calling sqlite3_reset() does not clear the bindings.
            le_result_t Reset();

            // sqlite3_reset() does not reset the value bindings in a prepared statement.
            // Using this function to reset all binding parameters to NULL.
            le_result_t ClearBindings();

            le_result_t BindValue(int index, int32_t value);
            le_result_t BindValue(int index, uint32_t value);
            le_result_t BindValue(int index, int64_t value);
            le_result_t BindValue(int index, double value);
            le_result_t BindValue(int index, const char *valuePtr);
            le_result_t BindValue(int index, const void *valuePtr, int size);
            le_result_t BindValue(int index);  // Bind a NULL value

            int GetIndex(const char *colName);

            // Execute step to get next row For SELECT.
            bool ExecuteRowStep();

            // Execute a one-step opertion for INSERT, UPDATE and DELETE.
            int ExecuteRow();

            const char *GetColumnName(int colIdx);
            int32_t GetColumnInt(int colIdx);
            uint32_t GetColumnUInt(int colIdx);
            int64_t GetColumnInt64(int colIdx);
            double GetColumnDouble(int colIdx);
            const char *GetColumnText(int colIdx, const char *defaultValue = "");
            const void *GetColumnBlob(int colIdx);

            // Pointer to the database statement.
            using StatementPtr = std::shared_ptr<sqlite3_stmt>;

        private:
            StatementPtr PrepareStatement();
            bool CheckDb();
            bool CheckDbStatement();

            // taf_DataAcsNVMType_t mType;
            sqlite3 *mSqlite3Ptr;
            std::string mQuery;
            int mColummCnt;
            StatementPtr mPrepareStmt;
            bool isDone = false;
            bool hasRow = false;
    };
}
}
#endif