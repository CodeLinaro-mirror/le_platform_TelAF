/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_DIAG_BACKEND_HPP
#define TAF_DIAG_BACKEND_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#ifndef LE_CONFIG_DIAG_VSTACK
#include "tafUDSStack.h"
#endif

#include <map>

#ifdef LE_CONFIG_DIAG_VSTACK
#define MAX_INTERFACE_NAME_LEN 30
    // Enumeration of supported logical target address types.
    typedef enum
    {
        TAF_UDS_TA_TYPE_PHYSICAL   = 0x00,    ///< Physical addressing.
        TAF_UDS_TA_TYPE_FUNCTIONAL = 0x01     ///< Functional addressing.
    }taf_uds_TaType_t;

    // Logical address information structure in uds communication.
    typedef struct
    {
        uint16_t            sa;         ///< Source address of message senders.
        uint16_t            ta;         ///< Target address of message recipients.
        taf_uds_TaType_t    taType;      ///< Target address type of message recipients.
        uint16_t            vlanId;     ///< VLAN ID. =0 if the interface is not vlan port.
        char                ifName[MAX_INTERFACE_NAME_LEN]; ///< Interface name.
    }taf_uds_AddrInfo_t;
#endif

namespace tafsvc {
    #define TAF_STACK_CONFIG_PATH   "./tafDoIP.json"

    typedef enum
    {
        TAF_DIAG_SUCCESS = 0,                             ///< Successful.
        TAF_DIAG_SERVICE_NOT_SUPPORTED = 0x11,            ///< Service is not supported
        TAF_DIAG_SUBFUNCTION_NOT_SUPPORTED = 0x12,        ///< SubFunction is not supported.
        TAF_DIAG_INCORRECT_MSG_LEN_OR_INVALID_FORMAT = 0x13, ///< Message length not correct.
        TAF_DIAG_BUSY_REPEAT_REQUEST = 0x21,              ///< Busy repeat request.
        TAF_DIAG_CONDITION_NOT_CORRECT = 0x22,            ///< Prerequisite conditions not correct.
        TAF_DIAG_REQUEST_SEQUENCE_ERROR = 0x24,           ///< Request sequence error.
        TAF_DIAG_REQUEST_OUT_OF_RANGE = 0x31              ///< Parameter is out of range.
    }taf_diag_ErrorCode_t;

    typedef struct
    {
        uint16_t       vlanId;
        uint8_t        currentSesType; // Default value
        le_dls_Link_t  link;
    }taf_RxVlanCurrentSesType_t;

    // Define the interface for each service.
    class taf_UDSInterface
    {
        public:
            // UDS message handler
            virtual void UDSMsgHandler(const taf_uds_AddrInfo_t* addrPtr,
                uint8_t sid, uint8_t* msgPtr, size_t msgLen) = 0;
    };

    class taf_DiagBackend
    {
        public:
            taf_DiagBackend(){};
            ~taf_DiagBackend(){};

            static taf_DiagBackend& GetInstance();
            void Init();
#ifndef LE_CONFIG_DIAG_VSTACK
            // Registered to UDS stack for reception message.
            static void UdsIndicationHanler(const taf_uds_AddrInfo_t*         addrInfoPtr,
                const taf_uds_DiagMsg_t* diagMsgPtr, le_result_t result, void* userPtr);
#else
            //Registered to KPIT stack for reception messsage
            static void IntIndicationHandler( taf_diagBackend_DiagInfRef_t ref,
                const taf_diagBackend_AddrInfo_t *addrInfoPtr, uint8_t serviceID,
                    const uint8_t* msgPtr, size_t msgSize, void* contextPtr);
#endif

            // UDS layer service identifier.
            le_result_t RegisterUdsService(uint8_t sid, taf_UDSInterface* service);
            void UnregisterUdsService(uint8_t sid);

            // Can be used in each service to respond the request.
            le_result_t RespDiagPositive(uint8_t sid, const taf_uds_AddrInfo_t* addrInfoPtr,
                const uint8_t* dataPtr, size_t dataLen);
            le_result_t RespDiagPositive(uint8_t sid, const taf_uds_AddrInfo_t* addrInfoPtr);
            le_result_t RespDiagNegative(uint8_t sid, const taf_uds_AddrInfo_t* addrInfoPtr,
                    uint8_t nrc);

            // Internal function to check application requested VLAN id is valid or not.
            bool isVlanIdValid(uint16_t vlanId);
            // Internal function to get the cuurent session type per vlanID,
            // For non vlan case vlanId = 0.
            le_result_t GetCurrentSesType(uint16_t vlanId, uint8_t* currentSesTypePtr);

            le_dls_List_t VlanAndSesTypeList = LE_DLS_LIST_INIT;

        private:
            le_result_t InitUdsStack();
            void DeInitUdsStack();

            le_mem_PoolRef_t VlanAndSesTypeMemPool;
            void ClearUDSVlanList(le_dls_List_t* vlanIdListPtr);
#ifndef LE_CONFIG_DIAG_VSTACK
            taf_uds_DiagIndicationHandlerRef_t udsIndHandlerRef = NULL;
#else
            taf_diagBackend_DiagEventHandlerRef_t integrationIndHandlerRef = NULL;
#endif

            uint8_t regstFlag = 0;
            std::map<uint8_t, taf_UDSInterface *> svcMap;
#ifdef LE_CONFIG_DIAG_VSTACK
            std::map<uint8_t, taf_diagBackend_DiagInfRef_t> refMap;
#endif
    };
}
#endif
