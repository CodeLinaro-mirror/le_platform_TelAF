/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include "tafBaseDAO.hpp"
#include "tafIOHandler.hpp"

using namespace taf::dataAccess;

DataStatement::DataStatement
(
    sqlite3 *sqlitePtr,
    const char *query
)
:
// mType(NVM_TYPE_SQLITE3),
mSqlite3Ptr(sqlitePtr),
mQuery(query),
mPrepareStmt(PrepareStatement())
{
    if (mPrepareStmt == nullptr)
    {
        mColummCnt = 0;
    }
    else
    {
        mColummCnt = sqlite3_column_count(mPrepareStmt.get());
    }
}

le_result_t DataStatement::Reset
(
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return LE_CLOSED;
    }

    int ret = sqlite3_reset(mPrepareStmt.get());
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Statement reset error:%s, ret=%d",
            sqlite3_errmsg(mSqlite3Ptr), ret);
        return LE_IO_ERROR;
    }

    isDone = false;
    hasRow = false;

    return LE_OK;
}

le_result_t DataStatement::ClearBindings
(
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return LE_CLOSED;
    }

    int ret = sqlite3_clear_bindings(mPrepareStmt.get());
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Statement clear bindings error:%s, ret=%d",
            sqlite3_errmsg(mSqlite3Ptr), ret);
        return LE_IO_ERROR;
    }

    return LE_OK;
}

le_result_t DataStatement::BindValue
(
    int index,
    int32_t value
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return LE_CLOSED;
    }

    int ret = sqlite3_bind_int(mPrepareStmt.get(), index, value);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Statement bind row%d error:%s, ret=%d",
            index, sqlite3_errmsg(mSqlite3Ptr), ret);
        return LE_IO_ERROR;
    }

    return LE_OK;
}

le_result_t DataStatement::BindValue
(
    int index,
    uint32_t value
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return LE_CLOSED;
    }

    int ret = sqlite3_bind_int64(mPrepareStmt.get(), index, value);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Statement bind row%d error:%s, ret=%d",
            index, sqlite3_errmsg(mSqlite3Ptr), ret);
        return LE_IO_ERROR;
    }

    return LE_OK;
}

le_result_t DataStatement::BindValue
(
    int index,
    int64_t value
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return LE_CLOSED;
    }

    int ret = sqlite3_bind_int64(mPrepareStmt.get(), index, value);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Statement bind row%d error:%s, ret=%d",
            index, sqlite3_errmsg(mSqlite3Ptr), ret);
        return LE_IO_ERROR;
    }

    return LE_OK;
}

le_result_t DataStatement::BindValue
(
    int index,
    double value
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return LE_CLOSED;
    }

    int ret = sqlite3_bind_double(mPrepareStmt.get(), index, value);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Statement bind row%d error:%s, ret=%d",
            index, sqlite3_errmsg(mSqlite3Ptr), ret);
        return LE_IO_ERROR;
    }

    return LE_OK;
}

le_result_t DataStatement::BindValue
(
    int index,
    const char *valuePtr
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return LE_CLOSED;
    }

    int ret = sqlite3_bind_text(mPrepareStmt.get(), index, valuePtr, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Statement bind row%d error:%s, ret=%d",
            index, sqlite3_errmsg(mSqlite3Ptr), ret);
        return LE_IO_ERROR;
    }

    return LE_OK;
}

le_result_t DataStatement::BindValue
(
    int index,
    const void *valuePtr,
    int size
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return LE_CLOSED;
    }

    int ret = sqlite3_bind_blob(mPrepareStmt.get(), index, valuePtr, size, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Statement bind row%d error:%s, ret=%d",
            index, sqlite3_errmsg(mSqlite3Ptr), ret);
        return LE_IO_ERROR;
    }

    return LE_OK;
}

le_result_t DataStatement::BindValue
(
    int index
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return LE_CLOSED;
    }

    int ret = sqlite3_bind_null(mPrepareStmt.get(), index);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Statement bind row%d error:%s, ret=%d",
            index, sqlite3_errmsg(mSqlite3Ptr), ret);
        return LE_IO_ERROR;
    }

    return LE_OK;
}

int DataStatement::GetIndex
(
    const char *colName
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return -1;
    }

    return sqlite3_bind_parameter_index(mPrepareStmt.get(), colName);
}

