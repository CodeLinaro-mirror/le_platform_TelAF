/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file tafDcsProfile.hpp
 * @brief TelAF Data Call Service's profile management class.
 *
 */

#ifndef _TAFDCS_PROFILE_HPP_
#define _TAFDCS_PROFILE_HPP_

#include "legato.h"
#include "interfaces.h"
#include "tafDataPa.hpp"

#include <map>
#include <string>
#include <utility>
#include <tuple>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <memory>
#include <stdexcept>
#include <set>
#include <future>
#include <atomic>
#include <chrono>

// For std::optional<std::reference_wrapper<T>>. Requires C++17
#include <optional>
#include <functional>
/***
 * Sample code for using std::optional<std::reference_wrapper<T>>:
 *  // Get the optional reference wrapper
 *  auto profile = getProfile(phoneId, profileId);
 *  // Check if the optional wrapper has a value
    TAF_ERROR_IF_RET_VAL((!profile.has_value()), nullptr, "getProfile failed.");
    return profile.value().get().GetReference();
 *
 */

namespace taf{
namespace svc{
namespace datacall{

/**
 * Internal structure to hold the list.
 */
struct TafDcsThroughputList_t
{
    std::vector<taf::pa::data::ThroughputInfo_t> infoList;

    // Track created ThroughputInfo refs, invalidate/cleanup on list delete
    std::vector<taf_dcs_ThroughputInfoRef_t> infoRefs;
    std::vector<taf::pa::data::ThroughputInfo_t*> infoObjs; // heap copies
};

// Structure to hold a single info entry (used for Reference)
struct TafDcsThroughputInfoEntry_t {
    taf::pa::data::ThroughputInfo_t info;
};

/**
 * The invalid profile ID
 */
const uint8_t TAF_DCS_INVALID_PROFILE_ID = 0;

// Profile ID and phone ID will uniquely identify a profile.
struct TafDcsProfile_t
{
    uint8_t phoneId;
    int32_t profileId;
};

/**
 * The internal data call state structure.
 */
struct TafDcsDataCallState_t
{
    taf_dcs_ConState_t  connState;
    taf_dcs_Pdp_t       ipType_pdp;
};

/**
 * For external API taf_dcs_AddSessionStateHandler().
 * Internal Event ID: sessionStateChangedEventId_
 */
struct TafDcsSessionStateChangedEvent_t
{
    taf_dcs_ProfileRef_t  profileRef;
    taf_dcs_ConState_t    connState;
    taf_dcs_Pdp_t         ipType;
};

/**
 * The internal DCS profile structure.
 * A profile will be uniquely identified by its profile ID and the phone ID.
 */
struct TafDcsProfileInfo_t
{
    uint32_t              id;                                        ///< The profile id.
    char                  apn[TAF_DCS_APN_NAME_MAX_BYTES];           ///< The access point name.
    char                  name[TAF_DCS_NAME_MAX_BYTES];              ///< The profile name.
    char                  userName[TAF_DCS_USER_NAME_MAX_BYTES];     ///< The user name.
    char                  password[TAF_DCS_PASSWORD_NAME_MAX_BYTES]; ///< The password.
    taf_dcs_Tech_t        techPref;                   ///< The technology preference.
    taf_dcs_Auth_t        authTypeBitmask;            ///< The authentication type.
    taf_dcs_Pdp_t         ipType;                     ///< The IP type.
    taf_dcs_ApnType_t     apnTypeMask;                ///< The APN type bitmask.
    bool                  emergencyCallSupport;       ///< Emergency call support.
};

/**
 * The internal DCS profile structure.
 * A profile will be uniquely identified by its profile ID and the phone ID.
 */
struct TafDcsUpdateProfileEvent_t
{
    uint8_t             slotId;                       ///< The slot id.
    TafDcsProfileInfo_t profileInfo;                  ///< The profile info.
};

/**
 * The structure for sessionStartEvtId_.
 * A profile will be uniquely identified by its profile ID and the phone ID.
 */
struct TafDcsSessionStartEvent_t
{
    TafDcsProfile_t profile; ///< The profile.
};

/**
 * The structure for sessionStopEvtId_.
 * A profile will be uniquely identified by its profile ID and the phone ID.
 */
struct TafDcsSessionStopEvent_t
{
    TafDcsProfile_t profile;    ///< The profile.
    taf_dcs_Pdp_t   ipType_pdp; ///< The IP type
};

/**
 * The structure for paSessionStateChangeEvtId_.
 */
struct TafDcsSessionChangeEvent_t
{
    TafDcsProfile_t                profile;           ///< The profile.
    taf_dcs_ConState_t             connState;         ///< The connection state.
    taf_dcs_Pdp_t                  ipType_pdp;        ///< The IP type.
    taf_dcs_Tech_t                 techPreference;    ///< The technology preference.
    taf_dcs_DataBearerTechnology_t bearerTech;        ///< The bearer technology.
    taf_dcs_CallEndReasonType_t    callEndReasonType; ///< The call end reason type.
    int32_t                        callEndReasonCode; ///< The call end reason code.
    uint64_t                       maxRxBitRate;      ///< The max rx data rate in bits/second.
    uint64_t                       maxTxBitRate;      ///< The max tx data rate in bits/second.
    char hostIfName[TAF_DCS_APN_NAME_MAX_BYTES]="";

    // IPv4 call details
    taf_dcs_ConState_t          ipv4ConnState; ///< The connection state.
    taf_dcs_CallEndReasonType_t ipv4CallEndReasonType;
    char ipv4Addr[TAF_DCS_IPV4_ADDR_MAX_LEN] = "";
    unsigned int ipv4AddrMask;
    char ipv4gwAddr[TAF_DCS_IPV4_ADDR_MAX_LEN] = "";
    unsigned int ipv4gwAddrMask;
    char ipv4PrimaryDnsAddr[TAF_DCS_IPV4_ADDR_MAX_LEN] = "";
    char ipv4SecondaryDnsAddr[TAF_DCS_IPV4_ADDR_MAX_LEN] = "";

