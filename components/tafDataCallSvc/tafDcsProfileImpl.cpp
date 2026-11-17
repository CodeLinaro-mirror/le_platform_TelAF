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
 *  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file tafDcsProfileImpl.hpp
 * @brief TelAF Data Call Service's profile class(TafDcsProfile) implementation.
 *
 */

#include "tafDcsProfile.hpp"
#include "tafDcsUtils.hpp"
#include "tafSvcIF.hpp"

using namespace taf::svc::datacall;

// The constructor.is private should be called only from TafDcsProfile::Create
TafDcsProfile::TafDcsProfile
(
    uint8_t slotId,
    uint8_t phoneId,
    const TafDcsProfileInfo_t &profileInfo
) : slotId_(slotId), phoneId_(phoneId) // slotId_ and phoneId_ are immutables
{
    LE_INFO("Constructor");
    profileId_ = profileInfo.id;
    tech_ = profileInfo.techPref;
    pdp_ = profileInfo.ipType;
    authTypeBitmask_ = profileInfo.authTypeBitmask;
    apnTypeBitmask_ = profileInfo.apnTypeMask;
    bEmergencyCallSupport_ = profileInfo.emergencyCallSupport;
    connState_ = TAF_DCS_DISCONNECTED;
    apn_ = profileInfo.apn;
    name_ = profileInfo.name;
    userName_ = profileInfo.userName;
    password_ = profileInfo.password;

    // Initialize the client session list and the call action context.
    clientSessionList_ = LE_DLS_LIST_INIT;
    pthread_mutex_init(&callCtxMutex_, NULL);
    // Use the monotonic clock for the timeout of WaitCallActionDone().
    pthread_condattr_t condAttr;
    pthread_condattr_init(&condAttr);
    pthread_condattr_setclock(&condAttr, CLOCK_MONOTONIC);
    pthread_cond_init(&callActionCond_, &condAttr);
    pthread_condattr_destroy(&condAttr);

    // Store the pointer to this object
    thisPtr_ = this;
    // Create a reference to this object
    reference_ =
        (taf_dcs_ProfileRef_t)le_ref_CreateRef(TafDcsProfile::getProfileRefMap(), (void *)thisPtr_);
    LE_DEBUG("Profile(%p) created with reference: %p", thisPtr_, reference_ );
}

TafDcsProfile::~TafDcsProfile()
{
    LE_INFO("Destructor");
    if (reference_)
        le_ref_DeleteRef(TafDcsProfile::getProfileRefMap(), reference_);

    // Release the client session list and the call action context.
    le_dls_Link_t *linkPtr = le_dls_Pop(&clientSessionList_);
    while (NULL != linkPtr)
    {
        le_mem_Release(CONTAINER_OF(linkPtr, TafDcsClientSession_t, link));
        linkPtr = le_dls_Pop(&clientSessionList_);
    }
    pthread_cond_destroy(&callActionCond_);
    pthread_mutex_destroy(&callCtxMutex_);
}

void TafDcsProfile::PopulateProfileInfoStruct (taf::pa::data::ProfileInfo_t &profileInfo) const
{
    profileInfo.profileId = static_cast<taf::pa::data::ProfileId_e>(profileId_);
    memset(profileInfo.apn, 0, taf::pa::data::MAX_APN_LEN);
    le_utf8_Copy(profileInfo.apn, apn_.c_str(), taf::pa::data::MAX_APN_LEN, NULL);
    memset(profileInfo.name, 0, taf::pa::data::MAX_NAME_LEN);
    le_utf8_Copy(profileInfo.name, name_.c_str(), taf::pa::data::MAX_NAME_LEN, NULL);
    memset(profileInfo.userName, 0, taf::pa::data::MAX_USERNAME_LEN);
    le_utf8_Copy(profileInfo.userName, userName_.c_str(), taf::pa::data::MAX_USERNAME_LEN, NULL);
    memset(profileInfo.password, 0, taf::pa::data::MAX_PASSWORD_LEN);
    le_utf8_Copy(profileInfo.password, password_.c_str(), taf::pa::data::MAX_PASSWORD_LEN, NULL);
    profileInfo.techPref = TafDcsUtils::ConvertTechPref(tech_);
    profileInfo.authType = TafDcsUtils::ConvertAuthType(authTypeBitmask_);
    profileInfo.ipType = TafDcsUtils::ConvertPDP(pdp_);
    profileInfo.apnTypeMask = TafDcsUtils::ConvertApnTypeMask(apnTypeBitmask_);
    profileInfo.emergencyCallSupport =
                                TafDcsUtils::ConvertEmergencyCallSupport(bEmergencyCallSupport_);
}