bool DataStatement::ExecuteRowStep
(
)
{
    if (!CheckDb())
    {
        LE_ERROR("Database is not open.");
        return false;
    }

    if (isDone == true)
    {
        LE_WARN("All row is read. Need to be reseted first");
        return false;
    }

    int ret = sqlite3_step(mPrepareStmt.get());
    if (ret == SQLITE_ROW)
    {
        hasRow = true;
    }
    else if (ret == SQLITE_DONE)
    {
        hasRow = false;
        isDone = true;
    }
    else
    {
        LE_ERROR("Execute step error: %s, ret=%d",
            sqlite3_errmsg(mSqlite3Ptr), ret);
        hasRow = false;
    }

    return hasRow;
}

int DataStatement::ExecuteRow
(
)
{
    if (!CheckDb())
    {
        LE_ERROR("Database is not open.");
        return -1;
    }

    if (isDone == true)
    {
        LE_WARN("All row is executed. Need to be reseted first");
        return 0;
    }

    int ret = sqlite3_step(mPrepareStmt.get());
    if (ret == SQLITE_ROW)
    {
        LE_ERROR("Return a unexpected result, use ExecuteRowStep() instead");
        hasRow = true;
        return 0;
    }
    else if (ret == SQLITE_DONE)
    {
        hasRow = false;
        isDone = true;
        return sqlite3_changes(mSqlite3Ptr);
    }
    else
    {
        LE_ERROR("Execute step error: %s, ret=%d",
            sqlite3_errmsg(mSqlite3Ptr), ret);
        hasRow = false;
        return -1;
    }
}

const char *DataStatement::GetColumnName
(
    int colIdx
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return nullptr;
    }

    return sqlite3_column_name(mPrepareStmt.get(), colIdx);
}

int32_t DataStatement::GetColumnInt
(
    int colIdx
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return 0;
    }

    return sqlite3_column_int(mPrepareStmt.get(), colIdx);
}

uint32_t DataStatement::GetColumnUInt
(
    int colIdx
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return 0;
    }

    return static_cast<uint32_t>(sqlite3_column_int64(mPrepareStmt.get(), colIdx));
}

int64_t DataStatement::GetColumnInt64
(
    int colIdx
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return 0;
    }

    return sqlite3_column_int64(mPrepareStmt.get(), colIdx);
}

double DataStatement::GetColumnDouble
(
    int colIdx
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return 0.0;
    }

    return sqlite3_column_double(mPrepareStmt.get(), colIdx);
}

const char *DataStatement::GetColumnText
(
    int colIdx,
    const char *defaultValue
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return defaultValue;
    }

    auto textPtr = reinterpret_cast<const char*>(sqlite3_column_text(mPrepareStmt.get(), colIdx));

    if (textPtr == nullptr)
    {
        return defaultValue;
    }

    return textPtr;
}

const void *DataStatement::GetColumnBlob
(
    int colIdx
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return nullptr;
    }

    return sqlite3_column_blob(mPrepareStmt.get(), colIdx);
}

int DataStatement::GetColumnBytes
(
    int colIdx
)
{
    if (!CheckDbStatement())
    {
        LE_ERROR("Database statement is null.");
        return 0;
    }

    return sqlite3_column_bytes(mPrepareStmt.get(), colIdx);
}

DataStatement::StatementPtr DataStatement::PrepareStatement
(
)
{
    sqlite3_stmt *stmt;

    if (!CheckDb())
    {
        LE_ERROR("Database is not open.");
        return nullptr;
    }

    LE_DEBUG("sqlite3=%p", mSqlite3Ptr);
    int ret = sqlite3_prepare_v2(mSqlite3Ptr, mQuery.c_str(),
        static_cast<int>(mQuery.size()), &stmt, nullptr);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Execute prepare(%s) error: ",
            mQuery.c_str());
        LE_ERROR("%s, ret=%d", sqlite3_errmsg(mSqlite3Ptr), ret);
        return nullptr;
    }

    return DataStatement::StatementPtr(stmt, [](sqlite3_stmt *stmt)
        {
            LE_DEBUG("Data state finalize.");

            // Delete sqlite statement resource when statement object delete.
            sqlite3_finalize(stmt);
        });
}

bool DataStatement::CheckDb()
{
    if (mSqlite3Ptr == nullptr)
    {
        return false;
    }

    return true;
}

bool DataStatement::CheckDbStatement()
{
    if (mPrepareStmt == nullptr)
    {
        return false;
    }

    return true;
}
