/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <ctime>
#include <string>

#include "legato.h"
#include "interfaces.h"

#include "tafBaseDAO.hpp"
#include "tafIOHandler.hpp"
#include "tafEventEntityDAO.hpp"
#include "configuration.hpp"

using namespace tafsvc;
using namespace taf::dataAccess;

EventEntityDao::EventEntityDao
(
)
{
}

EventEntityDao::~EventEntityDao
(
)
{
    if (mHandler != nullptr)
    {
        mHandler->DetachDao();
    }
}

EventEntityDao &EventEntityDao::GetInstance
(
)
{
    static EventEntityDao instance;

    return instance;
}

void EventEntityDao::Init
(
    const char *dbName,
    int expectedVer
)
{
    auto &factory = IOFactory::GetInstance();
    std::shared_ptr<IOHandler<EventEntity, int32_t>> handler =
        factory.getHandler<EventEntity, int32_t>(storageType, dbName);

    LE_DEBUG("IO handler is %p", handler.get());

    Init(EVENT_TABLE_NAME, handler);
    handler->AttachDao(this);

    if (handler->CheckTableExist())
    {
        LE_DEBUG("Table exist. check if need upgrade");
        int currentVer = 0;

        handler->GetVersion(currentVer);
        UpdateTable(handler, currentVer, expectedVer);
    }
    else
    {
        LE_DEBUG("Event table does not exist. creat!");
        CreateTable(handler, true);
    }

    // Store the column name information.
    pkList.push_back("Event_ID");
    columnList.push_back("Event_ID");
    if (expectedVer >= 2)
    {
        columnList.push_back("Event_Name");  // Added in version2
    }
    columnList.push_back("DTC");
    columnList.push_back("Status");
    columnList.push_back("Test_Failed_Counter");
    columnList.push_back("Creation_Time");
    columnList.push_back("Update_Time");
}

le_result_t EventEntityDao::Load
(
)
{
    // Load all events into the table.
    if (mHandler->CheckTableEmpty())
    {
        LE_DEBUG("Event table is empty. loading all event name into it");
        return InitTableWithConfig();
    }
    else
    {
        LE_DEBUG("Event table is not empty.");
    }

    return LE_OK;
}

le_result_t EventEntityDao::CreateTable
(
    std::shared_ptr<IOHandler<EventEntity, int32_t>> handler,
    bool ifNotExists
)
{
    std::string constraint = ifNotExists ? "IF NOT EXISTS " : "";
    std::string sql = "CREATE TABLE " + constraint + EVENT_TABLE_NAME + " (" +
        "Event_ID INTEGER PRIMARY KEY," +       // Row-0: Event ID.
        "Event_Name TEXT," +                    // Row-1: Event name.
        "DTC INTEGER," +                        // Row-2: DTC.
        "Status INTEGER," +                     // Row-3: Event status.
        "Test_Failed_Counter INTEGER," +        // Row-4: Test failed counter.
        "Creation_Time TEXT," +                 // Row-5: Creation Time.
        "Update_Time TEXT)";                    // Row-6: Update Time.

    le_result_t ret = handler->ExecRaw(sql);
    if (ret != LE_OK)
    {
        LE_ERROR("Create table(%s) failed.", EVENT_TABLE_NAME);
        return ret;
    }

    return LE_OK;
}

le_result_t EventEntityDao::DropTable
(
    std::shared_ptr<IOHandler<EventEntity, int32_t>> handler,
    bool ifExists
)
{
    std::string constraint = ifExists ? "IF EXISTS " : "";
    std::string sql = "DROP TABLE " + constraint + EVENT_TABLE_NAME;

    return handler->ExecRaw(sql);
}