// This is a static function that returns the reference map for the profile objects.
le_ref_MapRef_t TafDcsProfile::getProfileRefMap()
{
    if (nullptr == profileRefsMap_)
    {
        LE_INFO("Create profile references map");
        profileRefsMap_ = le_ref_CreateMap("profileRefsMap_", TAF_DCS_PROFILE_LIST_MAX_ENTRY);
    }
    LE_DEBUG("Return profile references map: %p", profileRefsMap_);
    return profileRefsMap_;
}

void TafDcsProfile::SetCallSetup(const bool bSetup)
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    bCallSetup_ = bSetup;
    LE_DEBUG("Setup: %d", bCallSetup_);
    return;
}

bool TafDcsProfile::GetCallSetup() const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Setup: %d", bCallSetup_);
    return bCallSetup_;
}

le_result_t TafDcsProfile::GetId(uint32_t &id) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    id = profileId_;
    return LE_OK;
}

le_result_t TafDcsProfile::GetSlotId(uint8_t &id) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Slot Id: %d", slotId_);
    id = slotId_;
    return LE_OK;
}

le_result_t TafDcsProfile::GetPhoneId(uint8_t &id) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    id = phoneId_;
    return LE_OK;
}

le_result_t TafDcsProfile::GetTech(taf_dcs_Tech_t &tech) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG ("Tech: %d", TO_INT(tech_));
    tech = tech_;
    return LE_OK;
}

le_result_t TafDcsProfile::GetPdp(taf_dcs_Pdp_t &pdp) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("PDP: %d", TO_INT(pdp_));
    pdp = pdp_;
    return LE_OK;
}

le_result_t TafDcsProfile::GetAuthTypeBitmask(taf_dcs_Auth_t &authTypeBitmask) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Auth Type Bitmask: %d", TO_INT(authTypeBitmask_));
    authTypeBitmask = authTypeBitmask_;
    return LE_OK;
}

le_result_t TafDcsProfile::GetApnTypeBitmask(taf_dcs_ApnType_t &apnTypeBitmask) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("APN Type Bitmask: %d", TO_INT(apnTypeBitmask_));
    apnTypeBitmask = apnTypeBitmask_;
    return LE_OK;
}

le_result_t TafDcsProfile::GetEmergencyCallSupport(bool &bEmerCallSupport) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Emergency call supported: %s", bEmergencyCallSupport_ ? "true" : "false");
    bEmerCallSupport = bEmergencyCallSupport_;
    return LE_OK;
}

le_result_t TafDcsProfile::GetMaxDataBitRates(uint64_t &rxRate, uint64_t &txRate) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Max Rx Rate: %lu, Max Tx Rate: %lu", maxRxBitRate_, maxTxBitRate_);
    rxRate = maxRxBitRate_;
    txRate = maxTxBitRate_;
    return LE_OK;
}

le_result_t TafDcsProfile::SetMaxDataBitRates(const uint64_t &rxRate, const uint64_t &txRate)
{
    LE_INFO("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    maxRxBitRate_ = rxRate;
    maxTxBitRate_ = txRate;
    LE_INFO("Max Rx Rate: %lu, Max Tx Rate: %lu", maxRxBitRate_, maxTxBitRate_);
    return LE_OK;
}

le_result_t TafDcsProfile::GetSessionState
(
    taf_dcs_ConState_t &state,
    taf_dcs_ConState_t &stateIPv4,
    taf_dcs_ConState_t &stateIPv6
) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Connection state     : %d", TO_INT(connState_));
    LE_DEBUG("Connection state IPv4: %d", TO_INT(connStateIPv4_));
    LE_DEBUG("Connection state IPv6: %d", TO_INT(connStateIPv6_));
    state = connState_;
    stateIPv4 = connStateIPv4_;
    stateIPv6 = connStateIPv6_;
    return LE_OK;
}

le_result_t TafDcsProfile::GetApn(std::string &apn) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("APN: %s", apn_.c_str());
    apn = apn_;
    return LE_OK;
}

le_result_t TafDcsProfile::GetName(std::string &name) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Name: %s", name_.c_str());
    name = name_;
    return LE_OK;
}

le_result_t TafDcsProfile::GetUserName(std::string &userName) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("User name: %s", userName_.c_str());
    userName = userName_;
    return LE_OK;
}

le_result_t TafDcsProfile::GetPassword(std::string &password) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Password: %s", password.c_str());
    password = password_;
    return LE_OK;
}

