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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
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

/*
 * @file       tafDcsSvr.cpp
 * @brief      This file provides the telematics application framework data call service APIs.
 */
#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include "tafSvcIF.hpp"
#include "tafDcsConnectionImpl.hpp"
#include "tafDcsProfileImpl.hpp"
#include "tafDcsHelper.hpp"

using namespace telux::data;
using namespace telux::common;
using namespace telux::tafsvc;

/**
 * The initialization of TelAF data profile component.
 */
void taf_dcs_profile_init()
{
    LE_INFO("taf data profile component init start...\n");
    auto &dataProfile = taf_DataProfile::GetInstance();
    dataProfile.Init();
    LE_INFO("taf data profile component init done...\n");

    return;
}

/**
 * Get the data profile list.
 *
 * @param [in] profileList               The proflie list.
 * @param [inout] listSize               The size of the profile list.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get profile list.
 */
le_result_t taf_dcs_GetProfileList(taf_dcs_ProfileInfo_t *profileList, size_t *listSize)
{
    le_result_t result = LE_OK;
    uint8_t slotId;

    TAF_ERROR_IF_RET_VAL(profileList == nullptr, LE_BAD_PARAMETER, "Null ptr(profileList)");
    TAF_ERROR_IF_RET_VAL(listSize == nullptr, LE_BAD_PARAMETER, "Null ptr(listSize)");

    auto &dataProfile = taf_DataProfile::GetInstance();

    result = dataProfile.getSlotIdFromPhoneId(DEFAULT_PHONE_ID_1, &slotId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Failed to get slot id from phone id");

    return dataProfile.ListProfile(slotId, profileList, listSize);
}

/**
 * Get the data profile list with the specified phone id.
 *
 * @param [in] phoneId                   The phone id.
 * @param [in] profileList               The proflie list.
 * @param [inout] listSize               The size of the profile list.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get profile list.
 */
le_result_t taf_dcs_GetProfileListEx
(
    uint8_t phoneId,
    taf_dcs_ProfileInfo_t *profileList,
    size_t *listSize
)
{
    le_result_t result = LE_OK;
    uint8_t slotId;
    TAF_ERROR_IF_RET_VAL(profileList == nullptr, LE_BAD_PARAMETER, "Null ptr(profileList)");
    TAF_ERROR_IF_RET_VAL(listSize == nullptr, LE_BAD_PARAMETER, "Null ptr(listSize)");

    auto &dataProfile = taf_DataProfile::GetInstance();

    result = dataProfile.getSlotIdFromPhoneId(phoneId, &slotId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Failed to get slot id from phone id");

    return dataProfile.ListProfile(slotId, profileList, listSize);
}

/**
 * Get the profile reference corresponding to specified profile index.
 *
 *
 * @param [in] index                    The profile index.
 *
 * @returns reference                   The profile reference, should not be NULL.
 */
taf_dcs_ProfileRef_t taf_dcs_GetProfile(uint32_t index)
{
    // Profile index cannot be 0 with this API.
    TAF_ERROR_IF_RET_VAL(0 == index, NULL, "Profile id cannot be 0.");
    le_result_t result = LE_OK;
    uint8_t slotId;

    auto &dataProfile = taf_DataProfile::GetInstance();

    result = dataProfile.getSlotIdFromPhoneId(DEFAULT_PHONE_ID_1, &slotId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, NULL, "Failed to get slot id from phone id");

    return dataProfile.GetProfileRef(slotId, index);
}

/**
 * Get the profile reference corresponding to specified profile index and phone id.
 *
 *
 * @param [in] phone id                 The phone id.
 * @param [in] profileId                The profile index.
 *
 * @returns reference                   The profile reference, should not be NULL.
 */
taf_dcs_ProfileRef_t taf_dcs_GetProfileEx(uint8_t phoneId, uint32_t profileId)
{
    le_result_t result = LE_OK;
    uint8_t slotId;

    auto &dataProfile = taf_DataProfile::GetInstance();
    LE_INFO("Profile ID: %d", profileId);
    result = dataProfile.getSlotIdFromPhoneId(phoneId, &slotId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, NULL,  "Failed to get slot id from phone id");

    return dataProfile.GetProfileRef(slotId, profileId);
}

/**
 * Get data profile index corresponding to specified profile reference.
 *
 * @param [in] profileRef               The profile reference.
 *
 * @returns profile index.
 */
uint32_t taf_dcs_GetProfileIndex
(
    taf_dcs_ProfileRef_t profileRef
)
{
    LE_WARN("This API is deprecated. Use taf_dcs_GetProfileId");
    int32_t profileId;
    uint8_t slotId;
    auto &dataProfile = taf_DataProfile::GetInstance();
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    if (result == LE_OK)
    {
        return profileId;
    }
    else
    {
        LE_ERROR("Getting profile id from reference(%p) is failed, will use default id", profileRef);
        return 0;
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the profile ID for the given profile reference.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Profile reference was not found.
 *  - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_GetProfileId
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    uint32_t* profileIdPtr
        ///< [OUT] A positive number returns the profile number in the device. <br>
        ///< 0 indicates the profile is not yet created.
)
{
    TAF_ERROR_IF_RET_VAL(profileRef   == nullptr, LE_BAD_PARAMETER, "Null ptr(profileRef)");
    TAF_ERROR_IF_RET_VAL(profileIdPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(profileIdPtr)");

    int32_t profileId;
    uint8_t slotId;
    auto &dataProfile = taf_DataProfile::GetInstance();
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    if (result != LE_OK)
    {
        LE_ERROR("Getting profile id from reference(%p) failed", profileRef);
        *profileIdPtr = TAF_DCS_UNDEFINED_PROFILE_ID;
        return result;
    }
    else
    {
        LE_INFO("Profile Id: %d", profileId);
        *profileIdPtr = profileId;
    }
    return LE_OK;
}

/**
 * Get phone id corresponding to specified profile reference.
 *
 * @param [in] profileRef               The profile reference.
 * @param [out] phoneIdPtr              The phone id.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get the phone id.
 */
le_result_t taf_dcs_GetPhoneId
(
    taf_dcs_ProfileRef_t profileRef,
    uint8_t* phoneIdPtr
)
{
    le_result_t result = LE_OK;
    uint8_t slotId;
    int32_t retProfileId;

    TAF_ERROR_IF_RET_VAL(profileRef == nullptr, LE_BAD_PARAMETER, "Null ptr(profileRef)");
    TAF_ERROR_IF_RET_VAL(phoneIdPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(phoneIdPtr)");

    auto &dataProfile = taf_DataProfile::GetInstance();

    result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &retProfileId);

    if (result == LE_OK)
    {
        result = dataProfile.getPhoneIdFromSlotId(slotId, phoneIdPtr);
        TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Failed to get phone id from slot id");
    }
    else
    {
        LE_ERROR("Getting phone id from reference(%p) is failed", profileRef);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Creates a new profile on the device. Client should have set at least the IP family type for the
 * profile using ::taf_dcs_SetPDP before calling this API.<br>
 * On success creation of profile, the profile reference will be updated with the created profile
 * ID. Clients can use ::taf_dcs_GetProfileId to get the created profile ID.
 *
 * @return
 *  - #LE_OK -- Succeeded.
 *  - #LE_BAD_PARAMETER -- Bad parameter.
 *  - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_CreateProfile
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The profile reference.
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.CreateProfile(profileRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes the profile that is referenced. Once a profile is deleted, the profile reference is not
 * valid anymore.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameter.
 *  - LE_NOT_FOUND -- Profile reference was not found.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_DeleteProfile
(
    taf_dcs_ProfileRef_t profileRef
        ///< [IN] The profile reference.
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.DeleteProfile(profileRef);
}

/**
 * Set data profile APN corresponding to specified profile reference.
 *
 * This function call is asynchronous, the result will be returned by data event handler if failed.
 *
 * @param [in] profileRef               The profile which want to be updated.
 * @param [in] apnPtr                   The new APN string.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to update profile infomation.
 */
le_result_t taf_dcs_SetAPN(taf_dcs_ProfileRef_t profileRef, const char *apnPtr)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.SetApn(profileRef, apnPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the data profile name.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Profile reference was not found.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_SetProfileName
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    const char* LE_NONNULL name
        ///< [IN] The profile name.
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.SetProfileName(profileRef, name);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data profile name.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Failed.
 *  - LE_OVERFLOW -- Apn size is smaller than APN_NAME_MAX_BYTES.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_GetProfileName
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    char* name,
        ///< [OUT] The profile name.
    size_t nameSize
        ///< [IN]
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.GetProfileName(profileRef, name, nameSize);
}

/**
 * Set data profile PDP corresponding to specified profile reference.
 *
 * This function call is asynchronous, the result will be returned by data event handler if failed.
 *
 * @param [in] profileRef               The profile which want to be updated.
 * @param [in] pdp                      The new PDP type.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to update profile infomation.
 */
le_result_t taf_dcs_SetPDP
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_Pdp_t        pdp
)
{
    int32_t profileId;
    uint8_t slotId;
    taf_dcs_ConState_t state = TAF_DCS_CONNECTED;
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    // Ensure profile ref is not NULL
    TAF_ERROR_IF_RET_VAL((profileRef == NULL), LE_BAD_PARAMETER, "profileRef is null");
    // Validate only supported PDP values are passed.
    TAF_ERROR_IF_RET_VAL(((pdp != TAF_DCS_PDP_IPV4) &&
                          (pdp != TAF_DCS_PDP_IPV6) &&
                          (pdp != TAF_DCS_PDP_IPV4V6)),
                         LE_BAD_PARAMETER, "Invalid PDP type: %d", pdp);

    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);

    // Check if the data session is currently not disconnected for the given profile.
    // Only check if the profile exists.
    if (0 != profileId)
    {
        if (dataConnection.GetConnectionState(slotId, profileId, &state) == LE_OK &&
            state != TAF_DCS_DISCONNECTED)
            return LE_FAULT;
    }

    return dataProfile.SetPdp(profileRef, pdp);
}

/**
 * Set data profile authentication type, username and password corresponding to specified profile reference.
 *
 * This function call is asynchronous, the result will be returned by data event handler if failed.
 *
 * @param [in] profileRef               The profile which want to be updated.
 * @param [in] type                     The authentication type.
 * @param [in] userName                 The authentication username string.
 * @param [in] password                 The authentication password string
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to update profile infomation.
 */
le_result_t taf_dcs_SetAuthentication
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_Auth_t       type,
    const char         *userName,
    const char         *password
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.SetAuth(profileRef, type, userName, password);
}

/**
 * Get data profile APN string corresponding to specified profile reference.
 *
 * @param [in] profileRef               The profile which want to be updated.
 * @param [in] apnSize                  The input APN string max size.
 * @param [out] apnPtr                  The APN string, the size of this string should not exceed apnSize.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get profile infomation.
 */
le_result_t taf_dcs_GetAPN
(
    taf_dcs_ProfileRef_t  profileRef,
    char                 *apnPtr,
    size_t                apnSize
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.GetApn(profileRef, apnPtr, apnSize);
}

/**
 * Get data profile APN type corresponding to specified profile reference.
 *
 * @param [in] profileRef               The profile which want to be updated.
 * @param [out] apnType                 The APN type.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get APN type.
 */
le_result_t taf_dcs_GetApnTypes
(
    taf_dcs_ProfileRef_t  profileRef,
    taf_dcs_ApnType_t     *apnTypePtr
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.GetApnTypes(profileRef, apnTypePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the data profile APN type.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_SetApnTypes
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_ApnType_t apnType
        ///< [IN] The APN type.
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.SetApnTypes(profileRef, apnType);
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the data profile technology preference.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Profile reference was not found.
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_SetTechPreference
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_Tech_t techPref
        ///< [IN] The technology preference.
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.SetTechPreference(profileRef, techPref);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the data profile technology preference.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_NOT_FOUND -- Profile reference was not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_dcs_GetTechPreference
(
    taf_dcs_ProfileRef_t profileRef,
        ///< [IN] The profile reference.
    taf_dcs_Tech_t* techPrefPtr
        ///< [OUT] The technology preference.
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.GetTechPreference(profileRef, techPrefPtr);
}

/**
 * Get data profile PDP corresponding to specified profile reference.
 *
 * @param [in] profileRef               The profile which want to be updated.
 *
 * @returns DDP type.
 */
taf_dcs_Pdp_t taf_dcs_GetPDP
(
    taf_dcs_ProfileRef_t profileRef
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.GetPdp(profileRef);
}

/**
 * Get data profile authentication type, username and password corresponding to specified profile reference.
 *
 * @param [in] profileRef               The profile which want to be updated.
 * @param [out] typePtr                 The authentication type.
 * @param [out] userNamePtr             The authentication username string.
 * @param [in] userNameSize             The authentication username max size, userNamePtr should not exceed this value.
 * @param [out] passwordPtr             The authentication password string.
 * @param [in] passwordSize             The authentication password max size, passwordPtr should not exceed this value.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get profile infomation.
 */
le_result_t taf_dcs_GetAuthentication
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_Auth_t      *typePtr,
    char                *userNamePtr,
    size_t              userNameSize,
    char                *passwordPtr,
    size_t              passwordSize
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.GetAuthentication(profileRef, typePtr, userNamePtr, userNameSize, passwordPtr, passwordSize);
}

/**
 * Start to make a synchronous call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 * This is a synchronous function call.
 *
 * @param [in] profileRef               The profile reference to be started.
 *
 * @returns LE_OK                       Success to start this data session.
 *          OTHER                       Failed to start this data session.
 */
le_result_t taf_dcs_StartSession(taf_dcs_ProfileRef_t profileRef)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    bool apnThrottled = false;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                "Profile not created yet.");
    //check if profile's APN is throttled.
    result = dataProfile.IsAPNThrottled(slotId,profileId,&apnThrottled);
    TAF_ERROR_IF_RET_VAL(apnThrottled == true, LE_NOT_POSSIBLE, "Profile's APN is throttled");

    taf_dcs_Pdp_t pdpType = dataProfile.GetPdp(profileRef);
    return dataConnection.StartSessionCmdSync(slotId, profileId, pdpType, taf_dcs_GetClientSessionRef());
}

/**
 * Start to make a asynchronous call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 * This is an asynchronous function call, the handlerPtr will be called after getting the result.
 *
 * @param [in] profileRef               The profile reference to be started.
 *
 * @returns None.
 */
void taf_dcs_StartSessionAsync
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();

    return dataConnection.StartSessionCmdAsync(profileRef, handlerPtr, contextPtr,
                                               taf_dcs_GetClientSessionRef());
}

/**
 * Synchronouly stop a call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 * This is a synchronous function call, the state events will be reported by session state handler.
 *
 * @param [in] profileRef               The profile reference to be stopped.
 *
 * @returns LE_OK                       Success to stop this data session.
 *          OTHER                       Failed to stop this data session.
 */
le_result_t taf_dcs_StopSession(taf_dcs_ProfileRef_t profileRef)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                    "Profile not created yet.");

    taf_dcs_Pdp_t pdpType = dataProfile.GetPdp(profileRef);
    return dataConnection.StopSessionCmdSync(slotId, profileId, pdpType,
                                             taf_dcs_GetClientSessionRef());
}

