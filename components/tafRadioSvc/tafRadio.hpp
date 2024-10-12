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
 *
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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
#include <telux/tel/ImsServingSystemManager.hpp>

#include <telux/data/DataFactory.hpp>
#include <telux/data/ServingSystemManager.hpp>

#include "tafSvcIF.hpp"

#define TAF_RADIO_PHONE_NUM 2

#define TAF_RADIO_PREFERRED_OPERATORS_LISTS_MAX_NUM 2
#define TAF_RADIO_PREFERRED_OPERATORS_MAX_NUM 100

#define TAF_RADIO_SCAN_INTERVAL 210
#define TAF_RADIO_SCAN_OPERATORS_LISTS_MAX_NUM 10
#define TAF_RADIO_SCAN_OPERATORS_MAX_NUM 20

#define TAF_RADIO_METRICS_MAX_NUM 2

#define TAF_RADIO_THREAD_STACK_SIZE 0x20000

#define TAF_RADIO_NEIGHBOR_CELLS_MAX_NUM 10
#define TAF_RADIO_NEIGHBOR_CELL_INFO_MAX_NUM 6

#define TAF_RADIO_SUBSYSTEM_TIMEOUT 15

#define TAF_RADIO_BAND_NUM_PER_GROUP 64

/*
 * @brief The emum of radio command type.
 */
typedef enum
{
    TAF_RADIO_CMD_TYPE_ASYNC_REG_MANUAL,
    TAF_RADIO_CMD_TYPE_ASYNC_NETWORK_SCAN,
    TAF_RADIO_CMD_TYPE_ASYNC_PCI_NETWORK_SCAN
} taf_RadioCmdType_t;

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
    telux::tel::RadioTechnology rat;
    le_sls_Link_t link;
} taf_RadioScanOp_t;

/*
 * @brief The struct of scan operation list in network.
 */
typedef struct
{
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
    taf_radio_RatBitMask_t ratMask;
    uint8_t phoneId;
    const char* mccPtr;
    const char* mncPtr;
} taf_RadioCmdReq_t;

/*
 * @brief The struct of GSM cell information.
 */
typedef struct
{
    int bsic;
    int cid;
    int lac;
    int ta;
    int arfcn;
} taf_RadioGsmCellInfo_t;

/*
 * @brief The struct of CDMA cell information.
 */
typedef struct
{
    int bsid;
    int ecio;
} taf_RadioCdmaCellInfo_t;

/*
 * @brief The struct of LTE cell information.
 */
typedef struct
{
    int cid;
    int pcid;
    int tac;
    int earfcn;
    int ta;
} taf_RadioLteCellInfo_t;

/*
 * @brief The struct of UMTS cell information.
 */
typedef struct
{
    int lac;
    int cid;
    int psc;
    int ecio;
    int uarfcn;
} taf_RadioUmtsCellInfo_t;

/*
 * @brief The struct of TDSCDMA cell information.
 */
typedef struct
{
    int lac;
    int cid;
} taf_RadioTdscdmaCellInfo_t;

/*
 * @brief The struct of NR5G cell information.
 */
typedef struct
{
    uint64_t cid;
    uint32_t pcid;
    int32_t tac;
    int32_t arfcn;
} taf_RadioNr5gCellInfo_t;

/*
 * @brief The struct of cell information.
 */
typedef struct
{
    taf_radio_Rat_t rat;
    int ss;
    char mcc[TAF_RADIO_MCC_BYTES];
    char mnc[TAF_RADIO_MCC_BYTES];
    union
    {
        taf_RadioGsmCellInfo_t gsm;
        taf_RadioCdmaCellInfo_t cdma;
        taf_RadioLteCellInfo_t lte;
        taf_RadioUmtsCellInfo_t umts;
        taf_RadioTdscdmaCellInfo_t tdscdma;
        taf_RadioNr5gCellInfo_t nr5g;
    };
} taf_RadioCellInfo_t;

typedef struct
{
    int ss;   ///< Signal Strength in dBm
    int ber;  ///< Bit Error Rate, range [0, 7]
    int sslv; ///< Signal Strength Level
} taf_RadioGsmSignalMetrics_t;

typedef struct
{
    int ss;   ///< Signal Strength in dBm
    int ber;  ///< Bit error rate
    int sslv; ///< Signal Strength Level
} taf_RadioUmtsSignalMetrics_t;

