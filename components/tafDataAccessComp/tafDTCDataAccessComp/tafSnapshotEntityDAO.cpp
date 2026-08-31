/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <ctime>
#include <string.h>
#include "legato.h"
#include "interfaces.h"

#include "tafBaseDAO.hpp"
#include "tafIOHandler.hpp"
#include "tafSnapshotEntityDAO.hpp"

using namespace taf::dataAccess;

SnapshotEntityDao::SnapshotEntityDao
(
)
{
}

SnapshotEntityDao::~SnapshotEntityDao
(
)
{
    if (mHandler != nullptr)
    {
        mHandler->DetachDao();
    }
}

SnapshotEntityDao &SnapshotEntityDao::GetInstance
(
)
{
    static SnapshotEntityDao instance;

    return instance;
}

void SnapshotEntityDao::Init
(
    const char *dbName
)
{
    auto &factory = IOFactory::GetInstance();
    std::shared_ptr<IOHandler<SnapshotEntity, int32_t>> handler =
        factory.getHandler<SnapshotEntity, int32_t>(storageType, dbName);

    LE_DEBUG("IO handler is %p", handler.get());

    Init(SNAPSHOT_TABLE_NAME, handler);
    handler->AttachDao(this);

    dtcRecordPool = le_mem_CreatePool("dtcRecordPool", sizeof(taf_DataAccess_SnapshotInfo_t));

    CreateTable(handler, true);

    // Store the column name information.
    pkList.push_back("ID");
    columnList.push_back("ID");
    columnList.push_back("DTC");
    columnList.push_back("DID");
    columnList.push_back("DID_Value");
    columnList.push_back("Record_Number");
    columnList.push_back("Creation_Time");
    columnList.push_back("Update_Time");
}

le_result_t SnapshotEntityDao::CreateTable
(
    std::shared_ptr<IOHandler<SnapshotEntity, int32_t>> handler,
    bool ifNotExists
)
{
    std::string constraint = ifNotExists ? "IF NOT EXISTS " : "";
    std::string sql = "CREATE TABLE " + constraint + SNAPSHOT_TABLE_NAME + " (" +
        "ID INTEGER PRIMARY KEY," +         // Row-0: ID.
        "DTC INTEGER," +                    // Row-1: DTC.
        "DID INTEGER," +                    // Row-2: DID.
        "DID_Value BLOB," +                 // Row-3: DID Value.
        "Record_Number INTEGER," +          // Row-4: Record Number.
        "Creation_Time TEXT," +             // Row-5: Creation Time.
        "Update_Time TEXT)";                // Row-6: Update Time.

    le_result_t ret = handler->ExecRaw(sql);
    if (ret != LE_OK)
    {
        LE_ERROR("Create table(%s) failed.", SNAPSHOT_TABLE_NAME);
        return ret;
    }

    return LE_OK;
}

le_result_t SnapshotEntityDao::DropTable
(
    std::shared_ptr<IOHandler<SnapshotEntity, int32_t>> handler,
    bool ifExists
)
{
    std::string constraint = ifExists ? "IF EXISTS " : "";
    std::string sql = "DROP TABLE " + constraint + SNAPSHOT_TABLE_NAME;

    return handler->ExecRaw(sql);
}

void SnapshotEntityDao::BindValues
(
    DataStatement &statement,
    SnapshotEntity &entity
)
{
    std::tm *tmPtr;
    char buf[FF_TIME_BUF_SIZE] = {0};

    // Clear all binding parameters on the statement.
    statement.ClearBindings();

    int32_t id = entity.GetId();
    if (id != -1)
    {
        statement.BindValue(1, id);
    }

    int32_t dtc = entity.GetDtc();
    if (dtc != -1)
    {
        statement.BindValue(2, dtc);
    }

    int32_t did = entity.GetDid();
    if (did != -1)
    {
        statement.BindValue(3, did);
    }

    uint8_t didVal[DATA_ACCESS_DID_DATA_SIZE_MAX];
    int32_t didValSize = DATA_ACCESS_DID_DATA_SIZE_MAX;
    entity.GetDidValue(didVal, didValSize);
    if (didValSize > 0)
    {
        statement.BindValue(4, didVal, didValSize);
    }

    int32_t srn = entity.GetRecordNum();
    if (srn != -1)
    {
        statement.BindValue(5, srn);
    }

    std::time_t create = entity.GetCreateTime();
    if (create != 0)
    {
        tmPtr = std::localtime(&create);
        std::strftime(buf, FF_TIME_BUF_SIZE, "%Y-%m-%d %H:%M:%S", tmPtr);

        statement.BindValue(6, static_cast<const char *>(buf));
    }

    std::time_t update = entity.GetUpdateTime();
    if (update != 0)
    {

        tmPtr = std::localtime(&update);
        std::strftime(buf, FF_TIME_BUF_SIZE, "%Y-%m-%d %H:%M:%S", tmPtr);

        statement.BindValue(7, static_cast<const char *>(buf));
    }
}

