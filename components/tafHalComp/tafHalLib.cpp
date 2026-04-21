/*
 *  Copyright (c) 2023, 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/** tafHalLib.cpp
 *
 * This is the telaf library load tool.
 *
 * The general format of commands is:
 *
 *
 *
 * The following are examples of supported commands:
 *
 *    --------------------------
 *    | cmd | commandParameter |
 *    --------------------------
 * e.g. taf_devMgr_LoadDrv
 *      taf_devMgr_UnloadDrv
 *
 *
 */

#include "tafHalLib.hpp"
#include "devManager.hpp"
#include "legato.h"
#include "limit.h"
#include "tafHalIF.hpp"
#include <ctype.h>
#include <dlfcn.h>
#include <setjmp.h>

#define TIMER_SAFECALL 5
DECLARE_SAFE_CALL();

// hash map for module interface and the so handle
static le_hashmap_Ref_t tafModInfMap = nullptr;
static bool isReady = false;

// Maximum vhal module that can be opened by app
#define TAF_HAL_MODULE_MAX_LOAD_NUM 32

// Time in second to retry connecting to deviceManger during failing
#define TIME_RETRY_CONNECT_TO_DEVICEMANAGER 1

//--------------------------------------------------------------------------------------------------
/**
 * True if an error response was received from the device Manager.
 **/
//--------------------------------------------------------------------------------------------------
static bool ErrorOccurred = false;
static le_msg_SessionRef_t ipcSessionRef = nullptr;

//--------------------------------------------------------------------------------------------------
/**
 * Handles a message received from the device managaer
 **/
//--------------------------------------------------------------------------------------------------
static void MsgReceiveHandler(
    le_msg_MessageRef_t msgRef,
    void* contextPtr // not used.
)
{
    const char* responsePtr;

    responsePtr = (const char*)le_msg_GetPayloadPtr(msgRef);

    // Print out whatever the deviceManager sent us.
    LE_INFO("%s", responsePtr);

    // If the first character of the response is a '*', then there has been an error.
    if (responsePtr[0] == '*')
    {
        ErrorOccurred = true;
    }
}