le_result_t TafDcsProfile::GetHostInterface(std::string &ifName) const
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    ifName = hostIfName_;
    LE_DEBUG("Host interface: %s", hostIfName_.c_str());
    return LE_OK;
}

le_result_t TafDcsProfile::SetHostInterface(const std::string &ifName)
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    hostIfName_ = ifName;
    LE_DEBUG("Host interface: %s", hostIfName_.c_str());
    return LE_OK;
}

void TafDcsProfile::ResetHostInterface()
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    hostIfName_.clear();
}

le_result_t TafDcsProfile::GetIPv4Addresses
(
    std::string &ipv4Addr,
    std::string &ipv4Gateway,
    std::string &ipv4DnsPrimary,
    std::string &ipv4DnsSecondary,
    unsigned int &ipv4AddrMask,
    unsigned int &ipv4GatewayMask
) const
{
    ipv4Addr         = ipv4Addr_;
    ipv4Gateway      = ipv4Gateway_;
    ipv4DnsPrimary   = ipv4DnsPrimary_;
    ipv4DnsSecondary = ipv4DnsSecondary_;
    ipv4AddrMask     = ipv4AddrMask_;
    ipv4GatewayMask  = ipv4GatewayMask_;
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("IPv4 address: %s", ipv4Addr_.c_str());
    LE_DEBUG("IPv4 gateway: %s", ipv4Gateway_.c_str());
    LE_DEBUG("IPv4 DNS primary: %s", ipv4DnsPrimary_.c_str());
    LE_DEBUG("IPv4 DNS secondary: %s", ipv4DnsSecondary_.c_str());
    LE_DEBUG("IPv4 address mask: %u", ipv4AddrMask_);
    LE_DEBUG("IPv4 gateway mask: %u", ipv4GatewayMask_);
    return LE_OK;
}
le_result_t TafDcsProfile::GetIPv6Addresses
(
    std::string &ipv6Addr,
    std::string &ipv6Gateway,
    std::string &ipv6DnsPrimary,
    std::string &ipv6DnsSecondary,
    unsigned int &ipv6AddrMask,
    unsigned int &ipv6GatewayMask
) const
{
    ipv6Addr         = ipv6Addr_;
    ipv6Gateway      = ipv6Gateway_;
    ipv6DnsPrimary   = ipv6DnsPrimary_;
    ipv6DnsSecondary = ipv6DnsSecondary_;
    ipv6AddrMask     = ipv6AddrMask_;
    ipv6GatewayMask  = ipv6GatewayMask_;
    LE_INFO("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_INFO("IPv6 address: %s", ipv6Addr_.c_str());
    LE_DEBUG("IPv6 gateway: %s", ipv6Gateway_.c_str());
    LE_DEBUG("IPv6 DNS primary: %s", ipv6DnsPrimary_.c_str());
    LE_DEBUG("IPv6 DNS secondary: %s", ipv6DnsSecondary_.c_str());
    LE_DEBUG("IPv6 address mask: %d", ipv6AddrMask_);
    LE_DEBUG("IPv6 gateway mask: %d", ipv6GatewayMask_);
    return LE_OK;
}

le_result_t TafDcsProfile::SetIPv4Addresses
(
    const std::string ipv4Addr,
    const std::string ipv4Gateway,
    const std::string ipv4DnsPrimary,
    const std::string ipv4DnsSecondary,
    const unsigned int ipv4AddrMask,
    const unsigned int ipv4GatewayMask
)
{
    ipv4Addr_         = ipv4Addr;
    ipv4Gateway_      = ipv4Gateway;
    ipv4DnsPrimary_   = ipv4DnsPrimary;
    ipv4DnsSecondary_ = ipv4DnsSecondary;
    ipv4AddrMask_     = ipv4AddrMask;
    ipv4GatewayMask_  = ipv4GatewayMask;
    LE_INFO("Phone Id: %d, Profile Id: %d, addr: %s", phoneId_, profileId_, ipv4Addr_.c_str());
    LE_INFO("IPv4 gateway: %s, DNS primary: %s, DNS secondary: %s", ipv4Gateway_.c_str(),
                                           ipv4DnsPrimary_.c_str(), ipv4DnsSecondary_.c_str());
    LE_INFO("IPv4 address mask: %u, gateway mask: %u", ipv4AddrMask_, ipv4GatewayMask_);
    return LE_OK;
}

void TafDcsProfile::ResetIPv4Addresses ()
{
    LE_INFO("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    ipv4Addr_.clear();
    ipv4Gateway_.clear();
    ipv4DnsPrimary_.clear();
    ipv4DnsSecondary_.clear();
    ipv4AddrMask_    = 0;
    ipv4GatewayMask_ = 0;
}

le_result_t TafDcsProfile::SetIPv6Addresses
(
    const std::string ipv6Addr,
    const std::string ipv6Gateway,
    const std::string ipv6DnsPrimary,
    const std::string ipv6DnsSecondary,
    const unsigned int ipv6AddrMask,
    const unsigned int ipv6GatewayMask
)
{
    ipv6Addr_         = ipv6Addr;
    ipv6Gateway_      = ipv6Gateway;
    ipv6DnsPrimary_   = ipv6DnsPrimary;
    ipv6DnsSecondary_ = ipv6DnsSecondary;
    ipv6AddrMask_     = ipv6AddrMask;
    ipv6GatewayMask_  = ipv6GatewayMask;
    LE_INFO("Phone Id: %d, Profile Id: %d, IPv6 address: %s", phoneId_, profileId_,
                                                                                ipv6Addr_.c_str());
    LE_INFO("IPv6 gateway: %s, DNS primary: %s, DNS secondary: %s", ipv6Gateway_.c_str(),
                                                ipv6DnsPrimary_.c_str(), ipv6DnsSecondary_.c_str());
    LE_INFO("IPv6 address mask: %d, gateway mask: %d", ipv6AddrMask_, ipv6GatewayMask_);
    return LE_OK;
}
void TafDcsProfile::ResetIPv6Addresses()
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    ipv6Addr_.clear();
    ipv6Gateway_.clear();
    ipv6DnsPrimary_.clear();
    ipv6DnsSecondary_.clear();
    ipv6AddrMask_    = 0;
    ipv6GatewayMask_ = 0;
}

taf_dcs_ProfileRef_t TafDcsProfile::GetReference() const
{

    LE_DEBUG("Reference: %p", reference_);
    return reference_;
};

le_result_t TafDcsProfile::SetId(const uint32_t &id)
{
    // Can be updated only if INVALID or UNDEFINED
    TAF_ERROR_IF_RET_VAL(
            TAF_DCS_INVALID_PROFILE_ID != profileId_ && TAF_DCS_UNDEFINED_PROFILE_ID != profileId_,
            LE_NOT_PERMITTED, "Profile ID is set already: %d", profileId_);
    profileId_ = id;
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    return LE_OK;
}

le_result_t TafDcsProfile::SetTech(const taf_dcs_Tech_t &tech)
{
    tech_ = tech;
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Tech : %d", TO_INT(tech_));
    return LE_OK;
}

le_result_t TafDcsProfile::SetPdp(const taf_dcs_Pdp_t &pdp)
{
    pdp_ = pdp;
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("PDP : %d", TO_INT(pdp_));
    return LE_OK;
}

le_result_t TafDcsProfile::SetAuthTypeBitmask(taf_dcs_Auth_t authTypeBitmask)
{
    authTypeBitmask_ = authTypeBitmask;
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Auth type bitmask: %d", TO_INT(authTypeBitmask_));
    return LE_OK;
}

le_result_t TafDcsProfile::SetApnTypeBitmask(taf_dcs_ApnType_t apnTypeBitmask)
{
    apnTypeBitmask_ = apnTypeBitmask;
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("APN type bitmask: %d", TO_INT(apnTypeBitmask_));
    return LE_OK;
}

le_result_t TafDcsProfile::SetEmergencyCallSupport(bool bEmerCallSupport)
{
    bEmergencyCallSupport_ = bEmerCallSupport;
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Emergency call supported: %s", bEmergencyCallSupport_ ? "true" : "false");
    return LE_OK;
}

/**
* @brief Sets or clears the custom network interface for the profile.
*/
void TafDcsProfile::SetCustomInterface(const std::string& name)
{
    if (name.empty()) {
        isInterfaceValid_ = false;
        interface_.clear();
    } else {
        isInterfaceValid_ = true;
        interface_ = name;
    }
}

/**
* @brief Retrieves the custom interface if valid.
*/
bool TafDcsProfile::GetCustomInterface(std::string& outInterface) const
{
    if (isInterfaceValid_) {
        outInterface = interface_;
        return true;
    }
    return false;
}

/**
 * @brief Convert connection state to string.
 *
 * TODO: Move to a common location.
 */
static inline const char *ToString(taf_dcs_ConState_t state)
{
    switch (state)
    {
    case TAF_DCS_DISCONNECTED:
        return "disconnected";
    case TAF_DCS_CONNECTING:
        return "connecting";
    case TAF_DCS_CONNECTED:
        return "connected";
    case TAF_DCS_DISCONNECTING:
        return "disconnecting";
    default:
        LE_WARN("unknown state: %d", state);
        return "unknown state";
    }
}

le_result_t TafDcsProfile::SetSessionState
(
    const taf_dcs_ConState_t state,
    const taf_dcs_ConState_t stateIPv4,
    const taf_dcs_ConState_t stateIPv6
)
{
    connState_     = state;
    connStateIPv4_ = stateIPv4;
    connStateIPv6_ = stateIPv6;
    LE_INFO("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_INFO("Conn state : %s (IPv4: %s, IPv6: %s)", ToString(connState_),
                                                            ToString(connStateIPv4_),
                                                            ToString(connStateIPv6_));
    return LE_OK;
}

le_result_t TafDcsProfile::GetCallEndReason
(
    taf_dcs_CallEndReasonType_t &type,
    int32_t                     &code,
    taf_dcs_CallEndReasonType_t &typeIPv4,
    taf_dcs_CallEndReasonType_t &typeIPv6
) const
{
    type     = callEndReasonType_;
    code     = callEndReasonCode_;
    typeIPv4 = callEndReasonTypeIPv4_;
    typeIPv6 = callEndReasonTypeIPv6_;
    LE_INFO("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_INFO("Reason     : %d, Code: %d (Reason IPv4: %d, Reason IPv6: %d)",
            TO_INT(callEndReasonType_), callEndReasonCode_,
            TO_INT(callEndReasonTypeIPv4_), TO_INT(callEndReasonTypeIPv6_));
    return LE_OK;
}

le_result_t TafDcsProfile::GetDataBearerTech(taf_dcs_DataBearerTechnology_t &tech) const
{
    tech = dataBearerTech_;
    LE_DEBUG("Phone Id: %d, Profile Id: %d, Tech: %d", phoneId_, profileId_,
                                                                          TO_INT(dataBearerTech_));
    return LE_OK;
}

le_result_t TafDcsProfile::SetDataBearerTech(const taf_dcs_DataBearerTechnology_t tech)
{
    dataBearerTech_  = tech;
    LE_DEBUG("Phone Id: %d, Profile Id: %d, Tech: %d", phoneId_, profileId_,
                                                                          TO_INT(dataBearerTech_));
    return LE_OK;
}

void TafDcsProfile::ResetDataBearerTech()
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    dataBearerTech_ = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;
}