/**
 * Asynchronouly stop a call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 * This is an asynchronous function call, the handlerPtr will be called after getting the result.
 *
 * @param [in] profileRef               The profile reference to be stopped.
 *
 * @returns None.
 */
void taf_dcs_StopSessionAsync
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();

    return dataConnection.StopSessionCmdAsync(profileRef, handlerPtr, contextPtr,
                                              taf_dcs_GetClientSessionRef());
}

/**
 * First session state handler used by le_event_AddLayeredHandler().
 *
 * @param [in] reportPtr               event pointer.
 * @param [in] subHandlerFunc          The secondary handler pointer, i.e. handlerPtr() from taf_dcs_AddSessionStateHandler().
 */
static void FirstSessionStateHandler(void* reportPtr, void* subHandlerFunc)
{
    int32_t profileId;
    uint8_t slotId;
    DataCallState_t* stateEvent = (DataCallState_t *)reportPtr;
    taf_dcs_SessionStateHandlerFunc_t handlerFunc = (taf_dcs_SessionStateHandlerFunc_t)subHandlerFunc;
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    LE_INFO("send callback to callRef: %p, callEvent: %s\n", stateEvent->callRef,
            taf_DCSHelper::CallEventToString(stateEvent->callEvent));

    le_result_t result = dataConnection.GetSlotIdAndProfileId(stateEvent->callRef, &slotId,
                                                              &profileId);

    TAF_ERROR_IF_RET_NIL(result != LE_OK, "Getting profile id and slot id failed");

    taf_dcs_ProfileRef_t profileRef = dataProfile.GetProfileRef(slotId, profileId);
    TAF_ERROR_IF_RET_NIL(profileRef == NULL, "cannot get profile ref from slot(%d) profile(%d)",
                         slotId, profileId);

    handlerFunc(profileRef, stateEvent->callEvent, &stateEvent->info, le_event_GetContextPtr());
}