    // IPv6 call details
    taf_dcs_ConState_t ipv6ConnState; ///< The connection state.
    taf_dcs_CallEndReasonType_t ipv6CallEndReasonType;
    char ipv6Addr[TAF_DCS_IPV6_ADDR_MAX_LEN] = "";
    unsigned int ipv6AddrMask;
    char ipv6gwAddr[TAF_DCS_IPV6_ADDR_MAX_LEN] = "";
    unsigned int ipv6gwAddrMask;
    char ipv6PrimaryDnsAddr[TAF_DCS_IPV6_ADDR_MAX_LEN] = "";
    char ipv6SecondaryDnsAddr[TAF_DCS_IPV6_ADDR_MAX_LEN] = "";
};

/**
 * The structure for paRoamingChangeEvtId_.
 */
struct TafDcsRoamingStatus_t
{
    uint8_t               phoneId;    ///< The phone id.
    bool                  isRoaming;  ///< The roaming status.
    taf_dcs_RoamingType_t type;       ///< The roaming type. Valid only if isRoaming is true.
};

/**
 * The structure for paThrottledAPNsEvtId_.
 */
struct TafDcsThrottledApnEventInfo_t
{
    uint8_t  phoneId;                     ///< The phone id.
    uint32_t profileId;                   ///< The profile id.
    uint32_t ipv4Time;                    ///< Remaining IPv4 throttled time in milliseconds.
    uint32_t ipv6Time;                    ///< Remaining IPv6 throttled time in milliseconds.
    bool isBlockedOnAllPLMNs;             ///< Is APN blocked on all plmns.
    char apn[TAF_DCS_APN_NAME_MAX_BYTES]; ///< APN.
    char mcc[TAF_DCS_MCC_BYTES];          ///< Mobile Country Code.
    char mnc[TAF_DCS_MNC_BYTES];          ///< Mobile Network Code.
};

/**
 * The structure for taf_dcs_ThrottledStatusHandlerFunc_t.
 * This is an internal structure as such an external struct is not defined.
 */
struct taf_dcs_ThrottledApnEvent_t
{
    taf_dcs_ProfileRef_t profileRef;      ///< The profile reference.
    bool isthrottled;                     ///< Is APN throttled.
    uint32_t ipv4Time;                    ///< Remaining IPv4 throttled time in milliseconds.
    uint32_t ipv6Time;                    ///< Remaining IPv6 throttled time in milliseconds.

};

/**
 * The structure for paHwAccelerationChangeEvtId_.
 */
struct TafDcsHwAccelerationChangeEvent_t
{
    uint8_t                      phoneId; ///< The phone id.
    taf_dcs_HwAccelerationState_t state;  ///< The HW acceleration state.
};

/**
 * The structure for taf_dcs_HwAccelerationStateHandlerFunc_t.
 * This is an internal structure as such an external struct is not defined.
 */
struct taf_dcs_HwAccelerationEvent_t
{
    taf_dcs_ProfileRef_t          profileRef; ///< The profile reference.
    taf_dcs_HwAccelerationState_t state;      ///< The HW acceleration state.
};

/**
 * The structure for taf_dcs_ThroughputInfoHandlerFunc_t.
 * This is an internal structure as such an external struct is not defined.
 */
struct TafDcsThroughputInfoChangeEvent_t
{
    uint8_t phoneId;
    taf_dcs_ThroughputInfoInd_t ind;
};

/**
 * The structure for paQosTftEvtId_.
 */
struct TafDcsQosTftEventInfo_t
{
    uint8_t phoneId;                    ///< The phone id.
    uint32_t profileId;                 ///< The profile id.
    uint32_t qosFlowId;                 ///< The QoS flow id.
    taf_dcs_QosFlowState_t state;       ///< The QoS flow state.
    taf_dcs_QosFlowBitMask_t paramMask; ///< The mask to check for valid parameters in the flow.
};

/**
 * The structure for taf_dcs_QosStatusHandlerFunc_t.
 * This is an internal structure as such an external struct is not defined.
 */
struct taf_dcs_QosTftEvent_t
{
    taf_dcs_QosFlowRef_t qosFlowRef; ///< The QOS flow reference.
    taf_dcs_QosFlowState_t qosState; ///< QOS state.
};

/**
 * The DCS profile class.
 * A profile will be uniquely identified by its profile ID and the phone ID.
 */
class TafDcsProfile
{
public:

    // Block copying so that thisPtr_ will not become invalid
    TafDcsProfile(const TafDcsProfile &) = delete;
    TafDcsProfile &operator=(const TafDcsProfile &) = delete;
    // Block moving so that thisPtr_ will not become invalid
    TafDcsProfile(TafDcsProfile &&) = delete;
    TafDcsProfile &operator=(TafDcsProfile &&) = delete;

    // Constructor. The slot Id and profileId in TafDcsProfileInfo_t will not be considered.
    TafDcsProfile(uint8_t slotId, uint8_t phoneId, const TafDcsProfileInfo_t &profileInfo);
    ~TafDcsProfile();

