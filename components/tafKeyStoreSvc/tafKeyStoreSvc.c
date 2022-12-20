/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "legato.h"
#include "interfaces.h"

#include "taf_pa_keystore.h"

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Session node structure used by a crypto opertion.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint64_t            handle;                  ///< Handle of the crypto operation
    bool                started;                 ///< True if the session is started
    le_dls_List_t       paramList;               ///< Parameter list
    le_dls_Link_t       link;                    ///< Link to key's cryptoSessionList
    le_msg_SessionRef_t clientSessionRef;        ///< Client session reference
    taf_ks_CryptoSessionRef_t cryptoSessionRef;  ///< Crypto session reference
    taf_ks_KeyRef_t     keyRef;                  ///< Key reference
}
taf_ks_CryptoSession_t;

//--------------------------------------------------------------------------------------------------
/**
 * Key structure for new created keys. New key is not allowed for any crypto operations until it's
 * provisioned with proper key value.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char keyName[TAF_KS_MAX_KEY_ID_SIZE+1];///< Key name string
    le_msg_SessionRef_t clientSessionRef;  ///< Client session reference
    taf_ks_KeyUsage_t keyUsage;            ///< Key usage
    le_dls_List_t tagList;                 ///< Tag list set to the new key
}
taf_ks_NewKey_t;

//--------------------------------------------------------------------------------------------------
/**
 * Key structure for standard keys.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_ks_NewKey_t*    newKeyPtr;         ///< new key reference if the key is new created.
    KeyMgt_KeyFileRef_t keyFilePtr;        ///< Key file reference to the key file
    le_dls_List_t       cryptoSessionList; ///< Crypto session list
    taf_ks_KeyRef_t     keyRef;            ///< Reference to the key
}
taf_ks_Key_t;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for crypto sessions.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t CryptoSessionRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for standard keys.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t KeyRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for crypto sessions.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t CryptoSessionPool;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for new keys.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t NewKeyPool;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for standard keys.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t KeyPool;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for KS tags.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t TagPool;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for KS parameters.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ParamPool;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for AES nonce.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AesNoncePool;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for common buffer.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t DataPool;

//--------------------------------------------------------------------------------------------------
/**
 * Search a key(value already provisioned) by keyFile reference
 */
