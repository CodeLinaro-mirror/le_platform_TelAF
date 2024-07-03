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
#ifndef TAF_SQLITE3_HANDLER_HPP
#define TAF_SQLITE3_HANDLER_HPP

#include <sstream>
#include <string>
#include "legato.h"
#include "interfaces.h"
#include "sqlite3.h"

#include "tafBaseDAO.hpp"
#include "tafDataStatement.hpp"
#include "tafIOHandler.hpp"

using namespace taf::dataAccess;

namespace taf{
namespace dataAccess{
    typedef int (*DatabaseCb)(int, char**, char**);

    class SQLite3DbUtil {
        public:
            SQLite3DbUtil();
            ~SQLite3DbUtil();

            le_result_t Open(const char *dbNamePtr);
            le_result_t Close();
            bool IsOpen();

            int ExecSQL(const char *sql);

            static DatabaseCb mExecCb;
            static int mRowNum;
            int ExecSQL(const char *sql, int (*callback)(int, char**, char**), void *);

            // statement function
            DataStatement ExecQuery(const char *sql);

            // Transation function

            sqlite3 *GetDbHandle();
            le_result_t GetVersion(int &ver);
            le_result_t SetVersion(int ver);
        private:
            static int ExecuteCallback(void *data, int argc, char **argv, char **azCloName);

            sqlite3 *mDbPtr = nullptr;
            std::string mDbName;
    };

    template <typename T, typename K>
    class SQLite3Handler : public IOHandler<T, K> {
        public:
            SQLite3Handler(const char *dbName) :
                    mDbName(dbName)
            {
                mDb.Open(dbName);
            }
            ~SQLite3Handler()
            {
                if (mpInsertStatement != nullptr)
                {
                    std::move(mpInsertStatement);
                }

                if (mpInsOrRepStatement != nullptr)
                {
                    std::move(mpInsOrRepStatement);
                }

                if (mpUpdateStatement != nullptr)
                {
                    std::move(mpUpdateStatement);
                }

                if (mpDeleteStatement != nullptr)
                {
                    std::move(mpDeleteStatement);
                }

                // This function is only called when the component exit.
                // Wait 3s to ensure the db statement is finalized.
                le_thread_Sleep(3);

                if (mDb.IsOpen())
                {
                    mDb.Close();
                }
            }

            le_result_t GetVersion(int &ver) override
            {
                return mDb.GetVersion(ver);
            }

            le_result_t SetVersion(int ver) override
            {
                return mDb.SetVersion(ver);
            }

            le_result_t Add(T &entity) override
            {
                DataStatement *statement = GetInsertStatement();
                if (statement == nullptr)
                {
                    LE_ERROR("Failed to prepare add statement.");
                    return LE_FAULT;
                }

                IOHandler<T, K>::mpDao->BindValues(*statement, entity);
                int rowId = statement->ExecuteRow();
                if (rowId < 0)
                {
                    LE_ERROR("Failed to add entity.");
                    return LE_IO_ERROR;
                }

                LE_DEBUG("Add row%d", rowId);

                return LE_OK;
            }

            le_result_t Remove(T &entity) override
            {
                DataStatement *statement = GetDeleteStatement();
                if (statement == nullptr)
                {
                    LE_ERROR("Failed to prepare delete statement.");
                    return LE_FAULT;
                }

                IOHandler<T, K>::mpDao->BindKeyValue(*statement, entity);
                int rowId = statement->ExecuteRow();
                if (rowId < 0)
                {
                    LE_ERROR("Failed to delete entity.");
                    return LE_IO_ERROR;
                }

                LE_DEBUG("Delete row%d", rowId);

                return LE_OK;
            }

            le_result_t Update(T &entity) override
            {
                DataStatement *statement = GetUpdateStatement();
                if (statement == nullptr)
                {
                    LE_ERROR("Failed to prepare update statement.");
                    return LE_FAULT;
                }

                IOHandler<T, K>::mpDao->BindValues(*statement, entity);
                std::vector<std::string> columns = IOHandler<T, K>::mpDao->GetColumnsName();
                int index = columns.size() + 1;
                K key = IOHandler<T, K>::mpDao->GetKey(entity);

                statement->BindValue(index, key);  // Only support single pk currently.
                int rowId = statement->ExecuteRow();
                if (rowId < 0)
                {
                    LE_ERROR("Failed to update entity.");
                    return LE_IO_ERROR;
                }

                LE_DEBUG("Update row%d", rowId);

                return LE_OK;
            }