    // Getters
    bool        GetCallSetup() const;
    le_result_t GetId(uint32_t &id) const;
    le_result_t GetSlotId(uint8_t &id) const;
    le_result_t GetPhoneId(uint8_t &id) const;
    le_result_t GetTech(taf_dcs_Tech_t &tech) const;
    le_result_t GetPdp(taf_dcs_Pdp_t &pdp) const;
    le_result_t GetAuthTypeBitmask(taf_dcs_Auth_t &authTypeBitmask) const;
    le_result_t GetApnTypeBitmask(taf_dcs_ApnType_t &apnTypeBitmask) const;
    le_result_t GetEmergencyCallSupport(bool &bEmerCallSupport) const;
    le_result_t GetMaxDataBitRates(uint64_t &rxRate, uint64_t &txRate) const;
    // The internal status
    le_result_t GetSessionState
    (
        taf_dcs_ConState_t &state,
        taf_dcs_ConState_t &stateIPv4,
        taf_dcs_ConState_t &stateIPv6
    ) const;
    le_result_t GetCallEndReason
    (
        taf_dcs_CallEndReasonType_t &type,
        int32_t                     &code,
        taf_dcs_CallEndReasonType_t &typeIPv4,
        taf_dcs_CallEndReasonType_t &typeIPv6
    ) const;
    le_result_t GetIPv4Addresses
    (
        std::string &ipv4Addr,
        std::string &ipv4Gateway,
        std::string &ipv4DnsPrimary,
        std::string &ipv4DnsSecondary,
        unsigned int &ipv4AddrMask,
        unsigned int &ipv4GatewayMask
    ) const;
    le_result_t GetIPv6Addresses
    (
        std::string &ipv6Addr,
        std::string &ipv6Gateway,
        std::string &ipv6DnsPrimary,
        std::string &ipv6DnsSecondary,
        unsigned int &ipv6AddrMask,
        unsigned int &ipv6GatewayMask
    ) const;
    le_result_t GetDataBearerTech(taf_dcs_DataBearerTechnology_t &tech) const;
    le_result_t GetApn(std::string &apn) const;
    le_result_t GetName(std::string &name) const;
    le_result_t GetUserName(std::string &name) const;
    le_result_t GetPassword(std::string &name) const;
    le_result_t GetHostInterface(std::string &ifName) const;
    taf_dcs_ProfileRef_t GetReference() const;

    // Setters
    void        SetCallSetup(const bool);
    le_result_t SetApn(const std::string &apn);
    le_result_t SetApnTypeBitmask(taf_dcs_ApnType_t apnTypeBitmask);
    le_result_t SetAuthTypeBitmask(taf_dcs_Auth_t authTypeBitmask);
    le_result_t SetEmergencyCallSupport(bool bEmerCallSupport);
    le_result_t SetId(const uint32_t &id);
    le_result_t SetMaxDataBitRates(const uint64_t &rxRate, const uint64_t &txRate);
    le_result_t SetName(const std::string &name);
    le_result_t SetPassword(const std::string &name);
    le_result_t SetHostInterface(const std::string &ifName);
    void        ResetHostInterface();
    le_result_t SetPdp(const taf_dcs_Pdp_t &pdp); // IP Type
    le_result_t SetReference(const taf_dcs_ProfileRef_t ref);
    le_result_t SetSessionState
    (
        const taf_dcs_ConState_t state,
        const taf_dcs_ConState_t stateIPv4,
        const taf_dcs_ConState_t stateIPv6
    );
    le_result_t SetCallEndReason
    (
        const taf_dcs_CallEndReasonType_t type,
        const int32_t code,
        const taf_dcs_CallEndReasonType_t typeIPv4,
        const taf_dcs_CallEndReasonType_t typeIPv6
    );
    void ResetCallEndReason();
    le_result_t SetIPv4Addresses
    (
        const std::string ipv4Addr,
        const std::string ipv4Gateway,
        const std::string ipv4DnsPrimary,
        const std::string ipv4DnsSecondary,
        const unsigned int ipv4AddrMask,
        const unsigned int ipv4GatewayMask
    );
    void ResetIPv4Addresses();
    le_result_t SetIPv6Addresses
    (
        const std::string ipv6Addr,
        const std::string ipv6Gateway,
        const std::string ipv6DnsPrimary,
        const std::string ipv6DnsSecondary,
        const unsigned int ipv6AddrMask,
        const unsigned int ipv6GatewayMask
    );
    void ResetIPv6Addresses();
    le_result_t SetDataBearerTech(const taf_dcs_DataBearerTechnology_t tech);
    void        ResetDataBearerTech();
    le_result_t SetTech(const taf_dcs_Tech_t &tech);
    le_result_t SetUserName(const std::string &name);
    le_result_t AddClient(le_msg_SessionRef_t clientRef, size_t &listSize);
    le_result_t RemoveClient(le_msg_SessionRef_t clientRef, size_t &listSize);
    bool        HasClientCalledSessionStart(le_msg_SessionRef_t clientRef);

    // Helper function to populate the profile info struct.
    void PopulateProfileInfoStruct(taf::pa::data::ProfileInfo_t &profileInfo) const;

    // Async start session clients management.
    bool AddStartSessionAsyncClient
    (
        le_msg_SessionRef_t client,
        taf_dcs_AsyncSessionHandlerFunc_t callback,
        void *context
    );
    std::vector<std::tuple<le_msg_SessionRef_t, taf_dcs_AsyncSessionHandlerFunc_t, void *>>
                                                                GetStartSessionAsyncClients() const;
    bool RemoveStartSessionAsyncClient(le_msg_SessionRef_t client);

    // Async stop session clients management.
    bool AddStopSessionAsyncClient
    (
        le_msg_SessionRef_t client,
        taf_dcs_AsyncSessionHandlerFunc_t callback,
        void *context
    );
    std::vector<std::tuple<le_msg_SessionRef_t, taf_dcs_AsyncSessionHandlerFunc_t, void *>>
                                                                GetStopSessionAsyncClients() const;
    bool RemoveStopSessionAsyncClient(le_msg_SessionRef_t client);

