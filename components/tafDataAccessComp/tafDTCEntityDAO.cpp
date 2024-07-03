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
#include "legato.h"
#include "interfaces.h"

#include <strings.h>
#include <sstream>
#include "tafBaseDAO.hpp"
#include "tafIOHandler.hpp"
#include "tafDTCEntityDAO.hpp"

using namespace taf::dataAccess;

DtcEntityDao::DtcEntityDao
(
)
{
}

DtcEntityDao::~DtcEntityDao
(
)
{
    if (mHandler != nullptr)
    {
        mHandler->DetachDao();
    }
}

DtcEntityDao &DtcEntityDao::GetInstance
(
)
{
    static DtcEntityDao instance;

    return instance;
}

void DtcEntityDao::Init
(
    const char *dbName,
    int expectedVer
)
{
    auto &factory = IOFactory::GetInstance();
    std::shared_ptr<IOHandler<DtcEntity, int32_t>> handler =
        factory.getHandler<DtcEntity, int32_t>(storageType, dbName);

    LE_DEBUG("IO handler is %p", handler.get());

    Init(DTC_TABLE_NAME, handler);
    handler->AttachDao(this);

    dtcStatusPool = le_mem_CreatePool("DTCStatusPool", sizeof(taf_DataAccess_DTCStatus_t));
    fdcPool = le_mem_CreatePool("FDCPool", sizeof(taf_DataAccess_FDCInfo_t));

    if (handler->CheckTableExist())
    {
        LE_DEBUG("Table exist. check if need upgrade");
        int currentVer = 0;

        handler->GetVersion(currentVer);
        UpdateTable(handler, currentVer, expectedVer);
    }
    else
    {
        LE_DEBUG("Table does not exist. creat the table");
        CreateTable(handler, true);
    }

    // Store the column name information.
    pkList.push_back("DTC");
    columnList.push_back("DTC");
    columnList.push_back("Status");
    columnList.push_back("Fault_Occurence_Counter");
    columnList.push_back("Aging_Counter");
    columnList.push_back("Aged_Counter");
    columnList.push_back("Activation");     // Added in version=1
    columnList.push_back("Suppression");    // Added in version=1
    columnList.push_back("Creation_Time");
    columnList.push_back("Update_Time");
    columnList.push_back("Test_Failed_Time");
    columnList.push_back("Confirmed_Time");
}

le_result_t DtcEntityDao::CreateTable
(
    std::shared_ptr<IOHandler<DtcEntity, int32_t>> handler,
    bool ifNotExists
)
{
    std::string constraint = ifNotExists ? "IF NOT EXISTS " : "";
    std::string sql = "CREATE TABLE " + constraint + DTC_TABLE_NAME + " (" +
        "DTC INTEGER PRIMARY KEY," +            // Row-0: dtc.
        "Status INTEGER," +                     // Row-1: status.
        "Fault_Occurence_Counter INTEGER," +    // Row-2: Fault Occurence Counter.
        "Aging_Counter INTEGER," +              // Row-3: Aging Counter.
        "Aged_Counter INTEGER," +               // Row-4: Aged Counter.
        "Activation INTEGER," +                 // Row-5: Activation status.
        "Suppression INTEGER," +                // Row-6: Suppression status.
        "Creation_Time TEXT," +                 // Row-7: Creation Time.
        "Update_Time TEXT," +                   // Row-8: Update Time.
        "Test_Failed_Time TEXT," +              // Row-9: Test Failed Time.
        "Confirmed_Time TEXT)";                 // Row-10: Confirmed Time.

    le_result_t ret = handler->ExecRaw(sql);
    if (ret != LE_OK)
    {
        LE_ERROR("Create table(%s) failed.", DTC_TABLE_NAME);
        return ret;
    }

    return LE_OK;
}

le_result_t DtcEntityDao::DropTable
(
    std::shared_ptr<IOHandler<DtcEntity, int32_t>> handler,
    bool ifExists
)
{
    std::string constraint = ifExists ? "IF EXISTS " : "";
    std::string sql = "DROP TABLE " + constraint + DTC_TABLE_NAME;

    return handler->ExecRaw(sql);
}

