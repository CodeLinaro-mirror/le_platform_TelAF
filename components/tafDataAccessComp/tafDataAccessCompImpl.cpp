/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include "tafDataAccessComp.h"
#include "tafDTCEntityDAO.hpp"
#include "tafEventEntityDAO.hpp"
#include "tafSnapshotEntityDAO.hpp"
#include "tafDataAccessCompImpl.hpp"
#include "configuration.hpp"

using namespace tafsvc;
using namespace std;
using namespace taf::dataAccess;

// Define the pool for snapshot data and extended data.
LE_MEM_DEFINE_STATIC_POOL(DemRecordDataPool, DEM_RECORD_DATA_SIZE, DEM_RECORD_DATA_BYTES);

DemDataHandler &DemDataHandler::GetInstance
(
)
{
    static DemDataHandler instance;

    return instance;
}

DemDataHandler::DemDataHandler
(
)
{
}

DemDataHandler::~DemDataHandler
(
)
{
}

void DemDataHandler::Init
(
)
{
    auto &tafDtcDao = DtcEntityDao::GetInstance();
    tafDtcDao.Init(DEM_DATABASE_NAME, DEM_DB_VERSION);

    auto &tafEventDao = EventEntityDao::GetInstance();
    tafEventDao.Init(DEM_DATABASE_NAME);

    auto &tafSnapshotDao = SnapshotEntityDao::GetInstance();
    tafSnapshotDao.Init(DEM_DATABASE_NAME);

    extDataPool = le_mem_CreatePool("ExtDataPool", sizeof(taf_DataAccess_ExtData_t));
    supportedDtcPool = le_mem_CreatePool("suppDtcPool", sizeof(taf_DataAccess_DTCStatus_t));
    dtcDataInfoPool = le_mem_CreatePool("DTCDatapool", sizeof(taf_DataAccess_DtcDataInfo_t));
    didNodePool = le_mem_CreatePool("DIDNodePool", sizeof(taf_DataAccess_DidNode_t));
    recordDataPool = le_mem_InitStaticPool(DemRecordDataPool, DEM_RECORD_DATA_SIZE,
        DEM_RECORD_DATA_BYTES);

    // Most DTC data is short, so create them from a sub-pool
    recordDataPool = le_mem_CreateReducedPool(recordDataPool, "RecordDataSmallPool",
        DEM_RECORD_DATA_TYPICAL_SIZE, DEM_RECORD_DATA_TYPICAL_BYTES);

    freezeFramePool = le_mem_CreatePool("FreezeFramePool", sizeof(taf_DataAccess_SnapshotData_t));

    // Update the DEM user_version after initialization.
    tafDtcDao.GetDaoHandler()->SetVersion(DEM_DB_VERSION);
}

