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
 * @file       tafRadio.hpp
 * @brief      Internal interface for Radio Service object. The functions
 *             in this file are impletmented internally.
 */

#ifndef TAFRADIO_HPP
#define TAFRADIO_HPP

#include "legato.h"
#include "interfaces.h"

#include <vector>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/Phone.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/PhoneListener.hpp>

#include "tafSvcIF.hpp"

#define TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM 2
#define TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM 100

#define TAF_RADIO_SCAN_INTERVAL 100
#define TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM 10
#define TAF_RADIO_SCAN_OPERATORS_MAX_NUM 20

#define TAF_RADIO_METRICS_MAX_NUM 2
#define TAF_RADIO_CELL_METRICS_MAX_NUM 2

#define TAF_RADIO_THREAD_STACK_SIZE 0x20000

#define TAF_RADIO_LTE_SNR_RATIO 0.1

/*
 * @brief The emum of radio command type.
 */
typedef enum
{
    TAF_RADIO_CMD_TYPE_ASYNC_REG_MANUAL,
    TAF_RADIO_CMD_TYPE_ASYNC_NETWORK_SCAN,
} taf_RadioCmdType_t;

/*
 * @brief The emum of radio cell information type.
 */
typedef enum
{
    TAF_RADIO_CELL_INFO_TYPE_GSM = 0,
    TAF_RADIO_CELL_INFO_TYPE_CDMA,
    TAF_RADIO_CELL_INFO_TYPE_LTE,
    TAF_RADIO_CELL_INFO_TYPE_WCDMA,
    TAF_RADIO_CELL_INFO_TYPE_TDSCDMA,
    TAF_RADIO_CELL_INFO_TYPE_MAX,
} taf_RadioCellInfoType_t;

/*
 * @brief The struct of safe reference for prefered operators in network.
 */
typedef struct
{
    void* safeRef;
    le_sls_Link_t link;
} taf_RadioPrefOpSafeRef_t;

/*
 * @brief The struct of prefered operator in network.
 */
typedef struct
{
    telux::tel::PreferredNetworkInfo info;
    le_sls_Link_t link;
} taf_RadioPrefOp_t;

/*
 * @brief The struct of prefered operator list in network.
 */
typedef struct
{
    le_sls_List_t prefOpList;
    le_sls_List_t safeRefList;
    le_sls_Link_t* currPtr;
} taf_RadioPrefOpList_t;

/*
 * @brief The struct of safe reference for scan operators in network.
 */
typedef struct
{
    void* safeRef;
    le_sls_Link_t link;
} taf_RadioScanOpSafeRef_t;

/*
 * @brief The struct of scan operator in network.
 */
typedef struct
{
    char name[TAF_RADIO_NETWORK_NAME_MAX_LEN];
    char mcc[TAF_RADIO_MCC_BYTES];
    char mnc[TAF_RADIO_MNC_BYTES];
    telux::tel::OperatorStatus status;
    le_sls_Link_t link;
} taf_RadioScanOp_t;

/*
 * @brief The struct of scan operation list in network.
 */
typedef struct
{
    uint32_t num;
    le_sls_List_t scanOpList;
    le_sls_List_t safeRefList;
    le_sls_Link_t* currPtr;
} taf_RadioScanOpList_t;

/*
 * @brief The struct of radio command request.
 */
typedef struct
{
    taf_RadioCmdType_t cmdType;
    void* handlerFuncPtr;
    void* contextPtr;
    uint8_t phoneId;
    const char* mccPtr;
    const char* mncPtr;
} taf_RadioCmdReq_t;

/*
 * @brief The struct of GSM cell identity information.
 */
typedef struct
{
    int bsic;
} taf_RadioGsmCellIdInfo_t;

/*
 * @brief The struct of CDMA cell identity information.
 */
typedef struct
{
    int nid;
    int sid;
    int bsid;
    int longitude;
    int latitude;
} taf_RadioCdmaCellIdInfo_t;

/*
 * @brief The struct of LTE cell identity information.
 */
typedef struct
{
    int pid;
    int tac;
} taf_RadioLteCellIdInfo_t;

/*
 * @brief The struct of WCDMA cell identity information.
 */
typedef struct
{
    int psc;
} taf_RadioWcdmaCellIdInfo_t;

/*
 * @brief The struct of TDSCDMA cell identity information.
 */
typedef struct
{
    int cpid;
} taf_RadioTdscdmaCellIdInfo_t;