void SnapshotEntityDao::BindKeyValue
(
    DataStatement &statement,
    SnapshotEntity &entity
)
{
    // Clear all binding parameters on the statement.
    statement.ClearBindings();

    int32_t id = entity.GetId();
    if (id != -1)
    {
        statement.BindValue(1, id);
    }
}

void SnapshotEntityDao::BindKeyValue
(
    DataStatement &statement,
    int index,
    int32_t key
)
{
    statement.BindValue(index, key);
}

void SnapshotEntityDao::ReadEntity
(
    DataStatement &statement,
    SnapshotEntity &entity
)
{
    entity.SetId(statement.GetColumnInt(0));
    entity.SetDtc(statement.GetColumnInt(1));
    entity.SetDid(statement.GetColumnInt(2));
    entity.SetDidValue(static_cast<const uint8_t*>(statement.GetColumnBlob(3)),
        static_cast<int32_t>(statement.GetColumnBytes(3)));
    entity.SetRecordNum(statement.GetColumnInt(4));

    auto createTimePtr = reinterpret_cast<const char*>(statement.GetColumnText(5, nullptr));
    if (createTimePtr == nullptr)
    {
        entity.SetCreateTime(0);
    }
    else
    {
        entity.SetCreateTime(String2Time(createTimePtr));
    }

    auto updateTimePtr = reinterpret_cast<const char*>(statement.GetColumnText(6, nullptr));
    if (updateTimePtr == nullptr)
    {
        entity.SetUpdateTime(0);
    }
    else
    {
        entity.SetUpdateTime(String2Time(updateTimePtr));
    }
}

int32_t SnapshotEntityDao::ReadKey
(
    DataStatement &statement
)
{
    return statement.GetColumnInt(0);
}

int32_t SnapshotEntityDao::GetKey
(
    SnapshotEntity &entity
)
{
    return entity.GetId();
}

bool SnapshotEntityDao::HasKey
(
    SnapshotEntity &entity
)
{
    return (entity.GetId() == -1) ? false : true;
}

std::vector<std::string> SnapshotEntityDao::GetColumnsName
(
)
{
    return columnList;
}

std::vector<std::string> SnapshotEntityDao::GetPrimaryKeysName
(
)
{
    return pkList;
}

