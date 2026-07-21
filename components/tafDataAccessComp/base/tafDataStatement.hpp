/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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

            int GetColumnBytes(int colIdx);

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