//--------------------------------------------------------------------------------------------------
static taf_ks_Key_t* SearchProvisionedKey
(
    KeyMgt_KeyFileRef_t keyFileRef ///< [IN] Key reference
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(KeyRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_ks_Key_t* keyPtr = le_ref_GetValue(iterRef);
        LE_ASSERT(keyPtr != NULL);

        if ((keyPtr->newKeyPtr == NULL) &&
            (keyPtr->keyFilePtr != NULL) &&
            (keyPtr->keyFilePtr == keyFileRef))
        {
            LE_INFO("found a provisioned key(%p).", keyPtr->keyFilePtr);
            return keyPtr;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Search a new created key(value not provisioned yet) by key name.
 */
//--------------------------------------------------------------------------------------------------
static taf_ks_Key_t* SearchNewKey
(
    const char* keyName ///< [IN] Key reference
)
{
    le_ref_IterRef_t iterRef = le_ref_GetIterator(KeyRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_ks_Key_t* keyPtr = le_ref_GetValue(iterRef);
        LE_ASSERT(keyPtr != NULL);

        if ((keyPtr->keyFilePtr == NULL) &&
            (keyPtr->newKeyPtr != NULL) &&
            (keyPtr->newKeyPtr->clientSessionRef == taf_ks_GetClientSessionRef()) &&
            (0 == strcmp(keyName, keyPtr->newKeyPtr->keyName)))
        {
            LE_INFO("found a new key(%p) for keyName '%s'.", keyPtr->newKeyPtr, keyName);
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
static le_result_t SetTag
(
    taf_ks_Key_t*    keyPtr,   ///< [IN] Key pointer
    taf_pa_ks_Tag_t* setTagPtr ///< [IN] Tag pointer
)
{
    taf_pa_ks_Tag_t* tagPtr = NULL;

    if ((setTagPtr == NULL) || (setTagPtr->id >= TAF_PA_KS_TAG_MAX_IDS) || (keyPtr == NULL))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    // Check if it's a new key, only new key is allowed to set the tag.
    if ((keyPtr->newKeyPtr == NULL) || (keyPtr->keyFilePtr != NULL))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Check if the tag is created already for this new key.
    le_dls_Link_t* linkPtr = le_dls_Peek(&(keyPtr->newKeyPtr->tagList));

    while (linkPtr)
    {
        tagPtr = CONTAINER_OF(linkPtr, taf_pa_ks_Tag_t, link);
        linkPtr = le_dls_PeekNext(&(keyPtr->newKeyPtr->tagList), linkPtr);

        if (tagPtr->id == setTagPtr->id)
        {
            // Remove the old tag from the tagList of the new key.
            LE_INFO("Remove old tag(id = %u) for new keyName '%s'.",
                    tagPtr->id, keyPtr->newKeyPtr->keyName);
            le_dls_Remove(&(keyPtr->newKeyPtr->tagList), &(tagPtr->link));

            // Release the tag object
            if ((tagPtr->id == TAF_PA_KS_TAG_APPLICATION_DATA) && (tagPtr->appDataPtr != NULL))
            {
                le_mem_Release(tagPtr->appDataPtr);
            }
            le_mem_Release(tagPtr);

            break;
        }
    }

    // Create a new tag with sepcified id/value for the new key.
    LE_INFO("Add tag(id = %u) for new keyName '%s'.", setTagPtr->id, keyPtr->newKeyPtr->keyName);

    // Add the tag into the tagList of the new key.
    le_dls_Queue(&keyPtr->newKeyPtr->tagList, &(setTagPtr->link));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set or update a specified parameter for a crypto session.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetParam
(
    taf_ks_CryptoSession_t* sessionPtr,   ///< [IN] Cryption session pointer
    taf_pa_ks_Param_t*     setParamPtr    ///< [IN] Parameter pointer
)
{
    taf_pa_ks_Param_t* paramPtr = NULL;
    taf_ks_Key_t* keyPtr = NULL;

    if (sessionPtr == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, sessionPtr->keyRef);

    if ((setParamPtr == NULL) ||
        (setParamPtr->id >= TAF_PA_KS_PARAM_MAX_IDS) ||
        (keyPtr == NULL))
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    // Check if it's a provisioned key, only the session of provisioned key
    // is allowed to set the parameter.
    if ((keyPtr->newKeyPtr != NULL) || (keyPtr->keyFilePtr == NULL))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Check if the parameter is created already for this session.
    le_dls_Link_t* linkPtr = le_dls_Peek(&(sessionPtr->paramList));

    while (linkPtr)
    {
        paramPtr = CONTAINER_OF(linkPtr, taf_pa_ks_Param_t, link);

        linkPtr = le_dls_PeekNext(&(sessionPtr->paramList), linkPtr);

        if (paramPtr->id == setParamPtr->id)
        {
            // Remove the parameter from the paramList for the session.
            LE_INFO("Remove old param(id = %u) for session(%p) of provisioned key(%p)",
                    paramPtr->id, sessionPtr, keyPtr->keyFilePtr);
            le_dls_Remove(&(sessionPtr->paramList), &(paramPtr->link));

            // Free the sub parameter.
            switch(paramPtr->id)
            {
                case TAF_PA_KS_PARAM_NONCE:
                   le_mem_Release(paramPtr->nonceDataPtr);
                   break;

                case TAF_PA_KS_PARAM_APPLICATION_DATA:
                   le_mem_Release(paramPtr->appDataPtr);
                   break;

                default:
                   LE_FATAL("Unrecognized param(id = %u)", paramPtr->id);
                   break;
            }
            // Free the parameter.
            le_mem_Release(paramPtr);

            break;
        }
    }

    // Create a new parameter with sepcified id/value for the session.
    LE_INFO("Add parameter(id = %u) for session(%p) of provisioned key(%p)",
            setParamPtr->id, sessionPtr, keyPtr->keyFilePtr);

    // Add the paramter into the paramList of the session
    le_dls_Queue(&(sessionPtr->paramList), &(setParamPtr->link));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear the tag list of a new key. Remove and free all the tag objects in the list.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ClearTagList
(
    taf_ks_Key_t*    keyPtr   ///< [IN] Key pointer
)
{
    taf_pa_ks_Tag_t* tagPtr;

    if (keyPtr == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    // Check if it's a new key, only new key has the tag list.
    if ((keyPtr->newKeyPtr == NULL) || (keyPtr->keyFilePtr != NULL))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Go through the tagList, free tag object and remove the node.
    le_dls_Link_t* linkPtr = le_dls_Pop(&(keyPtr->newKeyPtr->tagList));

    while (linkPtr)
    {
        tagPtr = CONTAINER_OF(linkPtr, taf_pa_ks_Tag_t, link);
        LE_ASSERT((tagPtr != NULL) && (tagPtr->id < TAF_PA_KS_TAG_MAX_IDS));

        LE_INFO("Removed the tag(id = %u) of new keyName '%s'.",
                tagPtr->id, keyPtr->newKeyPtr->keyName);
        // Release the tag object
        if ((tagPtr->id == TAF_PA_KS_TAG_APPLICATION_DATA) && (tagPtr->appDataPtr != NULL))
        {
            le_mem_Release(tagPtr->appDataPtr);
        }
        le_mem_Release(tagPtr);

        linkPtr = le_dls_Pop(&(keyPtr->newKeyPtr->tagList));
    }

    // The list must be empty after clearing.
    LE_ASSERT(le_dls_NumLinks(&(keyPtr->newKeyPtr->tagList)) == 0);
    LE_INFO("Cleared tag list of new keyName '%s'", keyPtr->newKeyPtr->keyName);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear the parameter list of specified crypto session. Remove and free all the parameter objects
 * in the list.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ClearParamList
(
    taf_ks_CryptoSession_t* sessionPtr   ///< [IN] Cryption session pointer
)
{
    taf_pa_ks_Param_t* paramPtr = NULL;
    taf_ks_Key_t* keyPtr = NULL;

    if (sessionPtr == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, sessionPtr->keyRef);

    if (keyPtr == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    // Check if it's a provisioned key, only the session of provisioned key
    // has the parameter list.
    if ((keyPtr->newKeyPtr != NULL) || (keyPtr->keyFilePtr == NULL))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Go through the paramList, free parameter object and remove the node.
    le_dls_Link_t* linkPtr = le_dls_Pop(&(sessionPtr->paramList));

    while (linkPtr)
    {
        paramPtr = CONTAINER_OF(linkPtr, taf_pa_ks_Param_t, link);
        LE_ASSERT((paramPtr != NULL) && (paramPtr->id < TAF_PA_KS_PARAM_MAX_IDS));

        LE_INFO("Removed the param(id = %u) of session(%p) of provisioned key(%p)",
                paramPtr->id, sessionPtr, keyPtr->keyFilePtr);
        // Free the sub parameter.
        switch(paramPtr->id)
        {
            case TAF_PA_KS_PARAM_NONCE:
               le_mem_Release(paramPtr->nonceDataPtr);
               break;

            case TAF_PA_KS_PARAM_APPLICATION_DATA:
               le_mem_Release(paramPtr->appDataPtr);
               break;

            default:
               LE_FATAL("Unrecognized param(id = %u)", paramPtr->id);
               break;
        }
        // Free the parameter.
        le_mem_Release(paramPtr);

        linkPtr = le_dls_Pop(&(sessionPtr->paramList));
    }

    // The list must be empty after clearing.
    LE_ASSERT(le_dls_NumLinks(&(sessionPtr->paramList)) == 0);
    LE_INFO("Cleared param list of session(%p) of provisioned key(%p)",
            sessionPtr, keyPtr->keyFilePtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete a crypto operation session. It also aborts the runing crypto operation if the session is
 * started.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RemoveCryptoSession
(
    taf_ks_CryptoSession_t* sessionPtr
        ///< [IN] Crypto session ptr.
)
{
    if (sessionPtr == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    taf_ks_Key_t* keyPtr = le_ref_Lookup(KeyRefMap, sessionPtr->keyRef);
    if ( keyPtr == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    // Abort the session if it's started.
    if (sessionPtr->started)
    {
        taf_pa_ks_CryptoSessionAbort(sessionPtr->handle);
        sessionPtr->started = false;
        sessionPtr->handle = 0;
    }

    // Remove parameter list of the session.
    LE_ASSERT(LE_OK == ClearParamList(sessionPtr));

    // Delete the sessionRef and free the session.
    le_ref_DeleteRef(CryptoSessionRefMap, sessionPtr->cryptoSessionRef);
    le_mem_Release(sessionPtr);
    LE_INFO("Deleted the session(%p) of provisioned key(%p)",
            sessionPtr, keyPtr->keyFilePtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear the crypto session list of a provisioned key.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ClearCryptoSessionList
(
    taf_ks_Key_t*    keyPtr   ///< [IN] Key pointer
)
{
    taf_ks_CryptoSession_t* sessionPtr = NULL;

    if (keyPtr == NULL)
    {
        LE_ERROR("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    // Check if it's a provisioned key, only provisioned key
    // has the cryto session list.
    if ((keyPtr->newKeyPtr != NULL) || (keyPtr->keyFilePtr == NULL))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Go through the session list, free session object and remove the node.
    le_dls_Link_t* linkPtr = le_dls_Pop(&(keyPtr->cryptoSessionList));

    while (linkPtr)
    {
        sessionPtr = CONTAINER_OF(linkPtr, taf_ks_CryptoSession_t, link);
        LE_ASSERT(sessionPtr != NULL);

        LE_ASSERT(LE_OK == RemoveCryptoSession(sessionPtr));

        linkPtr = le_dls_Pop(&(keyPtr->cryptoSessionList));
    }

    // The list must be empty after clearing.
    LE_ASSERT(le_dls_NumLinks(&(keyPtr->cryptoSessionList)) == 0);
    LE_INFO("Cleared crypto session list for provisioned key(%p)",
            keyPtr->keyFilePtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove new keys which created by the specified client
 */
//--------------------------------------------------------------------------------------------------
static void RemoveNewKeysForClient
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] client session reference
    void*               contextPtr   ///< [IN]
)
{
    void* keyRef;
    taf_ks_Key_t* keyPtr;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(KeyRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        keyPtr = le_ref_GetValue(iterRef);
        LE_ASSERT(keyPtr != NULL);

        if ((keyPtr->keyFilePtr == NULL) &&
            (keyPtr->newKeyPtr != NULL) &&
            (keyPtr->newKeyPtr->clientSessionRef == sessionRef))
        {
            // Remove the key reference.
            keyRef = (void*)le_ref_GetSafeRef(iterRef);
            LE_ASSERT(keyRef != NULL);
            le_ref_DeleteRef(KeyRefMap, keyRef);

            // Clear the tag list and free the new key part.
            LE_ASSERT(LE_OK == ClearTagList(keyPtr));
            LE_INFO("Remove a new key(%p) of keyName '%s' for client(%p).",
                    keyPtr->newKeyPtr, keyPtr->newKeyPtr->keyName, sessionRef);
            le_mem_Release(keyPtr->newKeyPtr);

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
static void RemoveCryptoSessionsForClient
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] client session reference
    void*               contextPtr   ///< [IN]
)
{
    taf_ks_CryptoSession_t* cryptoSessionPtr;
    taf_ks_Key_t* keyPtr;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(CryptoSessionRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        cryptoSessionPtr = le_ref_GetValue(iterRef);
        LE_ASSERT(cryptoSessionPtr != NULL);

        if (cryptoSessionPtr->clientSessionRef == sessionRef)
        {
            // Remove the crypto session from the key's cryptoSession list.
            keyPtr = le_ref_Lookup(KeyRefMap, cryptoSessionPtr->keyRef);
            LE_ASSERT(keyPtr != NULL);
            le_dls_Remove(&(keyPtr->cryptoSessionList), &(cryptoSessionPtr->link));

            // Delete the crypto session.
            LE_ASSERT(LE_OK == RemoveCryptoSession(cryptoSessionPtr));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if there is running crypto session for a given key.
 */
//--------------------------------------------------------------------------------------------------
static bool HasRunningCryptoSession
(
    le_dls_List_t cryptoSessionList
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&cryptoSessionList);

    while (linkPtr != NULL)
    {
        taf_ks_CryptoSession_t* sessionPtr = CONTAINER_OF(linkPtr, taf_ks_CryptoSession_t, link);
        if (sessionPtr->started)
        {
            return true;
        }
        linkPtr = le_dls_PeekNext(&cryptoSessionList, linkPtr);
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Creates a new key.
 *
 * New keys initially have no value and cannot be used for any crypto operations. Call SetKey API to
 * set optional attributions for key access control if needed, then call Provision APIs to provision
 * the key value and save the key into the backend storage. The Crypto APIs are vailable to use only
 * after the key value is correctly provisioned.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CreateKey
(
    const char* keyName,
        ///< [IN] Key name
    taf_ks_KeyUsage_t keyUsage,
        ///< [IN] Key usage
    taf_ks_KeyRef_t* keyRefPtr
        ///< [OUT] Key reference
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef;
    le_msg_SessionRef_t clientSessionRef;
    taf_ks_Key_t* keyPtr;
    char appName[LE_LIMIT_APP_NAME_LEN + 1] = { 0 };
    pid_t pid;
    uid_t uid;

    if ((keyName == NULL) || (keyRefPtr == NULL) || (keyUsage >= TAF_KS_KEYUSAGE_MAX))
    {
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    // Check if a new key of the specified key name for the client is already created,
    // and just return the same reference if so.
    keyPtr = SearchNewKey(keyName);
    if (keyPtr != NULL)
    {
        LE_WARN("New key for keyName '%s' already exists.", keyName);
        return LE_NOT_PERMITTED;
    }

    // Check if a provisoned key of the specified key name for the client already exists.
    // We don't allow to create it again if so, thus we expect it return LE_NOT_FOUND.
    clientSessionRef = taf_ks_GetClientSessionRef();
    result = taf_pa_ks_GetKey(clientSessionRef, keyName, &keyFileRef);
    if (result == LE_OK)
    {
        LE_ASSERT(keyFileRef != NULL);
        LE_WARN("Provisioned key for keyName '%s' already exists.", keyName);
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
            LE_INFO("An obselete provisioned key(%p) deleted.", keyPtr->keyFilePtr);

            LE_ASSERT(LE_OK == ClearCryptoSessionList(keyPtr));
            le_ref_DeleteRef(KeyRefMap, keyPtr->keyRef);
            le_mem_Release(keyPtr);
        }
    }

    // Get the application name of the client if found.
    if((LE_OK == le_msg_GetClientUserCreds(clientSessionRef, &uid, &pid)) &&
       (LE_OK == le_appInfo_GetName(pid, appName, sizeof(appName)-1)))
    {
        LE_INFO("Create a new key of keyName '%s' for application '%s'.", keyName, appName);
    }

    // Create a new key.
    // Note the new key part will be freed after the key is provisioned. Then the key
    // becomes a provisioned key.
    keyPtr = le_mem_ForceAlloc(KeyPool);
    memset(keyPtr, 0, sizeof(taf_ks_Key_t));
    keyPtr->keyRef = le_ref_CreateRef(KeyRefMap, keyPtr); //Save the key reference.
    keyPtr->keyFilePtr = NULL; // Not provisioned yet.
    keyPtr->cryptoSessionList = LE_DLS_LIST_INIT;

    keyPtr->newKeyPtr = le_mem_ForceAlloc(NewKeyPool);
    memset(keyPtr->newKeyPtr, 0, sizeof(taf_ks_NewKey_t));
    keyPtr->newKeyPtr->tagList = LE_DLS_LIST_INIT;
    keyPtr->newKeyPtr->clientSessionRef = clientSessionRef;// Save client session.
    keyPtr->newKeyPtr->keyUsage = keyUsage;  // Save key usage.
    LE_ASSERT(LE_OK == le_utf8_Copy(keyPtr->newKeyPtr->keyName, keyName,
                                    sizeof(keyPtr->newKeyPtr->keyName), NULL));// Save the key name.

    // Return the key reference of the new key.
    *keyRefPtr = keyPtr->keyRef;
    LE_INFO("a new key(%p) created.", keyPtr->newKeyPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get key reference to a provisioned key by key name.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_GetKey
(
    const char* keyName,
        ///< [IN] Key name
    taf_ks_KeyRef_t* keyRefPtr
        ///< [OUT] Key reference
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef;
    le_msg_SessionRef_t clientSessionRef;
    taf_ks_Key_t* keyPtr;
    char appName[LE_LIMIT_APP_NAME_LEN + 1] = { 0 };
    pid_t pid;
    uid_t uid;

    if ((keyName == NULL) || (keyRefPtr == NULL))
    {
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    // Get the key file reference from the PA interface.
    clientSessionRef = taf_ks_GetClientSessionRef();
    result = taf_pa_ks_GetKey(clientSessionRef, keyName, &keyFileRef);
    if (result == LE_OK)
    {
        LE_ASSERT(keyFileRef != NULL);

        // Search if a key reference to the key is already created.
        keyPtr = SearchProvisionedKey(keyFileRef);
        if(keyPtr != NULL)
        {
            *keyRefPtr = keyPtr->keyRef;
            return LE_OK;
        }

        if((LE_OK == le_msg_GetClientUserCreds(clientSessionRef, &uid, &pid)) &&
           (LE_OK == le_appInfo_GetName(pid, appName, sizeof(appName)-1)))
        {
            LE_INFO("Create a provisioned key of keyName '%s' for application '%s'.",
                    keyName, appName);
        }

        // Create a provisioned key.
        keyPtr = le_mem_ForceAlloc(KeyPool);
        memset(keyPtr, 0, sizeof(taf_ks_Key_t));

        keyPtr->newKeyPtr = NULL;
        keyPtr->keyFilePtr = keyFileRef; // Save the key file reference.
        keyPtr->cryptoSessionList = LE_DLS_LIST_INIT;
        keyPtr->keyRef = le_ref_CreateRef(KeyRefMap, keyPtr); //Save the key reference.

        // Return the key reference of the provisioned key.
        *keyRefPtr = keyPtr->keyRef;
        LE_INFO("a privisoned key(%p) created.", keyPtr->keyFilePtr);
    }
    else if ((result == LE_NOT_FOUND) && (keyFileRef != NULL))
    {
        // An obselete key file was deleted in PA layer so we need delete it in
        // Service layer if exists.
        taf_ks_Key_t* keyPtr = SearchProvisionedKey(keyFileRef);
        if (keyPtr != NULL)
        {
            LE_INFO("An obselete provisioned key(%p) deleted.", keyPtr->keyFilePtr);

            LE_ASSERT(LE_OK == ClearCryptoSessionList(keyPtr));
            le_ref_DeleteRef(KeyRefMap, keyPtr->keyRef);
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
le_result_t taf_ks_DeleteKey
(
    taf_ks_KeyRef_t keyRef
        ///< [IN] Key reference
)
{
    taf_ks_Key_t* keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Key is not allowed to be deleted if it has running crypto session.
    if (HasRunningCryptoSession(keyPtr->cryptoSessionList))
    {
        LE_WARN("The key has running crypto session.");
        return LE_NOT_PERMITTED;
    }

    if (keyPtr->keyFilePtr != NULL)
    {
        // Delete a provisioned key
        le_result_t result = taf_pa_ks_DeleteKey(taf_ks_GetClientSessionRef(), keyPtr->keyFilePtr);
        if (result != LE_OK)
        {
            LE_ERROR("Failed to delete provisioned key(%p) (%s).",
                     keyPtr->keyFilePtr, LE_RESULT_TXT(result));
            return result;
        }

        LE_ASSERT(LE_OK == ClearCryptoSessionList(keyPtr));
        LE_INFO("a provisioned key(%p) deleted.", keyPtr->keyFilePtr);
    }
    else
    {
        // Delete a new key
        if(keyPtr->newKeyPtr != NULL)
        {
            // Clear the tag list and free the new key part.
            LE_ASSERT(LE_OK == ClearTagList(keyPtr));
            LE_INFO("a new key(%p) of keyName '%s' deleted.",
                    keyPtr->newKeyPtr, keyPtr->newKeyPtr->keyName);
            le_mem_Release(keyPtr->newKeyPtr);
        }
    }

    // Delete the key reference and free the key object.
    le_ref_DeleteRef(KeyRefMap, keyPtr->keyRef);
    le_mem_Release(keyPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get key usage of a key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_GetKeyUsage
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    taf_ks_KeyUsage_t* keyUsagePtr
        ///< [OUT] Key usage
)
{
    if (keyUsagePtr == NULL)
    {
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    taf_ks_Key_t* keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // This is a new key, get the keyUsage from the key object.
    if (keyPtr->newKeyPtr != NULL)
    {
        if (keyPtr->newKeyPtr->clientSessionRef != taf_ks_GetClientSessionRef())
        {
            LE_ERROR("Invalid client session.");
            return LE_NOT_PERMITTED;
        }

        // Return key usage of a new key.
        *keyUsagePtr = keyPtr->newKeyPtr->keyUsage;
        return LE_OK;
    }

    // This is a provisioned key, return key usage from the PA.
    return taf_pa_ks_GetKeyUsage(taf_ks_GetClientSessionRef(), keyPtr->keyFilePtr, keyUsagePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of times that a key may be used between system reboots.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_SetKeyMaxUsesPerBoot
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    uint32_t value
        ///< [IN] Uses per boot
)
{
    taf_ks_Key_t* keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key, only new key is allowed to set the tag.
    if ((keyPtr->newKeyPtr == NULL) || (keyPtr->keyFilePtr != NULL))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    taf_pa_ks_Tag_t* newTagPtr = le_mem_ForceAlloc(TagPool);
    memset(newTagPtr, 0, sizeof(taf_pa_ks_Tag_t));

    newTagPtr->id = TAF_PA_KS_TAG_MAX_USES_PER_BOOT;
    newTagPtr->maxUsesPerBoot = value;
    LE_ASSERT(LE_OK == SetTag(keyPtr, newTagPtr));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the minimum amount of time that elapses between allowed operations using a key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_SetKeyMinSecondsBetweenOps
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    uint32_t value
        ///< [IN] Seconds interval between allowed operations.
)
{
    taf_ks_Key_t* keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key, only new key is allowed to set the tag.
    if ((keyPtr->newKeyPtr == NULL) || (keyPtr->keyFilePtr != NULL))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    taf_pa_ks_Tag_t* newTagPtr = le_mem_ForceAlloc(TagPool);
    memset(newTagPtr, 0, sizeof(taf_pa_ks_Tag_t));

    newTagPtr->id = TAF_PA_KS_TAG_MIN_SECONDS_BETWEEN_OPS;
    newTagPtr->minSecondsBetweenOps = value;
    LE_ASSERT(LE_OK == SetTag(keyPtr, newTagPtr));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set application data to the key.
 *
 * When the attribute is provided to the key, the same data must be also provided to crypto API.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_SetKeyAppData
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
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    taf_ks_Key_t* keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key, only new key is allowed to set the tag.
    if ((keyPtr->newKeyPtr == NULL) || (keyPtr->keyFilePtr != NULL))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    taf_pa_ks_Tag_t* newTagPtr = le_mem_ForceAlloc(TagPool);
    memset(newTagPtr, 0, sizeof(taf_pa_ks_Tag_t));

    newTagPtr->id = TAF_PA_KS_TAG_APPLICATION_DATA;
    newTagPtr->appDataPtr = le_mem_ForceAlloc(DataPool);
    memcpy(newTagPtr->appDataPtr->data, dataPtr, dataSize);
    newTagPtr->appDataPtr->size = dataSize;
    LE_ASSERT(LE_OK == SetTag(keyPtr, newTagPtr));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the date and time at which the key becomes active. Prior to this time any attempt to use the
 * key will get failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_SetKeyActiveDateTime
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    uint64_t value
        ///< [IN] Milliseconds since January 1, 1970.
)
{
    taf_ks_Key_t* keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key, only new key is allowed to set the tag.
    if ((keyPtr->newKeyPtr == NULL) || (keyPtr->keyFilePtr != NULL))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    taf_pa_ks_Tag_t* newTagPtr = le_mem_ForceAlloc(TagPool);
    memset(newTagPtr, 0, sizeof(taf_pa_ks_Tag_t));

    newTagPtr->id = TAF_PA_KS_TAG_ACTIVE_DATETIME;
    newTagPtr->activeDateTime = value;
    LE_ASSERT(LE_OK == SetTag(keyPtr, newTagPtr));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the date and time at which the key expires for signing and encryption. After this time any
 * attempt to use a key for signing or encryption will get failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_SetKeyOriginationExpireDateTime
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    uint64_t value
        ///< [IN] Milliseconds since January 1, 1970.
)
{
    taf_ks_Key_t* keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key, only new key is allowed to set the tag.
    if ((keyPtr->newKeyPtr == NULL) || (keyPtr->keyFilePtr != NULL))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    taf_pa_ks_Tag_t* newTagPtr = le_mem_ForceAlloc(TagPool);
    memset(newTagPtr, 0, sizeof(taf_pa_ks_Tag_t));

    newTagPtr->id = TAF_PA_KS_TAG_ORIGINATION_EXPIRE_DATETIME;
    newTagPtr->originationExpireDateTime = value;
    LE_ASSERT(LE_OK == SetTag(keyPtr, newTagPtr));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the date and time at which the key expires for verification and decryption. After this time
 * any attempt to use a key for verification and decryption will get failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_SetKeyUsageExpireDateTime
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    uint64_t value
        ///< [IN] Milliseconds since January 1, 1970.
)
{
    taf_ks_Key_t* keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key, only new key is allowed to set the tag.
    if ((keyPtr->newKeyPtr == NULL) || (keyPtr->keyFilePtr != NULL))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    taf_pa_ks_Tag_t* newTagPtr = le_mem_ForceAlloc(TagPool);
    memset(newTagPtr, 0, sizeof(taf_pa_ks_Tag_t));

    newTagPtr->id = TAF_PA_KS_TAG_USAGE_EXPIRE_DATETIME;
    newTagPtr->usageExpireDateTime = value;
    LE_ASSERT(LE_OK == SetTag(keyPtr, newTagPtr));

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
le_result_t taf_ks_ProvisionRsaEncKeyValue
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    taf_ks_RsaKeySize_t keySize,
        ///< [IN] Key Size, ignored if impData is provided
    taf_ks_RsaEncPadding_t padding,
        ///< [IN] RSA encryption padding type
    const uint8_t* impDataPtr,
        ///< [IN] Imported key data
    size_t impDataSize
        ///< [IN]
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef;
    taf_pa_ks_EncPurpose_t keyUsage;
    taf_ks_Key_t* keyPtr;

    if ((keySize >= TAF_KS_RSA_SIZE_MAX) || (padding >= TAF_KS_RSA_ENC_PAD_MAX))
    {
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key since only new key can be provisioned.
    // Also check if the key is suitable for this key value provision.
    if ((keyPtr->keyFilePtr != NULL) ||
        (keyPtr->newKeyPtr == NULL) ||
        (keyPtr->newKeyPtr->clientSessionRef != taf_ks_GetClientSessionRef()) ||
        ((keyPtr->newKeyPtr->keyUsage != TAF_KS_RSA_ENCRYPT_DECRYPT) &&
         (keyPtr->newKeyPtr->keyUsage != TAF_KS_RSA_ENCRYPT_ONLY) &&
         (keyPtr->newKeyPtr->keyUsage != TAF_KS_RSA_DECRYPT_ONLY)))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Provision the RSA encryption key.
    keyUsage = (keyPtr->newKeyPtr->keyUsage == TAF_KS_RSA_ENCRYPT_DECRYPT) ?
        TAF_PA_KS_ENCRYPT_DECRYPT :
        ((keyPtr->newKeyPtr->keyUsage == TAF_KS_RSA_ENCRYPT_ONLY) ?
        TAF_PA_KS_ENCRYPT_ONLY : TAF_PA_KS_DECRYPT_ONLY);

    result = taf_pa_ks_GenerateRsaEncKey(taf_ks_GetClientSessionRef(),
                                         keyPtr->newKeyPtr->keyName,
                                         keySize,
                                         keyUsage,
                                         padding,
                                         &(keyPtr->newKeyPtr->tagList),
                                         impDataPtr,
                                         impDataSize,
                                         &keyFileRef);
    if (result == LE_OK)
    {
        // Clear the tag list after successful provisioned.
        LE_ASSERT(LE_OK == ClearTagList(keyPtr));

        // Free the new key part.
        le_mem_Release(keyPtr->newKeyPtr);
        keyPtr->newKeyPtr = NULL;

        // set key provisioned.
        keyPtr->keyFilePtr = keyFileRef;
        keyPtr->cryptoSessionList = LE_DLS_LIST_INIT;
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
le_result_t taf_ks_ProvisionRsaSigKeyValue
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    taf_ks_RsaKeySize_t keySize,
        ///< [IN] Key Size, ignored if impData is provided
    taf_ks_RsaSigPadding_t padding,
        ///< [IN] RSA signature padding type
    const uint8_t* impDataPtr,
        ///< [IN] Imported key data
    size_t impDataSize
        ///< [IN]
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef;
    taf_pa_ks_SigPurpose_t keyUsage;
    taf_ks_Key_t* keyPtr;

    if ((keySize >= TAF_KS_RSA_SIZE_MAX) || (padding >= TAF_KS_RSA_SIG_PAD_MAX))
    {
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key since only new key can be provisioned.
    // Also check if the key is suitable for this key value provision.
    if ((keyPtr->keyFilePtr != NULL) ||
        (keyPtr->newKeyPtr == NULL) ||
        (keyPtr->newKeyPtr->clientSessionRef != taf_ks_GetClientSessionRef()) ||
        ((keyPtr->newKeyPtr->keyUsage != TAF_KS_RSA_SIGN_VERIFY) &&
         (keyPtr->newKeyPtr->keyUsage != TAF_KS_RSA_SIGN_ONLY) &&
         (keyPtr->newKeyPtr->keyUsage != TAF_KS_RSA_VERIFY_ONLY)))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Provision the RSA signing key.
    keyUsage = (keyPtr->newKeyPtr->keyUsage == TAF_KS_RSA_SIGN_VERIFY) ?
        TAF_PA_KS_SIGN_VERIFY :
        ((keyPtr->newKeyPtr->keyUsage == TAF_KS_RSA_SIGN_ONLY) ?
        TAF_PA_KS_SIGN_ONLY : TAF_PA_KS_VERIFY_ONLY);

    result = taf_pa_ks_GenerateRsaSigKey(taf_ks_GetClientSessionRef(),
                                         keyPtr->newKeyPtr->keyName,
                                         keySize,
                                         keyUsage,
                                         padding,
                                         &(keyPtr->newKeyPtr->tagList),
                                         impDataPtr,
                                         impDataSize,
                                         &keyFileRef);
    if (result == LE_OK)
    {
        // Clear the tag list after successful provisioned.
        LE_ASSERT(LE_OK == ClearTagList(keyPtr));

        // Free the new key part.
        le_mem_Release(keyPtr->newKeyPtr);
        keyPtr->newKeyPtr = NULL;

        // set key provisioned.
        keyPtr->keyFilePtr = keyFileRef;
        keyPtr->cryptoSessionList = LE_DLS_LIST_INIT;
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
le_result_t taf_ks_ProvisionEcdsaKeyValue
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    taf_ks_EccKeySize_t keySize,
        ///< [IN] ECC curve, ignored if impData is provided
    taf_ks_Digest_t digest,
        ///< [IN] Digest
    const uint8_t* impDataPtr,
        ///< [IN] Imported key data
    size_t impDataSize
        ///< [IN]
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef;
    taf_pa_ks_SigPurpose_t keyUsage;
    taf_ks_Key_t* keyPtr;

    if ((keySize >= TAF_KS_ECC_SIZE_MAX) || (digest >= TAF_KS_DIGEST_MAX))
    {
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key since only new key can be provisioned.
    // Also check if the key is suitable for this key value provision.
    if ((keyPtr->keyFilePtr != NULL) ||
        (keyPtr->newKeyPtr == NULL) ||
        (keyPtr->newKeyPtr->clientSessionRef != taf_ks_GetClientSessionRef()) ||
        ((keyPtr->newKeyPtr->keyUsage != TAF_KS_ECDSA_SIGN_VERIFY) &&
         (keyPtr->newKeyPtr->keyUsage != TAF_KS_ECDSA_SIGN_ONLY) &&
         (keyPtr->newKeyPtr->keyUsage != TAF_KS_ECDSA_VERIFY_ONLY)))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Provision the ECDSA key.
    keyUsage = (keyPtr->newKeyPtr->keyUsage == TAF_KS_ECDSA_SIGN_VERIFY) ?
        TAF_PA_KS_SIGN_VERIFY :
        ((keyPtr->newKeyPtr->keyUsage == TAF_KS_ECDSA_SIGN_ONLY) ?
        TAF_PA_KS_SIGN_ONLY : TAF_PA_KS_VERIFY_ONLY);

    result = taf_pa_ks_GenerateEcdsaKey(taf_ks_GetClientSessionRef(),
                                        keyPtr->newKeyPtr->keyName,
                                        keySize,
                                        keyUsage,
                                        digest,
                                        &(keyPtr->newKeyPtr->tagList),
                                        impDataPtr,
                                        impDataSize,
                                        &keyFileRef);
    if (result == LE_OK)
    {
        // Clear the tag list after successful provisioned.
        LE_ASSERT(LE_OK == ClearTagList(keyPtr));

        // Free the new key part.
        le_mem_Release(keyPtr->newKeyPtr);
        keyPtr->newKeyPtr = NULL;

        // set key provisioned.
        keyPtr->keyFilePtr = keyFileRef;
        keyPtr->cryptoSessionList = LE_DLS_LIST_INIT;
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
le_result_t taf_ks_ProvisionAesKeyValue
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    taf_ks_AesKeySize_t keySize,
        ///< [IN] AES key size, ignored if impData is provided
    taf_ks_AesBlockMode_t mode,
        ///< [IN] AES block mode
    const uint8_t* impDataPtr,
        ///< [IN] Imported key data
    size_t impDataSize
        ///< [IN]
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef;
    taf_pa_ks_EncPurpose_t keyUsage;
    taf_ks_Key_t* keyPtr;

    if ((keySize >= TAF_KS_AES_SIZE_MAX) || (mode >= TAF_KS_AES_MODE_MAX))
    {
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key since only new key can be provisioned.
    // Also check if the key is suitable for this key value provision.
    if ((keyPtr->keyFilePtr != NULL) ||
        (keyPtr->newKeyPtr == NULL) ||
        (keyPtr->newKeyPtr->clientSessionRef != taf_ks_GetClientSessionRef()) ||
        ((keyPtr->newKeyPtr->keyUsage != TAF_KS_AES_ENCRYPT_DECRYPT) &&
         (keyPtr->newKeyPtr->keyUsage != TAF_KS_AES_ENCRYPT_ONLY) &&
         (keyPtr->newKeyPtr->keyUsage != TAF_KS_AES_DECRYPT_ONLY)))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Provision the AES key.
    keyUsage = (keyPtr->newKeyPtr->keyUsage == TAF_KS_AES_ENCRYPT_DECRYPT) ?
        TAF_PA_KS_ENCRYPT_DECRYPT :
        ((keyPtr->newKeyPtr->keyUsage == TAF_KS_AES_ENCRYPT_ONLY) ?
        TAF_PA_KS_ENCRYPT_ONLY : TAF_PA_KS_DECRYPT_ONLY);

    result = taf_pa_ks_GenerateAesKey(taf_ks_GetClientSessionRef(),
                                      keyPtr->newKeyPtr->keyName,
                                      keySize,
                                      keyUsage,
                                      mode,
                                      &(keyPtr->newKeyPtr->tagList),
                                      impDataPtr,
                                      impDataSize,
                                      &keyFileRef);
    if (result == LE_OK)
    {
        // Clear the tag list after successful provisioned.
        LE_ASSERT(LE_OK == ClearTagList(keyPtr));

        // Free the new key part.
        le_mem_Release(keyPtr->newKeyPtr);
        keyPtr->newKeyPtr = NULL;

        // set key provisioned.
        keyPtr->keyFilePtr = keyFileRef;
        keyPtr->cryptoSessionList = LE_DLS_LIST_INIT;
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
le_result_t taf_ks_ProvisionHmacKeyValue
(
    taf_ks_KeyRef_t keyRef,
        ///< [IN] Key reference
    uint32_t keySize,
        ///< [IN] HMAC Key Size, ignored if impData is provided
    taf_ks_Digest_t digest,
        ///< [IN] digest
    const uint8_t* impDataPtr,
        ///< [IN] Imported key data
    size_t impDataSize
        ///< [IN]
)
{
    le_result_t result;
    KeyMgt_KeyFileRef_t keyFileRef;
    taf_pa_ks_SigPurpose_t keyUsage;
    taf_ks_Key_t* keyPtr;

    if ((keySize < TAF_KS_MIN_HMAC_KEY_SIZE) ||
        (keySize > TAF_KS_MAX_HMAC_KEY_SIZE) ||
        (digest >= TAF_KS_DIGEST_MAX))
    {
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a new key since only new key can be provisioned.
    // Also check if the key is suitable for this key value provision.
    if ((keyPtr->keyFilePtr != NULL) ||
        (keyPtr->newKeyPtr == NULL) ||
        (keyPtr->newKeyPtr->clientSessionRef != taf_ks_GetClientSessionRef()) ||
        ((keyPtr->newKeyPtr->keyUsage != TAF_KS_HMAC_SIGN_VERIFY) &&
         (keyPtr->newKeyPtr->keyUsage != TAF_KS_HMAC_SIGN_ONLY) &&
         (keyPtr->newKeyPtr->keyUsage != TAF_KS_HMAC_VERIFY_ONLY)))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Provision the HMAC key.
    keyUsage = (keyPtr->newKeyPtr->keyUsage == TAF_KS_HMAC_SIGN_VERIFY) ?
        TAF_PA_KS_SIGN_VERIFY :
        ((keyPtr->newKeyPtr->keyUsage == TAF_KS_HMAC_SIGN_ONLY) ?
        TAF_PA_KS_SIGN_ONLY : TAF_PA_KS_VERIFY_ONLY);

    result = taf_pa_ks_GenerateHmacKey(taf_ks_GetClientSessionRef(),
                                       keyPtr->newKeyPtr->keyName,
                                       keySize,
                                       keyUsage,
                                       digest,
                                       &(keyPtr->newKeyPtr->tagList),
                                       impDataPtr,
                                       impDataSize,
                                       &keyFileRef);
    if (result == LE_OK)
    {
        // Clear the tag list after successful provisioned.
        LE_ASSERT(LE_OK == ClearTagList(keyPtr));

        // Free the new key part.
        le_mem_Release(keyPtr->newKeyPtr);
        keyPtr->newKeyPtr = NULL;

        // set key provisioned.
        keyPtr->keyFilePtr = keyFileRef;
        keyPtr->cryptoSessionList = LE_DLS_LIST_INIT;
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
le_result_t taf_ks_ExportKey
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
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    taf_ks_Key_t* keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed to export the key.
    if ((keyPtr->newKeyPtr != NULL) || (keyPtr->keyFilePtr == NULL))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Export the key data.
    return taf_pa_ks_ExportKey(taf_ks_GetClientSessionRef(),
                               keyPtr->keyFilePtr,
                               appDataPtr,
                               appDataSize,
                               expDataPtr,
                               expDataSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a crypto operation session for the key.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionCreate
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
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("Key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only the session of provisioned key
    // is allowed to create crypto sessions.
    if ((keyPtr->newKeyPtr != NULL) || (keyPtr->keyFilePtr == NULL))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Create a new crypto session for the key.
    sessionPtr = le_mem_ForceAlloc(CryptoSessionPool);
    memset(sessionPtr, 0, sizeof(taf_ks_CryptoSession_t));
    sessionPtr->clientSessionRef = taf_ks_GetClientSessionRef();
    sessionPtr->started = false;
    sessionPtr->handle = 0;
    sessionPtr->paramList = LE_DLS_LIST_INIT;
    sessionPtr->link = LE_DLS_LINK_INIT;
    sessionPtr->keyRef = keyPtr->keyRef;
    sessionPtr->cryptoSessionRef = le_ref_CreateRef(CryptoSessionRefMap, sessionPtr);

    // Add the session into the session list of the provisioned key.
    le_dls_Queue(&(keyPtr->cryptoSessionList), &(sessionPtr->link));
    *sessionRefPtr = sessionPtr->cryptoSessionRef;

    LE_INFO("A new crypto session(%p) for provisioned key(%p) created.",
            sessionPtr, keyPtr->keyFilePtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the nonce or IVs for AES GCM, CBC, CTR for a crypto session. For AES GCM the nonce size must
 * be 12 byte, for AES CBC, CTR the IV must be 16 byte.
 *
 * This API must be called before CryptoSessionStart API if needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionSetAesNonce
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
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    sessionPtr = le_ref_Lookup(CryptoSessionRefMap, sessionRef);
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, sessionPtr->keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed
    if ((keyPtr->newKeyPtr != NULL) || (keyPtr->keyFilePtr == NULL) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    taf_pa_ks_Param_t* newParamPtr = le_mem_ForceAlloc(ParamPool);
    memset(newParamPtr, 0, sizeof(taf_pa_ks_Param_t));

    newParamPtr->id = TAF_PA_KS_PARAM_NONCE;
    newParamPtr->nonceDataPtr = le_mem_ForceAlloc(AesNoncePool);
    memset(newParamPtr->nonceDataPtr, 0, sizeof(taf_pa_ks_Nonce_t));

    memcpy(newParamPtr->nonceDataPtr->data, dataPtr, dataSize);
    newParamPtr->nonceDataPtr->size = dataSize;
    LE_ASSERT(LE_OK == SetParam(sessionPtr, newParamPtr));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Application data for a crypto session.
 *
 * This API must be called before CryptoSessionStart API if need.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionSetAppData
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
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    sessionPtr = le_ref_Lookup(CryptoSessionRefMap, sessionRef);
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, sessionPtr->keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed
    // to set the session parameter.
    if ((keyPtr->newKeyPtr != NULL) || (keyPtr->keyFilePtr == NULL) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    taf_pa_ks_Param_t* newParamPtr = le_mem_ForceAlloc(ParamPool);
    memset(newParamPtr, 0, sizeof(taf_pa_ks_Param_t));

    newParamPtr->id = TAF_PA_KS_PARAM_APPLICATION_DATA;
    newParamPtr->appDataPtr = le_mem_ForceAlloc(DataPool);
    memset(newParamPtr->appDataPtr, 0, sizeof(taf_pa_ks_Data_t));

    memcpy(newParamPtr->appDataPtr->data, dataPtr, dataSize);
    newParamPtr->appDataPtr->size = dataSize;
    LE_ASSERT(LE_OK == SetParam(sessionPtr, newParamPtr));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start the crypto session for the given crypto operation.
 *
 * The crypto session will be automatically deleted if error is returned.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionStart
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
    uint64_t handle;

    if (cryptoPurpose >= TAF_KS_CRYPTO_MAX)
    {
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    sessionPtr = le_ref_Lookup(CryptoSessionRefMap, sessionRef);
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, sessionPtr->keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed
    if ((keyPtr->newKeyPtr != NULL) || (keyPtr->keyFilePtr == NULL) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Check if the session is already started.
    if (sessionPtr->started)
    {
        LE_WARN("Session(%p) for provisioned key(%p) is already started.",
                sessionPtr, keyPtr->keyFilePtr);
        return LE_DUPLICATE;
    }

    result = taf_pa_ks_CryptoSessionStart(taf_ks_GetClientSessionRef(),
                                          keyPtr->keyFilePtr,
                                          cryptoPurpose,
                                          &(sessionPtr->paramList),
                                          &handle);
    if (result == LE_OK)
    {
        // Save the handle and set the session as started.
        sessionPtr->started = true;
        sessionPtr->handle = handle;
        LE_ASSERT(LE_OK == ClearParamList(sessionPtr));
    }
    else
    {
        // Remove the session if any error happens.
        le_dls_Remove(&(keyPtr->cryptoSessionList), &(sessionPtr->link));
        LE_ASSERT(LE_OK == RemoveCryptoSession(sessionPtr));
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Provides AES AEAD to the running crypto session started with CryptoSessionStart API for AES GCM
 * mode.This API can be called for multiple times but must before CryptoSessionProcess API.
 *
 * The crypto session will be automatically deleted if error is returned.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionProcessAead
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
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    sessionPtr = le_ref_Lookup(CryptoSessionRefMap, sessionRef);
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, sessionPtr->keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed
    if ((keyPtr->newKeyPtr != NULL) || (keyPtr->keyFilePtr == NULL) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Check if the session is started.
    if (sessionPtr->started == false)
    {
        LE_WARN("Session(%p) for provisioned key(%p) is not started.",
                sessionPtr, keyPtr->keyFilePtr);
        return LE_NOT_PERMITTED;
    }

    result = taf_pa_ks_CryptoSessionProcessAead(sessionPtr->handle,
                                                inputDataPtr,
                                                inputDataSize);
    if (result != LE_OK)
    {
        // Process failed means the session is already aborted.
        sessionPtr->started = false;
        sessionPtr->handle = 0;

        le_dls_Remove(&(keyPtr->cryptoSessionList), &(sessionPtr->link));
        LE_ASSERT(LE_OK == RemoveCryptoSession(sessionPtr));
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Provides data to, and possibly receives output from, an runing crypto session started with
 * CryptoSessionStart API. It can be called for multiple times before CryptoSessionEnd API is
 * called.
 *
 * The crypto session will be automatically deleted if error is returned.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionProcess
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
        LE_KILL_CLIENT("Bad parameter.");
        return LE_BAD_PARAMETER;
    }

    sessionPtr = le_ref_Lookup(CryptoSessionRefMap, sessionRef);
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, sessionPtr->keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed
    if ((keyPtr->newKeyPtr != NULL) || (keyPtr->keyFilePtr == NULL) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Check if the session is started.
    if (sessionPtr->started == false)
    {
        LE_WARN("Session(%p) for provisioned key(%p) is not started.",
                sessionPtr, keyPtr->keyFilePtr);
        return LE_NOT_PERMITTED;
    }

    result = taf_pa_ks_CryptoSessionProcess(sessionPtr->handle,
                                            inputDataPtr,
                                            inputDataSize,
                                            outputDataPtr,
                                            outputDataSizePtr);
    if (result != LE_OK)
    {
        // Process failed means the session is already aborted.
        sessionPtr->started = false;
        sessionPtr->handle = 0;

        le_dls_Remove(&(keyPtr->cryptoSessionList), &(sessionPtr->link));
        LE_ASSERT(LE_OK == RemoveCryptoSession(sessionPtr));
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
le_result_t taf_ks_CryptoSessionEnd
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

    sessionPtr = le_ref_Lookup(CryptoSessionRefMap, sessionRef);
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, sessionPtr->keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed
    if ((keyPtr->newKeyPtr != NULL) || (keyPtr->keyFilePtr == NULL) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Check if the session is started.
    if (sessionPtr->started == false)
    {
        LE_WARN("Session(%p) for provisioned key(%p) is not started.",
                sessionPtr, keyPtr->keyFilePtr);
        return LE_NOT_PERMITTED;
    }

    result = taf_pa_ks_CryptoSessionEnd(sessionPtr->handle,
                                        inputDataPtr,
                                        inputDataSize,
                                        outputDataPtr,
                                        outputDataSizePtr);
    // Set the session is stopped.
    sessionPtr->started = false;
    sessionPtr->handle = 0;

    // Delete the crypto session.
    le_dls_Remove(&(keyPtr->cryptoSessionList), &(sessionPtr->link));
    LE_ASSERT(LE_OK == RemoveCryptoSession(sessionPtr));

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Abort the crypto session started with CryptoSessionStart API.
 *
 * The crypto session will be then automatically deleted.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_ks_CryptoSessionAbort
(
    taf_ks_CryptoSessionRef_t sessionRef
        ///< [IN] Session reference
)
{
    le_result_t result = LE_OK;
    taf_ks_CryptoSession_t* sessionPtr = NULL;
    taf_ks_Key_t* keyPtr = NULL;

    sessionPtr = le_ref_Lookup(CryptoSessionRefMap, sessionRef);
    if (sessionPtr == NULL)
    {
        LE_ERROR("Session is not found.");
        return LE_NOT_FOUND;
    }

    keyPtr = le_ref_Lookup(KeyRefMap, sessionPtr->keyRef);
    if (keyPtr == NULL)
    {
        LE_ERROR("key is not found.");
        return LE_NOT_FOUND;
    }

    // Check if it's a provisioned key, only provisioned key is allowed.
    if ((keyPtr->newKeyPtr != NULL) || (keyPtr->keyFilePtr == NULL) ||
        (sessionPtr->clientSessionRef != taf_ks_GetClientSessionRef()))
    {
        LE_ERROR("Not permitted.");
        return LE_NOT_PERMITTED;
    }

    // Check if the session is started.
    if (sessionPtr->started == false)
    {
        LE_WARN("Session(%p) for provisioned key(%p) is not started.",
                sessionPtr, keyPtr->keyFilePtr);
        return LE_NOT_PERMITTED;
    }

    // Abort the session.
    result = taf_pa_ks_CryptoSessionAbort(sessionPtr->handle);
    sessionPtr->started = false;
    sessionPtr->handle = 0;

    // Delete the crypto session.
    le_dls_Remove(&(keyPtr->cryptoSessionList), &(sessionPtr->link));
    LE_ASSERT(LE_OK == RemoveCryptoSession(sessionPtr));

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * The keyStore daemon's initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Create memory pools
    CryptoSessionPool = le_mem_CreatePool("CryptoSessionPool", sizeof(taf_ks_CryptoSession_t));
    NewKeyPool = le_mem_CreatePool("NewKeyPool", sizeof(taf_ks_NewKey_t));
    KeyPool = le_mem_CreatePool("KeyPool", sizeof(taf_ks_Key_t));

    TagPool = le_mem_CreatePool("TagPool", sizeof(taf_pa_ks_Tag_t));
    ParamPool = le_mem_CreatePool("ParamPool", sizeof(taf_pa_ks_Param_t));

    AesNoncePool = le_mem_CreatePool("AesNoncePool", sizeof(taf_pa_ks_Nonce_t));
    DataPool = le_mem_CreatePool("DataPool", sizeof(taf_pa_ks_Data_t));

    // Create reference maps
    CryptoSessionRefMap = le_ref_CreateMap("CryptoSessionRefMap", 20);
    KeyRefMap = le_ref_CreateMap("KeyRefMap", 500);

    // Set session close handlers
    le_msg_AddServiceCloseHandler(taf_ks_GetServiceRef(), RemoveNewKeysForClient, NULL);
    le_msg_AddServiceCloseHandler(taf_ks_GetServiceRef(), RemoveCryptoSessionsForClient, NULL);

    if (LE_OK == taf_pa_ks_Init())
    {
        LE_INFO("Telaf keyStore Service initialized.");
    }
    else
    {
        LE_FATAL("taf_pa_ks_Init() failed.");
    }
}