typedef struct
{
    int rscp; ///< Received Signal Code Power in dBm.
} taf_RadioTdscdmaSignalMetrics_t;

typedef struct
{
    int ss;   ///< Signal Strength in dBm
    int ecio; ///< CDMA Ec/IO in dB
    int io;   ///< EVDO EC/IO in dB
    int snr;  ///< Signal Noise Ratio, range [0, 8]
    int sslv; ///< Signal Strength Level
} taf_RadioCdmaSignalMetrics_t;

typedef struct
{
    int ss;   ///< Signal Strength in dBm
    int rsrq; ///< Reference Signal Receive Quality in dB
    int rsrp; ///< Reference Signal Receive Power in dBm
    int snr;  ///< Signal Noise Ratio in 0.1 dB
    int sslv; ///< Signal Strength Level
} taf_RadioLteSignalMetrics_t;

typedef struct
{
    int rsrq; ///< Reference Signal Receive Quality in dB
    int rsrp; ///< Reference Signal Receive Power in dBm
    int snr;  ///< Signal Noise Ratio in 0.1 dB
    int sslv; ///< Signal Strength Level
} taf_RadioNr5gSignalMetrics_t;
/*
 * @brief The struct of signal metrics.
 */
typedef struct
{
    uint8_t phoneId;
    taf_radio_RatBitMask_t ratMask;
    taf_RadioGsmSignalMetrics_t gsm;
    taf_RadioUmtsSignalMetrics_t umts;
    taf_RadioTdscdmaSignalMetrics_t tdscdma;
    taf_RadioCdmaSignalMetrics_t cdma;
    taf_RadioLteSignalMetrics_t lte;
    taf_RadioNr5gSignalMetrics_t nr5g;
} taf_RadioSignalMetrics_t;

/*
 * @brief The struct of cell list information.
 */
typedef struct
{
    std::vector<std::shared_ptr<taf_RadioCellInfo_t>> servingCell;
    std::vector<std::shared_ptr<taf_RadioCellInfo_t>> neighborCell;
} taf_RadioCellListInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Neighboring cell information safe reference structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    void* safeRef;
    le_sls_Link_t link;
} taf_RadioNgbrCellInfoSafeRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Neighboring cell information structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    taf_RadioCellInfo_t cell;
    le_sls_Link_t link;
} taf_RadioNgbrCellInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Neighboring cells structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sls_List_t cellInfoList;
    le_sls_List_t safeRefList;
    le_sls_Link_t* currPtr;
} taf_RadioNgbrCells_t;

