/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDataAccessComp.h"
#include "tafIOFactory.hpp"
#include "tafDataAccessCompImpl.hpp"

using namespace taf::dataAccess;

//--------------------------------------------------------------------------------------------------
/**
 * Component once initializer.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT_ONCE
{
    LE_INFO("tafDataAccessComp Init...");

    auto &factory = IOFactory::GetInstance();
    factory.Init();

    auto &demHandler = DemDataHandler::GetInstance();
    demHandler.Init();

    LE_INFO("tafDataAccessComp Ready...");
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization function
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_DEBUG("DataAccess component initializing");
}

//-------------------------------------------------------------------------------------------------
/**
 * Initialize Data Access component.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_Init
(
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.Load();
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the number of DTCs by the input status mask.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_GetNumOfDtcByStatusMask
(
    uint8_t statusMask,                     ///< [IN]
    taf_DataAccess_NumOfDTC_t *numOfDtcPtr  ///< [Out]
)
{
    if (numOfDtcPtr == nullptr)
    {
        LE_ERROR("GetNumOfDtcByStatusMask error: Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetNumOfDtcByStatusMask(statusMask, numOfDtcPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the available DTC status mask.
 *
 * @return
 *  - The available DTC status mask.
 */
//-------------------------------------------------------------------------------------------------
uint8_t taf_DataAccess_GetAvailableStatusMask
(
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetAvailableStatusMask();
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the DTC format identifier.
 *
 * @return
 *  - DTC format identifier.
 */
//-------------------------------------------------------------------------------------------------
uint8_t taf_DataAccess_GetDtcFormatId
(
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetDtcFormatId();
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the DTCs by status mask.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_GetDtcByStatusMask
(
    uint8_t statusMask,                         ///< [IN]
    taf_DataAccess_DTCStatusRec_t *dtcStatusPtr ///< [Out]
)
{
    if (dtcStatusPtr == nullptr)
    {
        LE_ERROR("GetDtcByStatusMask error: Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetDtcByStatusMask(statusMask, dtcStatusPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the supported DTCs.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_GetSupportedDtc
(
    taf_DataAccess_DTCStatusRec_t *dtcStatusPtr ///< [Out]
)
{
    if (dtcStatusPtr == nullptr)
    {
        LE_ERROR("GetSupportedDtc error: Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetSupportedDtc(dtcStatusPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the freeze frame(snapshot) information.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_GetSnapshotIdentification
(
    taf_DataAccess_SnapshotInfoRec_t *snapshotRecInfoPtr ///< [Out]
)
{
    if (snapshotRecInfoPtr == nullptr)
    {
        LE_ERROR("GetSnapshotIdentification error: Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetSnapshotIdentification(snapshotRecInfoPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the freeze frame(snapshot) record based on DTC.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_GetSnapshotRecByDtc
(
    uint32_t    dtc,    ///< [IN]
    uint8_t     recNumber,  ///< [IN]
    taf_DataAccess_SnapshotDataRec_t *snapshotDataRecPtr ///< [Out]
)
{
    if (snapshotDataRecPtr == nullptr)
    {
        LE_ERROR("GetSnapshotRecByDtc error: Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetSnapshotRecByDtc(dtc, recNumber, snapshotDataRecPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the extended data record based on DTC.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_GetExtDataRecByDtc
(
    uint32_t    dtc,    ///< [IN]
    uint8_t     recNumber,  ///< [IN]
    taf_DataAccess_ExtDataRec_t *extDataRecPtr ///< [Out]
)
{
    if (extDataRecPtr == nullptr)
    {
        LE_ERROR("GetExtDataRecByDtc error: Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetExtDataRecByDtc(dtc, recNumber, extDataRecPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * Set event status and save it in storage.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_SetEventStatus
(
    uint16_t eventId,    ///< [IN]
    uint8_t status       ///< [IN]
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.SetEventStatus(eventId, status);
}

//-------------------------------------------------------------------------------------------------
/**
 * Get event status from storage media.
 *
 * @return
 *  - The status of the event. If not exist, will return 0.
 */
//-------------------------------------------------------------------------------------------------
uint8_t taf_DataAccess_GetEventStatus
(
    uint16_t eventId     ///< [IN]
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetEventStatus(eventId);
}

//-------------------------------------------------------------------------------------------------
/**
 * Set DTC status and save it in storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_SetDTCStatus
(
    uint32_t dtc,               ///< [IN]
    uint8_t status,             ///< [IN]
    uint8_t occurrenceCounter   ///< [IN]
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.SetDTCStatus(dtc, status, occurrenceCounter);
}

//-------------------------------------------------------------------------------------------------
/**
 * Get DTC status from storage media.
 *
 * @return
 *  - The status of the DTC. If not exist, will return 0.
 */
//-------------------------------------------------------------------------------------------------
uint8_t taf_DataAccess_GetDTCStatus
(
    uint32_t dtc        ///< [IN]
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetDTCStatus(dtc);
}

//-------------------------------------------------------------------------------------------------
/**
 * Get DTC occurence counter from storage media.
 *
 * @return
 *  - The occurence counter of the DTC. If not exist, will return 0.
 */
//-------------------------------------------------------------------------------------------------
uint8_t taf_DataAccess_GetDTCOccurrenceCounter
(
    uint32_t dtc        ///< [IN]
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetDTCOccurrenceCounter(dtc);
}

//-------------------------------------------------------------------------------------------------
/**
 * Set event test failed counter and save it in storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_SetEventFailedCounter
(
    uint16_t eventId,       ///< [IN]
    uint8_t failedCounter   ///< [IN]
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.SetEventFailedCounter(eventId, failedCounter);
}

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
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetEventFailedCounter(eventId);
}

//-------------------------------------------------------------------------------------------------
/**
 * Delete all DTC and Event data from storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_DeleteAllData
(
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.DeleteAllData();
}

//-------------------------------------------------------------------------------------------------
/**
 * Delete the DTC data from storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_DeleteData
(
    uint32_t dtc
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.DeleteData(dtc);
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the DTC activation status from storage media.
 *
 * @return
 *  - The status of the DTC activation. If not exist, will return 1.
 */
//-------------------------------------------------------------------------------------------------
uint8_t taf_DataAccess_GetDTCActivation
(
    uint32_t dtc        ///< [IN]
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetActivation(dtc);
}

//-------------------------------------------------------------------------------------------------
/**
 * Set the DTC activation status into storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_SetDTCActivation
(
    uint32_t dtc,               ///< [IN]
    uint8_t activationStatus    ///< [IN]
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.SetActivation(dtc, activationStatus);
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the DTC suppression status from storage media.
 *
 * @return
 *  - The status of the DTC suppression. If not exist, will return 0.
 */
//-------------------------------------------------------------------------------------------------
uint8_t taf_DataAccess_GetDTCSuppression
(
    uint32_t dtc        ///< [IN]
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetSuppression(dtc);
}

//-------------------------------------------------------------------------------------------------
/**
 * Set the DTC suppression status into storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 *  - LE_BAD_PARAMETER  Bad parameter.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_SetDTCSuppression
(
    uint32_t dtc,               ///< [IN]
    uint8_t suppressionStatus   ///< [IN]
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    if (suppressionStatus != 0 && suppressionStatus != 1)
    {
        LE_ERROR("SetDTCSuppression error: Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    return demHandler.SetSuppression(dtc, suppressionStatus);
}

//-------------------------------------------------------------------------------------------------
/**
 * Get all the datas of the specified DTC.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_GetDTCData
(
    uint32_t dtc,       ///< [IN]
    le_dls_List_t *list ///< [OUT]
)
{
    if (list == nullptr)
    {
        LE_ERROR("taf_DataAccess_GetDTCData error: Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.GetDTCData(dtc, list);
}

//-------------------------------------------------------------------------------------------------
/**
 * Release the data resources.
 *
 * @return
 */
//-------------------------------------------------------------------------------------------------
void taf_DataAccess_ReleaseDTCData
(
    le_dls_List_t *list
)
{
    if (list == nullptr)
    {
        LE_ERROR("taf_DataAccess_ReleaseDTCData error: Bad parameter.");
        return;
    }

    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.ReleaseDTCData(list);
}

//-------------------------------------------------------------------------------------------------
/**
 * Save the snapshot data into storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_SetSnapshotData
(
    uint32_t dtc,
    le_dls_List_t *list,
    void *action
)
{
    if (list == nullptr)
    {
        LE_ERROR("taf_DataAccess_ReleaseDTCData error: Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    auto &demHandler = DemDataHandler::GetInstance();
    le_result_t ret = demHandler.SetSnapshotData(dtc, 0, list);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to save snapshot data, ret=%d.", (int)ret);
        return ret;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Update the snapshot data for supplier fault code into storage media.
 *
 * @return
 *  - LE_OK                  Funtion success.
 *  - LE_BAD_PARAMETER       Bad parameter.
 *  - LE_FAULT               Failed
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_UpdateFaultCodeSnapshotData
(
    uint32_t dtc,
    taf_DataAccess_DidNode_t* node
)
{
    if(node == NULL || node->len == 0 || node->val == NULL)
    {
        LE_ERROR("Wrong node or supplier fault code data");
        return LE_BAD_PARAMETER;
    }

    auto &demHandler = DemDataHandler::GetInstance();
    le_result_t ret = demHandler.UpdateFaultCodeSnapshotData(dtc, node);

    if (ret != LE_OK)
    {
        LE_ERROR("Failed to update fault code snapshot data, ret=%d.", (int)ret);
        return ret;
    }

    return LE_OK;
}
//-------------------------------------------------------------------------------------------------
/**
 * Save the suppression status of all DTCs into storage media.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_SetAllDTCSuppression
(
    uint8_t suppressionStatus
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    if (suppressionStatus != 0 && suppressionStatus != 1)
    {
        LE_ERROR("SetDTCSuppression error: Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    return demHandler.SetAllSuppression(suppressionStatus);
}

//-------------------------------------------------------------------------------------------------
/**
 * clear up all the DTC datas in the storage media. Include suppressional DTCs.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_IO_ERROR       IO operation failed.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DataAccess_ResetDTCStorage
(
)
{
    auto &demHandler = DemDataHandler::GetInstance();

    return demHandler.ResetAllData();
}
