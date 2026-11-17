/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFKEYSTORE_HPP
#define TAFKEYSTORE_HPP

#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "tafSvcIF.hpp"
#include <errno.h>

extern "C" {
#include "tafKeystorePa.h"
}

#include "tafPaResultConvert.hpp"

//--------------------------------------------------------------------------------------------------
// Enum for key type.
//--------------------------------------------------------------------------------------------------
typedef enum
{
    KS_NEW_CREATED_KEY,  ///< New created key.
    KS_PROVISIONED_KEY,  ///< Provisioned key.
}
KeyType_t;

//--------------------------------------------------------------------------------------------------
// Enum for event operation.
//--------------------------------------------------------------------------------------------------
typedef enum
{
    KS_OP_CREATE,        ///< Create a key.
    KS_OP_SHARE,         ///< Share a key.
}
KeyOp_t;

//--------------------------------------------------------------------------------------------------
/**
 * Keystore Tag struct
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;        ///< Link to the tag list of the key.
    taf_pa_ks_Tag_t tag;       ///< KS tag.
}
KeyTag_t;

//--------------------------------------------------------------------------------------------------
/**
 * Keystore Param struct
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;        ///< Link to the param list of the key.
    taf_pa_ks_Param_t param;   ///< KS param.
}
KeyParam_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data struct for shared app object
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_pa_ks_SharedApp_t appInfo; ///< Share app object
    le_sls_Link_t link;            ///< Link to the list.
}
SharedApp_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data struct for shared app list for a client.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_ks_KeyRef_t keyRef;                       ///< Reference to the key
    le_msg_SessionRef_t clientSessionRef;         ///< Client session reference
    le_dls_Link_t link;                           ///< link to key's shareApp list
    le_sls_List_t appList;                        ///< ShareApp list for this client session
    le_sls_Link_t* currPtr;                       ///< Position for current shared appName.
}
SharedAppList_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data struct for key sharing state change event. The event could be triggered once a key is shared
 * or a key sharing event hander is registered.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_ks_SharingState_t state;                   ///< Key sharing state
    char keyId[TAF_KS_MAX_KEY_ID_SIZE + 1];        ///< Key ID
    char ownerAppName[TAF_KS_MAX_APP_NAME_SIZE + 1]; ///< Owner app name
    char sharedAppName[TAF_KS_MAX_APP_NAME_SIZE + 1];///< Shared app name
}
KeySharing_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data struct for generic key event in service layer.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    KeyOp_t op;                                    ///< Operation type.
    KeyMgt_KeyFileRef_t keyFileRef;                ///< Key file reference from PA layer
    KeySharing_t shareInfo;                        ///< Sharing info is needed if op=KS_OP_SHARE
}
KeyEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data struct for new created key.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char keyId[TAF_KS_MAX_KEY_ID_SIZE + 1];///< Key ID string
    le_msg_SessionRef_t clientSessionRef;  ///< Client session reference
    taf_ks_KeyUsage_t keyUsage;            ///< Key usage
    le_dls_List_t tagList;                 ///< Tag list set to the new key
}
NewKey_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data struct for provisioned key.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    KeyMgt_KeyFileRef_t keyFileRef;        ///< Key file reference from PA layer
    le_dls_List_t       cryptoSessionList; ///< Crypto session list
    le_dls_List_t       sharedAppList;     ///< Shared app list
}
ProKey_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data struct for generic key.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_ks_KeyRef_t     keyRef;            ///< Reference to the key
    KeyType_t           keyType;           ///< Key type
    union
    {
        NewKey_t newKey;                   ///< New key object
        ProKey_t proKey;                   ///< Provisioned key object
    };
}
taf_ks_Key_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data struct for crypto session.
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
 * Data struct for key sharing handler.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
   le_msg_SessionRef_t clientSessionRef;             ///< Client session reference
   char keyId[TAF_KS_MAX_KEY_ID_SIZE + 1];           ///< Key ID string
   char ownerAppName[TAF_KS_MAX_APP_NAME_SIZE + 1];  ///< Key owner application
   taf_ks_SharingState_t state;                      ///< Sharing state
   taf_ks_KeySharingHandlerFunc_t handleFunc;        ///< Handler function
   void* context;                                    ///< Handler context
   taf_ks_KeySharingHandlerRef_t handlerRef;         ///< Handler reference
}
taf_ks_Handler_t;

//--------------------------------------------------------------------------------------------------
/**
 * Parms list passed to PA layer.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_PARAM_NUMBER 32

//--------------------------------------------------------------------------------------------------
/**
 * Tags list passed to PA layer.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_TAG_NUMBER 32

//--------------------------------------------------------------------------------------------------
/**
 * TelAF KeyStore Service Class
 */
//--------------------------------------------------------------------------------------------------
namespace tafsvc
{
    class taf_KeyStore : public ITafSvc
    {
        public:
            taf_KeyStore() {};
            ~taf_KeyStore() {};
            void Init();