le_result_t DemDataHandler::Load
(
)
{
    // Get configuration from diagConfig module.
    try
    {
        std::vector<uint32_t> dtc_code_list = cfg::get_dtc_codes();
        for (const auto & dtc_code: dtc_code_list)
        {
            cfg::Node & dtc = cfg::get_dtc_node(dtc_code);

            // Init snapshot.
            cfg::Node & ffs = dtc.get_child("snapshots.freeze_frames");
            for (auto &ff : ffs)
            {
                string item = ff.second.get_value<string>("");

                // Lookup the info from freeze frames.
                cfg::Node & ffNode = cfg::top_freeze_frames<string>("short_name", item);
                int rn = ffNode.get<int>("record_number");
                if (std::string("firstOccurrence") == item)
                {
                    snapshotRnVec.push_back(std::make_tuple(
                            rn, dtc_code, Snapshot_FirstOccurrence)
                            );
                }
                else if (std::string("lastOccurrence") == item)
                {
                    snapshotRnVec.push_back(std::make_tuple(
                            rn, dtc_code, Snapshot_LastOccurrence)
                            );
                }
                else if (std::string("lastDisappearance") == item)
                {
                    snapshotRnVec.push_back(std::make_tuple(
                            rn, dtc_code, Snapshot_LastDisappearance)
                            );
                }
                else
                {
                    LE_WARN("Unexpected FF(%s) for DTC0x%x", item.c_str(), dtc_code);
                }
            }

            // Init extended data.
            cfg::Node & extds = dtc.get_child("identification.extended_data_records");
            for (auto &extd : extds)
            {
                string item = extd.second.get_value<string>("");

                // Lookup the info from freeze frames.
                cfg::Node & extdNode = cfg::top_extended_data_records<string>("short_name", item);
                int rn = extdNode.get<int>("record_number");
                if (std::string("OccurrenceCounter") == item)
                {
                    extendedRnVec.push_back(std::make_tuple(
                            rn, dtc_code, ExtendData_OccurenceCounter)
                            );
                }
                else  // Only supported OccurenceCounter for extended data records.
                {
                    LE_WARN("Unexpected ExtData(%s) for DTC0x%x", item.c_str(), dtc_code);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        // Failed to read diag configuration.
        LE_WARN("Exception: %s", e.what() );
        return LE_FAULT;
    }

    auto &tafDtcDao = DtcEntityDao::GetInstance();

    return tafDtcDao.Load();
}

le_result_t DemDataHandler::GetNumOfDtcByStatusMask
(
    uint8_t statusMask,
    taf_DataAccess_NumOfDTC_t *numOfDtcPtr
)
{
    auto &tafDtcDao = DtcEntityDao::GetInstance();
    uint8_t available = taf_DataAccess_GetAvailableStatusMask();
    LE_DEBUG("available mask:0x%x, status mask:0x%x", available, statusMask);
    statusMask &= available;

    int32_t count = tafDtcDao.ReadDtcCountByStatusWithSuppress(static_cast<int32_t>(statusMask));

    numOfDtcPtr->availableMask = available;
    numOfDtcPtr->formatIdentifier = GetDtcFormatId();
    numOfDtcPtr->dtcCnt = static_cast<uint16_t>(count);

    return LE_OK;
}

uint8_t DemDataHandler::GetAvailableStatusMask
(
)
{
    return DEM_DTC_AVAILABLE_MASK_DEF;
}

uint8_t DemDataHandler::GetDtcFormatId
(
)
{
    return DEM_DTC_FORMAT_IDENTIFIER_DEF;
}

le_result_t DemDataHandler::GetDtcByStatusMask
(
    uint8_t statusMask,
    taf_DataAccess_DTCStatusRec_t *dtcStatusPtr
)
{
    auto &tafDtcDao = DtcEntityDao::GetInstance();
    uint8_t available = GetAvailableStatusMask();
    LE_DEBUG("available mask:0x%x, status mask:0x%x", available, statusMask);
    statusMask &= available;

    le_result_t ret = tafDtcDao.ReadDtcByStatus(static_cast<int32_t>(statusMask), dtcStatusPtr);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get DTC by status. ret=%d", (int32_t)ret);
        return ret;
    }

    dtcStatusPtr->availableMask = available;

    return LE_OK;
}

le_result_t DemDataHandler::GetSupportedDtc
(
    taf_DataAccess_DTCStatusRec_t *dtcStatusPtr
)
{
    // 1. Get the supported DTCs from YAML file
    // 2. Call int32_t GetStatusByDtc(int32_t dtc) to get status.
    int32_t status = 0;
    int32_t occurrence = 0;
    int32_t activation = 1;
    int32_t suppression = 0;
    le_result_t ret;
    taf_DataAccess_DTCStatus_t *dtcStaPtr;

    try
    {
        std::vector<uint32_t> dtc_code_list = cfg::get_dtc_codes();
        auto &tafDtcDao = DtcEntityDao::GetInstance();

        dtcStatusPtr->dtcStatusRecList = LE_DLS_LIST_INIT;
        for (const auto & dtc_code: dtc_code_list)
        {
            ret = tafDtcDao.ReadStatusByDtc(dtc_code, status, occurrence, activation, suppression);
            if (ret != LE_OK)
            {
                // Not found or other error.
                status = 0;
            }
            else
            {
                if (suppression != 0)
                {
                    // Skip the suppressional DTC
                    continue;
                }
            }

            dtcStaPtr = (taf_DataAccess_DTCStatus_t*)le_mem_ForceAlloc(supportedDtcPool);
            memset(dtcStaPtr, 0, sizeof(taf_DataAccess_DTCStatus_t));

            LE_DEBUG("Get status0x%x for DTC0x%x", dtc_code, status);
            dtcStaPtr->dtc = static_cast<uint32_t>(dtc_code);
            dtcStaPtr->status = static_cast<uint8_t>(status & GetAvailableStatusMask());
            dtcStaPtr->link = LE_DLS_LINK_INIT;
            le_dls_Queue(&dtcStatusPtr->dtcStatusRecList, &dtcStaPtr->link);
        }

        dtcStatusPtr->availableMask = GetAvailableStatusMask();
    }
    catch (const std::exception& e)
    {
        // Not event in this DTC
        LE_WARN("Exception: %s", e.what() );
        return LE_FAULT;
    }

    return LE_OK;
}

le_result_t DemDataHandler::GetSnapshotIdentification
(
    taf_DataAccess_SnapshotInfoRec_t *snapshotRecInfoPtr
)
{
    auto &snapshotDao = SnapshotEntityDao::GetInstance();

    snapshotRecInfoPtr->snapshotInfoList = LE_DLS_LIST_INIT;
    snapshotDao.GetAllDTCRecord(&snapshotRecInfoPtr->snapshotInfoList);

    taf_DataAccess_SnapshotInfo_t *nodePtr;
    le_dls_Link_t* linkPtr = le_dls_Peek(&snapshotRecInfoPtr->snapshotInfoList);
    while (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, taf_DataAccess_SnapshotInfo_t, link);
        if (nodePtr != NULL)
        {
            if (GetSuppression(nodePtr->dtc) != 0)
            {
                // This DTC is under suppression. Remove from the list.
                le_dls_Link_t* tmpLinkPtr = linkPtr;
                linkPtr = le_dls_PeekNext(&snapshotRecInfoPtr->snapshotInfoList, linkPtr);
                le_dls_Remove(&snapshotRecInfoPtr->snapshotInfoList, tmpLinkPtr);
                le_mem_Release(nodePtr);
                continue;
            }
        }

        // Process next node.
        linkPtr = le_dls_PeekNext(&snapshotRecInfoPtr->snapshotInfoList, linkPtr);
    }

    return LE_OK;
}

le_result_t DemDataHandler::GetSnapshotRecByDtc
(
    uint32_t    dtc,
    uint8_t     recNumber,
    taf_DataAccess_SnapshotDataRec_t *snapshotDataRecPtr
)
{
    le_result_t ret;
    auto &tafDtcDao = DtcEntityDao::GetInstance();

    // Check if this DTC is under suppression.
    int32_t status, occurrence, activation, suppression;
    ret = tafDtcDao.ReadStatusByDtc(dtc, status, occurrence, activation, suppression);
    if (ret != LE_OK)
    {
        // Not found or other error.
        LE_ERROR("Not found snapshot for DTC0x%x", dtc);
        return LE_NOT_FOUND;
    }

    if (suppression != 0)
    {
        LE_WARN("This DTC0x%x is under suppression", dtc);
        return LE_UNAVAILABLE;
    }

    snapshotDataRecPtr->dtc = dtc;
    snapshotDataRecPtr->status = status;
    snapshotDataRecPtr->snapshotDataList = LE_DLS_LIST_INIT;

    if (recNumber == 0xFF)
    {
        return GetAllSnapshotRecByDtc(dtc, &snapshotDataRecPtr->snapshotDataList);
    }
    else
    {
        if (!IsSnapshotDataRecordNumSupported(dtc, recNumber))
        {
            LE_INFO("Record number(%d) for DTC0x%x is not supported.", recNumber, dtc);
            return LE_UNSUPPORTED;
        }

        // return GetSpecSnapshotRecByDtc(dtc, recNumber, &snapshotDataRecPtr->snapshotDataList);
        if (GetSpecSnapshotRecByDtc(dtc, recNumber, &snapshotDataRecPtr->snapshotDataList) != LE_OK)
        {
            LE_INFO("Record Number (%02x) <-- ", recNumber);
        }

        return LE_OK;
    }
}

le_result_t DemDataHandler::GetExtDataRecByDtc
(
    uint32_t    dtc,
    uint8_t     recNumber,
    taf_DataAccess_ExtDataRec_t *extDataRecPtr
)
{
    int32_t status;
    int32_t occurCounter;
    taf_DataAccess_ExtData_t *extDataPtr;
    auto &tafDtcDao = DtcEntityDao::GetInstance();

    if (recNumber != 0xFE && recNumber != 0xFF && recNumber != 1)
    {
        // Only support record number1 currently.
        LE_ERROR("Unknow the record number%d.", recNumber);
        return LE_BAD_PARAMETER;
    }

    if (GetSuppression(dtc) != 0)
    {
        LE_WARN("This DTC(0x%x) is under suppression", dtc);
        return LE_UNAVAILABLE;
    }

    // Initilze extended data list.
    extDataRecPtr->extDataList = LE_DLS_LIST_INIT;

    le_result_t ret = tafDtcDao.ReadStatusByDtc(dtc, status, occurCounter);
    if (ret != LE_OK)
    {
        if (ret == LE_NOT_FOUND)
        {
            extDataRecPtr->dtc = dtc;
            extDataRecPtr->status = 0;
            return LE_OK;
        }
        else
        {
            LE_ERROR("Failed to read status by dtc0x%x. ret=%d",
                dtc, (int32_t)ret);
            return ret;
        }
    }

    extDataPtr = (taf_DataAccess_ExtData_t*)le_mem_ForceAlloc(extDataPool);
    memset(extDataPtr, 0, sizeof(taf_DataAccess_ExtData_t));

    extDataPtr->recNumber = 1;
    extDataPtr->extDataSize  = 1;
    extDataPtr->extData[0] = static_cast<uint8_t>(occurCounter & 0xFF);
    extDataPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&extDataRecPtr->extDataList, &extDataPtr->link);

    extDataRecPtr->dtc = dtc;
    extDataRecPtr->status = static_cast<uint8_t>(status & 0xFF);

    return LE_OK;
}