/**
 * Add a session state handler to monitor the specified data call.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [in] handlerPtr               The handler function.
 * @param [in] contextPtr               The handler context.
 *
 * @returns reference                   Success to add session state handler.
 *          NULL                        Failed to add session state handler.
 */
taf_dcs_SessionStateHandlerRef_t taf_dcs_AddSessionStateHandler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_SessionStateHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    TAF_ERROR_IF_RET_VAL((profileRef == NULL) || (handlerPtr == NULL) , NULL,
                          "some pointers may be null");
    
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, NULL, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, NULL,
                                                                        "Profile not created yet.");

    le_event_Id_t sessionStateEvent = dataConnection.GetSessionStateEvent(slotId, profileId);
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                    "DataCallState",
                                                    sessionStateEvent,
                                                    FirstSessionStateHandler,
                                                    (void *)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_dcs_SessionStateHandlerRef_t)(handlerRef);
}

/**
 * Remove session state handler.
 *
 * @param [in] handlerRef               The state handler reference returned by taf_dcs_AddSessionStateHandler().
 *
 * @returns NA
 *
 * @note    NA
 */
void taf_dcs_RemoveSessionStateHandler
(
    taf_dcs_SessionStateHandlerRef_t handlerRef
)
{
    TAF_ERROR_IF_RET_NIL((handlerRef == NULL) , "handlerRef is null");
    le_event_RemoveHandler((le_event_HandlerRef_t) handlerRef);
    return;
}

