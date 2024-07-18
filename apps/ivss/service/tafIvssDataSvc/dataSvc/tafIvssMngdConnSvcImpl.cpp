/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssMngdConnSvc.hpp"

using namespace v0::com::qualcomm::qti::modem;

taf_IvssMngdConn_DataInfo_t DataTable[IVSS_MAX_DATA_NUM] = {};
taf_mngdConn_DataStateHandlerRef_t DataStateHandlerRef[IVSS_MAX_DATA_NUM] = {};
//--------------------------------------------------------------------------------------------------
/**
 * Get the single instance of TelAF Ivss MngdConn server.
 */
//--------------------------------------------------------------------------------------------------
std::shared_ptr<tafIvssMngdConnSvc> tafIvssMngdConnSvc::GetInstance()
{
    static std::shared_ptr<tafIvssMngdConnSvc> instance = std::make_shared<tafIvssMngdConnSvc>();
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'StartData'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssMngdConnSvc::StartDataHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssMngdConn_Ind_t* indPtr = (taf_IvssMngdConn_Ind_t*)reportPtr;

    taf_mngdConn_DataRef_t dataRef = taf_mngdConn_GetDataByName(indPtr->startData.name);
    if (dataRef == nullptr)
    {
        indPtr->result = LE_FAULT;
        le_sem_Post(indPtr->semRef);
        LE_ERROR("taf_mngdConn_GetDataByName failed - %s", LE_RESULT_TXT(indPtr->result));
        return;
    }

    indPtr->result = taf_mngdConn_StartData(dataRef);
    if (indPtr->result != LE_OK)
    {
        le_sem_Post(indPtr->semRef);
        LE_ERROR("taf_mngdConn_StartData failed - %s", LE_RESULT_TXT(indPtr->result));
        return;
    }

    uint32_t index;
    for (index = 0; index < IVSS_MAX_DATA_NUM; index++)
    {
        if (DataTable[index].enable == false)
        {
            indPtr->result = taf_mngdConn_GetDataConnectionState(dataRef, &DataTable[index].state);
            if (indPtr->result != LE_OK)
            {
                le_sem_Post(indPtr->semRef);
                LE_ERROR("taf_mngdConn_GetDataConnectionState failed - %s",
                    LE_RESULT_TXT(indPtr->result));
                return;
            }

            DataStateHandlerRef[index] = taf_mngdConn_AddDataStateHandler(dataRef,
                (taf_mngdConn_DataStateHandlerFunc_t)taf_ivss_mngdConn_DataStateHandler, NULL);
            le_utf8_Copy(DataTable[index].name, indPtr->startData.name,
                sizeof(DataTable[index].name), nullptr);
            DataTable[index].enable = true;

            le_sem_Post(indPtr->semRef);
            return;
        }
    }

    indPtr->result = LE_FAULT;
    le_sem_Post(indPtr->semRef);
    LE_ERROR("StartDataHandler failed : No idle data table found.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Starts a data session for the given data ID.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssMngdConnSvc::StartData
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    std::string _name,
    StartDataReply_t _reply
)
{
    // Create a generic response message object.
    taf_IvssMngdConn_Ind_t* indPtr = (taf_IvssMngdConn_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssMngdConn_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss StartData", 0);
    le_utf8_Copy(indPtr->startData.name, _name.c_str(), sizeof(indPtr->startData.name), nullptr);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(StartDataEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for method 'StopData'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssMngdConnSvc::StopDataHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssMngdConn_Ind_t* indPtr = (taf_IvssMngdConn_Ind_t*)reportPtr;

    taf_mngdConn_DataRef_t dataRef = taf_mngdConn_GetDataByName(indPtr->startData.name);
    if (dataRef == nullptr)
    {
        indPtr->result = LE_FAULT;
        le_sem_Post(indPtr->semRef);
        LE_ERROR("taf_mngdConn_GetDataByName failed - %s", LE_RESULT_TXT(indPtr->result));
        return;
    }

    indPtr->result = taf_mngdConn_StopData(dataRef);
    if (indPtr->result != LE_OK)
    {
        le_sem_Post(indPtr->semRef);
        LE_ERROR("taf_mngdConn_StopData failed - %s", LE_RESULT_TXT(indPtr->result));
        return;
    }

    uint32_t index;
    for (index = 0; index < IVSS_MAX_DATA_NUM; index++)
    {
        if (std::strcmp(indPtr->startData.name, DataTable[index].name) == 0)
        {
            taf_mngdConn_RemoveDataStateHandler(DataStateHandlerRef[index]);
            memset(DataTable[index].name, 0, sizeof(DataTable[index].name));
            DataTable[index].state = TAF_MNGDCONN_DATA_DISCONNECTED;
            DataTable[index].enable = false;

            le_sem_Post(indPtr->semRef);
            return;
        }
    }

    indPtr->result = LE_FAULT;
    le_sem_Post(indPtr->semRef);
    LE_ERROR("StopDataHandler failed : No data table with the current data name found.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Stops a data session for the given data ID.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssMngdConnSvc::StopData
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    std::string _name,
    StopDataReply_t _reply
)
{
    // Create a generic response message object.
    taf_IvssMngdConn_Ind_t* indPtr = (taf_IvssMngdConn_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssMngdConn_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss StopData", 0);
    le_utf8_Copy(indPtr->stopData.name, _name.c_str(), sizeof(indPtr->stopData.name), nullptr);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(StopDataEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Stops a data session for the given data ID.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssMngdConnSvc::GetDataList
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    GetDataListReply_t _reply
)
{
    uint8_t dataNum = 0;
    std::vector<std::string> name = {};
    std::vector<MngdConnSvc::DataState> dataState = {};

    uint32_t index;
    for (index = 0; index < IVSS_MAX_DATA_NUM; index++)
    {
        if (DataTable[index].enable)
        {
            dataNum++;
            name.push_back(std::string(DataTable[index].name));
            dataState.push_back(DataStateMngdConnToIvss(DataTable[index].state));
        }
    }

    _reply(ResultLeToIvss(LE_OK), dataNum, name, dataState);
};

//--------------------------------------------------------------------------------------------------
/**
 * Handler for CellInfo changes.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssMngdConnSvc::taf_ivss_mngdConn_DataStateHandler
(
    taf_mngdConn_DataRef_t dataRef,         ///< [IN] The data reference.
    taf_mngdConn_DataState_t dataState,     ///< [IN] The data state.
    void* contextPtr                        ///< [IN] Handler context.
)
{
    auto ivssMngdConn = tafIvssMngdConnSvc::GetInstance();
    char dataName[TAF_MNGDCONN_MAX_NAME_LEN];

    le_result_t result = taf_mngdConn_GetDataNameByRef(dataRef, dataName, sizeof(dataName));
    TAF_ERROR_IF_RET_NIL(result != LE_OK, "taf_mngdConn_GetDataNameByRef fail - %s",
        LE_RESULT_TXT(result));

    uint32_t index;
    for (index = 0; index < IVSS_MAX_DATA_NUM; index++)
    {
        if (std::strcmp(dataName, DataTable[index].name) == 0)
        {
            DataTable[index].state = dataState;
            ivssMngdConn->fireDataStateEvent(std::string(dataName),
                DataStateMngdConnToIvss(dataState));
            LE_DEBUG("tafivssMngdConnSvc DataState Event");
            return;
        }
    }

    LE_ERROR("taf_ivss_mngdConn_DataStateHandler: No data table with the current data name found.");
};

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssMngdConnSvc::Init
(
    void
)
{
    uint32_t index;
    for (index = 0; index < IVSS_MAX_DATA_NUM; index++)
    {
        memset(DataTable[index].name, 0, sizeof(DataTable[index].name));
        DataTable[index].state = TAF_MNGDCONN_DATA_DISCONNECTED;
        DataTable[index].enable = false;
    }

    // Init the memory pool
    EventPool = le_mem_CreatePool("Ivss MngdConn EventPool", sizeof(taf_IvssMngdConn_Ind_t));

    // Init events.
    StartDataEvent = le_event_CreateIdWithRefCounting("StartDataEvent");
    StopDataEvent = le_event_CreateIdWithRefCounting("StopDataHandler");

    // Init event handler.
    StartDataEventHandlerRef = le_event_AddHandler("StartDataEvent Handler", StartDataEvent,
        tafIvssMngdConnSvc::StartDataHandler);
    StopDataEventHandlerRef = le_event_AddHandler("StopDataEvent Handler", StopDataEvent,
        tafIvssMngdConnSvc::StopDataHandler);

    LE_INFO("tafIvssMngdConnSvc Service initialized");
};