le_result_t DemDataHandler::GetFaultDetCounter
(
    taf_DataAccess_FDCInfoRec_t *fdcInfoRecPtr
)
{
    auto &tafDtcDao = DtcEntityDao::GetInstance();

    le_result_t ret = tafDtcDao.ReadFaultDetectionCounter(fdcInfoRecPtr);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get FDC. ret=%d", (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

le_result_t DemDataHandler::SetEventStatus
(
    uint16_t eventId,
    uint8_t status
)
{
    uint32_t dtc = 0;
    le_result_t ret;

    auto &tafEventDao = EventEntityDao::GetInstance();

    ret = tafEventDao.WriteStatusAndDtcByEventId(eventId,
                                                static_cast<int32_t>(status),
                                                static_cast<int32_t>(dtc));
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set event0x%x with status0x%x in EVENT DAO. ret=%d",
            eventId, status, (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

uint8_t DemDataHandler::GetEventStatus
(
    uint16_t eventId
)
{
    auto &tafEventDao = EventEntityDao::GetInstance();

    return static_cast<uint8_t>(tafEventDao.ReadEventStatusByEventId
            (static_cast<int32_t>(eventId)));
}

le_result_t DemDataHandler::SetDTCStatus
(
    uint32_t dtc,
    uint8_t status,
    uint8_t occurrenceCounter
)
{
    le_result_t ret;

    // Check if the DTC is supported from configuration module.
    // DEM will check it.

    auto &tafDtcDao = DtcEntityDao::GetInstance();

    ret = tafDtcDao.WriteStatusByDtc(static_cast<int32_t>(dtc),
                                    static_cast<int32_t>(status),
                                    static_cast<int32_t>(occurrenceCounter));
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set dtc0x%x status0x%x in DTC DAO. ret=%d",
            dtc, status, (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

uint8_t DemDataHandler::GetDTCStatus
(
    uint32_t dtc
)
{
    le_result_t ret;
    int32_t storeStatus;

    // Check if the DTC is supported from configuration module.
    // DEM will check it.

    auto &tafDtcDao = DtcEntityDao::GetInstance();

    ret = tafDtcDao.ReadStatusByDtc(static_cast<int32_t>(dtc), storeStatus);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to read status by dtc0x%x, use 0. ret=%d",
            dtc, (int32_t)ret);
        return 0;
    }

    return static_cast<uint8_t>(storeStatus & 0xFF);
}

uint8_t DemDataHandler::GetDTCOccurrenceCounter
(
    uint32_t dtc
)
{
    le_result_t ret;
    int32_t storeStatus;
    int32_t occurCounter;

    // Check if the DTC is supported from configuration module.

    auto &tafDtcDao = DtcEntityDao::GetInstance();

    ret = tafDtcDao.ReadStatusByDtc(static_cast<int32_t>(dtc), storeStatus, occurCounter);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to read occurrence counter by dtc0x%x. ret=%d",
            dtc, (int32_t)ret);
        return 0;
    }

    return static_cast<uint8_t>(occurCounter & 0xFF);
}

le_result_t DemDataHandler::SetEventFailedCounter
(
    uint16_t eventId,
    uint8_t failedCounter
)
{
    le_result_t ret;
    auto &tafEventDao = EventEntityDao::GetInstance();

    ret = tafEventDao.WriteFailedCounterByEventId(static_cast<int32_t>(eventId),
                                                static_cast<int32_t>(failedCounter));
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set event0x%x with failedCounter0x%x in EVENT DAO. ret=%d",
            eventId, failedCounter, (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

uint8_t DemDataHandler::GetEventFailedCounter
(
    uint16_t eventId
)
{
    auto &tafEventDao = EventEntityDao::GetInstance();

    return static_cast<uint8_t>(tafEventDao.ReadFailedCounterByEventId
            (static_cast<int32_t>(eventId)));
}

le_result_t DemDataHandler::ResetAllData
(
)
{
    le_result_t ret;
    auto &tafDtcDao = DtcEntityDao::GetInstance();
    auto &tafEventDao = EventEntityDao::GetInstance();

    LE_DEBUG("Clear DTC table");
    ret = tafDtcDao.ClearDtcRecord();
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to delete all data of DTC. ret=%d", (int32_t)ret);
        return ret;
    }

    LE_DEBUG("Clear Event table");
    ret = tafEventDao.ClearEventRecord();
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to delete all data of EVENT. ret=%d", (int32_t)ret);
        return ret;
    }

    LE_DEBUG("Clear sanpshot table");
    auto &tafSnapshotDao = SnapshotEntityDao::GetInstance();
    ret = tafSnapshotDao.ClearFFRecord();
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to delete all snapshot of DTC. ret=%d", (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

le_result_t DemDataHandler::DeleteAllData
(
)
{
    le_result_t ret;
    auto &tafDtcDao = DtcEntityDao::GetInstance();
    std::vector<int32_t> dtcs = tafDtcDao.GetDTCWithUnsuppress();

    for (const auto &dtc : dtcs)
    {
        ret = DeleteData(static_cast<uint32_t>(dtc));
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to delete the data of DTC0x%x.", (uint32_t)dtc);
            return ret;
        }
    }

    return LE_OK;
}

le_result_t DemDataHandler::DeleteData
(
    uint32_t dtc
)
{
    le_result_t ret;
    auto &tafDtcDao = DtcEntityDao::GetInstance();
    auto &tafEventDao = EventEntityDao::GetInstance();

    if (GetSuppression(dtc) != 0)
    {
        LE_WARN("This DTC(0x%x) is under suppression", dtc);
        return LE_UNAVAILABLE;
    }

    ret = tafDtcDao.ClearDtcRecord(static_cast<int32_t>(dtc));
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to delete the data of DTC0x%x. ret=%d", dtc, (int32_t)ret);
        return ret;
    }

    try
    {
        vector<uint16_t> eid_list = cfg::get_event_ids(dtc);
        for (auto &eid : eid_list)
        {
            ret = tafEventDao.ClearEventRecord(static_cast<int32_t>(eid));
            if (ret != LE_OK)
            {
                LE_ERROR("Failed to delete all data of EVENT. ret=%d", (int32_t)ret);
                return ret;
            }
        }
    }
    catch (const std::exception& e)
    {
        // Not event in this DTC
        LE_WARN("Exception: %s", e.what() );
    }

    auto &tafSnapshotDao = SnapshotEntityDao::GetInstance();
    ret = tafSnapshotDao.ClearFFRecordByDtc(static_cast<int32_t>(dtc));
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to delete the snapshot of DTC0x%x. ret=%d", dtc, (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

uint8_t DemDataHandler::GetActivation
(
    uint32_t dtc
)
{
    le_result_t ret;
    int32_t activation;

    auto &tafDtcDao = DtcEntityDao::GetInstance();

    ret = tafDtcDao.ReadActivationByDtc(dtc, activation);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to read activation by dtc0x%x, use 1. ret=%d",
            dtc, (int32_t)ret);
        return 1;  // Default value is 1.
    }

    return static_cast<uint8_t>(activation & 0xFF);
}

