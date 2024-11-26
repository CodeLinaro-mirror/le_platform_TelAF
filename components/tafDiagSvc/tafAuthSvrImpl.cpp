/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <string>
#include "tafAuthSvr.hpp"
#include "configuration.hpp"
#include <arpa/inet.h>

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Get an instance of TelAF IOCtrl server.
 */
//--------------------------------------------------------------------------------------------------
taf_AuthSvr &taf_AuthSvr::GetInstance
(
)
{
    static taf_AuthSvr instance;

    return instance;
}

//-------------------------------------------------------------------------------------------------
/**
 * Create a reference for the service, or get the reference of a service if the reference already
 * exist.
 */
//-------------------------------------------------------------------------------------------------
taf_diagAuth_ServiceRef_t taf_AuthSvr::GetService
(
)
{
    LE_DEBUG("Gets the Authentication service!");

    taf_AuthSvc_t* servicePtr = GetServiceObj(taf_diagAuth_GetClientSessionRef());

    // Create a service object if it doesn't exist in the list.
    if (servicePtr == NULL)
    {
        servicePtr = (taf_AuthSvc_t *)le_mem_ForceAlloc(SvcPool);
        memset(servicePtr, 0, sizeof(taf_AuthSvc_t));

        // Init the service Rx Handler.
        servicePtr->rxHandlerRef = NULL;
        servicePtr->expHandlerRef = NULL;

        // Init message list.
        servicePtr->rxMsgList  = LE_DLS_LIST_INIT;
        servicePtr->supportedVlanList = LE_DLS_LIST_INIT;

        // Attach the service to the client.
        servicePtr->sessionRef = taf_diagAuth_GetClientSessionRef();

        // Create a Safe Reference for this service object
        servicePtr->svcRef = (taf_diagAuth_ServiceRef_t)le_ref_CreateRef(SvcRefMap,
                servicePtr);

        LE_DEBUG("svcRef %p of client %p is created for authentication",
                servicePtr->svcRef, servicePtr->sessionRef);
    }

    LE_INFO("Get serviceRef %p for Diag Authentication service.", servicePtr->svcRef);

    return servicePtr->svcRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the service instacnce object, if the service reference already created and return the
 * object pointer.
 */
//-------------------------------------------------------------------------------------------------
taf_AuthSvc_t* taf_AuthSvr::GetServiceObj
(
    le_msg_SessionRef_t sessionRef
)
{
    LE_DEBUG("Find the service object!");

    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_AuthSvc_t* servicePtr = (taf_AuthSvc_t *)le_ref_GetValue(iterRef);
        if ((servicePtr != NULL) && (sessionRef == servicePtr->sessionRef))
        {
            return servicePtr;
        }
    }

    return NULL;
}

