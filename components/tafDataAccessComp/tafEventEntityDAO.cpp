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
#include <ctime>
#include <string>

#include "legato.h"
#include "interfaces.h"

#include "tafBaseDAO.hpp"
#include "tafIOHandler.hpp"
#include "tafEventEntityDAO.hpp"

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
    const char *dbName
)
{
    auto &factory = IOFactory::GetInstance();
    std::shared_ptr<IOHandler<EventEntity, int32_t>> handler =
        factory.getHandler<EventEntity, int32_t>(storageType, dbName);

    LE_DEBUG("IO handler is %p", handler.get());

    Init(EVENT_TABLE_NAME, handler);
    handler->AttachDao(this);

    CreateTable(handler, true);

    // Store the column name information.
    pkList.push_back("Event_ID");
    columnList.push_back("Event_ID");
    columnList.push_back("DTC");
    columnList.push_back("Status");
    columnList.push_back("Test_Failed_Counter");
    columnList.push_back("Creation_Time");
    columnList.push_back("Update_Time");
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
        "DTC INTEGER," +                        // Row-1: DTC.
        "Status INTEGER," +                     // Row-2: Event status.
        "Test_Failed_Counter INTEGER," +        // Row-3: Test failed counter.
        "Creation_Time TEXT," +                 // Row-4: Creation Time.
        "Update_Time TEXT)";                    // Row-5: Update Time.

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

    int32_t dtc = entity.GetEventDtc();
    if (dtc != -1)
    {
        statement.BindValue(2, dtc);
    }

    int32_t status = entity.GetEventStatus();
    if (status != -1)
    {
        statement.BindValue(3, status);
    }

    int32_t counter = entity.GetTestFailedCounter();
    if (counter != -1)
    {
        statement.BindValue(4, counter);
    }

    std::time_t create = entity.GetCreateTime();
    if (create != 0)
    {

        tmPtr = std::localtime(&create);
        std::strftime(buf, EVENT_TIME_BUF_SIZE, "%Y-%m-%d %H:%M:%S", tmPtr);

        statement.BindValue(5, static_cast<const char *>(buf));
    }

    std::time_t update = entity.GetUpdateTime();
    if (update != 0)
    {

        tmPtr = std::localtime(&update);
        std::strftime(buf, EVENT_TIME_BUF_SIZE, "%Y-%m-%d %H:%M:%S", tmPtr);

        statement.BindValue(6, static_cast<const char *>(buf));
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
    entity.SetEventDtc(statement.GetColumnInt(1));
    entity.SetEventStatus(statement.GetColumnInt(2));
    entity.SetTestFailedCounter(statement.GetColumnInt(3));

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
    LE_DEBUG("ReadDtcCountByStatus: where is %s", where.c_str());

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
        entity.SetEventDtc(dtc);
        entity.SetEventStatus(status);

        if (status != 0)
        {
            entity.SetTestFailedCounter(0);
        }
        else
        {
            entity.SetTestFailedCounter(1);
        }

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

        int32_t counter = entity.GetTestFailedCounter();
        if (counter < TEST_FAILED_COUNTER_MAX)
        {
            counter++;
            entity.SetTestFailedCounter(counter);
        }
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
    ret = Remove(entity);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to delete event0x%x record from event table. ret=%d",
            eventId, (int32_t)ret);
        return ret;
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

    entity.SetEventDtc(dtc);
    ret = Remove(entity);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to delete dtc0x%x record from event table. ret=%d",
            dtc, (int32_t)ret);
        return ret;
    }

    return LE_OK;
}