            // Query by key, only one result
            le_result_t QueryByKey(T &entity) override
            {
                if (!IOHandler<T, K>::mpDao->HasKey(entity))
                {
                    LE_ERROR("Failed to query by key: Unknow key.");
                    return LE_BAD_PARAMETER;
                }

                std::stringstream sqlSs;
                std::string tableName = IOHandler<T, K>::mpDao->mFileName;
                std::vector<std::string> columns = IOHandler<T, K>::mpDao->GetColumnsName();
                std::vector<std::string> pk = IOHandler<T, K>::mpDao->GetPrimaryKeysName();

                sqlSs << "SELECT ";
                for (size_t i = 0; i < columns.size(); i++)
                {
                    sqlSs << columns[i];
                    if (i != (columns.size() - 1))
                    {
                        sqlSs << ", ";
                    }
                }

                sqlSs << " FROM " << tableName << " WHERE ";
                for (size_t i = 0; i < pk.size(); i++)
                {
                    sqlSs << pk[i] << "= ?";
                    if (i != (pk.size() - 1))
                    {
                        sqlSs << ", ";
                    }
                }

                std::string sql = sqlSs.str();
                LE_DEBUG("QueryByKey statement: %s", sql.c_str());
                printf("%s\n", sql.c_str());

                DataStatement statement(mDb.GetDbHandle(), sql.c_str());
                IOHandler<T, K>::mpDao->BindKeyValue(statement, entity);

                if (!statement.ExecuteRowStep())
                {
                    LE_WARN("Can not find a result, The key is not exist.");
                    return LE_NOT_FOUND;
                }

                IOHandler<T, K>::mpDao->ReadEntity(statement, entity);

                return LE_OK;
            }

            le_result_t ExecRaw(std::string &cmd) override
            {
                if (mDb.ExecSQL(cmd.c_str()) < 0)
                {
                    LE_ERROR("Failed to execute %s", cmd.c_str());
                    return LE_IO_ERROR;
                }

                return LE_OK;
            }

            // Get one or more row result by binding value and looping to execute ExecuteRowStep().
            // For example: where="WHERE id = ?"(Caller Shall bind value) or where="WHERE id=1".
            DataStatement Query() override
            {
                std::stringstream sqlSs;
                std::string tableName = IOHandler<T, K>::mpDao->mFileName;
                std::vector<std::string> columns = IOHandler<T, K>::mpDao->GetColumnsName();

                sqlSs << "SELECT ";
                for (size_t i = 0; i < columns.size(); i++)
                {
                    sqlSs << tableName << "." << columns[i];
                    if (i != (columns.size() - 1))
                    {
                        sqlSs << ", ";
                    }
                }

                sqlSs << " FROM " << tableName;

                std::string sql = sqlSs.str();
                LE_DEBUG("Query contents statement: %s", sql.c_str());

                return mDb.ExecQuery(sql.c_str());
            }

            DataStatement Query(std::string &where) override
            {
                std::stringstream sqlSs;
                std::string tableName = IOHandler<T, K>::mpDao->mFileName;
                std::vector<std::string> columns = IOHandler<T, K>::mpDao->GetColumnsName();

                sqlSs << "SELECT ";
                for (size_t i = 0; i < columns.size(); i++)
                {
                    sqlSs << tableName << "." << columns[i];
                    if (i != (columns.size() - 1))
                    {
                        sqlSs << ", ";
                    }
                }

                sqlSs << " FROM " << tableName << " " << where;

                std::string sql = sqlSs.str();
                LE_DEBUG("Query contents statement: %s", sql.c_str());

                return mDb.ExecQuery(sql.c_str());
            }

            DataStatement QueryCount(std::string &where) override
            {
                std::stringstream sqlSs;
                std::string tableName = IOHandler<T, K>::mpDao->mFileName;

                sqlSs << "SELECT count(*) ";
                sqlSs << " FROM " << tableName << " " << where;

                std::string sql = sqlSs.str();
                LE_DEBUG("QueryCount statement: %s", sql.c_str());

                return mDb.ExecQuery(sql.c_str());
            }