taf_AuthSvc_t* taf_AuthSvr::GetServiceObj
(
    uint16_t vlanId
)
{
    LE_DEBUG("Find the service object for vlan(0x%x)!", vlanId);
    bool isFound = false;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_AuthSvc_t* servicePtr = (taf_AuthSvc_t *)le_ref_GetValue(iterRef);
        if (servicePtr != NULL)
        {
            // In some cases. the interface may not set the vlan.
            if (vlanId == 0 && le_dls_NumLinks(&servicePtr->supportedVlanList) == 0)
            {
                return servicePtr;
            }

            // Verify if the vlan is match.
            le_dls_Link_t* linkPtr = NULL;
            linkPtr = le_dls_Peek(&servicePtr->supportedVlanList);
            while (linkPtr)
            {
                taf_AuthVlanIdNode_t *vlan = CONTAINER_OF(linkPtr,
                    taf_AuthVlanIdNode_t, link);
                if (vlan != NULL && vlan->vlanId == vlanId)
                {
                    // Match.
                    isFound = true;
                    break;
                }
                linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
            }

            if (isFound)
            {
                return servicePtr;
            }
        }
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Authentication service handler function
 */
//-------------------------------------------------------------------------------------------------
void taf_AuthSvr::AuthSvcMsgHandler
(
    const taf_uds_AddrInfo_t* addrPtr,
    uint8_t* msgPtr,
    size_t msgLen
)
{
    uint8_t errCode = 0;
    uint8_t msgPos = 1;  // Skip sid
    uint8_t subFunc = msgPtr[msgPos] & 0x7F;
    if (subFunc != TAF_DIAGAUTH_DEAUTH &&
        subFunc != TAF_DIAGAUTH_VERIFY_CERT_UNI &&
        subFunc != TAF_DIAGAUTH_PROOF_OF_OWNERSHIP &&
        subFunc != TAF_DIAGAUTH_TRANSMIT_CERT &&
        subFunc != TAF_DIAGAUTH_CONFIG_AUTH)
    {
        LE_DEBUG("Sunfunction(0x%x) is invalid", subFunc);
        errCode = TAF_DIAG_SUBFUNCTION_NOT_SUPPORTED; // SubfunctionNotSupported
        SendNRCResp(reqAuthSvcId, addrPtr, errCode);
        return;
    }

    taf_AuthRxMsg_t *rxMsgPtr = NULL;
    rxMsgPtr = (taf_AuthRxMsg_t*)le_mem_ForceAlloc(RxMsgPool);
    memset(rxMsgPtr, 0, sizeof(taf_AuthRxMsg_t));

    memcpy(&rxMsgPtr->addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));
    rxMsgPtr->subFunc = subFunc;
    rxMsgPtr->link = LE_DLS_LINK_INIT;
    rxMsgPtr->rxMsgRef = (taf_diagAuth_RxMsgRef_t)le_ref_CreateRef(RxMsgRefMap, rxMsgPtr);
    rxMsgPtr->respLen = 1; // Return code.

    // Store playload in RxMsg structure.
    msgPos++;
    if (subFunc == TAF_DIAGAUTH_VERIFY_CERT_UNI)
    {
        taf_AuthVerifyCertUniMsg_t *tmpPtr = (taf_AuthVerifyCertUniMsg_t *)
                &(rxMsgPtr->playload.verifyCertUniMsg);
        tmpPtr->commConf = msgPtr[msgPos];
        msgPos += 1;
        tmpPtr->certLen = (msgPtr[msgPos] << 8) | msgPtr[msgPos+1];
        msgPos += 2;
        if (tmpPtr->certLen > TAF_DIAGAUTH_MAX_CERT_SIZE)
        {
            LE_WARN("Certificate size is 0x%x, out of range", tmpPtr->certLen);
            errCode = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            SendNRCResp(reqAuthSvcId, addrPtr, errCode);

            // Free the message
            le_ref_DeleteRef(RxMsgRefMap, rxMsgPtr->rxMsgRef);
            le_mem_Release(rxMsgPtr);
            return;
        }

        if (tmpPtr->certLen != 0)
        {
            memcpy(tmpPtr->certData, &(msgPtr[msgPos]), tmpPtr->certLen);
            msgPos += tmpPtr->certLen;
        }

        tmpPtr->challengeLen = (msgPtr[msgPos] << 8) | msgPtr[msgPos+1];
        msgPos += 2;
        if (tmpPtr->challengeLen > TAF_DIAGAUTH_MAX_CHALLENGE_SIZE)
        {
            LE_WARN("Challenge size is 0x%x, out of range", tmpPtr->challengeLen);
            errCode = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            SendNRCResp(reqAuthSvcId, addrPtr, errCode);

            // Free the message
            le_ref_DeleteRef(RxMsgRefMap, rxMsgPtr->rxMsgRef);
            le_mem_Release(rxMsgPtr);
            return;
        }

        if (tmpPtr->challengeLen != 0)
        {
            memcpy(tmpPtr->challengeData, &(msgPtr[msgPos]), tmpPtr->challengeLen);
            msgPos += tmpPtr->challengeLen;
        }
    }
    else if (subFunc == TAF_DIAGAUTH_PROOF_OF_OWNERSHIP)
    {
        taf_AuthPOWNMsg_t *tmpPtr = (taf_AuthPOWNMsg_t*)
                &(rxMsgPtr->playload.POWNMsg);
        tmpPtr->POWNLen = (msgPtr[msgPos] << 8) | msgPtr[msgPos+1];
        msgPos += 2;
        if (tmpPtr->POWNLen > TAF_DIAGAUTH_MAX_POWN_SIZE)
        {
            LE_WARN("POWN size is 0x%x, out of range", tmpPtr->POWNLen);
            errCode = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            SendNRCResp(reqAuthSvcId, addrPtr, errCode);

            // Free the message
            le_ref_DeleteRef(RxMsgRefMap, rxMsgPtr->rxMsgRef);
            le_mem_Release(rxMsgPtr);
            return;
        }

        if (tmpPtr->POWNLen != 0)
        {
            memcpy(tmpPtr->POWNData, &(msgPtr[msgPos]), tmpPtr->POWNLen);
            msgPos += tmpPtr->POWNLen;
        }

        tmpPtr->publicKeyLen = (msgPtr[msgPos] << 8) | msgPtr[msgPos+1];
        msgPos += 2;
        if (tmpPtr->publicKeyLen > TAF_DIAGAUTH_MAX_PUBIC_KEY_SIZE)
        {
            LE_WARN("POWN size is 0x%x, out of range", tmpPtr->publicKeyLen);
            errCode = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            SendNRCResp(reqAuthSvcId, addrPtr, errCode);

            // Free the message
            le_ref_DeleteRef(RxMsgRefMap, rxMsgPtr->rxMsgRef);
            le_mem_Release(rxMsgPtr);
            return;
        }

        if (tmpPtr->publicKeyLen != 0)
        {
            memcpy(tmpPtr->publicKey, &(msgPtr[msgPos]), tmpPtr->publicKeyLen);
            msgPos += tmpPtr->publicKeyLen;
        }
    }
    else if (subFunc == TAF_DIAGAUTH_TRANSMIT_CERT)
    {
        taf_AuthTransCertMsg_t *tmpPtr = (taf_AuthTransCertMsg_t*)
            &(rxMsgPtr->playload.transCertMsg);
        tmpPtr->certEvalId = ntohs(*((uint16_t*)(msgPtr + msgPos)));
        msgPos += 2;

        tmpPtr->certLen = (msgPtr[msgPos] << 8) | msgPtr[msgPos+1];
        msgPos += 2;
        if (tmpPtr->certLen > TAF_DIAGAUTH_MAX_CERT_SIZE)
        {
            LE_WARN("Certificate size is 0x%x, out of range", tmpPtr->certLen);
            errCode = TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT;
            SendNRCResp(reqAuthSvcId, addrPtr, errCode);

            // Free the message
            le_ref_DeleteRef(RxMsgRefMap, rxMsgPtr->rxMsgRef);
            le_mem_Release(rxMsgPtr);
            return;
        }

        if (tmpPtr->certLen != 0)
        {
            memcpy(tmpPtr->certData, &(msgPtr[msgPos]), tmpPtr->certLen);
            msgPos += tmpPtr->certLen;
        }
    }
    else
    {
        // No playload
        LE_DEBUG("Subfunciton(0x%x) in authentication service does not have playload.", subFunc);
    }

    // Report the Authentication request message to message handler in service layer.
    le_event_ReportWithRefCounting(ReqEvent, rxMsgPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * Authentication expiry notify handler function
 */
//-------------------------------------------------------------------------------------------------
void taf_AuthSvr::AuthNotifyMsgHandler
(
    const taf_uds_AddrInfo_t* addrPtr,
    uint8_t* msgPtr,
    size_t msgLen
)
{
    uint8_t msgPos = 1;  // Skip sid
    taf_AuthNotifyMsg_t authNotifyMsg;

    authNotifyMsg.roleId = ((uint64_t)msgPtr[msgPos] << 56) | ((uint64_t)msgPtr[msgPos + 1] << 48)
        | ((uint64_t)msgPtr[msgPos + 2] << 40) | ((uint64_t)msgPtr[msgPos + 3] << 32)
        | ((uint64_t)msgPtr[msgPos + 4] << 24) | ((uint64_t)msgPtr[msgPos + 5] << 16)
        | ((uint64_t)msgPtr[msgPos + 6] << 8) | ((uint64_t)msgPtr[msgPos + 7]);

    LE_DEBUG("Notify role Id is %" PRIu64, authNotifyMsg.roleId);

    memcpy(&authNotifyMsg.addrInfo, addrPtr, sizeof(taf_uds_AddrInfo_t));

    le_event_Report(AuthNotifyEvent, &authNotifyMsg, sizeof(authNotifyMsg));
}

//-------------------------------------------------------------------------------------------------
/**
 * UDS stack handler function
 */
//-------------------------------------------------------------------------------------------------
void taf_AuthSvr::UDSMsgHandler
(
    const taf_uds_AddrInfo_t* addrPtr,
    uint8_t sid,
    uint8_t* msgPtr,
    size_t msgLen
)
{
    uint8_t errCode = 0;

    LE_DEBUG("Authentication service UDSMsgHandler!");

    TAF_ERROR_IF_RET_NIL(addrPtr == NULL, "Invalid addrPtr");
    TAF_ERROR_IF_RET_NIL(msgPtr == NULL, "Invalid msgPtr");

    if (sid == reqAuthSvcId)
    {
        AuthSvcMsgHandler(addrPtr, msgPtr, msgLen);
    }
    else if (sid == authNotifyId)
    {
        AuthNotifyMsgHandler(addrPtr, msgPtr, msgLen);
    }
    else
    {
        LE_DEBUG("Service(0x%x) is invalid", sid);
        errCode = TAF_DIAG_SERVICE_NOT_SUPPORTED; // ServiceNotSupported
        SendNRCResp(sid, addrPtr, errCode);
        return;
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a Rx handler for a given service (Authentication).
 */
//-------------------------------------------------------------------------------------------------
taf_diagAuth_RxMsgHandlerRef_t taf_AuthSvr::AddRxMsgHandler
(
    taf_diagAuth_ServiceRef_t svcRef,
    taf_diagAuth_RxMsgHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_DEBUG("Authentication AddRxMsgHandler!");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    taf_AuthSvc_t *svcPtr = (taf_AuthSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(svcPtr == NULL, NULL, "Invalid service reference provided");

    if (svcPtr->rxHandlerRef != NULL)
    {
        LE_ERROR("Rx msg handler is already registered");
        return NULL;
    }

    taf_AuthReqHandler_t *handlerObjPtr = (taf_AuthReqHandler_t*)le_mem_ForceAlloc(ReqHandlerPool);

    // Initialize the RxHandler object.
    handlerObjPtr->svcRef       = svcRef;
    handlerObjPtr->func         = handlerPtr;
    handlerObjPtr->ctxPtr       = contextPtr;
    handlerObjPtr->handlerRef   = (taf_diagAuth_RxMsgHandlerRef_t)
            le_ref_CreateRef(ReqHandlerRefMap, handlerObjPtr);

    // Attach handler to service.
    svcPtr->rxHandlerRef = handlerObjPtr->handlerRef;

    LE_INFO("Authentication: Registered Rx Handler");

    return handlerObjPtr->handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the Rx handler for a given service (Authentication).
 */
//-------------------------------------------------------------------------------------------------
void taf_AuthSvr::RemoveRxMsgHandler
(
    taf_diagAuth_RxMsgHandlerRef_t handlerRef
)
{
    LE_DEBUG("Authentication RemoveRxMsgHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    taf_AuthReqHandler_t *handlerObjPtr = (taf_AuthReqHandler_t*)
            le_ref_Lookup(ReqHandlerRefMap, handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerObjPtr == NULL, "Invalid handlerObjPtr");
    TAF_ERROR_IF_RET_NIL(handlerObjPtr->handlerRef != handlerRef, "Invalid handler reference");

    taf_AuthSvc_t *svcPtr = (taf_AuthSvc_t*)le_ref_Lookup(SvcRefMap, handlerObjPtr->svcRef);
    if (svcPtr == NULL)
    {
        LE_WARN("The handler is not belong to this service.");
        le_ref_DeleteRef(ReqHandlerRefMap, handlerRef);
        le_mem_Release(handlerObjPtr);

        return;
    }

    // Detach the handler from service.
    svcPtr->rxHandlerRef = NULL;

    // Clear Rx Handler resources
    handlerObjPtr->handlerRef = NULL;
    handlerObjPtr->svcRef     = NULL;
    handlerObjPtr->func       = NULL;
    handlerObjPtr->ctxPtr     = NULL;

    // Free the handler.
    le_ref_DeleteRef(ReqHandlerRefMap, handlerRef);
    le_mem_Release(handlerObjPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Add a Rx handler for a given service (Authentication).
 */
//-------------------------------------------------------------------------------------------------
taf_diagAuth_AuthStateExpHandlerRef_t taf_AuthSvr::AddAuthExpHandler
(
    taf_diagAuth_ServiceRef_t svcRef,
    taf_diagAuth_AuthStateExpHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    LE_DEBUG("Authentication AddAuthExpHandler!");
    TAF_ERROR_IF_RET_VAL(handlerPtr == NULL, NULL, "Invalid handlerPtr!");

    taf_AuthSvc_t *svcPtr = (taf_AuthSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(svcPtr == NULL, NULL, "Invalid service reference provided");

    if (svcPtr->expHandlerRef != NULL)
    {
        LE_ERROR("Auth expiry handler is already registered");
        return NULL;
    }

    taf_AuthExpiryHandler_t *handlerObjPtr = (taf_AuthExpiryHandler_t*)
            le_mem_ForceAlloc(ExpHandlerPool);

    // Initialize the RxHandler object.
    handlerObjPtr->svcRef       = svcRef;
    handlerObjPtr->func         = handlerPtr;
    handlerObjPtr->ctxPtr       = contextPtr;
    handlerObjPtr->handlerRef   = (taf_diagAuth_AuthStateExpHandlerRef_t)
            le_ref_CreateRef(ExpHandlerRefMap, handlerObjPtr);

    // Attach handler to service.
    svcPtr->expHandlerRef = handlerObjPtr->handlerRef;

    LE_INFO("Authentication: Registered auth expiry Handler");

    return handlerObjPtr->handlerRef;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the Rx handler for a given service (Authentication).
 */
//-------------------------------------------------------------------------------------------------
void taf_AuthSvr::RemoveAuthExpHandler
(
    taf_diagAuth_AuthStateExpHandlerRef_t handlerRef
)
{
    LE_DEBUG("Authentication RemoveRxMsgHandler!");

    TAF_ERROR_IF_RET_NIL(handlerRef == NULL, "Invalid handlerRef");

    taf_AuthExpiryHandler_t *handlerObjPtr = (taf_AuthExpiryHandler_t*)
            le_ref_Lookup(ExpHandlerRefMap, handlerRef);
    TAF_ERROR_IF_RET_NIL(handlerObjPtr == NULL, "Invalid handlerObjPtr");
    TAF_ERROR_IF_RET_NIL(handlerObjPtr->handlerRef != handlerRef, "Invalid handler reference");

    taf_AuthSvc_t *svcPtr = (taf_AuthSvc_t*)le_ref_Lookup(SvcRefMap, handlerObjPtr->svcRef);
    if (svcPtr == NULL)
    {
        LE_WARN("The handler is not belong to this service.");
        le_ref_DeleteRef(ExpHandlerRefMap, handlerRef);
        le_mem_Release(handlerObjPtr);

        return;
    }

    // Detach the handler from service.
    svcPtr->expHandlerRef = NULL;

    // Clear Rx Handler resources
    handlerObjPtr->handlerRef = NULL;
    handlerObjPtr->svcRef     = NULL;
    handlerObjPtr->func       = NULL;
    handlerObjPtr->ctxPtr     = NULL;

    // Free the handler.
    le_ref_DeleteRef(ExpHandlerRefMap, handlerRef);
    le_mem_Release(handlerObjPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get communication configuration from RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::GetCommConfVal
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    uint8_t* commConfPtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(commConfPtr == NULL, LE_BAD_PARAMETER, "Invalid commConfPtr");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc != TAF_DIAGAUTH_VERIFY_CERT_UNI)
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    *commConfPtr = rxMsgPtr->playload.verifyCertUniMsg.commConf;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get certificate size from RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::GetCertSize
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    uint16_t* sizePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(sizePtr == NULL, LE_BAD_PARAMETER, "Invalid sizePtr");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc == TAF_DIAGAUTH_VERIFY_CERT_UNI)
    {
        *sizePtr = rxMsgPtr->playload.verifyCertUniMsg.certLen;
    }
    else if (rxMsgPtr->subFunc == TAF_DIAGAUTH_TRANSMIT_CERT)
    {
        *sizePtr = rxMsgPtr->playload.transCertMsg.certLen;
    }
    else
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get certificate data from RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::GetCertData
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    uint8_t* certPtr,
    size_t* certSizePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(certPtr == NULL, LE_BAD_PARAMETER, "Invalid certPtr");
    TAF_ERROR_IF_RET_VAL(certSizePtr == NULL, LE_BAD_PARAMETER, "Invalid certSizePtr");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc == TAF_DIAGAUTH_VERIFY_CERT_UNI)
    {
        if (*certSizePtr < rxMsgPtr->playload.verifyCertUniMsg.certLen)
        {
            LE_ERROR("Buffer is not enough for certificate.%" PRIuS "-%" PRIuS,
                *certSizePtr, (size_t)rxMsgPtr->playload.verifyCertUniMsg.certLen);
            return LE_OVERFLOW;
        }

        memcpy(certPtr, rxMsgPtr->playload.verifyCertUniMsg.certData,
            rxMsgPtr->playload.verifyCertUniMsg.certLen);
        *certSizePtr = rxMsgPtr->playload.verifyCertUniMsg.certLen;
    }
    else if (rxMsgPtr->subFunc == TAF_DIAGAUTH_TRANSMIT_CERT)
    {
        if (*certSizePtr < rxMsgPtr->playload.transCertMsg.certLen)
        {
            LE_ERROR("Buffer is not enough for certificate.%" PRIuS "-%" PRIuS,
                *certSizePtr, (size_t)rxMsgPtr->playload.transCertMsg.certLen);
            return LE_OVERFLOW;
        }

        memcpy(certPtr, rxMsgPtr->playload.transCertMsg.certData,
            rxMsgPtr->playload.transCertMsg.certLen);
        *certSizePtr = rxMsgPtr->playload.transCertMsg.certLen;
    }
    else
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get challenge data from RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::GetChallengeSize
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    uint16_t* sizePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(sizePtr == NULL, LE_BAD_PARAMETER, "Invalid sizePtr");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc == TAF_DIAGAUTH_VERIFY_CERT_UNI)
    {
        *sizePtr = rxMsgPtr->playload.verifyCertUniMsg.challengeLen;
    }
    else
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get challenge data from RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::GetChallengeData
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    uint8_t* challengePtr,
    size_t* challengeSizePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(challengePtr == NULL, LE_BAD_PARAMETER, "Invalid challengePtr");
    TAF_ERROR_IF_RET_VAL(challengeSizePtr == NULL, LE_BAD_PARAMETER, "Invalid challengeSizePtr");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc == TAF_DIAGAUTH_VERIFY_CERT_UNI)
    {
        if (*challengeSizePtr < rxMsgPtr->playload.verifyCertUniMsg.challengeLen)
        {
            LE_ERROR("Buffer is not enough for challenge.%" PRIuS "-%" PRIuS,
                *challengeSizePtr, (size_t)rxMsgPtr->playload.verifyCertUniMsg.challengeLen);
            return LE_OVERFLOW;
        }

        memcpy(challengePtr, rxMsgPtr->playload.verifyCertUniMsg.challengeData,
            rxMsgPtr->playload.verifyCertUniMsg.challengeLen);
        *challengeSizePtr = rxMsgPtr->playload.verifyCertUniMsg.challengeLen;
    }
    else
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get proof of ownership size from RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::GetPOWNSize
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    uint16_t* sizePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(sizePtr == NULL, LE_BAD_PARAMETER, "Invalid sizePtr");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc == TAF_DIAGAUTH_PROOF_OF_OWNERSHIP)
    {
        *sizePtr = rxMsgPtr->playload.POWNMsg.POWNLen;
    }
    else
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get proof of ownership data from RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::GetPOWNData
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    uint8_t* proofPtr,
    size_t* proofSizePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(proofPtr == NULL, LE_BAD_PARAMETER, "Invalid proofPtr");
    TAF_ERROR_IF_RET_VAL(proofSizePtr == NULL, LE_BAD_PARAMETER, "Invalid proofSizePtr");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc == TAF_DIAGAUTH_PROOF_OF_OWNERSHIP)
    {
        if (*proofSizePtr < rxMsgPtr->playload.POWNMsg.POWNLen)
        {
            LE_ERROR("Buffer is not enough for challenge.%" PRIuS "-%" PRIuS,
                *proofSizePtr, (size_t)rxMsgPtr->playload.POWNMsg.POWNLen);
            return LE_OVERFLOW;
        }

        memcpy(proofPtr, rxMsgPtr->playload.POWNMsg.POWNData,
            rxMsgPtr->playload.POWNMsg.POWNLen);
        *proofSizePtr = rxMsgPtr->playload.POWNMsg.POWNLen;
    }
    else
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get public key size from RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::GetPublicKeySize
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    uint16_t* sizePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(sizePtr == NULL, LE_BAD_PARAMETER, "Invalid sizePtr");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc == TAF_DIAGAUTH_PROOF_OF_OWNERSHIP)
    {
        *sizePtr = rxMsgPtr->playload.POWNMsg.publicKeyLen;
    }
    else
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get public key data from RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::GetPublicKeyData
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    uint8_t* publicKeyPtr,
    size_t* publicKeySizePtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(publicKeyPtr == NULL, LE_BAD_PARAMETER, "Invalid publicKeyPtr");
    TAF_ERROR_IF_RET_VAL(publicKeySizePtr == NULL, LE_BAD_PARAMETER, "Invalid publicKeySizePtr");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc == TAF_DIAGAUTH_PROOF_OF_OWNERSHIP)
    {
        if (*publicKeySizePtr < rxMsgPtr->playload.POWNMsg.publicKeyLen)
        {
            LE_ERROR("Buffer is not enough for challenge.%" PRIuS "-%" PRIuS,
                *publicKeySizePtr, (size_t)rxMsgPtr->playload.POWNMsg.publicKeyLen);
            return LE_OVERFLOW;
        }

        memcpy(publicKeyPtr, rxMsgPtr->playload.POWNMsg.publicKey,
            rxMsgPtr->playload.POWNMsg.publicKeyLen);
        *publicKeySizePtr = rxMsgPtr->playload.POWNMsg.publicKeyLen;
    }
    else
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get certificate evaluation id from RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::GetCertEvalId
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    uint16_t* idPtr
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(idPtr == NULL, LE_BAD_PARAMETER, "Invalid idPtr");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc == TAF_DIAGAUTH_TRANSMIT_CERT)
    {
        *idPtr = rxMsgPtr->playload.transCertMsg.certEvalId;
    }
    else
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set role to RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::SetRole
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    uint64_t role
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc != TAF_DIAGAUTH_PROOF_OF_OWNERSHIP)
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    taf_uds_DiagMsg_t diagMsg;
    uint8_t roleId[8];
    roleId[7] = role & 0xFF;
    roleId[6] = (role >> 8) & 0xFF;
    roleId[5] = (role >> 16) & 0xFF;
    roleId[4] = (role >> 24) & 0xFF;
    roleId[3] = (role >> 32) & 0xFF;
    roleId[2] = (role >> 40) & 0xFF;
    roleId[1] = (role >> 48) & 0xFF;
    roleId[0] = (role >> 56) & 0xFF;

    diagMsg.dataPtr = roleId;
    diagMsg.dataLen = sizeof(roleId);

    le_result_t ret = taf_uds_SetData(&rxMsgPtr->addrInfo, &diagMsg, TAF_UDS_DATA_TYPE_ROLE);
    if (ret != LE_OK)
    {
        LE_ERROR("taf_uds_SetData failed");
        return ret;
    }

    LE_DEBUG("Set role id %" PRIu64, role);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set challenge to RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::SetChallenge
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    const uint8_t* challengePtr,
    size_t challengeSize
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(challengePtr == NULL, LE_BAD_PARAMETER, "Invalid challengePtr");
    TAF_ERROR_IF_RET_VAL(challengeSize == (size_t)0, LE_BAD_PARAMETER, "Invalid challengeSize");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc != TAF_DIAGAUTH_VERIFY_CERT_UNI)
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    // Skip return code.
    rxMsgPtr->resp[1] = (challengeSize >> 8) & 0xFF;
    rxMsgPtr->resp[2] = challengeSize & 0xFF;
    memcpy(rxMsgPtr->resp + 3, challengePtr, challengeSize);
    rxMsgPtr->respLen = 3 + challengeSize;  // Include return code.

    // Init public key size to 00.
    rxMsgPtr->resp[rxMsgPtr->respLen] = 0;
    rxMsgPtr->resp[rxMsgPtr->respLen+1] = 0;
    rxMsgPtr->respLen += 2;

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set ephemeral public key to RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::SetPublicKey
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    const uint8_t* publicKeyPtr,
    size_t publicKeySize
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");
    TAF_ERROR_IF_RET_VAL(publicKeyPtr == NULL, LE_BAD_PARAMETER, "Invalid publicKeyPtr");
    TAF_ERROR_IF_RET_VAL(publicKeySize == (size_t)0, LE_BAD_PARAMETER, "Invalid publicKeySize");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc != TAF_DIAGAUTH_VERIFY_CERT_UNI)
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    if (rxMsgPtr->respLen <= 3)
    {
        LE_ERROR("Please set challenge first");
        return LE_FAULT;
    }

    // Skip return code, and overlay public key len.
    rxMsgPtr->resp[rxMsgPtr->respLen-2] = (publicKeySize >> 8) & 0xFF;
    rxMsgPtr->resp[rxMsgPtr->respLen-1] = publicKeySize & 0xFF;

    memcpy(rxMsgPtr->resp + rxMsgPtr->respLen, publicKeyPtr, publicKeySize);
    rxMsgPtr->respLen += publicKeySize;  // Include return code.

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set session key information to RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::SetSessKeyInfo
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    const uint8_t* sessKeyInfoPtr,
    size_t sessKeyInfoSize
)
{
    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

    if (rxMsgPtr->subFunc != TAF_DIAGAUTH_PROOF_OF_OWNERSHIP)
    {
        LE_ERROR("Subfunciton mismatch");
        return LE_OUT_OF_RANGE;
    }

    if (sessKeyInfoSize != 0 && sessKeyInfoPtr != NULL)
    {
        // Skip return code.
        rxMsgPtr->resp[1] = (sessKeyInfoSize >> 8) & 0xFF;
        rxMsgPtr->resp[2] = sessKeyInfoSize & 0xFF;
        memcpy(rxMsgPtr->resp + 3, sessKeyInfoPtr, sessKeyInfoSize);
        rxMsgPtr->respLen = 3 + sessKeyInfoSize;  // Include return code.
    }
    else if (sessKeyInfoSize == 0)
    {
        rxMsgPtr->resp[1] = 0;
        rxMsgPtr->resp[2] = 0;
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send Authentication response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::SendResp
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    uint8_t errCode,
    uint8_t retVal
)
{
    LE_DEBUG("Authentication service SendResp");

    TAF_ERROR_IF_RET_VAL(rxMsgRef == NULL, LE_BAD_PARAMETER, "Invalid rxMsgRef");

    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the reqMsg");
        return LE_NOT_FOUND;
    }

#ifndef LE_CONFIG_DIAG_VSTACK
    taf_AuthSvc_t* svcPtr = (taf_AuthSvc_t*)GetServiceObj(rxMsgPtr->addrInfo.vlanId);
#else
    taf_AuthSvc_t* svcPtr = (taf_AuthSvc_t*)GetServiceObj((uint16_t)0);
#endif
    if (svcPtr == NULL)
    {
        LE_ERROR("Not found registered Authentication service for this request!");
        return LE_NOT_FOUND;
    }

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = rxMsgPtr->addrInfo.ta;
    addrInfo.ta = rxMsgPtr->addrInfo.sa;
    addrInfo.taType = rxMsgPtr->addrInfo.taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = rxMsgPtr->addrInfo.vlanId;
    le_utf8_Copy(addrInfo.ifName, rxMsgPtr->addrInfo.ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif

    le_result_t ret = LE_OK;
    if (errCode == 0)
    {
        // Positive response.
        rxMsgPtr->resp[0] = retVal;
        ret = backend.RespDiagPositive(reqAuthSvcId, &addrInfo, rxMsgPtr->resp, rxMsgPtr->respLen);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send authentication positive response.(%d)", ret);
            return ret;
        }
    }
    else
    {
        // Negative response.
        ret = backend.RespDiagNegative(reqAuthSvcId, &addrInfo, errCode);
        if (ret != LE_OK)
        {
            LE_ERROR("Failed to send authentication negative response.(%d)", ret);
        }
    }

    // Remove the message from service message list.
    le_dls_Remove(&(svcPtr->rxMsgList), &(rxMsgPtr->link));

    // Free the message
    le_ref_DeleteRef(RxMsgRefMap, rxMsgPtr->rxMsgRef);
    le_mem_Release(rxMsgPtr);

    return ret;
}