    le_event_Id_t GetSessionStateChangedEventId();
    le_event_Id_t GetQosStatusChangedEventId();
    le_event_Id_t GetThrottledStatusEventId();
    // These events are not profile specific and cannot be accessed via the profile object
    le_event_Id_t GetHwAccelStateChangedEventId();
    static le_event_Id_t GetRoamingStateChangedEventId();

private:
    // static getter to get the profiler reference map.
    static le_ref_MapRef_t getProfileRefMap();

    taf_dcs_ProfileRef_t reference_ = nullptr;
    const uint8_t     slotId_    = 0; ///< Slot ID; Immutable after creation.
    const uint8_t     phoneId_   = 0; ///< TAF_DCS_PHONE_ID_UNKNOWN;
    uint32_t          profileId_ = 0; ///< TAF_DCS_INVALID_PROFILE_ID;
                                      ///< Can be updated only if INVALID or UNDEFINED
    taf_dcs_Tech_t    tech_      = TAF_DCS_TECH_UNKNOWN;
    taf_dcs_Pdp_t     pdp_       = TAF_DCS_PDP_UNKNOWN;
    taf_dcs_Auth_t    authTypeBitmask_ = TAF_DCS_AUTH_NONE;        ///< The authentication type.
    taf_dcs_ApnType_t apnTypeBitmask_  = TAF_DCS_APN_TYPE_UNSPECIFIED; ///< The APN type bitmask.
    bool              bEmergencyCallSupport_ = false;              ///< Emergency call support.

    // Bit rates
    uint64_t maxRxBitRate_; ///< The max rx data rate in bits/second.
    uint64_t maxTxBitRate_; ///< The max tx data rate in bits/second.

    // External state.
    taf_dcs_ConState_t          connState_         = TAF_DCS_DISCONNECTED;
    taf_dcs_CallEndReasonType_t callEndReasonType_ = TAF_DCS_CE_TYPE_UNKNOWN;
    int32_t                     callEndReasonCode_ = 0;

    // Internal status.
    bool                        bCallSetup_            = false;
    taf_dcs_ConState_t          connStateIPv4_         = TAF_DCS_DISCONNECTED;
    taf_dcs_CallEndReasonType_t callEndReasonTypeIPv4_ = TAF_DCS_CE_TYPE_UNKNOWN;
    taf_dcs_ConState_t          connStateIPv6_         = TAF_DCS_DISCONNECTED;
    taf_dcs_CallEndReasonType_t callEndReasonTypeIPv6_ = TAF_DCS_CE_TYPE_UNKNOWN;

    // IP V4 addresses
    std::string  ipv4Addr_;
    std::string  ipv4Gateway_;
    std::string  ipv4DnsPrimary_;
    std::string  ipv4DnsSecondary_;
    unsigned int ipv4AddrMask_;
    unsigned int ipv4GatewayMask_;

    // IP V6 addresses
    std::string  ipv6Addr_;
    std::string  ipv6Gateway_;
    std::string  ipv6DnsPrimary_;
    std::string  ipv6DnsSecondary_;
    unsigned int ipv6AddrMask_;
    unsigned int ipv6GatewayMask_;

    taf_dcs_DataBearerTechnology_t dataBearerTech_ = TAF_DCS_DATA_BEARER_TECHNOLOGY_UNKNOWN;

    std::string   apn_;
    std::string   name_;
    std::string   userName_;
    std::string   password_;
    std::string   hostIfName_;
    // A raw pointer to this object. This is used only for "taf_dcs_ProfileRef_t".
    TafDcsProfile *thisPtr_;

    // The set of all clients who have requested data with this profile.
    std::set<le_msg_SessionRef_t> dataReqClients_;
    std::shared_mutex             dataReqClientsSetMutex_;

    // List of clients who have called taf_dcs_StartSessionAsync() for this profile.
    std::map<le_msg_SessionRef_t, std::pair<taf_dcs_AsyncSessionHandlerFunc_t, void *>>
        startSessionAsyncRequestorsMap_;
    mutable std::shared_mutex startSessionAsyncRequestorsMapMutex_;

    // List of clients who have called taf_dcs_StopSessionAsync() for this profile.
    std::map<le_msg_SessionRef_t, std::pair<taf_dcs_AsyncSessionHandlerFunc_t, void *>>
                                                                    stopSessionAsyncRequestorsMap_;
    mutable std::shared_mutex stopSessionAsyncRequestorsMapMutex_;

    // The map from which profile references are created. The "inline" keyword is needed to init the
    // variable to a nullptr and requires c++17.
    inline static le_ref_MapRef_t profileRefsMap_ = nullptr;

    // Variables for data session state handler
    le_event_Id_t sessionStateChangedEventId_ = nullptr;

    // Variables for QoS status changed handler
    le_event_Id_t qosStatusChangedEventId_ = nullptr;

    // Variables for HW acceleration state changed handler
    le_event_Id_t hwAccelStateChangedEventId_ = nullptr;

    // Variables for throttled status  handler
    le_event_Id_t throttledStatusEventId_ = nullptr;

    // Variables for roaming status handler. This is not profile specific, so is static
    inline static le_event_Id_t roamingStateChangedEventId_ = nullptr;
};

/**
 * A singleton class that manages objects of TafDcsProfile
 */
class TafDcsProfileManager
{
public:
    // Delete copy constructor and assignment operator to prevent copying
    // of singleton instance.
    TafDcsProfileManager(const TafDcsProfileManager &) = delete;
    TafDcsProfileManager &operator=(const TafDcsProfileManager &) = delete;

    // Get the singleton instance of TafDcsProfileManager.
    static TafDcsProfileManager &GetInstance();

    // Initialization function
    void Init();
    void InitProfiles();
    // Deinitialization function
    void Deinit();

