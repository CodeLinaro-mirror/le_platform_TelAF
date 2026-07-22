/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include <ctime>
#include <sstream>
#include "tafBaseDAO.hpp"
#include "tafIOHandler.hpp"
#include "tafDIDEntityDAO.hpp"

using namespace taf::dataAccess;

DIDEntityDAO::~DIDEntityDAO
(
)
{
    if (mHandler != nullptr)
    {
        mHandler->DetachDao();
    }
}

DIDEntityDAO &DIDEntityDAO::GetInstance
(
)
{
    static DIDEntityDAO instance;

    return instance;
}

void DIDEntityDAO::Init
(
    const char *dbName,
    int expectedVer
)
{
    auto &factory = IOFactory::GetInstance();
    std::shared_ptr<IOHandler<DIDEntity, int32_t>> handler =
        factory.getHandler<DIDEntity, int32_t>(storageType, dbName);

    LE_DEBUG("IO handler is %p", handler.get());

    Init(DID_TABLE_NAME, handler);
    handler->AttachDao(this);

    if (handler->CheckTableExist())
    {
        LE_DEBUG("DID table exists.");
    }
    else
    {
        LE_DEBUG("DID table does not exist. Creating table.");
        CreateTable(handler, true);
    }

    pkList.push_back("DID");
    columnList.push_back("DID");
    columnList.push_back("DID_Value");
    columnList.push_back("Creation_Time");
    columnList.push_back("Update_Time");
}

le_result_t DIDEntityDAO::Load
(
)
{
    LE_DEBUG("DID table loaded.");
    return LE_OK;
}

le_result_t DIDEntityDAO::CreateTable
(
    std::shared_ptr<IOHandler<DIDEntity, int32_t>> handler,
    bool ifNotExists
)
{
    std::string constraint = ifNotExists ? "IF NOT EXISTS " : "";
    std::string sql = "CREATE TABLE " + constraint + DID_TABLE_NAME + " (" +
        "DID INTEGER PRIMARY KEY," +    // Row-0: did (primary key).
        "DID_Value BLOB," +             // Row-1: did value bytes.
        "Creation_Time TEXT," +         // Row-2: creation timestamp.
        "Update_Time TEXT)";            // Row-3: last update timestamp.

    le_result_t ret = handler->ExecRaw(sql);
    if (ret != LE_OK)
    {
        LE_ERROR("Create table(%s) failed.", DID_TABLE_NAME);
        return ret;
    }

    return LE_OK;
}

le_result_t DIDEntityDAO::DropTable
(
    std::shared_ptr<IOHandler<DIDEntity, int32_t>> handler,
    bool ifExists
)
{
    std::string constraint = ifExists ? "IF EXISTS " : "";
    std::string sql = "DROP TABLE " + constraint + DID_TABLE_NAME;

    return handler->ExecRaw(sql);
}

void DIDEntityDAO::BindValues
(
    DataStatement &statement,
    DIDEntity &entity
)
{
    char buf[DID_TIME_BUF_SIZE] = {0};
    std::tm *tmPtr;

    statement.ClearBindings();

    int32_t did = entity.GetDid();
    if (did != -1)
    {
        statement.BindValue(1, did);
    }

    int32_t len = entity.GetDIDValueLen();
    if (len > 0)
    {
        statement.BindValue(2, static_cast<const void *>(entity.GetDIDValue().data()), len);
    }

    std::time_t create = entity.GetCreateTime();
    if (create != 0)
    {
        tmPtr = std::localtime(&create);
        std::strftime(buf, DID_TIME_BUF_SIZE, "%Y-%m-%d %H:%M:%S", tmPtr);
        statement.BindValue(3, static_cast<const char *>(buf));
    }

    std::time_t update = entity.GetUpdateTime();
    if (update != 0)
    {
        tmPtr = std::localtime(&update);
        std::strftime(buf, DID_TIME_BUF_SIZE, "%Y-%m-%d %H:%M:%S", tmPtr);
        statement.BindValue(4, static_cast<const char *>(buf));
    }
}

void DIDEntityDAO::BindKeyValue
(
    DataStatement &statement,
    DIDEntity &entity
)
{
    statement.ClearBindings();

    int32_t did = entity.GetDid();
    if (did != -1)
    {
        statement.BindValue(1, did);
    }
}

void DIDEntityDAO::BindKeyValue
(
    DataStatement &statement,
    int index,
    int32_t key
)
{
    statement.BindValue(index, key);
}