le_result_t DemDataHandler::SetActivation
(
    uint32_t dtc,
    uint8_t activation
)
{
    le_result_t ret;

    auto &tafDtcDao = DtcEntityDao::GetInstance();

    ret = tafDtcDao.WriteActivationByDtc(static_cast<int32_t>(dtc),
        static_cast<int32_t>(activation));
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set dtc0x%x activation%x in DTC DAO. ret=%d",
            dtc, activation, (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

uint8_t DemDataHandler::GetSuppression
(
    uint32_t dtc
)
{
    le_result_t ret;
    int32_t suppression;

    auto &tafDtcDao = DtcEntityDao::GetInstance();

    ret = tafDtcDao.ReadSuppressionByDtc(dtc, suppression);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to read suppression by dtc0x%x, use 0. ret=%d",
            dtc, (int32_t)ret);
        return 0;  // Default value is 0.
    }

    return static_cast<uint8_t>(suppression & 0xFF);
}

le_result_t DemDataHandler::SetSuppression
(
    uint32_t dtc,
    uint8_t suppression
)
{
    le_result_t ret;

    auto &tafDtcDao = DtcEntityDao::GetInstance();

    ret = tafDtcDao.WriteSuppressionByDtc(static_cast<int32_t>(dtc),
        static_cast<int32_t>(suppression));
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set dtc0x%x suppression0x%x in DTC DAO. ret=%d",
            dtc, suppression, (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

le_result_t DemDataHandler::SetAllSuppression
(
    uint8_t suppression
)
{
    le_result_t ret;

    auto &tafDtcDao = DtcEntityDao::GetInstance();

    ret = tafDtcDao.WriteAllSuppression(static_cast<int32_t>(suppression));
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set all DTCs' suppression to 0x%x in DTC DAO. ret=%d",
            suppression, (int32_t)ret);
        return ret;
    }

    return LE_OK;
}