    // DCS service's APIs implementation.
    taf_dcs_ProfileRef_t SvcGetProfileRef(uint8_t phoneId, uint32_t profileId);
    le_result_t SvcGetProfilesList
    (
        uint8_t phoneId,
        taf_dcs_ProfileInfo_t *profilesListPtr,
        size_t *profilesListSizePtr
    );
    le_result_t SvcGetProfileId(taf_dcs_ProfileRef_t, uint32_t *profileIdPtr);
    le_result_t SvcGetPhoneId(taf_dcs_ProfileRef_t, uint8_t *phoneIdPtr);

    le_result_t SvcCreateProfile(taf_dcs_ProfileRef_t profileRef);
    le_result_t SvcDeleteProfile(taf_dcs_ProfileRef_t profileRef);

    le_result_t SvcGetAPN(taf_dcs_ProfileRef_t, char *apnName, size_t apnNameSize);
    le_result_t SvcGetProfileName(taf_dcs_ProfileRef_t, char *name, size_t nameSize);
    le_result_t SvcGetTechPreference(taf_dcs_ProfileRef_t profileRef,taf_dcs_Tech_t *techPrefPtr);
    le_result_t SvcGetApnTypes(taf_dcs_ProfileRef_t profileRef,taf_dcs_ApnType_t *apnTypePtr);
    le_result_t SvcGetPDP(taf_dcs_ProfileRef_t, taf_dcs_Pdp_t &pdp);
    le_result_t SvcGetRoamingStatus
    (
        uint8_t phoneId,
        bool *isRoamingPtr,
        taf_dcs_RoamingType_t *typePtr
    );

    le_result_t SvcSetAPN(taf_dcs_ProfileRef_t profileRef, const char *apnStr);
    le_result_t SvcSetApnTypes( taf_dcs_ProfileRef_t profileRef, taf_dcs_ApnType_t apnType);
    le_result_t SvcSetProfileName(taf_dcs_ProfileRef_t profileRef, const char *nameStr);
    le_result_t SvcSetTechPreference(taf_dcs_ProfileRef_t profileRef,taf_dcs_Tech_t techPref);
    le_result_t SvcSetPDP(taf_dcs_ProfileRef_t profileRef,taf_dcs_Pdp_t pdp);
    le_result_t SvcSetAuthentication(
        taf_dcs_ProfileRef_t profileRef,
        taf_dcs_Auth_t auth,
        const char *userName,
        const char *password);

    le_result_t SvcStartSessionSync(taf_dcs_ProfileRef_t, le_msg_SessionRef_t clientRef);
    void SvcStartSessionASync
    (
        taf_dcs_ProfileRef_t profileRef,
        taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
        void* contextPtr,
        le_msg_SessionRef_t clientRef
    );
    le_result_t SvcStopSessionSync(taf_dcs_ProfileRef_t, le_msg_SessionRef_t clientRef);
    void SvcStopSessionASync
    (
        taf_dcs_ProfileRef_t profileRef,
        taf_dcs_AsyncSessionHandlerFunc_t handlerPtr,
        void* contextPtr,
        le_msg_SessionRef_t clientRef
    );
    le_result_t SvcGetSessionState(taf_dcs_ProfileRef_t,taf_dcs_ConState_t *connStatePtr);
    le_result_t SvcGetDefaultProfileIndexEx (uint8_t phoneId, uint32_t *profileIdPtr);
    le_result_t SvcSetDefaultProfileIndexEx (uint8_t phoneId, uint32_t profileId);
    le_result_t SvcGetDefaultPhoneIdAndProfileId(uint8_t *phoneIdPtr,uint32_t *profileIdPtr);
    le_result_t SvcGetPhoneIdByInterfaceName(const char *LE_NONNULL ifName, uint8_t *phoneIdPtr);
    le_result_t SvcGetProfileIdByInterfaceName(const char *LE_NONNULL ifName,
                                                                            uint32_t *profileIdPtr);
    le_result_t SvcGetMtu(taf_dcs_ProfileRef_t profileRef, uint16_t *mtuPtr);
    bool        SvcIsIPv4(taf_dcs_ProfileRef_t profileRef);
    bool        SvcIsIPv6(taf_dcs_ProfileRef_t profileRef);
    le_result_t SvcGetIPv4Address(taf_dcs_ProfileRef_t, char *ipAddr, size_t ipAddrSize);
    le_result_t SvcGetIPv4SubnetMask(taf_dcs_ProfileRef_t profileRef,uint32_t *maskPtr);
    le_result_t SvcGetIPv6Address(taf_dcs_ProfileRef_t, char *ipAddr, size_t ipAddrSize);
    le_result_t SvcGetIPv6SubnetMask(taf_dcs_ProfileRef_t profileRef, uint32_t *maskPtr);
    le_result_t SvcGetIPv4DNSAddresses
    (
        taf_dcs_ProfileRef_t profileRef,
        char *dns1AddrStr,
        size_t dns1AddrStrSize,
        char *dns2AddrStr,
        size_t dns2AddrStrSize
    );
    le_result_t SvcGetIPv6DNSAddresses
    (
        taf_dcs_ProfileRef_t profileRef,
        char *dns1AddrStr,
        size_t dns1AddrStrSize,
        char *dns2AddrStr,
        size_t dns2AddrStrSize
    );
    le_result_t SvcGetIPv4GatewayAddress
    (
        taf_dcs_ProfileRef_t profileRef,
        char *gatewayAddr,
        size_t gatewayAddrSize
    );
    le_result_t SvcGetIPv6GatewayAddress
    (
        taf_dcs_ProfileRef_t profileRef,
        char *gatewayAddr,
        size_t gatewayAddrSize
    );
    le_result_t SvcGetAuthentication
    (
        taf_dcs_ProfileRef_t profileRef,
        taf_dcs_Auth_t *authPtr,
        char *userName,
        size_t userNameSize,
        char *password,
        size_t passwordSize
    );
    le_result_t SvcGetInterfaceName
    (
        taf_dcs_ProfileRef_t profileRef,
        char *ifNameStr,
        size_t ifNameStrSize
    );
    le_result_t SvcGetDataBearerTechnology
    (
        taf_dcs_ProfileRef_t profileRef,
        taf_dcs_DataBearerTechnology_t *dlDataBearerTechPtrPtr,
        taf_dcs_DataBearerTechnology_t *ulDataBearerTechPtrPtr
    );
    le_result_t SvcGetCallEndReason
    (
        taf_dcs_ProfileRef_t profileRef,
        taf_dcs_Pdp_t pdpType,
        taf_dcs_CallEndReasonType_t *callEndReasonTypePtr,
        int32_t *callEndReasonCodePtr
    );
    le_result_t SvcGetMaxDataBitRates
    (
        taf_dcs_ProfileRef_t profileRef,
        uint64_t *maxRxBitRatePtr,
        uint64_t *maxTxBitRatePtr
    );
    le_result_t SvcGetAPNThrottledStatus
    (
        taf_dcs_ProfileRef_t profileRef,
        bool *isThrottledPtr,
        uint32_t *ipv4RemainingTimePtr,
        uint32_t *ipv6RemainingTimePtr
    );
    le_result_t SvcGetAPNThrottledPLMN
    (
        taf_dcs_ProfileRef_t profileRef,
        bool *areAllPLMNsThrottledPtr,
        char *mcc,
        size_t mccSize,
        char *mnc,
        size_t mncSize
    );