void TafDcsProfile::ResetCallEndReason()
{
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    callEndReasonType_ = TAF_DCS_CE_TYPE_UNKNOWN;
    callEndReasonCode_ = 0;
    callEndReasonTypeIPv4_ = TAF_DCS_CE_TYPE_UNKNOWN;
    callEndReasonTypeIPv6_ = TAF_DCS_CE_TYPE_UNKNOWN;
}

le_result_t TafDcsProfile::SetCallEndReason
(
    const taf_dcs_CallEndReasonType_t type,
    const int32_t                     code,
    const taf_dcs_CallEndReasonType_t typeIPv4,
    const taf_dcs_CallEndReasonType_t typeIPv6
)
{
    callEndReasonType_     = type;
    callEndReasonCode_     = code;
    callEndReasonTypeIPv4_ = typeIPv4;
    callEndReasonTypeIPv6_ = typeIPv6;
    LE_INFO("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_INFO("Reason     : %d, Code: %d", TO_INT(callEndReasonType_), callEndReasonCode_);
    LE_INFO("Reason IPv4: %d, IPv6: %d", TO_INT(connStateIPv4_), TO_INT(connStateIPv6_));
    return LE_OK;
}

le_result_t TafDcsProfile::SetApn(const std::string &apn)
{
    apn_ = apn;
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("APN: %s", apn_.c_str());
    return LE_OK;
}

le_result_t TafDcsProfile::SetName(const std::string &name)
{
    name_ = name;
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Name: %s", name_.c_str());
    return LE_OK;
}

le_result_t TafDcsProfile::SetUserName(const std::string &userName)
{
    userName_ = userName;
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("User name: %s", userName_.c_str());
    return LE_OK;
}

le_result_t TafDcsProfile::SetPassword(const std::string &password)
{
    password_ = password;
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Password: %s", password.c_str());
    return LE_OK;
}

le_result_t TafDcsProfile::SetReference(taf_dcs_ProfileRef_t ref)
{
    reference_ = ref;
    LE_DEBUG("Phone Id: %d, Profile Id: %d", phoneId_, profileId_);
    LE_DEBUG("Reference: %p", reference_);
    return LE_OK;
}

const std::vector<taf_dcs_QosFlowRef_t>& TafDcsProfile::GetQosFlowRefs() const
{
    return qosFlowRefs_;
}

void TafDcsProfile::AddQosFlowRef(taf_dcs_QosFlowRef_t ref)
{
    // Ensure not to add duplicate tickets
    if (std::find(qosFlowRefs_.begin(), qosFlowRefs_.end(), ref) == qosFlowRefs_.end()) {
        qosFlowRefs_.push_back(ref);
    }
}

void TafDcsProfile::RemoveQosFlowRef(taf_dcs_QosFlowRef_t ref)
{
    // Return the ticket (remove from the profile's list)
    auto it = std::remove(qosFlowRefs_.begin(), qosFlowRefs_.end(), ref);
    if (it != qosFlowRefs_.end()) {
        qosFlowRefs_.erase(it, qosFlowRefs_.end());
    }
}

// This is a static function that returns the memory pool of the client session nodes.
le_mem_PoolRef_t TafDcsProfile::getClientSessionPool()
{
    // The static local variable is created only once even if it is called from multiple threads.
    static le_mem_PoolRef_t clientSessionPool =
        le_mem_CreatePool("DcsClientSessionPool", sizeof(TafDcsClientSession_t));
    return clientSessionPool;
}

TafDcsClientSession_t *TafDcsProfile::findClientSessionLocked(le_msg_SessionRef_t clientRef)
{
    le_dls_Link_t *linkPtr = le_dls_Peek(&clientSessionList_);
    while (NULL != linkPtr)
    {
        TafDcsClientSession_t *sessionPtr = CONTAINER_OF(linkPtr, TafDcsClientSession_t, link);
        if (sessionPtr->clientRef == clientRef)
        {
            return sessionPtr;
        }
        linkPtr = le_dls_PeekNext(&clientSessionList_, linkPtr);
    }
    return NULL;
}

le_result_t TafDcsProfile::addClientLocked(le_msg_SessionRef_t clientRef)
{
    if (NULL != findClientSessionLocked(clientRef))
    {
        LE_DEBUG("Client %p is already in the list", clientRef);
        return LE_DUPLICATE;
    }

    TafDcsClientSession_t *sessionPtr =
        static_cast<TafDcsClientSession_t *>(le_mem_ForceAlloc(getClientSessionPool()));
    sessionPtr->clientRef = clientRef;
    sessionPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&clientSessionList_, &sessionPtr->link);
    LE_DEBUG("Client %p is added to the list", clientRef);
    return LE_OK;
}

