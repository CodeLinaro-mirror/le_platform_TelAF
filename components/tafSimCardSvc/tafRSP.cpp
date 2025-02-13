/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include <telux/tel/PhoneFactory.hpp>
#include "tafRSP.hpp"

using namespace telux::tel;
using namespace telux::common;
using namespace tafsvc;

le_result_t  taf_simRsp_GetEID
(
    taf_sim_Id_t slotId,
    char* eidPtr,
    size_t eidLen
)
{
    TAF_ERROR_IF_RET_VAL(eidPtr == NULL, LE_BAD_PARAMETER, "eidPtr is NULL");
    TAF_ERROR_IF_RET_VAL(eidLen < TAF_SIMRSP_EID_BYTES, LE_OVERFLOW, "Incorrect buffer size");
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.GetEID(slotId, eidPtr, eidLen);
}

le_result_t taf_simRsp_AddProfile
(
    taf_sim_Id_t     slotId,
    const char*      activationCode,
    const char*      confirmationCode,
    bool             userConsentSupported
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.AddProfile(slotId, activationCode,
                 confirmationCode, userConsentSupported);
}

le_result_t taf_simRsp_DeleteProfile
(
    taf_sim_Id_t     slotId,
    uint32_t         profileId
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.DeleteProfile(slotId, profileId);
}

le_result_t taf_simRsp_SetProfile
(
    taf_sim_Id_t     slotId,
    uint32_t         profileId,
    bool             enable
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.SetProfile(slotId, profileId, enable);
}

le_result_t taf_simRsp_UpdateNickName
(
    taf_sim_Id_t     slotId,
    uint32_t         profileId,
    const char*      nickName
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.UpdateNickName(slotId, profileId, nickName);
}

le_result_t taf_simRsp_GetProfileList
(
    taf_sim_Id_t                     slotId,
    taf_simRsp_ProfileListNodeRef_t*    profileListPtr,
    size_t*                          profileCount
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.RequestProfileList(slotId, profileListPtr, profileCount);
}

le_result_t taf_simRsp_SetServerAddress
(
    taf_sim_Id_t                   slotId,
    const char*                    smdpAddress
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.SetServerAddress(slotId, smdpAddress);
}

le_result_t taf_simRsp_GetServerAddress
(
    taf_sim_Id_t                   slotId,
    char*                          smdpAddress,
    size_t                         smdpLength,
    char*                          smdsAddress,
    size_t                         smdsLength
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.GetServerAddress(slotId, smdpAddress, smdpLength, smdsAddress, smdsLength);
}

le_result_t taf_simRsp_ProvideUserConsent
(
    taf_sim_Id_t                     slotId,
    bool                             userConsent,
    taf_simRsp_UserConsentReasonType_t  reason
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.ProvideUserConsent(slotId, userConsent, reason);
}

le_result_t taf_simRsp_ProvideConfirmationCode
(
    taf_sim_Id_t            slotId,
    const char*             code
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.ProvideConfirmationCode(slotId, code, strlen(code));
}

taf_simRsp_ProfileDownloadHandlerRef_t taf_simRsp_AddProfileDownloadHandler
(
    taf_simRsp_ProfileDownloadHandlerFunc_t handlerPtr,
    void* contextPtr
)
{

    le_event_HandlerRef_t handlerRef;
    auto &rsp = taf_simRsp::GetInstance();
    handlerRef = (le_event_HandlerRef_t)rsp.AddProfileDownloadHandler(handlerPtr, contextPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_simRsp_ProfileDownloadHandlerRef_t)(handlerRef);

}

void taf_simRsp_RemoveProfileDownloadHandler(taf_simRsp_ProfileDownloadHandlerRef_t handlerRef)
{
    auto &rsp = taf_simRsp::GetInstance();
    rsp.RemoveProfileDownloadHandler(handlerRef);
}

