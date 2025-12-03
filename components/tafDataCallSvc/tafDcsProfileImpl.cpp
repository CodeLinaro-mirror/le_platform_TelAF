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

le_result_t TafDcsProfile::AddClient(le_msg_SessionRef_t clientRef, size_t &listSize)
{
    LE_WARN_IF (0 == clientRef, "clientRef is 0");
    // Insert after locking
    std::unique_lock lock(dataReqClientsSetMutex_);
    auto result = dataReqClients_.insert(clientRef);
    if (result.second)
        LE_DEBUG("Inserted %p", clientRef);
    else
        LE_DEBUG("Already in the set: %p", clientRef);

    // Update the size
    listSize = dataReqClients_.size();
    LE_INFO ("Number of clients: %zu", listSize);

    return LE_OK;
}

le_result_t TafDcsProfile::RemoveClient(le_msg_SessionRef_t clientRef, size_t &listSize)
{
    le_result_t result = LE_OK;
    LE_WARN_IF(0 == clientRef, "clientRef is 0");
    // Erase after locking
    std::unique_lock lock(dataReqClientsSetMutex_);
    if (dataReqClients_.erase(clientRef))
    {
        LE_INFO("Erased %p", clientRef);
    }
    else
    {
        LE_INFO("Not available in the set: %p", clientRef);
        result = LE_NOT_FOUND;
    }

    // Update the size
    listSize = dataReqClients_.size();
    LE_INFO ("Number of clients: %zu", listSize);

    return result;
}

bool TafDcsProfile::HasClientCalledSessionStart(le_msg_SessionRef_t clientRef)
{
    LE_WARN_IF(0 == clientRef, "clientRef is 0");

    // Check after locking
    std::shared_lock lock(dataReqClientsSetMutex_);
    auto client = dataReqClients_.find(clientRef);
    if (client != dataReqClients_.end())
    {
        LE_INFO("Client %p has requested data: true", clientRef);
        return true;
    }
    LE_DEBUG("Client %p has requested data: false", clientRef);
    return false;
}

// Async start session clients management.
bool TafDcsProfile::AddStartSessionAsyncClient
(
    le_msg_SessionRef_t client,
    taf_dcs_AsyncSessionHandlerFunc_t callback,
    void *context
)
{
    // Get a write lock
    std::unique_lock<std::shared_mutex> lock(startSessionAsyncRequestorsMapMutex_);
    auto result = startSessionAsyncRequestorsMap_.emplace(client,std::make_pair(callback, context));
    LE_INFO("Added client %p: %s", client, (result.second ? "true" : "false"));
    // Returns true if insertion was successful, false if client already exists
    return result.second;
}

std::vector<std::tuple<le_msg_SessionRef_t, taf_dcs_AsyncSessionHandlerFunc_t, void *>>
TafDcsProfile::GetStartSessionAsyncClients() const
{
    // Get a read lock
    std::shared_lock<std::shared_mutex> lock(startSessionAsyncRequestorsMapMutex_);

    std::vector<std::tuple<le_msg_SessionRef_t, taf_dcs_AsyncSessionHandlerFunc_t, void *>> entries;
    for (const auto &[client, pair] : startSessionAsyncRequestorsMap_)
    {
        LE_DEBUG("Add client %p", client);
        entries.emplace_back(client, pair.first, pair.second);
    }
    LE_INFO("Num entries: %zu", entries.size());
    return entries;
}

bool TafDcsProfile::RemoveStartSessionAsyncClient(le_msg_SessionRef_t client)
{
    //Get a read lock
    std::shared_lock<std::shared_mutex> lock(startSessionAsyncRequestorsMapMutex_);
    // Returns true if the entry was removed, false if the key was not found
    if (startSessionAsyncRequestorsMap_.erase(client))
    {
        LE_INFO("Client :%p was erased.", client);
        return true;
    }
    LE_DEBUG("Client :%p was not erased.", client);
    return false;
}

// Async stop session clients management.
bool TafDcsProfile::AddStopSessionAsyncClient
(
    le_msg_SessionRef_t client,
    taf_dcs_AsyncSessionHandlerFunc_t callback,
    void *context
)
{
    // Get a write lock
    std::unique_lock<std::shared_mutex> lock(stopSessionAsyncRequestorsMapMutex_);
    auto result = stopSessionAsyncRequestorsMap_.insert({client, {callback, context}});
    LE_INFO("Added client %p: %s", client, (result.second ? "true" : "false"));
    // Returns true if insertion was successful, false if client already exists
    return result.second;
}

std::vector<std::tuple<le_msg_SessionRef_t, taf_dcs_AsyncSessionHandlerFunc_t, void *>>
TafDcsProfile::GetStopSessionAsyncClients() const
{
    // Get a read lock
    std::shared_lock<std::shared_mutex> lock(stopSessionAsyncRequestorsMapMutex_);
    std::vector<std::tuple<le_msg_SessionRef_t, taf_dcs_AsyncSessionHandlerFunc_t, void *>> entries;
    for (const auto &[client, pair] : stopSessionAsyncRequestorsMap_)
    {
        LE_DEBUG("Add client %p", client);
        entries.emplace_back(client, pair.first, pair.second);
    }
    LE_INFO("Num entries: %zu", entries.size());
    return entries;
}

bool TafDcsProfile::RemoveStopSessionAsyncClient(le_msg_SessionRef_t client)
{
    //Get a read lock
    std::shared_lock<std::shared_mutex> lock(stopSessionAsyncRequestorsMapMutex_);
    // Returns true if the entry was removed, false if the key was not found
    if (stopSessionAsyncRequestorsMap_.erase(client))
    {
        LE_INFO("Client %p was erased.", client);
        return true;
    }
    LE_DEBUG("Client %p was not erased.", client);
    return false;
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
