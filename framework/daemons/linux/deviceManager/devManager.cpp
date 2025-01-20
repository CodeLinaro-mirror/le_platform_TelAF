/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "limit.h"

#include "tafSvcIF.hpp"
#include "tafHalIF.hpp"
#include "devManager.hpp"

#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#ifdef LE_CONFIG_ENABLE_SELINUX
#include <selinux/selinux.h>
#endif

DECLARE_SAFE_CALL();

#ifdef LE_CONFIG_VHAL_DRIVER_DIR
#       define DRIVER_TMP_STORAGE LE_CONFIG_VHAL_DRIVER_DIR
#else
#       define DRIVER_TMP_STORAGE "/data/tmp/drivers/"
#endif

#define DEV_MANAGER_STORAGE "/tmp/legato/devManager/"
#define DEV_MANAGER_DRIVER_STORAGE DEV_MANAGER_STORAGE"drivers/"
#define DEV_MANAGER_TMP_STORAGE DEV_MANAGER_STORAGE"tmp/"
#define DEV_MANAGER_CMD_MAX_LEN   255
#define DEV_MANAGER_STORAGE_CONTEXT "system_u:object_r:telaf_devmgr_data_t:s0"

#define TIMER_SAFECALL 5

//--------------------------------------------------------------------------------------------------
/**
 * The structure to store information of process that loads driver
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char appPath[LIMIT_MAX_PATH_BYTES];
    pid_t pid;
    le_dls_Link_t link; // pid list;
}
ProcessInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * The structure to store information of installed driver
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char name[DEV_MANAGER_DRIVER_NAME_MAX_LEN];
    uint16_t  majorVer;
	uint16_t  minorVer;
    char vendor[DEV_MANAGER_DRIVER_VENDOR_MAX_LEN];

    uint8_t drvType; // driver type

    void *drvHandle; // taf driver handle
    uint8_t serviceMax;  // determine how many services can use this driver

    le_dls_Link_t link; // driver list

    char loc[DEV_MANAGER_DRIVER_LOCATION_MAX_LEN]; // full path
    int  status; // driver status

    le_dls_List_t ProcessInfoList;  // process info list
    uint32_t clntRef;	// simply record how many client use this driver
}
DeviceDriver_t;

//--------------------------------------------------------------------------------------------------
/**
 * The resources to maintain drivers and processes
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t DeviceDriverPoolRef;
static le_dls_List_t DriverList = LE_DLS_LIST_INIT;

static le_mem_PoolRef_t ProcessInfoPoolRef;


#ifdef __cplusplus
    extern "C" {
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Checks whether the process of the input session is using the specified driver
 */
