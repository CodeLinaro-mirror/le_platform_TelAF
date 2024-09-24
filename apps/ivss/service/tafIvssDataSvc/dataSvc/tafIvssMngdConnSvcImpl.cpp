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
 * Find the index of the data name in DataTable
 */
//--------------------------------------------------------------------------------------------------
bool findDataNameIndex(const char* name, uint32_t* index) {
    if (index == NULL) {
        return false;
    }

    for (uint32_t i = 0; i < IVSS_MAX_DATA_NUM; i++) {
        if (std::strcmp(name, DataTable[i].name) == 0 && DataTable[i].startEnable) {
            *index = i;
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Find the index of the free data in DataTable
 */
//--------------------------------------------------------------------------------------------------
bool findFreeDataIndex(const char* name, uint32_t* index) {
    if (index == NULL) {
        return false;
    }

    for (uint32_t i = 0; i < IVSS_MAX_DATA_NUM; i++) {
        if (DataTable[i].startEnable == false) {
            *index = i;
            return true;
        }
    }
    return false;
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
    if (dataRef == NULL)
    {
        indPtr->result = LE_FAULT;
        TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
            "taf_mngdConn_GetDataByName failed - %s", LE_RESULT_TXT(indPtr->result));
    }

    // Get data table index.
    uint32_t index;
    if (!findDataNameIndex(indPtr->startData.name, &index))
    {
        if (!findFreeDataIndex(indPtr->startData.name, &index))
        {
            indPtr->result = LE_FAULT;
            TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
                "StartDataHandler findFreeDataIndex failed - %s", LE_RESULT_TXT(indPtr->result));
        }
        else
        {
            le_utf8_Copy(DataTable[index].name, indPtr->startData.name,
                sizeof(DataTable[index].name), NULL);
            DataTable[index].startEnable = true;
        }
    }

    // Add DataState handler.
    if (!DataTable[index].handleEable)
    {
        DataStateHandlerRef[index] = taf_mngdConn_AddDataStateHandler(dataRef,
            (taf_mngdConn_DataStateHandlerFunc_t)taf_ivss_mngdConn_DataStateHandler, NULL);
        DataTable[index].handleEable = true;
    }

    // Start Data.
    indPtr->result = taf_mngdConn_StartData(dataRef);
    TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
        "taf_mngdConn_StartData failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
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
    LE_INFO("tafIvssMngdConnSvc StartData \n");

    taf_IvssMngdConn_Ind_t* indPtr = (taf_IvssMngdConn_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssMngdConn_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss StartData", 0);
    le_utf8_Copy(indPtr->startData.name, _name.c_str(), sizeof(indPtr->startData.name), NULL);

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
    if (dataRef == NULL)
    {
        indPtr->result = LE_FAULT;
        TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
            "taf_mngdConn_GetDataByName failed - %s", LE_RESULT_TXT(indPtr->result));
    }

    // Get data table index.
    uint32_t index;
    if (!findDataNameIndex(indPtr->startData.name, &index))
    {
        indPtr->result = LE_FAULT;
        TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
            "StopDataHandler findDataNameIndex failed - %s", LE_RESULT_TXT(indPtr->result));
    }

    // Stop Data.
    DataTable[index].stopEnable = true;
    indPtr->result = taf_mngdConn_StopData(dataRef);
    TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
        "taf_mngdConn_StopData failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
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
    LE_INFO("tafIvssMngdConnSvc StopData \n");

    taf_IvssMngdConn_Ind_t* indPtr = (taf_IvssMngdConn_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssMngdConn_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss StopData", 0);
    le_utf8_Copy(indPtr->stopData.name, _name.c_str(), sizeof(indPtr->stopData.name), NULL);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(StopDataEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result));

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
};

