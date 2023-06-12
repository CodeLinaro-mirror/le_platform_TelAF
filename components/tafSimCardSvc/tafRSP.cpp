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

/*  Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
using namespace telux::tafsvc;

le_result_t  taf_rsp_GetEID
(
    taf_sim_Id_t slotId,
    char* eidPtr,
    size_t eidLen
)
{
    TAF_ERROR_IF_RET_VAL(eidPtr == NULL, LE_BAD_PARAMETER, "eidPtr is NULL");
    TAF_ERROR_IF_RET_VAL(eidLen < TAF_RSP_EID_BYTES, LE_OVERFLOW, "Incorrect buffer size");
    auto &rsp = taf_rsp::GetInstance();
    return rsp.GetEID(slotId, eidPtr, eidLen);
}

le_result_t taf_rsp_AddProfile
(
    taf_sim_Id_t     slotId,
    const char*      activationCode,
    const char*      confirmationCode,
    bool             userConsentSupported
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.AddProfile(slotId, activationCode,
                 confirmationCode, userConsentSupported);
}

le_result_t taf_rsp_DeleteProfile
(
    taf_sim_Id_t     slotId,
    uint32_t         profileId
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.DeleteProfile(slotId, profileId);
}

le_result_t taf_rsp_SetProfile
(
    taf_sim_Id_t     slotId,
    uint32_t         profileId,
    bool             enable
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.SetProfile(slotId, profileId, enable);
}

le_result_t taf_rsp_UpdateNickName
(
    taf_sim_Id_t     slotId,
    uint32_t         profileId,
    const char*      nickName
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.UpdateNickName(slotId, profileId, nickName);
}

le_result_t taf_rsp_GetProfileList
(
    taf_sim_Id_t                     slotId,
    taf_rsp_ProfileListNodeRef_t*    profileListPtr,
    size_t*                          profileCount
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.RequestProfileList(slotId, profileListPtr, profileCount);
}

le_result_t taf_rsp_SetServerAddress
(
    taf_sim_Id_t                   slotId,
    const char*                    smdpAddress
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.SetServerAddress(slotId, smdpAddress);
}

le_result_t taf_rsp_GetServerAddress
(
    taf_sim_Id_t                   slotId,
    char*                          smdpAddress,
    size_t                         smdpLength,
    char*                          smdsAddress,
    size_t                         smdsLength
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.GetServerAddress(slotId, smdpAddress, smdpLength, smdsAddress, smdsLength);
}

le_result_t taf_rsp_ProvideUserConsent
(
    taf_sim_Id_t                     slotId,
    bool                             userConsent,
    taf_rsp_UserConsentReasonType_t  reason
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.ProvideUserConsent(slotId, userConsent, reason);
}

le_result_t taf_rsp_ProvideConfirmationCode
(
    taf_sim_Id_t            slotId,
    const char*             code
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.ProvideConfirmationCode(slotId, code, strlen(code));
}

taf_rsp_ProfileDownloadHandlerRef_t taf_rsp_AddProfileDownloadHandler
(
    taf_rsp_ProfileDownloadHandlerFunc_t handlerPtr,
    void* contextPtr
)
{

    le_event_HandlerRef_t handlerRef;
    auto &rsp = taf_rsp::GetInstance();
    handlerRef = (le_event_HandlerRef_t)rsp.AddProfileDownloadHandler(handlerPtr, contextPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_rsp_ProfileDownloadHandlerRef_t)(handlerRef);

}

void taf_rsp_RemoveProfileDownloadHandler(taf_rsp_ProfileDownloadHandlerRef_t handlerRef)
{
    auto &rsp = taf_rsp::GetInstance();
    rsp.RemoveProfileDownloadHandler(handlerRef);
}

taf_rsp_ProfileUserConsentHandlerRef_t taf_rsp_AddProfileUserConsentHandler
(
    taf_rsp_ProfileUserConsentHandlerFunc_t handlerPtr,
    void* contextPtr
)
{

    le_event_HandlerRef_t handlerRef;
    auto &rsp = taf_rsp::GetInstance();
    handlerRef = (le_event_HandlerRef_t)rsp.AddProfileUserConsentHandler(handlerPtr, contextPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_rsp_ProfileUserConsentHandlerRef_t)(handlerRef);

}

void taf_rsp_RemoveProfileUserConsentHandler(taf_rsp_ProfileUserConsentHandlerRef_t handlerRef)
{
    auto &rsp = taf_rsp::GetInstance();
    rsp.RemoveProfileUserConsentHandler(handlerRef);
}

taf_rsp_ProfileConfirmationCodeHandlerRef_t taf_rsp_AddProfileConfirmationCodeHandler
(
    taf_rsp_ProfileConfirmationCodeHandlerFunc_t handlerPtr,
    void* contextPtr
)
{

    le_event_HandlerRef_t handlerRef;
    auto &rsp = taf_rsp::GetInstance();
    handlerRef = (le_event_HandlerRef_t)rsp.AddProfileConfirmationCodeHandler(handlerPtr, contextPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_rsp_ProfileConfirmationCodeHandlerRef_t)(handlerRef);

}

void taf_rsp_RemoveProfileConfirmationCodeHandler(taf_rsp_ProfileConfirmationCodeHandlerRef_t handlerRef)
{
    auto &rsp = taf_rsp::GetInstance();
    rsp.RemoveProfileConfirmationCodeHandler(handlerRef);
}

taf_rsp_ProfileListNodeRef_t taf_rsp_GetProfile
(
    uint32_t index
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.GetProfileListNodeRef(index);
}

uint32_t taf_rsp_GetProfileIndex
(
    taf_rsp_ProfileListNodeRef_t profileRef
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.GetProfileIndex(profileRef);
}

taf_rsp_ProfileType_t taf_rsp_GetProfileType
(
    taf_rsp_ProfileListNodeRef_t profileRef
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.GetProfileType(profileRef);
}

le_result_t taf_rsp_GetIccid
(
    taf_rsp_ProfileListNodeRef_t profileRef,
    char * iccidPtr,
    size_t iccidLen
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.GetIccid(profileRef, iccidPtr, iccidLen);
}

bool taf_rsp_GetProfileActiveStatus
(
    taf_rsp_ProfileListNodeRef_t profileRef
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.GetProfileActiveStatus(profileRef);
}

le_result_t taf_rsp_GetNickName
(
    taf_rsp_ProfileListNodeRef_t profileRef,
    char * nickNamePtr,
    size_t nickNameLen
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.GetNickName(profileRef, nickNamePtr, nickNameLen);
}

le_result_t taf_rsp_GetName
(
    taf_rsp_ProfileListNodeRef_t profileRef,
    char * namePtr,
    size_t nameLen
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.GetName(profileRef, namePtr, nameLen);
}

le_result_t taf_rsp_GetSpn
(
    taf_rsp_ProfileListNodeRef_t profileRef,
    char* spnPtr,
    size_t spnLen
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.GetSpn(profileRef, spnPtr, spnLen);
}

taf_rsp_IconType_t taf_rsp_GetIconType
(
    taf_rsp_ProfileListNodeRef_t profileRef
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.GetIconType(profileRef);
}

taf_rsp_ProfileClass_t taf_rsp_GetProfileClass
(
    taf_rsp_ProfileListNodeRef_t profileRef
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.GetProfileClass(profileRef);
}

uint32_t taf_rsp_GetMask
(
    taf_rsp_ProfileListNodeRef_t profileRef
)
{
    auto &rsp = taf_rsp::GetInstance();
    return rsp.GetMask(profileRef);
}