taf_simRsp_ProfileUserConsentHandlerRef_t taf_simRsp_AddProfileUserConsentHandler
(
    taf_simRsp_ProfileUserConsentHandlerFunc_t handlerPtr,
    void* contextPtr
)
{

    le_event_HandlerRef_t handlerRef;
    auto &rsp = taf_simRsp::GetInstance();
    handlerRef = (le_event_HandlerRef_t)rsp.AddProfileUserConsentHandler(handlerPtr, contextPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_simRsp_ProfileUserConsentHandlerRef_t)(handlerRef);

}

void taf_simRsp_RemoveProfileUserConsentHandler(taf_simRsp_ProfileUserConsentHandlerRef_t handlerRef)
{
    auto &rsp = taf_simRsp::GetInstance();
    rsp.RemoveProfileUserConsentHandler(handlerRef);
}

taf_simRsp_ProfileConfirmationCodeHandlerRef_t taf_simRsp_AddProfileConfirmationCodeHandler
(
    taf_simRsp_ProfileConfirmationCodeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{

    le_event_HandlerRef_t handlerRef;
    auto &rsp = taf_simRsp::GetInstance();
    handlerRef = (le_event_HandlerRef_t)rsp.AddProfileConfirmationCodeHandler(handlerPtr, contextPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_simRsp_ProfileConfirmationCodeHandlerRef_t)(handlerRef);

}

void taf_simRsp_RemoveProfileConfirmationCodeHandler(taf_simRsp_ProfileConfirmationCodeHandlerRef_t handlerRef)
{
    auto &rsp = taf_simRsp::GetInstance();
    rsp.RemoveProfileConfirmationCodeHandler(handlerRef);
}

taf_simRsp_ProfileListNodeRef_t taf_simRsp_GetProfile
(
    uint32_t index
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.GetProfileListNodeRef(index);
}

uint32_t taf_simRsp_GetProfileIndex
(
    taf_simRsp_ProfileListNodeRef_t profileRef
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.GetProfileIndex(profileRef);
}

taf_simRsp_ProfileType_t taf_simRsp_GetProfileType
(
    taf_simRsp_ProfileListNodeRef_t profileRef
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.GetProfileType(profileRef);
}

le_result_t taf_simRsp_GetIccid
(
    taf_simRsp_ProfileListNodeRef_t profileRef,
    char * iccidPtr,
    size_t iccidLen
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.GetIccid(profileRef, iccidPtr, iccidLen);
}

bool taf_simRsp_GetProfileActiveStatus
(
    taf_simRsp_ProfileListNodeRef_t profileRef
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.GetProfileActiveStatus(profileRef);
}

le_result_t taf_simRsp_GetNickName
(
    taf_simRsp_ProfileListNodeRef_t profileRef,
    char * nickNamePtr,
    size_t nickNameLen
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.GetNickName(profileRef, nickNamePtr, nickNameLen);
}

le_result_t taf_simRsp_GetName
(
    taf_simRsp_ProfileListNodeRef_t profileRef,
    char * namePtr,
    size_t nameLen
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.GetName(profileRef, namePtr, nameLen);
}

le_result_t taf_simRsp_GetSpn
(
    taf_simRsp_ProfileListNodeRef_t profileRef,
    char* spnPtr,
    size_t spnLen
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.GetSpn(profileRef, spnPtr, spnLen);
}

taf_simRsp_IconType_t taf_simRsp_GetIconType
(
    taf_simRsp_ProfileListNodeRef_t profileRef
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.GetIconType(profileRef);
}

taf_simRsp_ProfileClass_t taf_simRsp_GetProfileClass
(
    taf_simRsp_ProfileListNodeRef_t profileRef
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.GetProfileClass(profileRef);
}

uint32_t taf_simRsp_GetMask
(
    taf_simRsp_ProfileListNodeRef_t profileRef
)
{
    auto &rsp = taf_simRsp::GetInstance();
    return rsp.GetMask(profileRef);
}