/**
 * First roaming status handler used by taf_dcs_AddRoamingStatusHandler().
 *
 * @param [in] reportPtr               event pointer.
 * @param [in] subHandlerFunc          The secondary handler pointer, i.e. handlerPtr() from taf_dcs_AddRoamingStatusHandler().
 */
static void FirstRoamingStatusHandler(void* reportPtr, void* subHandlerFunc)
{
    TAF_ERROR_IF_RET_NIL(reportPtr == nullptr, "Null ptr(reportPtr)");

    TAF_ERROR_IF_RET_NIL(subHandlerFunc == nullptr, "Null ptr(subHandlerFunc)");

    taf_dcs_RoamingStatusHandlerFunc_t handlerFunc =
                                                 (taf_dcs_RoamingStatusHandlerFunc_t)subHandlerFunc;
    handlerFunc((taf_dcs_RoamingStatusInd_t*)reportPtr, le_event_GetContextPtr());

    le_mem_Release(reportPtr);

}

/**
 * Add a roaming status handler to monitor the roaming status.
 *
 * @param [in] handlerPtr               The handler function.
 * @param [in] contextPtr               The handler context.
 *
 * @returns reference                   Success to add roaming status handler.
 *          NULL                        Failed to add roaming status handler.
 */
taf_dcs_RoamingStatusHandlerRef_t taf_dcs_AddRoamingStatusHandler
(
    taf_dcs_RoamingStatusHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                    "RoamingStatus",
                                                    dataConnection.RoamingStatusEvtId,
                                                    FirstRoamingStatusHandler,
                                                    (void *)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_dcs_RoamingStatusHandlerRef_t)(handlerRef);
}

/**
 * Remove roaming status handler.
 *
 * @param [in] handlerRef The state handler reference returned by taf_dcs_AddRoamingStatusHandler().
 *
 * @returns NA
 *
 * @note    NA
 */
void taf_dcs_RemoveRoamingStatusHandler
(
    taf_dcs_RoamingStatusHandlerRef_t handlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t) handlerRef);
    return;
}

/**
 * Get the roaming status.
 *
 * @param [in] phoneId                  The phone id.
 * @param [out] isRoamingPtr            Is roaming on or off.
 * @param [out] typePtr                 The roaming type.
 *
 * @returns LE_OK                       Success to get the roaming status.
 *          OTHER                       Failed to get the roaming status.
 *
 */