le_result_t DemDataHandler::GetDTCData
(
    uint32_t dtc,
    le_dls_List_t *list
)
{
    le_result_t ret;
    int32_t status;
    int32_t occurCounter;
    auto &tafDtcDao = DtcEntityDao::GetInstance();
    auto &tafSnapshotDao = SnapshotEntityDao::GetInstance();

    // Reset to empty.
    *list = LE_DLS_LIST_INIT;

    // Get extended datas of specified DTC.
    ret = tafDtcDao.ReadStatusByDtc(dtc, status, occurCounter);
    if (ret == LE_OK)
    {
        // Save the extended data record in the list. Only support OccurenceCounter.
        int rn;
        ret = GetExtendedDataRecordNumByType(dtc, ExtendData_OccurenceCounter, rn);
        if (ret == LE_OK)
        {
            taf_DataAccess_DtcDataInfo_t *dtcDataInfoPtr;
            dtcDataInfoPtr = (taf_DataAccess_DtcDataInfo_t*)le_mem_ForceAlloc(dtcDataInfoPool);
            memset(dtcDataInfoPtr, 0, sizeof(taf_DataAccess_DtcDataInfo_t));

            dtcDataInfoPtr->dtcCode = dtc;
            dtcDataInfoPtr->recordNum  = static_cast<uint8_t>(rn);
            dtcDataInfoPtr->occurrenceType = static_cast<uint8_t>(occurCounter & 0xFF);
            dtcDataInfoPtr->dataType = EXTENDED_DATA;
            dtcDataInfoPtr->dataId = 0;
            dtcDataInfoPtr->dataLen = 1;
            dtcDataInfoPtr->dataValue = static_cast<uint8_t*>(le_mem_ForceVarAlloc(
                recordDataPool, 1));
            dtcDataInfoPtr->dataValue[0] = static_cast<uint8_t>(occurCounter & 0xFF);
            dtcDataInfoPtr->link = LE_DLS_LINK_INIT;
            le_dls_Queue(list, &dtcDataInfoPtr->link);
        }
    }

    // Get snapshot datas of specified DTC.
    for (const auto& item : snapshotRnVec)
    {
        if (std::get<1>(item) != dtc)
        {
            continue;
        }

        std::vector<DIDInfoPtr> dids = tafSnapshotDao.GetDIDRecord(
                static_cast<int32_t>(dtc), static_cast<int32_t>(std::get<0>(item)));
        for (const auto& did : dids)
        {
            taf_DataAccess_DtcDataInfo_t *dtcDataInfoPtr;
            dtcDataInfoPtr = (taf_DataAccess_DtcDataInfo_t*)le_mem_ForceAlloc(dtcDataInfoPool);
            memset(dtcDataInfoPtr, 0, sizeof(taf_DataAccess_DtcDataInfo_t));

            dtcDataInfoPtr->dtcCode = dtc;
            dtcDataInfoPtr->recordNum  = static_cast<uint8_t>(std::get<0>(item));
            dtcDataInfoPtr->occurrenceType = 0;
            dtcDataInfoPtr->dataType = SNAPSHOT_DATA;
            dtcDataInfoPtr->dataId = did->did;
            dtcDataInfoPtr->dataLen = did->len;
            dtcDataInfoPtr->dataValue = static_cast<uint8_t*>(le_mem_ForceVarAlloc(
                recordDataPool, did->len));
            memcpy(dtcDataInfoPtr->dataValue, did->val, did->len);
            dtcDataInfoPtr->link = LE_DLS_LINK_INIT;
            le_dls_Queue(list, &dtcDataInfoPtr->link);
        }
    }

    return LE_OK;
}