    // External event handlers
    taf_dcs_RoamingStatusHandlerRef_t SvcAddRoamingStatusHandler
    (
        taf_dcs_ProfileRef_t profileRef,
        taf_dcs_RoamingStatusHandlerFunc_t handlerPtr,
        void *contextPtr
    );
    void SvcRemoveRoamingStatusHandler(taf_dcs_RoamingStatusHandlerRef_t handlerRef);

    taf_dcs_SessionStateHandlerRef_t SvcAddSessionStateHandler
    (
        taf_dcs_ProfileRef_t profileRef,
        taf_dcs_SessionStateHandlerFunc_t handlerPtr,
        void *contextPtr
    );
    void SvcRemoveSessionStateHandler(taf_dcs_SessionStateHandlerRef_t handlerRef);

    taf_dcs_QosStatusHandlerRef_t SvcAddQosStatusHandler
    (
        taf_dcs_ProfileRef_t profileRef,
        taf_dcs_QosStatusHandlerFunc_t handlerPtr,
        void *contextPtr
    );
    void SvcRemoveQosStatusHandler(taf_dcs_QosStatusHandlerRef_t handlerRef);

    taf_dcs_HwAccelerationStateHandlerRef_t SvcAddHwAccelerationStateHandler
    (
        taf_dcs_ProfileRef_t profileRef,
        taf_dcs_HwAccelerationStateHandlerFunc_t handlerPtr,
        void *contextPtr
    );
    void SvcRemoveHwAccelerationStateHandler(taf_dcs_HwAccelerationStateHandlerRef_t handlerRef);

    taf_dcs_ThrottledStatusHandlerRef_t SvcAddThrottledStatusHandler(
        taf_dcs_ProfileRef_t profileRef,
        taf_dcs_ThrottledStatusHandlerFunc_t handlerPtr,
        void *contextPtr);
    void SvcRemoveThrottledStatusHandler(taf_dcs_ThrottledStatusHandlerRef_t handlerRef);

    // Internal APIs.
    le_result_t GetPhones(std::vector<taf::pa::data::PhoneId_e>&) const;
    // Function to find all TafDcsProfile entries for a specific Phone ID
    std::vector<std::shared_ptr<TafDcsProfile>> FindProfilesByPhoneId(uint8_t phoneId);

    // Throughput API Implementations
    le_result_t SvcSetThroughputReport(uint8_t phoneId, taf_dcs_LinkDirection_t direction, bool enabled, uint32_t interval);
    le_result_t SvcGetLastThroughputInfoList(uint8_t phoneId, taf_dcs_ThroughputInfoListRef_t* listRefPtr);
    le_result_t SvcDeleteLastThroughputInfoList(taf_dcs_ThroughputInfoListRef_t listRef);
    le_result_t SvcGetThroughputInfoCount(taf_dcs_ThroughputInfoListRef_t listRef, uint32_t* countPtr);
    le_result_t SvcGetThroughputInfo(taf_dcs_ThroughputInfoListRef_t listRef, uint32_t index, taf_dcs_ThroughputInfoRef_t* infoRefPtr);

    // Throughput Info Getters
    le_result_t SvcGetThroughputApnName(taf_dcs_ThroughputInfoRef_t infoRef, char* name, size_t nameSize);
    le_result_t SvcGetThroughputActualRate(taf_dcs_ThroughputInfoRef_t infoRef, taf_dcs_LinkDirection_t direction, uint32_t* ratePtr);
    le_result_t SvcGetThroughputAllowedRate(taf_dcs_ThroughputInfoRef_t infoRef, taf_dcs_LinkDirection_t direction, uint32_t* ratePtr);
    le_result_t SvcGetThroughputQueueSize(taf_dcs_ThroughputInfoRef_t infoRef, taf_dcs_LinkDirection_t direction, uint32_t* sizePtr);
    le_result_t SvcGetThroughputQuality(taf_dcs_ThroughputInfoRef_t infoRef, taf_dcs_LinkDirection_t direction, taf_dcs_ThroughputQuality_t* qualityPtr);