le_result_t TafDcsProfile::removeClientLocked(le_msg_SessionRef_t clientRef)
{
    TafDcsClientSession_t *sessionPtr = findClientSessionLocked(clientRef);
    if (NULL == sessionPtr)
    {
        LE_DEBUG("Client %p is not in the list", clientRef);
        return LE_NOT_FOUND;
    }

    le_dls_Remove(&clientSessionList_, &sessionPtr->link);
    le_mem_Release(sessionPtr);
    LE_DEBUG("Client %p is removed from the list", clientRef);
    return LE_OK;
}

le_result_t TafDcsProfile::AddClient(le_msg_SessionRef_t clientRef, size_t &listSize)
{
    LE_WARN_IF(0 == clientRef, "clientRef is 0");

    pthread_mutex_lock(&callCtxMutex_);
    le_result_t result = addClientLocked(clientRef);
    listSize = le_dls_NumLinks(&clientSessionList_);
    pthread_mutex_unlock(&callCtxMutex_);

    LE_INFO("Add client %p, result: %d, number of clients: %zu", clientRef, TO_INT(result),
                                                                                    listSize);
    return result;
}

le_result_t TafDcsProfile::RemoveClient(le_msg_SessionRef_t clientRef, size_t &listSize)
{
    LE_WARN_IF(0 == clientRef, "clientRef is 0");

    pthread_mutex_lock(&callCtxMutex_);
    le_result_t result = removeClientLocked(clientRef);
    listSize = le_dls_NumLinks(&clientSessionList_);
    pthread_mutex_unlock(&callCtxMutex_);

    LE_INFO("Remove client %p, result: %d, number of clients: %zu", clientRef, TO_INT(result),
                                                                                    listSize);
    return result;
}