le_result_t DtcEntityDao::UpdateTable
(
    std::shared_ptr<IOHandler<DtcEntity, int32_t>> handler,
    int currentVer,
    int expectedVer
)
{
    std::string createSql = std::string("CREATE TABLE ") + DTC_TABLE_NAME + " (" +
        "DTC INTEGER PRIMARY KEY," +            // Row-0: dtc.
        "Status INTEGER," +                     // Row-1: status.
        "Fault_Occurence_Counter INTEGER," +    // Row-2: Fault Occurence Counter.
        "Aging_Counter INTEGER," +              // Row-3: Aging Counter.
        "Aged_Counter INTEGER," +               // Row-4: Aged Counter.
        "Activation INTEGER," +                 // Row-5: Activation status.
        "Suppression INTEGER," +                // Row-6: Suppression status.
        "Creation_Time TEXT," +                 // Row-7: Creation Time.
        "Update_Time TEXT," +                   // Row-8: Update Time.
        "Test_Failed_Time TEXT," +              // Row-9: Test Failed Time.
        "Confirmed_Time TEXT);";                 // Row-10: Confirmed Time.

    std::string upgrade[] = {
        // Version0
        "",

        // Version1
        std::string("BEGIN TRANSACTION;") +
        "ALTER TABLE " + DTC_TABLE_NAME + " RENAME TO _temp_" + DTC_TABLE_NAME + ";" +
        createSql +
        "INSERT INTO " + DTC_TABLE_NAME + " SELECT DTC, Status, Fault_Occurence_Counter, " +
        "Aging_Counter, Aged_Counter, 1, 0, Creation_Time, Update_Time, Test_Failed_Time, " +
        "Confirmed_Time FROM _temp_" + DTC_TABLE_NAME + ";" +
        "DROP TABLE _temp_" + DTC_TABLE_NAME + ";" +
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

            le_result_t ret = handler->ExecRaw(upgrade[i]);
            if (ret != LE_OK)
            {
                LE_ERROR("Update table(%s) failed.", DTC_TABLE_NAME);
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

void DtcEntityDao::BindValues
(
    DataStatement &statement,
    DtcEntity &entity
)
{
    std::tm *tmPtr;
    char buf[DTC_TIME_BUF_SIZE] = {0};

    // Clear all binding parameters on the statement.
    statement.ClearBindings();

    int32_t dtc = entity.GetDtc();
    if (dtc != -1)
    {
        statement.BindValue(1, dtc);
    }

    int32_t status = entity.GetStatus();
    if (status != -1)
    {
        statement.BindValue(2, status);
    }

    int32_t foc = entity.GetFaultOccurenceCounter();
    if (foc != -1)
    {
        statement.BindValue(3, foc);
    }

    int32_t aging = entity.GetAgingCounter();
    if (aging != -1)
    {
        statement.BindValue(4, aging);
    }

    int32_t aged = entity.GetAgedCounter();
    if (aged != -1)
    {
        statement.BindValue(5, aged);
    }

    int32_t activation = entity.GetActivation();
    if (activation != -1)
    {
        statement.BindValue(6, activation);
    }

    int32_t suppression = entity.GetSuppression();
    if (suppression != -1)
    {
        statement.BindValue(7, suppression);
    }

    std::time_t create = entity.GetCreateTime();
    if (create != 0)
    {

        tmPtr = std::localtime(&create);
        std::strftime(buf, DTC_TIME_BUF_SIZE, "%Y-%m-%d %H:%M:%S", tmPtr);

        statement.BindValue(8, static_cast<const char *>(buf));
    }

    std::time_t update = entity.GetUpdateTime();
    if (update != 0)
    {

        tmPtr = std::localtime(&update);
        std::strftime(buf, DTC_TIME_BUF_SIZE, "%Y-%m-%d %H:%M:%S", tmPtr);

        statement.BindValue(9, static_cast<const char *>(buf));
    }

    std::time_t testFailed = entity.GetTestFailedTime();
    if (testFailed != 0)
    {

        tmPtr = std::localtime(&testFailed);
        std::strftime(buf, DTC_TIME_BUF_SIZE, "%Y-%m-%d %H:%M:%S", tmPtr);

        statement.BindValue(10, static_cast<const char *>(buf));
    }

    std::time_t confirmed = entity.GetConfirmedTime();
    if (confirmed != 0)
    {

        tmPtr = std::localtime(&confirmed);
        std::strftime(buf, DTC_TIME_BUF_SIZE, "%Y-%m-%d %H:%M:%S", tmPtr);

        statement.BindValue(11, static_cast<const char *>(buf));
    }
}

void DtcEntityDao::BindKeyValue
(
    DataStatement &statement,
    DtcEntity &entity
)
{
    // Clear all binding parameters on the statement.
    statement.ClearBindings();

    int32_t dtc = entity.GetDtc();
    if (dtc != -1)
    {
        statement.BindValue(1, dtc);
    }
}

void DtcEntityDao::BindKeyValue
(
    DataStatement &statement,
    int index,
    int32_t key
)
{
    statement.BindValue(index, key);
}

void DtcEntityDao::ReadEntity
(
    DataStatement &statement,
    DtcEntity &entity
)
{
    entity.SetDtc(statement.GetColumnInt(0));
    entity.SetStatus(statement.GetColumnInt(1));
    entity.SetFaultOccurenceCounter(statement.GetColumnInt(2));
    entity.SetAgingCounter(statement.GetColumnInt(3));
    entity.SetAgedCounter(statement.GetColumnInt(4));
    entity.SetActivation(statement.GetColumnInt(5));
    entity.SetSuppression(statement.GetColumnInt(6));

    auto createTimePtr = reinterpret_cast<const char*>(statement.GetColumnText(7, nullptr));
    if (createTimePtr == nullptr)
    {
        entity.SetCreateTime(0);
    }
    else
    {
        entity.SetCreateTime(String2Time(createTimePtr));
    }

    auto updateTimePtr = reinterpret_cast<const char*>(statement.GetColumnText(8, nullptr));
    if (updateTimePtr == nullptr)
    {
        entity.SetUpdateTime(0);
    }
    else
    {
        entity.SetUpdateTime(String2Time(updateTimePtr));
    }

    auto failedTimePtr = reinterpret_cast<const char*>(statement.GetColumnText(9, nullptr));
    if (failedTimePtr == nullptr)
    {
        entity.SetTestFailedTime(0);
    }
    else
    {
        entity.SetTestFailedTime(String2Time(failedTimePtr));
    }

    auto confirmedTimePtr = reinterpret_cast<const char*>(statement.GetColumnText(10, nullptr));
    if (confirmedTimePtr == nullptr)
    {
        entity.SetConfirmedTime(0);
    }
    else
    {
        entity.SetConfirmedTime(String2Time(confirmedTimePtr));
    }
}

int32_t DtcEntityDao::ReadKey
(
    DataStatement &statement
)
{
    return statement.GetColumnInt(0);
}

int32_t DtcEntityDao::GetKey
(
    DtcEntity &entity
)
{
    return entity.GetDtc();
}

bool DtcEntityDao::HasKey
(
    DtcEntity &entity
)
{
    return (entity.GetDtc() == -1) ? false : true;
}

std::vector<std::string> DtcEntityDao::GetColumnsName
(
)
{
    return columnList;
}

std::vector<std::string> DtcEntityDao::GetPrimaryKeysName
(
)
{
    return pkList;
}

int32_t DtcEntityDao::ReadDtcCountByDtc
(
    int32_t dtc
)
{
    std::stringstream ss;

    ss << "WHERE " << mFileName << "." << "DTC=" << "'" << dtc << "'";

    std::string where = ss.str();
    LE_DEBUG("ReadDtcCountByDtc: where is %s", where.c_str());

    DataStatement statement(QueryCount(where));
    (void)statement.ExecuteRowStep();  // Cannot return false as this query always return a result.

    return statement.GetColumnInt(0);
}

int32_t DtcEntityDao::ReadDtcCountByDtcWithSuppress
(
    int32_t dtc
)
{
    std::stringstream ss;

    ss << "WHERE " << mFileName << "." << "DTC=" << "'" << dtc << "'";
    ss << " AND Suppression != 1";

    std::string where = ss.str();
    LE_DEBUG("ReadDtcCountByDtcWithSuppress: where is %s", where.c_str());

    DataStatement statement(QueryCount(where));
    (void)statement.ExecuteRowStep();  // Cannot return false as this query always return a result.

    return statement.GetColumnInt(0);
}

int32_t DtcEntityDao::ReadDtcCountByStatus
(
    int32_t status
)
{
    std::stringstream ss;

    ss << "WHERE " << mFileName << "." << "Status & " << status << " != 0";

    std::string where = ss.str();
    LE_DEBUG("ReadDtcCountByStatus: where is %s", where.c_str());

    DataStatement statement(QueryCount(where));
    (void)statement.ExecuteRowStep();  // Cannot return false as this query always return a result.

    return statement.GetColumnInt(0);
}

int32_t DtcEntityDao::ReadDtcCountByStatusWithSuppress
(
    int32_t status
)
{
    std::stringstream ss;

    ss << "WHERE " << mFileName << "." << "Status & " << status << " != 0";
    ss << " AND Suppression != 1";

    std::string where = ss.str();
    LE_DEBUG("ReadDtcCountByStatus: where is %s", where.c_str());

    DataStatement statement(QueryCount(where));
    (void)statement.ExecuteRowStep();  // Cannot return false as this query always return a result.

    return statement.GetColumnInt(0);
}

le_result_t DtcEntityDao::ReadDtcByStatus
(
    int32_t status,
    taf_DataAccess_DTCStatusRec_t *dtcStatusPtr
)
{
    taf_DataAccess_DTCStatus_t *dtcStaPtr;
    std::stringstream ss;
    uint8_t available = taf_DataAccess_GetAvailableStatusMask();

    ss << "WHERE " << mFileName << "." << "Status & " << status << " != 0";
    ss << " AND Suppression != 1";

    std::string where = ss.str();
    LE_DEBUG("ReadDtcByStatus: where is %s", where.c_str());

    dtcStatusPtr->dtcStatusRecList  = LE_DLS_LIST_INIT;
    DataStatement statement(Query(where));

    while (statement.ExecuteRowStep())
    {
        LE_DEBUG("Execute step get a result");
        dtcStaPtr = (taf_DataAccess_DTCStatus_t*)le_mem_ForceAlloc(dtcStatusPool);
        memset(dtcStaPtr, 0, sizeof(taf_DataAccess_DTCStatus_t));

        LE_DEBUG("DTC is 0x%x, status is 0x%x",
            statement.GetColumnInt(0),
            statement.GetColumnInt(1));
        dtcStaPtr->dtc = static_cast<uint32_t>(statement.GetColumnInt(0));
        dtcStaPtr->status = static_cast<uint8_t>(statement.GetColumnInt(1));
        dtcStaPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&dtcStatusPtr->dtcStatusRecList, &dtcStaPtr->link);
    }

    dtcStatusPtr->availableMask = available;

    return LE_OK;
}

le_result_t DtcEntityDao::ReadStatusByDtc
(
    int32_t dtc,
    int32_t& status
)
{
    le_result_t ret;
    DtcEntity entity;

    entity.SetDtc(dtc);
    ret = QueryByKey(entity);
    if (ret != LE_OK)
    {
        if (ret == LE_NOT_FOUND)
        {
            status = 0;
        }
        else
        {
            LE_ERROR("Failed to query entity by key. ret=%d", (int32_t)ret);
        }
        return ret;
    }

    LE_DEBUG("ReadStatusByDtc: Get status(0x%x) for DTC0x%x", entity.GetStatus(), dtc);
    status = entity.GetStatus();

    return LE_OK;
}

le_result_t DtcEntityDao::ReadStatusByDtc
(
    int32_t dtc,
    int32_t& status,
    int32_t& occurCounter
)
{
    le_result_t ret;
    DtcEntity entity;

    entity.SetDtc(dtc);
    ret = QueryByKey(entity);
    if (ret != LE_OK)
    {
        if (ret == LE_NOT_FOUND)
        {
            status = 0;
            occurCounter = 0;
        }
        else
        {
            LE_ERROR("Failed to query entity by key. ret=%d", (int32_t)ret);
        }
        return ret;
    }

    LE_DEBUG("ReadStatusByDtc: Get status(0x%x) for DTC0x%x", entity.GetStatus(), dtc);
    status = entity.GetStatus();
    occurCounter = entity.GetFaultOccurenceCounter();

    return LE_OK;
}

le_result_t DtcEntityDao::ReadStatusByDtc
(
    int32_t dtc,
    int32_t& status,
    int32_t& occurCounter,
    int32_t& activation,
    int32_t& suppression
)
{
    le_result_t ret;
    DtcEntity entity;

    entity.SetDtc(dtc);
    ret = QueryByKey(entity);
    if (ret != LE_OK)
    {
        if (ret == LE_NOT_FOUND)
        {
            status = 0;
            occurCounter = 0;
        }
        else
        {
            LE_ERROR("Failed to query entity by key. ret=%d", (int32_t)ret);
        }
        return ret;
    }

    LE_DEBUG("ReadStatusByDtc: Get status(0x%x) for DTC0x%x", entity.GetStatus(), dtc);
    status = entity.GetStatus();
    occurCounter = entity.GetFaultOccurenceCounter();
    activation = entity.GetActivation();
    suppression = entity.GetSuppression();

    return LE_OK;
}

le_result_t DtcEntityDao::ReadFaultDetectionCounter
(
    taf_DataAccess_FDCInfoRec_t *fdcInfoRecPtr
)
{
    taf_DataAccess_FDCInfo_t *fdcPtr;
    std::stringstream ss;

    ss << "WHERE " << mFileName << "." << "Fault_Occurence_Counter>" << 0;

    std::string where = ss.str();
    LE_DEBUG("GetDtcByStatus: where is %s", where.c_str());

    DataStatement statement(Query(where));
    fdcInfoRecPtr->fdcInfoList = LE_DLS_LIST_INIT;

    while (statement.ExecuteRowStep())
    {
        fdcPtr = (taf_DataAccess_FDCInfo_t*)le_mem_ForceAlloc(fdcPool);
        memset(fdcPtr, 0, sizeof(taf_DataAccess_FDCInfo_t));

        fdcPtr->dtc = static_cast<uint32_t>(statement.GetColumnInt(0));
        fdcPtr->faultDetCounter = static_cast<uint8_t>(statement.GetColumnInt(2));
        fdcPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&fdcInfoRecPtr->fdcInfoList, &fdcPtr->link);
    }

    return LE_OK;
}

le_result_t DtcEntityDao::WriteStatusByDtc
(
    int32_t dtc,
    int32_t status
)
{
    le_result_t ret;
    DtcEntity entity;

    entity.SetDtc(dtc);
    ret = QueryByKey(entity);
    if (ret != LE_OK)  // Not record
    {
        // Insert.
        entity.SetDtc(dtc);
        entity.SetStatus(status);
        entity.SetFaultOccurenceCounter(0);
        entity.SetAgingCounter(0);
        entity.SetAgedCounter(0);
        entity.SetActivation(1);
        entity.SetSuppression(0);
        std::time_t now = std::time(nullptr);
        entity.SetCreateTime(now);
        entity.SetUpdateTime(now);

        ret = Add(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to insert dtc0x%x with status0x%x. ret=%d",
                dtc, status, (int32_t)ret);
            return ret;
        }
    }
    else
    {
        // Update.
        entity.SetDtc(dtc);
        entity.SetStatus(status);
        std::time_t now = std::time(nullptr);
        entity.SetUpdateTime(now);

        ret = Update(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to update dtc0x%x with status0x%x. ret=%d",
                dtc, status, (int32_t)ret);
            return ret;
        }
    }

    return LE_OK;
}

