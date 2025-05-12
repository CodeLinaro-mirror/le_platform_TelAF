/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "tafAppMgmt.hpp"

using namespace tafsvc;

/*======================================================================
 FUNCTION        taf_appMgmt_CreateAppList
 DESCRIPTION     Create an app list
 PARAMETERS      void
 RETURN VALUE    taf_appMgmt_AppListRef_t: App list reference
======================================================================*/
taf_appMgmt_AppListRef_t taf_appMgmt_CreateAppList(void)
{
    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn(TAF_APPMGMT_SYSTEM_APPS);
    if (le_cfg_GoToFirstChild(cfgIter) == LE_NOT_FOUND) {
        LE_ERROR("No apps installed.");
        le_cfg_CancelTxn(cfgIter);
        return nullptr;
    }

    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    taf_AppMgmtAppList_t* appList = (taf_AppMgmtAppList_t*)le_mem_ForceAlloc(tafAppMgmt.appListPool);
    appList->appList = LE_SLS_LIST_INIT;
    appList->safeRefList = LE_SLS_LIST_INIT;
    appList->currPtr = NULL;

    do {
        // Get app name
        char appName[TAF_APPMGMT_APP_NAME_BYTES];
        if (le_cfg_GetNodeName(cfgIter, "", appName, TAF_APPMGMT_APP_NAME_BYTES) != LE_OK) {
            LE_ERROR("Failed to get app name.");
            le_cfg_CancelTxn(cfgIter);
            return nullptr;
        }

        if (!tafAppMgmt.IsSysApp(appName))
        {
            // Get app version
            char appVersion[TAF_APPMGMT_APP_VERSION_BYTES];
            if (le_cfg_GetString(cfgIter, "version", appVersion, TAF_APPMGMT_APP_VERSION_BYTES, "") != LE_OK)
            {
                LE_ERROR("Failed to get %s version.", appName);
                break;
            }

            // Get app hash
            char appHash[TAF_APPMGMT_APP_HASH_BYTES];
            if (le_appInfo_GetHash(appName, appHash, TAF_APPMGMT_APP_HASH_BYTES) != LE_OK)
            {
                LE_ERROR("Fail to get %s hash.", appName);
                break;
            }

            taf_AppMgmtAppInfo_t* appInfoPtr = (taf_AppMgmtAppInfo_t*)le_mem_ForceAlloc(tafAppMgmt.appInfoPool);
            le_utf8_Copy(appInfoPtr->name, appName, TAF_APPMGMT_APP_NAME_BYTES, NULL);
            le_utf8_Copy(appInfoPtr->version, appVersion, TAF_APPMGMT_APP_VERSION_BYTES, NULL);
            le_utf8_Copy(appInfoPtr->hash, appHash, TAF_APPMGMT_APP_HASH_BYTES, NULL);

            // Get app start mode
            appInfoPtr->isStartManual = le_cfg_GetBool(cfgIter, "startManual", false);

            // If app is sandboxed
            appInfoPtr->isSandboxed = le_cfg_GetBool(cfgIter, "sandboxed", true);

            // If app is activated
            appInfoPtr->isActivated = le_cfg_GetBool(cfgIter, "activated", true);

            appInfoPtr->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&(appList->appList), &(appInfoPtr->link));
        }
    } while (le_cfg_GoToNextSibling(cfgIter) == LE_OK);

    le_cfg_CancelTxn(cfgIter);

    return (taf_appMgmt_AppListRef_t)le_ref_CreateRef(tafAppMgmt.appListRefMap, (void*)appList);
}