bool TafDcsProfile::HasClientCalledSessionStart(le_msg_SessionRef_t clientRef)
{
    LE_WARN_IF(0 == clientRef, "clientRef is 0");

    pthread_mutex_lock(&callCtxMutex_);
    bool hasClient = (NULL != findClientSessionLocked(clientRef));
    pthread_mutex_unlock(&callCtxMutex_);

    LE_DEBUG("Client %p has requested data: %s", clientRef, (hasClient ? "true" : "false"));
    return hasClient;
}

size_t TafDcsProfile::GetNumClients()
{
    pthread_mutex_lock(&callCtxMutex_);
    size_t numClients = le_dls_NumLinks(&clientSessionList_);
    pthread_mutex_unlock(&callCtxMutex_);

    return numClients;
}

bool TafDcsProfile::IsCallActionInProgress()
{
    pthread_mutex_lock(&callCtxMutex_);
    bool inProgress = (TAF_DCS_CALL_ACTION_NONE != callAction_.type);
    pthread_mutex_unlock(&callCtxMutex_);

    return inProgress;
}

// Begin a call action. It must be called before the request is sent to PA, so the session state
// event from PA can not be missed.
void TafDcsProfile::BeginCallAction(const TafDcsCallAction_t &action)
{
    pthread_mutex_lock(&callCtxMutex_);
    callAction_ = action;
    pthread_mutex_unlock(&callCtxMutex_);

    LE_INFO("Phone Id: %d, Profile Id: %d, begin call action: %d, client: %p", phoneId_,
                                        profileId_, TO_INT(action.type), action.clientRef);
}

