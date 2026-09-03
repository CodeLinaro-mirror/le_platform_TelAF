/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafKeyStore.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Check if the string follows POSIX file name format. It only contains character in [A-Za-z0-9._-].
 */
//--------------------------------------------------------------------------------------------------
static bool IsPosixFileNameFormat(const char* str)
{
    while (*str)
    {
        if ((!isalnum((unsigned char)*str)) &&
            (*str != '_') &&
            (*str != '.') &&
            (*str != '-'))
        {
            return false;
        }

        str++;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write a boot KPI marker.
 */
//--------------------------------------------------------------------------------------------------
static void SetBootKpiMarker(const char* markerPtr)
{
    const char *kpi_file = "/sys/kernel/boot_kpi/kpi_values";
    FILE *file = fopen(kpi_file, "w");
    if (file == NULL)
    {
        LE_ERROR("%s not able to open due to %s", kpi_file, strerror(errno));
        return;
    }
    if (fwrite(markerPtr, sizeof(char), strlen(markerPtr), file) != strlen(markerPtr))
    {
        LE_ERROR("failed to write %s to %s", markerPtr, kpi_file);
    }
    fclose(file);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the singleton instance of the KeyStore service.
 */
//--------------------------------------------------------------------------------------------------
taf_KeyStore& taf_KeyStore::GetInstance()
{
    static taf_KeyStore instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Search a provisoned key by key file reference.
 */
//--------------------------------------------------------------------------------------------------
taf_ks_Key_t* taf_KeyStore::SearchProvisionedKey
(
    KeyMgt_KeyFileRef_t keyFileRef ///< [IN] Key file reference
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(KeyRefMap_);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_GetValue(iterRef));

        if ((keyPtr != NULL) &&
            (keyPtr->keyType == KS_PROVISIONED_KEY) &&
            (keyPtr->proKey.keyFileRef == keyFileRef))
        {
            LE_DEBUG("Found a provisioned key(%p) for key file(%p).",
                    keyPtr->keyRef, keyPtr->proKey.keyFileRef);

            return keyPtr;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Search a new created key(not provisioned yet) by key ID.
 */
//--------------------------------------------------------------------------------------------------
taf_ks_Key_t* taf_KeyStore::SearchNewKey
(
    const char* keyId ///< [IN] Key ID
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(KeyRefMap_);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_GetValue(iterRef));

        if ((keyPtr != NULL) &&
            (keyPtr->keyType == KS_NEW_CREATED_KEY) &&
            (keyPtr->newKey.clientSessionRef == taf_ks_GetClientSessionRef()) &&
            (0 == strcmp(keyId, keyPtr->newKey.keyId)))
        {
            LE_DEBUG("Found a new key(%p) for keyId '%s' of client(%p).", keyPtr->keyRef, keyId,
                    keyPtr->newKey.clientSessionRef);

            return keyPtr;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set or update a specified tag for a new key.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::SetTag
(
    taf_ks_Key_t*    keyPtr,    ///< [IN] Key pointer
    KeyTag_t* setTagPtr  ///< [IN] Tag pointer
)
{
    KeyTag_t* tagPtr = NULL;

    if ((setTagPtr != NULL) && (setTagPtr->tag.id < TAF_PA_KS_TAG_MAX_IDS) &&
        (keyPtr != NULL) && (keyPtr->keyType == KS_NEW_CREATED_KEY))
    {
        // Check if the tag is already created for this new key.
        le_dls_Link_t* linkPtr = le_dls_Peek(&(keyPtr->newKey.tagList));
        while (linkPtr)
        {
            tagPtr = CONTAINER_OF(linkPtr, KeyTag_t, link);
            linkPtr = le_dls_PeekNext(&(keyPtr->newKey.tagList), linkPtr);

            if (tagPtr->tag.id == setTagPtr->tag.id)
            {
                // Remove the old tag from the tagList of the new key.
                LE_WARN("Remove old tag(%u) for new keyId '%s'.",
                        tagPtr->tag.id, keyPtr->newKey.keyId);
                le_dls_Remove(&(keyPtr->newKey.tagList), &(tagPtr->link));

                if ((tagPtr->tag.id == TAF_PA_KS_TAG_APPLICATION_DATA) &&
                    (tagPtr->tag.appDataPtr != NULL))
                {
                    // Release the appData object in tag object.
                    le_mem_Release(tagPtr->tag.appDataPtr);
                }

                // Release the tag object.
                le_mem_Release(tagPtr);

                break;
            }
        }

        // Create a new tag with sepcified id/value for the new key.
        LE_DEBUG("Add tag(%u) for new keyId '%s' for client(%p).",
                setTagPtr->tag.id, keyPtr->newKey.keyId,
                keyPtr->newKey.clientSessionRef);

        // Add the tag into the tagList of the new key.
        le_dls_Queue(&keyPtr->newKey.tagList, &(setTagPtr->link));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set or update a specified parameter for a crypto session.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::SetParam
(
    taf_ks_CryptoSession_t* sessionPtr,///< [IN] Cryption session pointer
    KeyParam_t* setParamPtr     ///< [IN] Parameter pointer
)
{
    KeyParam_t* paramPtr = NULL;

    if ((sessionPtr != NULL) && (setParamPtr != NULL) &&
        (setParamPtr->param.id < TAF_PA_KS_PARAM_MAX_IDS))
    {
        // Check if the parameter is already created for this session.
        le_dls_Link_t* linkPtr = le_dls_Peek(&(sessionPtr->paramList));
        while (linkPtr)
        {
            paramPtr = CONTAINER_OF(linkPtr, KeyParam_t, link);
            linkPtr = le_dls_PeekNext(&(sessionPtr->paramList), linkPtr);

            if (paramPtr->param.id == setParamPtr->param.id)
            {
                // Remove the parameter from the paramList for the session.
                LE_WARN("Remove old param(%u) for crypto session(%p) of key(%p).",
                        paramPtr->param.id, sessionPtr->cryptoSessionRef, sessionPtr->keyRef);
                le_dls_Remove(&(sessionPtr->paramList), &(paramPtr->link));

                // Free the sub parameter.
                switch(paramPtr->param.id)
                {
                    case TAF_PA_KS_PARAM_NONCE:
                        le_mem_Release(paramPtr->param.nonceDataPtr);
                        break;

                    case TAF_PA_KS_PARAM_APPLICATION_DATA:
                        le_mem_Release(paramPtr->param.appDataPtr);
                        break;

                    case TAF_PA_KS_PARAM_RSA_PADDING_TYPE:
                        break;

                    default:
                        LE_FATAL("Unrecognized param(%u)", paramPtr->param.id);
                        break;
                }

                // Free the parameter.
                le_mem_Release(paramPtr);

                break;
            }
        }

        // Create a new parameter with sepcified id/value for the session.
        LE_DEBUG("Add parameter(%u) for crypto session(%p) of key(%p).",
                setParamPtr->param.id, sessionPtr->cryptoSessionRef, sessionPtr->keyRef);

        // Add the paramter into the paramList of the session
        le_dls_Queue(&(sessionPtr->paramList), &(setParamPtr->link));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear the tag list of a new key. Remove and free all the tag objects in the list.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::ClearTagList
(
    taf_ks_Key_t* keyPtr   ///< [IN] Key pointer
)
{
    KeyTag_t* tagPtr;

    if ((keyPtr != NULL) && (keyPtr->keyType == KS_NEW_CREATED_KEY))
    {
        // Go through the tagList, free tag object and remove the node.
        le_dls_Link_t* linkPtr = le_dls_Pop(&(keyPtr->newKey.tagList));
        while (linkPtr)
        {
            tagPtr = CONTAINER_OF(linkPtr, KeyTag_t, link);

            LE_DEBUG("Removed the tag(%u) of new Key ID '%s'.",
                    tagPtr->tag.id, keyPtr->newKey.keyId);
            // Release the tag object
            if ((tagPtr->tag.id == TAF_PA_KS_TAG_APPLICATION_DATA) &&
                (tagPtr->tag.appDataPtr != NULL))
            {
                le_mem_Release(tagPtr->tag.appDataPtr);
            }
            le_mem_Release(tagPtr);

            linkPtr = le_dls_Pop(&(keyPtr->newKey.tagList));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear the parameter list of specified crypto session. Remove and free all the parameter objects
 * in the list.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::ClearParamList
(
    taf_ks_CryptoSession_t* sessionPtr   ///< [IN] Crypto session pointer
)
{
    KeyParam_t* paramPtr = NULL;

    if (sessionPtr != NULL)
    {
        // Go through the paramList, free parameter object and remove the node.
        le_dls_Link_t* linkPtr = le_dls_Pop(&(sessionPtr->paramList));
        while (linkPtr)
        {
            paramPtr = CONTAINER_OF(linkPtr, KeyParam_t, link);

            LE_DEBUG("Removed the param(%u) for crypto session(%p) of key(%p)",
                    paramPtr->param.id, sessionPtr->cryptoSessionRef, sessionPtr->keyRef);
            // Free the sub parameter.
            switch(paramPtr->param.id)
            {
                case TAF_PA_KS_PARAM_NONCE:
                    le_mem_Release(paramPtr->param.nonceDataPtr);
                    break;

                case TAF_PA_KS_PARAM_APPLICATION_DATA:
                    le_mem_Release(paramPtr->param.appDataPtr);
                    break;

                case TAF_PA_KS_PARAM_RSA_PADDING_TYPE:
                    break;

                default:
                    LE_FATAL("Unrecognized param(%u)", paramPtr->param.id);
                    break;
            }
            // Free the parameter.
            le_mem_Release(paramPtr);

            linkPtr = le_dls_Pop(&(sessionPtr->paramList));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete a crypto operation session. It also aborts the runing crypto operation if the session is
 * started.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::RemoveCryptoSession
(
    taf_ks_CryptoSession_t* sessionPtr  ///< [IN] Crypto session ptr.
)
{
    if (sessionPtr != NULL)
    {
        // Abort the session if it's started.
        if (sessionPtr->started)
        {
            taf_pa_result_t paResult = taf_pa_ks_CryptoSessionAbort(sessionPtr->handle);
            if (paResult != TAF_PA_OK)
            {
                LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_CryptoSessionAbort reason=%d", paResult);
            }
            sessionPtr->started = false;
            sessionPtr->handle = 0;
        }

        // Remove parameter list of the session.
        ClearParamList(sessionPtr);

        // Delete the sessionRef and free the session.
        LE_DEBUG("Deleted the crypto session(%p) for provisioned key(%p).",
                sessionPtr->cryptoSessionRef, sessionPtr->keyRef);

        le_ref_DeleteRef(CryptoSessionRefMap_, sessionPtr->cryptoSessionRef);
        le_mem_Release(sessionPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear the crypto session list of a provisioned key.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::ClearCryptoSessionList
(
    taf_ks_Key_t* keyPtr   ///< [IN] Key pointer
)
{
    taf_ks_CryptoSession_t* sessionPtr = NULL;

    if ((keyPtr != NULL) && (keyPtr->keyType == KS_PROVISIONED_KEY))
    {
        // Go through the session list, free session object and remove the node.
        le_dls_Link_t* linkPtr = le_dls_Pop(&(keyPtr->proKey.cryptoSessionList));
        while (linkPtr)
        {
            sessionPtr = CONTAINER_OF(linkPtr, taf_ks_CryptoSession_t, link);
            RemoveCryptoSession(sessionPtr);

            linkPtr = le_dls_Pop(&(keyPtr->proKey.cryptoSessionList));
        }

        LE_DEBUG("Cleared crypto session list of key(%p)", keyPtr->keyRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove new keys created by the given client
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::RemoveNewKeysForClient
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] client session reference
    void*               contextPtr   ///< [IN]
)
{
    taf_KeyStore& myKeyStore = taf_KeyStore::GetInstance();
    taf_ks_Key_t* keyPtr;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(myKeyStore.KeyRefMap_);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        keyPtr = static_cast<taf_ks_Key_t*>(le_ref_GetValue(iterRef));

        if ((keyPtr != NULL) &&
            (keyPtr->keyType == KS_NEW_CREATED_KEY) &&
            (keyPtr->newKey.clientSessionRef == sessionRef))
        {
            // Remove the key reference.
            le_ref_DeleteRef(myKeyStore.KeyRefMap_, keyPtr->keyRef);

            // Clear the tag list.
            myKeyStore.ClearTagList(keyPtr);

            // Free the key object.
            le_mem_Release(keyPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove all crypto sessions which created by the specified client
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::RemoveCryptoSessionsForClient
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] client session reference
    void*               contextPtr   ///< [IN]
)
{
    taf_KeyStore& myKeyStore = taf_KeyStore::GetInstance();
    taf_ks_CryptoSession_t* cryptoSessionPtr;
    taf_ks_Key_t* keyPtr;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(myKeyStore.CryptoSessionRefMap_);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        cryptoSessionPtr = static_cast<taf_ks_CryptoSession_t*>(le_ref_GetValue(iterRef));

        if ((cryptoSessionPtr != NULL) &&
            (cryptoSessionPtr->clientSessionRef == sessionRef))
        {
            // Remove the crypto session from the key's cryptoSession list.
            keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(myKeyStore.KeyRefMap_,
                                                               cryptoSessionPtr->keyRef));
            le_dls_Remove(&(keyPtr->proKey.cryptoSessionList), &(cryptoSessionPtr->link));

            // Delete the crypto session.
            myKeyStore.RemoveCryptoSession(cryptoSessionPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if there is running crypto session for a given key.
 */
//--------------------------------------------------------------------------------------------------
bool taf_KeyStore::HasRunningCryptoSession
(
    le_dls_List_t cryptoSessionList
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&cryptoSessionList);

    while (linkPtr != NULL)
    {
        taf_ks_CryptoSession_t* sessionPtr = CONTAINER_OF(linkPtr, taf_ks_CryptoSession_t, link);
        if ((sessionPtr != NULL) && (sessionPtr->started))
        {
            return true;
        }
        linkPtr = le_dls_PeekNext(&cryptoSessionList, linkPtr);
    }

    return false;
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the application name of the process with the specified client session reference.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::GetAppNameBySessionRef
(
    le_msg_SessionRef_t clientSessionRef,           ///< [IN]  client session reference.
    char    *appNameStr,                            ///< [OUT] Application name buffer.
    size_t   appNameSize                            ///< [IN]  Buffer size.
)
{
    pid_t pid;
    char procPath[LIMIT_MAX_PATH_BYTES] = {0};
    char appPath[LIMIT_MAX_PATH_BYTES] = {0};

    // Parameter check.
    if ((clientSessionRef == NULL) || (appNameStr == NULL) || (appNameSize == 0))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    // Get pid from the sessionRef.
    if (le_msg_GetClientProcessId(clientSessionRef, &pid) != LE_OK)
    {
        LE_ERROR("Failed to get the pid from client session reference.");
        return LE_FAULT;
    }

    // Get the app name from the pid for telaf app.
    if (le_appInfo_GetName(pid, appPath, sizeof(appPath)) != LE_OK)
    {
        // It's not a telaf app but a legacy app.
        // Read the executable full path from the softlink of /proc/<pid>/exe .
        LE_ASSERT(static_cast<size_t>(snprintf(procPath, sizeof(procPath), "/proc/%d/exe", pid))
                  < sizeof(procPath));

        memset(appPath, 0, sizeof(appPath));
        if (readlink(procPath, appPath, sizeof(appPath)) < 0)
        {
            LE_ERROR("readlink(%s) failed %s", procPath, LE_ERRNO_TXT(errno));
            return LE_FAULT;
        }
    }

    return le_utf8_Copy(appNameStr, appPath, appNameSize, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Create application key objects in service layer.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::CreateAppProKey
(
    KeyMgt_KeyFileRef_t keyFileRef                  ///< Key file reference
)
{
    if (keyFileRef != NULL)
    {
        // Create a provisioned key.
        taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_mem_ForceAlloc(KeyPool_));
        memset(keyPtr, 0, sizeof(taf_ks_Key_t));

        keyPtr->keyType = KS_PROVISIONED_KEY;                 // Save the key type.
        keyPtr->proKey.cryptoSessionList= LE_DLS_LIST_INIT;   // Save the cryptoSesion list.
        keyPtr->proKey.sharedAppList = LE_DLS_LIST_INIT;      // Save the shared application list.
        keyPtr->proKey.keyFileRef = keyFileRef;               // Save the key file reference.
        // Save the key reference.
        keyPtr->keyRef = static_cast<taf_ks_KeyRef_t>(le_ref_CreateRef(KeyRefMap_, keyPtr));

        LE_INFO("A provisoned key(%p) created.", keyPtr->keyRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Key creation handler registered to PA.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::KeyCreationPAHandler
(
    KeyMgt_KeyFileRef_t keyFileRef                 ///< Key file reference
)
{
    taf_KeyStore& myKeyStore = taf_KeyStore::GetInstance();

#if 0
    if (keyFileRef != NULL)
    {
        KeyEvent_t keyEvent;
        memset(&keyEvent, 0, sizeof(KeyEvent_t));

        keyEvent.op = KS_OP_CREATE;
        keyEvent.keyFileRef = keyFileRef;

        le_event_Report(myKeyStore.KeyEventId_, &keyEvent, sizeof(KeyEvent_t));
    }
#endif
    myKeyStore.CreateAppProKey(keyFileRef);

}

//--------------------------------------------------------------------------------------------------
/**
 * Key sharing handler registered to PA.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::KeySharingPAHandler
(
    const char* keyIdPtr,                          ///< Key ID string
    const char* ownerAppNamePtr,                   ///< Owner app name string
    const char* sharedAppNamePtr,                  ///< Shared app name string
    taf_pa_ks_SharingState_t state,                ///< Key sharing state
    KeyMgt_KeyFileRef_t keyFileRef                 ///< Key file reference
)
{
    if ((keyIdPtr != NULL) && (keyFileRef != NULL) &&
        (ownerAppNamePtr != NULL) && (sharedAppNamePtr != NULL))
    {
        taf_KeyStore& myKeyStore = taf_KeyStore::GetInstance();
        KeyEvent_t keyEvent;
        memset(&keyEvent, 0, sizeof(KeyEvent_t));

        keyEvent.op = KS_OP_SHARE;
        keyEvent.keyFileRef = keyFileRef;

        keyEvent.shareInfo.state = static_cast<taf_ks_SharingState_t>(state);
        le_utf8_Copy(keyEvent.shareInfo.keyId, keyIdPtr, sizeof(keyEvent.shareInfo.keyId), NULL);
        le_utf8_Copy(keyEvent.shareInfo.sharedAppName, sharedAppNamePtr,
                     sizeof(keyEvent.shareInfo.sharedAppName), NULL);
        le_utf8_Copy(keyEvent.shareInfo.ownerAppName, ownerAppNamePtr,
                     sizeof(keyEvent.shareInfo.ownerAppName), NULL);

        le_event_Report(myKeyStore.KeyEventId_, &keyEvent, sizeof(KeyEvent_t));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Notify clients that a key is shared.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::NotifyClientsForKeySharing
(
    const char* keyIdPtr,                          ///< Key ID string
    const char* ownerAppNamePtr,                   ///< Owner app name string
    const char* sharedAppNamePtr,                  ///< Shared app name string
    taf_ks_SharingState_t state                    ///< Key sharing state
)
{
    if ((keyIdPtr != NULL) && (ownerAppNamePtr != NULL) && (sharedAppNamePtr != NULL))
    {
        char appName[TAF_KS_MAX_APP_NAME_SIZE + 1] = { 0 };

        le_ref_IterRef_t iterRef = le_ref_GetIterator(HandlerRefMap_);
        while (le_ref_NextNode(iterRef) == LE_OK)
        {
            taf_ks_Handler_t* handlerPtr = static_cast<taf_ks_Handler_t*>(le_ref_GetValue(iterRef));

            // Get client app name.
            if ((handlerPtr != NULL) &&
                (LE_OK == GetAppNameBySessionRef(handlerPtr->clientSessionRef,
                                                 appName, sizeof(appName))))
            {
                // If the key sharing notification is for this client, call the client handler.
                if ((0 == strcmp(keyIdPtr, handlerPtr->keyId)) &&
                    (0 == strcmp(ownerAppNamePtr, handlerPtr->ownerAppName)) &&
                    (0 == strcmp(sharedAppNamePtr, appName)) &&
                    ((state == TAF_KS_SHARING_UPDATED) || (state != handlerPtr->state)))
                {
                    handlerPtr->state = state;
                    handlerPtr->handleFunc(handlerPtr->keyId, handlerPtr->ownerAppName,
                                           handlerPtr->state, handlerPtr->context);
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Event handler for key sharing state change.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::KeyEventHandler
(
    void* reportPtr
)
{
    if(reportPtr != NULL)
    {
        taf_KeyStore& myKeyStore = taf_KeyStore::GetInstance();
        KeyEvent_t* keyEventPtr = static_cast<KeyEvent_t*>(reportPtr);

        // Common info
        KeyOp_t op = keyEventPtr->op;
        KeyMgt_KeyFileRef_t keyFileRef = keyEventPtr->keyFileRef;

        // Key sharing info
        taf_ks_SharingState_t state = keyEventPtr->shareInfo.state;
        const char* keyIdPtr = keyEventPtr->shareInfo.keyId;
        const char* ownerAppNamePtr = keyEventPtr->shareInfo.ownerAppName;
        const char* sharedAppNamePtr = keyEventPtr->shareInfo.sharedAppName;

        switch(op)
        {
            case KS_OP_CREATE:
                myKeyStore.CreateAppProKey(keyFileRef);
            break;

            case KS_OP_SHARE:
                LE_INFO("sharing Key event: keyId(%s), ownApp(%s), sharedApp(%s), shareState(%u).",
                        keyIdPtr, ownerAppNamePtr, sharedAppNamePtr, state);
                myKeyStore.NotifyClientsForKeySharing(keyIdPtr, ownerAppNamePtr, sharedAppNamePtr,
                                                       state);
            break;

            default:
                LE_FATAL("Unknown op(%d).", op);
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear the appList.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::ClearAppList
(
    SharedAppList_t* appListPtr   ///< [IN] app list pointer
)
{
    SharedApp_t* shareAppPtr = NULL;

    if (appListPtr != NULL)
    {
        // Go through the appList, and free the app object.
        le_sls_Link_t* linkPtr = le_sls_Pop(&(appListPtr->appList));
        while (linkPtr)
        {
            shareAppPtr = CONTAINER_OF(linkPtr, SharedApp_t, link);
            le_mem_Release(shareAppPtr);

            linkPtr = le_sls_Pop(&(appListPtr->appList));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Add an appList of a given key for a client.
 */
//--------------------------------------------------------------------------------------------------
SharedAppList_t* taf_KeyStore::UpdateAppListForClient
(
    le_msg_SessionRef_t sessionRef,                   ///< [IN] client session reference
    taf_ks_Key_t* keyPtr,                             ///< [IN] key pointer
    const taf_pa_ks_sharedAppList_t* sharedAppListPtr ///< [IN] sharedAppList from PA
)
{
    SharedAppList_t* appListPtr = NULL;
    le_dls_Link_t* linkPtr = NULL;

    if ((keyPtr != NULL) && (keyPtr->keyType == KS_PROVISIONED_KEY))
    {
        linkPtr = le_dls_Peek(&(keyPtr->proKey.sharedAppList));
        while (linkPtr != NULL)
        {
            appListPtr = CONTAINER_OF(linkPtr, SharedAppList_t, link);
            linkPtr = le_dls_PeekNext(&(keyPtr->proKey.sharedAppList), linkPtr);

            // Remove the old shareAppList of the key for this client.
            if (appListPtr->clientSessionRef == sessionRef)
            {
                ClearAppList(appListPtr);
                le_dls_Remove(&(keyPtr->proKey.sharedAppList), &(appListPtr->link));
                le_mem_Release(appListPtr);

                break;
            }
        }

        // Create a new shareAppList of the key for this client.
        if (sharedAppListPtr != NULL)
        {
            appListPtr = static_cast<SharedAppList_t*>(le_mem_ForceAlloc(SharedAppListPool_));
            memset(appListPtr, 0, sizeof(SharedAppList_t));

            appListPtr->clientSessionRef = sessionRef;
            appListPtr->appList = LE_SLS_LIST_INIT;
            appListPtr->link = LE_DLS_LINK_INIT;
            appListPtr->keyRef = keyPtr->keyRef;
            appListPtr->currPtr = NULL;

            for (int i = 0; i < TAF_PA_KS_MAX_SHARED_APPS; i++)
            {
               size_t len = strlen(sharedAppListPtr->appInfo[i].appName);
               const char* namePtr = sharedAppListPtr->appInfo[i].appName;
               taf_ks_KeyUsage_t keyCap =
                   static_cast<taf_ks_KeyUsage_t>(sharedAppListPtr->appInfo[i].keyCap);
               taf_ks_AppCapMask_t appCap = sharedAppListPtr->appInfo[i].appCap;

               if ((len > 0) && (len <= TAF_KS_MAX_APP_NAME_SIZE))
               {
                   // Create a sharedApp object for each shared app.
                   SharedApp_t* appPtr =
                       static_cast<SharedApp_t*>(le_mem_ForceAlloc(SharedAppPool_));
                   memset(appPtr, 0, sizeof(SharedApp_t));

                   appPtr->link = LE_SLS_LINK_INIT;
                   le_utf8_Copy(appPtr->appInfo.appName, namePtr,
                                sizeof(appPtr->appInfo.appName), NULL);
                   appPtr->appInfo.keyCap = static_cast<taf_pa_ks_KeyUsage_t>(keyCap);
                   appPtr->appInfo.appCap = appCap;
                   le_sls_Queue(&(appListPtr->appList), &(appPtr->link));
               }
            }

            // No shareApp objects in the list, just free the appList and return.
            if (le_sls_NumLinks(&(appListPtr->appList)) == 0)
            {
               le_mem_Release(appListPtr);
               return NULL;
            }

            // Queue the applist .
            le_dls_Queue(&(keyPtr->proKey.sharedAppList), &(appListPtr->link));

            return appListPtr;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove all shared app lists for a client.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::RemoveAppListsForClient
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] client session reference
    void* contextPtr                 ///< [IN] context
)
{
    taf_KeyStore& myKeyStore = taf_KeyStore::GetInstance();
    taf_ks_Key_t* keyPtr = NULL;
    SharedAppList_t* appListPtr = NULL;
    le_dls_Link_t* linkPtr = NULL;

    le_ref_IterRef_t iterRef = le_ref_GetIterator(myKeyStore.KeyRefMap_);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        keyPtr = static_cast<taf_ks_Key_t*>(le_ref_GetValue(iterRef));

        // For provisioned key, check the client session.
        if ((keyPtr != NULL) && (keyPtr->keyType == KS_PROVISIONED_KEY))
        {
            linkPtr = le_dls_Peek(&(keyPtr->proKey.sharedAppList));
            while (linkPtr != NULL)
            {
                appListPtr = CONTAINER_OF(linkPtr, SharedAppList_t, link);
                linkPtr = le_dls_PeekNext(&(keyPtr->proKey.sharedAppList), linkPtr);

                // Remove all appLists created by this client session.
                if (appListPtr->clientSessionRef == sessionRef)
                {
                    myKeyStore.ClearAppList(appListPtr);
                    le_dls_Remove(&(keyPtr->proKey.sharedAppList), &(appListPtr->link));
                    le_mem_Release(appListPtr);

                    break;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear all sharedApp list for a given key.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::ClearSharedAppList
(
    taf_ks_Key_t*	 keyPtr   ///< [IN] Key pointer
)
{
    le_dls_Link_t* linkPtr = NULL;
    SharedAppList_t* appListPtr = NULL;

    // For provisioned key, check the client session.
    if ((keyPtr != NULL) && (keyPtr->keyType == KS_PROVISIONED_KEY))
    {
        linkPtr = le_dls_Pop(&(keyPtr->proKey.sharedAppList));
        while (linkPtr != NULL)
        {
            appListPtr = CONTAINER_OF(linkPtr, SharedAppList_t, link);

            ClearAppList(appListPtr);
            le_mem_Release(appListPtr);

            linkPtr = le_dls_Pop(&(keyPtr->proKey.sharedAppList));
        }

        LE_INFO("Cleared all sharedApp lists for key(%p)", keyPtr->keyRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Creates a new key by key ID.
 *
 * New keys initially have no value and cannot be used for any crypto operations. Call SetKey API to
 * set optional attributions for key access control if needed, then call Provision APIs to provision
 * the key value and save the key into the backend storage. The Crypto APIs are available to use
 * only after the key value is correctly provisioned.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::CreateKey
(
    const char* keyId,
        ///< [IN] Key ID
    taf_ks_KeyUsage_t keyUsage,
        ///< [IN] Key usage
    taf_ks_KeyRef_t* keyRefPtr
        ///< [OUT] Key reference
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef = NULL;
    le_msg_SessionRef_t clientSessionRef = taf_ks_GetClientSessionRef();
    taf_ks_Key_t* keyPtr;
    char appName[TAF_KS_MAX_APP_NAME_SIZE + 1] = { 0 };

    // Parameter check.
    if ((keyId == NULL) || (keyRefPtr == NULL) || (keyUsage >= TAF_KS_KEYUSAGE_MAX))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    // Get appName from clientSession.
    if (LE_OK != GetAppNameBySessionRef(clientSessionRef, appName, sizeof(appName)))
    {
        LE_ERROR("Failed to get client appName.");
        return LE_FAULT;
    }

    // Check if keyId string follows the POSIX file name format.
    if (!IsPosixFileNameFormat(keyId))
    {
        LE_ERROR("Invalid key ID format (%s).", keyId);
        return LE_BAD_PARAMETER;
    }

    // Check if a new key of the specified key ID for the client is already created,
    // and just return the same reference.
    keyPtr = SearchNewKey(keyId);
    if (keyPtr != NULL)
    {
        LE_WARN("New key for keyId '%s' already exists.", keyId);
        return LE_NOT_PERMITTED;
    }

    // Check if a provisoned key of the specified key ID for the client already exists.
    // We don't allow to create it again if so, thus we expect it return LE_NOT_FOUND.
    taf_pa_result_t paResult = taf_pa_ks_GetKey(le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                                                keyId, &keyFileRef);
    result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK && paResult != TAF_PA_NOT_FOUND)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_GetKey reason=%d", paResult);
    }
    if (result == LE_OK)
    {
        LE_WARN("Provisioned key for keyName '%s' already exists.", keyId);
        return LE_NOT_PERMITTED;
    }

    if (result != LE_NOT_FOUND)
    {
        LE_ERROR("Internal error occured.");
        return LE_FAULT;
    }

    if (keyFileRef != NULL)
    {
        // An obselete key file was deleted in PA layer so we need delete it in
        // Service layer if existss.
        taf_ks_Key_t* keyPtr = SearchProvisionedKey(keyFileRef);
        if (keyPtr != NULL)
        {
            LE_WARN("An obselete provisioned key(%p) deleted.", keyPtr->keyRef);

            ClearCryptoSessionList(keyPtr);
            ClearSharedAppList(keyPtr);
            le_ref_DeleteRef(KeyRefMap_, keyPtr->keyRef);
            le_mem_Release(keyPtr);
        }
    }

    LE_INFO("Create a new key of KeyId '%s' for application '%s'.", keyId, appName);

    // Create a new key. The new key will become a provisioned key after key provisioning.
    keyPtr = static_cast<taf_ks_Key_t*>(le_mem_ForceAlloc(KeyPool_));
    memset(keyPtr, 0, sizeof(taf_ks_Key_t));

    // Save the key reference.
    keyPtr->keyRef = static_cast<taf_ks_KeyRef_t>(le_ref_CreateRef(KeyRefMap_, keyPtr));
    keyPtr->keyType = KS_NEW_CREATED_KEY;                 // Save key type.
    keyPtr->newKey.tagList = LE_DLS_LIST_INIT;            // Save the tag list.
    keyPtr->newKey.clientSessionRef = clientSessionRef;   // Save client session.
    keyPtr->newKey.keyUsage = keyUsage;                   // Save key usage.
    LE_ASSERT(LE_OK == le_utf8_Copy(keyPtr->newKey.keyId, keyId,
                                    sizeof(keyPtr->newKey.keyId), NULL));// Save the key name.

    // Return the key reference of the new key.
    *keyRefPtr = keyPtr->keyRef;
    LE_INFO("A new key(%p) created.", keyPtr->keyRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get key reference to a provisioned key by key name.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::GetKey
(
    const char* keyName,
        ///< [IN] Key name
    taf_ks_KeyRef_t* keyRefPtr
        ///< [OUT] Key reference
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef = NULL;
    le_msg_SessionRef_t clientSessionRef = taf_ks_GetClientSessionRef();
    taf_ks_Key_t* keyPtr;
    char myAppName[TAF_KS_MAX_APP_NAME_SIZE + 1] = { 0 };
    char ownerAppName[TAF_KS_MAX_APP_NAME_SIZE + 1] = { 0 };
    const char* keyIdPtr = NULL;
    const char* ownerAppPtr = NULL;

    // Parameter check.
    if ((keyName == NULL) || (keyRefPtr == NULL))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    // Get appName from clientSession.
    if (LE_OK != GetAppNameBySessionRef(clientSessionRef, myAppName, sizeof(myAppName)))
    {
        LE_ERROR("Failed to get client appName.");
        return LE_FAULT;
    }

    // For a shared Key, the keyName = "<appName>::<keyId>".
    // Otherwise it's our own key.
    keyIdPtr = strstr(keyName, "::");
    if (keyIdPtr != NULL)
    {
        // Get the keyId from the keyName.
        keyIdPtr = keyIdPtr + 2;

        // Get the owner appName from the keyName.
        le_utf8_CopyUpToSubStr(ownerAppName, keyName, "::", sizeof(ownerAppName), NULL);

        // Sanity check.
        if ((keyIdPtr[0] == '\0') || (ownerAppName[0] == '\0'))
        {
            LE_ERROR("Invalid keyName format (%s).", keyName);
            return LE_BAD_PARAMETER;
        }

        ownerAppPtr = ownerAppName;
        if (strcmp(myAppName, ownerAppName) == 0)
        {
            ownerAppPtr = NULL;
        }
    }
    else
    {
        keyIdPtr = keyName;
    }

    // Check if keyId string follows the POSIX file name format.
    if (!IsPosixFileNameFormat(keyIdPtr))
    {
        LE_ERROR("Invalid keyId format (%s).", keyIdPtr);
        return LE_BAD_PARAMETER;
    }

    if (ownerAppPtr != NULL)
    {
        LE_DEBUG("Get keyId('%s') of app('%s').", keyIdPtr, ownerAppPtr);
        taf_pa_result_t paResult = taf_pa_ks_GetSharedKey(
                                 le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                                 keyIdPtr, ownerAppPtr, &keyFileRef);
        result = ConvertPaToLeRet(paResult);
        if (paResult != TAF_PA_OK && paResult != TAF_PA_NOT_FOUND)
        {
            LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_GetSharedKey reason=%d", paResult);
        }
    }
    else
    {
        // Get the key file of our own key.
        LE_DEBUG("Get keyId('%s').", keyIdPtr);
        taf_pa_result_t paResult = taf_pa_ks_GetKey(
                                   le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                                   keyIdPtr, &keyFileRef);
        result = ConvertPaToLeRet(paResult);
        if (paResult != TAF_PA_OK && paResult != TAF_PA_NOT_FOUND)
        {
            LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_GetKey reason=%d", paResult);
        }
    }

    if (result == LE_OK)
    {
        // Search if a key reference to the key is already created.
        keyPtr = SearchProvisionedKey(keyFileRef);
        if(keyPtr != NULL)
        {
            *keyRefPtr = keyPtr->keyRef;
            return LE_OK;
        }

        // Create a provisioned key.
        keyPtr = static_cast<taf_ks_Key_t*>(le_mem_ForceAlloc(KeyPool_));
        memset(keyPtr, 0, sizeof(taf_ks_Key_t));

        keyPtr->keyType = KS_PROVISIONED_KEY;              // Save the key type.
        keyPtr->proKey.cryptoSessionList= LE_DLS_LIST_INIT;// Save the cryptoSesion list
        keyPtr->proKey.sharedAppList = LE_DLS_LIST_INIT;      // Save the shared application list.
        keyPtr->proKey.keyFileRef = keyFileRef;            // Save the key file reference.
        // Save the key reference.
        keyPtr->keyRef = static_cast<taf_ks_KeyRef_t>(le_ref_CreateRef(KeyRefMap_, keyPtr));

        // Return the key reference of the provisioned key.
        *keyRefPtr = keyPtr->keyRef;
        LE_INFO("A provisoned key(%p) created.", keyPtr->keyRef);
    }
    else if ((result == LE_NOT_FOUND) && (keyFileRef != NULL))
    {
        // An obselete key file was deleted in PA layer so we need delete it in
        // Service layer if exists.
        keyPtr = SearchProvisionedKey(keyFileRef);
        if (keyPtr != NULL)
        {
            LE_WARN("An obselete provisioned key(%p) deleted.", keyPtr->keyRef);

            ClearCryptoSessionList(keyPtr);
            ClearSharedAppList(keyPtr);
            le_ref_DeleteRef(KeyRefMap_, keyPtr->keyRef);
            le_mem_Release(keyPtr);
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete a key by key reference
 *
 * The key is only allowed to be deleted if it doesn't have ongoing crypto sessions, otherwise
 * LE_BUSY will be returned.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::DeleteKey
(
    taf_ks_KeyRef_t keyRef
        ///< [IN] Key reference
)
{
    taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Key is not allowed to be deleted if it has running crypto session.
    if ((keyPtr->keyType == KS_PROVISIONED_KEY) &&
         HasRunningCryptoSession(keyPtr->proKey.cryptoSessionList))
    {
        LE_WARN("Key has running crypto session.");
        return LE_NOT_PERMITTED;
    }

    if (keyPtr->keyType == KS_PROVISIONED_KEY)
    {
        // Delete a provisioned key.
        taf_pa_result_t paResult = taf_pa_ks_DeleteKey(
                        le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                        keyPtr->proKey.keyFileRef);
        le_result_t result = ConvertPaToLeRet(paResult);
        if (paResult != TAF_PA_OK)
        {
            LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_DeleteKey reason=%d", paResult);
        }
        if (result != LE_OK)
        {
            LE_ERROR("Failed to delete provisioned key(%p) (%s).",
                     keyPtr->keyRef, LE_RESULT_TXT(result));
            return result;
        }

        // Clear the cryptoSession list.
        ClearCryptoSessionList(keyPtr);

        // Clear the shared application list.
        ClearSharedAppList(keyPtr);

        LE_INFO("A provisioned key(%p) deleted.", keyPtr->keyRef);
    }
    else
    {
        // Clear the tag list.
        ClearTagList(keyPtr);

        LE_INFO("A new key(%p) of keyId '%s' deleted.", keyPtr->keyRef, keyPtr->newKey.keyId);
    }

    // Delete the key reference and free the key object.
    le_ref_DeleteRef(KeyRefMap_, keyPtr->keyRef);
    le_mem_Release(keyPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get key usage of a key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::GetKeyUsage
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    taf_ks_KeyUsage_t* keyUsagePtr
        ///< [OUT] Key usage
)
{
    if (keyUsagePtr == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // This is a new key, get the keyUsage from the key object.
    if (keyPtr->keyType == KS_NEW_CREATED_KEY)
    {
        if (keyPtr->newKey.clientSessionRef != taf_ks_GetClientSessionRef())
        {
            LE_ERROR("Invalid client session.");
            return LE_NOT_PERMITTED;
        }

        // Return key usage of a new key.
        *keyUsagePtr = keyPtr->newKey.keyUsage;
        return LE_OK;
    }

    // This is a provisioned key, return key usage from the PA.
    taf_pa_ks_KeyUsage_t paKeyUsage;
    taf_pa_result_t paResult = taf_pa_ks_GetKeyUsage(
                    le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                    keyPtr->proKey.keyFileRef, &paKeyUsage);
    le_result_t result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_GetKeyUsage reason=%d", paResult);
    }
    if (result == LE_OK)
    {
        *keyUsagePtr = (taf_ks_KeyUsage_t)paKeyUsage;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of times that a key may be used between system reboots.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::SetKeyMaxUsesPerBoot
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    uint32_t value
        ///< [IN] Uses per boot
)
{
    taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key, only new key is allowed to set the tag.
    if (keyPtr->keyType != KS_NEW_CREATED_KEY)
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    KeyTag_t* newTagPtr = static_cast<KeyTag_t*>(le_mem_ForceAlloc(TagPool_));
    memset(newTagPtr, 0, sizeof(KeyTag_t));

    newTagPtr->link = LE_DLS_LINK_INIT;
    newTagPtr->tag.id = TAF_PA_KS_TAG_MAX_USES_PER_BOOT;
    newTagPtr->tag.maxUsesPerBoot = value;
    SetTag(keyPtr, newTagPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the minimum amount of time that elapses between allowed operations using a key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::SetKeyMinSecondsBetweenOps
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    uint32_t value
        ///< [IN] Seconds interval between allowed operations.
)
{
    taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key, only new key is allowed to set the tag.
    if (keyPtr->keyType != KS_NEW_CREATED_KEY)
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    KeyTag_t* newTagPtr = static_cast<KeyTag_t*>(le_mem_ForceAlloc(TagPool_));
    memset(newTagPtr, 0, sizeof(KeyTag_t));

    newTagPtr->link = LE_DLS_LINK_INIT;
    newTagPtr->tag.id = TAF_PA_KS_TAG_MIN_SECONDS_BETWEEN_OPS;
    newTagPtr->tag.minSecondsBetweenOps = value;
    SetTag(keyPtr, newTagPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set application data to the key.
 *
 * When the attribute is provided to the key, the same data must be also provided to crypto API.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::SetKeyAppData
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    const uint8_t* dataPtr,
        ///< [IN] Data buffer to hold the application data
    size_t dataSize
        ///< [IN]
)
{
    if ((dataPtr == NULL) || (dataSize == 0))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key, only new key is allowed to set the tag.
    if (keyPtr->keyType != KS_NEW_CREATED_KEY)
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    KeyTag_t* newTagPtr = static_cast<KeyTag_t*>(le_mem_ForceAlloc(TagPool_));
    memset(newTagPtr, 0, sizeof(KeyTag_t));

    newTagPtr->link = LE_DLS_LINK_INIT;
    newTagPtr->tag.id = TAF_PA_KS_TAG_APPLICATION_DATA;
    newTagPtr->tag.appDataPtr = static_cast<taf_pa_ks_Data_t*>(le_mem_ForceAlloc(DataPool_));
    memcpy(newTagPtr->tag.appDataPtr->data, dataPtr, dataSize);
    newTagPtr->tag.appDataPtr->size = dataSize;
    SetTag(keyPtr, newTagPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the date and time at which the key becomes active. Prior to this time any attempt to use the
 * key will get failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::SetKeyActiveDateTime
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    uint64_t value
        ///< [IN] Milliseconds since January 1, 1970.
)
{
    taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key, only new key is allowed to set the tag.
    if (keyPtr->keyType != KS_NEW_CREATED_KEY)
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    KeyTag_t* newTagPtr = static_cast<KeyTag_t*>(le_mem_ForceAlloc(TagPool_));
    memset(newTagPtr, 0, sizeof(KeyTag_t));

    newTagPtr->link = LE_DLS_LINK_INIT;
    newTagPtr->tag.id = TAF_PA_KS_TAG_ACTIVE_DATETIME;
    newTagPtr->tag.activeDateTime = value;
    SetTag(keyPtr, newTagPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the date and time at which the key expires for signing and encryption. After this time any
 * attempt to use a key for signing or encryption will get failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::SetKeyOriginationExpireDateTime
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    uint64_t value
        ///< [IN] Milliseconds since January 1, 1970.
)
{
    taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key, only new key is allowed to set the tag.
    if (keyPtr->keyType != KS_NEW_CREATED_KEY)
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    KeyTag_t* newTagPtr = static_cast<KeyTag_t*>(le_mem_ForceAlloc(TagPool_));
    memset(newTagPtr, 0, sizeof(KeyTag_t));

    newTagPtr->link = LE_DLS_LINK_INIT;
    newTagPtr->tag.id = TAF_PA_KS_TAG_ORIGINATION_EXPIRE_DATETIME;
    newTagPtr->tag.originationExpireDateTime = value;
    SetTag(keyPtr, newTagPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the date and time at which the key expires for verification and decryption. After this time
 * any attempt to use a key for verification and decryption will get failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::SetKeyUsageExpireDateTime
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    uint64_t value
        ///< [IN] Milliseconds since January 1, 1970.
)
{
    taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key, only new key is allowed to set the tag.
    if (keyPtr->keyType != KS_NEW_CREATED_KEY)
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    KeyTag_t* newTagPtr = static_cast<KeyTag_t*>(le_mem_ForceAlloc(TagPool_));
    memset(newTagPtr, 0, sizeof(KeyTag_t));

    newTagPtr->link = LE_DLS_LINK_INIT;
    newTagPtr->tag.id = TAF_PA_KS_TAG_USAGE_EXPIRE_DATETIME;
    newTagPtr->tag.usageExpireDateTime = value;
    SetTag(keyPtr, newTagPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Provison a RSA encryption key value to the new created key so that the key can be available to
 * use for RSA encryption/decryption.
 *
 * The key value can be generated from either internal crypto engine or imported key data.
 * The impData holds the RSA encryption key must be PKCS#8 der format if provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::ProvisionRsaEncKeyValue
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    taf_ks_RsaKeySize_t keySize,
        ///< [IN] Key size. Shall match the import key size if impData is provided.
    taf_ks_RsaEncPadding_t padding,
        ///< [IN] RSA encryption padding type
    const uint8_t* impDataPtr,
        ///< [IN] Imported key data
    size_t impDataSize
        ///< [IN]
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef = NULL;
    taf_pa_ks_EncPurpose_t keyUsage;
    taf_ks_Key_t* keyPtr;

    if ((keySize >= TAF_KS_RSA_SIZE_MAX) || (padding >= TAF_KS_RSA_ENC_PAD_MAX))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key since only new key can be provisioned.
    // Also check if the key is suitable for this key value provision.
    if ((keyPtr->keyType != KS_NEW_CREATED_KEY) ||
        (keyPtr->newKey.clientSessionRef != taf_ks_GetClientSessionRef()) ||
        ((keyPtr->newKey.keyUsage != TAF_KS_RSA_ENCRYPT_DECRYPT) &&
         (keyPtr->newKey.keyUsage != TAF_KS_RSA_ENCRYPT_ONLY) &&
         (keyPtr->newKey.keyUsage != TAF_KS_RSA_DECRYPT_ONLY)))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Provision the RSA encryption key.
    keyUsage = (keyPtr->newKey.keyUsage == TAF_KS_RSA_ENCRYPT_DECRYPT) ?
        TAF_PA_KS_ENCRYPT_DECRYPT :
        ((keyPtr->newKey.keyUsage == TAF_KS_RSA_ENCRYPT_ONLY) ?
        TAF_PA_KS_ENCRYPT_ONLY : TAF_PA_KS_DECRYPT_ONLY);

    taf_pa_result_t paResult = taf_pa_ks_GenerateRsaEncKey(
                                         le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                                         keyPtr->newKey.keyId,
                                         static_cast<taf_pa_ks_RsaKeySize_t>(keySize),
                                         keyUsage,
                                         static_cast<taf_pa_ks_RsaEncPadding_t>(padding),
                                         (const taf_pa_ks_Tag_t**)TagPtrList_,
                                         0,
                                         impDataPtr,
                                         impDataSize,
                                         &keyFileRef);
    result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_GenerateRsaEncKey reason=%d", paResult);
    }
    if (result == LE_OK)
    {
        // Clear the tag list after successful provisioned.
        ClearTagList(keyPtr);

        // set key provisioned.
        keyPtr->keyType = KS_PROVISIONED_KEY;
        keyPtr->proKey.keyFileRef = keyFileRef;
        keyPtr->proKey.cryptoSessionList = LE_DLS_LIST_INIT;
        keyPtr->proKey.sharedAppList = LE_DLS_LIST_INIT;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Provision a RSA signing key value to the new created key so that the key can be available to
 * use for RSA signing/verification.
 *
 * The key value can be generated from either internal crypto engine or imported key data.
 * The impData holds the RSA signing key must be PKCS#8 der format if provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::ProvisionRsaSigKeyValue
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    taf_ks_RsaKeySize_t keySize,
        ///< [IN] Key size. Shall match the import key size if impData is provided.
    taf_ks_RsaSigPadding_t padding,
        ///< [IN] RSA signature padding type
    const uint8_t* impDataPtr,
        ///< [IN] Imported key data
    size_t impDataSize
        ///< [IN]
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef = NULL;
    taf_pa_ks_SigPurpose_t keyUsage;
    taf_ks_Key_t* keyPtr;

    if ((keySize >= TAF_KS_RSA_SIZE_MAX) || (padding >= TAF_KS_RSA_SIG_PAD_MAX))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key since only new key can be provisioned.
    // Also check if the key is suitable for this key value provision.
    if ((keyPtr->keyType != KS_NEW_CREATED_KEY) ||
        (keyPtr->newKey.clientSessionRef != taf_ks_GetClientSessionRef()) ||
        ((keyPtr->newKey.keyUsage != TAF_KS_RSA_SIGN_VERIFY) &&
         (keyPtr->newKey.keyUsage != TAF_KS_RSA_SIGN_ONLY) &&
         (keyPtr->newKey.keyUsage != TAF_KS_RSA_VERIFY_ONLY)))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Provision the RSA signing key.
    keyUsage = (keyPtr->newKey.keyUsage == TAF_KS_RSA_SIGN_VERIFY) ?
        TAF_PA_KS_SIGN_VERIFY :
        ((keyPtr->newKey.keyUsage == TAF_KS_RSA_SIGN_ONLY) ?
        TAF_PA_KS_SIGN_ONLY : TAF_PA_KS_VERIFY_ONLY);

    taf_pa_result_t paResult = taf_pa_ks_GenerateRsaSigKey(
                                         le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                                         keyPtr->newKey.keyId,
                                         static_cast<taf_pa_ks_RsaKeySize_t>(keySize),
                                         keyUsage,
                                         static_cast<taf_pa_ks_RsaSigPadding_t>(padding),
                                         (const taf_pa_ks_Tag_t**)TagPtrList_,
                                         0,
                                         impDataPtr,
                                         impDataSize,
                                         &keyFileRef);
    result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_GenerateRsaSigKey reason=%d", paResult);
    }
    if (result == LE_OK)
    {
        // Clear the tag list after successful provisioned.
        ClearTagList(keyPtr);

        // set key provisioned.
        keyPtr->keyType = KS_PROVISIONED_KEY;
        keyPtr->proKey.keyFileRef = keyFileRef;
        keyPtr->proKey.cryptoSessionList = LE_DLS_LIST_INIT;
        keyPtr->proKey.sharedAppList = LE_DLS_LIST_INIT;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Provision an ECDSA key value to the new created key so that the key can be available to use for
 * ECDSA signing/verification.
 *
 * The key value can be generated from either internal crypto engine or imported key data.
 * The impData holds the ECDSA key must be PKCS#8 der format if provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::ProvisionEcdsaKeyValue
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    taf_ks_EccKeySize_t keySize,
        ///< [IN] Key size. Shall match the import key size if impData is provided.
    taf_ks_Digest_t digest,
        ///< [IN] Digest
    const uint8_t* impDataPtr,
        ///< [IN] Imported key data
    size_t impDataSize
        ///< [IN]
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef = NULL;
    taf_pa_ks_SigPurpose_t keyUsage;
    taf_ks_Key_t* keyPtr;

    if ((keySize >= TAF_KS_ECC_SIZE_MAX) || (digest >= TAF_KS_DIGEST_MAX))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key since only new key can be provisioned.
    // Also check if the key is suitable for this key value provision.
    if ((keyPtr->keyType != KS_NEW_CREATED_KEY) ||
        (keyPtr->newKey.clientSessionRef != taf_ks_GetClientSessionRef()) ||
        ((keyPtr->newKey.keyUsage != TAF_KS_ECDSA_SIGN_VERIFY) &&
         (keyPtr->newKey.keyUsage != TAF_KS_ECDSA_SIGN_ONLY) &&
         (keyPtr->newKey.keyUsage != TAF_KS_ECDSA_VERIFY_ONLY)))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Provision the ECDSA key.
    keyUsage = (keyPtr->newKey.keyUsage == TAF_KS_ECDSA_SIGN_VERIFY) ?
        TAF_PA_KS_SIGN_VERIFY :
        ((keyPtr->newKey.keyUsage == TAF_KS_ECDSA_SIGN_ONLY) ?
        TAF_PA_KS_SIGN_ONLY : TAF_PA_KS_VERIFY_ONLY);

    taf_pa_result_t paResult = taf_pa_ks_GenerateEcdsaKey(
                                        le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                                        keyPtr->newKey.keyId,
                                        static_cast<taf_pa_ks_EccKeySize_t>(keySize),
                                        keyUsage,
                                        static_cast<taf_pa_ks_Digest_t>(digest),
                                        (const taf_pa_ks_Tag_t**)TagPtrList_,
                                        0,
                                        impDataPtr,
                                        impDataSize,
                                        &keyFileRef);
    result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_GenerateEcdsaKey reason=%d", paResult);
    }
    if (result == LE_OK)
    {
        // Clear the tag list after successful provisioned.
        ClearTagList(keyPtr);

        // set key provisioned.
        keyPtr->keyType = KS_PROVISIONED_KEY;
        keyPtr->proKey.keyFileRef = keyFileRef;
        keyPtr->proKey.cryptoSessionList = LE_DLS_LIST_INIT;
        keyPtr->proKey.sharedAppList = LE_DLS_LIST_INIT;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Provision an AES key value to the new created key so that the key can be available to use for
 * AES encryption/decryption.
 *
 * The key value can be generated from either internal crypto engine or imported key data.
 * The impData holds the AES key must be raw key bytes format if provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::ProvisionAesKeyValue
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    taf_ks_AesKeySize_t keySize,
        ///< [IN] Key size. Shall match the import key size if impData is provided.
    taf_ks_AesBlockMode_t mode,
        ///< [IN] AES block mode
    const uint8_t* impDataPtr,
        ///< [IN] Imported key data
    size_t impDataSize
        ///< [IN]
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef = NULL;
    taf_pa_ks_EncPurpose_t keyUsage;
    taf_ks_Key_t* keyPtr;

    if ((keySize >= TAF_KS_AES_SIZE_MAX) || (mode >= TAF_KS_AES_MODE_MAX))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key since only new key can be provisioned.
    // Also check if the key is suitable for this key value provision.
    if ((keyPtr->keyType != KS_NEW_CREATED_KEY) ||
        (keyPtr->newKey.clientSessionRef != taf_ks_GetClientSessionRef()) ||
        ((keyPtr->newKey.keyUsage != TAF_KS_AES_ENCRYPT_DECRYPT) &&
         (keyPtr->newKey.keyUsage != TAF_KS_AES_ENCRYPT_ONLY) &&
         (keyPtr->newKey.keyUsage != TAF_KS_AES_DECRYPT_ONLY)))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Provision the AES key.
    keyUsage = (keyPtr->newKey.keyUsage == TAF_KS_AES_ENCRYPT_DECRYPT) ?
        TAF_PA_KS_ENCRYPT_DECRYPT :
        ((keyPtr->newKey.keyUsage == TAF_KS_AES_ENCRYPT_ONLY) ?
        TAF_PA_KS_ENCRYPT_ONLY : TAF_PA_KS_DECRYPT_ONLY);

    taf_pa_result_t paResult = taf_pa_ks_GenerateAesKey(
                                      le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                                      keyPtr->newKey.keyId,
                                      static_cast<taf_pa_ks_AesKeySize_t>(keySize),
                                      keyUsage,
                                      static_cast<taf_pa_ks_AesBlockMode_t>(mode),
                                      (const taf_pa_ks_Tag_t**)TagPtrList_,
                                      0,
                                      impDataPtr,
                                      impDataSize,
                                      &keyFileRef);
    result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_GenerateAesKey reason=%d", paResult);
    }
    if (result == LE_OK)
    {
        // Clear the tag list after successful provisioned.
        ClearTagList(keyPtr);

        // set key provisioned.
        keyPtr->keyType = KS_PROVISIONED_KEY;
        keyPtr->proKey.keyFileRef = keyFileRef;
        keyPtr->proKey.cryptoSessionList = LE_DLS_LIST_INIT;
        keyPtr->proKey.sharedAppList = LE_DLS_LIST_INIT;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Provision a HMAC key value to the new created key so that the key can be available to use for
 * HMAC signing/verification.
 *
 * The key value can be generated from either internal crypto engine or imported key data.
 * The impData holds the HMAC key must be raw key bytes format if provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::ProvisionHmacKeyValue
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    uint32_t keySize,
        ///< [IN] Key size. Shall match the import key size if impData is provided.
    taf_ks_Digest_t digest,
        ///< [IN] digest
    const uint8_t* impDataPtr,
        ///< [IN] Imported key data
    size_t impDataSize
        ///< [IN]
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef = NULL;
    taf_pa_ks_SigPurpose_t keyUsage;
    taf_ks_Key_t* keyPtr;

    if ((keySize < TAF_KS_MIN_HMAC_KEY_SIZE) ||
        (keySize > TAF_KS_MAX_HMAC_KEY_SIZE) ||
        (digest >= TAF_KS_DIGEST_MAX))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key since only new key can be provisioned.
    // Also check if the key is suitable for this key value provision.
    if ((keyPtr->keyType != KS_NEW_CREATED_KEY) ||
        (keyPtr->newKey.clientSessionRef != taf_ks_GetClientSessionRef()) ||
        ((keyPtr->newKey.keyUsage != TAF_KS_HMAC_SIGN_VERIFY) &&
         (keyPtr->newKey.keyUsage != TAF_KS_HMAC_SIGN_ONLY) &&
         (keyPtr->newKey.keyUsage != TAF_KS_HMAC_VERIFY_ONLY)))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Provision the HMAC key.
    keyUsage = (keyPtr->newKey.keyUsage == TAF_KS_HMAC_SIGN_VERIFY) ?
        TAF_PA_KS_SIGN_VERIFY :
        ((keyPtr->newKey.keyUsage == TAF_KS_HMAC_SIGN_ONLY) ?
        TAF_PA_KS_SIGN_ONLY : TAF_PA_KS_VERIFY_ONLY);

    taf_pa_result_t paResult = taf_pa_ks_GenerateHmacKey(
                                       le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                                       keyPtr->newKey.keyId,
                                       keySize,
                                       keyUsage,
                                       static_cast<taf_pa_ks_Digest_t>(digest),
                                       (const taf_pa_ks_Tag_t**)TagPtrList_,
                                       0,
                                       impDataPtr,
                                       impDataSize,
                                       &keyFileRef);
    result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_GenerateHmacKey reason=%d", paResult);
    }
    if (result == LE_OK)
    {
        // Clear the tag list after successful provisioned.
        ClearTagList(keyPtr);

        // set key provisioned.
        keyPtr->keyType = KS_PROVISIONED_KEY;
        keyPtr->proKey.keyFileRef = keyFileRef;
        keyPtr->proKey.cryptoSessionList = LE_DLS_LIST_INIT;
        keyPtr->proKey.sharedAppList = LE_DLS_LIST_INIT;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Export a key into specified key data format.
 *
 * For AES and HMAC key the API exports raw key data. For RSA and ECDSA key the API exports x.509
 * DER format certificate which only contains the public key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::ExportKey
(
    taf_ks_KeyRef_t    keyRef, ///< [IN] Key reference
    const uint8_t* appDataPtr, ///< [IN] Application data
    size_t appDataSize,        ///< [IN]
    uint8_t* expDataPtr,       ///< [OUT] Export data
    size_t* expDataSizePtr     ///< [INOUT]
)
{
    if ((expDataPtr == NULL) || (expDataSizePtr == NULL))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed to export the key.
    if (keyPtr->keyType != KS_PROVISIONED_KEY)
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Export the key data.
    taf_pa_result_t paResult = taf_pa_ks_ExportKey(
                               le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                               keyPtr->proKey.keyFileRef,
                               appDataPtr,
                               appDataSize,
                               expDataPtr,
                               expDataSizePtr);
    le_result_t result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_ExportKey reason=%d", paResult);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Share a key to another application to use. Key sharing is persistent until cancelled by API
 * taf_ks_CancelKeySharing().
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::ShareKey
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference.
    const char* LE_NONNULL appName,
        ///< [IN] Shared application name.
    taf_ks_KeyUsage_t keyCap,
        ///< [IN] Shared key capability.
    taf_ks_AppCapMask_t appCap
        ///< [IN] Shared application capability.
)
{
    if (appName == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    char myAppName[TAF_KS_MAX_APP_NAME_SIZE + 1] = { 0 };

    // Get appName from clientSession.
    if (LE_OK != GetAppNameBySessionRef(taf_ks_GetClientSessionRef(),
                                        myAppName, sizeof(myAppName)))
    {
        LE_ERROR("Failed to get client appName.");
        return LE_FAULT;
    }

    // Check if the key owner is the calling application.
    if (strcmp(myAppName, appName) == 0)
    {
        LE_ERROR("The key owner app is the calling app('%s').", appName);
        return LE_FAULT;
    }

    taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed to share.
    if (keyPtr->keyType != KS_PROVISIONED_KEY)
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Export the key data.
    taf_pa_result_t paResult = taf_pa_ks_ShareKey(
                              le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                              keyPtr->proKey.keyFileRef,
                              static_cast<taf_pa_ks_KeyUsage_t>(keyCap),
                              appCap,
                              appName);
    le_result_t result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_ShareKey reason=%d", paResult);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Cancel the key sharing. Only the key owner app can cancel the key sharing.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::CancelKeySharing
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference.
    const char* LE_NONNULL appName
        ///< [IN] Name of the app that the key is shared to before.
)
{
    if (appName == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed to share.
    if (keyPtr->keyType != KS_PROVISIONED_KEY)
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Check if the key has running crypto sessions.
    if (HasRunningCryptoSession(keyPtr->proKey.cryptoSessionList))
    {
        LE_WARN("Key has running crypto session.");
        return LE_NOT_PERMITTED;
    }

    // Cancel the key sharing.
    taf_pa_result_t paResult = taf_pa_ks_CancelKeySharing(
                                      le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                                      keyPtr->proKey.keyFileRef,
                                      appName);
    le_result_t result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_CancelKeySharing reason=%d", paResult);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the application name of the calling client. It returns app name for a TelAF application
 * or returns the full path of the executable for a legacy application.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::GetCallingAppName
(
    char* appName,
        ///< [OUT] Application name.
    size_t appNameSize
        ///< [IN]
)
{
    return GetAppNameBySessionRef(taf_ks_GetClientSessionRef(), appName, appNameSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the first application that the given key is shared to.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::GetFirstSharedApp
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference.
    char* appName,
        ///< [OUT] Shared application name.
    size_t appNameSize,
        ///< [IN]
    taf_ks_KeyUsage_t* keyCapPtr,
        ///< [OUT] Shared key capability.
    taf_ks_AppCapMask_t* appCapPtr
        ///< [OUT] Shared application capability.
)
{
    if ((appName == NULL) || (appNameSize == 0) ||
        (keyCapPtr == NULL) || (appCapPtr == NULL))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed to share.
    if (keyPtr->keyType != KS_PROVISIONED_KEY)
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    le_msg_SessionRef_t clientSessionRef = taf_ks_GetClientSessionRef();
    taf_pa_ks_sharedAppList_t appList;
    memset(&appList, 0, sizeof(appList));

    // Get the sharing state of the key from PA layer.
    taf_pa_result_t paResult = taf_pa_ks_GetSharedAppList(
                                                    le_msg_GetClientFd(clientSessionRef),
                                                    keyPtr->proKey.keyFileRef, &appList);
    le_result_t result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK && paResult != TAF_PA_NOT_FOUND)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_GetSharedAppList reason=%d", paResult);
    }

    if (result != LE_OK)
    {
        // Key is not shared to any applications, return LE_NOT_FOUND.
        return LE_NOT_FOUND;
    }

    // Update the appList of the key for this client.
    SharedAppList_t* appListPtr = UpdateAppListForClient(clientSessionRef, keyPtr, &appList);
    if (appListPtr == NULL)
    {
        return LE_NOT_FOUND;
    }

    // Now get the first shareApp object.
    SharedApp_t* appPtr = NULL;
    appListPtr->currPtr = le_sls_Peek(&(appListPtr->appList));
    if (appListPtr->currPtr == NULL)
    {
        return LE_NOT_FOUND;
    }

    appPtr = CONTAINER_OF(appListPtr->currPtr, SharedApp_t, link);

    le_utf8_Copy(appName, appPtr->appInfo.appName, appNameSize, NULL);
    *keyCapPtr = static_cast<taf_ks_KeyUsage_t>(appPtr->appInfo.keyCap);
    *appCapPtr = appPtr->appInfo.appCap;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next application that the given key is shared to.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::GetNextSharedApp
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference.
    char* appName,
        ///< [OUT] Shared application name.
    size_t appNameSize,
        ///< [IN]
    taf_ks_KeyUsage_t* keyCapPtr,
        ///< [OUT] Shared key capability.
    taf_ks_AppCapMask_t* appCapPtr
        ///< [OUT] Shared application capability.
)
{
    if ((appName == NULL) || (appNameSize == 0) ||
        (keyCapPtr == NULL) || (appCapPtr == NULL))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    taf_ks_Key_t* keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed to share.
    if (keyPtr->keyType != KS_PROVISIONED_KEY)
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    le_msg_SessionRef_t clientSessionRef = taf_ks_GetClientSessionRef();
    SharedAppList_t* appListPtr = NULL;
    SharedApp_t* appPtr = NULL;

    le_dls_Link_t* linkPtr = le_dls_Peek(&(keyPtr->proKey.sharedAppList));
    while (linkPtr != NULL)
    {
        appListPtr = CONTAINER_OF(linkPtr, SharedAppList_t, link);
        linkPtr = le_dls_PeekNext(&(keyPtr->proKey.sharedAppList), linkPtr);

        if ((appListPtr != NULL) && (appListPtr->clientSessionRef == clientSessionRef))
        {
            appListPtr->currPtr = le_sls_PeekNext(&(appListPtr->appList), appListPtr->currPtr);
            if (appListPtr->currPtr != NULL)
            {
                appPtr = CONTAINER_OF(appListPtr->currPtr, SharedApp_t, link);
                le_utf8_Copy(appName, appPtr->appInfo.appName, appNameSize, NULL);
                *keyCapPtr = static_cast<taf_ks_KeyUsage_t>(appPtr->appInfo.keyCap);
                *appCapPtr = appPtr->appInfo.appCap;

                return LE_OK;
            }

            // Free the appList.
            ClearAppList(appListPtr);
            le_dls_Remove(&(keyPtr->proKey.sharedAppList), &(appListPtr->link));
            le_mem_Release(appListPtr);

            return LE_NOT_FOUND;
        }
    }

    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_ks_KeySharing'
 */
//--------------------------------------------------------------------------------------------------
taf_ks_KeySharingHandlerRef_t taf_KeyStore::AddKeySharingHandler
(
    const char* LE_NONNULL keyId,
        ///< [IN] Key ID.
    const char* LE_NONNULL appName,
        ///< [IN] Key owner app.
    taf_ks_KeySharingHandlerFunc_t handlerPtr,
        ///< [IN] Handler is called when the required key of the
        ///< application shared to the calling application.
    void* contextPtr
        ///< [IN]
)
{
    // Parameter check.
    if ((appName == NULL) || (keyId == NULL) || (handlerPtr == NULL))
    {
        LE_ERROR("Bad parameter.");
        return NULL;
    }

    char myAppName[TAF_KS_MAX_APP_NAME_SIZE + 1] = { 0 };

    // Get appName from clientSession.
    if (LE_OK != GetAppNameBySessionRef(taf_ks_GetClientSessionRef(),
                                        myAppName, sizeof(myAppName)))
    {
        LE_ERROR("Failed to get client appName.");
        return NULL;
    }

    // Check if keyId string follows the POSIX file name format.
    if (!IsPosixFileNameFormat(keyId))
    {
        LE_ERROR("Invalid key ID format (%s).", keyId);
        return NULL;
    }

    // Check if the key owner is the calling application.
    if (strcmp(myAppName, appName) == 0)
    {
        LE_ERROR("The key owner app is the calling app('%s').", appName);
        return NULL;
    }

    // Create a handler object.
    taf_ks_Handler_t* myHandlerPtr =
        static_cast<taf_ks_Handler_t*>(le_mem_ForceAlloc(HandlerPool_));
    memset(myHandlerPtr, 0, sizeof(taf_ks_Handler_t));

    // Save handler info.
    myHandlerPtr->clientSessionRef = taf_ks_GetClientSessionRef();
    myHandlerPtr->handleFunc = handlerPtr;
    myHandlerPtr->context = contextPtr;

    // Save the keyId and ownerAppName.
    le_utf8_Copy(myHandlerPtr->keyId, keyId, sizeof(myHandlerPtr->keyId), NULL);
    le_utf8_Copy(myHandlerPtr->ownerAppName, appName, sizeof(myHandlerPtr->ownerAppName), NULL);

    // Save sharing state and handler reference.
    myHandlerPtr->state = TAF_KS_SHARING_DISABLED;
    myHandlerPtr->handlerRef = static_cast<taf_ks_KeySharingHandlerRef_t>(
                                    le_ref_CreateRef(HandlerRefMap_, myHandlerPtr));

    // Get the sharing state of the desired key.
    KeyMgt_KeyFileRef_t keyFileRef = NULL;
    taf_pa_result_t paResult = taf_pa_ks_GetSharedKey(
                                                le_msg_GetClientFd(myHandlerPtr->clientSessionRef),
                                                myHandlerPtr->keyId,
                                                myHandlerPtr->ownerAppName,
                                                &keyFileRef);
    le_result_t result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK && paResult != TAF_PA_NOT_FOUND)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_GetSharedKey reason=%d", paResult);
    }

    // Trigger the handler immediately if the key is already shared to the client.
    if ((result == LE_OK) && (keyFileRef != NULL))
    {
        myHandlerPtr->state = TAF_KS_SHARING_ENABLED;
        myHandlerPtr->handleFunc(myHandlerPtr->keyId, myHandlerPtr->ownerAppName,
                                 myHandlerPtr->state, myHandlerPtr->context);
    }

    return myHandlerPtr->handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_ks_KeySharing'
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::RemoveKeySharingHandler
(
    taf_ks_KeySharingHandlerRef_t handlerRef
        ///< [IN]
)
{
    taf_ks_Handler_t* myHandlerPtr = static_cast<taf_ks_Handler_t*>(
                                          le_ref_Lookup(HandlerRefMap_, handlerRef));
    if (myHandlerPtr != NULL)
    {
        le_ref_DeleteRef(HandlerRefMap_, handlerRef);
        le_mem_Release(myHandlerPtr);
    }

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a crypto operation session for the key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::CryptoSessionCreate
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key to use for this crypto session.
    taf_ks_CryptoSessionRef_t* sessionRefPtr
        ///< [OUT] Session reference.
)
{
    taf_ks_Key_t* keyPtr = NULL;
    taf_ks_CryptoSession_t* sessionPtr = NULL;

    if (sessionRefPtr == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only the session of provisioned key
    // is allowed to create crypto sessions.
    if (keyPtr->keyType != KS_PROVISIONED_KEY)
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Create a new crypto session for the key.
    sessionPtr = static_cast<taf_ks_CryptoSession_t*>(le_mem_ForceAlloc(CryptoSessionPool_));
    memset(sessionPtr, 0, sizeof(taf_ks_CryptoSession_t));
    sessionPtr->clientSessionRef = taf_ks_GetClientSessionRef();
    sessionPtr->started = false;
    sessionPtr->handle = 0;
    sessionPtr->paramList = LE_DLS_LIST_INIT;
    sessionPtr->link = LE_DLS_LINK_INIT;
    sessionPtr->keyRef = keyPtr->keyRef;
    sessionPtr->cryptoSessionRef = static_cast<taf_ks_CryptoSessionRef_t>(
                                        le_ref_CreateRef(CryptoSessionRefMap_, sessionPtr));

    // Add the session into the session list of the provisioned key.
    le_dls_Queue(&(keyPtr->proKey.cryptoSessionList), &(sessionPtr->link));
    *sessionRefPtr = sessionPtr->cryptoSessionRef;

    LE_DEBUG("A new crypto session(%p) for provisioned key(%p) created.",
            sessionPtr->cryptoSessionRef, keyPtr->keyRef);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the nonce or IVs for AES GCM, CBC, CTR for a crypto session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::CryptoSessionSetAesNonce
(
    taf_ks_CryptoSessionRef_t sessionRef,
        ///< [IN] Session reference
    const uint8_t* dataPtr,
        ///< [IN] Data buffer to hold the nonce or IV
    size_t dataSize
        ///< [IN]
)
{
    taf_ks_CryptoSession_t* sessionPtr = NULL;
    taf_ks_Key_t* keyPtr = NULL;

    if ((dataPtr == NULL) ||
        ((dataSize != TAF_PA_KS_AES_GCM_NONCE_SIZE) &&
         (dataSize != TAF_PA_KS_AES_CBC_NONCE_SIZE)))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    sessionPtr =
        static_cast<taf_ks_CryptoSession_t*>(le_ref_Lookup(CryptoSessionRefMap_, sessionRef));
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, sessionPtr->keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed.
    if ((keyPtr->keyType != KS_PROVISIONED_KEY) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    KeyParam_t* newParamPtr = static_cast<KeyParam_t*>(le_mem_ForceAlloc(ParamPool_));
    memset(newParamPtr, 0, sizeof(KeyParam_t));

    newParamPtr->link = LE_DLS_LINK_INIT;
    newParamPtr->param.id = TAF_PA_KS_PARAM_NONCE;
    newParamPtr->param.nonceDataPtr = static_cast<taf_pa_ks_Nonce_t*>(
                                            le_mem_ForceAlloc(AesNoncePool_));
    memset(newParamPtr->param.nonceDataPtr, 0, sizeof(taf_pa_ks_Nonce_t));

    memcpy(newParamPtr->param.nonceDataPtr->data, dataPtr, dataSize);
    newParamPtr->param.nonceDataPtr->size = static_cast<taf_pa_ks_NonceSize_t>(dataSize);
    SetParam(sessionPtr, newParamPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the general padding type used for a RSA signing/verification cryptographic session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::CryptoSessionSetRsaPadding
(
    taf_ks_CryptoSessionRef_t sessionRef,
        ///< [IN] [IN] Session reference for a RSA signing key.
    taf_ks_RsaPaddingType_t paddingType
        ///< [IN] [IN] General padding type.
)
{
    taf_ks_CryptoSession_t* sessionPtr = NULL;
    taf_ks_Key_t* keyPtr = NULL;

    if ((paddingType != TAF_KS_RSA_PKCS1_V15) && (paddingType != TAF_KS_RSA_PSS))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    sessionPtr =
        static_cast<taf_ks_CryptoSession_t*>(le_ref_Lookup(CryptoSessionRefMap_, sessionRef));
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, sessionPtr->keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed.
    if ((keyPtr->keyType != KS_PROVISIONED_KEY) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    KeyParam_t* newParamPtr = static_cast<KeyParam_t*>(le_mem_ForceAlloc(ParamPool_));
    memset(newParamPtr, 0, sizeof(KeyParam_t));

    newParamPtr->link = LE_DLS_LINK_INIT;
    newParamPtr->param.id = TAF_PA_KS_PARAM_RSA_PADDING_TYPE;
    newParamPtr->param.rsaPaddingType = static_cast<taf_pa_ks_RsaPaddingType_t>(paddingType);

    SetParam(sessionPtr, newParamPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Application data for a crypto session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::CryptoSessionSetAppData
(
    taf_ks_CryptoSessionRef_t sessionRef,
        ///< [IN] Session reference
    const uint8_t* dataPtr,
        ///< [IN] Data buffer to hold the application data
    size_t dataSize
        ///< [IN]
)
{
    taf_ks_CryptoSession_t* sessionPtr = NULL;
    taf_ks_Key_t* keyPtr = NULL;

    if ((dataPtr == NULL) || (dataSize == 0))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    sessionPtr =
        static_cast<taf_ks_CryptoSession_t*>(le_ref_Lookup(CryptoSessionRefMap_, sessionRef));
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, sessionPtr->keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed
    // to set the session parameter.
    if ((keyPtr->keyType != KS_PROVISIONED_KEY) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    KeyParam_t* newParamPtr = static_cast<KeyParam_t*>(le_mem_ForceAlloc(ParamPool_));
    memset(newParamPtr, 0, sizeof(KeyParam_t));

    newParamPtr->link = LE_DLS_LINK_INIT;
    newParamPtr->param.id = TAF_PA_KS_PARAM_APPLICATION_DATA;
    newParamPtr->param.appDataPtr = static_cast<taf_pa_ks_Data_t*>(le_mem_ForceAlloc(DataPool_));
    memset(newParamPtr->param.appDataPtr, 0, sizeof(taf_pa_ks_Data_t));

    memcpy(newParamPtr->param.appDataPtr->data, dataPtr, dataSize);
    newParamPtr->param.appDataPtr->size = dataSize;
    SetParam(sessionPtr, newParamPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start the crypto session for the given crypto operation.
 *
 * The crypto session will be automatically deleted if error is returned.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::CryptoSessionStart
(
    taf_ks_CryptoSessionRef_t sessionRef,
        ///< [IN] Session reference
    taf_ks_CryptoPurpose_t cryptoPurpose
        ///< [IN] Crypto purpose
)
{
    le_result_t result = LE_OK;
    taf_ks_CryptoSession_t* sessionPtr = NULL;
    taf_ks_Key_t* keyPtr = NULL;
    uint64_t handle = 0;

    if (cryptoPurpose >= TAF_KS_CRYPTO_MAX)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    sessionPtr =
        static_cast<taf_ks_CryptoSession_t*>(le_ref_Lookup(CryptoSessionRefMap_, sessionRef));
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, sessionPtr->keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed
    if ((keyPtr->keyType != KS_PROVISIONED_KEY)||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Check if the session is already started.
    if (sessionPtr->started)
    {
        LE_WARN("Session(%p) for provisioned key(%p) is already started.",
                sessionPtr, keyPtr->keyRef);
        return LE_DUPLICATE;
    }

    // Populate the param list of the session to PA layer.
    uint8_t pNum = 0;
    KeyParam_t* paramObjPtr = NULL;
    le_dls_Link_t* linkPtr = le_dls_Peek(&(sessionPtr->paramList));
    while (linkPtr)
    {
        paramObjPtr = CONTAINER_OF(linkPtr, KeyParam_t, link);
        linkPtr = le_dls_PeekNext(&(sessionPtr->paramList), linkPtr);

        if ((pNum < MAX_PARAM_NUMBER) && (paramObjPtr != NULL))
        {
            ParamPtrList_[pNum] = &(paramObjPtr->param);
            pNum++;
        }
    }

    taf_pa_result_t paResult = taf_pa_ks_CryptoSessionStart(
                                          le_msg_GetClientFd(taf_ks_GetClientSessionRef()),
                                          keyPtr->proKey.keyFileRef,
                                          static_cast<taf_pa_ks_CryptoPurpose_t>(cryptoPurpose),
                                          (const taf_pa_ks_Param_t**)ParamPtrList_,
                                          pNum,
                                          &handle);
    result = ConvertPaToLeRet(paResult);
    if (result != LE_OK && result != LE_DUPLICATE)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_CryptoSessionStart reason=%d", paResult);
    }
    if (result == LE_OK)
    {
        // Save the handle and set the session as started.
        sessionPtr->started = true;
        sessionPtr->handle = handle;
        ClearParamList(sessionPtr);
    }
    else
    {
        // Remove the session if any error happens.
        le_dls_Remove(&(keyPtr->proKey.cryptoSessionList), &(sessionPtr->link));
        RemoveCryptoSession(sessionPtr);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Provides AES AEAD to the running crypto session started with CryptoSessionStart API for AES GCM
 * mode.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::CryptoSessionProcessAead
(
    taf_ks_CryptoSessionRef_t sessionRef,
        ///< [IN] Session reference
    const uint8_t* inputDataPtr,
        ///< [IN] Data buffer to hold the AEAD data
    size_t inputDataSize
        ///< [IN]
)
{
    le_result_t result = LE_OK;
    taf_ks_CryptoSession_t* sessionPtr = NULL;
    taf_ks_Key_t* keyPtr = NULL;

    if ((inputDataPtr == NULL) || (inputDataSize == 0))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    sessionPtr =
        static_cast<taf_ks_CryptoSession_t*>(le_ref_Lookup(CryptoSessionRefMap_, sessionRef));
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, sessionPtr->keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed
    if ((keyPtr->keyType != KS_PROVISIONED_KEY) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Check if the session is started.
    if (sessionPtr->started == false)
    {
        LE_WARN("Session(%p) for provisioned key(%p) is not started.",
                sessionPtr, keyPtr->keyRef);
        return LE_NOT_PERMITTED;
    }

    taf_pa_result_t paResult = taf_pa_ks_CryptoSessionProcessAead(sessionPtr->handle,
                                                inputDataPtr,
                                                inputDataSize);
    result = ConvertPaToLeRet(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_CryptoSessionProcessAead reason=%d", paResult);
        // Process failed means the session is already aborted.
        sessionPtr->started = false;
        sessionPtr->handle = 0;

        le_dls_Remove(&(keyPtr->proKey.cryptoSessionList), &(sessionPtr->link));
        RemoveCryptoSession(sessionPtr);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Provides data to, and possibly receives output from, an runing crypto session started with
 * CryptoSessionStart API.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::CryptoSessionProcess
(
    taf_ks_CryptoSessionRef_t sessionRef,
        ///< [IN] Session reference
    const uint8_t* inputDataPtr,
        ///< [IN] InputData can be one of below 4 cases:
        ///< 1: plain text for encryption session.
        ///< 2: cipher text for decryption session.
        ///< 3: message to sign for signing session.
        ///< 4: message to verify for verification session.
    size_t inputDataSize,
        ///< [IN]
    uint8_t* outputDataPtr,
        ///< [OUT] OutputData can be one of below 3 cases:
        ///< 1: encrypted data for encryption session.
        ///< 2: decrypted data for decryption session.
        ///< 3: Ignre for signing and verification sessions.
    size_t* outputDataSizePtr
        ///< [INOUT]
)
{
    le_result_t result = LE_OK;
    taf_ks_CryptoSession_t* sessionPtr = NULL;
    taf_ks_Key_t* keyPtr = NULL;

    if ((inputDataPtr == NULL) || (inputDataSize == 0))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    sessionPtr =
        static_cast<taf_ks_CryptoSession_t*>(le_ref_Lookup(CryptoSessionRefMap_, sessionRef));
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, sessionPtr->keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed
    if ((keyPtr->keyType != KS_PROVISIONED_KEY) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Check if the session is started.
    if (sessionPtr->started == false)
    {
        LE_WARN("Session(%p) for provisioned key(%p) is not started.",
                sessionPtr, keyPtr->keyRef);
        return LE_NOT_PERMITTED;
    }

    taf_pa_result_t paResult = taf_pa_ks_CryptoSessionProcess(sessionPtr->handle,
                                            inputDataPtr,
                                            inputDataSize,
                                            outputDataPtr,
                                            outputDataSizePtr);
    result = ConvertPaToLeRet(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_CryptoSessionProcess reason=%d", paResult);
        // Process failed means the session is already aborted.
        sessionPtr->started = false;
        sessionPtr->handle = 0;

        le_dls_Remove(&(keyPtr->proKey.cryptoSessionList), &(sessionPtr->link));
        RemoveCryptoSession(sessionPtr);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Finalizes the crypto session started with CryptoSessionStart API.
 *
 * The crypto session will be then automatically deleted.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::CryptoSessionEnd
(
    taf_ks_CryptoSessionRef_t sessionRef,
        ///< [IN] Session reference
    const uint8_t* inputDataPtr,
        ///< [IN] Signature to verify for verification session type
        ///< and ignored for other session types.
    size_t inputDataSize,
        ///< [IN]
    uint8_t* outputDataPtr,
        ///< [OUT] OutputData can be one of below 3 cases:
        ///< 1: encrypted data for encryption session.
        ///< 2: decrypted data for decryption session.
        ///< 3: signature for signing session.
        ///<    for verfication session.
    size_t* outputDataSizePtr
        ///< [INOUT]
)
{
    le_result_t result = LE_OK;
    taf_ks_CryptoSession_t* sessionPtr = NULL;
    taf_ks_Key_t* keyPtr = NULL;

    sessionPtr =
        static_cast<taf_ks_CryptoSession_t*>(le_ref_Lookup(CryptoSessionRefMap_, sessionRef));
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, sessionPtr->keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed
    if ((keyPtr->keyType != KS_PROVISIONED_KEY) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Check if the session is started.
    if (sessionPtr->started == false)
    {
        LE_WARN("Session(%p) for provisioned key(%p) is not started.",
                sessionPtr, keyPtr->keyRef);
        return LE_NOT_PERMITTED;
    }

    taf_pa_result_t paResult = taf_pa_ks_CryptoSessionEnd(sessionPtr->handle,
                                        inputDataPtr,
                                        inputDataSize,
                                        outputDataPtr,
                                        outputDataSizePtr);
    result = ConvertPaToLeRet(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_CryptoSessionEnd reason=%d", paResult);
    }
    // Set the session is stopped.
    sessionPtr->started = false;
    sessionPtr->handle = 0;

    // Delete the crypto session.
    le_dls_Remove(&(keyPtr->proKey.cryptoSessionList), &(sessionPtr->link));
    RemoveCryptoSession(sessionPtr);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Abort the crypto session started with CryptoSessionStart API.
 *
 * The crypto session will be then automatically deleted.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_KeyStore::CryptoSessionAbort
(
    taf_ks_CryptoSessionRef_t sessionRef
        ///< [IN] Session reference
)
{
    le_result_t result = LE_OK;
    taf_ks_CryptoSession_t* sessionPtr = NULL;
    taf_ks_Key_t* keyPtr = NULL;

    sessionPtr =
        static_cast<taf_ks_CryptoSession_t*>(le_ref_Lookup(CryptoSessionRefMap_, sessionRef));
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = static_cast<taf_ks_Key_t*>(le_ref_Lookup(KeyRefMap_, sessionPtr->keyRef));
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed.
    if ((keyPtr->keyType != KS_PROVISIONED_KEY) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Check if the session is started.
    if (sessionPtr->started == false)
    {
        LE_WARN("Session(%p) for provisioned key(%p) is not started.",
                sessionPtr, keyPtr->keyRef);
        return LE_NOT_PERMITTED;
    }

    // Abort the session.
    taf_pa_result_t paResult = taf_pa_ks_CryptoSessionAbort(sessionPtr->handle);
    result = ConvertPaToLeRet(paResult);
    if (result != LE_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_CryptoSessionAbort reason=%d", paResult);
    }
    sessionPtr->started = false;
    sessionPtr->handle = 0;

    // Delete the crypto session.
    le_dls_Remove(&(keyPtr->proKey.cryptoSessionList), &(sessionPtr->link));
    RemoveCryptoSession(sessionPtr);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * The handler for PA initialization check.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::InitTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    static uint8_t Count = 0;
    taf_KeyStore& myKeyStore = taf_KeyStore::GetInstance();

    taf_pa_result_t paResult = taf_pa_ks_Init();
    le_result_t result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK && paResult != TAF_PA_IO_ERROR)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_Init reason=%d", paResult);
    }
    if (result == LE_OK)
    {
        LE_INFO("TelAF keyStore PA initialized. (count=%d)", Count);

        // Advertise the service.
        taf_ks_AdvertiseService();

        // Set session close handlers.
        le_msg_AddServiceCloseHandler(taf_ks_GetServiceRef(),
                                       taf_KeyStore::RemoveNewKeysForClient, NULL);
        le_msg_AddServiceCloseHandler(taf_ks_GetServiceRef(),
                                       taf_KeyStore::RemoveCryptoSessionsForClient, NULL);
        le_msg_AddServiceCloseHandler(taf_ks_GetServiceRef(),
                                       taf_KeyStore::RemoveAppListsForClient, NULL);

        le_timer_Delete(timerRef);
        myKeyStore.InitTimer_ = NULL;

        // Add boot KPI marker
        SetBootKpiMarker("L - TelAF keystore service is ready");

        return;
    }

    if ((result == LE_IO_ERROR) && (Count++ < 10))
    {
        LE_WARN("Continue for service initialization (count=%d).", Count);
    }
    else
    {
        le_timer_Delete(timerRef);
        myKeyStore.InitTimer_ = NULL;

        LE_FATAL("Service initialization failed.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The keyStore service's initialization method. Replaces the original COMPONENT_INIT body.
 */
//--------------------------------------------------------------------------------------------------
void taf_KeyStore::Init()
{
    // Create memory pools
    CryptoSessionPool_ = le_mem_CreatePool("CryptoSessionPool", sizeof(taf_ks_CryptoSession_t));
    KeyPool_ = le_mem_CreatePool("KeyPool", sizeof(taf_ks_Key_t));

    TagPool_ = le_mem_CreatePool("TagPool", sizeof(KeyTag_t));
    ParamPool_ = le_mem_CreatePool("ParamPool", sizeof(KeyParam_t));

    AesNoncePool_ = le_mem_CreatePool("AesNoncePool", sizeof(taf_pa_ks_Nonce_t));
    DataPool_ = le_mem_CreatePool("DataPool", sizeof(taf_pa_ks_Data_t));

    HandlerPool_ = le_mem_CreatePool("HandlerPool", sizeof(taf_ks_Handler_t));
    SharedAppPool_ = le_mem_CreatePool("SharedAppPool", sizeof(SharedApp_t));
    SharedAppListPool_ = le_mem_CreatePool("SharedAppListPool", sizeof(SharedAppList_t));

    // Create reference maps
    CryptoSessionRefMap_ = le_ref_CreateMap("CryptoSessionRefMap", 20);
    KeyRefMap_ = le_ref_CreateMap("KeyRefMap", 500);
    HandlerRefMap_ = le_ref_CreateMap("KeySharingHandler", 20);

    // Create key event
    KeyEventId_ = le_event_CreateId("KeyEvent", sizeof(KeyEvent_t));
    le_event_AddHandler("KeyEventhandler", KeyEventId_, taf_KeyStore::KeyEventHandler);

    // Register callback functions in PA layer.
    taf_pa_result_t regCreationResult =
        taf_pa_ks_RegKeyCreationHandler(taf_KeyStore::KeyCreationPAHandler);
    if (regCreationResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_RegKeyCreationHandler reason=%d",
            regCreationResult);
    }

    taf_pa_result_t regSharingResult =
        taf_pa_ks_RegKeySharingHandler(taf_KeyStore::KeySharingPAHandler);
    if (regSharingResult != TAF_PA_OK)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_RegKeySharingHandler reason=%d",
            regSharingResult);
    }

    taf_pa_result_t paResult = taf_pa_ks_Init();
    le_result_t result = ConvertPaToLeRet(paResult);
    if (paResult != TAF_PA_OK && paResult != TAF_PA_IO_ERROR)
    {
        LE_ERROR("[PA_CALL_FAILED] api=taf_pa_ks_Init reason=%d", paResult);
    }
    if (LE_OK == result)
    {
        LE_INFO("TelAF keyStore PA initialized.");

        // Advertise the service.
        taf_ks_AdvertiseService();

        // Set session close handlers.
        le_msg_AddServiceCloseHandler(taf_ks_GetServiceRef(), taf_KeyStore::RemoveNewKeysForClient,
                                       NULL);
        le_msg_AddServiceCloseHandler(taf_ks_GetServiceRef(),
                                       taf_KeyStore::RemoveCryptoSessionsForClient, NULL);
        le_msg_AddServiceCloseHandler(taf_ks_GetServiceRef(), taf_KeyStore::RemoveAppListsForClient,
                                       NULL);
        // Add boot KPI marker
        SetBootKpiMarker("L - TelAF keystore service is ready");
    }
    else if (LE_IO_ERROR == result)
    {
        // keyMaster initialization failed, start a timer for later initialization.
        InitTimer_ = le_timer_Create("InitTimer");
        le_timer_SetMsInterval(InitTimer_, 1000);
        le_timer_SetHandler(InitTimer_, taf_KeyStore::InitTimerHandler);
        le_timer_SetRepeat(InitTimer_, 0); // repeat indefinitely
        le_timer_SetWakeup(InitTimer_, false); // Non-wakeup
        le_timer_Start(InitTimer_);
    }
    else
    {
        LE_FATAL("taf_pa_ks_Init() failed.");
    }
}