//--------------------------------------------------------------------------------------------------
static bool IsProcessUsingDriver
(
    le_msg_SessionRef_t sessionRef,
    DeviceDriver_t* driverPtr
)
{
    TAF_ERROR_IF_RET_VAL(sessionRef == nullptr, false, "sessionRef is nullptr");
    TAF_ERROR_IF_RET_VAL(driverPtr == nullptr, false, "driverPtr is nullptr");

    pid_t pid;
    uid_t uid;

    if (le_msg_GetClientUserCreds(sessionRef, &uid, &pid) == LE_OK)
    {
        LE_INFO("Client process id %d, open the driver", pid);
    }
    else
    {
        LE_ERROR("Cannot get the client pid");
        return false;
    }

    le_dls_Link_t* linkPtr;
    linkPtr = le_dls_Peek(&(driverPtr->ProcessInfoList));

    while(linkPtr != nullptr)
    {
        ProcessInfo_t* processInfo = CONTAINER_OF(linkPtr, ProcessInfo_t, link);

        if(processInfo == nullptr)
        {
            return false;
        }
        else if(pid == processInfo->pid)
        {
            // found the pid in the list
            LE_INFO("PID %d was found for driver %s !", pid, driverPtr->name);

            return true;
        }

        linkPtr = le_dls_PeekNext(&(driverPtr->ProcessInfoList), linkPtr);
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stores the process information of the input session for driver
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StoreProcessInfoToDriver
(
    le_msg_SessionRef_t sessionRef,
    DeviceDriver_t* driverPtr
)
{
    TAF_ERROR_IF_RET_VAL(sessionRef == nullptr, LE_BAD_PARAMETER, "sessionRef is nullptr");
    TAF_ERROR_IF_RET_VAL(driverPtr == nullptr, LE_BAD_PARAMETER, "driverPtr is nullptr");

    ProcessInfo_t* processInfo = (ProcessInfo_t *)le_mem_ForceAlloc(ProcessInfoPoolRef);

    processInfo->link = LE_DLS_LINK_INIT;

    pid_t pid;
    uid_t uid;

    if (le_msg_GetClientUserCreds(sessionRef, &uid, &pid) == LE_OK)
    {
        LE_INFO("Client process id %d, open the driver", pid);

        char procPath[LIMIT_MAX_PATH_BYTES] = {0};
        char appPath[LIMIT_MAX_PATH_BYTES] = {0};

        // Read the program name from the softlink of /proc/<pid>/exe .
        LE_ASSERT(snprintf(procPath, sizeof(procPath), "/proc/%d/exe", pid)
                < (int)sizeof(procPath));

        memset(appPath, 0, sizeof(appPath));
        if (readlink(procPath, appPath, sizeof(appPath)) < 0)
        {
            LE_ERROR("readlink(%s) failed %s", procPath, LE_ERRNO_TXT(errno));
            return LE_FAULT;
        }

        LE_INFO("Get app full name: %s.", appPath);

        processInfo->pid = pid;
        le_utf8_Copy(processInfo->appPath, appPath, LIMIT_MAX_PATH_BYTES, nullptr);
    }
    else
    {
        LE_ERROR("Cannot get the client pid");
        le_mem_Release(processInfo);
        return LE_FAULT;
    }

    le_dls_Queue(&(driverPtr->ProcessInfoList), &(processInfo->link));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks if the process is still running in the system
 */
//--------------------------------------------------------------------------------------------------
static bool ProcessExists(pid_t pid)
{
    if (0 == kill(pid, 0))
    {
        LE_INFO("pid %d exists", pid);
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove the process for specified driver
 * (used in QueueFunction to remove process for driver)
 */
//--------------------------------------------------------------------------------------------------
static void SafeRemoveProcess
(
    void* param1,
    void* param2
)
{
    if(param1 == nullptr || param2 == nullptr)
    {
        LE_ERROR("nullptr pointer,please check!!!");
        return ;
    }

    DeviceDriver_t* driverPtr = (DeviceDriver_t*)param1;
    ProcessInfo_t* processPtr = (ProcessInfo_t*)param2;

    // remove the process from list and free it
    le_dls_Remove(&(driverPtr->ProcessInfoList), &(processPtr->link));
    le_mem_Release(processPtr);
}

static uint GetProcessCountForDrv
(
    DeviceDriver_t* driverPtr
)
{
    TAF_ERROR_IF_RET_VAL(driverPtr == nullptr, LE_BAD_PARAMETER, "driverPtr is nullptr");

    uint pidCount = 0;
    le_dls_Link_t* linkPtr;
    linkPtr = le_dls_Peek(&(driverPtr->ProcessInfoList));

    while(linkPtr != nullptr)
    {
        ProcessInfo_t* processInfo = CONTAINER_OF(linkPtr, ProcessInfo_t, link);

        if(processInfo == nullptr)
        {
            LE_ERROR("processInfo is nullptr");
            return 0;
        }
        else if(ProcessExists(processInfo->pid))
        {
            LE_ERROR("ProcessExists");
            char procPath[LIMIT_MAX_PATH_BYTES] = {0};
            char appPath[LIMIT_MAX_PATH_BYTES] = {0};

            // Read the program name from the softlink of /proc/<pid>/exe .
            LE_ASSERT(snprintf(procPath, sizeof(procPath), "/proc/%d/exe", processInfo->pid)
                    < (int)sizeof(procPath));
            LE_ERROR("procPath: %s", procPath);
            memset(appPath, 0, sizeof(appPath));
            if (readlink(procPath, appPath, sizeof(appPath)) < 0)
            {
                LE_ERROR("readlink(%s) failed %s", procPath, LE_ERRNO_TXT(errno));
                return LE_FAULT;
            }

            LE_INFO("Get app full name: %s.", appPath);

            if(strncmp(processInfo->appPath, appPath, LIMIT_MAX_PATH_BYTES) == 0)
            {
                pidCount++;

                LE_INFO("PID %d was found for driver %s !", processInfo->pid, driverPtr->name);
                LE_INFO("PID count: %u", pidCount);
            }
            else // if the appPath is not matched
            {
                le_event_QueueFunction(SafeRemoveProcess, driverPtr, processInfo);
            }
        }
        else // if the process of pid is not running
        {
            LE_INFO("PID %d doesn't exist anymore, remove it", processInfo->pid);
            le_event_QueueFunction(SafeRemoveProcess, driverPtr, processInfo);
        }
        linkPtr = le_dls_PeekNext(&(driverPtr->ProcessInfoList), linkPtr);
    }

    return pidCount;
}

static int GetDrvStatus
(
    DeviceDriver_t* driverPtr
)
{
    if(GetProcessCountForDrv(driverPtr) == 0)
    {
        return DEV_MANAGER_DRV_IDLE;
    }
    return DEV_MANAGER_DRV_BUSY;
}

/*
 * Error message should start with "***"
 * e.g. "***Error: Installing the driver %s"
 */

static void SendToDrvTool
(
    le_msg_SessionRef_t ipcSessionRef,
    const char* msgPtr
)
{
    le_result_t result;

    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(ipcSessionRef);

    char* msgToSendPtr = (char *) le_msg_GetPayloadPtr(msgRef);

    result = le_utf8_Copy(msgToSendPtr, msgPtr, le_msg_GetMaxPayloadSize(msgRef), nullptr);

    if(result == LE_OVERFLOW)
    {
        LE_WARN("Send Message is too long, truncated");
    }

    le_msg_Send(msgRef);
}

static void ListDrivers
(
    char* buffer,
    size_t bufferSize
)
{
    uint offset = 0;

    le_dls_Link_t* linkPtr;
    DeviceDriver_t* drvPtr;
    linkPtr = le_dls_Peek(&DriverList);

    while(offset < bufferSize && linkPtr != nullptr)
    {
        drvPtr = CONTAINER_OF(linkPtr, DeviceDriver_t, link);

        char driverName[DEV_MANAGER_DRIVER_NAME_MAX_LEN] = {};

        int n = snprintf(driverName, DEV_MANAGER_DRIVER_NAME_MAX_LEN,
                         "%s %.02u.%.02u\n", drvPtr->name, drvPtr->majorVer, drvPtr->minorVer);
        snprintf(buffer + offset, bufferSize - offset, "%s", driverName);

        offset += n;

        linkPtr = le_dls_PeekNext(&DriverList, linkPtr);
    }
}

static DeviceDriver_t* FindDrv
(
    const char* name,
    const uint16_t majorVer,
    const uint16_t minorVer,
    bool ignoreVersion
)
{
    le_dls_Link_t* linkPtr;
    DeviceDriver_t* drvPtr;
    linkPtr = le_dls_Peek(&DriverList);

    while(linkPtr != nullptr)
    {
        drvPtr = CONTAINER_OF(linkPtr, DeviceDriver_t, link);

        if((strncmp(drvPtr->name, name, DEV_MANAGER_DRIVER_NAME_MAX_LEN) == 0) &&
           ((ignoreVersion == true) ||
           ((drvPtr->majorVer == majorVer) && (drvPtr->minorVer == minorVer))))
        {
            // found the drv in the list
            LE_INFO("Driver %s %u.%u was found!", name, drvPtr->majorVer, drvPtr->minorVer);

            return drvPtr;
        }

        linkPtr = le_dls_PeekNext(&DriverList, linkPtr);
    }

    return nullptr;
}

// Used in QueueFunction to close driver
static void SafeCloseDrv
(
    void* param1,
    void* param2
)
{
    // the fisrt parameter is the DeviceDriver_t
    // the second parameter, need free or not

    bool needToFree = (bool)param2;

    if(param1 == nullptr)
    {
        LE_ERROR("nullptr pointer,please check!!!");
        return ;
    }

    DeviceDriver_t* drvPtr = (DeviceDriver_t*)param1;

    LE_INFO("Close module: %s %u.%u", drvPtr->name, drvPtr->majorVer, drvPtr->minorVer);

    // now close the so
    dlclose(drvPtr->drvHandle);

    // free it if needed
    if(needToFree)
    {
        le_mem_Release(drvPtr);
    }
    else
    {
        drvPtr->drvHandle = nullptr; // no longer valid
    }
}

le_result_t OpenDrvToGetInfo
(
    const char* drvFile,
    DeviceDriver_t** drvPtr,
    const char** drvName,
    uint16_t* majorVer,
    uint16_t* minorVer,
    char* respPtr
)
{
    TAF_HAL_MGR_INF_t *mgrInf;

    void* drvHandle = nullptr;
    char* errMsg = nullptr;

    // Try to open it first
    drvHandle = dlopen(drvFile, RTLD_NOW);

    if(drvHandle == nullptr)
    {
        LE_ERROR("Failed to load the driver %s", drvFile);

        if((errMsg = dlerror()) != nullptr)
        {
            LE_ERROR("dlerror: %s", errMsg);
        }

        snprintf(respPtr, DEV_MANAGER_MAX_RESP_MSG_BYTES, "***ERROR: Failed to open the driver");

        //remove the driver
        unlink(drvFile);
        return LE_FAULT;
    }

    LE_INFO("Driver %s open successfully",drvFile);

    // create a temp object, process exits if fails to allocate
    *drvPtr = (DeviceDriver_t *)le_mem_ForceAlloc(DeviceDriverPoolRef);

    // fill the handle
    (*drvPtr)->drvHandle = drvHandle;

    // cast the info table to mgr interface
    mgrInf = (TAF_HAL_MGR_INF_t*)dlsym(drvHandle, TAF_HAL_INFO_TAB_STR);

    if((errMsg = dlerror()) != nullptr || mgrInf == nullptr)
    {
        LE_ERROR("Failed to load the driver %s with error: %s",
                    drvFile, errMsg ? errMsg : "mgrInf is null");

        snprintf(respPtr, DEV_MANAGER_MAX_RESP_MSG_BYTES, "***ERROR: Failed to load the driver");

        //remove the driver
        le_event_QueueFunction(SafeCloseDrv, *drvPtr, (void*)true);
        unlink(drvFile);
        return LE_FAULT;
    }

    LE_INFO("Module name: %s",mgrInf->name);
    LE_INFO("majorVer: %u",mgrInf->majorVer);
    LE_INFO("minorVer: %u",mgrInf->minorVer);
    LE_INFO("vendor: %s",mgrInf->vendor);

    *drvName = mgrInf->name;

    if(*drvName == nullptr)
    {
        LE_ERROR("Module name is empty!");
        return LE_FAULT;
    }

    // check driver name and version (no ":" allowed in the name)
    if((**drvName == '\0') || (strstr(*drvName, ":") != nullptr))
    {
        LE_ERROR("No valid driver name %s", *drvName);
        dlclose(drvHandle);

        snprintf(respPtr, DEV_MANAGER_MAX_RESP_MSG_BYTES, "***ERROR: Not a vaild telaf driver");

        //remove the driver
        le_event_QueueFunction(SafeCloseDrv, *drvPtr, (void*)true);
        unlink(drvFile);
        return LE_FAULT;
    }

    // 2. Get the version
    *majorVer = mgrInf->majorVer;
    *minorVer = mgrInf->minorVer;

    return LE_OK;
}

//---------------------------------------------------------------------------------------------------
/**
 * Install driver
 **/
//---------------------------------------------------------------------------------------------------

le_result_t InstallDrvToDevManager
(
    DeviceDriver_t* drvPtr,
    const char* name,
    const uint16_t majorVer,
    const uint16_t minorVer,
    const char* fname
)
{
    TAF_HAL_MGR_INF_t* mgrInf;
    char* errMsg = nullptr;
    DeviceDriver_t* tmpDrv;

    const char* vendorName;

    if(drvPtr == nullptr)
    {
        LE_ERROR("nullptr pointer");
        return LE_NOT_FOUND;
    }

    // check if the drv is installed by searching the list
    tmpDrv = FindDrv(name, majorVer, minorVer, false);

    if(tmpDrv != nullptr)
    {
        // Driver exist
        LE_INFO("Driver %s already exists", name);
        return LE_DUPLICATE;
    }

    // new driver, allocate a node
    drvPtr->link = LE_DLS_LINK_INIT;
    drvPtr->ProcessInfoList = LE_DLS_LIST_INIT;

    // Fill the driver info
    if(le_utf8_Copy(drvPtr->name, name, sizeof(drvPtr->name), nullptr) != LE_OK)
    {
        LE_WARN("Driver name %s was truncated!",name);
    }

    drvPtr->majorVer = majorVer;
    drvPtr->minorVer = minorVer;

    // re-get the mgr interface
    // cast the info table to mgr interface
    mgrInf = (TAF_HAL_MGR_INF_t *)dlsym(drvPtr->drvHandle, TAF_HAL_INFO_TAB_STR);

    if((errMsg = dlerror()) != nullptr || mgrInf == nullptr)
    {
        LE_ERROR("No valid symbol name with error: %s", errMsg ? errMsg : "mgrInf is null");

        return LE_UNAVAILABLE;
    }

    // fill the vendor info
    vendorName = mgrInf->vendor;

    if((vendorName != nullptr) && (*vendorName == '\0'))
    {
        LE_ERROR("No valid vendor name\n");
        return LE_BAD_PARAMETER;
    }

    if(le_utf8_Copy(drvPtr->vendor, vendorName, sizeof(drvPtr->vendor), nullptr) != LE_OK)
    {
        LE_WARN("Vendor name %s was truncated!",drvPtr->vendor);
    }

    //Fill the driver location info
    if(le_utf8_Copy(drvPtr->loc, fname, sizeof(drvPtr->loc), nullptr) != LE_OK)
    {
        LE_ERROR("Installing drive file %s failed!",fname);

        return LE_BAD_PARAMETER;
    }

    if(mgrInf->moduleType == TAF_MODULETYPE_HAL)
    {
        // Power on
        int ret = 0;
        ENTER_SAFE_CALL(TIMER_SAFECALL, ret, (*(mgrInf->powerOnInf)));
        EXIT_SAFE_CALL();

        if(ret == -1)
        {
            LE_ERROR("Called powerOnInf failed");
            return LE_TERMINATED;
        }

        // Hardware init
        ret = 0;
        ENTER_SAFE_CALL(TIMER_SAFECALL, ret, (*(mgrInf->hwInitInf)));
        EXIT_SAFE_CALL();

        if(ret == -1)
        {
            LE_ERROR("Called hwInitInf failed");
            return LE_TERMINATED;
        }
    }


    // Hard initializaiton done, make it ready
    drvPtr->status = GetDrvStatus(drvPtr);

    // put the driver to the driver list
    le_dls_Queue(&DriverList, &drvPtr->link);

    return LE_OK;
}

static bool ParseDrvInfo
(
    const char* drvInfo,        // IN
    char*       name,           // OUT
    bool*       ignoreVersion,  // OUT
    uint16_t*   majorVer,       // OUT
    uint16_t*	minorVer        // OUT
)
{
    char ver[DEV_MANAGER_DRIVER_VERSION_MAX_LEN];
    size_t nBytes_name, nBytes_majVer;

    // parameter format.
    // [cmd][driver name][:][majorVer][.][minorVer]
    // commandData contains name and ver seperate by ":"
    if(le_utf8_CopyUpToSubStr(name, &drvInfo[0], ":", DEV_MANAGER_DRIVER_NAME_MAX_LEN, &nBytes_name) != LE_OK)
    {
        LE_ERROR("Not valid drvInfo %s", drvInfo);
        return false;
    }

    LE_INFO("name: %s", name);

    // move to the next version string
    if(le_utf8_Copy(ver, &drvInfo[nBytes_name + 1], sizeof(ver), nullptr) != LE_OK)
    {
        LE_ERROR("Not valid drvInfo %s", drvInfo);
        return false;
    }

    LE_INFO("ver: %s", ver);

    // check if it is '*', means ignore the version
    if(ver[0] == '*')
    {
        LE_INFO("Ignore version");
        *ignoreVersion = true;
        return true;
    }
    else
    {
        char majorVerBuf[3] = {};
        char minorVerBuf[3] = {};
        int majorInt, minorInt;

        // copy major version
        if(le_utf8_CopyUpToSubStr(majorVerBuf, &ver[0], ".", sizeof(majorVerBuf), &nBytes_majVer) != LE_OK)
        {
            LE_ERROR("Not valid drvInfo %s", drvInfo);
            return false;
        }

        LE_DEBUG("majorVerBuf: %s", majorVerBuf);

        // copy minor version
        if(le_utf8_Copy(minorVerBuf, &ver[nBytes_majVer + 1], sizeof(minorVerBuf), nullptr) != LE_OK)
        {
            LE_ERROR("Not valid drvInfo %s", drvInfo);
            return false;
        }

        if(le_utf8_ParseInt(&majorInt, majorVerBuf) != LE_OK)
        {
            LE_ERROR("Not valid version %s", majorVerBuf);
            return false;
        }

        if(le_utf8_ParseInt(&minorInt, minorVerBuf) != LE_OK)
        {
            LE_ERROR("Not valid version %s", minorVerBuf);
            return false;
        }
        *ignoreVersion = false;

        *majorVer = (uint16_t)majorInt;
        *minorVer = (uint16_t)minorInt;

        LE_INFO("Version: %u.%u", *majorVer, *minorVer);
    }
    return true;
}

static bool ParseServiceReqPacket
(
    const char* reqPacketPtr,  // IN
    char*       req,           // OUT
    char*	    drvInfo        // OUT
)
{
    const char* payloadPtr = reqPacketPtr;
    size_t numBytes = 0;

    // the first byte is the command
    char reqByte = *payloadPtr++;

    if(reqByte == '\0')
    {
        LE_ERROR("Command code missed in the message!\n");
        return false;
    }

    if(payloadPtr != nullptr)
    {
        *req = reqByte;
    }

    // Check the commands
    switch(reqByte)
    {
        // return the loc to the client
        case DEV_MANAGER_SERVICE_OPEN_CMD:

        // The loc will be sent by the client to close the driver
        case DEV_MANAGER_SERVICE_CLOSE_CMD:

            // copy the second byte(driver info) to the buffer
            le_utf8_Copy(drvInfo, payloadPtr, DEV_MANAGER_CMD_MAX_LEN, &numBytes);

            if(numBytes == 0)
            {
                LE_ERROR("Command args missed in the message!\n");

                return false;
            }

            return true;

        default:
            LE_ERROR("Unknown command in the message!\n");

            return false;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process a message received from a connected session client.
 **/
//--------------------------------------------------------------------------------------------------
void ClientMsgReceiveHandler
(
    le_msg_MessageRef_t msgRef,     ///< [IN] Reference to the message received.
    void*               contextPtr  ///< Not used.
)
{
    // handle the install driver nad uninstall driver
    char req;
    char drvInfo[DEV_MANAGER_CMD_MAX_LEN]; // full path
    char name[DEV_MANAGER_DRIVER_NAME_MAX_LEN];
    bool ignoreVersion = false;
    uint16_t  majorVer, minorVer;

    DeviceDriver_t* drvPtr;
    char* msgPtrStart;

    le_msg_SessionRef_t clntSessionRef = le_msg_GetSession(msgRef);

    const char* rxBuf = (const char*)le_msg_GetPayloadPtr(msgRef);

    msgPtrStart = (char* )rxBuf;
    LE_INFO("received client msg");

    if(ParseServiceReqPacket(rxBuf, &req, drvInfo)==true)
    {
        LE_INFO("inside parse request");

        LE_INFO("drvInfo: %s", drvInfo);

        if(ParseDrvInfo(drvInfo, name, &ignoreVersion, &majorVer, &minorVer) == false)
        {
            LE_ERROR("Not valid remove cmd packet %s !", drvInfo);
            // reuse the message for response
            snprintf(msgPtrStart, le_msg_GetMaxPayloadSize(msgRef), "*-1");
            le_msg_Respond(msgRef);
            return;
        }

        drvPtr = FindDrv(name, majorVer, minorVer, ignoreVersion);

        switch (req)
        {
            case DEV_MANAGER_SERVICE_OPEN_CMD:

                LE_INFO("DEV_MANAGER_SERVICE_OPEN_CMD");
                if(drvPtr != nullptr)
                {
                    // return the so to the client, we do not maintain a session between client and device manager
                    // mark the driver in use and close the session
                    drvPtr->status = DEV_MANAGER_DRV_BUSY;

                    // check if the client has opened the driver, all the process info to driver list
                    if(!IsProcessUsingDriver(clntSessionRef, drvPtr))
                    {
                        // fill the pid into driver structure
                        StoreProcessInfoToDriver(clntSessionRef, drvPtr);
                    }

                    snprintf(msgPtrStart, le_msg_GetMaxPayloadSize(msgRef), "%s", drvPtr->loc);
                    le_msg_Respond(msgRef);
                }
                else // error start with '*'
                {
                    LE_ERROR("Cannot find the driver %s",name);
                    snprintf(msgPtrStart, le_msg_GetMaxPayloadSize(msgRef), "*-3");
                    le_msg_Respond(msgRef);
                }

                break;

            case DEV_MANAGER_SERVICE_CLOSE_CMD:

                LE_INFO("received DEV_MANAGER_SERVICE_CLOSE_CMD");
                if(drvPtr != nullptr)
                {
                    // if only one session is using the driver
                    if(GetProcessCountForDrv(drvPtr) == 1)
                    {
                        LE_INFO("Only this client is using the driver, set to IDLE state");
                        drvPtr->status = DEV_MANAGER_DRV_IDLE;
                    }
                    snprintf(msgPtrStart, le_msg_GetMaxPayloadSize(msgRef), "OK");
                    le_msg_Respond(msgRef);
                }
                else
                {
                    LE_ERROR("Cannot find the driver %s",name);
                    snprintf(msgPtrStart, le_msg_GetMaxPayloadSize(msgRef), "*-3");
                    le_msg_Respond(msgRef);
                }

                break;

            default:
                LE_ERROR("Unknown request type: %c",req);
                le_msg_CloseSession(clntSessionRef);
                return;
        }
    }

    // close the session, we do not want to keep connected with clients all the time
    le_msg_CloseSession(clntSessionRef);
    return;
}

static bool ParseToolCmdPacket
(
    const char* cmdPacktPtr, // IN
    char*  toolCmdPtr,       // OUT
    char*  cmdArgsPtr        // OUT
)
{
    const char* cmdPtr = cmdPacktPtr;
    size_t numBytes = 0;

    // the first byte is the command
    char  cmdByte = *cmdPtr++;

    if(cmdByte == '\0')
    {
        LE_ERROR("Command code missed in the message!\n");
        return false;
    }

    if(cmdPtr != nullptr)
    {
        *toolCmdPtr = cmdByte;
    }

    // Check the commands
    switch(cmdByte)
    {
        case DEV_MANAGER_TOOL_LIST_DRIVERS:

            // no other data
            *cmdArgsPtr = '\0';
            return true;

        case DEV_MANAGER_TOOL_QUERY_DRIVERS:
        case DEV_MANAGER_TOOL_INSTALL_CMD:
        case DEV_MANAGER_TOOL_REMOVE_CMD:
            // copy the 2nd byte(args) to the buffer
            le_utf8_Copy(cmdArgsPtr,cmdPtr,DEV_MANAGER_CMD_MAX_LEN,&numBytes);

            if(numBytes == 0)
            {
                LE_ERROR("Command args missed in the message!\n");

                return false;
            }

            return true;

        default:
            LE_ERROR("Unknown command in the message!\n");

            return false;

    }

    return false;
}

//---------------------------------------------------------------------------------------------------
/**
 * Process a message received from driver tool.
 * e.g. tafDriver [i/r/l/q] [path]
 **/
//---------------------------------------------------------------------------------------------------
void ToolMsgReceiveHandler
(
    le_msg_MessageRef_t msgRef,
    void*               contextPtr
)
{
    char toolCmd;
    char commandData[DEV_MANAGER_CMD_MAX_LEN];
    const char* baseName;
    char newName[DEV_MANAGER_DRIVER_NAME_MAX_LEN]="";
    char tempName[DEV_MANAGER_DRIVER_NAME_MAX_LEN]="";
    char respPtr[DEV_MANAGER_MAX_RESP_MSG_BYTES] = "";

    DeviceDriver_t* drvPtr = nullptr;
    DeviceDriver_t* tempDrvPtr = nullptr;

    struct stat sb;
    bool installingNewFile = true;

    TAF_HAL_MGR_INF_t *mgrInf;

    void* drvHandle = nullptr;
    char* errMsg = nullptr;
    const char* drvName;
    bool ignoreVersion;
    uint16_t  majorVer, minorVer;

    int ret = 0;
    le_result_t res = LE_OK;

    le_msg_SessionRef_t ipcSessionRef = le_msg_GetSession(msgRef);

    const char* rxBufPtr = (const char *) le_msg_GetPayloadPtr(msgRef);

    if(ParseToolCmdPacket(rxBufPtr, &toolCmd, commandData) == true)
    {
        switch (toolCmd)
        {
            case DEV_MANAGER_TOOL_INSTALL_CMD:

                // to be safe
                commandData[DEV_MANAGER_CMD_MAX_LEN - 1] = '\0';

                LE_INFO("Device Manager - install the driver %s", commandData);

                // install it to the device manager storage
                baseName = le_path_GetBasenamePtr(commandData, "/");

                // create a new path
                if(le_path_Concat("/", newName, sizeof(newName), DEV_MANAGER_DRIVER_STORAGE, baseName, nullptr) != LE_OK)
                {
                    // respond to driver tool with error message
                    LE_ERROR("***ERROR: Failed to install the driver %s", baseName);
                    snprintf(respPtr, sizeof(respPtr), "***ERROR: Failed to install the driver %s", newName);
                    SendToDrvTool(ipcSessionRef, respPtr);
                    break;
                }

                // check if the newName exist or not, install it with a hard link
                if (stat(newName, &sb) == 0)
                {
                    LE_INFO("file exists");

                    // check if it's a regular file
                    if((sb.st_mode & S_IFMT) == S_IFREG)
                    {
                        installingNewFile = false;
                        LE_WARN("Module file %s exist, continue to use it", newName);
                    }
                    else
                    {
                        LE_ERROR("Remove the unknown objects %s",newName);
                        unlink(newName);
                        snprintf(respPtr, sizeof(respPtr), "***ERROR: Unknown object exists");
                        SendToDrvTool(ipcSessionRef, respPtr);
                        break;
                    }
                }
                else if(link(commandData, newName) != 0)
                {
                    LE_ERROR("Cannot install it to the dev manager storage %s", newName);
                    LE_ERROR("error: %s", strerror(errno));
                    snprintf(respPtr, sizeof(respPtr), "***ERROR: Failed to install it to device manager");
                    SendToDrvTool(ipcSessionRef, respPtr);
                    break;
                }

                LE_INFO("link successfull and %d is stat",stat(newName,&sb));
#ifdef LE_CONFIG_ENABLE_SELINUX
                // Set selinux context to the installed driver
                if(setfilecon(newName, DEV_MANAGER_STORAGE_CONTEXT) != 0)
                {
                    LE_ERROR("Failed to change SELinux context");
                }
#endif
                // Open driver and get information
                if (OpenDrvToGetInfo(newName, &drvPtr, &drvName, &majorVer, &minorVer, respPtr) != LE_OK)
                {
                    SendToDrvTool(ipcSessionRef, respPtr);
                    break;
                }

                LE_INFO("Start to install the driver");

                // Install driver
                res = InstallDrvToDevManager(drvPtr, drvName, majorVer, minorVer, newName);
                if(res == LE_OK)
                {
                    LE_INFO("Driver %s installed successfully!", drvName);
                    snprintf(respPtr, sizeof(respPtr),
                             "TelAF HAL module %s:%u.%u installed successfully",
                             drvName, majorVer, minorVer);
                    SendToDrvTool(ipcSessionRef, respPtr);

                    drvName = nullptr;

                    // we can not close the handle immediately, do not relase
                    le_event_QueueFunction(SafeCloseDrv, drvPtr, (void*)false);

                    LE_INFO("Successfully close the handle");
                }
                else
                {
                    LE_ERROR("Fail to install the driver");
                    if(res == LE_DUPLICATE)
                    {
                        snprintf(respPtr, sizeof(respPtr), "***ERROR: Driver already exists");
                    }
                    else if(res == LE_TERMINATED)
                    {
                        snprintf(respPtr, sizeof(respPtr), "***ERROR: Failed to call driver function");
                    }
                    else
                    {
                        unlink(newName);
                        snprintf(respPtr, sizeof(respPtr), "***ERROR: Failed to install it to device manager");
                    }

                    if(installingNewFile)
                    {
                        unlink(newName);
                    }

                    SendToDrvTool(ipcSessionRef, respPtr);
                    le_event_QueueFunction(SafeCloseDrv, drvPtr, (void*)true);
                }

                break;

            case DEV_MANAGER_TOOL_REMOVE_CMD:

                LE_INFO("Device Manager - remove the driver\n");

                // to be safe
                commandData[DEV_MANAGER_CMD_MAX_LEN-1] = '\0';

                if(ParseDrvInfo(commandData, tempName, &ignoreVersion, &majorVer, &minorVer) == false)
                {
                    LE_ERROR("Not valid remove cmd packet %s !",commandData);
                    snprintf(respPtr, sizeof(respPtr), "***ERROR: Not a vaild command");
                    SendToDrvTool(ipcSessionRef,respPtr);
                    break;
                }

                // search the driver list
                drvPtr = FindDrv(tempName, majorVer, minorVer, ignoreVersion);

                // go ahead uninstalling the driver
                if(drvPtr != nullptr)
                {
                    // open the so to get real name
                    drvHandle = dlopen(drvPtr->loc, RTLD_NOW);

                    if((errMsg = dlerror()) != nullptr || drvHandle == nullptr)
                    {
                        LE_ERROR("Failed to load the driver %s %s",drvPtr->loc, errMsg);
                        snprintf(respPtr, sizeof(respPtr), "***ERROR: Failed to load the driver: %s",
                                    errMsg ? errMsg : "mgrInf is null");
                        SendToDrvTool(ipcSessionRef,respPtr);

                        //remove the driver
                        unlink(drvPtr->loc);
                        break;
                    }

                    mgrInf = (TAF_HAL_MGR_INF_t *)dlsym(drvHandle, TAF_HAL_INFO_TAB_STR);

                    if((errMsg = dlerror()) != nullptr || mgrInf == nullptr)
                    {
                        LE_ERROR("No HAL Information table");

                        // close later
                        snprintf(respPtr, sizeof(respPtr), "***ERROR: Invalid TelAF HAL module: %s",
                                    errMsg ? errMsg : "mgrInf is null");
                        SendToDrvTool(ipcSessionRef, respPtr);

                        // we need to close this so
                        tempDrvPtr = (DeviceDriver_t *)le_mem_ForceAlloc(DeviceDriverPoolRef);

                        if(tempDrvPtr == nullptr)
                        {
                            LE_CRIT("Not enough memory!");

                            // simply close session and return as we can not close it properly
                            break;
                        }

                        // close it and return
                        tempDrvPtr->drvHandle = drvHandle;
                        le_event_QueueFunction(SafeCloseDrv, tempDrvPtr, (void*)true);

                        break;
                    }

                    // save the handle for later use
                    drvPtr->drvHandle = drvHandle;

                    // 1 check if there is service is using the driver
                    if(GetDrvStatus(drvPtr) != DEV_MANAGER_DRV_IDLE)
                    {
                        LE_ERROR("Driver is in use!");
                        snprintf(respPtr, sizeof(respPtr), "***ERROR: driver in use");
                        SendToDrvTool(ipcSessionRef, respPtr);

                        // close it and keep this node in the list
                        le_event_QueueFunction(SafeCloseDrv, drvPtr, (void*)false);

                        break;
                    }

                    if(mgrInf != nullptr && mgrInf->moduleType == TAF_MODULETYPE_HAL)
                    {
                        // 2 turn off the power
                        ENTER_SAFE_CALL(TIMER_SAFECALL, ret, (*(mgrInf->powerOffInf)));
                        EXIT_SAFE_CALL();
                        if(ret == -1)
                        {
                            LE_ERROR("Called powerOnInf failed");
                        }
                    }
                }
                else
                {
                    // close later
                    snprintf(respPtr, sizeof(respPtr), "***ERROR: TelAF HAL module is not found");
                    SendToDrvTool(ipcSessionRef, respPtr);
                    break;
                }

                // continue to remove it from the driver list
                if(drvPtr != nullptr)
                {
                    le_dls_Remove(&DriverList, &(drvPtr->link));
                    unlink(drvPtr->loc);

                    snprintf(respPtr, sizeof(respPtr), "Telaf HAL Module was removed successfully");
                    SendToDrvTool(ipcSessionRef, respPtr);

                    // close the handle and release the memory
                    le_event_QueueFunction(SafeCloseDrv, drvPtr, (void*)true);
                    LE_INFO("Driver %s was uninstalled successfully!", drvPtr->loc);
                }
                else // not found
                {
                    // remove those driver not managered by device manager
                    // simply remove the file link
                    LE_ERROR("Cannot find the driver, please delete the file under device manager storage");
                    snprintf(respPtr, sizeof(respPtr), "***ERROR: TelAF HAL module not found, please delete the file directly");
                    SendToDrvTool(ipcSessionRef, respPtr);

                    // close this so safely
                    tempDrvPtr = (DeviceDriver_t *)le_mem_ForceAlloc(DeviceDriverPoolRef);

                    // close it and return
                    tempDrvPtr->drvHandle = drvHandle;
                    le_event_QueueFunction(SafeCloseDrv, tempDrvPtr, (void*)true);
                }

                 break;

            case DEV_MANAGER_TOOL_LIST_DRIVERS:
                LE_INFO("Device Manager - list all the available drivers");

                ListDrivers(respPtr, sizeof(respPtr));
                SendToDrvTool(ipcSessionRef, respPtr);

                break;

            case DEV_MANAGER_TOOL_QUERY_DRIVERS:

                LE_INFO("Device Manager - check the driver status\n");
                break;

            default:

                LE_ERROR("Not supported command!");
                le_msg_CloseSession(ipcSessionRef);
                return;
        }
    }

    le_msg_CloseSession(ipcSessionRef);
    le_msg_ReleaseMsg(msgRef);

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copy file in chunks
 */
//--------------------------------------------------------------------------------------------------
le_result_t copyFile(const char* sourcePath, const char* destPath)
{
    FILE* sourceFile = fopen(sourcePath, "rb");
    if (sourceFile == nullptr) {
        LE_ERROR("Error opening source file");
        return LE_FAULT;
    }

    FILE* destFile = fopen(destPath, "wb");
    if (destFile == nullptr) {
        LE_ERROR("Error opening destination file");
        fclose(sourceFile);
        return LE_FAULT;
    }

    // Read and write in chunks
    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0)
    {
        fwrite(buffer, 1, bytesRead, destFile);
    }

    fclose(sourceFile);
    fclose(destFile);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * List the drivers in temp file, and copy devManager storage
 */
//--------------------------------------------------------------------------------------------------
static void ScanStoredDriversAndCopy(const char* path)
{
    struct dirent **fileList;
    int n = 0;
    n = scandir(path, &fileList, nullptr, alphasort);

    while (n > 0)
    {
        LE_INFO("file name: %s", fileList[n - 1]->d_name);
        n--;

        char srcStr[DEV_MANAGER_DRIVER_LOCATION_MAX_LEN];
        snprintf(srcStr, sizeof(srcStr), "%s%s", path,
        le_path_GetBasenamePtr(fileList[n]->d_name, "/"));

        struct stat sb;
        if (stat(srcStr, &sb) == 0)
        {
            // check if it is a regular file
            if((sb.st_mode & S_IFMT) == S_IFREG || (sb.st_mode & S_IFMT) == S_IFLNK)
            {
                LE_INFO("driver file %s exist", srcStr);
            }
            else
            {
                break;
            }
        }

        char destStr[DEV_MANAGER_DRIVER_LOCATION_MAX_LEN];
        snprintf(destStr, sizeof(destStr), "%s%s", DEV_MANAGER_TMP_STORAGE,
                 le_path_GetBasenamePtr(fileList[n]->d_name, "/"));

        char linkStr[DEV_MANAGER_DRIVER_LOCATION_MAX_LEN];
        snprintf(linkStr, sizeof(linkStr), "%s%s", DEV_MANAGER_DRIVER_STORAGE,
                 le_path_GetBasenamePtr(fileList[n]->d_name, "/"));

        // Determine the target of the symbolic link
        char targetPath[DEV_MANAGER_DRIVER_LOCATION_MAX_LEN];
        ssize_t len = readlink(srcStr, targetPath, sizeof(targetPath) - 1);
        if (len == -1)
        {
            LE_WARN("Error reading symbolic link, try to copy the file directly");
            snprintf(targetPath, sizeof(targetPath), "%s", srcStr);
        }
        else
        {
            targetPath[len] = '\0';
        }

        LE_INFO("Try to copy %s to %s", targetPath, destStr);

        le_result_t res = copyFile(targetPath, destStr);
        if(res != LE_OK)
        {
            LE_INFO("Copy driver %s failed(%d)", fileList[n]->d_name, res);
        }
        else
        {
            LE_INFO("Try to create hard link to %s", linkStr);
            link(destStr, linkStr);

            // remove the copied file, only keep the hard link
            unlink(destStr);

#ifdef LE_CONFIG_ENABLE_SELINUX
            // Set selinux context to created folder
            if(setfilecon(linkStr, DEV_MANAGER_STORAGE_CONTEXT) != 0)
            {
                LE_ERROR("Failed to change SELinux context");
            }
#endif
        }
    }
}

static void InstallPersistentDrivers()
{
    struct dirent **driverList;
    int n = 0;
    n = scandir(DEV_MANAGER_DRIVER_STORAGE, &driverList, nullptr, alphasort);

    while (n > 0)
    {
        LE_INFO("driver name: %s", driverList[n - 1]->d_name);
        n--;

        char srcStr[DEV_MANAGER_DRIVER_LOCATION_MAX_LEN];
        snprintf(srcStr, sizeof(srcStr), "%s%s", DEV_MANAGER_DRIVER_STORAGE,
        le_path_GetBasenamePtr(driverList[n]->d_name, "/"));

        struct stat sb;

        char respPtr[DEV_MANAGER_MAX_RESP_MSG_BYTES] = "";
        DeviceDriver_t* drvPtr = nullptr;
        const char* drvName;
        uint16_t  majorVer, minorVer;
        bool freeDrv = false;

        if (stat(srcStr, &sb) == 0)
        {
            // check if it's a symbolic link
            if((sb.st_mode & S_IFMT) == S_IFDIR)
            {
                break;
            }
            else
            {
                LE_INFO("driver file %s exist", srcStr);
            }
        }

        if (OpenDrvToGetInfo(srcStr, &drvPtr, &drvName, &majorVer, &minorVer, respPtr) != LE_OK)
        {
            LE_ERROR("Open/Load driver: %s failed", srcStr);
            break;
        }

        LE_INFO("Start to install the driver");

        if(InstallDrvToDevManager(drvPtr, drvName, majorVer, minorVer, srcStr) == LE_OK)
        {
            LE_INFO("Driver %s installed successfully!", drvName);
        }
        else
        {
            LE_INFO("Fail to install the driver");
            freeDrv = true;
        }

        // we can not close the handle immediately, do not relase
        le_event_QueueFunction(SafeCloseDrv, drvPtr, (void*)freeDrv);
    }
}

static void ClientIpcSessionClosed
(
    le_msg_SessionRef_t ipcSessionRef,
    void*               contextPtr
)
{
    LE_INFO("Client session %p is closed", ipcSessionRef);
    return;
}

#ifdef __cplusplus
    }
#endif


//--------------------------------------------------------------------------------------------------
/**
 * The main function for the device manager.  Listens for commands from process/components and tools
 * and processes the commands.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("COMPONENT_INIT - DeviceManager");

    //Create the storage for installing drivers
    struct stat sb;

    // assume /data/persist dir exists in the system
    if(stat(DEV_MANAGER_STORAGE, &sb) == -1)
    {
        LE_INFO("Dev Manager storage dir does not exist, create it.");

        if(mkdir(DEV_MANAGER_STORAGE, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", DEV_MANAGER_STORAGE);
            exit(-1);
        }

#ifdef LE_CONFIG_ENABLE_SELINUX
        // Set selinux context to created folder
        if(setfilecon(DEV_MANAGER_STORAGE, DEV_MANAGER_STORAGE_CONTEXT) != 0)
        {
            LE_ERROR("Failed to change SELinux context");
        }
#endif

        // go ahead creating the sub dir
        if(mkdir(DEV_MANAGER_DRIVER_STORAGE, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s",DEV_MANAGER_STORAGE);
            exit(-1);
        }

        // go ahead creating the sub dir
        if(mkdir(DEV_MANAGER_TMP_STORAGE,0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s",DEV_MANAGER_TMP_STORAGE);
            exit(-1);
        }
    }
    else if((sb.st_mode & S_IFMT) == S_IFDIR)
    {
        // assume the sub dir was created as well. do nothing
        LE_INFO("Dev Manager storage was found!");
    }
    else
    {
        // some other file objects. delete first
        LE_ERROR("Delete the file object, then create the storage");

        // try to delete it
        unlink(DEV_MANAGER_STORAGE);

        // Create the directory
        if(mkdir(DEV_MANAGER_STORAGE,0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s",DEV_MANAGER_STORAGE);
            exit(-1);
        }

        // go ahead creating the sub dir
        if(mkdir(DEV_MANAGER_DRIVER_STORAGE,0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s",DEV_MANAGER_STORAGE);
            exit(-1);
        }

        // go ahead creating the sub dir
        if(mkdir(DEV_MANAGER_TMP_STORAGE,0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s",DEV_MANAGER_TMP_STORAGE);
            exit(-1);
        }
    }

    // Create the memory pools.
    DeviceDriverPoolRef = le_mem_CreatePool("DeviceDriver", sizeof(DeviceDriver_t));

    // Create the client PId memory pools
    ProcessInfoPoolRef = le_mem_CreatePool("ProcessInfo", sizeof(ProcessInfo_t));

    // Tune the pools' initial sizes.
    le_mem_ExpandPool(DeviceDriverPoolRef, 300); // 300 drivers supported max

    // Tune the pools' initial sizes.
    le_mem_ExpandPool(ProcessInfoPoolRef, 20); // 20 clients supported max

    // load driver from configTree before service- try to leverage mdef/modules

    // Get a reference to the Protocol identification.
    le_msg_ProtocolRef_t protocolRef = le_msg_GetProtocolRef(DEV_MANAGER_PROTOCOL_ID,
                                                             DEV_MANAGER_MAX_CMD_PACKET_BYTES);

    // Create and advertise the north bound interface for kinds of services.
    le_msg_ServiceRef_t serviceRef = le_msg_CreateService(protocolRef, DEV_MANAGER_SERVICE_NAME);
    le_msg_SetServiceRecvHandler(serviceRef, ClientMsgReceiveHandler, nullptr);
    le_msg_AddServiceCloseHandler(serviceRef, ClientIpcSessionClosed, nullptr);
    le_msg_AdvertiseService(serviceRef);

    // Create and advertise service for the tools like tafDrivers
    serviceRef = le_msg_CreateService(protocolRef, DEV_MANAGER_TOOL_NAME);
    le_msg_SetServiceRecvHandler(serviceRef,ToolMsgReceiveHandler, nullptr);
    le_msg_AdvertiseService(serviceRef);

    ScanStoredDriversAndCopy(DRIVER_TMP_STORAGE);

    InstallPersistentDrivers();

    // Close the fd that we inherited from the Supervisor.  This will let the Supervisor know that
    // we are initialized.  Then re-open it to /dev/null so that it cannot be reused later.
    FILE* filePtr;
    do
    {
        filePtr = freopen("/dev/null", "r", stdin);
    }
    while ( (filePtr == nullptr) && (errno == EINTR) );

    LE_FATAL_IF(filePtr == nullptr, "Failed to redirect standard in to /dev/null.  %m.");

    LE_INFO("Device Manager ready.");
}