le_result_t taf_dcs_GetRoamingStatus
(
    uint8_t phoneId,
    bool* isRoamingPtr,
    taf_dcs_RoamingType_t* typePtr
)
{
    le_result_t result = LE_OK;
    uint8_t slotId;

    TAF_ERROR_IF_RET_VAL(isRoamingPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(isRoamingPtr)");
    TAF_ERROR_IF_RET_VAL(typePtr == nullptr, LE_BAD_PARAMETER, "Null ptr(typePtr)");

    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    result = dataProfile.getSlotIdFromPhoneId(phoneId, &slotId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Failed to get slot id from phone id");

    return dataConnection.GetRoamingStatus(slotId, isRoamingPtr, typePtr);
}

/**
 * Get the interface address corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] addrPtr                 The interface name address.
 * @param [in] addrSize                 The max size of interface name string, specified in taf_dcs.api.
 *
 * @returns LE_OK                       Success to interface name.
 *          OTHER                       Failed to interface name.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetInterfaceName(taf_dcs_ProfileRef_t profileRef, char* namePtr, size_t nameSize)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                    "Profile not created yet.");

    return dataConnection.GetInterfaceName(slotId, profileId, namePtr, nameSize);
}

/**
 * Get the IPv4 interface address corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] addrPtr                 The IP address.
 * @param [in] addrSize                 The max size of gateway string, specified in taf_dcs.api.
 *
 * @returns LE_OK                       Success to get IP address.
 *          OTHER                       Failed to get IP address.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv4Address(taf_dcs_ProfileRef_t profileRef, char *addrPtr, size_t addrSize)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                        "Profile not created yet.");

    return dataConnection.GetIpv4Address(slotId, profileId, addrPtr, addrSize);
}

/**
 * Get the IPv6 interface mask corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to
 * specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] mask                    The IP mask.
 *
 * @returns LE_OK                       Success to get mask.
 *          OTHER                       Failed to get mask.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv4SubnetMask(taf_dcs_ProfileRef_t profileRef, uint32_t* mask)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                        "Profile not created yet.");

    return dataConnection.GetIpv4SubnetMask(slotId, profileId, mask);
}

/**
 * Get the IPv4 gateway corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] addrPtr                 The gateway address.
 * @param [in] addrSize                 The max size of gateway string, specified in taf_dcs.api.
 *
 * @returns LE_OK                       Success to get gateway address.
 *          OTHER                       Failed to get gateway address.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv4GatewayAddress
(
    taf_dcs_ProfileRef_t    profileRef,
    char*   addrPtr,
    size_t  addrSize
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                        "Profile not created yet.");

    return dataConnection.GetIpv4Gateway(slotId, profileId, addrPtr, addrSize);
}

/**
 * Get the IPv4 DNS corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] dns1AddrPtr             The first DNS address.
 * @param [in] dns1AddrSize             The max size of DNS address, specified in taf_dcs.api.
 * @param [out] dns2AddrPtr             The secondary DNS address.
 * @param [in] dns2AddrSize             The max size of DNS address, specified in taf_dcs.api..
 *
 * @returns LE_OK                       Success to get DNS address.
 *          OTHER                       Failed to get DNS address.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv4DNSAddresses
(
    taf_dcs_ProfileRef_t    profileRef,
    char*   dns1AddrPtr,
    size_t  dns1AddrSize,
    char*   dns2AddrPtr,
    size_t  dns2AddrSize
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                    "Profile not created yet.");

    return dataConnection.GetIpv4Dns(slotId, profileId, dns1AddrPtr, dns1AddrSize, dns2AddrPtr,
                                     dns2AddrSize);
}

/**
 * Get the IPv6 interface address corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] addrPtr                 The IP address.
 * @param [in] addrSize                 The max size of gateway string, specified in taf_dcs.api.
 *
 * @returns LE_OK                       Success to get IP address.
 *          OTHER                       Failed to get IP address.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv6Address(taf_dcs_ProfileRef_t profileRef, char *addrPtr, size_t addrSize)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                        "Profile not created yet.");

    return dataConnection.GetIpv6Address(slotId, profileId, addrPtr, addrSize);
}

/**
 * Get the IPv6 interface mask corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding
 * to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] mask                    The IP mask.
 *
 * @returns LE_OK                       Success to get mask.
 *          OTHER                       Failed to get mask.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv6SubnetMask(taf_dcs_ProfileRef_t profileRef, uint32_t* mask)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                        "Profile not created yet.");

    return dataConnection.GetIpv6SubnetMask(slotId, profileId, mask);
}


/**
 * Get the IPv6 gateway corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] addrPtr                 The gateway address.
 * @param [in] addrSize                 The max size of gateway string, specified in taf_dcs.api.
 *
 * @returns LE_OK                       Success to get gateway address.
 *          OTHER                       Failed to get gateway address.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv6GatewayAddress
(
    taf_dcs_ProfileRef_t    profileRef,
    char*   addrPtr,
    size_t  addrSize
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                        "Profile not created yet.");

    return dataConnection.GetIpv6Gateway(slotId, profileId, addrPtr, addrSize);
}

/**
 * Get the IPv6 DNS corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] dns1AddrPtr             The first DNS address.
 * @param [in] dns1AddrSize             The max size of DNS address, specified in taf_dcs.api.
 * @param [out] dns2AddrPtr             The secondary DNS address.
 * @param [in] dns2AddrSize             The max size of DNS address, specified in taf_dcs.api..
 *
 * @returns LE_OK                       Success to get DNS address.
 *          OTHER                       Failed to get DNS address.
 *
 * @note    If this session is not connected, '\0' string will be set on output addr pointers.
 */
le_result_t taf_dcs_GetIPv6DNSAddresses
(
    taf_dcs_ProfileRef_t    profileRef,
    char*   dns1AddrPtr,
    size_t  dns1AddrSize,
    char*   dns2AddrPtr,
    size_t  dns2AddrSize
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                        "Profile not created yet.");

    return dataConnection.GetIpv6Dns(slotId, profileId, dns1AddrPtr, dns1AddrSize, dns2AddrPtr,
                                     dns2AddrSize);
}

/**
 * Get the session state corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [out] statePtr                The session state enumeration, check ConState in taf_dcs.api.
 *
 * @returns LE_OK                       Success to get session state.
 *          OTHER                       Failed to get session state.
 *
 * @note    NA
 */
le_result_t taf_dcs_GetSessionState
(
    taf_dcs_ProfileRef_t    profileRef,
    taf_dcs_ConState_t*     statePtr
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                        "Profile not created yet.");

    return dataConnection.GetConnectionState(slotId, profileId, statePtr);
}

/**
 * Get the data bearer technology corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked
 * @param [out] dlDataBearerTechPtr     The down link data bearer technology
 * @param [out] ulDataBearerTechPtr     The up link data bearer technology
 *
 * @returns LE_OK                       Success to get the data bearer technology
 *          OTHER                       Failed to get data bearer technology
 *
 * @note    NA
 */