/*
 * @brief The struct of cell identity information.
 */
typedef struct
{
    int mcc;
    int mnc;
    int lac;
    int cid;
    int arfcn;
    union {
        taf_RadioGsmCellIdInfo_t gsm;
        taf_RadioCdmaCellIdInfo_t cmda;
        taf_RadioLteCellIdInfo_t lte;
        taf_RadioWcdmaCellIdInfo_t wcdma;
        taf_RadioTdscdmaCellIdInfo_t tdscdma;
    };
} taf_RadioCellIdInfo_t;

/*
 * @brief The struct of CDMA signal strength information.
 */
typedef struct
{
    int cdmaEcio;
    int evdoEcio;
} taf_RadioCdmaSignalStrengthInfo_t;

/*
 * @brief The struct of LTE signal strength information.
 */
typedef struct
{
    int rsrq;
    int cqi;
} taf_RadioLteSignalStrengthInfo_t;

/*
 * @brief The struct of TD-SCDMA signal strength information.
 */
typedef struct
{
    int rscp;
} taf_RadioTdscdmaSignalStrengthInfo_t;

/*
 * @brief The struct of signal strength information.
 */
typedef struct
{
    int strength;
    int ber;
    int snr;
    int dbm;
    int ta;
    telux::tel::SignalStrengthLevel level;
    union {
        taf_RadioCdmaSignalStrengthInfo_t cdma;
        taf_RadioLteSignalStrengthInfo_t lte;
        taf_RadioTdscdmaSignalStrengthInfo_t tdscdma;
    };
} taf_RadioSignalStrengthInfo_t;

/*
 * @brief The struct of signal metrics.
 */
typedef struct
{
    bool isRegistered;
    taf_RadioCellIdInfo_t cellId;
    taf_RadioSignalStrengthInfo_t signalStrength;
} taf_RadioSignalMetrics_t;

/*
 * @brief The struct of cell metrics.
 */
typedef struct
{
    taf_radio_CellRatMask_t cellRatMask;
    taf_RadioSignalMetrics_t signalMetrics[TAF_RADIO_CELL_INFO_TYPE_MAX];
} taf_RadioCellMetrics_t;

namespace telux {
namespace tafsvc {
    /*
     * @brief The class of functions used in Radio Services interfaces.
     */
    class taf_RadioFunctions {
    public:
        /*
         * This function is called when checking MCC and MNC from Client.
         *
         * @param [in] mccPtr    The mobile country code string.
         * @param [in] mncPtr    The mobile network code string.
         * @returns    Check result, LE_BAD_PARAMETER or LE_OK.
         */
        static le_result_t taf_radio_CheckMccMnc(const char* mccPtr, const char* mncPtr);
    };

    /*
     * @brief The phone listener is registered for the radio state updates.
     */
    class taf_RadioPhoneListener : public telux::tel::IPhoneListener {
    public:
        static telux::tel::VoiceServiceState vocSrvState;
        ~taf_RadioPhoneListener() {};
        /*
         * This function is called when radio state changes.
         *
         * @param [in] phoneId       The phone id.
         * @param [in] radioState    The radio state of phone.
         */
        void onRadioStateChanged(int phoneId, telux::tel::RadioState radioState) override;

        /*
         * This function is called when RAT changes.
         *
         * @param [in] phoneId            The phone id.
         * @param [in] radioTechnology    The radio technology.
         */
        void onVoiceRadioTechnologyChanged(int phoneId, telux::tel::RadioTechnology radioTechnology) override;

        /*
         * This function is called when radio state changes.
         *
         * @param [in] phoneId    The phone id.
         * @param [in] srvInfo    A network service information pointer.
         */
        void onVoiceServiceStateChanged(int phoneId, const std::shared_ptr<telux::tel::VoiceServiceInfo> &srvInfo) override;

        /*
         * This function is called when signal strength changes.
         *
         * @param [in] phoneId           The phone id.
         * @param [in] signalStrength    A signal strength information pointer.
         */
        void onSignalStrengthChanged(int phoneId, std::shared_ptr<telux::tel::SignalStrength> signalStrength) override;
    };

    /*
     * @brief The network listener is registered for the network selection mode updates.
     */
    class taf_RadioNetworkSelectionListener : public telux::tel::INetworkSelectionListener {
    public:
        /*
         * This function is called when network selection mode changes.
         *
         * @param [in] mode    The network selection mode of phone.
         */
        void onSelectionModeChanged(telux::tel::NetworkSelectionMode mode) override;
    };