            // Static member functions.
            static taf_KeyStore &GetInstance();
            static void KeyCreationPAHandler(KeyMgt_KeyFileRef_t keyFileRef);
            static void KeySharingPAHandler(const char* keyIdPtr,
                                                 const char* ownerAppNamePtr,
                                                 const char* sharedAppNamePtr,
                                                 taf_pa_ks_SharingState_t state,
                                                 KeyMgt_KeyFileRef_t keyFileRef);
            static void KeyEventHandler(void* reportPtr);
            static void RemoveNewKeysForClient(le_msg_SessionRef_t sessionRef, void* contextPtr);
            static void RemoveCryptoSessionsForClient(le_msg_SessionRef_t sessionRef,
                                                            void* contextPtr);
            static void RemoveAppListsForClient(le_msg_SessionRef_t sessionRef, void* contextPtr);
            static void InitTimerHandler(le_timer_Ref_t timerRef);

            // Public methods (correspond to the taf_ks IPC API entries).
            le_result_t CreateKey(const char* keyId, taf_ks_KeyUsage_t keyUsage,
                                       taf_ks_KeyRef_t* keyRefPtr);
            le_result_t GetKey(const char* keyName, taf_ks_KeyRef_t* keyRefPtr);
            le_result_t DeleteKey(taf_ks_KeyRef_t keyRef);
            le_result_t GetKeyUsage(taf_ks_KeyRef_t keyRef, taf_ks_KeyUsage_t* keyUsagePtr);
            le_result_t SetKeyMaxUsesPerBoot(taf_ks_KeyRef_t keyRef, uint32_t value);
            le_result_t SetKeyMinSecondsBetweenOps(taf_ks_KeyRef_t keyRef, uint32_t value);
            le_result_t SetKeyAppData(taf_ks_KeyRef_t keyRef, const uint8_t* dataPtr,
                                           size_t dataSize);
            le_result_t SetKeyActiveDateTime(taf_ks_KeyRef_t keyRef, uint64_t value);
            le_result_t SetKeyOriginationExpireDateTime(taf_ks_KeyRef_t keyRef, uint64_t value);
            le_result_t SetKeyUsageExpireDateTime(taf_ks_KeyRef_t keyRef, uint64_t value);
            le_result_t ProvisionRsaEncKeyValue(taf_ks_KeyRef_t keyRef,
                                                     taf_ks_RsaKeySize_t keySize,
                                                     taf_ks_RsaEncPadding_t padding,
                                                     const uint8_t* impDataPtr,
                                                     size_t impDataSize);
            le_result_t ProvisionRsaSigKeyValue(taf_ks_KeyRef_t keyRef,
                                                     taf_ks_RsaKeySize_t keySize,
                                                     taf_ks_RsaSigPadding_t padding,
                                                     const uint8_t* impDataPtr,
                                                     size_t impDataSize);
            le_result_t ProvisionEcdsaKeyValue(taf_ks_KeyRef_t keyRef,
                                                    taf_ks_EccKeySize_t keySize,
                                                    taf_ks_Digest_t digest,
                                                    const uint8_t* impDataPtr,
                                                    size_t impDataSize);
            le_result_t ProvisionAesKeyValue(taf_ks_KeyRef_t keyRef,
                                                  taf_ks_AesKeySize_t keySize,
                                                  taf_ks_AesBlockMode_t mode,
                                                  const uint8_t* impDataPtr,
                                                  size_t impDataSize);
            le_result_t ProvisionHmacKeyValue(taf_ks_KeyRef_t keyRef,
                                                   uint32_t keySize,
                                                   taf_ks_Digest_t digest,
                                                   const uint8_t* impDataPtr,
                                                   size_t impDataSize);
            le_result_t ExportKey(taf_ks_KeyRef_t keyRef, const uint8_t* appDataPtr,
                                       size_t appDataSize, uint8_t* expDataPtr,
                                       size_t* expDataSizePtr);
            le_result_t ShareKey(taf_ks_KeyRef_t keyRef, const char* LE_NONNULL appName,
                                      taf_ks_KeyUsage_t keyCap, taf_ks_AppCapMask_t appCap);
            le_result_t CancelKeySharing(taf_ks_KeyRef_t keyRef, const char* LE_NONNULL appName);
            le_result_t GetCallingAppName(char* appName, size_t appNameSize);
            le_result_t GetFirstSharedApp(taf_ks_KeyRef_t keyRef, char* appName,
                                               size_t appNameSize, taf_ks_KeyUsage_t* keyCapPtr,
                                               taf_ks_AppCapMask_t* appCapPtr);
            le_result_t GetNextSharedApp(taf_ks_KeyRef_t keyRef, char* appName,
                                              size_t appNameSize, taf_ks_KeyUsage_t* keyCapPtr,
                                              taf_ks_AppCapMask_t* appCapPtr);
            taf_ks_KeySharingHandlerRef_t AddKeySharingHandler(
                const char* LE_NONNULL keyId,
                const char* LE_NONNULL appName,
                taf_ks_KeySharingHandlerFunc_t handlerPtr,
                void* contextPtr);
            void RemoveKeySharingHandler(taf_ks_KeySharingHandlerRef_t handlerRef);
            le_result_t CryptoSessionCreate(taf_ks_KeyRef_t keyRef,
                                                 taf_ks_CryptoSessionRef_t* sessionRefPtr);
            le_result_t CryptoSessionSetAesNonce(taf_ks_CryptoSessionRef_t sessionRef,
                                                      const uint8_t* dataPtr, size_t dataSize);
            le_result_t CryptoSessionSetRsaPadding(taf_ks_CryptoSessionRef_t sessionRef,
                                                        taf_ks_RsaPaddingType_t paddingType);
            le_result_t CryptoSessionSetAppData(taf_ks_CryptoSessionRef_t sessionRef,
                                                     const uint8_t* dataPtr, size_t dataSize);
            le_result_t CryptoSessionStart(taf_ks_CryptoSessionRef_t sessionRef,
                                                taf_ks_CryptoPurpose_t cryptoPurpose);
            le_result_t CryptoSessionProcessAead(taf_ks_CryptoSessionRef_t sessionRef,
                                                      const uint8_t* inputDataPtr,
                                                      size_t inputDataSize);
            le_result_t CryptoSessionProcess(taf_ks_CryptoSessionRef_t sessionRef,
                                                  const uint8_t* inputDataPtr,
                                                  size_t inputDataSize,
                                                  uint8_t* outputDataPtr,
                                                  size_t* outputDataSizePtr);
            le_result_t CryptoSessionEnd(taf_ks_CryptoSessionRef_t sessionRef,
                                              const uint8_t* inputDataPtr,
                                              size_t inputDataSize,
                                              uint8_t* outputDataPtr,
                                              size_t* outputDataSizePtr);
            le_result_t CryptoSessionAbort(taf_ks_CryptoSessionRef_t sessionRef);