le_result_t EventEntityDao::UpdateTable
(
    std::shared_ptr<IOHandler<EventEntity, int32_t>> handler,
    int currentVer,
    int expectedVer
)
{
    std::string createSql = std::string("CREATE TABLE ") + EVENT_TABLE_NAME + " (" +
        "Event_ID INTEGER PRIMARY KEY AUTOINCREMENT," + // Row-0: Event ID.
        "Event_Name TEXT," +                            // Row-1: Event name.
        "DTC INTEGER," +                                // Row-2: DTC.
        "Status INTEGER," +                             // Row-3: Event status.
        "Test_Failed_Counter INTEGER," +                // Row-4: Test failed counter.
        "Creation_Time TEXT," +                         // Row-5: Creation Time.
        "Update_Time TEXT);";                           // Row-6: Update Time.

    char buf[EVENT_TIME_BUF_SIZE] = {0};
    std::time_t now = std::time(nullptr);
    std::tm *tmPtr = std::localtime(&now);
    std::strftime(buf, EVENT_TIME_BUF_SIZE, "%Y-%m-%d %H:%M:%S", tmPtr);
    LE_INFO("Time: %s", buf);

    std::string backupForVer2Sql = std::string("INSERT INTO ") + EVENT_TABLE_NAME + 
        "(Event_Name, DTC, Status, Test_Failed_Counter, Creation_Time, Update_Time) VALUES ";

    try
    {
        std::vector<std::pair<uint16_t, std::string>> idNamePairs = cfg::get_event_idNames();
        for (size_t i = 0; i < idNamePairs.size(); ++i)
        {
            backupForVer2Sql += "('" + idNamePairs[i].second + "'," +
                "COALESCE((SELECT DTC FROM _temp_" + EVENT_TABLE_NAME + " WHERE Event_ID=" +
                    std::to_string(idNamePairs[i].first) + "), 0)," +
                "COALESCE((SELECT Status FROM _temp_" + EVENT_TABLE_NAME + " WHERE Event_ID=" +
                    std::to_string(idNamePairs[i].first) + "), 0)," +
                "COALESCE((SELECT Test_Failed_Counter FROM _temp_" + EVENT_TABLE_NAME +
                    " WHERE Event_ID=" + std::to_string(idNamePairs[i].first) + "), 0)," +
                "COALESCE((SELECT Creation_Time FROM _temp_" + EVENT_TABLE_NAME +
                    " WHERE Event_ID=" + std::to_string(idNamePairs[i].first) + "), '" +
                    std::string(buf) + "')," +
                "COALESCE((SELECT Update_Time FROM _temp_" + EVENT_TABLE_NAME + " WHERE Event_ID=" +
                    std::to_string(idNamePairs[i].first) + "), '" + std::string(buf) + "'))";
            if (i < (idNamePairs.size() - 1))
            {
                backupForVer2Sql += ",";
            }
        }
        backupForVer2Sql += ";";
    }
    catch (const std::exception& e)
    {
        // Not events
        LE_WARN("Exception: %s", e.what() );
        return LE_FAULT;
    }

    std::string upgrade[] = {
        // Version0
        "",

        // Version1
        "",

        // Version2
        std::string("BEGIN TRANSACTION;") +
        "ALTER TABLE " + EVENT_TABLE_NAME + " RENAME TO _temp_" + EVENT_TABLE_NAME + ";" +
        createSql +
        backupForVer2Sql +
        "DROP TABLE _temp_" + EVENT_TABLE_NAME + ";" +
        "END TRANSACTION;"
    };

    if (currentVer < expectedVer)
    {
        // Upgrade
        int num = sizeof(upgrade) / sizeof(upgrade[0]);

        for (int i = currentVer + 1; i <= expectedVer && i < num; i++)
        {
            if (upgrade[i].empty())
            {
                continue;
            }

            LE_DEBUG("Upgrade: %s", upgrade[i].c_str());
            le_result_t ret = handler->ExecRaw(upgrade[i]);
            if (ret != LE_OK)
            {
                LE_FATAL("Update event table(%s) failed.", EVENT_TABLE_NAME);
                return ret;
            }
        }
        return LE_OK;
    }
    else if (expectedVer < currentVer)
    {
        // Downgrade
        return LE_UNSUPPORTED;
    }
    else
    {
        return LE_OK;
    }
}

le_result_t EventEntityDao::InitTableWithConfig
(
)
{
    le_result_t ret;

    try
    {
        std::vector<std::pair<uint16_t, std::string>> idNamePairs = cfg::get_event_idNames();
        for (const auto & idNamePair: idNamePairs)
        {
            // Insert data into the table.
            EventEntity entity;
            entity.SetEventName(idNamePair.second.c_str());
            entity.SetEventDtc(0);
            entity.SetEventStatus(0);
            entity.SetTestFailedCounter(0);
            std::time_t now = std::time(nullptr);
            entity.SetCreateTime(now);
            entity.SetUpdateTime(now);
            ret = Add(entity);
            if (ret != LE_OK)
            {
                LE_ERROR("Failed to insert event name(%s). ret=%d",
                    idNamePair.second.c_str(), (int32_t)ret);
                return ret;
            }
        }
    }
    catch (const std::exception& e)
    {
        // No event name
        LE_WARN("Exception: %s", e.what() );
        return LE_FAULT;
    }

    return LE_OK;
}