void TafDcsProfile::endCallActionLocked(le_result_t result)
{
    LE_INFO("Phone Id: %d, Profile Id: %d, end call action: %d, result: %d", phoneId_,
                                        profileId_, TO_INT(callAction_.type), TO_INT(result));
    callAction_ = {TAF_DCS_CALL_ACTION_NONE, nullptr, nullptr, nullptr};
    callActionResult_ = result;
    pthread_cond_broadcast(&callActionCond_);
}

// End the call action without a session state event from PA, e.g. the request is rejected by PA.
void TafDcsProfile::EndCallAction(le_result_t result)
{
    pthread_mutex_lock(&callCtxMutex_);
    endCallActionLocked(result);
    pthread_mutex_unlock(&callCtxMutex_);
}

static bool isStartCallAction(TafDcsCallActionType_t type)
{
    return (TAF_DCS_CALL_ACTION_SYNC_START == type) || (TAF_DCS_CALL_ACTION_ASYNC_START == type);
}

/**
 * Complete the call action with the result.
 * If the call action is failed, the client list is rolled back: the client of a start is removed
 * from the client list, and the client of a stop is added back to the client list.
 * Then the async handler is called and the sync waiter is woken up.
 */
void TafDcsProfile::completeCallActionLocked(le_result_t result)
{
    if (LE_OK != result)
    {
        if (isStartCallAction(callAction_.type))
        {
            removeClientLocked(callAction_.clientRef);
        }
        else
        {
            addClientLocked(callAction_.clientRef);
        }
    }

    // Call the handler with the lock held, so it can not be removed by RemoveAsyncHandler() at the
    // same time.
    if (NULL != callAction_.handlerPtr)
    {
        LE_INFO("Send result %d to client %p", TO_INT(result), callAction_.clientRef);
        callAction_.handlerPtr(reference_, result, callAction_.contextPtr);
    }

    endCallActionLocked(result);
}

/**
 * Finish the call action with the final session state from PA.
 * - A start which does not end in CONNECTED is failed with LE_TERMINATED.
 * - A stop which does not end in DISCONNECTED is failed with LE_FAULT.
 *
 * @return true if a call action was in progress, false otherwise.
 */
bool TafDcsProfile::FinishCallAction(taf_dcs_ConState_t connState)
{
    pthread_mutex_lock(&callCtxMutex_);

    if (TAF_DCS_CALL_ACTION_NONE == callAction_.type)
    {
        pthread_mutex_unlock(&callCtxMutex_);
        return false;
    }

    le_result_t result = LE_OK;
    if (isStartCallAction(callAction_.type))
    {
        if (TAF_DCS_CONNECTED != connState)
        {
            result = LE_TERMINATED;
        }
    }
    else if (TAF_DCS_DISCONNECTED != connState)
    {
        result = LE_FAULT;
    }

    completeCallActionLocked(result);
    pthread_mutex_unlock(&callCtxMutex_);
    return true;
}

/**
 * Fail the call action with LE_FAULT, as the start or stop request is failed by TelSDK.
 *
 * @return true if a start (isStart is true) or stop (isStart is false) call action was in
 *         progress, false otherwise.
 */
bool TafDcsProfile::FailCallAction(bool isStart)
{
    pthread_mutex_lock(&callCtxMutex_);

    if ((TAF_DCS_CALL_ACTION_NONE == callAction_.type) ||
        (isStart != isStartCallAction(callAction_.type)))
    {
        pthread_mutex_unlock(&callCtxMutex_);
        return false;
    }

    completeCallActionLocked(LE_FAULT);
    pthread_mutex_unlock(&callCtxMutex_);
    return true;
}