void DIDEntityDAO::ReadEntity
(
    DataStatement &statement,
    DIDEntity &entity
)
{
    entity.SetDid(statement.GetColumnInt(0));

    const void *blobPtr = statement.GetColumnBlob(1);
    int blobLen = statement.GetColumnBytes(1);
    if (blobPtr != nullptr && blobLen > 0)
    {
        entity.SetDIDValue(static_cast<const uint8_t *>(blobPtr),
                           static_cast<int32_t>(blobLen));
    }

    auto createTimePtr = reinterpret_cast<const char *>(statement.GetColumnText(2, nullptr));
    if (createTimePtr == nullptr)
    {
        entity.SetCreateTime(0);
    }
    else
    {
        entity.SetCreateTime(String2Time(createTimePtr));
    }

    auto updateTimePtr = reinterpret_cast<const char *>(statement.GetColumnText(3, nullptr));
    if (updateTimePtr == nullptr)
    {
        entity.SetUpdateTime(0);
    }
    else
    {
        entity.SetUpdateTime(String2Time(updateTimePtr));
    }
}

int32_t DIDEntityDAO::ReadKey
(
    DataStatement &statement
)
{
    return statement.GetColumnInt(0);
}

int32_t DIDEntityDAO::GetKey
(
    DIDEntity &entity
)
{
    return entity.GetDid();
}

bool DIDEntityDAO::HasKey
(
    DIDEntity &entity
)
{
    return (entity.GetDid() == -1) ? false : true;
}

std::vector<std::string> DIDEntityDAO::GetColumnsName
(
)
{
    return columnList;
}

std::vector<std::string> DIDEntityDAO::GetPrimaryKeysName
(
)
{
    return pkList;
}

le_result_t DIDEntityDAO::WriteDID
(
    uint16_t did,
    const uint8_t *value,
    size_t len
)
{
    le_result_t ret;
    DIDEntity entity;

    entity.SetDid(static_cast<int32_t>(did));
    ret = QueryByKey(entity);
    if (ret != LE_OK)
    {
        // Insert new record.
        entity.SetDid(static_cast<int32_t>(did));
        entity.SetDIDValue(value, static_cast<int32_t>(len));
        std::time_t now = std::time(nullptr);
        entity.SetCreateTime(now);
        entity.SetUpdateTime(now);

        ret = Add(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to insert DID 0x%x. ret=%d", did, (int32_t)ret);
            return ret;
        }
    }
    else
    {
        // Update existing record.
        entity.SetDIDValue(value, static_cast<int32_t>(len));
        std::time_t now = std::time(nullptr);
        entity.SetUpdateTime(now);

        ret = Update(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to update DID 0x%x. ret=%d", did, (int32_t)ret);
            return ret;
        }
    }

    return LE_OK;
}

le_result_t DIDEntityDAO::ReadDID
(
    uint16_t did,
    uint8_t *value,
    size_t *len
)
{
    le_result_t ret;
    DIDEntity entity;

    entity.SetDid(static_cast<int32_t>(did));
    ret = QueryByKey(entity);
    if (ret != LE_OK)
    {
        LE_ERROR("DID 0x%x not found. ret=%d", did, (int32_t)ret);
        *len = 0;
        return ret;
    }

    const std::vector<uint8_t> &data = entity.GetDIDValue();
    if (*len < data.size())
    {
        LE_ERROR("Buffer too small for DID 0x%x. Need %zu, got %zu", did, data.size(), *len);
        *len = data.size();
        return LE_OVERFLOW;
    }

    memcpy(value, data.data(), data.size());
    *len = data.size();

    return LE_OK;
}

bool DIDEntityDAO::RemoveDID
(
    uint16_t did
)
{
    DIDEntity entity;
    entity.SetDid(static_cast<int32_t>(did));

    le_result_t ret = Remove(entity);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to remove DID 0x%x. ret=%d", did, (int32_t)ret);
        return false;
    }

    return true;
}

std::vector<DIDEntity> DIDEntityDAO::FindAll
(
)
{
    std::vector<DIDEntity> entities;
    DataStatement statement(Query());

    while (statement.ExecuteRowStep())
    {
        DIDEntity entity;
        ReadEntity(statement, entity);
        entities.push_back(entity);
    }

    return entities;
}

bool DIDEntityDAO::Clear
(
)
{
    le_result_t ret = DropTable(mHandler, true);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to drop DID table.");
        return false;
    }

    ret = CreateTable(mHandler, false);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to recreate DID table.");
        return false;
    }

    return true;
}