le_result_t taf_dcs_GetDataBearerTechnology
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_DataBearerTechnology_t* downDataBearerTechPtr,
    taf_dcs_DataBearerTechnology_t* upDataBearerTechPtr
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                    "Profile not created yet.");

    return dataConnection.GetDataBearerTechnology(slotId, profileId, downDataBearerTechPtr,
                                                  upDataBearerTechPtr);
}

/**
 * Check whether this data profile supports IPv6 or not.
 *
 * @param [in] profileRef     The profile reference to be checked
 *
 * @returns false             NOT support IPv6 or profile reference is invalid
 *          true              Support IPv4
 *
 * @note    NA
 */
bool taf_dcs_IsIPv4(taf_dcs_ProfileRef_t profileRef)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != false, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, false,
                                                                        "Profile not created yet.");

    return dataConnection.IsIpv4(slotId, profileId);
}

/**
 * Check whether this data profile supports IPv6 or not.
 *
 * @param [in] profileRef     The profile reference to be checked
 *
 * @returns false             NOT support IPv6 or profile reference is invalid
 *          true              Support IP6v
 *
 * @note    NA
 */
bool taf_dcs_IsIPv6(taf_dcs_ProfileRef_t profileRef)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, false, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, false,
                                                                        "Profile not created yet.");

    return dataConnection.IsIpv6(slotId, profileId);
}

/**
 * Use this method to send call state event to registered clients.
 */
void SendSessionStateEvent
(
    taf_dcs_ConState_t event,
    taf_dcs_StateInfo_t *infoPtr,
    taf_dcs_CallCtx_t *callCtxPtr
)
{
    DataCallState_t stateEvent;

    stateEvent.callRef   = callCtxPtr->callRef;
    stateEvent.callEvent = event;
    memcpy((char *)&stateEvent.info, (char *)infoPtr, sizeof(taf_dcs_StateInfo_t));
    le_event_Report(callCtxPtr->sessionStateEvent, &stateEvent, sizeof(stateEvent));
    return;
}

/**
 * Set the deafult data profile index.
 *
 * @param [in] index                    The default profile index.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to set default profile.
 */