    /*
     * @brief The serving system listener is registered for the service domain.
     */
    class taf_RadioServingSystemListener : public telux::tel::IServingSystemListener {
    public:
        /*
         * This function is called when service domain changes.
         *
         * @param [in] preference    The service domain preference.
         */
        void onServiceDomainPreferenceChanged(telux::tel::ServiceDomainPreference preference) override;
    };

    /*
     * @brief The sbscription listener is registered for the sbscriptions updates.
     */
    class taf_RadioSubscriptionListener : public telux::tel::ISubscriptionListener {
    public:
        /*
         * This function is called when subscription infomation changes.
         *
         * @param [in] subscription    A subscription pointer with infomation of phone.
         */
        void onSubscriptionInfoChanged(std::shared_ptr<telux::tel::ISubscription> subscription) override;
        /*
         * This function is called when subscription infomation changes.
         *
         * @param [in] count    The number of subscription.
         */
        void onNumberOfSubscriptionsChanged(int count) override;
    };

    /*
     * @brief A radio power callback class must be provided when configuring the radio power.
     */
    class taf_RadioPowerCallback : public telux::common::ICommandResponseCallback {
    public:
        /*
         * This function is called after configuration of radio power.
         *
         * @param [in] error    The error code of radio power configuration.
         */
        void commandResponse(telux::common::ErrorCode error);
    };

    /*
     * @brief A radio tech callback class must be provided when request for the rat in use.
     */
    class taf_RadioVoiceRadioTechnologyCallback {
    public:
        le_sem_Ref_t semaphore;
        telux::tel::RadioTechnology radioTech;
        /*
         * This function is called after getting the rat in use.
         *
         * @param [in] radioTechnology    The radio technology in use.
         * @param [in] error              The error code of getting the rat in use.
         */
        void voiceRadioTechnologyResponse(telux::tel::RadioTechnology radioTechnology, telux::common::ErrorCode error);
    };

    /*
     * @brief A voice service state callback class must be provided when request for the service state.
     */
    class taf_RadioVoiceServiceStateCallback : public telux::tel::IVoiceServiceStateCallback {
    public:
        static le_sem_Ref_t semaphore;
        static telux::tel::VoiceServiceState vocSrvState;
        /*
         * This function is called after getting the service state.
         *
         * @param [in] serviceInfo    The service information.
         * @param [in] error          The error code of getting the service state.
         */
        void voiceServiceStateResponse(const std::shared_ptr<telux::tel::VoiceServiceInfo> &serviceInfo,
            telux::common::ErrorCode error) override;
};

    /*
     * @brief A signal strength callback class must be provided when request for the signal strength.
     */
    class taf_RadioSignalStrengthCallback : public telux::tel::ISignalStrengthCallback {
    public:
        le_sem_Ref_t semaphore;
        telux::tel::SignalStrengthLevel signalStrengthLevel;
        /*
         * This function is called after getting the signal strength.
         *
         * @param [in] signalStrength    The signal strength information.
         * @param [in] error             The error code of getting the signal strength.
         */
        void signalStrengthResponse(std::shared_ptr<telux::tel::SignalStrength> signalStrength,
            telux::common::ErrorCode error) override;
    };

    /*
     * @brief A radio network selection callback class must be provided when configuring the network selection mode
     *        and network preference.
     */
    class taf_RadioNetworkResponseCallback {
    public:
        static le_sem_Ref_t semNetSelModeRespCb;
        static le_sem_Ref_t semPrefNetRespCb;
        static int32_t errCode;
        static telux::common::ErrorCode errorCode;
        /*
         * This function is called after configuration of network selection mode.
         *
         * @param [in] error    The error code of network selection mode configuration.
         */
        static void setNetworkSelectionModeResponseCb(telux::common::ErrorCode error);
        /*
         * This function is called after configuration of network preference.
         *
         * @param [in] error    The error code of network preference configuration.
         */
        static void setPreferredNetworksResponseCb(telux::common::ErrorCode error);
    };

    /*
     * @brief A radio network selection callback class must be provided when performing the network scan.
     */
    class taf_RadioPerformNetworkScanCallback {
    public:
        static le_sem_Ref_t semaphore;
        static std::vector<telux::tel::OperatorInfo> opInfos;
        /*
         * This function is called after performing the network scan.
         *
         * @param [in] operatorInfos    The network scan information.
         * @param [in] error            The error code of performing the network scan.
         */
        static void performNetworkScanResponse(std::vector<telux::tel::OperatorInfo> operatorInfos,
            telux::common::ErrorCode error);
    };

