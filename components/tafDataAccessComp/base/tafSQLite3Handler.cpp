/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <strings.h>

#include "legato.h"
#include "interfaces.h"

#include "tafBaseDAO.hpp"
#include "tafIOFactory.hpp"
#include "tafSQLite3Handler.hpp"

using namespace taf::dataAccess;

DatabaseCb SQLite3DbUtil::mExecCb = nullptr;
int SQLite3DbUtil::mRowNum = 0;

SQLite3DbUtil::SQLite3DbUtil
(
)
{
}

SQLite3DbUtil::~SQLite3DbUtil
(
)
{
}

SQLite3DbUtil &SQLite3DbUtil::GetInstance
(
)
{
    static SQLite3DbUtil instance;

    return instance;
}

le_result_t SQLite3DbUtil::Open
(
    const char *dbNamePtr
)
{
    if (mDbPtr != nullptr)
    {
        return LE_IO_ERROR;
    }

    int ret = sqlite3_open(dbNamePtr, &mDbPtr);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Open DB(%s) error: %s, ret=%d", dbNamePtr, sqlite3_errmsg(mDbPtr), ret);
        return LE_IO_ERROR;
    }

    mDbName.assign(dbNamePtr);
    LE_DEBUG("sqlite3=%p", mDbPtr);

    return LE_OK;
}

bool SQLite3DbUtil::IsOpen
(
)
{
    if (mDbPtr == nullptr)
    {
        return false;
    }
    else
    {
        return true;
    }
}

le_result_t SQLite3DbUtil::Close
(
)
{
    if (mDbPtr == nullptr)
    {
        LE_ERROR("Database is not open.");
        return LE_CLOSED;
    }

    int ret = sqlite3_close(mDbPtr);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Close DB(%s) error: %s, ret=%d", mDbName.c_str(), sqlite3_errmsg(mDbPtr), ret);
        return LE_IO_ERROR;
    }
    else
    {
        LE_INFO("Close DB successful");
    }

    mDbPtr = nullptr;

    return LE_OK;
}

int SQLite3DbUtil::ExecSQL
(
    const char *sql
)
{
    char *errMsg = 0;

    if (mDbPtr == nullptr)
    {
        LE_ERROR("Database is not open.");
        return -1;
    }

    int ret = sqlite3_exec(mDbPtr, sql, nullptr, nullptr, &errMsg);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Execute SQL(%s ) error: ", sql);
        LE_ERROR("%s, ret=%d", errMsg, ret);
        sqlite3_free(errMsg);
        return -1;
    }

    // Get the number of rows modified by sql(INSERT, UPDATE or DELETE only).
    return sqlite3_changes(mDbPtr);
}

int SQLite3DbUtil::ExecSQL
(
    const char *sql,
    int (*callback)(int, char**, char**),
    void *data
)
{
    int nb;
    char *errMsg = nullptr;

    if (mDbPtr == nullptr)
    {
        LE_ERROR("Database is not open.");
        return -1;
    }

    // Save user information first when executing sql.
    mRowNum = 0;
    mExecCb = callback;

    int ret = sqlite3_exec(mDbPtr, sql, ExecuteCallback, data, &errMsg);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Execute SQL(%s) error: ", sql);
        LE_ERROR("%s, ret=%d", errMsg, ret);
        sqlite3_free(errMsg);
        return -1;
    }

    LE_DEBUG("Excure %s get %d row(s)", sql, mRowNum);
    nb = mRowNum;
    mExecCb = nullptr;
    mRowNum = 0;

    // Get the number of rows modified by sql(INSERT, UPDATE or DELETE only).
    return nb;
}

DataStatement SQLite3DbUtil::ExecQuery
(
    const char *sql
)
{
    if (strncasecmp("SELECT", sql, 6) != 0)
    {
        LE_WARN("%s is not a query sql", sql);
        // Still execute
    }

    return DataStatement(mDbPtr, sql);
}

sqlite3 *SQLite3DbUtil::GetDbHandle
(
)
{
    return mDbPtr;
}

le_result_t SQLite3DbUtil::GetVersion
(
    int &ver
)
{
    int ret;
    char *errMsg = nullptr;
    const char *sql = "PRAGMA user_version;";

    if (mDbPtr == nullptr)
    {
        LE_ERROR("Database is not open.");
        return LE_BAD_PARAMETER;
    }

    ret = sqlite3_exec(mDbPtr, sql, [](void *data, int argc, char **argv, char **azColName)
        {
            int *verPtr = (int *)data;
            
            *verPtr = argv != nullptr ? (argv[0] != nullptr ? atoi(argv[0]) : 0) : 0;
            return 0;
        }, (void *)&ver, &errMsg);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Get DB version error: %s, ret=%d", errMsg, ret);
        sqlite3_free(errMsg);
        return LE_IO_ERROR;
    }

    LE_DEBUG("DB version is %d", ver);

    return LE_OK;
}

le_result_t SQLite3DbUtil::SetVersion
(
    int ver
)
{
    int ret;
    char *errMsg = nullptr;
    std::stringstream sqlSs;

    sqlSs << "PRAGMA user_version = " << ver << ";";

    if (mDbPtr == nullptr)
    {
        LE_ERROR("Database is not open.");
        return LE_BAD_PARAMETER;
    }

    std::string sql = sqlSs.str();
    LE_DEBUG("QueryByKey statement: %s", sql.c_str());

    ret = sqlite3_exec(mDbPtr, sql.c_str(), nullptr, nullptr, &errMsg);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Set DB version error: %s, ret=%d", errMsg, ret);
        sqlite3_free(errMsg);
        return LE_IO_ERROR;
    }

    return LE_OK;
}

le_result_t SQLite3DbUtil::EnableWAL
(
)
{
    int ret;
    char *errMsg = nullptr;
    std::stringstream sqlSs;

    sqlSs << "PRAGMA journal_mode = WAL;";
    sqlSs << "PRAGMA synchronous = NORMAL;";
    sqlSs << "PRAGMA wal_autocheckpoint = 1000;";

    if (mDbPtr == nullptr)
    {
        LE_ERROR("Database is not open.");
        return LE_BAD_PARAMETER;
    }

    std::string sql = sqlSs.str();
    LE_DEBUG("sql: %s", sql.c_str());

    ret = sqlite3_exec(mDbPtr, sql.c_str(), nullptr, nullptr, &errMsg);
    if (ret != SQLITE_OK)
    {
        LE_ERROR("Enable WAL error: %s, ret=%d", errMsg, ret);
        sqlite3_free(errMsg);
        return LE_IO_ERROR;
    }

    return LE_OK;
}

int SQLite3DbUtil::ExecuteCallback
(
    void *data,
    int argc,
    char **argv,
    char **azCloName
)
{
    if (mExecCb != nullptr)
    {
        mRowNum++;
        return mExecCb(argc, argv, azCloName);
    }

    return LE_OK;
}
