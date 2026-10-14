/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_DATA_ACCESS_COMP_H
#define TAF_DATA_ACCESS_COMP_H

#include "legato.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define DID_OF_SUPPLIER_FC 0xEF01

typedef enum
{
    DTC_FORMAT_IDENTIFIER_SAE_J2012_00 = 0,
    DTC_FORMAT_IDENTIFIER_ISO_14229,
    DTC_FORMAT_IDENTIFIER_SAE_J1939,
    DTC_FORMAT_IDENTIFIER_ISO_11992,
    DTC_FORMAT_IDENTIFIER_SAE_J2012_04
}taf_DataAccess_DTC_Format_t;

typedef struct
{
    uint8_t     availableMask;  // User configuration mask
    uint8_t     formatIdentifier;
    uint16_t    dtcCnt;
}taf_DataAccess_NumOfDTC_t;

typedef struct
{
    uint32_t    dtc;
    uint8_t     status;
    le_dls_Link_t link;
}taf_DataAccess_DTCStatus_t;

typedef struct
{
    uint8_t     availableMask;  // User configuration mask
    le_dls_List_t dtcStatusRecList;
}taf_DataAccess_DTCStatusRec_t;

typedef struct
{
    uint32_t    dtc;
    uint8_t     recNumber;
    le_dls_Link_t link;
}taf_DataAccess_SnapshotInfo_t;

typedef struct
{
    le_dls_List_t snapshotInfoList;
}taf_DataAccess_SnapshotInfoRec_t;

// Max number of DID in one freeze frame
#define DATA_ACCESS_FF_DID_SIZE_MAX     10
#define DATA_ACCESS_DID_DATA_SIZE_MAX    128
typedef struct
{
    uint16_t    did;
    uint8_t     didDataSize;
    uint8_t     didData[DATA_ACCESS_DID_DATA_SIZE_MAX];
}taf_DataAccess_FF_DID_t;

typedef struct
{
    uint8_t     recNumber;
    uint8_t     recDidSize;
    taf_DataAccess_FF_DID_t didSet[DATA_ACCESS_FF_DID_SIZE_MAX];
    le_dls_Link_t link;
}taf_DataAccess_SnapshotData_t;

typedef struct
{
    uint32_t    dtc;
    uint8_t     status;
    le_dls_List_t snapshotDataList;
}taf_DataAccess_SnapshotDataRec_t;

// Max number of bytes in one extended data record
#define DATA_ACCESS_EXT_DATA_SIZE_MAX   128
typedef struct
{
    uint8_t     recNumber;
    uint16_t    extDataSize;
    uint8_t     extData[DATA_ACCESS_EXT_DATA_SIZE_MAX];
    le_dls_Link_t link;
}taf_DataAccess_ExtData_t;

typedef struct
{
    uint32_t    dtc;
    uint8_t     status;
    le_dls_List_t extDataList;
}taf_DataAccess_ExtDataRec_t;

typedef struct
{
    uint16_t    eventId;
    uint32_t    dtc;
    uint8_t     status;
    le_dls_Link_t link;
}taf_DataAccess_EventInfo_t;

typedef struct
{
    le_dls_List_t   eventList;
}taf_DataAccess_EventInfoRec_t;