    // Client-facing throughput indication registration
    taf_dcs_ThroughputInfoChangeHandlerRef_t SvcAddThroughputInfoChangeHandler(
        uint8_t phoneId,
        taf_dcs_ThroughputInfoHandlerFunc_t handlerPtr,
        void *contextPtr);

    void SvcRemoveThroughputInfoChangeHandler(
        taf_dcs_ThroughputInfoChangeHandlerRef_t handlerRef);

private:
    /**
     * Private functions.
     */

    // Initialize internal events and memory
    void
    registerInternalEventCallbacks();
    void deinitEventsAndMemory();
    // Initialize profiles during startup. This will be done asynchronously in the background.
    void initProfiles();
    void deinitProfiles();
    void updateDefaultProfiles();
    // Register PA callbacks
    void registerPACallbacks();
    void deregisterPACallbacks();

    // Check if the provided phone ID is valid based on current configuration.
    bool isPhoneIdValid(taf::pa::data::PhoneId_e phoneId) const;

    // Get the MTU for the specified host inteface
    le_result_t getMtu(const std::string &ifNameStr, uint16_t &mtu);

    // Update profile objects' static details (updated by clients)
    le_result_t updateProfile(const TafDcsUpdateProfileEvent_t *eventPtr);
    // Update profile object's session details(dynamically based on from PA)
    le_result_t updateSessionDetails(const TafDcsSessionChangeEvent_t *eventPtr);
    // Update throttled APN details
    le_result_t updateThrottledApnStatus(const taf::pa::data::ThrottledApnEventInfo_t &event);

    // Get TafDcsProfile shared pointer based on phone ID and profile ID.
    std::optional<std::reference_wrapper<TafDcsProfile>> getProfile
    (
        uint8_t phoneId,
        uint32_t profileId
    );
    // Get TafDcsProfile pointer based on reference.
    std::optional<std::reference_wrapper<TafDcsProfile>> getProfile
    (
        taf_dcs_ProfileRef_t profileRef
    );
    le_result_t deleteProfile(taf_dcs_ProfileRef_t profileRef, uint8_t phoneId, uint32_t profileId);

    // True on success, false on failure. This will update profilesMap_
    bool addToProfilesMap(uint32_t profileId, uint8_t phoneId, std::shared_ptr<TafDcsProfile>);
    // True on success, false on failure. This will remove an entry profilesMap_
    bool removeFromProfilesMap(uint32_t profileId, uint8_t phoneId);
    // True on success, false on failure. This will update the key that refers to a profile object
    bool updateProfilesMapKey(uint32_t oldProfileId, uint32_t newProfileId, uint8_t phoneId);

    // True on success, false on failure. This will update profilesRefMap_
    bool addToProfileRefsMap(taf_dcs_ProfileRef_t, std::shared_ptr<TafDcsProfile>);
    // True on success, false on failure. This will remove an entry profilesRefMap_
    bool removeFromProfileRefsMap(taf_dcs_ProfileRef_t);

    // The ID of the callback registered for data events
    uint16_t dataEventsCallbackId_ = 0;
    // Forward declarations for PA handlers
    static void tafPaDataCallEventsCb
    (
        const taf::pa::data::DataCallEventInfo_t &,
        std::shared_ptr<void>
    );
    // The ID of the callback registered for roaming events
    uint16_t roamingEventsCallbackId_ = 0;
    static void tafPaRoamingEventsCb
    (
        const taf::pa::data::RoamingStatus_t &roamingEventInfo,
        std::shared_ptr<void> context
    );
    // The ID of the callback registered for throttled APN events
    uint16_t throttledApnEventsCallbackId_ = 0;
    static void tafPaThrottledApnEventsCb
    (
        const std::vector<taf::pa::data::ThrottledApnEventInfo_t> &throttledApnEventsList,
        std::shared_ptr<void> context
    );
    // The ID of the callback registered for QoS events
    uint16_t qosTftEventsCallbackId_ = 0;
    static void tafPaQosTftEventsCb
    (
        const taf::pa::data::QosTftEventInfo_t &qosTftEventInfo,
        std::shared_ptr<void> context
    );
    // The ID of the callback registered for HW acceleration events
    uint16_t hwAccelerationEventsCallbackId_ = 0;
    static void tafPaHwAccelerationEventsCb
    (
        const taf::pa::data::HwAccelerationChangeEvent_t &hwAccelerationEventInfo,
        std::shared_ptr<void> context
    );

    //Declarations for client event handlers
    static void firstDataCallSessionStateHandler (void *reportPtr, void *clientHandlerFunc);
    le_result_t sendSessionSateEvent   (const TafDcsSessionChangeEvent_t &eventPtr);
    static void firstRoamingStatusHandler        (void *reportPtr, void *clientHandlerFunc);
    le_result_t sendRoamingEvent       (const TafDcsRoamingStatus_t *eventPtr);
    static void firstThrottleStatusHandler        (void *reportPtr, void *clientHandlerFunc);
    le_result_t sendThrottledApnEvent  (const TafDcsThrottledApnEventInfo_t *eventPtr);
    static void firstHwAccelerationStateHandler  (void *reportPtr, void *clientHandlerFunc);
    le_result_t sendHwAccelerationEvent(const TafDcsHwAccelerationChangeEvent_t *eventPtr);
    static void firstQosStatusHandler(void *reportPtr, void *clientHandlerFunc);
    le_result_t sendQosTftEvent(const TafDcsQosTftEventInfo_t *eventPtr);