/**
 * Wait until the call action in progress is done.
 *
 * @return
 *  - The result of the call action.
 *  - LE_TIMEOUT if the call action is not done in time. The call action is ended if
 *    abandonOnTimeout is true, otherwise it is still in progress. When a stop is abandoned, its
 *    client is added back to the client list, as the data session may still be connected.
 */
le_result_t TafDcsProfile::WaitCallActionDone(uint32_t timeoutSec, bool abandonOnTimeout)
{
    struct timespec deadline;
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    deadline.tv_sec += timeoutSec;

    pthread_mutex_lock(&callCtxMutex_);

    int rc = 0;
    while ((TAF_DCS_CALL_ACTION_NONE != callAction_.type) && (ETIMEDOUT != rc))
    {
        rc = pthread_cond_timedwait(&callActionCond_, &callCtxMutex_, &deadline);
    }

    le_result_t result = callActionResult_;
    if (TAF_DCS_CALL_ACTION_NONE != callAction_.type)
    {
        LE_ERROR("Phone Id: %d, Profile Id: %d, call action %d timeout", phoneId_, profileId_,
                                                                        TO_INT(callAction_.type));
        if (abandonOnTimeout)
        {
            // The client of a stop is removed before the request, add it back so that the client
            // can retry the stop. The client of a start is kept, as the session may connect later.
            if (!isStartCallAction(callAction_.type))
            {
                addClientLocked(callAction_.clientRef);
            }
            endCallActionLocked(LE_TIMEOUT);
        }
        result = LE_TIMEOUT;
    }

    pthread_mutex_unlock(&callCtxMutex_);
    return result;
}

// Remove the async handler of a disconnected client, so the handler is not called any more.
void TafDcsProfile::RemoveAsyncHandler(le_msg_SessionRef_t clientRef)
{
    pthread_mutex_lock(&callCtxMutex_);
    if ((TAF_DCS_CALL_ACTION_NONE != callAction_.type) && (clientRef == callAction_.clientRef))
    {
        LE_INFO("Remove the async handler of client %p", clientRef);
        callAction_.handlerPtr = NULL;
        callAction_.contextPtr = NULL;
    }
    pthread_mutex_unlock(&callCtxMutex_);
}

le_event_Id_t TafDcsProfile::GetSessionStateChangedEventId()
{
    if (sessionStateChangedEventId_)
        return sessionStateChangedEventId_;

    // Create the event
    LE_DEBUG("Create session event ID");
    sessionStateChangedEventId_ = le_event_CreateId("sessionStateChangedEventId_",
                                                     sizeof(TafDcsSessionStateChangedEvent_t));
    return sessionStateChangedEventId_;
}

le_event_Id_t TafDcsProfile::GetRoamingStateChangedEventId()
{
    if (roamingStateChangedEventId_)
        return roamingStateChangedEventId_;

    // Create the event
    LE_DEBUG("Create roaming event ID");
    roamingStateChangedEventId_ = le_event_CreateId("roamingStateChangedEventId_",
                                                            sizeof(taf_dcs_RoamingStatusInd_t));
    return roamingStateChangedEventId_;
}

le_event_Id_t TafDcsProfile::GetQosStatusChangedEventId()
{
    if (qosStatusChangedEventId_)
        return qosStatusChangedEventId_;
    // Create the event
    LE_DEBUG("Create qos event ID");
    qosStatusChangedEventId_ = le_event_CreateId("qosStatusChangedEventId_",
                                                        sizeof(taf_dcs_QosTftEvent_t));
    return qosStatusChangedEventId_;
}

le_event_Id_t TafDcsProfile::GetHwAccelStateChangedEventId()
{
    if (hwAccelStateChangedEventId_)
        return hwAccelStateChangedEventId_;
    // Create the event
    LE_DEBUG("Create hwAccel event ID");
    hwAccelStateChangedEventId_ = le_event_CreateId("hwAccelStateChangedEventId_",
                                                        sizeof(taf_dcs_HwAccelerationEvent_t));
    return hwAccelStateChangedEventId_;
}

le_event_Id_t TafDcsProfile::GetThrottledStatusEventId()
{
    if (throttledStatusEventId_)
        return throttledStatusEventId_;
    // Create the event
    LE_DEBUG("Create throttled event ID");
    throttledStatusEventId_ = le_event_CreateId("throttledStatusEventId_",
                                                            sizeof(taf_dcs_ThrottledApnEvent_t));
    return throttledStatusEventId_;
}