//--------------------------------------------------------------------------------------------------
/**
 * Gets connection state information for all currently running datas.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssMngdConnSvc::GetDataList
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    GetDataListReply_t _reply
)
{
    LE_INFO("tafIvssMngdConnSvc GetDataList \n");

    uint8_t dataNum = 0;
    std::vector<std::string> name = {};
    std::vector<MngdConnSvc::DataState> dataState = {};

    uint32_t index;
    for (index = 0; index < IVSS_MAX_DATA_NUM; index++)
    {
        if (DataTable[index].stateEnable)
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
 * Add handler function for method 'GetDataIpv4Info'
 */
//--------------------------------------------------------------------------------------------------
void tafIvssMngdConnSvc::GetDataIpv4InfoHandler
(
    void* reportPtr
)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == NULL, "Null ptr(reportPtr)");

    taf_IvssMngdConn_Ind_t* indPtr = (taf_IvssMngdConn_Ind_t*)reportPtr;
    taf_mngdConn_DataRef_t dataRef = taf_mngdConn_GetDataByName(indPtr->startData.name);
    if (dataRef == NULL)
    {
        indPtr->result = LE_FAULT;
        TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
            "taf_mngdConn_GetDataByName failed - %s", LE_RESULT_TXT(indPtr->result));
    }

    uint8_t profileId;
    indPtr->result = taf_mngdConn_GetProfileNumberByRef(dataRef, &profileId);
    TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
        "taf_mngdConn_GetProfileNumberByRef failed - %s", LE_RESULT_TXT(indPtr->result));

    uint8_t phoneId;
    indPtr->result = taf_mngdConn_GetPhoneIdByRef(dataRef, &phoneId);
    TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
        "taf_mngdConn_GetPhoneIdByRef failed - %s", LE_RESULT_TXT(indPtr->result));

    taf_dcs_ProfileRef_t profileRef = NULL;
    profileRef= taf_dcs_GetProfileEx(profileId, phoneId);
    if (profileRef == NULL)
    {
        indPtr->result = LE_FAULT;
        TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
            "taf_dcs_GetProfileEx failed - %s", LE_RESULT_TXT(indPtr->result));
    }

    if (taf_dcs_IsIPv4(profileRef) == false)
    {
        indPtr->result = LE_FAULT;
        TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
            "taf_dcs_IsIPv4 failed - %s", LE_RESULT_TXT(indPtr->result));
    }

    indPtr->result = taf_dcs_GetInterfaceName(profileRef, indPtr->getDataIpv4Info.ifName,
        sizeof(indPtr->getDataIpv4Info.ifName));
    TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
        "taf_dcs_GetInterfaceName failed - %s", LE_RESULT_TXT(indPtr->result));

    indPtr->result = taf_dcs_GetIPv4Address(profileRef, indPtr->getDataIpv4Info.ipAddr,
        sizeof(indPtr->getDataIpv4Info.ipAddr));
    TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
        "taf_dcs_GetIPv4Address failed - %s", LE_RESULT_TXT(indPtr->result));

    indPtr->result = taf_dcs_GetIPv4GatewayAddress(profileRef, indPtr->getDataIpv4Info.gatewayAddr,
        sizeof(indPtr->getDataIpv4Info.gatewayAddr));
    TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
        "taf_dcs_GetIpv4GatewayAddress failed - %s", LE_RESULT_TXT(indPtr->result));

    indPtr->result = taf_dcs_GetIPv4DNSAddresses(profileRef, indPtr->getDataIpv4Info.dns1Addr,
        sizeof(indPtr->getDataIpv4Info.dns1Addr), indPtr->getDataIpv4Info.dns2Addr,
        sizeof(indPtr->getDataIpv4Info.dns2Addr));
    TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
        "taf_dcs_GetIPv4DNSAddresses failed - %s", LE_RESULT_TXT(indPtr->result));

    indPtr->result = taf_dcs_GetIPv4SubnetMask(profileRef, &indPtr->getDataIpv4Info.ipMask);
    TAF_IVSS_ERROR_IF_RET_NIL(indPtr->result != LE_OK, indPtr->semRef,
        "taf_dcs_GetIPv4SubnetMask failed - %s", LE_RESULT_TXT(indPtr->result));

    le_sem_Post(indPtr->semRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data Ipv4 address, gateway, DNS, and mask.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssMngdConnSvc::GetDataIpv4Info
(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    std::string _name,
    GetDataIpv4InfoReply_t _reply
)
{
    // Create a generic response message object.
    LE_INFO("tafIvssMngdConnSvc GetDataIpv4Info \n");

    taf_IvssMngdConn_Ind_t* indPtr = (taf_IvssMngdConn_Ind_t*)le_mem_ForceAlloc(EventPool);
    memset(indPtr, 0, sizeof(taf_IvssMngdConn_Ind_t));
    indPtr->semRef = le_sem_Create("Ivss GetDataIpv4Info", 0);
    le_utf8_Copy(indPtr->getDataIpv4Info.name, _name.c_str(), sizeof(indPtr->getDataIpv4Info.name),
        NULL);

    // Report to the common COMMONAPI msg handler in service layer.
    le_event_ReportWithRefCounting(GetDataIpv4InfoEvent, (void*)indPtr);
    le_sem_Wait(indPtr->semRef);
    _reply(ResultLeToIvss(indPtr->result), std::string(indPtr->getDataIpv4Info.ifName),
        MngdConnSvc::DataIpInfo{indPtr->getDataIpv4Info.ipAddr, indPtr->getDataIpv4Info.gatewayAddr,
        indPtr->getDataIpv4Info.dns1Addr, indPtr->getDataIpv4Info.dns2Addr,
        indPtr->getDataIpv4Info.ipMask});

    le_sem_Delete(indPtr->semRef);
    le_mem_Release(indPtr);
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

    // Get data table index.
    uint32_t index;
    if (!findDataNameIndex(dataName, &index))
    {
        LE_ERROR("taf_ivss_mngdConn_DataStateHandler: findDataNameIndex failed.");
        return;
    }

    // Send broadcast.
    DataTable[index].state = dataState;
    DataTable[index].stateEnable = true;
    ivssMngdConn->fireDataStateEvent(std::string(dataName), DataStateMngdConnToIvss(dataState));
    LE_DEBUG("tafivssMngdConnSvc DataState Event");

    // Remove DataState handler.
    if (DataTable[index].stopEnable)
    {
        if (dataState == TAF_MNGDCONN_DATA_DISCONNECTED)
        {
            taf_mngdConn_RemoveDataStateHandler(DataStateHandlerRef[index]);
            memset(&DataTable[index], 0, sizeof(DataTable[index]));
        }
        else
        {
            LE_ERROR("stopData but received status is not DISCONNECTED: (%d)",
                static_cast<int>(dataState));
        }
    }
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
        memset(&DataTable[index], 0, sizeof(DataTable[index]));
    }

    // Init the memory pool
    EventPool = le_mem_CreatePool("Ivss MngdConn EventPool", sizeof(taf_IvssMngdConn_Ind_t));

    // Init events.
    StartDataEvent = le_event_CreateIdWithRefCounting("StartDataEvent");
    StopDataEvent = le_event_CreateIdWithRefCounting("StopDataEvent");
    GetDataIpv4InfoEvent = le_event_CreateIdWithRefCounting("GetDataIpv4InfoEvent");

    // Init event handler.
    StartDataEventHandlerRef = le_event_AddHandler("StartDataEvent Handler", StartDataEvent,
        tafIvssMngdConnSvc::StartDataHandler);
    StopDataEventHandlerRef = le_event_AddHandler("StopDataEvent Handler", StopDataEvent,
        tafIvssMngdConnSvc::StopDataHandler);
    GetDataIpv4InfoEventHandlerRef = le_event_AddHandler("GetDataIpv4InfoEvent Handler",
        GetDataIpv4InfoEvent, tafIvssMngdConnSvc::GetDataIpv4InfoHandler);

    LE_INFO("tafIvssMngdConnSvc Service initialized");
};
