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
#ifndef TAF_DATA_HANDLER_HPP
#define TAF_DATA_HANDLER_HPP

#include "legato.h"
#include "interfaces.h"

#include "tafDataAccessComp.h"
#include "tafBaseDAO.hpp"
#include "tafDataStatement.hpp"
#include "tafIOHandler.hpp"
#include "tafDTCEntityDAO.hpp"
#include <map>

namespace taf{
namespace dataAccess{
    #define DEM_DATABASE_DIR    "/data/diag/"
    #define DEM_DATABASE_NAME   DEM_DATABASE_DIR"dem.db"
    #define DEM_DATABASE_CONTEXT    "system_u:system_r:telaf_sys_t:s0-s15"
    #define DEM_DTC_AVAILABLE_MASK_DEF  0x7F
    #define DEM_DTC_FORMAT_IDENTIFIER_DEF   DTC_FORMAT_IDENTIFIER_ISO_14229

    #define DEM_RECORD_DATA_SIZE        2
    #define DEM_RECORD_DATA_BYTES       128
    #define DEM_RECORD_DATA_TYPICAL_SIZE    6
    #define DEM_RECORD_DATA_TYPICAL_BYTES   4

    constexpr int DEM_DB_VERSION = 1;

    typedef enum {
        ExtendData_OccurenceCounter = 0,
        ExtendData_CumulativeDistanceOCCWithTestFailed,
        ExtendData_IUMPRNumerator,
        ExtendData_IUMPRDenominator,
        ExtendData_All      // If record number is 0xFE or 0xFF.
    }ExtendData_Type_t;

    typedef enum {
        Snapshot_FirstOccurrence = 0,
        Snapshot_LastOccurrence,
        Snapshot_LastDisappearance,
        Snapshot_All        // If record number is 0xFF.
    }Snapshot_Type_t;

    class DemDataHandler {
        public:
            DemDataHandler();
            ~DemDataHandler();
            static DemDataHandler &GetInstance();

            void Init();

            le_result_t GetNumOfDtcByStatusMask(uint8_t statusMask,
                    taf_DataAccess_NumOfDTC_t *numOfDtcPtr);
            uint8_t GetAvailableStatusMask();
            uint8_t GetDtcFormatId();
            le_result_t GetDtcByStatusMask(uint8_t statusMask,
                    taf_DataAccess_DTCStatusRec_t *dtcStatusPtr);
            le_result_t GetSupportedDtc(taf_DataAccess_DTCStatusRec_t *dtcStatusPtr);
            le_result_t GetSnapshotIdentification(
                    taf_DataAccess_SnapshotInfoRec_t *snapshotRecInfoPtr);
            le_result_t GetSnapshotRecByDtc(uint32_t dtc, uint8_t recNumber,
                    taf_DataAccess_SnapshotDataRec_t *snapshotDataRecPtr);
            le_result_t GetExtDataRecByDtc(uint32_t dtc, uint8_t recNumber,
                    taf_DataAccess_ExtDataRec_t *extDataRecPtr);
            le_result_t GetFaultDetCounter(taf_DataAccess_FDCInfoRec_t *fdcInfoRecPtr);
            le_result_t SetEventStatus(uint16_t eventId, uint8_t status);
            uint8_t GetEventStatus(uint16_t eventId);
            le_result_t SetDTCStatus(uint32_t dtc, uint8_t status, uint8_t occurrenceCounter);
            uint8_t GetDTCStatus(uint32_t dtc);
            uint8_t GetDTCOccurrenceCounter(uint32_t dtc);
            le_result_t SetEventFailedCounter(uint16_t eventId, uint8_t failedCounter);
            le_result_t ResetAllData(); // Reset data storage.
            le_result_t DeleteAllData(); // Delete all unsuppressional DTCs.
            le_result_t DeleteData(uint32_t dtc);

            uint8_t GetActivation(uint32_t dtc);
            le_result_t SetActivation(uint32_t dtc, uint8_t activation);
            uint8_t GetSuppression(uint32_t dtc);
            le_result_t SetSuppression(uint32_t dtc, uint8_t suppression);
            le_result_t SetAllSuppression(uint8_t suppression);
            le_result_t GetDTCData(uint32_t dtc, le_dls_List_t *list);
            void ReleaseDTCData(le_dls_List_t *list);
            le_result_t SetSnapshotData(uint32_t dtc, uint8_t triggerType, le_dls_List_t *list);
        private:
            int mRecNum = 0;
            int mRecMax;
            le_mem_PoolRef_t extDataPool;
            le_mem_PoolRef_t supportedDtcPool;

            le_mem_PoolRef_t dtcDataInfoPool;
            le_mem_PoolRef_t didNodePool;
            le_mem_PoolRef_t recordDataPool;

            le_mem_PoolRef_t freezeFramePool;

            std::map<int,ExtendData_Type_t> extendedRnMap;   // Extended record number map.
            std::map<int, Snapshot_Type_t> snapshotRnMap;   // Snapshot record number map.

            le_result_t GetExtendedDataTypeByRecordNum(int rn, ExtendData_Type_t &type);
            le_result_t GetExtendedDataRecordNumByType(ExtendData_Type_t type, int &rn);
            le_result_t GetSnapshotDataTypeByRecordNum(int rn, Snapshot_Type_t &type);
            le_result_t GetSnapshotDataRecordNumByType(Snapshot_Type_t type, int &rn);
            le_result_t GetAllSnapshotRecByDtc(uint32_t        dtc, le_dls_List_t *list);
            le_result_t GetSpecSnapshotRecByDtc(uint32_t dtc, uint8_t recNumber, le_dls_List_t *list);
    };
}
}
#endif