void EventEntityDao::BindValues
(
    DataStatement &statement,
    EventEntity &entity
)
{
    std::tm *tmPtr;
    char buf[EVENT_TIME_BUF_SIZE] = {0};

    // Clear all binding parameters on the statement.
    statement.ClearBindings();

    int32_t id = entity.GetEventId();
    if (id != -1)
    {
        statement.BindValue(1, id);
    }

    const char *name = entity.GetEventName();
    if (name != nullptr)
    {
        statement.BindValue(2, name);
    }

    int32_t dtc = entity.GetEventDtc();
    if (dtc != -1)
    {
        statement.BindValue(3, dtc);
    }

    int32_t status = entity.GetEventStatus();
    if (status != -1)
    {
        statement.BindValue(4, status);
    }

    int32_t counter = entity.GetTestFailedCounter();
    if (counter != -1)
    {
        statement.BindValue(5, counter);
    }

    std::time_t create = entity.GetCreateTime();
    if (create != 0)
    {
        tmPtr = std::localtime(&create);
        std::strftime(buf, EVENT_TIME_BUF_SIZE, "%Y-%m-%d %H:%M:%S", tmPtr);

        statement.BindValue(6, static_cast<const char *>(buf));
    }

    std::time_t update = entity.GetUpdateTime();
    if (update != 0)
    {

        tmPtr = std::localtime(&update);
        std::strftime(buf, EVENT_TIME_BUF_SIZE, "%Y-%m-%d %H:%M:%S", tmPtr);

        statement.BindValue(7, static_cast<const char *>(buf));
    }
}

void EventEntityDao::BindKeyValue
(
    DataStatement &statement,
    EventEntity &entity
)
{
    // Clear all binding parameters on the statement.
    statement.ClearBindings();

    int32_t id = entity.GetEventId();
    if (id != -1)
    {
        statement.BindValue(1, id);
    }
}

void EventEntityDao::BindKeyValue
(
    DataStatement &statement,
    int index,
    int32_t key
)
{
    statement.BindValue(index, key);
}

void EventEntityDao::ReadEntity
(
    DataStatement &statement,
    EventEntity &entity
)
{
    entity.SetEventId(statement.GetColumnInt(0));
    entity.SetEventName(statement.GetColumnText(1));
    entity.SetEventDtc(statement.GetColumnInt(2));
    entity.SetEventStatus(statement.GetColumnInt(3));
    entity.SetTestFailedCounter(statement.GetColumnInt(4));

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

int32_t EventEntityDao::ReadKey
(
    DataStatement &statement
)
{
    return statement.GetColumnInt(0);
}

int32_t EventEntityDao::GetKey
(
    EventEntity &entity
)
{
    return entity.GetEventId();
}

bool EventEntityDao::HasKey(EventEntity &entity)
{
    return (entity.GetEventId() == -1) ? false : true;
}

std::vector<std::string> EventEntityDao::GetColumnsName
(
)
{
    return columnList;
}

std::vector<std::string> EventEntityDao::GetPrimaryKeysName
(
)
{
    return pkList;
}

int32_t EventEntityDao::ReadEventCountByEventId
(
    int32_t eventId
)
{
    std::stringstream ss;

    ss << "WHERE " << mFileName << "." << "Event_ID=" << "'" << eventId << "'";

    std::string where = ss.str();
    LE_DEBUG("ReadEventCountByEventId: where is %s", where.c_str());

    DataStatement statement(QueryCount(where));
    (void)statement.ExecuteRowStep();  // Cannot return false as this query always return a result.

    return statement.GetColumnInt(0);
}

int32_t EventEntityDao::ReadEventCountByDtc
(
    int32_t dtc
)
{
    std::stringstream ss;

    ss << "WHERE " << mFileName << "." << "DTC=" << "'" << dtc << "'";

    std::string where = ss.str();
    LE_DEBUG("ReadEventCountByDtc: where is %s", where.c_str());

    DataStatement statement(QueryCount(where));
    (void)statement.ExecuteRowStep();  // Cannot return false as this query always return a result.

    return statement.GetColumnInt(0);
}

int32_t EventEntityDao::ReadEventCountByStatus
(
    int32_t status
)
{
    std::stringstream ss;

    ss << "WHERE " << mFileName << "." << "Status=" << "'" << status << "'";

    std::string where = ss.str();
    LE_DEBUG("ReadEventCountByStatus: where is %s", where.c_str());

    DataStatement statement(QueryCount(where));
    (void)statement.ExecuteRowStep();  // Cannot return false as this query always return a result.

    return statement.GetColumnInt(0);
}

int32_t EventEntityDao::ReadEventStatusByEventId
(
    int32_t eventId
)
{
    int ret;
    EventEntity entity;

    entity.SetEventId(eventId);
    ret = QueryByKey(entity);
    if (ret != LE_OK)
    {
        if (ret == LE_NOT_FOUND)
        {
            LE_WARN("The key is not exist. ");
        }
        else
        {
            LE_ERROR("Failed to query entity by key. ret=%d", ret);
        }

        return 0;
    }

    LE_DEBUG("ReadEventStatusByEventId: Get status(0x%x) for event0x%x",
        entity.GetEventStatus(), eventId);

    return entity.GetEventStatus();
}

le_result_t EventEntityDao::ReadEventInfoByEventId
(
    int32_t eventId,
    taf_DataAccess_EventInfo_t *eventInfoPtr
)
{
    le_result_t ret;
    EventEntity entity;

    entity.SetEventId(eventId);
    ret = QueryByKey(entity);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to query entity by key. ret=%d", (int32_t)ret);
        return ret;
    }

    LE_DEBUG("ReadEventInfoByEventId: Get status(0x%x) and DTC0x%x for event0x%x",
        entity.GetEventStatus(), entity.GetEventDtc(), eventId);

    eventInfoPtr->eventId = entity.GetEventId();
    eventInfoPtr->dtc = entity.GetEventDtc();
    eventInfoPtr->status = entity.GetEventStatus();
    eventInfoPtr->link = LE_DLS_LINK_INIT;

    return LE_OK;
}