le_result_t taf_dcs_SetDefaultProfileIndex(uint32_t profileId)
{

    uint32_t profileIdGet;
    uint8_t slotIdGet, slotId;
    le_result_t result = LE_OK;

    auto &dataProfile = taf_DataProfile::GetInstance();
    auto &dataConnection = taf_DataConnection::GetInstance();

    result = dataConnection.GetDefaultProfileIdSync(&slotIdGet, &profileIdGet);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Getting default profile failed");

    result = dataProfile.getSlotIdFromPhoneId(DEFAULT_PHONE_ID_1, &slotId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Failed to get slot id from phone id");

    if (slotIdGet == slotId && profileIdGet == profileId)
    {
        LE_INFO("profile id(%d) does not change", profileIdGet);
        return LE_OK;
    }

    return dataConnection.SetDefaultProfileIdSync(slotId, profileId);
}

/**
 * Set the deafult data profile index and phone id.
 *
 * @param [in] phoneId                      The default phone id.
 * @param [in] profileId                    The default profile index.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to set default profile.
 */
le_result_t taf_dcs_SetDefaultProfileIndexEx(uint8_t phoneId, uint32_t profileId)
{
    le_result_t result = LE_OK;
    uint32_t profileIdGet;
    uint8_t slotIdGet, slotId;

    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    result = dataProfile.getSlotIdFromPhoneId(phoneId, &slotId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Failed to get slot id from phone id");

    result = dataConnection.GetDefaultProfileIdSync(&slotIdGet, &profileIdGet);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Getting default profile failed");

    if (slotIdGet == slotId && profileIdGet == profileId)
    {
        LE_INFO("profile id(%d) does not change", profileIdGet);
        return LE_OK;
    }

    return dataConnection.SetDefaultProfileIdSync(slotId, profileId);
}

/**
 * Get the deafult data profile index.
 *
 * @returns default profile index
 */
uint32_t taf_dcs_GetDefaultProfileIndex()
{
    uint32_t profileId = TAF_DCS_DEFAULT_PROFILE;
    uint8_t slotId;
    le_result_t result = LE_OK;

    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    result = dataProfile.getSlotIdFromPhoneId(DEFAULT_PHONE_ID_1, &slotId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, TAF_DCS_DEFAULT_PROFILE,
                         "Failed to get slot id from phone id");

    dataConnection.GetDefaultProfileIdSync(&slotId, &profileId);

    return profileId;
}

/**
 * Get the deafult data profile index and phone id.
 *
 * @param [out] phoneIdPtr                  The phone id.
 * @param [out] profileIdPtr                The profile index.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get the phone id and profile index.

 */
le_result_t taf_dcs_GetDefaultPhoneIdAndProfileId(uint8_t* phoneIdPtr, uint32_t* profileIdPtr)
{
    le_result_t result = LE_OK;
    uint8_t slotId;

    TAF_ERROR_IF_RET_VAL(phoneIdPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(phoneIdPtr)");
    TAF_ERROR_IF_RET_VAL(profileIdPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(profileIdPtr)");

    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    result = dataConnection.GetDefaultProfileIdSync(&slotId, profileIdPtr);

    if (result == LE_OK)
    {
        result = dataProfile.getPhoneIdFromSlotId(slotId, phoneIdPtr);
        TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Failed to get phone id from slot id");
    }
    else
    {
        LE_ERROR("Getting default profile id and phone id failed");
    }

    return result;
}

/**
 * Get the data profile index by interface name.
 *
 * @param [in] intfName                     The interface name.
 * @param [out] profileId                   The profile index.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get the profile index.
 */
le_result_t taf_dcs_GetProfileIdByInterfaceName(const char* intfName,uint32_t* profileIdPtr)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    uint8_t slotId;

    le_result_t result = dataConnection.GetSlotIdAndProfileIdByIfName(intfName, &slotId,
                                                                      profileIdPtr);

    return result;
}

/**
 * Get the data profile index and phone id by interface name.
 *
 * @param [in] intfName                     The interface name.
 * @param [out] phoneIdPtr                  The phone id.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get the phone id index.

 */
le_result_t taf_dcs_GetPhoneIdByInterfaceName
(
    const char* intfName,
    uint8_t *phoneIdPtr
)
{
    le_result_t result = LE_OK;
    uint8_t slotId;
    uint32_t profileId;

    TAF_ERROR_IF_RET_VAL(intfName == nullptr, LE_BAD_PARAMETER, "Null ptr(intfName)");
    TAF_ERROR_IF_RET_VAL(phoneIdPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(phoneIdPtr)");

    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    result = dataConnection.GetSlotIdAndProfileIdByIfName(intfName, &slotId, &profileId);

    if (result == LE_OK)
    {
        result = dataProfile.getPhoneIdFromSlotId(slotId, phoneIdPtr);
        TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "Failed to get phone id from slot id");
    }
    else
    {
        LE_ERROR("Getting phone id by interface name(%s) is failed", intfName);
    }

    return result;
}

/**
 * First throttle state handler used by le_event_AddLayeredHandler().
 *
 * @param [in] reportPtr               event pointer.
 * @param [in] subHandlerFunc          The secondary handler pointer, i.e. handlerPtr()
 * from taf_dcs_AddThrottleStateHandler().
 */
static void FirstThrottleStateHandler(void* reportPtr, void* subHandlerFunc)
{
    ThrottleStatus_t* stateEvent = (ThrottleStatus_t *)reportPtr;
    taf_dcs_ThrottledStatusHandlerFunc_t handlerFunc =
                                                (taf_dcs_ThrottledStatusHandlerFunc_t)subHandlerFunc;
    auto &dataProfile = taf_DataProfile::GetInstance();

    taf_dcs_ProfileRef_t profileRef = dataProfile.GetProfileRef(stateEvent->slotId,
                                                                stateEvent->profileId);
    TAF_ERROR_IF_RET_NIL(profileRef == NULL, "cannot get profile ref from slot(%d) profile(%d)",
                         stateEvent->slotId, stateEvent->profileId);

    handlerFunc(profileRef, stateEvent->throttleState, stateEvent->ipv4Time, stateEvent->ipv6Time,
                                                       le_event_GetContextPtr());
}

/**
 * Add a throttle state handler to monitor the specified profile.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to
 * specified profile index.
 *
 * @param [in] profileRef               The profile reference to be checked.
 * @param [in] handlerPtr               The handler function.
 * @param [in] contextPtr               The handler context.
 *
 * @returns reference                   Success to add throttle state handler.
 *          NULL                        Failed to add throttle state handler.
 */
taf_dcs_ThrottledStatusHandlerRef_t taf_dcs_AddThrottledStatusHandler
(
    taf_dcs_ProfileRef_t profileRef,
    taf_dcs_ThrottledStatusHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();

    int32_t profileId;
    uint8_t slotId;
    bool apnThrottled = false;
    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, NULL, "profile reference(%p) is invalid", profileRef);

    le_event_Id_t throttleStateEvent = dataProfile.GetThrottleStateEvent(slotId, profileId);
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                    "DataThrottleState",
                                                    throttleStateEvent,
                                                    FirstThrottleStateHandler,
                                                    (void *)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    //check if profile's APN is throttled; if yes send event.
    dataProfile.SendAPNThrottledInfo(slotId,profileId,&apnThrottled);

    return (taf_dcs_ThrottledStatusHandlerRef_t)(handlerRef);
}

/**
 * Remove session state handler.
 *
 * @param [in] handlerRef
 * The state handler reference returned by taf_dcs_AddThrottledStatusHandler().
 *
 * @returns NA
 *
 * @note    NA
 */
void taf_dcs_RemoveThrottledStatusHandler
(
    taf_dcs_ThrottledStatusHandlerRef_t handlerRef
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t) handlerRef);
    return;
}

/**
 * Get the apn throttle and remaining time using profile reference.
 *
 * @param [in] intfName                   The profile reference.
 * @param [out] isThrottled               The throttled status.
 *                                        True when APN is throttled. False when APN is unthrottled
 * @param [out] ipv4RemainingTime         The remaining IPv4 throttled time in milliseconds.
 *                                        0 when throttle status is False.
 *                                        FFFF when APN does not support ipv4.
 * @param [out] ipv6RemainingTime         The remaining IPv6 throttled time in milliseconds.
 *                                        0 when throttle status is False.
 *                                        FFFF when APN does not support ipv6.
 *
 * @returns LE_OK                       Success.
 *          OTHER                       Failed to get the profile id reference.
 */
le_result_t taf_dcs_GetAPNThrottledStatus
(
    taf_dcs_ProfileRef_t    profileRef,
    bool         *isThrottled,
    uint32_t     *ipv4RemainingTime,
    uint32_t     *ipv6RemainingTime
)
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    return dataConnection.GetAPNThrottledStatus(profileRef,isThrottled,ipv4RemainingTime,
                                                                         ipv4RemainingTime);
}

