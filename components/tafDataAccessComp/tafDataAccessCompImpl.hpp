/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
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

    #ifdef LE_CONFIG_DIAG_FEATURE_A
        #define DEM_DTC_FORMAT_IDENTIFIER_DEF   DTC_FORMAT_IDENTIFIER_SAE_J2012_04
    #else
        #define DEM_DTC_FORMAT_IDENTIFIER_DEF   DTC_FORMAT_IDENTIFIER_ISO_14229
    #endif

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
            le_result_t Load();

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
            uint8_t GetEventFailedCounter(uint16_t eventId);
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
            le_result_t UpdateFaultCodeSnapshotData(uint32_t dtc, taf_DataAccess_DidNode_t* node);
        private:
            int mRecNum = 0;
            int mRecMax;
            le_mem_PoolRef_t extDataPool;
            le_mem_PoolRef_t supportedDtcPool;

            le_mem_PoolRef_t dtcDataInfoPool;
            le_mem_PoolRef_t didNodePool;
            le_mem_PoolRef_t recordDataPool;

            le_mem_PoolRef_t freezeFramePool;

            // Extended record number vector: <record number, dtc, type>.
            std::vector<std::tuple<int, uint32_t, ExtendData_Type_t>> extendedRnVec;

            // Snapshot record number vector: <record number, dtc, type>.
            std::vector<std::tuple<int, uint32_t, Snapshot_Type_t>> snapshotRnVec;

            le_result_t GetExtendedDataTypeByRecordNum(
                    uint32_t dtc, int rn, ExtendData_Type_t &type);
            le_result_t GetExtendedDataRecordNumByType(
                    uint32_t dtc, ExtendData_Type_t type, int &rn);
            le_result_t GetSnapshotDataTypeByRecordNum(
                    uint32_t dtc, int rn, Snapshot_Type_t &type);
            le_result_t GetSnapshotDataRecordNumByType(
                    uint32_t dtc, Snapshot_Type_t type, int &rn);
            le_result_t GetAllSnapshotRecByDtc(uint32_t        dtc, le_dls_List_t *list);
            le_result_t GetSpecSnapshotRecByDtc(
                    uint32_t dtc, uint8_t recNumber, le_dls_List_t *list);
            bool IsSnapshotDataRecordNumSupported(uint32_t dtc, uint8_t rn);
            bool IsExtendedDataRecordNumSupported(uint32_t dtc, uint8_t rn);
    };
}
}
#endif