int32_t EventEntityDao::ReadEventStatusByName
(
    const char *eventName
)
{
    std::stringstream ss;
    EventEntity entity;
    uint32_t status = 0;

    if (eventName == nullptr)
    {
        LE_DEBUG("Parameter error: event name is null");
        return 0;
    }

    ss << "WHERE " << mFileName << "." << "Event_Name = '" << eventName << "'";

    std::string where = ss.str();
    LE_DEBUG("ReadEventStatusByName: where is %s", where.c_str());

    DataStatement statement(Query(where));
    while (statement.ExecuteRowStep())
    {
        LE_DEBUG("Execute step get a result");

        LE_DEBUG("event id is 0x%x, event name is %s, status is 0x%x",
            statement.GetColumnInt(0),
            statement.GetColumnText(1),
            statement.GetColumnInt(3));
        status |= static_cast<uint32_t>(statement.GetColumnInt(3));
    }

    LE_DEBUG("ReadEventStatusByName: Get status(0x%x) for event name(%s)",
        status, eventName);

    return static_cast<int32_t>(status);
}

int32_t EventEntityDao::ReadFailedCounterByEventId
(
    int32_t eventId
)
{
    int ret;
    EventEntity entity;

    entity.SetEventId(eventId);
    ret = QueryByKey(entity);
    if (ret != LE_OK)
    {
        if (ret == LE_NOT_FOUND)
        {
            LE_WARN("The key is not exist. ");
        }
        else
        {
            LE_ERROR("Failed to query entity by key. ret=%d", ret);
        }

        return 0;
    }

    LE_DEBUG("ReadFailedCounterByEventId: Get failed counter(0x%x) for event0x%x",
        entity.GetTestFailedCounter(), eventId);

    return entity.GetTestFailedCounter();
}