le_result_t DtcEntityDao::WriteStatusByDtc
(
    int32_t dtc,
    int32_t status,
    int32_t occurCounter
)
{
    le_result_t ret;
    DtcEntity entity;

    entity.SetDtc(dtc);
    ret = QueryByKey(entity);
    if (ret != LE_OK)  // Not record
    {
        // Insert.
        entity.SetDtc(dtc);
        entity.SetStatus(status);
        entity.SetFaultOccurenceCounter(occurCounter);
        entity.SetAgingCounter(0);
        entity.SetAgedCounter(0);
        entity.SetActivation(1);
        entity.SetSuppression(0);
        std::time_t now = std::time(nullptr);
        entity.SetCreateTime(now);
        entity.SetUpdateTime(now);

        ret = Add(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to insert dtc0x%x with status0x%x. ret=%d",
                dtc, status, (int32_t)ret);
            return ret;
        }
    }
    else
    {
        // Update.
        entity.SetDtc(dtc);
        entity.SetStatus(status);
        entity.SetFaultOccurenceCounter(occurCounter);
        std::time_t now = std::time(nullptr);
        entity.SetUpdateTime(now);

        ret = Update(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to update dtc0x%x with status0x%x. ret=%d",
                dtc, status, (int32_t)ret);
            return ret;
        }
    }

    return LE_OK;
}