        private:
            // Internal search functions.
            taf_ks_Key_t* SearchProvisionedKey(KeyMgt_KeyFileRef_t keyFileRef);
            taf_ks_Key_t* SearchNewKey(const char* keyId);

            // Internal set functions.
            void SetTag(taf_ks_Key_t* keyPtr, KeyTag_t* setTagPtr);
            void SetParam(taf_ks_CryptoSession_t* sessionPtr, KeyParam_t* setParamPtr);

            // Internal clear functions.
            void ClearTagList(taf_ks_Key_t* keyPtr);
            void ClearParamList(taf_ks_CryptoSession_t* sessionPtr);
            void ClearCryptoSessionList(taf_ks_Key_t* keyPtr);
            void ClearAppList(SharedAppList_t* appListPtr);
            void ClearSharedAppList(taf_ks_Key_t* keyPtr);

            // Internal crypto session helpers.
            void RemoveCryptoSession(taf_ks_CryptoSession_t* sessionPtr);
            bool HasRunningCryptoSession(le_dls_List_t cryptoSessionList);

            // Internal misc helpers.
            le_result_t GetAppNameBySessionRef(le_msg_SessionRef_t clientSessionRef,
                                                     char* appNameStr, size_t appNameSize);
            void CreateAppProKey(KeyMgt_KeyFileRef_t keyFileRef);
            void NotifyClientsForKeySharing(const char* keyIdPtr,
                                                 const char* ownerAppNamePtr,
                                                 const char* sharedAppNamePtr,
                                                 taf_ks_SharingState_t state);
            SharedAppList_t* UpdateAppListForClient(
                le_msg_SessionRef_t sessionRef,
                taf_ks_Key_t* keyPtr,
                const taf_pa_ks_sharedAppList_t* sharedAppListPtr);

            // Key and event object.
            le_ref_MapRef_t KeyRefMap_;
            le_mem_PoolRef_t KeyPool_;
            le_mem_PoolRef_t TagPool_;
            le_mem_PoolRef_t ParamPool_;
            le_mem_PoolRef_t AesNoncePool_;
            le_mem_PoolRef_t DataPool_;
            le_event_Id_t KeyEventId_;

            // Crypto session object.
            le_ref_MapRef_t CryptoSessionRefMap_;
            le_mem_PoolRef_t CryptoSessionPool_;

            // Key sharing handler object.
            le_ref_MapRef_t HandlerRefMap_;
            le_mem_PoolRef_t HandlerPool_;

            // Key sharing app list object.
            le_mem_PoolRef_t SharedAppPool_;
            le_mem_PoolRef_t SharedAppListPool_;

            // Timer used for PA initialization.
            le_timer_Ref_t InitTimer_;

            // Parm/Tag list buffers passed to PA layer.
            taf_pa_ks_Param_t* ParamPtrList_[MAX_PARAM_NUMBER];
            taf_pa_ks_Tag_t* TagPtrList_[MAX_TAG_NUMBER];
    };
}
#endif /* #ifndef TAFKEYSTORE_HPP */