le_result_t EventEntityDao::WriteStatusAndDtcByEventId
(
    int32_t eventId,
    int32_t status,
    int32_t dtc
)
{
    le_result_t ret;
    EventEntity entity;

    entity.SetEventId(eventId);
    ret = QueryByKey(entity);
    if (ret == LE_NOT_FOUND)  // Not record
    {
        // Insert.
        entity.SetEventId(eventId);
        entity.SetEventName("");
        entity.SetEventDtc(dtc);
        entity.SetEventStatus(status);
        entity.SetTestFailedCounter(0);

        std::time_t now = std::time(nullptr);
        entity.SetCreateTime(now);
        entity.SetUpdateTime(now);

        ret = Add(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to insert event0x%x dtc0x%x with status0x%x. ret=%d",
                eventId, dtc, status, (int32_t)ret);
            return ret;
        }
    }
    else if (ret == LE_OK)
    {
        // Update.
        if (entity.GetEventDtc() != dtc)
        {
            LE_ERROR("Failed to update status0x%x for event0x%x. dtc no match(0x%x-0x%x)",
                status, eventId, dtc, entity.GetEventDtc());
            return LE_UNSUPPORTED;
        }

        entity.SetEventStatus(status);

        std::time_t now = std::time(nullptr);
        entity.SetUpdateTime(now);

        ret = Update(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to update event0x%x dtc0x%x with status0x%x. ret=%d",
                eventId, dtc, status, (int32_t)ret);
            return ret;
        }
    }
    else
    {
        LE_ERROR("Failed to query by event id0x%x. ret=%d", eventId, (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

le_result_t EventEntityDao::WriteStatusAndDtcByEventName
(
    const char * eventName,
    int32_t status,
    int32_t dtc
)
{
    le_result_t ret;
    std::stringstream ss;
    EventEntity entity;

    if (eventName == nullptr)
    {
        LE_DEBUG("Parameter error: event name is null");
        return LE_FAULT;
    }

    ss << "WHERE " << mFileName << "." << "Event_Name = '" << eventName << "'";

    std::string where = ss.str();
    LE_DEBUG("WriteStatusAndDtcByEventName: where is %s", where.c_str());

    DataStatement statement(Query(where));
    if (!statement.ExecuteRowStep())
    {
        // Insert data into the table.
        entity.SetEventName(eventName);
        entity.SetEventDtc(dtc);
        entity.SetEventStatus(status);
        entity.SetTestFailedCounter(0);
        std::time_t now = std::time(nullptr);
        entity.SetCreateTime(now);
        entity.SetUpdateTime(now);
        ret = Add(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to insert event name(%s). ret=%d",
                eventName, (int32_t)ret);
            return ret;
        }
    }
    else
    {
        // Update data.
        ReadEntity(statement, entity);
        entity.SetEventStatus(status);
        entity.SetEventDtc(dtc);
        std::time_t now = std::time(nullptr);
        entity.SetUpdateTime(now);

        ret = Update(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to update event name(%s). ret=%d",
                eventName, (int32_t)ret);
            return ret;
        }

        while (statement.ExecuteRowStep())
        {
            // Get the repetitive line in the db, shall update too.
            LE_DEBUG("Get a repetitive result. event name(%s).",
                eventName);
            Update(entity);
        }
    }

    return LE_OK;
}

le_result_t EventEntityDao::WriteFailedCounterByEventId
(
    int32_t eventId,
    int32_t counter
)
{
    le_result_t ret;
    EventEntity entity;

    entity.SetEventId(eventId);
    ret = QueryByKey(entity);
    if (ret == LE_NOT_FOUND)  // Not record
    {
        // Insert.
        entity.SetEventId(eventId);
        entity.SetEventName("");
        entity.SetTestFailedCounter(counter);

        std::time_t now = std::time(nullptr);
        entity.SetCreateTime(now);
        entity.SetUpdateTime(now);

        ret = Add(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to insert event0x%x with counter%d. ret=%d",
                eventId, counter, (int32_t)ret);
            return ret;
        }
    }
    else if (ret == LE_OK)
    {
        // Update.
        entity.SetTestFailedCounter(counter);
        std::time_t now = std::time(nullptr);
        entity.SetUpdateTime(now);

        ret = Update(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to update event0x%x with status0x%x. ret=%d",
                eventId, counter, (int32_t)ret);
            return ret;
        }
    }
    else
    {
        LE_ERROR("Failed to query by event id0x%x. ret=%d", eventId, (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

le_result_t EventEntityDao::ClearEventRecord
(
)
{
    le_result_t ret;

    DropTable(mHandler, false);
    ret = CreateTable(mHandler, false);
    if (ret != LE_OK)
    {
        // If drop table failure, here will return NOK
        LE_ERROR("Failed to clear Event table. ret=%d", (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

le_result_t EventEntityDao::ClearEventRecord
(
    int32_t eventId
)
{
    le_result_t ret;
    EventEntity entity;

    entity.SetEventId(eventId);
    ret = QueryByKey(entity);
    if (ret == LE_OK)
    {
        // Update.
        entity.SetEventStatus(0);
        std::time_t now = std::time(nullptr);
        entity.SetUpdateTime(now);

        ret = Update(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to clear event0x%x. ret=%d",
                eventId, (int32_t)ret);
            return ret;
        }
    }

    return LE_OK;
}

le_result_t EventEntityDao::ClearEventRecordByDtc
(
    int32_t dtc
)
{
    le_result_t ret;
    EventEntity entity;
    std::stringstream ss;

    ss << "WHERE " << mFileName << "." << "DTC=" << "'" << dtc << "'";

    std::string where = ss.str();
    LE_DEBUG("ClearEventRecordByDtc: where is %s", where.c_str());

    DataStatement statement(Query(where));
    while (statement.ExecuteRowStep())
    {
        ReadEntity(statement, entity);
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