//--------------------------------------------------------------------------------------------------
/**
 * IMS status structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;
    taf_radio_ImsRegStatus_t status;
    taf_radio_ImsIndBitMask_t bitmask;
    taf_radio_ImsRef_t imsRef;
} taf_RadioImsStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data callback information structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sem_Ref_t semaphore;
    le_result_t result;
    taf_radio_NetRegState_t psState;
} taf_RadioDataCallbackInfo_t;
//--------------------------------------------------------------------------------------------------
/**
 * Operator Name callback information structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sem_Ref_t semaphore;
    le_result_t result;
    char longOpNamePtr[TAF_RADIO_NETWORK_NAME_MAX_LEN];
    char shortOpNamePtr[TAF_RADIO_NETWORK_NAME_MAX_LEN];
} taf_OperatorNameCallbackInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Signal strength structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;
    int32_t rssi;
    int32_t rsrp;
} taf_RadioSsInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Cell info change structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;
    taf_radio_CellInfoStatus_t cellInfoStatus;
} taf_RadioCellInfoInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Hysteresis structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;
    taf_radio_SigType_t sigType;
    uint16_t hysteresisdB;
} taf_RadioHysteresisConfig_t;

//--------------------------------------------------------------------------------------------------
/**
 * Network status indication structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t phoneId;
    taf_radio_NetStatusIndBitMask_t bitmask;
    taf_radio_NetStatusRef_t netStatusRef;
} taf_RadioNetStatusInd_t;

namespace telux {
namespace tafsvc {
    /*
     * @brief The network listener is registered for the network selection mode updates.
     */
    class taf_RadioNetworkSelectionListener : public telux::tel::INetworkSelectionListener {
    public:
        le_sem_Ref_t semaphore;
        std::vector<telux::tel::OperatorInfo> opInfos;
        void onNetworkScanResults(telux::tel::NetworkScanStatus scanStatus,
            std::vector<telux::tel::OperatorInfo> operatorInfos) override;
    };

    class taf_RadioServSysListener : public telux::tel::IServingSystemListener
    {
        public:
            uint8_t phone = DEFAULT_PHONE_ID;
            telux::tel::RadioTechnology rat = telux::tel::RadioTechnology::RADIO_TECH_UNKNOWN;
            telux::tel::ServiceDomain serviceDomain = telux::tel::ServiceDomain::UNKNOWN;
            taf_RadioServSysListener(uint8_t phone);
            void onSystemInfoChanged(telux::tel::ServingSystemInfo sysInfo) override;
            void onNetworkRejection(telux::tel::NetworkRejectInfo rejectInfo) override;
            void onLteCsCapabilityChanged(telux::tel::LteCsCapability lteCapability) override;
    };

    class taf_RadioImsServSysListener : public telux::tel::IImsServingSystemListener
    {
        public:
            SlotId slot = DEFAULT_SLOT_ID;
            uint8_t phone = DEFAULT_PHONE_ID;

            taf_RadioImsServSysListener(SlotId slotId);
            void onImsRegStatusChange(telux::tel::ImsRegistrationInfo status) override;
#ifdef LE_CONFIG_FEATURE_ENHANCED_IMS
            void onImsServiceInfoChange(telux::tel::ImsServiceInfo service) override;
            void onImsPdpStatusInfoChange(telux::tel::ImsPdpStatusInfo status) override;
#endif
    };

    class taf_RadioPhoneListener : public telux::tel::IPhoneListener
    {
        public:
            void onVoiceServiceStateChanged(
                int phoneId,
                const std::shared_ptr<telux::tel::VoiceServiceInfo> &serviceInfo) override;
            void onOperatingModeChanged(telux::tel::OperatingMode mode) override;
            void onSignalStrengthChanged(int phoneId,
                std::shared_ptr<telux::tel::SignalStrength> signalStrength) override;
            void onCellInfoListChanged(int phoneId,
                std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList) override;
    };

    class taf_RadioDataServSysListener : public telux::data::IServingSystemListener
    {
        public:
            SlotId slot = DEFAULT_SLOT_ID;
            uint8_t phone = DEFAULT_PHONE_ID;
            taf_radio_NetRegState_t currState = TAF_RADIO_NET_REG_STATE_UNKNOWN;
            bool inService = false;
            bool isRoaming = false;

            taf_RadioDataServSysListener(SlotId slotId);
            void onServiceStateChanged(telux::data::ServiceStatus status) override;
            void onRoamingStatusChanged(telux::data::RoamingStatus status) override;
    };

    class taf_RadioSetOperatingModeCallback {
    public:
        le_sem_Ref_t semaphore;
        le_result_t result;
        void setOperatingModeResponse(telux::common::ErrorCode error);
    };

    class taf_RadioGetOperatingModeCallback : public telux::tel::IOperatingModeCallback
    {
        public:
            le_sem_Ref_t semaphore;
            le_result_t result;
            telux::tel::OperatingMode opMode;

            void operatingModeResponse(telux::tel::OperatingMode operatingMode,
                telux::common::ErrorCode error) override;
    };

    /*
     * @brief A signal strength callback class must be provided when request for the signal strength.
     */
    class taf_RadioSignalStrengthCallback : public telux::tel::ISignalStrengthCallback {
    public:
        le_sem_Ref_t semaphore;
        le_result_t result;
        taf_RadioSignalMetrics_t ssMetrics;
        /*
         * This function is called after getting the signal strength.
         *
         * @param [in] signalStrength    The signal strength information.
         * @param [in] error             The error code of getting the signal strength.
         */
        void signalStrengthResponse(std::shared_ptr<telux::tel::SignalStrength> signalStrength,
            telux::common::ErrorCode error) override;
    };

    class taf_RadioConfigureSignalStrengthCallback
    {
        public:
            static le_sem_Ref_t semaphore;
            static le_result_t result;

            static void configureSignalStrengthResponse(telux::common::ErrorCode error);
    };

    class taf_RadioVoiceServiceStateCallback : public telux::tel::IVoiceServiceStateCallback
    {
        public:
            le_sem_Ref_t semaphore;
            le_result_t result;
            telux::tel::VoiceServiceState voiceSvcState;

            void voiceServiceStateResponse(
                const std::shared_ptr<telux::tel::VoiceServiceInfo> &serviceInfo,
                telux::common::ErrorCode error) override;
    };

    class taf_RadioSelectionModeResponseCallback
    {
        public:
            static le_sem_Ref_t semaphore;
            static le_result_t result;
            static telux::tel::NetworkModeInfo selectModeInfo;

            static void selectionModeResponse(telux::tel::NetworkModeInfo info,
                telux::common::ErrorCode error);
    };
    /*
     * @brief A radio network selection callback class must be provided when configuring the network selection mode
     *        and network preference.
     */
    class taf_RadioNetworkResponseCallback {
    public:
        static le_sem_Ref_t selModeSem;
        static le_sem_Ref_t prefNetSem;
        static le_result_t selModeRes;
        static le_result_t prefNetRes;

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
        static le_result_t result;
        /*
         * This function is called after performing the network scan.
         *
         * @param [in] error            The error code of performing the network scan.
         */
        static void performNetworkScanResponse(telux::common::ErrorCode error);
    };

    /*
     * @brief A radio preferred networks callback class must be provided when getting the network preference.
     */
    class taf_RadioPreferredNetworksResponseCallback {
    public:
        static std::vector<telux::tel::PreferredNetworkInfo> preferredNetworksInfo;
        static le_sem_Ref_t semaphore;
        static le_result_t result;

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
     * @brief A serving system callback class must be provided when configuring rat preference.
     */
    class taf_RadioServingSystemResponseCallback {
    public:
        static le_sem_Ref_t semaphore;
        static le_result_t result;

        /*
         * This function is called after configuration of the rat preference.
         *
         * @param [in] error         The error code of the rat preference configuration.
         */
        static void servingSystemResponse(telux::common::ErrorCode error);
    };

    class taf_RadioRFBandCapabilityResponseCallback
    {
        public:
            static le_sem_Ref_t semaphore;
            static le_result_t result;
            static taf_radio_BandBitMask_t bandCapability;
            static uint64_t lteBandCapability[TAF_RADIO_LTE_BAND_GROUP_NUM];

            static void rfBandCapabilityResponse(
                std::shared_ptr<telux::tel::IRFBandList> capabilityList,
                telux::common::ErrorCode error);
    };

    class taf_RadioRFBandPrefResponseCallback
    {
        public:
            static le_sem_Ref_t semaphore;
            static le_result_t result;
            static taf_radio_BandBitMask_t bandPreferences;
            static uint64_t lteBandPreferences[TAF_RADIO_LTE_BAND_GROUP_NUM];

            static void rfBandPrefResponse(std::shared_ptr<telux::tel::IRFBandList> prefList,
                telux::common::ErrorCode error);
            static void setRFBandPrefResponse(telux::common::ErrorCode error);
    };

    /*
     * @brief A RAT preference callback class must be provided when getting rat preference.
     */
    class taf_RadioRatPreferenceResponseCallback {
    public:
        static le_sem_Ref_t semaphore;
        static le_result_t result;
        static telux::tel::RatPreference ratPref;

        /*
         * This function is called after getting of the service domain preference.
         *
         * @param [in] preference    Rat preference.
         * @param [in] error         The error code of the service domain preference configuration.
         */
        static void ratPreferenceResponse(telux::tel::RatPreference preference, telux::common::ErrorCode error);
    };

    class taf_RadioServiceDomainPreferenceResponseCallback
    {
        public:
            static le_sem_Ref_t semaphore;
            static le_result_t result;
            static telux::tel::ServiceDomainPreference domainPref;

            static void serviceDomainPrefResponse(telux::tel::ServiceDomainPreference preference,
                telux::common::ErrorCode error);
    };

    /*
     * @brief A cell information callback class must be provided when requesting cell information.
     */
    class taf_RadioCellInfoCallback {
    public:
        static le_sem_Ref_t semaphore;
        static le_result_t result;
        static taf_RadioCellListInfo_t cellListInfo;
        /*
         * This function is called after requesting cell information.
         *
         * @param [in] cellInfoList    Pointer of cell information list.
         * @param [in] error           The error code of the rat preference configuration.
         */
        static void cellInfoListResponse(std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList,
            telux::common::ErrorCode error);
    };

    class taf_RadioImsServSysCallback
    {
    public:
        static le_sem_Ref_t semaphore;
        static le_result_t result;
        static telux::tel::RegistrationStatus status;

        static void imsRegStateResponse(telux::tel::ImsRegistrationInfo info,
            telux::common::ErrorCode error);

#ifdef LE_CONFIG_FEATURE_ENHANCED_IMS
        static taf_radio_ImsSvcStatus_t voip;
        static taf_radio_ImsSvcStatus_t sms;
        static taf_radio_PdpError_t pdpError;

        static void imsSvcInfoResponse(telux::tel::ImsServiceInfo info,
            telux::common::ErrorCode error);
        static void imsPdpStatusResponse(telux::tel::ImsPdpStatusInfo status,
            telux::common::ErrorCode error);
#endif
    };

    class taf_RadioImsSettingCallback {
    public:
        static le_sem_Ref_t semaphore;
        static le_result_t result;
        static telux::tel::ImsServiceConfig config;
        static char sipUserAgentPtr[TAF_RADIO_IMS_USER_AGENT_BYTES];

        static void onRequestImsServiceConfig(SlotId slotId,
            telux::tel::ImsServiceConfig configType, telux::common::ErrorCode error);
        static void onResponseCallback(telux::common::ErrorCode error);
        static void onRequestImsSipUserAgentConfig(SlotId slotId, std::string sipUserAgent,
            telux::common::ErrorCode errorCode);
    };

     /*
     * @brief A cellular capability callback class must be provided when request for hardware
     *        capabilities.
     */
    class taf_RadioCellularCapsCallback : public telux::tel::ICellularCapabilityCallback {
    public:
        le_sem_Ref_t semaphore;
        le_result_t result;
        uint8_t simCount;
        uint8_t maxActiveSIM;
        std::vector<telux::tel::SimRatCapability> simRatCaps;
        std::vector<telux::tel::DeviceRatCapability> deviceRatCaps;
        /*
         * This function is called after getting the hardware capability.
         *
         * @param [in] signalStrength    The signal strength information.
         * @param [in] error             The error code of getting the signal strength.
         */
        void cellularCapabilityResponse( telux::tel::CellularCapabilityInfo  capabilityInfo,
            telux::common::ErrorCode error) override;
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

        taf_radio_Rat_t taf_radio_CovertRat(telux::tel::RadioTechnology rat);
        static void taf_radio_LayerImsRegStateHandler(void* reportPtr, void* layerHandlerFunc);
        static void taf_radio_LayerOpModeHandler(void* reportPtr, void* layerHandlerFunc);
        static void taf_radio_LayerNetRegStateHandler(void* reportPtr, void* layerHandlerFunc);
        static void taf_radio_LayerImsStateHandler(void* reportPtr, void* layerHandlerFunc);
        static void taf_radio_LayerSsHandler(void* reportPtr, void* layerHandlerFunc);
        static void taf_radio_LayerCellInfoHandler(void* reportPtr, void* layerHandlerFunc);
        static void taf_radio_LayerNetStatusHandler(void* reportPtr, void* layerHandlerFunc);
        static void taf_radio_LayerRatChangeHandler(void* reportPtr, void* layerHandlerFunc);
        static void taf_radio_LayerNetRejectHandler(void* reportPtr, void* layerHandlerFunc);

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
         * The initialization function of the Radio Service.
         */
        void Init(void);

        le_mem_PoolRef_t prefOpsListPool;
        le_mem_PoolRef_t prefOpPool;
        le_mem_PoolRef_t prefOpSafeRefPool;
        le_mem_PoolRef_t scanOpsListPool;
        le_mem_PoolRef_t scanOpPool;
        le_mem_PoolRef_t scanOpSafeRefPool;
        le_mem_PoolRef_t ngbrCellsPool;
        le_mem_PoolRef_t ngbrCellInfoPool;
        le_mem_PoolRef_t ngbrCellInfoSafeRefPool;
        le_mem_PoolRef_t metricsPool;
        le_mem_PoolRef_t phonePool;
        le_mem_PoolRef_t imsStatusChangePool;
        le_mem_PoolRef_t opModeChangePool;
        le_mem_PoolRef_t netRegStatePool;
        le_mem_PoolRef_t packSwStatePool;
        le_mem_PoolRef_t ssChangePool;
        le_mem_PoolRef_t cellInfoChangePool;
        le_mem_PoolRef_t ratChangePool;
        le_mem_PoolRef_t netStatusPool;
        le_mem_PoolRef_t netRegRejPool;

        le_ref_MapRef_t prefOpListRefMap;
        le_ref_MapRef_t prefOpSafeRefMap;
        le_ref_MapRef_t scanOpListRefMap;
        le_ref_MapRef_t scanOpSafeRefMap;
        le_ref_MapRef_t ngbrCellsRefMap;
        le_ref_MapRef_t ngbrCellInfoSafeRefMap;
        le_ref_MapRef_t metricsRefMap;
        le_ref_MapRef_t imsRefMap;
        le_ref_MapRef_t netStatusRefMap;

        le_event_Id_t imsRegStatusChangeId;
        le_event_Id_t opModeChangeId;
        le_event_Id_t netRegStateEvId;
        le_event_Id_t packSwStateEvId;
        le_event_Id_t imsStatusChangeId;
        le_event_Id_t gsmSsChangeEvId;
        le_event_Id_t umtsSsChangeEvId;
        le_event_Id_t cdmaSsChangeEvId;
        le_event_Id_t lteSsChangeEvId;
        le_event_Id_t nr5gSsChangeEvId;
        le_event_Id_t cellInfoChangeEvId;
        le_event_Id_t ratChangeEvId;
        le_event_Id_t netStatusEvId;
        le_event_Id_t netRegRejEvId;
        static le_event_Id_t radioCmdEvId;

        int32_t netRejectCause = TAF_RADIO_NET_REJ_CAUSE_UNDEFINED;
        taf_RadioDataCallbackInfo_t dataInfoCb;
        taf_OperatorNameCallbackInfo_t opNameCb;
        std::shared_ptr<taf_RadioSignalStrengthCallback> signalStrengthCb;
        std::shared_ptr<taf_RadioVoiceServiceStateCallback> voiceSrvStateCb;
        std::shared_ptr<taf_RadioSetOperatingModeCallback> setOperatingModeCb;
        std::shared_ptr<taf_RadioGetOperatingModeCallback> getOperatingModeCb;
        std::shared_ptr<taf_RadioCellularCapsCallback> cellularCapsCb;
        std::vector<std::shared_ptr<taf_RadioNetworkSelectionListener>> networkListeners;
        std::vector<std::shared_ptr<taf_RadioServSysListener>> servSysListeners;
        std::map<SlotId, std::shared_ptr<telux::tel::IImsServingSystemListener>> imsServSysListeners;
        std::map<SlotId, std::shared_ptr<telux::data::IServingSystemListener>> dataServSysListeners;
        std::vector<std::shared_ptr<telux::tel::IPhone>> phones;
        std::vector<std::shared_ptr<telux::tel::INetworkSelectionManager>> networkManagers;
        std::vector<std::shared_ptr<telux::tel::IServingSystemManager>> servingSystemManagers;
        std::shared_ptr<telux::tel::IPhoneManager> phoneManager;
        std::map<SlotId, std::shared_ptr<telux::tel::IImsServingSystemManager>> imsServingSystemMgrs;
        std::shared_ptr<telux::tel::IImsSettingsManager> imsSettingMgr;
        std::map<SlotId, std::shared_ptr<telux::data::IServingSystemManager>> dataServSysManagers;
        std::map<taf_radio_NetStatusChangeHandlerRef_t, taf_radio_NetStatusChangeHandlerRef_t> netStatRefMap;
        taf_radio_ImsRef_t imsRefs[TAF_RADIO_PHONE_NUM];
        taf_radio_NetStatusRef_t netStatusRefs[TAF_RADIO_PHONE_NUM];
        std::shared_ptr<taf_RadioPhoneListener> phoneListener;
        uint16_t hysteresisTimer[TAF_RADIO_PHONE_NUM] = {0,0};
        std::vector<taf_RadioHysteresisConfig_t> hysteresisConfigs;
    };
}
}

#endif /* #ifndef TAFRADIO_H */