//-------------------------------------------------------------------------------------------------
/**
 * Send NRC response to UDS stack.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::SendNRCResp
(
    uint8_t sid,
    const taf_uds_AddrInfo_t* addrInfoPtr,
    uint8_t errCode
)
{
    LE_DEBUG("Authentication service SendNRCResp");

    TAF_ERROR_IF_RET_VAL(addrInfoPtr == NULL, LE_BAD_PARAMETER, "Invalid addrInfoPtr");

    taf_uds_AddrInfo_t addrInfo;
    addrInfo.sa = addrInfoPtr->ta;
    addrInfo.ta = addrInfoPtr->sa;
    addrInfo.taType = addrInfoPtr->taType;
#ifndef LE_CONFIG_DIAG_VSTACK
    addrInfo.vlanId = addrInfoPtr->vlanId;
    le_utf8_Copy(addrInfo.ifName, addrInfoPtr->ifName, MAX_INTERFACE_NAME_LEN, NULL);
#endif

    // Call UDS function to send the response message.
    auto &backend = taf_DiagBackend::GetInstance();
    backend.RespDiagNegative(sid, &addrInfo, errCode);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Remove the created service and release the alloted memory.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::RemoveSvc
(
    taf_diagAuth_ServiceRef_t svcRef
)
{
    LE_DEBUG("RemoveSvc");

    taf_AuthSvc_t* servicePtr = (taf_AuthSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid servicePtr");

    // Release Authentication message resources.
    ClearMsgList(servicePtr);

    // Clear the registered Authentication RxMsg handler
    if (servicePtr->rxHandlerRef != NULL)
    {
        RemoveRxMsgHandler(servicePtr->rxHandlerRef);
        servicePtr->rxHandlerRef = NULL;
    }

    // Clear the registered Authentication StateExp handler
    if (servicePtr->expHandlerRef != NULL)
    {
        RemoveAuthExpHandler(servicePtr->expHandlerRef);
        servicePtr->expHandlerRef = NULL;
    }

    // Clear service object
    le_ref_DeleteRef(SvcRefMap, (void*)servicePtr->svcRef);
    le_mem_Release(servicePtr);

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear IOCtrl message list.
 */