le_result_t taf_dcs_GetAPNThrottledPLMN
(
    taf_dcs_ProfileRef_t    profileRef,     ///< The profile reference.
    bool       *areAllPLMNsThrottled,       ///< True if APN is throttled on all PLMNs.
    char                 *mccPtr,           ///< MCC of the PLMN on which the APN is throttled.
    size_t                mccSize,
    char                 *mncPtr,           ///< MNC of the PLMN on which the APN is throttled.
    size_t                mncSize
)
{
    auto &dataProfile = taf_DataProfile::GetInstance();
    return dataProfile.GetAPNThrottledPLMN(profileRef,areAllPLMNsThrottled,mccPtr,mccSize,
                                                                             mncPtr,mncSize);
}


/**
 * The init function of TelAF data connection component.
 */
void taf_dcs_connection_init()
{
    auto &dataConnection = taf_DataConnection::GetInstance();
    dataConnection.Init();

    dataConnection.RegisterSessionStateHandler(SendSessionStateEvent);

    LE_INFO("taf data profile component init done...\n");
}

/**
 * Start to make a synchronous permanent call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to
 * specified profile index.
 * This is a synchronous function call, the state events will be reported by session state handler.
 *
 * @param [in] profileRef               The profile reference to be started.
 *
 * @returns LE_OK                       Success to start this data session.
 *          OTHER                       Failed to start this data session.
 */
le_result_t taf_mdc_StartSession(taf_dcs_ProfileRef_t profileRef)
{
    LE_DEBUG("-----------taf_mdc_StartSession------------");
    int32_t profileId;
    uint8_t slotId;

    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);
    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                        "Profile not created yet.");

    taf_dcs_Pdp_t pdpType = dataProfile.GetPdp(profileRef);
    // Start a data call with a fixed value 0 for sessionRef, and when the client loses the
    // connection with data call service,the data call will not be stopped
    return dataConnection.StartSessionCmdSync(slotId, profileId, pdpType, 0);

}

/**
 * Start to make a asynchronous call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to
 * specified profile index.
 * This is an asynchronous function call, the state events will be reported by session state handler
 *
 * @param [in] profileRef               The profile reference to be started.
 *
 * @returns LE_OK                       Success to start this data session.
 *          OTHER                       Failed to start this data session.
 */
le_result_t taf_mdc_StartSessionAsync(taf_dcs_ProfileRef_t profileRef)
{
    LE_DEBUG("-----------taf_mdc_StartSessionAsync------------");
    int32_t profileId;
    uint8_t slotId;
    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                    "Profile not created yet.");

    taf_dcs_Pdp_t pdpType = dataProfile.GetPdp(profileRef);
    // Start a data call with a fixed value 0 for sessionRef, and when the client loses the
    // connection with data call service,the data call will not be stopped
    return dataConnection.StartSessionCmdSync(slotId, profileId, pdpType, 0);

}

/**
 * Synchronouly stop a permanent call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to
 * specified profile index.
 * This is an asynchronous function call, the state events will be reported by session state handler
 *
 * @param [in] profileRef               The profile reference to be stopped.
 *
 * @returns LE_OK                       Success to stop this data session.
 *          OTHER                       Failed to stop this data session.
 */
le_result_t taf_mdc_StopSession(taf_dcs_ProfileRef_t profileRef)
{
    LE_DEBUG("-----------taf_mdc_StopSession------------");
    int32_t profileId;
    uint8_t slotId;

    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                        "Profile not created yet.");

    taf_dcs_Pdp_t pdpType = dataProfile.GetPdp(profileRef);
    // When the application calls taf_mdc_StartSession() to start a data call, this function
    // can stop that data call
    return dataConnection.StopSessionCmdSync(slotId, profileId, pdpType, 0);
}

/**
 * Asynchronouly stop a call corresponding to specified profile reference.
 *
 * If this profile is not brought up so far, the call context will be created corresponding to
 * specified profile index.
 * This is an asynchronous function call, the state events will be reported by session state handler
 *
 * @param [in] profileRef               The profile reference to be stopped.
 *
 * @returns LE_OK                       Success to stop this data session.
 *          OTHER                       Failed to stop this data session.
 */
le_result_t taf_mdc_StopSessionAsync(taf_dcs_ProfileRef_t profileRef)
{
    LE_DEBUG("-----------taf_mdc_StopSessionAsync------------");
    int32_t profileId;
    uint8_t slotId;

    auto &dataConnection = taf_DataConnection::GetInstance();
    auto &dataProfile = taf_DataProfile::GetInstance();

    le_result_t result = dataProfile.GetSlotIdAndProfileId(profileRef, &slotId, &profileId);

    TAF_ERROR_IF_RET_VAL(result != LE_OK, result, "profile reference(%p) is invalid", profileRef);
    TAF_ERROR_IF_RET_VAL(TAF_DCS_UNDEFINED_PROFILE_ID == profileId, LE_NOT_POSSIBLE,
                                                                        "Profile not created yet.");

    taf_dcs_Pdp_t pdpType = dataProfile.GetPdp(profileRef);
    // When the application calls taf_mdc_StartSessionAsync() to start a data call, this function
    // can stop that data call
    return dataConnection.StopSessionCmdSync(slotId, profileId, pdpType, 0);

}

COMPONENT_INIT
{
    taf_dcs_profile_init();
    taf_dcs_connection_init();
    // Add boot KPI marker
    const char *kpi_file = "/sys/kernel/boot_kpi/kpi_values";
    const char *kpi_marker = "L - TelAF data call service is ready";
    FILE *file = fopen(kpi_file, "w");
    if (file == NULL)
    {
        LE_ERROR("%s does not exist", kpi_file);
        return;
    }
    if (fwrite(kpi_marker, sizeof(char), strlen(kpi_marker), file) != strlen(kpi_marker))
    {
        LE_ERROR("failed to write %s to %s", kpi_marker, kpi_file);
    }
    fclose(file);

    LE_INFO("the initialization of TelAf data call service is finished\n");
}