le_result_t DtcEntityDao::WriteFaultDetectionCounterByDtc
(
    int32_t dtc,
    int32_t fdc
)
{
    le_result_t ret;
    DtcEntity entity;

    entity.SetDtc(dtc);
    ret = QueryByKey(entity);
    if (ret != LE_OK)  // Not found.
    {
        // Insert.
        entity.SetDtc(dtc);
        entity.SetStatus(0);
        entity.SetFaultOccurenceCounter(fdc);
        entity.SetAgingCounter(0);
        entity.SetAgedCounter(0);
        entity.SetActivation(1);
        entity.SetSuppression(0);
        std::time_t now = std::time(nullptr);
        entity.SetCreateTime(now);
        entity.SetUpdateTime(now);

        ret = Add(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to insert dtc0x%x with FaultDetectionCounter%d. ret=%d",
                dtc, fdc, (int32_t)ret);
            return ret;
        }
    }
    else
    {
        // Update.
        entity.SetDtc(dtc);
        entity.SetFaultOccurenceCounter(fdc);
        std::time_t now = std::time(nullptr);
        entity.SetUpdateTime(now);

        ret = Update(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to update dtc0x%x with FaultDetectionCounter%d. ret=%d",
                dtc, fdc, (int32_t)ret);
            return ret;
        }
    }

    return LE_OK;
}