    /*
     * @brief A radio preferred networks callback class must be provided when getting the network preference.
     */
    class taf_RadioPreferredNetworksResponseCallback {
    public:
        static std::vector<telux::tel::PreferredNetworkInfo> preferredNetworksInfo;
        static le_sem_Ref_t semaphore;
        /*
         * This function is called after  getting the network preference.
         *
         * @param [in] preferredNetworks3gppInfo      The non-static prefered network.
         * @param [in] staticPreferredNetworksInfo    The static prefered network.
         * @param [in] error                          The error code of radio power configuration.
         */
        static void preferredNetworksResponse(
            std::vector<telux::tel::PreferredNetworkInfo> preferredNetworks3gppInfo,
            std::vector<telux::tel::PreferredNetworkInfo> staticPreferredNetworksInfo,
            telux::common::ErrorCode error);
    };

    /*
     * @brief A radio network selection callback class must be provided when getting the network selection mode.
     */
    class taf_RadioSelectionModeResponseCallback {
    public:
        static bool isRegModeMannual;
        static le_sem_Ref_t semaphore;
        /*
         * This function is called after configuration of network selection mode.
         *
         * @param [in] networkSelectionMode    Automatic or mannual mode.
         * @param [in] error                   The error code of network selection mode configuration.
         */
        static void selectionModeResponse(telux::tel::NetworkSelectionMode networkSelectionMode,
            telux::common::ErrorCode error);
    };

    /*
     * @brief A service domain callback class must be provided when getting the service domain preference.
     */
    class taf_RadioServiceDomainResponseCallback {
    public:
        static telux::tel::ServiceDomainPreference svcDomainPref;
        static le_sem_Ref_t semaphore;
        /*
         * This function is called after getting of the service domain preference.
         *
         * @param [in] preference    Circuit Switched only(CS), Packet Switched only(PS) or Circuit and Packet Switched.
         * @param [in] error         The error code of the service domain preference configuration.
         */
        static void serviceDomainResponse(telux::tel::ServiceDomainPreference preference,
            telux::common::ErrorCode error);
    };

    /*
     * @brief A serving system callback class must be provided when configuring rat preference.
     */
    class taf_RadioServingSystemResponseCallback {
    public:
        /*
         * This function is called after configuration of the rat preference.
         *
         * @param [in] error         The error code of the rat preference configuration.
         */
        static void servingSystemResponse(telux::common::ErrorCode error);
    };

    /*
     * @brief A RAT preference callback class must be provided when getting rat preference.
     */
    class taf_RadioRatPreferenceResponseCallback {
    public:
        static le_sem_Ref_t semaphore;
        static telux::tel::RatPreference ratPref;
        /*
         * This function is called after getting of the service domain preference.
         *
         * @param [in] preference    Rat preference.
         * @param [in] error         The error code of the service domain preference configuration.
         */
        static void ratPreferenceResponse(telux::tel::RatPreference preference, telux::common::ErrorCode error);
    };

    /*
     * @brief A cell information callback class must be provided when requesting cell information.
     */
    class taf_RadioCellInfoCallback {
    public:
        static le_sem_Ref_t semaphore;
        static taf_RadioCellMetrics_t cellMetrics;
        /*
         * This function is called after requesting cell information.
         *
         * @param [in] cellInfoList    Pointer of cell information list.
         * @param [in] error           The error code of the rat preference configuration.
         */
        static void cellInfoListResponse(std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList,
            telux::common::ErrorCode error);
    };

    /*
     * @brief The Radio Service class defined as a middleware between interfaces and implementation.
     */
    class taf_Radio : public ITafSvc {
    public:
        taf_Radio() {};
        ~taf_Radio() {};

        /*
         * This function is used for visiting the members of instance.
         *
         * @returns    Static reference of instance.
         */
        static taf_Radio &GetInstance();

        /*
         * Command thread in radio service.
         * @param [in] contextPtr    Context pointer.
         *
         * @returns    Null.
         */
        static void* RadioCmdThread(void* contextPtr);

        /*
         * Handler for command thread in radio service.
         * @param [in] cmdReqPtr    Command request pointer.
         *
         * @returns    Null.
         */
        static void RadioProcCmdHandler(void* cmdReqPtr);

