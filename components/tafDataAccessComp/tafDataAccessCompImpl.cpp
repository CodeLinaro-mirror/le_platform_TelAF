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

#include "tafDataAccessComp.h"
#include "tafDTCEntityDAO.hpp"
#include "tafEventEntityDAO.hpp"
#include "tafDataAccessCompImpl.hpp"
#include "configuration.hpp"

using namespace telux::tafsvc;
using namespace std;

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
    tafDtcDao.Init(DEM_DATABASE_NAME);

    auto &tafEventDao = EventEntityDao::GetInstance();
    tafEventDao.Init(DEM_DATABASE_NAME);

    extDataPool = le_mem_CreatePool("ExtDataPool", sizeof(taf_DataAccess_ExtData_t));
    supportedDtcPool = le_mem_CreatePool("suppDtcPool", sizeof(taf_DataAccess_DTCStatus_t));
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

    int32_t count = tafDtcDao.ReadDtcCountByStatus(static_cast<int32_t>(statusMask));

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
    int32_t status;
    le_result_t ret;
    taf_DataAccess_DTCStatus_t *dtcStaPtr;
    std::vector<uint32_t> dtc_code_list = cfg::get_dtc_codes();
    auto &tafDtcDao = DtcEntityDao::GetInstance();

    dtcStatusPtr->dtcStatusRecList = LE_DLS_LIST_INIT;
    for (const auto & dtc_code: dtc_code_list)
    {
        ret = tafDtcDao.ReadStatusByDtc(dtc_code, status);
        if (ret != LE_OK)
        {
            // Not found or other error.
            status = 0;
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

    return LE_OK;
}

le_result_t DemDataHandler::GetSnapshotIdentification
(
    taf_DataAccess_SnapshotInfoRec_t *snapshotRecInfoPtr
)
{
    return LE_NOT_IMPLEMENTED;
}

le_result_t DemDataHandler::GetSnapshotRecByDtc
(
    uint32_t    dtc,
    uint8_t     recNumber,
    taf_DataAccess_SnapshotDataRec_t *snapshotDataRecPtr
)
{
    return LE_NOT_IMPLEMENTED;
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

le_result_t DemDataHandler::DeleteAllData
(
)
{
    le_result_t ret;
    auto &tafDtcDao = DtcEntityDao::GetInstance();
    auto &tafEventDao = EventEntityDao::GetInstance();

    ret = tafDtcDao.ClearDtcRecord();
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to delete all data of DTC. ret=%d", (int32_t)ret);
        return ret;
    }

    ret = tafEventDao.ClearEventRecord();
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to delete all data of EVENT. ret=%d", (int32_t)ret);
        return ret;
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
        ret = tafEventDao.ClearEventRecord(static_cast<int32_t>(eid));
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to delete all data of EVENT. ret=%d", (int32_t)ret);
            return ret;
        }
    }
    catch (const std::exception& e)
    {
        // Not event in this DTC
        LE_WARN("Exception: %s", e.what() );
    }

    return LE_OK;
}
