/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 /*
  * @file       tafAuthSvc.cpp
  * @brief      This file provides the taf authentication service as interfaces described
  *             in taf_diagAuth.api. The Diag authentication service will be started automatically.
  */

#include "legato.h"
#include "interfaces.h"
#include "tafDiagBackend.hpp"
#include "tafAuthSvr.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference to a Authentication service, if there's no Authentication service,
 * a new one will be created.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
taf_diagAuth_ServiceRef_t taf_diagAuth_GetService
(
    void
)
{
    LE_DEBUG("taf_diagAuth_GetService");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.GetService();
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets VLAN ID to the service to filter authentication requests.
 * If the VLAN ID is not found or does not existed, it will return an error.
 * This function shall be called before registering RxMsgHandler and AuthStateExpHandler.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_UNSUPPORTED -- VLAN ID is unknown.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_SetVlanId
(
    taf_diagAuth_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    uint16_t vlanId
        ///< [IN] VLAN ID
)
{
    LE_DEBUG("taf_diagAuth_SetVlanId: vlan is 0x%x", vlanId);
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.SetVlanId(svcRef, vlanId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagAuth_RxMsg'
 *
 * This event provides information on Rx Authentication message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagAuth_RxMsgHandlerRef_t taf_diagAuth_AddRxMsgHandler
(
    taf_diagAuth_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagAuth_RxMsgHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diagAuth_AddRxMsgHandler");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.AddRxMsgHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagAuth_RxMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagAuth_RemoveRxMsgHandler
(
    taf_diagAuth_RxMsgHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_DEBUG("taf_diagAuth_RemoveRxMsgHandler");
    auto &authIns = taf_AuthSvr::GetInstance();

    authIns.RemoveRxMsgHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagAuth_AuthStateExp'
 *
 * This event provides information on Rx Authentication message.
 */
//--------------------------------------------------------------------------------------------------
taf_diagAuth_AuthStateExpHandlerRef_t taf_diagAuth_AddAuthStateExpHandler
(
    taf_diagAuth_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagAuth_AuthStateExpHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    LE_DEBUG("taf_diagAuth_AddAuthStateExpHandler");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.AddAuthExpHandler(svcRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagAuth_AuthStateExp'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagAuth_RemoveAuthStateExpHandler
(
    taf_diagAuth_AuthStateExpHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_DEBUG("taf_diagAuth_RemoveAuthStateExpHandler");
    auto &authIns = taf_AuthSvr::GetInstance();

    authIns.RemoveAuthExpHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the communication configuration of the Rx VERIFY_CERT_UNI
 * Authentication message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_GetCommConf
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t* commConfPtr
        ///< [OUT] Communication configuration.
)
{
    LE_DEBUG("taf_diagAuth_GetCommConf");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.GetCommConfVal(rxMsgRef, commConfPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the size of client certicicate of the Rx VERIFY_CERT_UNI/TRANSMIT_CERT
 * Authentication message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_GetCertSize
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint16_t* sizePtr
        ///< [OUT] The size of certicicate.
)
{
    LE_DEBUG("taf_diagAuth_GetCertSize");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.GetCertSize(rxMsgRef, sizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the size of client certicicate of the Rx VERIFY_CERT_UNI/VERIFY_CERT_BI/TRANSMIT_CERT
 * Authentication message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_GetCert
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t* certPtr,
        ///< [OUT] The client certicicate.
    size_t* certSizePtr
        ///< [INOUT]
)
{
    LE_DEBUG("taf_diagAuth_GetCert");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.GetCertData(rxMsgRef, certPtr, certSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the size of client challenge of the Rx VERIFY_CERT_UNI/VERIFY_CERT_BI
 * Authentication message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_GetChallengeSize
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint16_t* sizePtr
        ///< [OUT] The size of client challenge.
)
{
    LE_DEBUG("taf_diagAuth_GetChallengeSize");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.GetChallengeSize(rxMsgRef, sizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data of client challenge of the Rx VERIFY_CERT_UNI/VERIFY_CERT_BI/
 * Authentication message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_GetChallenge
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t* challengePtr,
        ///< [OUT] The client challenge.
    size_t* challengeSizePtr
        ///< [INOUT]
)
{
    LE_DEBUG("taf_diagAuth_GetChallenge");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.GetChallengeData(rxMsgRef, challengePtr, challengeSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the size of proof of ownership in the Rx PROOF_OF_OWNERSHIP
 * Authentication message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_GetPOWNSize
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint16_t* sizePtr
        ///< [OUT] The size of proof of ownership.
)
{
    LE_DEBUG("taf_diagAuth_GetPOWNSize");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.GetPOWNSize(rxMsgRef, sizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data of proof of ownership in the Rx PROOF_OF_OWNERSHIP
 * Authentication message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_GetPOWN
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t* proofPtr,
        ///< [OUT] The data of proof of ownership.
    size_t* proofSizePtr
        ///< [INOUT]
)
{
    LE_DEBUG("taf_diagAuth_GetPOWN");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.GetPOWNData(rxMsgRef, proofPtr, proofSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the size of public key of proof of ownership in the Rx PROOF_OF_OWNERSHIP
 * Authentication message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_GetPublicKeySize
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint16_t* sizePtr
        ///< [OUT] The size of ephemeral public key.
)
{
    LE_DEBUG("taf_diagAuth_GetPublicKeySize");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.GetPublicKeySize(rxMsgRef, sizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the public key of proof of ownership of the Rx PROOF_OF_OWNERSHIP
 * Authentication message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_GetPublicKey
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t* publicKeyPtr,
        ///< [OUT] The ephemeral public key.
    size_t* publicKeySizePtr
        ///< [INOUT]
)
{
    LE_DEBUG("taf_diagAuth_GetPublicKey");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.GetPublicKeyData(rxMsgRef, publicKeyPtr, publicKeySizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the role of the Rx PROOF_OF_OWNERSHIP Authentication message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_SetRole
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint64_t role
        ///< [IN] The role of proof of ownership.
)
{
    LE_DEBUG("taf_diagAuth_SetRole");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.SetRole(rxMsgRef, role);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the certificate evaluation id of the Rx TRANSMIT_CERT
 * Authentication message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_GetCertEvalId
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint16_t* idPtr
        ///< [OUT] The certificate evaluation id.
)
{
    LE_DEBUG("taf_diagAuth_GetCertEvalId");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.GetCertEvalId(rxMsgRef, idPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the VLAN ID of the Rx authenticate message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- VLAN ID not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_GetVlanIdFromMsg
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Receive message reference.
    uint16_t* vlanIdPtr
        ///< [OUT] VLAN ID.
)
{
    LE_DEBUG("taf_diagAuth_GetVlanIdFromMsg");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.GetVlanIdFromMsg(rxMsgRef, vlanIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the server challenge for the response of the Rx Authentication VERIFY_CERT_UNI message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_SetChallenge
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    const uint8_t* challengePtr,
        ///< [IN] The server challenge.
    size_t challengeSize
        ///< [IN]
)
{
    LE_DEBUG("taf_diagAuth_SetChallenge");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.SetChallenge(rxMsgRef, challengePtr, challengeSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the server public key for the response of the Rx Authentication VERIFY_CERT_UNI message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_SetPublicKey
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    const uint8_t* publicKeyPtr,
        ///< [IN] The server public key.
    size_t publicKeySize
        ///< [IN]
)
{
    LE_DEBUG("taf_diagAuth_SetPublicKey");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.SetPublicKey(rxMsgRef, publicKeyPtr, publicKeySize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the session key information for the response of the Rx Authentication
 * PROOF_OF_OWNERSHIP message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_SetSessKeyInfo
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    const uint8_t* sessKeyInfoPtr,
        ///< [IN] The session key information.
    size_t sessKeyInfoSize
        ///< [IN]
)
{
    LE_DEBUG("taf_diagAuth_SetSessKeyInfo");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.SetSessKeyInfo(rxMsgRef, sessKeyInfoPtr, sessKeyInfoSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sends a response message for the Rx Authentication message.
 *
 * @b NOTE: This function must be called to send a response if receiving a message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid rxMsgRef.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_FAULT -- Failed.
 *
 * @b NOTE: The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_SendResp
(
    taf_diagAuth_RxMsgRef_t rxMsgRef,
        ///< [IN] Received message reference.
    uint8_t errCode,
        ///< [IN] Error code type.
    uint8_t retVal
        ///< [IN] Authentication result.
)
{
    LE_DEBUG("taf_diagAuth_SendResp");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.SendResp(rxMsgRef, errCode, retVal);
}

//--------------------------------------------------------------------------------------------------
/**
 ** Releases a authentication expiry notification message.
 **
 ** @return
 **     - LE_OK -- Succeeded.
 **     - LE_BAD_PARAMETER -- Invalid msgRef or invalid service of the msgRef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_ReleaseAuthExpMsg
(
    taf_diagAuth_StateExpMsgRef_t expMsg
        ///< [IN] Authentication expiry notification reference.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removes the Authentication server service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagAuth_RemoveSvc
(
    taf_diagAuth_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    LE_DEBUG("taf_diagAuth_RemoveSvc");
    auto &authIns = taf_AuthSvr::GetInstance();

    return authIns.RemoveSvc(svcRef);
}
