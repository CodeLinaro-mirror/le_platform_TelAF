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

le_result_t SQLite3DbUtil::Open
(
    const char *dbNamePtr
)
{
    if (mDbPtr != nullptr)
    {
        LE_WARN("DB is open for %s, can not open for %s", mDbName.c_str(), dbNamePtr);
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
    char *errMsg = 0;

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