//-------------------------------------------------------------------------------------------------
/**
 * Initialize Data Access component.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_Init
(
);

//-------------------------------------------------------------------------------------------------
/**
 * Get the number of DTCs by the input status mask.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_GetNumOfDtcByStatusMask
(
    uint8_t statusMask,                     ///< [IN]
    taf_DataAccess_NumOfDTC_t *numOfDtcPtr  ///< [Out]
);

//-------------------------------------------------------------------------------------------------
/**
 * Get the available DTC status mask.
 *
 * @return
 *  - The available DTC status mask.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED uint8_t taf_DataAccess_GetAvailableStatusMask
(
);

//-------------------------------------------------------------------------------------------------
/**
 * Get the DTC format identifier.
 *
 * @return
 *  - DTC format identifier.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED uint8_t taf_DataAccess_GetDtcFormatId
(
);

//-------------------------------------------------------------------------------------------------
/**
 * Get the DTCs by status mask.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_GetDtcByStatusMask
(
    uint8_t statusMask,                         ///< [IN]
    taf_DataAccess_DTCStatusRec_t *dtcStatusPtr ///< [Out]
);

//-------------------------------------------------------------------------------------------------
/**
 * Get the supported DTCs.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_GetSupportedDtc
(
    taf_DataAccess_DTCStatusRec_t *dtcStatusPtr ///< [Out]
);

//-------------------------------------------------------------------------------------------------
/**
 * Get the freeze frame(snapshot) information.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_GetSnapshotIdentification
(
    taf_DataAccess_SnapshotInfoRec_t *snapshotRecInfoPtr ///< [Out]
);

//-------------------------------------------------------------------------------------------------
/**
 * Get the freeze frame(snapshot) record based on DTC.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_GetSnapshotRecByDtc
(
    uint32_t    dtc,    ///< [IN]
    uint8_t     recNumber,  ///< [IN]
    taf_DataAccess_SnapshotDataRec_t *snapshotDataRecPtr ///< [Out]
);

//-------------------------------------------------------------------------------------------------
/**
 * Get the extended data record based on DTC.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_GetExtDataRecByDtc
(
    uint32_t    dtc,    ///< [IN]
    uint8_t     recNumber,  ///< [IN]
    taf_DataAccess_ExtDataRec_t *extDataRecPtr ///< [Out]
);

//-------------------------------------------------------------------------------------------------
/**
 * Set event status and save it in storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_SetEventStatus
(
    uint16_t eventId,    ///< [IN]
    uint8_t status       ///< [IN]
);

//-------------------------------------------------------------------------------------------------
/**
 * Set event status by event name and save it in storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_SetEventStatusByName
(
    const char *eventName,  ///< [IN]
    uint8_t status          ///< [IN]
);

//-------------------------------------------------------------------------------------------------
/**
 * Get event status from storage media.
 *
 * @return
 *  - The status of the event. If not exist, will return 0.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED uint8_t taf_DataAccess_GetEventStatus
(
    uint16_t eventId     ///< [IN]
);

//-------------------------------------------------------------------------------------------------
/**
 * Get event status from storage media by event name.
 *
 * @return
 *  - The status of the event. If not exist, will return 0.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED uint8_t taf_DataAccess_GetEventStatusByName
(
    const char *eventName   ///< [IN]
);

//-------------------------------------------------------------------------------------------------
/**
 * Set DTC status and save it in storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_SetDTCStatus
(
    uint32_t dtc,               ///< [IN]
    uint8_t status,             ///< [IN]
    uint8_t occurrenceCounter   ///< [IN]
);

//-------------------------------------------------------------------------------------------------
/**
 * Get DTC status from storage media.
 *
 * @return
 *  - The status of the DTC. If not exist, will return 0.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED uint8_t taf_DataAccess_GetDTCStatus
(
    uint32_t dtc        ///< [IN]
);

//-------------------------------------------------------------------------------------------------
/**
 * Get DTC occurence counter from storage media.
 *
 * @return
 *  - The occurence counter of the DTC. If not exist, will return 0.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED uint8_t taf_DataAccess_GetDTCOccurrenceCounter
(
    uint32_t dtc        ///< [IN]
);

//-------------------------------------------------------------------------------------------------
/**
 * Set event test failed counter and save it in storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_SetEventFailedCounter
(
    uint16_t eventId,       ///< [IN]
    uint8_t failedCounter   ///< [IN]
);

//-------------------------------------------------------------------------------------------------
/**
 * Set event test failed counter and save it in storage media.
 *
 * @return
 *  - The failed counter of the event. If not exist, will return 0.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED uint8_t taf_DataAccess_GetEventFailedCounter
(
    uint16_t eventId        ///< [IN]
);

//-------------------------------------------------------------------------------------------------
/**
 * Delete all DTC and Event data from storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_DeleteAllData
(
);

//-------------------------------------------------------------------------------------------------
/**
 * Delete the DTC data from storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_DeleteData
(
    uint32_t dtc
);

// newly increased
//-------------------------------------------------------------------------------------------------
/**
 * Get the DTC activation status from storage media.
 *
 * @return
 *  - The status of the DTC activation. If not exist, will return 1.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED uint8_t taf_DataAccess_GetDTCActivation
(
    uint32_t dtc        ///< [IN]
);

//-------------------------------------------------------------------------------------------------
/**
 * Set the DTC activation status into storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_SetDTCActivation
(
    uint32_t dtc,               ///< [IN]
    uint8_t activationStatus    ///< [IN]
);

//-------------------------------------------------------------------------------------------------
/**
 * Get the DTC suppression status from storage media.
 *
 * @return
 *  - The status of the DTC suppression. If not exist, will return 0.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED uint8_t taf_DataAccess_GetDTCSuppression
(
    uint32_t dtc        ///< [IN]
);

//-------------------------------------------------------------------------------------------------
/**
 * Set the DTC suppression status into storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_SetDTCSuppression
(
    uint32_t dtc,               ///< [IN]
    uint8_t suppressionStatus   ///< [IN]
);

typedef enum {
    SNAPSHOT_DATA = 0x00,              ///< Snapshot data.
    EXTENDED_DATA                      ///< Extended data.
}taf_DataAccess_DataType_t;

typedef struct
{
    uint32_t dtcCode;
    uint8_t recordNum;
    uint8_t occurrenceType;  // Not used.
    taf_DataAccess_DataType_t dataType;
    uint16_t dataId;  // 0 if dataType is extended data.
    uint16_t dataLen;
    uint8_t *dataValue;
    le_dls_Link_t link;
}taf_DataAccess_DtcDataInfo_t;

//-------------------------------------------------------------------------------------------------
/**
 * Get all the datas of the specified DTC.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_GetDTCData
(
    uint32_t dtc,       ///< [IN]
    le_dls_List_t *list ///< [OUT]
);

//-------------------------------------------------------------------------------------------------
/**
 * Release the data resources.
 *
 * @return
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED void taf_DataAccess_ReleaseDTCData
(
    le_dls_List_t *list
);

typedef struct {
    uint16_t did;
    uint8_t *val;
    uint16_t len;
    le_dls_Link_t link;
}taf_DataAccess_DidNode_t;

//-------------------------------------------------------------------------------------------------
/**
 * Save the snapshot data into storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_SetSnapshotData
(
    uint32_t dtc,
    le_dls_List_t *list,
    void *action
);

//-------------------------------------------------------------------------------------------------
/**
 * Update the fault code snapshot data into storage media.
 *
 * @return
 *  - LE_OK                  Funtion success.
 *  - LE_BAD_PARAMETER       Bad parameter.
 *  - LE_FAULT               Failed
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_UpdateFaultCodeSnapshotData
(
    uint32_t dtc,
    taf_DataAccess_DidNode_t* node
);

//-------------------------------------------------------------------------------------------------
/**
 * Save the suppression status of all DTCs into storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_SetAllDTCSuppression
(
    uint8_t suppressionStatus
);

//-------------------------------------------------------------------------------------------------
/**
 * clear up all the DTC datas in the storage media. Include suppressional DTCs.
 * Currently, this function only uses for test.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DataAccess_ResetDTCStorage
(
);

#ifdef  __cplusplus
}
#endif

#endif  // TAF_DATA_ACCESS_COMP_H