        /*
         * The first layer handler function for network registration rejection.
         *
         * @param [in] reportPtr                 Pointer to report details.
         * @param [in] secondLayerHandlerFunc    The second layer handler function for network registration rejection.
         */
        static void FirstLayerNetRegRejectHandler(void* reportPtr, void* secondLayerHandlerFunc);

        /*
         * The first layer handler function for RAT change.
         *
         * @param [in] reportPtr                 Pointer to report details.
         * @param [in] secondLayerHandlerFunc    The second layer handler function for RAT change.
         */
        static void FirstLayerRatChangeHandler(void* reportPtr, void* secondLayerHandlerFunc);

        /*
         * The first layer handler function for network registration state.
         *
         * @param [in] reportPtr                 Pointer to report details.
         * @param [in] secondLayerHandlerFunc    The second layer handler function for network registration state.
         */
        static void FirstLayerNetRegStateEventHandler(void* reportPtr, void* secondLayerHandlerFunc);

        /*
         * The first layer handler function for service domain state.
         *
         * @param [in] reportPtr                 Pointer to report details.
         * @param [in] secondLayerHandlerFunc    The second layer handler function for service domain state.
         */
        static void FirstLayerPacketSwChangeHandler(void* reportPtr, void* secondLayerHandlerFunc);
        /*
         * The first layer handler function for signal strength change.
         *
         * @param [in] reportPtr                 Pointer to report details.
         * @param [in] secondLayerHandlerFunc    The second layer handler function for signal strength change.
         */
        static void FirstLayerSsChangeHandler(void* reportPtr, void* secondLayerHandlerFunc);

        /*
         * The initialization function of the Radio Service.
         */
        void Init(void);

        le_mem_PoolRef_t prefOpsListPool;
        le_mem_PoolRef_t prefOpPool;
        le_mem_PoolRef_t prefOpSafeRefPool;
        le_mem_PoolRef_t scanOpsListPool;
        le_mem_PoolRef_t scanOpPool;
        le_mem_PoolRef_t scanOpSafeRefPool;
        le_mem_PoolRef_t cellMetricsPool;
        le_mem_PoolRef_t netRegRejectPool;
        le_mem_PoolRef_t ratChangePool;
        le_mem_PoolRef_t netRegStatePool;
        le_mem_PoolRef_t packetSwChangePool;
        le_mem_PoolRef_t ssChangePool;
        le_ref_MapRef_t prefOpListRefMap;
        le_ref_MapRef_t prefOpSafeRefMap;
        le_ref_MapRef_t scanOpListRefMap;
        le_ref_MapRef_t scanOpSafeRefMap;
        le_ref_MapRef_t metricsRefMap;
        static le_event_Id_t radioCmdEvId;
        le_event_Id_t netRegRejectEvId;
        le_event_Id_t ratChangeEvId;
        le_event_Id_t netRegStateEvId;
        le_event_Id_t packetSwChangeEvId;
        le_event_Id_t gsmSsChangeEvId;
        le_event_Id_t cdmaSsChangeEvId;
        le_event_Id_t lteSsChangeEvId;
        le_event_Id_t wcdmaSsChangeEvId;
        le_event_Id_t tdscdmaSsChangeEvId;
        std::shared_ptr<taf_RadioPowerCallback> radioPowerCb;
        std::shared_ptr<taf_RadioVoiceServiceStateCallback> voiceSrvStateCb;
        std::shared_ptr<taf_RadioVoiceRadioTechnologyCallback> voiceRadioTechCb;
        std::shared_ptr<taf_RadioSignalStrengthCallback> signalStrengthCb;
        std::vector<std::shared_ptr<telux::tel::IPhone>> phones;
        std::vector<std::shared_ptr<telux::tel::INetworkSelectionManager>> networkManagers;
        std::vector<std::shared_ptr<telux::tel::IServingSystemManager>> servingSystemManagers;
        std::shared_ptr<telux::tel::ISubscriptionManager> subscriptionManager;

    private:
        std::shared_ptr<telux::tel::IPhoneManager> phoneManager;
        std::shared_ptr<telux::tel::IPhoneListener> phoneListener;
        std::shared_ptr<telux::tel::INetworkSelectionListener> networkListener;
        std::shared_ptr<telux::tel::IServingSystemListener> servingSystemListener;
        std::shared_ptr<taf_RadioSubscriptionListener> subscriptionListener;
    };
}
}

#endif /* #ifndef TAFRADIO_H */