            bool CheckTableExist() override
            {
                std::string tableName = IOHandler<T, K>::mpDao->mFileName;

                std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";

                DataStatement statement(mDb.GetDbHandle(), sql.c_str());
                statement.ClearBindings();
                statement.BindValue(1, tableName.c_str());
                
                if (statement.ExecuteRowStep())
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        private:
            DataStatement *GetInsertStatement()
            {
                if (mpInsertStatement == nullptr)
                {
                    std::vector<std::string> columns = IOHandler<T, K>::mpDao->GetColumnsName();
                    std::string tableName = IOHandler<T, K>::mpDao->mFileName;
                    std::stringstream sqlSs;

                    sqlSs << "INSERT INTO " << tableName << " (";

                    for (size_t i = 0; i < columns.size(); i++)
                    {
                        sqlSs << columns[i];
                        if (i != (columns.size() - 1))
                        {
                            sqlSs << ", ";
                        }
                        else
                        {
                            sqlSs << ") VALUES (";
                        }
                    }

                    for (size_t i = 0; i < columns.size(); i++)
                    {
                        if (i != (columns.size() - 1))
                        {
                            sqlSs <<  "?, ";
                        }
                        else
                        {
                            sqlSs <<  "?)";
                        }
                    }

                    std::string sql = sqlSs.str();
                    LE_DEBUG("Insert statement: %s", sql.c_str());

                    mpInsertStatement.reset(new DataStatement(mDb.GetDbHandle(), sql.c_str()));
                }
                else
                {
                    mpInsertStatement->Reset();
                }

                return mpInsertStatement.get();
            }

            DataStatement *GetInsertOrReplaceStatement()
            {
                if (mpInsOrRepStatement == nullptr)
                {
                    std::vector<std::string> columns = IOHandler<T, K>::mpDao->GetColumnsName();
                    std::string tableName = IOHandler<T, K>::mpDao->mFileName;
                    std::stringstream sqlSs;

                    sqlSs << "INSERT OR REPLACE INTO " << tableName << " (";

                    for (size_t i = 0; i < columns.size(); i++)
                    {
                        sqlSs << columns[i];
                        if (i != (columns.size() - 1))
                        {
                            sqlSs << ", ";
                        }
                        else
                        {
                            sqlSs << ") VALUES (";
                        }
                    }

                    for (size_t i = 0; i < columns.size(); i++)
                    {
                        if (i != (columns.size() - 1))
                        {
                            sqlSs <<  "?, ";
                        }
                        else
                        {
                            sqlSs <<  "?)";
                        }
                    }

                    std::string sql = sqlSs.str();
                    LE_DEBUG("Insert or Replace statement: %s", sql.c_str());

                    mpInsOrRepStatement.reset(new DataStatement(mDb.GetDbHandle(), sql.c_str()));
                }
                else
                {
                    mpInsOrRepStatement->Reset();
                }

                return mpInsOrRepStatement.get();
            }

            DataStatement *GetUpdateStatement()
            {
                if (mpUpdateStatement == nullptr)
                {
                    std::vector<std::string> columns = IOHandler<T, K>::mpDao->GetColumnsName();
                    std::vector<std::string> pk = IOHandler<T, K>::mpDao->GetPrimaryKeysName();
                    std::string tableName = IOHandler<T, K>::mpDao->mFileName;
                    std::stringstream sqlSs;

                    sqlSs << "UPDATE " << tableName << " SET ";

                    for (size_t i = 0; i < columns.size(); i++)
                    {
                        sqlSs << columns[i] << " = ?";
                        if (i != (columns.size() - 1))
                        {
                            sqlSs << ", ";
                        }
                    }

                    sqlSs << " WHERE ";

                    for (size_t i = 0; i < pk.size(); i++)
                    {
                        sqlSs << tableName << "." << pk[i] << " = ?";
                        if (i != (pk.size() - 1))
                        {
                            sqlSs << ", ";
                        }
                    }

                    std::string sql = sqlSs.str();
                    LE_DEBUG("Update statement: %s", sql.c_str());

                    mpUpdateStatement.reset(new DataStatement(mDb.GetDbHandle(), sql.c_str()));
                }
                else
                {
                    mpUpdateStatement->Reset();
                }

                return mpUpdateStatement.get();
            }

            DataStatement *GetDeleteStatement()
            {
                if (mpDeleteStatement == nullptr)
                {
                    std::vector<std::string> pk = IOHandler<T, K>::mpDao->GetPrimaryKeysName();
                    std::string tableName = IOHandler<T, K>::mpDao->mFileName;
                    std::stringstream sqlSs;

                    sqlSs << "DELETE FROM " << tableName;

                    if (pk.size() != 0)
                    {
                        sqlSs << " WHERE ";
                        for (size_t i = 0; i < pk.size(); i++)
                        {
                            sqlSs << tableName << "." << pk[i] << " = ?";
                            if (i != (pk.size() - 1))
                            {
                                sqlSs << ", ";
                            }
                        }
                    }

                    std::string sql = sqlSs.str();
                    LE_DEBUG("Delete statement: %s", sql.c_str());

                    mpDeleteStatement.reset(new DataStatement(mDb.GetDbHandle(), sql.c_str()));
                }
                else
                {
                    mpDeleteStatement->Reset();
                }

                return mpDeleteStatement.get();
            }

            std::unique_ptr<DataStatement> mpInsertStatement;   // For insert.
            std::unique_ptr<DataStatement> mpInsOrRepStatement; // For insert or replace.
            std::unique_ptr<DataStatement> mpUpdateStatement;   // For update.
            std::unique_ptr<DataStatement> mpDeleteStatement;   // For delete.
            std::string mDbName;
            SQLite3DbUtil mDb;
    };
}
}
#endif