static void AppendToCommand(
    le_msg_MessageRef_t msgRef, ///< Command message to which the text should be appended.
    const char* textPtr ///< Text to append to the message.
)
{
    if (LE_OVERFLOW == le_utf8_Append((char*)le_msg_GetPayloadPtr(msgRef), textPtr, le_msg_GetMaxPayloadSize(msgRef), nullptr))
    {
        LE_ERROR("Command string is too long.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handles the deviceManager closing the IPC session.
 **/
//--------------------------------------------------------------------------------------------------
static void SessionCloseHandler(
    le_msg_SessionRef_t sessionRef, // not used.
    void* contextPtr // not used.
)
{
    if (ErrorOccurred)
    {
        LE_ERROR("Errors received from Device manager");
    }
    else
    {
        isReady = false;
        LE_INFO("Close the session from Device manager");
    }

    // We do not exit as it is expected that device manager close the session after serving
}

//--------------------------------------------------------------------------------------------------
/**
 * Creates an IPC session to device manager.
 */
//--------------------------------------------------------------------------------------------------
static void CreateSessionToDeviceManager()
{
    // do something before loading if needed
    le_msg_ProtocolRef_t protocolRef;

    protocolRef = le_msg_GetProtocolRef(DEV_MANAGER_PROTOCOL_ID, DEV_MANAGER_MAX_CMD_PACKET_BYTES);
    ipcSessionRef = le_msg_CreateSession(protocolRef, DEV_MANAGER_SERVICE_NAME);

    le_msg_SetSessionRecvHandler(ipcSessionRef, MsgReceiveHandler, nullptr);
    le_msg_SetSessionCloseHandler(ipcSessionRef, SessionCloseHandler, nullptr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Creates a hash map to store module-inf handles
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateInfHashmap()
{
    // Create hash table
    tafModInfMap = le_hashmap_Create("tafModInfMap",
                                        TAF_HAL_MODULE_MAX_LOAD_NUM,
                                        le_hashmap_HashVoidPointer,
                                        le_hashmap_EqualsVoidPointer);

    if (tafModInfMap == nullptr)
    {
        LE_ERROR("Can not create Map table for TAF VHAL");
        return LE_NO_MEMORY;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Opens an IPC session to device manager.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ConnectToDeviceManager()
{
    if(isReady == false)
    {
        CreateSessionToDeviceManager();
    }

    le_result_t result = le_msg_TryOpenSessionSync(ipcSessionRef);

    if (result != LE_OK)
    {
        fprintf(stderr, "ERROR: Can't communicate with the device manager.\n");

        switch (result)
        {
        case LE_UNAVAILABLE:
            fprintf(stderr, "Service not offered by device manager.\n"
                            "Check if the device manager is running with 'ps'\n");
            break;

        case LE_NOT_PERMITTED:
            fprintf(stderr, "Missing binding to device manager\n"
                            "Please check binding info\n");
            break;

        case LE_COMM_ERROR:
            fprintf(stderr, "Service Directory is unreachable.\n"
                            "Perhaps the Service Directory is not running?\n");
            break;

        default:
            fprintf(stderr, "Unexpected result code %d (%s)\n", result, LE_RESULT_TXT(result));
            break;
        }
        return result;
    }

    return result;
}

COMPONENT_INIT_ONCE
{
    if(CreateInfHashmap() != LE_OK)
    {
        LE_FATAL("Failed to initialize hash map");
    }
    CreateSessionToDeviceManager();
}

__attribute__((destructor)) void _telaf_unInitHal(
    void)
{
    // do something before dlclose return if needed
}

extern "C" LE_SHARED void* taf_devMgr_LoadDrv(const char* drvName, const char* drvVer)
{
    le_msg_MessageRef_t responseMsgRef;
    int ret = 0;
    char drvFile[256];
    size_t numBytes = 0;
    void* drvHandle = nullptr;
    char* errMsg = nullptr;
    TAF_HAL_MGR_INF_t* mgrInf;

    // Fill the driver name
    if (drvName == nullptr)
    {
        LE_ERROR("Module name is not correct");
        return nullptr;
    }

    // We may still try to connect
    if (isReady == false)
    {
        LE_ERROR("Not ready yet, try to connect!");

        // connected,usually this get disconnected
        if (ConnectToDeviceManager() != LE_OK)
        {

            LE_ERROR("Can not connect to device manager");
            return nullptr;
        }
        else
        {
            LE_INFO("Connect to device manager succussfully");
            isReady = true;
        }
    }

    // Send the command 'o' to ask device manager to open the module

    // 1. create a message from the session
    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(ipcSessionRef);
    char* payloadPtr = (char*)le_msg_GetPayloadPtr(msgRef);

    // fill command first , 'o' open request
    // Construct the message.
    payloadPtr[0] = DEV_MANAGER_SERVICE_OPEN_CMD;
    payloadPtr[1] = '\0';

    AppendToCommand(msgRef, (const char*)drvName);
    AppendToCommand(msgRef, ":");

    if (drvVer != nullptr)
    {
        if (strlen(drvVer) != 5)
        {
            return nullptr;
        }

        if (!isdigit(drvVer[0]) || !isdigit(drvVer[1]) ||
            drvVer[2] != '.' ||
            !isdigit(drvVer[3]) || !isdigit(drvVer[4]))
        {
            return nullptr;
        }

        AppendToCommand(msgRef, (const char*)drvVer);
    }
    else
    {
        // use the wildcard * to match any version of the driver
        AppendToCommand(msgRef, "*");
    }

    // send the request, as we do not maintain a session between client and server
    responseMsgRef = le_msg_RequestSyncResponse(msgRef);
    LE_INFO("Got response from devManager");

    isReady = false;

    if (responseMsgRef == nullptr) // Fatal error, no valid response
    {
        LE_ERROR("No valid response from device manager");

        // the session will be closed, but the app will exit
        return nullptr;
    }

    // Get the reponse payload, the full path of the driver
    payloadPtr = (char*)le_msg_GetPayloadPtr(responseMsgRef);

    le_msg_ReleaseMsg(responseMsgRef);

    le_utf8_Copy(drvFile, payloadPtr, sizeof(drvFile), &numBytes);

    if ((numBytes == 0) || (drvFile[0] == '*')) // * means error from device manager
    {
        LE_ERROR("No valid driver was found");
        return nullptr;
    }

    // open the so on behalf of the app
    // Try to open it first
    ENTER_SAFE_CALL_EX(TIMER_SAFECALL, ret, drvHandle, dlopen(drvFile, RTLD_NOW));
    EXIT_SAFE_CALL();
    if(ret == -1)
    {
        LE_ERROR("Failed to dlopen the driver %s", drvFile);
        return nullptr;
    }

    if (drvHandle == nullptr)
    {
        LE_ERROR("Failed to load the driver %s", drvFile);
        LE_ERROR("Error: %s", dlerror());
        //remove the driver
        return nullptr;
    }

    // find the interface
    mgrInf = (TAF_HAL_MGR_INF_t*)dlsym(drvHandle, TAF_HAL_INFO_TAB_STR);

    if((errMsg = dlerror()) != nullptr || mgrInf == nullptr)
    {
        LE_ERROR("Failed to get the hal information table: %s", errMsg ? errMsg : "mgrInf is null");
        return nullptr;
    }

    // Get the specif interface and return it, instead of returning the so handle
    // in this way, the manager interface is hidden from services/apps
    // put in the hash table first
    void* infRef = (*(mgrInf->getModInf))();
    if (le_hashmap_Put(tafModInfMap, infRef, drvHandle))
    {
        // already exist, re-load without closing it
        // we do not return the existing module interface as there is something wrong in the app/service
        // Check the app/service rather than returning a module interface
        LE_ERROR("The VHAL module has been opened yet");

        // close the handle
        dlclose(drvHandle);

        return nullptr;
    }
    LE_INFO("tafModInfMap size after put: %" PRIuS, le_hashmap_Size(tafModInfMap));

    return infRef;
}

// Once this is called, not more data/function shall be called in the app/service,
// or some unexpected behavior will happen.
extern "C" LE_SHARED bool taf_devMgr_UnloadDrv(void* handle)
{
    void* drvHandle;
    bool ret = true;
    char drvName[128];
    uint16_t majorVer, minorVer;
    TAF_HAL_MGR_INF_t* mgrInf;
    le_msg_MessageRef_t responseMsgRef;
    char* errMsg;

    LE_INFO("tafModInfMap size before remove: %" PRIuS, le_hashmap_Size(tafModInfMap));

    // Check if it exist, the input is module inteface
    drvHandle = le_hashmap_Remove(tafModInfMap, handle);
    if (!drvHandle)
    {
        LE_ERROR("Cannot find the VHAL module to be closed");
        return false;
    }

    // get the driver name

    mgrInf = (TAF_HAL_MGR_INF_t*)dlsym(drvHandle, TAF_HAL_INFO_TAB_STR);

    errMsg = dlerror();
    if (errMsg != nullptr)
    {
        LE_ERROR("Failed to get the hal information table");
        ret = false;
    }

    if (mgrInf == nullptr)
    {
        LE_ERROR("mgrInf is null");
        return false;
    }

    if (le_utf8_Copy(drvName, mgrInf->name, sizeof(drvName), nullptr) != LE_OK)
    {
        ret = false;
        LE_ERROR("Cannot get the module name");
    }

    majorVer = mgrInf->majorVer;
    minorVer = mgrInf->minorVer;
    if (dlclose(drvHandle) != 0)
    {
        LE_ERROR("Can not close the VHAL module");

        // do some other cleanup?

        ret = false;
    }

    // Send the command 'c' to tell device manager

    if (ret != false)
    {
        // usually this is null
        // We may still try to connect
        if (isReady == false)
        {
            LE_ERROR("Not ready yet, try to connect!");

            // connect...
            if (ConnectToDeviceManager() != LE_OK)
            {

                LE_ERROR("Can not connect to device manager");
            }
            else
            {
                LE_INFO("Successfully connect to device manager");
                isReady = true;
            }
        }

        // Send the command 'c' to ask device manager to close the module
        if (isReady == true)
        {

            // 1. create a message from the session
            le_msg_MessageRef_t msgRef = le_msg_CreateMsg(ipcSessionRef);
            char* payloadPtr = (char*)le_msg_GetPayloadPtr(msgRef);

            // fill command first , 'o' open request
            // Construct the message.
            payloadPtr[0] = DEV_MANAGER_SERVICE_CLOSE_CMD;
            payloadPtr[1] = '\0';

            char versionBuff[6] = {};
            const uint16_t maxVersion = 99;
            if (majorVer > maxVersion || minorVer > maxVersion) {
                LE_ERROR("Error: Version number out of range\n");
                return false;
            }
            snprintf(versionBuff, sizeof(versionBuff), "%.02u.%02u", majorVer, minorVer);

            AppendToCommand(msgRef, drvName);
            AppendToCommand(msgRef, ":");
            AppendToCommand(msgRef, versionBuff);

            // tell the device manager
            responseMsgRef = le_msg_RequestSyncResponse(msgRef);

            isReady = false;

            if (responseMsgRef == nullptr) // Fatal error, no valid response
            {
                LE_ERROR("No valid response from device manager");

                // the session will be closed, but the app will exit
                ret = false;
            }
            else
            {
                char* payloadPtr = (char*)le_msg_GetPayloadPtr(responseMsgRef);
                le_msg_ReleaseMsg(responseMsgRef);

                // device manager return error
                if ((*payloadPtr) == '*')
                {
                    LE_ERROR("Device manager failed to close the driver");
                    ret = false;
                }
                else
                {
                    LE_INFO("Driver close successfully");
                    return true;
                }
            }
        }
    }
    // return the status
    return ret;
}

COMPONENT_INIT
{
    // initializing
    le_result_t ret;

    // Connect to Device Manager
    ret = ConnectToDeviceManager();

    // retry it again
    if (ret != LE_OK)
    {
        LE_ERROR("Can not connect to device manager, retrying...");

        le_event_QueueFunction(&COMPONENT_INIT_NAME, nullptr, nullptr);

        // back off serval seconds and retrying
        le_thread_Sleep(TIME_RETRY_CONNECT_TO_DEVICEMANAGER);

        return;
    }

    // ready to server
    isReady = true;
}