void DemDataHandler::ReleaseDTCData
(
    le_dls_List_t *list
)
{
    taf_DataAccess_DtcDataInfo_t *dataPtr;
    le_dls_Link_t* linkPtr = le_dls_Pop(list);
    while (linkPtr != NULL)
    {
        dataPtr = CONTAINER_OF(linkPtr, taf_DataAccess_DtcDataInfo_t, link);
        if (dataPtr != NULL)
        {
            LE_DEBUG("Release DTC0x%x data", dataPtr->dtcCode);
            le_mem_Release(dataPtr->dataValue);
            le_mem_Release(dataPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(list);
    }
}

le_result_t DemDataHandler::SetSnapshotData
(
    uint32_t dtc,
    uint8_t triggerType,
    le_dls_List_t *list
)
{
    le_result_t ret;
    auto &tafSnapshotDao = SnapshotEntityDao::GetInstance();

    // Only support occurrence type.

    // Prepare DID datas for storage.
    std::vector<DIDInfoPtr> dids;
    taf_DataAccess_DidNode_t *nodePtr;
    le_dls_Link_t* linkPtr = le_dls_Peek(list);
    while (linkPtr != NULL)
    {
        nodePtr = CONTAINER_OF(linkPtr, taf_DataAccess_DidNode_t, link);
        if (nodePtr != NULL)
        {
            LE_DEBUG("Set snapshot: dtc0x%x, did0x%x, did size=%d",
                dtc, nodePtr->did, (int)nodePtr->len);
            DIDInfoPtr didPtr = std::make_shared<taf_DataAccess_DIDInfo_t>(
                static_cast<int32_t>(nodePtr->did),
                nodePtr->val,
                static_cast<int32_t>(nodePtr->len));
            dids.push_back(didPtr);
        }

        // Process next node.
        linkPtr = le_dls_PeekNext(list, linkPtr);
    }

    int firstRn, lastRn;
    ret = GetSnapshotDataRecordNumByType(dtc, Snapshot_LastOccurrence, lastRn);
    if (ret == LE_OK)
    {
        // Last occurrence is configured.
        LE_DEBUG("Get last record number%d for DTC0x%x successfully", lastRn, dtc);
        int32_t count = tafSnapshotDao.ReadDIDCount(static_cast<int32_t>(dtc),
                static_cast<int32_t>(lastRn));
        if (count != 0)
        {
            // last occurrence. Check if the list size is correct.
            if (count != static_cast<int32_t>(le_dls_NumLinks(list)))
            {
                LE_ERROR("The nodes is uncorrect. %d-%" PRIuS, count, le_dls_NumLinks(list));
                return LE_NOT_POSSIBLE;
            }

            // Only update last occurrence record in this situation
            return tafSnapshotDao.SetDIDRecord(static_cast<int32_t>(dtc),
                static_cast<int32_t>(lastRn), dids);
        }

        // First time to set the snapshot
        ret = GetSnapshotDataRecordNumByType(dtc, Snapshot_FirstOccurrence, firstRn);
        if (ret == LE_OK)
        {
            // Set the first occurrence record if it is configured in the yaml.
            count = tafSnapshotDao.ReadDIDCount(static_cast<int32_t>(dtc),
                    static_cast<int32_t>(firstRn));
            if (count != 0)
            {
                // Check if the list size is correct.
                if (count != static_cast<int32_t>(le_dls_NumLinks(list)))
                {
                    LE_ERROR("The nodes is uncorrect. %d-%" PRIuS, count, le_dls_NumLinks(list));
                    return LE_NOT_POSSIBLE;
                }
            }

            // First occurrence.
            ret = tafSnapshotDao.SetDIDRecord(static_cast<int32_t>(dtc),
                        static_cast<int32_t>(firstRn), dids);
            if (ret != LE_OK)
            {
                LE_ERROR("Failed to save DID record for DTC0x%x first occurrence", dtc);
                return LE_FAULT;
            }
        }

        ret = tafSnapshotDao.SetDIDRecord(static_cast<int32_t>(dtc),
                    static_cast<int32_t>(lastRn), dids);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to save DID record for first occurrence");
            return LE_FAULT;
        }
    }
    else
    {
        // Last occurrence is not configured.
        ret = GetSnapshotDataRecordNumByType(dtc, Snapshot_FirstOccurrence, firstRn);
        if (ret != LE_OK)
        {
            LE_INFO("No need to save snapshot for DTC0x%x", dtc);
            return LE_OK;
        }

        // Only configure to save first occurrence record.
        int32_t count = tafSnapshotDao.ReadDIDCount(static_cast<int32_t>(dtc),
                static_cast<int32_t>(firstRn));
        if (count != 0)
        {
            // Check if the list size is correct.
            if (count != static_cast<int32_t>(le_dls_NumLinks(list)))
            {
                LE_ERROR("The nodes is uncorrect. %d-%" PRIuS, count, le_dls_NumLinks(list));
                return LE_NOT_POSSIBLE;
            }
        }

        // Save to first occurrence.
        ret = tafSnapshotDao.SetDIDRecord(static_cast<int32_t>(dtc),
                    static_cast<int32_t>(firstRn), dids);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to save DID record for first occurrence");
            return LE_FAULT;
        }
    }

    return LE_OK;
}