/*======================================================================
 FUNCTION        taf_appMgmt_DeleteAppList
 DESCRIPTION     Delete an app list
 PARAMETERS      [IN] appListRef: App list reference
 RETURN VALUE    le_result_t: Result of deleting app list
======================================================================*/
le_result_t taf_appMgmt_DeleteAppList(taf_appMgmt_AppListRef_t appListRef)
{
    TAF_ERROR_IF_RET_VAL(appListRef == nullptr, LE_BAD_PARAMETER,
        "Null reference(appListRef)");

    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    taf_AppMgmtAppList_t* appListPtr = (taf_AppMgmtAppList_t*)le_ref_Lookup(tafAppMgmt.appListRefMap,
        appListRef);
    TAF_ERROR_IF_RET_VAL(appListPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    taf_AppMgmtAppInfo_t* appInfoPtr;
    le_sls_Link_t *linkPtr;
    while ((linkPtr = le_sls_Pop(&(appListPtr->appList))) != NULL) {
        appInfoPtr = CONTAINER_OF(linkPtr, taf_AppMgmtAppInfo_t, link);
        le_mem_Release(appInfoPtr);
    }

    taf_AppMgmtAppInfoSafeRef_t* safeRefPtr;
    while ((linkPtr = le_sls_Pop(&(appListPtr->safeRefList))) != NULL) {
        safeRefPtr = CONTAINER_OF(linkPtr, taf_AppMgmtAppInfoSafeRef_t, link);
        le_ref_DeleteRef(tafAppMgmt.appInfoSafeRefMap, safeRefPtr->safeRef);
        le_mem_Release(safeRefPtr);
    }

    le_ref_DeleteRef(tafAppMgmt.appListRefMap, appListRef);

    le_mem_Release(appListPtr);

    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_appMgmt_GetFirstApp
 DESCRIPTION     Get reference of the first app
 PARAMETERS      [IN] appListRef: App list reference
 RETURN VALUE    le_result_t: Result of getting app reference
======================================================================*/
taf_appMgmt_AppRef_t taf_appMgmt_GetFirstApp(taf_appMgmt_AppListRef_t appListRef)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    taf_AppMgmtAppList_t* appListPtr = (taf_AppMgmtAppList_t*)le_ref_Lookup(tafAppMgmt.appListRefMap,
        appListRef);

    TAF_ERROR_IF_RET_VAL(appListPtr == nullptr, nullptr,
        "Failed to look up the reference:%p", appListRef);

    le_sls_Link_t* linkPtr = le_sls_Peek(&(appListPtr->appList));
    TAF_ERROR_IF_RET_VAL(linkPtr == nullptr, nullptr, "Empty list");

    taf_AppMgmtAppInfo_t* appInfoPtr = CONTAINER_OF(linkPtr, taf_AppMgmtAppInfo_t, link);
    appListPtr->currPtr = linkPtr;

    taf_AppMgmtAppInfoSafeRef_t* safeRefPtr = (taf_AppMgmtAppInfoSafeRef_t*)le_mem_ForceAlloc(tafAppMgmt.appInfoSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(tafAppMgmt.appInfoSafeRefMap, (void*)appInfoPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(appListPtr->safeRefList), &(safeRefPtr->link));

    return (taf_appMgmt_AppRef_t)safeRefPtr->safeRef;
}

/*======================================================================
 FUNCTION        taf_appMgmt_GetNextApp
 DESCRIPTION     Get reference of next app
 PARAMETERS      [IN] appListRef: App list reference
 RETURN VALUE    le_result_t: Result of getting app reference
======================================================================*/
taf_appMgmt_AppRef_t taf_appMgmt_GetNextApp(taf_appMgmt_AppListRef_t appListRef)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    taf_AppMgmtAppList_t* appListPtr = (taf_AppMgmtAppList_t*)le_ref_Lookup(tafAppMgmt.appListRefMap,
        appListRef);

    TAF_ERROR_IF_RET_VAL(appListPtr == nullptr, nullptr,
        "Failed to look up the reference:%p", appListRef);

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&(appListPtr->appList), appListPtr->currPtr);
    if (linkPtr == nullptr) {
        LE_WARN("Reach to the end of list.");
        return nullptr;
    }

    taf_AppMgmtAppInfo_t* appInfoPtr = CONTAINER_OF(linkPtr, taf_AppMgmtAppInfo_t, link);
    appListPtr->currPtr = linkPtr;

    taf_AppMgmtAppInfoSafeRef_t* safeRefPtr = (taf_AppMgmtAppInfoSafeRef_t*)le_mem_ForceAlloc(tafAppMgmt.appInfoSafeRefPool);
    safeRefPtr->safeRef = le_ref_CreateRef(tafAppMgmt.appInfoSafeRefMap, (void*)appInfoPtr);
    safeRefPtr->link = LE_SLS_LINK_INIT;
    le_sls_Queue(&(appListPtr->safeRefList) ,&(safeRefPtr->link));

    return (taf_appMgmt_AppRef_t)safeRefPtr->safeRef;
}