le_result_t DtcEntityDao::ClearDtcRecord
(
)
{
    le_result_t ret;

    DropTable(mHandler, false);
    ret = CreateTable(mHandler, false);
    if (ret != LE_OK)
    {
        // If drop table failure, here will return NOK
        LE_ERROR("Failed to clear DTC table. ret=%d", (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

le_result_t DtcEntityDao::ClearDtcRecord
(
    int32_t dtc
)
{
    le_result_t ret;
    DtcEntity entity;

    entity.SetDtc(dtc);
    ret = Remove(entity);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to delete dtc0x%x record. ret=%d",
            dtc, (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

le_result_t DtcEntityDao::ReadActivationByDtc
(
    int32_t dtc,
    int32_t& activation
)
{
    le_result_t ret;
    DtcEntity entity;

    entity.SetDtc(dtc);
    ret = QueryByKey(entity);
    if (ret != LE_OK)
    {
        if (ret == LE_NOT_FOUND)
        {
            activation = 1;  // Default value is 1.
        }
        else
        {
            LE_ERROR("Failed to query entity by key. ret=%d", (int32_t)ret);
        }
        return ret;
    }

    LE_DEBUG("ReadStatusByDtc: Get activation(0x%x) for DTC0x%x", entity.GetActivation(), dtc);
    activation = entity.GetActivation();

    return LE_OK;
}

le_result_t DtcEntityDao::WriteActivationByDtc
(
    int32_t dtc,
    int32_t activation
)
{
    le_result_t ret;
    DtcEntity entity;

    entity.SetDtc(dtc);
    ret = QueryByKey(entity);
    if (ret != LE_OK)  // Not record
    {
        // Insert.
        entity.SetDtc(dtc);
        entity.SetStatus(0);
        entity.SetFaultOccurenceCounter(0);
        entity.SetAgingCounter(0);
        entity.SetAgedCounter(0);
        entity.SetActivation(activation);
        entity.SetSuppression(0);
        std::time_t now = std::time(nullptr);
        entity.SetCreateTime(now);
        entity.SetUpdateTime(now);

        ret = Add(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to insert dtc0x%x with activation%x. ret=%d",
                dtc, activation, (int32_t)ret);
            return ret;
        }
    }
    else
    {
        // Update.
        entity.SetDtc(dtc);
        entity.SetActivation(activation);
        std::time_t now = std::time(nullptr);
        entity.SetUpdateTime(now);

        ret = Update(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to update dtc0x%x with activation%x. ret=%d",
                dtc, activation, (int32_t)ret);
            return ret;
        }
    }

    return LE_OK;
}

le_result_t DtcEntityDao::ReadSuppressionByDtc
(
    int32_t dtc,
    int32_t& suppression
)
{
    le_result_t ret;
    DtcEntity entity;

    entity.SetDtc(dtc);
    ret = QueryByKey(entity);
    if (ret != LE_OK)
    {
        if (ret == LE_NOT_FOUND)
        {
            suppression = 0;  // Default value is 0.
        }
        else
        {
            LE_ERROR("Failed to query entity by key. ret=%d", (int32_t)ret);
        }
        return ret;
    }

    LE_DEBUG("ReadStatusByDtc: Get suppression(0x%x) for DTC0x%x", entity.GetSuppression(), dtc);
    suppression = entity.GetSuppression();

    return LE_OK;
}

le_result_t DtcEntityDao::WriteSuppressionByDtc
(
    int32_t dtc,
    int32_t suppression
)
{
    le_result_t ret;
    DtcEntity entity;

    entity.SetDtc(dtc);
    ret = QueryByKey(entity);
    if (ret != LE_OK)  // Not record
    {
        // Insert.
        entity.SetDtc(dtc);
        entity.SetStatus(0);
        entity.SetFaultOccurenceCounter(0);
        entity.SetAgingCounter(0);
        entity.SetAgedCounter(0);
        entity.SetActivation(1);
        entity.SetSuppression(suppression);
        std::time_t now = std::time(nullptr);
        entity.SetCreateTime(now);
        entity.SetUpdateTime(now);

        ret = Add(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to insert dtc0x%x with suppression%x. ret=%d",
                dtc, suppression, (int32_t)ret);
            return ret;
        }
    }
    else
    {
        // Update.
        entity.SetDtc(dtc);
        entity.SetSuppression(suppression);
        std::time_t now = std::time(nullptr);
        entity.SetUpdateTime(now);

        ret = Update(entity);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to update dtc0x%x with suppression0x%x. ret=%d",
                dtc, suppression, (int32_t)ret);
            return ret;
        }
    }

    return LE_OK;
}

le_result_t DtcEntityDao::WriteAllSuppression
(
    int32_t suppression
)
{
    std::stringstream ss;
    ss << "UPDATE " << mFileName << " SET Suppression = " << suppression;
    ss << " WHERE Suppression != " << suppression << ";";
    std::string sql = ss.str();

    le_result_t ret = mHandler->ExecRaw(sql);
    if (ret != LE_OK)
    {
        LE_ERROR("Update suppression to %d failed.", suppression);
        return ret;
    }

    return LE_OK;
}

std::vector<int32_t> DtcEntityDao::GetDTCWithUnsuppress
(
)
{
    std::stringstream ss;
    std::vector<int32_t> dtcs;

    ss << "WHERE " << mFileName << "." << "Suppression = " << 0;

    std::string where = ss.str();
    LE_DEBUG("GetDTCWithUnsuppress: where is %s", where.c_str());

    DataStatement statement(Query(where));

    while (statement.ExecuteRowStep())
    {
        int32_t dtc;

        dtc = statement.GetColumnInt(0);
        dtcs.push_back(dtc);
    }

    return dtcs;
}

std::vector<int32_t> DtcEntityDao::GetDTCWithSuppress
(
)
{
    std::stringstream ss;
    std::vector<int32_t> dtcs;

    ss << "WHERE " << mFileName << "." << "Suppression = " << 1;

    std::string where = ss.str();
    LE_DEBUG("GetDTCWithSuppress: where is %s", where.c_str());

    DataStatement statement(Query(where));

    while (statement.ExecuteRowStep())
    {
        int32_t dtc;

        dtc = statement.GetColumnInt(0);
        dtcs.push_back(dtc);
    }

    return dtcs;
}