le_result_t DemDataHandler::UpdateFaultCodeSnapshotData
(
    uint32_t dtc,
    taf_DataAccess_DidNode_t* node
)
{
    le_result_t ret;
    auto &tafSnapshotDao = SnapshotEntityDao::GetInstance();

    if(node == NULL || node->len == 0 || node->val == NULL)
    {
        LE_ERROR("Wrong node or supplier fault code data");
        return LE_BAD_PARAMETER;
    }

    // Prepare supplier fault code snapshot data for storage.
    std::vector<DIDInfoPtr> dids;

    LE_DEBUG("Set snapshot: dtc0x%x, did0x%x, did size=%d", dtc, node->did, (int)node->len);
    DIDInfoPtr didPtr = std::make_shared<taf_DataAccess_DIDInfo_t>( static_cast<int32_t>(node->did),
            node->val, static_cast<int32_t>(node->len));
    dids.push_back(didPtr);

    int firstRn, lastRn;
    ret = GetSnapshotDataRecordNumByType(dtc, Snapshot_LastOccurrence, lastRn);
    if (ret == LE_OK)
    {
        // Last occurrence is configured.
        LE_DEBUG("Get last record number%d for DTC0x%x successfully", lastRn, dtc);
        int32_t count = tafSnapshotDao.ReadDIDCount(static_cast<int32_t>(dtc),
                static_cast<int32_t>(lastRn));
        // Last occurrence record exists
        if (count != 0)
        {
            // Only update last occurrence record for supplier fault code in this situation
            return tafSnapshotDao.SetDIDRecord(static_cast<int32_t>(dtc),
                static_cast<int32_t>(lastRn), dids);
        }
        else
        {
            LE_ERROR("No snapshot data present");
            return LE_FAULT;
        }
    }
    else
    {
        // Last occurrence is not configured.
        ret = GetSnapshotDataRecordNumByType(dtc, Snapshot_FirstOccurrence, firstRn);
        if (ret != LE_OK)
        {
            LE_INFO("No need to save snapshot for DTC0x%x", dtc);
            return LE_OK;
        }

        // Only configure to save first occurrence record.
        int32_t count = tafSnapshotDao.ReadDIDCount(static_cast<int32_t>(dtc),
                static_cast<int32_t>(firstRn));
        if (count != 0)
        {
            // Save to first occurrence.
            ret = tafSnapshotDao.SetDIDRecord(static_cast<int32_t>(dtc),
                        static_cast<int32_t>(firstRn), dids);
            if (ret != LE_OK)
            {
                LE_ERROR("Failed to save DID record for first occurrence");
                return LE_FAULT;
            }
        }
        else
        {
            LE_ERROR("No snapshot data present");
            return LE_FAULT;
        }
    }

    return LE_OK;
}

le_result_t DemDataHandler::GetExtendedDataTypeByRecordNum
(
    uint32_t dtc,
    int rn,
    ExtendData_Type_t &type
)
{
    bool found = false;

    for (const auto& item : extendedRnVec)
    {
        if (std::get<1>(item) == dtc && std::get<0>(item) == rn)
        {
            found = true;
            type = std::get<2>(item);
            break;
        }
    }

    if (!found)
    {
        LE_WARN("Unknowd exended data record number(%d) for DTC0x%x.", rn, dtc);
        return LE_NOT_FOUND;
    }

    return LE_OK;
}