/*======================================================================
 FUNCTION        taf_appMgmt_GetAppDetails
 DESCRIPTION     Get the detail information of an app
 PARAMETERS      [IN]  appInfoRef: App reference
                 [OUT] appInfoPtr: App information
 RETURN VALUE    le_result_t: Result of getting app details
======================================================================*/
le_result_t taf_appMgmt_GetAppDetails(taf_appMgmt_AppRef_t appInfoRef, taf_appMgmt_AppInfo_t* appInfoPtr)
{
    TAF_ERROR_IF_RET_VAL(appInfoRef == nullptr, LE_BAD_PARAMETER, "Null reference(appInfoRef)");

    TAF_ERROR_IF_RET_VAL(appInfoPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(appInfoPtr)");

    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    taf_AppMgmtAppInfo_t* infoPtr = (taf_AppMgmtAppInfo_t*)le_ref_Lookup(tafAppMgmt.appInfoSafeRefMap, appInfoRef);
    TAF_ERROR_IF_RET_VAL(infoPtr == nullptr, LE_NOT_FOUND, "Invalid para(null reference ptr)");

    le_utf8_Copy(appInfoPtr->name, infoPtr->name, TAF_APPMGMT_APP_NAME_BYTES, NULL);
    le_utf8_Copy(appInfoPtr->version, infoPtr->version, TAF_APPMGMT_APP_VERSION_BYTES, NULL);
    le_utf8_Copy(appInfoPtr->hash, infoPtr->hash, TAF_APPMGMT_APP_HASH_BYTES, NULL);

    appInfoPtr->startMode = infoPtr->isStartManual ? TAF_APPMGMT_START_MANUAL : TAF_APPMGMT_START_AUTO;
    appInfoPtr->isSandboxed = infoPtr->isSandboxed;
    appInfoPtr->isActivated = infoPtr->isActivated;

    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_appMgmt_GetState
 DESCRIPTION     Get the app running state
 PARAMETERS      [IN] appName: App name
 RETURN VALUE    taf_appMgmt_AppState_t: Started or stopped
======================================================================*/
taf_appMgmt_AppState_t taf_appMgmt_GetState(const char* appName)
{
    return (taf_appMgmt_AppState_t)le_appInfo_GetState(appName);
}

/*======================================================================
 FUNCTION        taf_appMgmt_GetVersion
 DESCRIPTION     Get the app version
 PARAMETERS      [IN] appName: App name
                 [OUT] versionPtr: App version
                 [IN] versionNumElements: App version length
 RETURN VALUE    le_result_t: Result of getting app version
======================================================================*/
le_result_t taf_appMgmt_GetVersion(const char* appName, char* versionPtr, size_t versionNumElements)
{
    TAF_ERROR_IF_RET_VAL(appName == nullptr, LE_BAD_PARAMETER, "Null ptr(appName)");

    TAF_ERROR_IF_RET_VAL(versionPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(versionPtr)");

    auto &tafAppMgmt = taf_AppMgmt::GetInstance();
    return tafAppMgmt.GetAppVersion(appName, versionPtr, versionNumElements);
}

/*======================================================================
 FUNCTION        taf_appMgmt_Start
 DESCRIPTION     Start an app
 PARAMETERS      [IN] appName: App name
 RETURN VALUE    le_result_t: Result of starting an app
======================================================================*/
le_result_t taf_appMgmt_Start(const char* appName)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();

    // Start probation if app is not activated.
    if (!tafAppMgmt.IsActivated(appName) && tafAppMgmt.prbtTime)
    {
        taf_AppMgmtUpdateReq_t updateReq;
        updateReq.event = TAF_APPMGMT_EV_PROBATION;
        le_utf8_Copy(updateReq.appName, appName, TAF_APPMGMT_APP_NAME_BYTES, NULL);
        le_event_Report(tafAppMgmt.appUpdateEvId, &updateReq, sizeof(taf_AppMgmtUpdateReq_t));
        return LE_OK;
    }

    return le_appCtrl_Start(appName);
}

/*======================================================================
 FUNCTION        taf_appMgmt_Stop
 DESCRIPTION     Stop an app
 PARAMETERS      [IN] appName: App name
 RETURN VALUE    le_result_t: Result of stopping an app
======================================================================*/
le_result_t taf_appMgmt_Stop(const char* appName)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();

    // Can not stop app during installation, probation or rollback.
    if (strcmp(appName, tafAppMgmt.appName) == 0 && tafAppMgmt.state != TAF_UPDATE_IDLE)
    {
        LE_ERROR("Can not stop app %s during installation, probation or rollback.", appName);
        return LE_FAULT;
    }

    return le_appCtrl_Stop(appName);
}

/*======================================================================
 FUNCTION        taf_appMgmt_Uninstall
 DESCRIPTION     Uninstall an app
 PARAMETERS      [IN] appName: App name
 RETURN VALUE    le_result_t: Result of removing an app
======================================================================*/
le_result_t taf_appMgmt_Uninstall(const char* appName)
{
    auto &tafAppMgmt = taf_AppMgmt::GetInstance();

    // Can not uninstall app during installation, probation or rollback.
    if (strcmp(appName, tafAppMgmt.appName) == 0 && tafAppMgmt.state != TAF_UPDATE_IDLE)
    {
        LE_ERROR("Can not uninstall app %s during installation, probation or rollback.", appName);
        return LE_FAULT;
    }

    char file[PATH_MAX] = {0};
    snprintf(file, sizeof(file), TAF_APPMGMT_APP_BACKUP_PATH, appName);

    if (access(file, 0) == 0)
    {
        remove(file);
        LE_INFO("%s removed.", file);
    }

    return le_appRemove_Remove(appName);
}