    // Forward declarations for internal event handlers
    static void getProfilesAsyncCb
    (
        taf::pa::data::PhoneId_e phoneId,                          ///< [IN] The phone id.
        pa_result_t paResult,                                 ///< [IN] The result of the operation.
        const std::vector<taf::pa::data::ProfileInfo_t> &profiles, ///< [IN] The profile list.
        void *contextPtr                                           ///< [IN] The context pointer.
    );
    static void registerUpdateProfileEvtHandler(void *, void *);
    static void updateProfileEvtHandler(void *);
    static void registerClientDisconnectedEvtHandler(void *param1Ptr, void *param2Ptr);
    static void clientDisconnectedEvtHandler(void *reqPtr);
    static void registerSessionStartEvtHandler(void *param1Ptr, void *param2Ptr);
    static void sessionStartEvtHandler(void *reqPtr);
    static void registerSessionStopEvtHandler(void *param1Ptr, void *param2Ptr);
    static void sessionStopEvtHandler(void *reqPtr);
    static void registerStartSessionAsyncRspEventHandler(void *param1Ptr, void *param2Ptr);
    static void startSessionAsyncRspEventHandler(void *reqPtr);
    static void registerStopSessionAsyncRspEventHandler(void *param1Ptr, void *param2Ptr);
    static void stopSessionAsyncRspEventHandler(void *reqPtr);

    // PA handlers
    static void registerPaSessionStateChangeEvtHandler(void *param1Ptr, void *param2Ptr);
    static void paSessionStateChangeEvtHandler(void *reqPtr);
    static void registerPaRoamingEvtHandler(void *param1Ptr, void *param2Ptr);
    static void paRoamingEvtHandler(void *reqPtr);
    static void registerPaThrottledAPNsEvtHandler(void *param1Ptr, void *param2Ptr);
    static void paThrottledAPNsEvtHandler(void *reqPtr);
    static void registerPaHwAccelerationEvtHandler(void *param1Ptr, void *param2Ptr);
    static void paHwAccelerationEvtHandler(void *reqPtr);
    static void registerPaQosTftEvtHandler(void *param1Ptr, void *param2Ptr);
    static void paQosTftEvtHandler(void *reqPtr);

    static void signalEventThreadInitComplete(void *param1Ptr, void *param2Ptr);

    /**
     * Private variables
     */
    // Promise for thread synchronization during initialization.
    std::promise<void> eventThreadInitPromise_;

    // Vector of supported phone IDs
    std::vector<taf::pa::data::PhoneId_e> phoneIds_;

    // Map that stores unique TafDcsProfile pointers with Profile Id and Phone Id as key.
    std::map<std::pair<uint32_t, uint8_t>, std::shared_ptr<TafDcsProfile>> profilesMap_;
    // Mutex to make operations thread safe. Needs C++17.
    std::shared_mutex profilesMapMutex_;

    // Map that stores unique TafDcsProfile pointers with profile reference as key.
    std::map<taf_dcs_ProfileRef_t, std::shared_ptr<TafDcsProfile>> profilesRefMap_;
    // Mutex to make operations thread safe. Needs C++17.
    std::shared_mutex profilesRefMapMutex_;

    // Map to track default profiles
    std::map<uint8_t, int32_t> defaultProfileIdMap_;
    // Mutex to make operations thread safe. Needs C++17.
    std::shared_mutex defaultProfileIdMapMutex_;

    // Map to of profile references and HW acceleration state handler references.
    std::map<le_event_HandlerRef_t, taf_dcs_ProfileRef_t> hwAccProfileRefMap_;
    // Mutex to make operations thread safe. Needs C++17.
    std::shared_mutex hwAccProfileRefMapMutex_;

    /**
     * Mutex to make profile read and write operations thread safe. Needs C++17.
     * Use unique_lock when writing and shared_lock from reading.
     * Try to get the write lock only from tafDcsEventsThread, meaning update profiles only from
     * that thread.
     */
    std::shared_mutex profileReadWriteMutex_;

    // Promise to synchronize commands.
    std::promise<le_result_t> syncCmdPromise_;
    // Flag to check if the promise is waiting for the future or not.
    std::atomic<bool> isSyncCmdPromiseWaiting_ = false;
    // TODO: Make this configurable
    const uint16_t syncSessionCmdTimeout_ = 60; // 60s timeout for Start and Stop session sync cmd.`

    // PA throughput callback id
    uint16_t throughputEventsCallbackId_ = 0;

    // PA callback -> OSS
    static void tafPaThroughputEventsCb(
        const std::vector<taf::pa::data::ThroughputInfo_t> &throughputInfoList,
        std::shared_ptr<void> context);

    // Internal DCS event thread handler for PA throughput event
    static void registerPaThroughputEvtHandler(void *param1Ptr, void *param2Ptr);
    static void paThroughputEvtHandler(void *reqPtr);

    // Event IDs for client indication dispatch: one event-id per phone
    le_event_Id_t getThroughputInfoEventId(uint8_t phone);

    std::map<uint8_t, le_event_Id_t> throughputEvtIdByPhone_;
    std::shared_mutex throughputEvtIdByPhoneMtx_;

    // Layered handler for client callbacks
    static void firstThroughputInfoHandler(void *reportPtr, void *clientHandlerFunc);

    // Reference Maps for Throughput Lists and Info objects
    le_ref_MapRef_t throughputInfoListRefMap_ = nullptr;
    le_ref_MapRef_t throughputInfoRefMap_ = nullptr;

    // Helper to get Ref Maps
    le_ref_MapRef_t getThroughputInfoListRefMap();
    le_ref_MapRef_t getThroughputInfoRefMap();

    // Private constructor to prevent instantiation from outside the class.
    TafDcsProfileManager() {};
};

} // namespace datacall
} // namespace svc
} // namespace taf

#endif //_TAFDCS_PROFILE_HPP_