le_result_t DemDataHandler::GetExtendedDataRecordNumByType
(
    uint32_t dtc,
    ExtendData_Type_t type,
    int &rn
)
{
    bool found = false;

    for (const auto& item : extendedRnVec)
    {
        if (std::get<1>(item) == dtc && std::get<2>(item) == type)
        {
            found = true;
            rn = std::get<0>(item);
            break;
        }
    }

    if (!found)
    {
        LE_WARN("Unknowd exended data type(%d) for DTC0x%x.", (int)type, dtc);
        return LE_NOT_FOUND;
    }

    return LE_OK;
}

le_result_t DemDataHandler::GetSnapshotDataTypeByRecordNum
(
    uint32_t dtc,
    int rn,
    Snapshot_Type_t &type
)
{
    bool found = false;

    for (const auto& item : snapshotRnVec)
    {
        if (std::get<1>(item) == dtc && std::get<0>(item) == rn)
        {
            found = true;
            type = std::get<2>(item);
            break;
        }
    }

    if (!found)
    {
        LE_WARN("Unknowd snapshot record number(%d) for DTC0x%x.", rn, dtc);
        return LE_NOT_FOUND;
    }

    return LE_OK;
}

le_result_t DemDataHandler::GetSnapshotDataRecordNumByType
(
    uint32_t dtc,
    Snapshot_Type_t type,
    int &rn
)
{
    bool found = false;

    for (const auto& item : snapshotRnVec)
    {
        if (std::get<1>(item) == dtc && std::get<2>(item) == type)
        {
            found = true;
            rn = std::get<0>(item);
            break;
        }
    }

    if (!found)
    {
        LE_WARN("Unknowd snapshot data type(%d) for DTC0x%x.", (int)type, dtc);
        return LE_NOT_FOUND;
    }

    return LE_OK;
}

le_result_t DemDataHandler::GetAllSnapshotRecByDtc
(
    uint32_t    dtc,
    le_dls_List_t *list
)
{
    for (const auto& item : snapshotRnVec)
    {
        if (std::get<1>(item) != dtc)
        {
            continue;
        }

        GetSpecSnapshotRecByDtc(dtc, static_cast<uint8_t>(std::get<0>(item)), list);
    }

    return LE_OK;
}

le_result_t DemDataHandler::GetSpecSnapshotRecByDtc
(
    uint32_t    dtc,
    uint8_t     recNumber,
    le_dls_List_t *list
)
{
    auto &snapshotDao = SnapshotEntityDao::GetInstance();

    std::vector<DIDInfoPtr> dids = snapshotDao.GetDIDRecord(
        static_cast<int32_t>(dtc), static_cast<int32_t>(recNumber));
    if (dids.size() == 0)
    {
        // Not record.
        return LE_NOT_FOUND;
    }

    taf_DataAccess_SnapshotData_t *ffPtr;
    ffPtr = (taf_DataAccess_SnapshotData_t*)le_mem_ForceAlloc(freezeFramePool);
    memset(ffPtr, 0, sizeof(taf_DataAccess_SnapshotData_t));

    ffPtr->recNumber = recNumber;
    if (dids.size() > DATA_ACCESS_FF_DID_SIZE_MAX)
    {
        LE_WARN("DID set(0x%x) is larger than container(0x%x)",
            (int)dids.size(), (int)DATA_ACCESS_FF_DID_SIZE_MAX);
        ffPtr->recDidSize = DATA_ACCESS_FF_DID_SIZE_MAX;
    }
    else
    {
        ffPtr->recDidSize = static_cast<uint8_t>(dids.size());
    }

    LE_DEBUG("Read DID size=%" PRIuS " for DTC=0x%x, RN=0x%x",
        dids.size(), dtc, recNumber);

    int i = 0;
    for (const auto& did : dids)
    {
        ffPtr->didSet[i].did = static_cast<uint16_t>(did->did);

        if (did->len > DATA_ACCESS_DID_DATA_SIZE_MAX)
        {
            ffPtr->didSet[i].didDataSize = DATA_ACCESS_DID_DATA_SIZE_MAX;
            memcpy(ffPtr->didSet[i].didData, did->val, DATA_ACCESS_DID_DATA_SIZE_MAX);
        }
        else
        {
            ffPtr->didSet[i].didDataSize = static_cast<uint8_t>(did->len);
            memcpy(ffPtr->didSet[i].didData, did->val, did->len);
        }
        i++;
    }
    le_dls_Queue(list, &ffPtr->link);

    return LE_OK;
}

bool DemDataHandler::IsSnapshotDataRecordNumSupported
(
    uint32_t dtc,
    uint8_t rn
)
{
    bool found = false;

    for (const auto& item : snapshotRnVec)
    {
        if (std::get<1>(item) == dtc && std::get<0>(item) == (int)rn)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        LE_WARN("Unknowd snapshot record number(%d) for DTC0x%x.", (int)rn, dtc);
        return false;
    }

    return true;
}

bool DemDataHandler::IsExtendedDataRecordNumSupported
(
    uint32_t dtc,
    uint8_t rn
)
{
    bool found = false;

    for (const auto& item : extendedRnVec)
    {
        if (std::get<1>(item) == dtc && std::get<0>(item) == (int)rn)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        LE_WARN("Unknowd exended data record number(%d) for DTC0x%x.", (int)rn, dtc);
        return false;
    }

    return true;
}