//-------------------------------------------------------------------------------------------------
void taf_AuthSvr::ClearMsgList
(
    taf_AuthSvc_t* servicePtr
)
{
    LE_DEBUG("ClearMsgList");
    TAF_ERROR_IF_RET_NIL(servicePtr == NULL, "Invalid servicePtr");

    // Clear the UDS Rx message of IOCtrl list.
    le_dls_Link_t* linkPtr = le_dls_Pop(&servicePtr->rxMsgList);
    while (linkPtr != NULL)
    {
        taf_AuthRxMsg_t* rxMsgPtr = CONTAINER_OF(linkPtr, taf_AuthRxMsg_t, link);
        if (rxMsgPtr != NULL)
        {
            LE_DEBUG("Release ReqMsg(ref=%p)", rxMsgPtr->rxMsgRef);
            // Free the message
            le_ref_DeleteRef(RxMsgRefMap, rxMsgPtr->rxMsgRef);
            le_mem_Release(rxMsgPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&servicePtr->rxMsgList);
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Authentication event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_AuthSvr::RxAuthEventHandler
(
    void* reportPtr
)
{
    LE_DEBUG("RxAuthEventHandler!");
    auto &authIns = taf_AuthSvr::GetInstance();

    taf_AuthRxMsg_t *rxMsgPtr = (taf_AuthRxMsg_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(rxMsgPtr == NULL, "rxMsgPtr is Null");

    taf_AuthSvc_t *svcPtr = NULL;
    taf_AuthReqHandler_t *handlerObjPtr = NULL;

#ifndef LE_CONFIG_DIAG_VSTACK
    svcPtr = (taf_AuthSvc_t*)authIns.GetServiceObj(rxMsgPtr->addrInfo.vlanId);
#else
    svcPtr = (taf_AuthSvc_t*)authIns.GetServiceObj((uint16_t)0);
#endif
    if (svcPtr == NULL)
    {
#ifndef LE_CONFIG_DIAG_VSTACK
        LE_WARN("Not found registered Authentication service for this request(vlan:0x%x)!",
            rxMsgPtr->addrInfo.vlanId);
#else
        LE_WARN("Not found registered Authentication service for this request!");
#endif
        // UDS_0x29_NRC_21: service pointer is null
        authIns.SendNRCResp(authIns.reqAuthSvcId, &(rxMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(authIns.RxMsgRefMap, rxMsgPtr->rxMsgRef);
        le_mem_Release(rxMsgPtr);
        return;
    }

    if (svcPtr->rxHandlerRef == NULL)
    {
        LE_WARN("Did not register handler for Authentication service.");
        // UDS_0x29_NRC_21: handler is not registered
        authIns.SendNRCResp(authIns.reqAuthSvcId, &(rxMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(authIns.RxMsgRefMap, rxMsgPtr->rxMsgRef);
        le_mem_Release(rxMsgPtr);
        return;
    }

    // Lookup message handler of this service
    handlerObjPtr = (taf_AuthReqHandler_t*)
            le_ref_Lookup(authIns.ReqHandlerRefMap, svcPtr->rxHandlerRef);
    if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
    {
        LE_ERROR("Can not find Authentication RxMsg handler object!");
        // UDS_0x29_NRC_21: handler is null
        authIns.SendNRCResp(authIns.reqAuthSvcId, &(rxMsgPtr->addrInfo),
                TAF_DIAG_BUSY_REPEAT_REQUEST);
        le_ref_DeleteRef(authIns.RxMsgRefMap, rxMsgPtr->rxMsgRef);
        le_mem_Release(rxMsgPtr);
        return;
    }

    // Add the message in service message list and notify to application.
    rxMsgPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&svcPtr->rxMsgList, &rxMsgPtr->link);
    handlerObjPtr->func(rxMsgPtr->rxMsgRef, (taf_diagAuth_Type_t)rxMsgPtr->subFunc,
            handlerObjPtr->ctxPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * AuthNotify event handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_AuthSvr::AuthNotifyEventHandler
(
    void* reportPtr
)
{
    taf_AuthNotifyMsg_t *notifyPtr = (taf_AuthNotifyMsg_t*)reportPtr;
    TAF_ERROR_IF_RET_NIL(notifyPtr == NULL, "notifyPtr is Null");

    auto &authIns = taf_AuthSvr::GetInstance();
    taf_AuthSvc_t* servicePtr = NULL;
    taf_AuthExpiryHandler_t* handlerObjPtr = NULL;

#ifdef LE_CONFIG_DIAG_VSTACK
    servicePtr = (taf_AuthSvc_t*)authIns.GetServiceObj((uint16_t)0);
#else
    servicePtr = (taf_AuthSvc_t*)authIns.GetServiceObj(notifyPtr->addrInfo.vlanId);
#endif
    if (servicePtr == NULL)
    {
        return;
    }

    if (servicePtr->expHandlerRef == NULL)
    {
        LE_WARN("Did not register handler for auth notfication.");
        return;
    }

    // Lookup message handler of this service
    handlerObjPtr =
            (taf_AuthExpiryHandler_t*)le_ref_Lookup(authIns.ExpHandlerRefMap,
            servicePtr->expHandlerRef);
    if (handlerObjPtr == NULL || handlerObjPtr->func == NULL)
    {
        LE_INFO("Did not register handler for auth notfication.");
        return;
    }

    // Add the message in service message list and notify to application.
    handlerObjPtr->func(NULL, notifyPtr->addrInfo.vlanId,
        notifyPtr->roleId, handlerObjPtr->ctxPtr);

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Session close handler.
 */
//-------------------------------------------------------------------------------------------------
void taf_AuthSvr::OnClientDisconnection
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    LE_DEBUG("OnClientDisconnection");

    auto &authIns = taf_AuthSvr::GetInstance();
    le_ref_IterRef_t iterRef = le_ref_GetIterator(authIns.SvcRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_AuthSvc_t* servicePtr = (taf_AuthSvc_t *)le_ref_GetValue(iterRef);
        LE_ASSERT(servicePtr != NULL);
        LE_ASSERT(servicePtr->svcRef == le_ref_GetSafeRef(iterRef));

        if (servicePtr->sessionRef == sessionRef)
        {
            authIns.RemoveSvc(servicePtr->svcRef);
        }
    }

    return;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set vlan id to service.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::SetVlanId
(
    taf_diagAuth_ServiceRef_t svcRef,
    uint16_t vlanId
)
{
    taf_AuthSvc_t* servicePtr = (taf_AuthSvc_t*)le_ref_Lookup(SvcRefMap, svcRef);
    TAF_ERROR_IF_RET_VAL(servicePtr == NULL, LE_BAD_PARAMETER, "Invalid service reference");

#ifndef LE_CONFIG_DIAG_VSTACK
    // Check if the vlan is set.
    le_dls_Link_t* linkPtr = NULL;
    linkPtr = le_dls_Peek(&servicePtr->supportedVlanList);
    while (linkPtr)
    {
        taf_AuthVlanIdNode_t *vlan = CONTAINER_OF(linkPtr,
            taf_AuthVlanIdNode_t, link);
        if (vlan != NULL && vlan->vlanId == vlanId)
        {
            LE_INFO("The Vlan id(0x%x) is set for ref%p", vlanId, svcRef);
            return LE_OK;
        }
        linkPtr = le_dls_PeekNext(&servicePtr->supportedVlanList, linkPtr);
    }

    taf_AuthVlanIdNode_t *vlanPtr = (taf_AuthVlanIdNode_t *)
        le_mem_ForceAlloc(vlanPool);
    if (vlanPtr == NULL)
    {
        LE_INFO("Failed to allocate memory.");
        return LE_NO_MEMORY;
    }

    vlanPtr->vlanId = vlanId;
    vlanPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&servicePtr->supportedVlanList, &vlanPtr->link);

    return LE_OK;
#else
    return LE_NOT_IMPLEMENTED;
#endif
}

//-------------------------------------------------------------------------------------------------
/**
 * Get vlan id from RxMsg.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_AuthSvr::GetVlanIdFromMsg
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
    uint16_t* vlanIdPtr
)
{
    TAF_ERROR_IF_RET_VAL(vlanIdPtr == NULL, LE_BAD_PARAMETER, "Invalid vlanIdPtr");

#ifndef LE_CONFIG_DIAG_VSTACK
    taf_AuthRxMsg_t* rxMsgPtr = (taf_AuthRxMsg_t*)
        le_ref_Lookup(RxMsgRefMap, rxMsgRef);
    if (rxMsgPtr == NULL)
    {
        LE_ERROR("Cannot find the rxMsg in IO ctrl service");
        return LE_NOT_FOUND;
    }

    *vlanIdPtr = rxMsgPtr->addrInfo.vlanId;

    return LE_OK;
#else
    return LE_NOT_IMPLEMENTED;
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void taf_AuthSvr::Init
(
    void
)
{
    LE_INFO("taf_AuthSvr Init!");

    // Create memory pools.
    SvcPool = le_mem_CreatePool("AuthSvcPool", sizeof(taf_AuthSvc_t));
    RxMsgPool = le_mem_CreatePool("AuthRxMsgPool", sizeof(taf_AuthRxMsg_t));
    ReqHandlerPool = le_mem_CreatePool("AuthReqHandlerPool",
            sizeof(taf_AuthReqHandler_t));
    ExpHandlerPool = le_mem_CreatePool("AuthExpHandlerPool",
            sizeof(taf_AuthExpiryHandler_t));
    vlanPool = le_mem_CreatePool("AuthVlanPool", sizeof(taf_AuthVlanIdNode_t));

    // Create reference maps
    SvcRefMap = le_ref_CreateMap("AuthSvcRefMap", DEFAULT_SVC_REF_CNT);
    RxMsgRefMap = le_ref_CreateMap("AuthRxMsgRefMap", DEFAULT_RX_MSG_REF_CNT);
    ReqHandlerRefMap = le_ref_CreateMap("AuthReqHandlerRefMap",
            DEFAULT_RX_HANDLER_REF_CNT);
    ExpHandlerRefMap = le_ref_CreateMap("AuthExpHandlerRefMap",
            DEFAULT_RX_HANDLER_REF_CNT);

    // Create the event and add event handler for Authentication (0x29).
    ReqEvent = le_event_CreateIdWithRefCounting("AuthEvent");
    ReqEventHandlerRef = le_event_AddHandler("AuthEventHandlerRef", ReqEvent,
            taf_AuthSvr::RxAuthEventHandler);

    // Create an event and add the event handler for authentication notification.
    AuthNotifyEvent = le_event_CreateId("AuthNotifyEvent", sizeof(taf_AuthNotifyMsg_t));
    AuthNotifyEventHandlerRef = le_event_AddHandler("AuthNotifyEventHandlerRef", AuthNotifyEvent,
            taf_AuthSvr::AuthNotifyEventHandler);

    // Set session close handler
    le_msg_AddServiceCloseHandler(taf_diagAuth_GetServiceRef(), OnClientDisconnection, NULL);

    auto& backend = taf_DiagBackend::GetInstance();
    backend.RegisterUdsService(reqAuthSvcId, this);
    backend.RegisterUdsService(authNotifyId, this);

    LE_INFO("Diag Authentication Service started!");
}