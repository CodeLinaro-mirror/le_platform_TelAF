/*
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __ETS_STUB_IMPL__
#define __ETS_STUB_IMPL__

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#include <vsomeip/vsomeip.hpp>
#include <CommonAPI/CommonAPI.hpp>
#include <v1/someip/testability/ETSStubDefault.hpp>
#include <v1/someip/testability/ETSSecondaryServiceProxy.hpp>
#include <v1/someip/testability/ETSTestService1StubDefault.hpp>
#include <v1/someip/testability/ETSTestService2StubDefault.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>

using namespace ::v1::someip::testability;

class etsStubImpl: public ETSStubDefault {
    private:
        static std::mutex etsServiceMtx;
        static std::shared_ptr<etsStubImpl> etsServicePtr;
        std::vector<std::thread> appThreadPool;
        bool secondaryClientActive;
        std::shared_ptr<ETSSecondaryServiceProxy<>> secProxy;
        uint32_t clientServiceUnicastEventSubscrptionStatus;
        uint32_t clientServiceMulticastEventSubscrptionStatus;
        uint32_t clientServiceReliableEventSubscrptionStatus;
        bool isAvailableSecondary;
        uint8_t lastUnicastuINT8Value;
        uint8_t lastMulticastuINT8Value;
        uint32_t startTimeout;
        uint32_t durationTimeout;
        uint32_t debounceTimeout;
        bool TestService1Context1Registered;
        bool TestService1Context2Registered;
        bool TestService2Context1Registered;
        uint8_t lastuINT8ValueReliable;
    public:
        etsStubImpl();
        ~etsStubImpl();
        etsStubImpl(const etsStubImpl &obj) = delete;
        static std::shared_ptr<etsStubImpl> getEtsServiceInstance();
        void checkByteOrder(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _checkByteOrder_ReqArg1, uint16_t _checkByteOrder_ReqArg2, checkByteOrderReply_t _reply) override;    
        void clientServiceActivate(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _clientServiceActivate_ReqArg1) override;    
        void clientServiceDeactivate(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _clientServiceDeactivate_ReqArg1) override;    
        void clientServiceGetLastValueOfEventUDPMulticast(const std::shared_ptr<CommonAPI::ClientId> _client, clientServiceGetLastValueOfEventUDPMulticastReply_t _reply) override;    
        void clientServiceGetLastValueOfEventUDPUnicast(const std::shared_ptr<CommonAPI::ClientId> _client, clientServiceGetLastValueOfEventUDPUnicastReply_t _reply) override;    
        void clientServiceSubscribeEventgroup(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _clientServiceSubscribeEventgroup_ReqArg1, uint32_t _clientServiceSubscribeEventgroup_ReqArg2) override;    
        void echoBitfields(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _echoBitfields_ReqArg1, uint16_t _echoBitfields_ReqArg2, uint32_t _echoBitfields_ReqArg3, echoBitfieldsReply_t _reply) override;    
        void echoCommonDatatypes(const std::shared_ptr<CommonAPI::ClientId> _client, bool _EchoCommonDatatypes_ReqArg1, uint8_t _EchoCommonDatatypes_ReqArg2, uint16_t _EchoCommonDatatypes_ReqArg3, uint32_t _EchoCommonDatatypes_ReqArg4, int8_t _EchoCommonDatatypes_ReqArg5, int16_t _EchoCommonDatatypes_ReqArg6, int32_t _EchoCommonDatatypes_ReqArg7, float _EchoCommonDatatypes_ReqArg8, double _EchoCommonDatatypes_ReqArg9, echoCommonDatatypesReply_t _reply) override;    
        void echoENUM(const std::shared_ptr<CommonAPI::ClientId> _client, ETS::Enum _echoENUM_ReqArg1, echoENUMReply_t _reply) override;    
        void echoFLOAT64(const std::shared_ptr<CommonAPI::ClientId> _client, double _echoFLOAT64_ReqArg1, echoFLOAT64Reply_t _reply) override;    
        void echoINT8(const std::shared_ptr<CommonAPI::ClientId> _client, int8_t _echoINT8_ReqArg1, echoINT8Reply_t _reply) override;    
        void echoStaticUINT8Array(const std::shared_ptr<CommonAPI::ClientId> _client, std::vector< uint8_t > _echoStaticUINT8Array_ReqArg1, echoStaticUINT8ArrayReply_t _reply) override;    
        void echoUINT8(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _echoUINT8_ReqArg1, echoUINT8Reply_t _reply) override;    
        void echoUINT8Array(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _echoUINT8Array_ReqArg1, std::vector< uint8_t > _echoUINT8Array_ReqArg2, echoUINT8ArrayReply_t _reply) override;    
        void echoUINT8Array16BitLength(const std::shared_ptr<CommonAPI::ClientId> _client, uint16_t _echoUINT8Array16BitLength_ReqArg1, std::vector< uint8_t > _echoUINT8Array16BitLength_ReqArg2, echoUINT8Array16BitLengthReply_t _reply) override;    
        void echoUINT8Array2Dim(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _echoUINT8Array2Dim_ReqArg1, std::vector< ETS::uint8ArrayArray > _echoUINT8Array2Dim_ReqArg2, echoUINT8Array2DimReply_t _reply) override;    
        void echoUINT8Array8BitLength(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _echoUINT8Array8BitLength_ReqArg1, std::vector< uint8_t > _echoUINT8Array8BitLength_ReqArg2, echoUINT8Array8BitLengthReply_t _reply) override;    
        void echoUINT8ArrayMinSize(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _echoUINT8ArrayMinSize_ReqArg1, std::vector< uint8_t > _echoUINT8ArrayMinSize_ReqArg2, echoUINT8ArrayMinSizeReply_t _reply) override;    
        void echoUTF16DYNAMIC(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _echoUTF16DYNAMIC_ReqArg1, std::string _echoUTF16DYNAMIC_ReqArg2, echoUTF16DYNAMICReply_t _reply) override;    
        void echoUTF16FIXED(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _echoUTF16FIXED_ReqArg1, echoUTF16FIXEDReply_t _reply) override;    
        void echoUTF8DYNAMIC(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _echoUTF8DYNAMIC_ReqArg1, std::string _echoUTF8DYNAMIC_ReqArg2, echoUTF8DYNAMICReply_t _reply) override;    
        void echoUTF8FIXED(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _echoUTF8FIXED_ReqArg1, echoUTF8FIXEDReply_t _reply) override;    
        void resetInterface(const std::shared_ptr<CommonAPI::ClientId> _client) override;    
        void suspendInterface(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _suspendInterface_ReqArg1, uint32_t _suspendInterface_ReqArg2) override;    
        void triggerEventUINT8(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT8_ReqArg1, uint32_t _triggerEventUINT8_ReqArg2, uint32_t _triggerEventUINT8_ReqArg3) override;    
        void triggerEventUINT8Array(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT8Array_ReqArg1, uint32_t _triggerEventUINT8Array_ReqArg2, uint32_t _triggerEventUINT8Array_ReqArg3) override;    
        void triggerEventUINT8E2E(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT8E2E_ReqArg1, uint32_t _triggerEventUINT8E2E_ReqArg2, uint32_t _triggerEventUINT8E2E_ReqArg3) override;    
        void triggerEventUINT8Multicast(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT8Multicast_ReqArg1, uint32_t _triggerEventUINT8Multicast_ReqArg2, uint32_t _triggerEventUINT8Multicast_ReqArg3) override;
        void echoUINT8E2E(const std::shared_ptr<CommonAPI::ClientId> _client, uint16_t _echoUINT8E2E_ReqArg1, uint16_t _echoUINT8E2E_ReqArg2, uint32_t _echoUINT8E2E_ReqArg3, uint32_t _echoUINT8E2E_ReqArg4, uint8_t _echoUINT8E2E_ReqArg5, echoUINT8E2EReply_t _reply) override;
        void echoUINT8ArrayLengthTP(const std::shared_ptr<CommonAPI::ClientId> _client, std::vector< uint8_t > _inUINT8Array_ReqArg1, echoUINT8ArrayLengthTPReply_t _reply) override;
        void echoUINT8ArrayLengthTPNoResponse(const std::shared_ptr<CommonAPI::ClientId> _client, std::vector< uint8_t > _echoUINT8ArrayLengthTPNoResponse_ReqArg1) override;
        void echoUINT8ArrayLengthInTP(const std::shared_ptr<CommonAPI::ClientId> _client, std::vector< uint8_t > _inUINT8Array_ReqArg1, echoUINT8ArrayLengthInTPReply_t _reply) override;
        void echoUINT8ArrayLengthOutTP(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _inUINT32_ReqArg1, echoUINT8ArrayLengthOutTPReply_t _reply) override;
        void triggerEventUINT8ArrayTP(const std::shared_ptr<CommonAPI::ClientId> _client, std::vector< uint8_t > _triggerEventUINT8ArrayTP_ReqArg1) override;
        void triggerEventUINT8ArrayTPNoReqTPPayload(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT32_ReqArg1) override;
        void triggerEventUINT32Periodic(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT32_ReqArg1) override;
        void triggerEventUINT32UpdateOnChange(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT32_ReqArg1) override;
        void activateTestSerivce(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _activateTestSerivce_ReqArg1, uint32_t _activateTestSerivce_ReqArg2);
        void deactivateTestSerivce(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _activateTestSerivce_ReqArg1, uint32_t _activateTestSerivce_ReqArg2);
        void echoUINT8RELIABLE(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _echoUINT8RELIABLE_ReqArg1, echoUINT8RELIABLEReply_t _reply);
        void triggerEventUINT8Reliable(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT8Reliable_ReqArg1, uint32_t _triggerEventUINT8Reliable_ReqArg2, uint32_t _triggerEventUINT8Reliable_ReqArg3);
        void clientServiceGetLastValueOfEventTCP(const std::shared_ptr<CommonAPI::ClientId> _client, clientServiceGetLastValueOfEventTCPReply_t _reply);
};

class etsStubImplService1: public ETSTestService1StubDefault {
    public:
        void echoUINT32(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _inUINT32_ReqArg1, echoUINT32Reply_t _reply);
};

class etsStubImplService2: public ETSTestService2StubDefault {
    public:
        void echoUINT32(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _inUINT32_ReqArg1, echoUINT32Reply_t _reply);
};

#endif