le_result_t SnapshotEntityDao::ClearFFRecord
(
)
{
    le_result_t ret;
    DropTable(mHandler, false);
    ret = CreateTable(mHandler, false);
    if (ret != LE_OK)
    {
        // If drop table failure, here will return NOK
        LE_ERROR("Failed to clear snapshot table. ret=%d", (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

le_result_t SnapshotEntityDao::ClearFFRecord
(
    int32_t id
)
{
    le_result_t ret;
    SnapshotEntity entity;

    entity.SetId(id);
    ret = Remove(entity);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to delete id0x%x record from snapshot table. ret=%d",
            id, (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

le_result_t SnapshotEntityDao::ClearFFRecordByDtc
(
    int32_t dtc
)
{
    le_result_t ret;
    SnapshotEntity entity;
    std::stringstream ss;

    ss << "WHERE " << mFileName << "." << "DTC=" << "'" << dtc << "'";

    std::string where = ss.str();
    LE_DEBUG("ClearFFRecordByDtc: where is %s", where.c_str());

    DataStatement statement(Query(where));
    while (statement.ExecuteRowStep())
    {
        ReadEntity(statement, entity);
        LE_DEBUG("Get id=%d in snapshot table.", entity.GetId());
        ret = Remove(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to delete dtc0x%x record from event table. ret=%d",
                dtc, (int32_t)ret);
            return ret;
        }
    }

    return LE_OK;
}

int32_t SnapshotEntityDao::ReadDIDCount
(
    int32_t dtc,
    int32_t recordNum
)
{
    std::stringstream ss;

    ss << "WHERE " << mFileName << "." << "DTC=" << dtc << " AND " << "Record_Number=" << recordNum;

    std::string where = ss.str();
    LE_DEBUG("ReadDIDCount: where is %s", where.c_str());

    DataStatement statement(QueryCount(where));
    (void)statement.ExecuteRowStep();  // Cannot return false as this query always return a result.

    return statement.GetColumnInt(0);
}

std::vector<DIDInfoPtr> SnapshotEntityDao::GetDIDRecord
(
    int32_t dtc,
    int32_t recordNum
)
{
    std::vector<DIDInfoPtr> dids;
    std::stringstream ss;

    ss << "WHERE " << mFileName << "." << "DTC=" << dtc << " AND " << "Record_Number=" << recordNum;

    std::string where = ss.str();
    LE_DEBUG("GetDIDRecord: where is %s", where.c_str());

    DataStatement statement(Query(where));
    while (statement.ExecuteRowStep())
    {
        LE_DEBUG("Execute step get a result for DID");
        DIDInfoPtr didPtr = std::make_shared<taf_DataAccess_DIDInfo_t>(
            static_cast<uint16_t>(statement.GetColumnInt(2)),
            static_cast<const uint8_t*>(statement.GetColumnBlob(3)),
            static_cast<uint16_t>(statement.GetColumnBytes(3)));

        dids.push_back(didPtr);
    }

    return dids;
}

le_result_t SnapshotEntityDao::SetDIDRecord
(
    int32_t dtc,
    int32_t recordNum,
    std::vector<DIDInfoPtr> dids
)
{
    le_result_t ret;
    SnapshotEntity entity;

    // USE "INSERT or REPLACE" command to optimize this implementation.
    for (const DIDInfoPtr& didPtr : dids)
    {
        std::stringstream ss;

        ss << "WHERE " << "DTC=" << dtc << " AND ";
        ss << "Record_Number=" << recordNum << " AND ";
        ss << "DID=" << didPtr->did << ";";

        std::string where = ss.str();
        LE_DEBUG("SetDIDRecord: where is %s", where.c_str());

        DataStatement statement(Query(where));
        if (!statement.ExecuteRowStep())
        {
            // Insert data into the table.
            entity.SetDtc(dtc);
            entity.SetDid(didPtr->did);
            entity.SetDidValue(didPtr->val, didPtr->len);
            entity.SetRecordNum(recordNum);
            std::time_t now = std::time(nullptr);
            entity.SetCreateTime(now);
            entity.SetUpdateTime(now);
            ret = Add(entity);
            if (ret != LE_OK)
            {
                LE_ERROR("Failed to insert dtc0x%x rn0x%x. ret=%d",
                    dtc, recordNum, (int32_t)ret);
                return ret;
            }
        }
        else
        {
            // Update data.
            ReadEntity(statement, entity);
            entity.SetDtc(dtc);
            entity.SetDid(didPtr->did);
            entity.SetDidValue(didPtr->val, didPtr->len);
            entity.SetRecordNum(recordNum);
            std::time_t now = std::time(nullptr);
            entity.SetUpdateTime(now);

            ret = Update(entity);
            if (ret != LE_OK)
            {
                LE_ERROR("Failed to update dtc0x%x rn0x%x. ret=%d",
                    dtc, recordNum, (int32_t)ret);
                return ret;
            }

            while (statement.ExecuteRowStep())
            {
                // Get the repetitive line in the db, shall update too.
                LE_DEBUG("Get a repetitive result. dtc=0x%x, did=0x%x, rn=%d",
                    dtc, didPtr->did, recordNum);
                Update(entity);
            }
        }
    }

    return LE_OK;
}

void SnapshotEntityDao::GetAllDTCRecord
(
    le_dls_List_t *list
)
{
    int32_t preDtc = -1, preRn, curDtc, curRn;
    taf_DataAccess_SnapshotInfo_t *infoPtr;
    DataStatement statement(Query());
    while (statement.ExecuteRowStep())
    {
        LE_DEBUG("Execute step get a result for DID");

        curDtc = statement.GetColumnInt(1);
        curRn = statement.GetColumnInt(4);
        if (preDtc == -1)
        {
            preDtc = curDtc;
            preRn = curRn;

            infoPtr = (taf_DataAccess_SnapshotInfo_t*)le_mem_ForceAlloc(dtcRecordPool);

            infoPtr->dtc = static_cast<uint32_t>(curDtc);
            infoPtr->recNumber = static_cast<uint8_t>(curRn);
            infoPtr->link = LE_DLS_LINK_INIT;
            le_dls_Queue(list, &infoPtr->link);
        }
        else
        {
            if ((preDtc == curDtc) && (preRn == curRn))
            {
                // Skip the same DTC & Record Number.
                continue;
            }

            preDtc = curDtc;
            preRn = curRn;

            infoPtr = (taf_DataAccess_SnapshotInfo_t*)le_mem_ForceAlloc(dtcRecordPool);

            infoPtr->dtc = static_cast<uint32_t>(curDtc);
            infoPtr->recNumber = static_cast<uint8_t>(curRn);
            infoPtr->link = LE_DLS_LINK_INIT;
            le_dls_Queue(list, &infoPtr->link);
        }
    }
}