/*
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "etsStubImpl.hpp"

std::mutex etsStubImpl::etsServiceMtx;
std::shared_ptr<etsStubImpl> etsStubImpl::etsServicePtr = nullptr;

etsStubImpl::etsStubImpl() {
    secondaryClientActive = false;
    clientServiceUnicastEventSubscrptionStatus = 0;
    clientServiceMulticastEventSubscrptionStatus = 0;
    clientServiceReliableEventSubscrptionStatus = 0;
    isAvailableSecondary = 0;
    lastUnicastuINT8Value = 0;
    lastMulticastuINT8Value = 0;
    startTimeout = 0;
    durationTimeout = 0;
    debounceTimeout = 0;
    TestService1Context1Registered = false;
    TestService1Context2Registered = false;
    TestService2Context1Registered = false;
    lastuINT8ValueReliable = 0;
}

etsStubImpl::~etsStubImpl() {
    for (auto& thread : appThreadPool) {
        thread.join();
    }
}

std::shared_ptr<etsStubImpl> etsStubImpl::getEtsServiceInstance() {
    if (nullptr == etsServicePtr) {
        etsServiceMtx.lock();
        if (nullptr == etsServicePtr) {
            etsServicePtr = std::make_shared<etsStubImpl>();
        }
        etsServiceMtx.unlock();
    }

    return etsServicePtr;
}

void etsStubImpl::checkByteOrder(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _checkByteOrder_ReqArg1, uint16_t _checkByteOrder_ReqArg2, checkByteOrderReply_t _reply) {
    uint32_t checkByteOrder_ResArg1 = 0;
    std::cout << "etsStubImpl::" << __func__ << " Received: _checkByteOrder_ReqArg1:" << std::hex << _checkByteOrder_ReqArg1
        << " _checkByteOrder_ReqArg2:" << std::hex << _checkByteOrder_ReqArg2 << std::endl;
    checkByteOrder_ResArg1 = (uint32_t)_checkByteOrder_ReqArg1 + (uint32_t)_checkByteOrder_ReqArg2;
    std::cout << "Sum of input arguments:" << std::hex <<  checkByteOrder_ResArg1 << std::endl;
    _reply(checkByteOrder_ResArg1);
    return;
}

void etsStubImpl::clientServiceActivate(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _clientServiceActivate_ReqArg1) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    startTimeout = (uint32_t)_clientServiceActivate_ReqArg1;

    appThreadPool.emplace_back([&]{
        uint32_t start = startTimeout;
        std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
        std::string domain = "local";
        std::string instance = "someip.testability.ETSSecondaryService";
        std::string connection = "ets_secondary_client";

        std::cout << "etsStubImpl::clientServiceActivate start timeout:" << start << std::endl; 
        while (start>0) {
            std::cout << "etsStubImpl::clientServiceActivate wait for start timeout " << start << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            start = start-1;
        }

        std::cout << "etsStubImpl::clientServiceActivate starting Secondary Client Proxy" << std::endl;

        secondaryClientActive = true;
        secProxy = runtime->buildProxy<ETSSecondaryServiceProxy>(domain, instance, connection);

        secProxy->getProxyStatusEvent().subscribe([&](const CommonAPI::AvailabilityStatus& val) {
            if (val == CommonAPI::AvailabilityStatus::AVAILABLE) {
                isAvailableSecondary = true;
                std::cout << "ETS Secondary Service available" << std::endl;
            }
            else {
                isAvailableSecondary = false;
                std::cout << "ETS Secondary Service not available" << std::endl;
            }
        });

        std::cout << "Checking availability of secondary service!" << std::endl;
        while (secondaryClientActive && !secProxy->isAvailable()) {
            std::cout << "Secondary Service not available, trying again in 100 milliseconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "ETS Secondary Service Available..." << std::endl;

        while (secondaryClientActive) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cout << "ETS Secondary Service Exit..." << std::endl;

    });

    return;
}

void etsStubImpl::clientServiceDeactivate(const std::shared_ptr<CommonAPI::ClientId> _client,
                                            uint8_t _clientServiceDeactivate_ReqArg1) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    if (secondaryClientActive) {
        secondaryClientActive = false;
        std::cout << "etsStubImpl::clientServiceDeactivate -> Send stop subscribe for unicast event" << std::endl;
        secProxy->getSecondaryEventUINT8Event().unsubscribe(clientServiceUnicastEventSubscrptionStatus);
        std::cout << "etsStubImpl::clientServiceDeactivate -> Send stop subscribe for multicast event" << std::endl;
        secProxy->getSecondaryMulticastEventUINT8Event().unsubscribe(clientServiceMulticastEventSubscrptionStatus);
        std::cout << "etsStubImpl::clientServiceDeactivate -> Send stop subscribe for reliable event" << std::endl;
        secProxy->getSecondaryEventUINT8ReliableEvent().unsubscribe(clientServiceReliableEventSubscrptionStatus);

    }
    else {
        std::cout << "etsStubImpl::" << __func__ << " Client service not yet active" << std::endl;
    }
    return;
}

void etsStubImpl::clientServiceGetLastValueOfEventUDPMulticast(const std::shared_ptr<CommonAPI::ClientId> _client,
                                                            clientServiceGetLastValueOfEventUDPMulticastReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << " lastMulticastuINT8Value:" << std::hex << (uint32_t)lastMulticastuINT8Value << std::endl;
    uint8_t lastval = lastMulticastuINT8Value;
    _reply(lastval);
    return;
}

void etsStubImpl::clientServiceGetLastValueOfEventUDPUnicast(const std::shared_ptr<CommonAPI::ClientId> _client,
                                                            clientServiceGetLastValueOfEventUDPUnicastReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << " lastUnicastuINT8Value:" << std::hex << (uint32_t)lastUnicastuINT8Value << std::endl;
    uint8_t lastval = lastUnicastuINT8Value;
    _reply(lastval);
    return;
}

void etsStubImpl::clientServiceSubscribeEventgroup(const std::shared_ptr<CommonAPI::ClientId> _client,
                                                    uint32_t _clientServiceSubscribeEventgroup_ReqArg1,
                                                    uint32_t _clientServiceSubscribeEventgroup_ReqArg2) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    startTimeout = _clientServiceSubscribeEventgroup_ReqArg1;
    durationTimeout = _clientServiceSubscribeEventgroup_ReqArg2;

    appThreadPool.emplace_back([&]{
        uint32_t start = startTimeout;
        uint32_t duration = durationTimeout;
        std::cout << "etsStubImpl::clientServiceSubscribeEventgroup start timeout:" << start << std::endl; 

        while (start>0) {
            std::cout << "etsStubImpl::clientServiceSubscribeEventgroup wait for start timeout " << start << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            start = start-1;
        }

        clientServiceUnicastEventSubscrptionStatus = secProxy->getSecondaryEventUINT8Event().subscribe([&](const uint8_t& uINT8Value) {
            std::cout << "etsStubImpl::clientServiceSubscribeEventgroup SecondaryEventUINT8 Notification:" << std::hex << (uint32_t)uINT8Value << std::endl;
            lastUnicastuINT8Value = uINT8Value;
        });

        clientServiceMulticastEventSubscrptionStatus = secProxy->getSecondaryMulticastEventUINT8Event().subscribe([&](const uint8_t& uINT8Value) {
            std::cout << "etsStubImpl::clientServiceSubscribeEventgroup SecondaryMulticastEventUINT8 Notification:" << std::hex << (uint32_t)uINT8Value << std::endl;
            lastMulticastuINT8Value = uINT8Value;
        });

        clientServiceReliableEventSubscrptionStatus = secProxy->getSecondaryEventUINT8ReliableEvent().subscribe([&](const uint8_t& uINT8ValueReliable) {
            std::cout << "etsStubImpl::clientServiceSubscribeEventgroup SecondaryEventUINT8Reliable Notification:" << std::hex << (uint32_t)uINT8ValueReliable << std::endl;
            lastuINT8ValueReliable = uINT8ValueReliable;
        });

        std::cout << "etsStubImpl::clientServiceSubscribeEventgroup duration timeout value:" << duration << std::endl;
        while (duration > 0) {
            std::cout << "etsStubImpl::clientServiceSubscribeEventgroup wait for duration timeout" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            duration = duration-1;
        }

        std::cout << "etsStubImpl::clientServiceSubscribeEventgroup duration timeout" << std::endl;
        std::cout << "etsStubImpl::clientServiceSubscribeEventgroup -> Send stop subscribe for unicast event" << std::endl;
        secProxy->getSecondaryEventUINT8Event().unsubscribe(clientServiceUnicastEventSubscrptionStatus);
        std::cout << "etsStubImpl::clientServiceSubscribeEventgroup -> Send stop subscribe for multicast event" << std::endl;
        secProxy->getSecondaryMulticastEventUINT8Event().unsubscribe(clientServiceMulticastEventSubscrptionStatus);
        std::cout << "etsStubImpl::clientServiceSubscribeEventgroup -> Send stop subscribe for reliable event" << std::endl;
        secProxy->getSecondaryEventUINT8ReliableEvent().unsubscribe(clientServiceReliableEventSubscrptionStatus);
    });

    return;
}

void etsStubImpl::echoBitfields(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _echoBitfields_ReqArg1,
                                uint16_t _echoBitfields_ReqArg2, uint32_t _echoBitfields_ReqArg3, echoBitfieldsReply_t _reply) {
    int idx;
    uint8_t echoBitfields_ResArg1 = 0;
    uint16_t echoBitfields_ResArg2 = 0;
    uint32_t echoBitfields_ResArg3 = 0;

    std::cout << "etsStubImpl::" << __func__ << " _echoBitfields_ReqArg1:" << std::hex << (uint32_t)_echoBitfields_ReqArg1
            << " _echoBitfields_ReqArg2:" << std::hex << (uint32_t)_echoBitfields_ReqArg2
            << " _echoBitfields_ReqArg3:" << std::hex << (uint32_t)_echoBitfields_ReqArg3 << std::endl;


    for (idx=0; idx<8; idx++) {
        echoBitfields_ResArg1 |= ((_echoBitfields_ReqArg1 >> idx) & 0x1);
        if (idx < 7) { /* Avoid shif for 8th iteration */
            echoBitfields_ResArg1 = echoBitfields_ResArg1 << 1;
        }
    }

    for (idx=0; idx<16; idx++) {
        echoBitfields_ResArg2 |= ((_echoBitfields_ReqArg2 >> idx) & 0x1);
        if (idx < 15) { /* Avoid shif for 16th iteration */
            echoBitfields_ResArg2 = echoBitfields_ResArg2 << 1;
        }
    }

    for (idx=0; idx<32; idx++) {
        echoBitfields_ResArg3 |= ((_echoBitfields_ReqArg3 >> idx) & 0x1);
        if (idx < 31) { /* Avoid shif for 32nd iteration */
            echoBitfields_ResArg3 = echoBitfields_ResArg3 << 1;
        }
    }

    std::cout << "etsStubImpl::" << __func__ << " echoBitfields_ResArg1:" << std::hex << (uint32_t)echoBitfields_ResArg1
            << " echoBitfields_ResArg2:" << std::hex << (uint32_t)echoBitfields_ResArg2
            << " echoBitfields_ResArg3:" << std::hex << (uint32_t)echoBitfields_ResArg3 << std::endl;

    _reply(echoBitfields_ResArg1, echoBitfields_ResArg2, echoBitfields_ResArg3);

    return;
}

void etsStubImpl::echoCommonDatatypes(const std::shared_ptr<CommonAPI::ClientId> _client,
                                    bool _EchoCommonDatatypes_ReqArg1, uint8_t _EchoCommonDatatypes_ReqArg2,
                                    uint16_t _EchoCommonDatatypes_ReqArg3, uint32_t _EchoCommonDatatypes_ReqArg4,
                                    int8_t _EchoCommonDatatypes_ReqArg5, int16_t _EchoCommonDatatypes_ReqArg6,
                                    int32_t _EchoCommonDatatypes_ReqArg7, float _EchoCommonDatatypes_ReqArg8,
                                    double _EchoCommonDatatypes_ReqArg9, echoCommonDatatypesReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;

    double EchoCommonDatatypes_ResArg1 = _EchoCommonDatatypes_ReqArg9;
    float EchoCommonDatatypes_ResArg2 = _EchoCommonDatatypes_ReqArg8;
    int32_t EchoCommonDatatypes_ResArg3 = _EchoCommonDatatypes_ReqArg7;
    int16_t EchoCommonDatatypes_ResArg4 = _EchoCommonDatatypes_ReqArg6;
    int8_t EchoCommonDatatypes_ResArg5 = _EchoCommonDatatypes_ReqArg5;
    uint32_t EchoCommonDatatypes_ResArg6 = _EchoCommonDatatypes_ReqArg4;
    uint16_t EchoCommonDatatypes_ResArg7 = _EchoCommonDatatypes_ReqArg3;
    uint8_t EchoCommonDatatypes_ResArg8 = _EchoCommonDatatypes_ReqArg2;
    bool EchoCommonDatatypes_ResArg9 = _EchoCommonDatatypes_ReqArg1;

    _reply(EchoCommonDatatypes_ResArg1, EchoCommonDatatypes_ResArg2, EchoCommonDatatypes_ResArg3,
        EchoCommonDatatypes_ResArg4, EchoCommonDatatypes_ResArg5, EchoCommonDatatypes_ResArg6,
        EchoCommonDatatypes_ResArg7,EchoCommonDatatypes_ResArg8, EchoCommonDatatypes_ResArg9);

    return;
}

void etsStubImpl::echoENUM(const std::shared_ptr<CommonAPI::ClientId> _client, ETS::Enum _echoENUM_ReqArg1, echoENUMReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    ETS::Enum echoENUM_ResArg1 = _echoENUM_ReqArg1;
    _reply(echoENUM_ResArg1);
    return;
}

void etsStubImpl::echoFLOAT64(const std::shared_ptr<CommonAPI::ClientId> _client, double _echoFLOAT64_ReqArg1, echoFLOAT64Reply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    double echoFLOAT64_ResArg1;
    echoFLOAT64_ResArg1 = _echoFLOAT64_ReqArg1;
    _reply(echoFLOAT64_ResArg1);
    return;
}

void etsStubImpl::echoINT8(const std::shared_ptr<CommonAPI::ClientId> _client, int8_t _echoINT8_ReqArg1, echoINT8Reply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    int8_t echoINT8_ResArg1 = _echoINT8_ReqArg1;
    _reply(echoINT8_ResArg1);
    return;
}

void etsStubImpl::echoStaticUINT8Array(const std::shared_ptr<CommonAPI::ClientId> _client, std::vector< uint8_t > _echoStaticUINT8Array_ReqArg1, echoStaticUINT8ArrayReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    std::vector< uint8_t > echoStaticUINT8Array_ResArg1;
    echoStaticUINT8Array_ResArg1.assign(_echoStaticUINT8Array_ReqArg1.begin(), _echoStaticUINT8Array_ReqArg1.end());
    _reply(echoStaticUINT8Array_ResArg1);
    return;
}

void etsStubImpl::echoUINT8(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _echoUINT8_ReqArg1, echoUINT8Reply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    uint8_t echoUINT8_ResArg1 = _echoUINT8_ReqArg1;
    _reply(echoUINT8_ResArg1);
    return;
}

void etsStubImpl::echoUINT8Array(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _echoUINT8Array_ReqArg1, std::vector< uint8_t > _echoUINT8Array_ReqArg2, echoUINT8ArrayReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    uint32_t echoUINT8Array_ResArg1 = _echoUINT8Array_ReqArg1;
    std::vector< uint8_t > echoUINT8Array_ResArg2;
    echoUINT8Array_ResArg2.assign(_echoUINT8Array_ReqArg2.begin(), _echoUINT8Array_ReqArg2.end());
    _reply(echoUINT8Array_ResArg1, echoUINT8Array_ResArg2);
    return;
}

void etsStubImpl::echoUINT8Array16BitLength(const std::shared_ptr<CommonAPI::ClientId> _client, uint16_t _echoUINT8Array16BitLength_ReqArg1, std::vector< uint8_t > _echoUINT8Array16BitLength_ReqArg2, echoUINT8Array16BitLengthReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    uint16_t echoUINT8Array16BitLength_ResArg1 = _echoUINT8Array16BitLength_ReqArg1;
    std::vector< uint8_t > echoUINT8Array16BitLength_ResArg2;
    echoUINT8Array16BitLength_ResArg2.assign(_echoUINT8Array16BitLength_ReqArg2.begin(), _echoUINT8Array16BitLength_ReqArg2.end());
    _reply(echoUINT8Array16BitLength_ResArg1, echoUINT8Array16BitLength_ResArg2);
    return;
}

void etsStubImpl::echoUINT8Array2Dim(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _echoUINT8Array2Dim_ReqArg1, std::vector< ETS::uint8ArrayArray > _echoUINT8Array2Dim_ReqArg2, echoUINT8Array2DimReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    uint32_t echoUINT8Array2Dim_ResArg1 = _echoUINT8Array2Dim_ReqArg1;
    std::vector< ETS::uint8ArrayArray > echoUINT8Array2Dim_ResArg2;
    echoUINT8Array2Dim_ResArg2.assign(_echoUINT8Array2Dim_ReqArg2.begin(), _echoUINT8Array2Dim_ReqArg2.end());
    _reply(echoUINT8Array2Dim_ResArg1, echoUINT8Array2Dim_ResArg2);
    return;
}

void etsStubImpl::echoUINT8Array8BitLength(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _echoUINT8Array8BitLength_ReqArg1, std::vector< uint8_t > _echoUINT8Array8BitLength_ReqArg2, echoUINT8Array8BitLengthReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    uint8_t echoUINT8Array8BitLength_ResArg1 = _echoUINT8Array8BitLength_ReqArg1;
    std::vector< uint8_t > echoUINT8Array8BitLength_ResArg2;
    echoUINT8Array8BitLength_ResArg2.assign(_echoUINT8Array8BitLength_ReqArg2.begin(), _echoUINT8Array8BitLength_ReqArg2.end());
    _reply(echoUINT8Array8BitLength_ResArg1, echoUINT8Array8BitLength_ResArg2);
    return;
}

void etsStubImpl::echoUINT8ArrayMinSize(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _echoUINT8ArrayMinSize_ReqArg1, std::vector< uint8_t > _echoUINT8ArrayMinSize_ReqArg2, echoUINT8ArrayMinSizeReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    uint32_t echoUINT8ArrayMinSize_ResArg1 = _echoUINT8ArrayMinSize_ReqArg1;
    std::vector< uint8_t > echoUINT8ArrayMinSize_ResArg2;
    echoUINT8ArrayMinSize_ResArg2.assign(_echoUINT8ArrayMinSize_ReqArg2.begin(), _echoUINT8ArrayMinSize_ReqArg2.end());
    _reply(echoUINT8ArrayMinSize_ResArg1, echoUINT8ArrayMinSize_ResArg2);
    return;
}

void etsStubImpl::echoUTF16DYNAMIC(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _echoUTF16DYNAMIC_ReqArg1,
                                    std::string _echoUTF16DYNAMIC_ReqArg2, echoUTF16DYNAMICReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    uint32_t echoUTF16DYNAMIC_ResArg1 = _echoUTF16DYNAMIC_ReqArg1;
    std::string echoUTF16DYNAMIC_ResArg2 = _echoUTF16DYNAMIC_ReqArg2;                                                  
    _reply(echoUTF16DYNAMIC_ResArg1, echoUTF16DYNAMIC_ResArg2);
    return;
}

void etsStubImpl::echoUTF16FIXED(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _echoUTF16FIXED_ReqArg1, echoUTF16FIXEDReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << " _echoUTF16FIXED_ReqArg1:" << _echoUTF16FIXED_ReqArg1 << std::endl;
    std::string echoUTF16FIXED_ResArg1 = _echoUTF16FIXED_ReqArg1;
    _reply(echoUTF16FIXED_ResArg1);
    return;
}

void etsStubImpl::echoUTF8DYNAMIC(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _echoUTF8DYNAMIC_ReqArg1,
                                std::string _echoUTF8DYNAMIC_ReqArg2, echoUTF8DYNAMICReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << " _echoUTF8DYNAMIC_ReqArg1:" << _echoUTF8DYNAMIC_ReqArg1
            << " _echoUTF8DYNAMIC_ReqArg2:" << _echoUTF8DYNAMIC_ReqArg2 << std::endl;
    uint32_t echoUTF8DYNAMIC_ResArg1 =_echoUTF8DYNAMIC_ReqArg1;
    std::string echoUTF8DYNAMIC_ResArg2 =_echoUTF8DYNAMIC_ReqArg2;
    _reply(echoUTF8DYNAMIC_ResArg1, echoUTF8DYNAMIC_ResArg2);
    return;
}

void etsStubImpl::echoUTF8FIXED(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _echoUTF8FIXED_ReqArg1, echoUTF8FIXEDReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << " _echoUTF8FIXED_ReqArg1:" << _echoUTF8FIXED_ReqArg1 << std::endl;
    std::string echoUTF8FIXED_ResArg1 = _echoUTF8FIXED_ReqArg1;
    _reply(echoUTF8FIXED_ResArg1);
    return;
}

void etsStubImpl::resetInterface(const std::shared_ptr<CommonAPI::ClientId> _client) {
    uint8_t TestFieldUINT8 = 0;
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    setTestFieldUINT8Attribute(TestFieldUINT8);
    return;
}

void etsStubImpl::suspendInterface(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _suspendInterface_ReqArg1, uint32_t _suspendInterface_ReqArg2) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    startTimeout = _suspendInterface_ReqArg1;
    durationTimeout = _suspendInterface_ReqArg2;

    appThreadPool.emplace_back([&]{
        uint32_t start = startTimeout;
        uint32_t duration = durationTimeout;
        int retry_counter = 0;
        std::string domain = "local";
        std::string instance = "someip.testability.ETS";
        std::string connection = "ets_default_service";
        std::shared_ptr<etsStubImpl> etsService = etsStubImpl::getEtsServiceInstance();
        std::cout << "etsStubImpl::suspendInterface start timeout:" << start << std::endl;
        while (start > 0) {
            std::cout << "etsStubImpl::suspendInterface wait for start timeout " << start << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            start = start-1;
        }

        /* Unregister Service */
        bool successfullydeRegistered = CommonAPI::Runtime::get()->unregisterService(domain, ETS::getInterface(), instance);
        while (!successfullydeRegistered && retry_counter<10) {
            std::cout << "etsStubImpl::" << __func__ << "Service Deregistration failed, trying again in 100 milliseconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            successfullydeRegistered = CommonAPI::Runtime::get()->unregisterService(domain, ETS::getInterface(), instance);
            ++retry_counter;
        }
        if (retry_counter<10) {
            std::cout << "etsStubImpl::" << __func__ << "Successfully DeRegistered ETS Service!" << std::endl;
            while (duration > 0) {
                std::cout << "etsStubImpl::suspendInterface wait for duration timeout " << duration << std::endl;
                duration = duration-1;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            /* Register Service */
            retry_counter = 0;
            uint8_t TestFieldUINT8 = 0;
            setTestFieldUINT8Attribute(TestFieldUINT8);
            successfullydeRegistered = CommonAPI::Runtime::get()->registerService(domain, instance, etsService, connection);
            while (!successfullydeRegistered && retry_counter<10) {
                std::cout << "Register Service failed, trying again in 100 milliseconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                successfullydeRegistered = CommonAPI::Runtime::get()->registerService(domain, instance, etsService, connection);
                ++retry_counter;
            }
            if (retry_counter<10) {
                std::cout << "etsStubImpl::" << __func__ << "Successfully Registered ETS Service" << std::endl;
            }
            else {
                std::cout << "etsStubImpl::" << __func__ << "Registration Of ETS Service Failed" << std::endl;
            }
        }
        else {
            std::cout << "etsStubImpl::" << __func__ << "DeRegistration Of ETS Service Failed!" << std::endl;
        }
    });

    return;
}

void etsStubImpl::triggerEventUINT8(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT8_ReqArg1,
                                     uint32_t _triggerEventUINT8_ReqArg2, uint32_t _triggerEventUINT8_ReqArg3) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    startTimeout = _triggerEventUINT8_ReqArg1;
    durationTimeout = _triggerEventUINT8_ReqArg2;
    debounceTimeout = _triggerEventUINT8_ReqArg3;

    appThreadPool.emplace_back([&]{
        uint32_t start = startTimeout;
        uint32_t duration = durationTimeout;
        uint32_t debounce = debounceTimeout;
        uint8_t testEventUINT8Val = 0;
        std::cout << "etsStubImpl::triggerEventUINT8 start timeout:" << start << std::endl;
        while (start > 0) {
            std::cout << "etsStubImpl::triggerEventUINT8 wait for start timeout " << start << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            start = start-1;
        }

        while (duration > 0) {
            std::cout << "etsStubImpl::triggerEventUINT8:" << (uint32_t)testEventUINT8Val << std::endl;
            fireTestEventUINT8Event(testEventUINT8Val);
            duration = duration-debounce;
            testEventUINT8Val = testEventUINT8Val+(uint8_t)1;
            std::this_thread::sleep_for(std::chrono::seconds(debounce));
        }
    });

    return;
}

void etsStubImpl::triggerEventUINT8Array(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT8Array_ReqArg1,
                                        uint32_t _triggerEventUINT8Array_ReqArg2, uint32_t _triggerEventUINT8Array_ReqArg3) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    startTimeout = _triggerEventUINT8Array_ReqArg1;
    durationTimeout = _triggerEventUINT8Array_ReqArg2;
    debounceTimeout = _triggerEventUINT8Array_ReqArg3;

    appThreadPool.emplace_back([&]{
        uint32_t start = startTimeout;
        uint32_t duration = durationTimeout;
        uint32_t debounce = debounceTimeout;
        std::vector< uint8_t > uINT8Array;
        uint32_t range = duration*debounce;
        /* substracted 4 bytes for reserving space for array length field */
        if (range < 4) {
            std::cerr << "etsStubImpl::triggerEventUINT8Array invalid input for array test" << std::endl;
        }
        range = range-4;
        for (uint32_t idx=0; idx<range; idx++) {
            uINT8Array.push_back(0xab);
        }
        std::cout << "etsStubImpl::triggerEventUINT8Array start timeout:" << start << std::endl;
        while (start > 0) {
            std::cout << "etsStubImpl::triggerEventUINT8Array wait for start timeout " << start << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            start = start-1;
        }

        while (duration > 0) {
            std::cout << "etsStubImpl::triggerEventUINT8Array" << std::endl;
            fireTestEventUINT8ArrayEvent(uINT8Array);
            duration = duration-debounce;
            std::this_thread::sleep_for(std::chrono::seconds(debounce));
        }
    });

    return;
}
    
void etsStubImpl::triggerEventUINT8E2E(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT8E2E_ReqArg1,
                                        uint32_t _triggerEventUINT8E2E_ReqArg2, uint32_t _triggerEventUINT8E2E_ReqArg3) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    startTimeout = _triggerEventUINT8E2E_ReqArg1;
    durationTimeout = _triggerEventUINT8E2E_ReqArg2;
    debounceTimeout = _triggerEventUINT8E2E_ReqArg3;

    appThreadPool.emplace_back([&]{
        uint32_t start = startTimeout;
        uint32_t duration = durationTimeout;
        uint32_t debounce = debounceTimeout;
        uint16_t length = 0x0;
        uint16_t counter = 0x0;
        uint32_t dataID = 0x0;
        uint32_t CRC = 0x0;
        uint8_t uINT8Value = 0x24;
        std::cout << "etsStubImpl::triggerEventUINT8E2E start timeout:" << start << std::endl;
        while (start > 0) {
            std::cout << "etsStubImpl::triggerEventUINT8E2E wait for start timeout " << start << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            start = start-1;
        }

        while (duration > 0) {
            std::cout << "etsStubImpl::triggerEventUINT8E2E" << std::endl;
            fireTestEventUINT8E2EEvent(length, counter, dataID, CRC, uINT8Value);
            duration = duration-debounce;
            std::this_thread::sleep_for(std::chrono::seconds(debounce));
        }
    });

    return;
}
    
void etsStubImpl::triggerEventUINT8Multicast(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT8Multicast_ReqArg1,
                                            uint32_t _triggerEventUINT8Multicast_ReqArg2, uint32_t _triggerEventUINT8Multicast_ReqArg3) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    startTimeout = _triggerEventUINT8Multicast_ReqArg1;
    durationTimeout = _triggerEventUINT8Multicast_ReqArg2;
    debounceTimeout = _triggerEventUINT8Multicast_ReqArg3;

    appThreadPool.emplace_back([&]{
        uint32_t start = startTimeout;
        uint32_t duration = durationTimeout;
        uint32_t debounce = debounceTimeout;
        uint8_t counter = 0x15;
        std::cout << "etsStubImpl::triggerEventUINT8Multicast start timeout:" << start << std::endl;
        while (start > 0) {
            std::cout << "etsStubImpl::triggerEventUINT8Multicast wait for start timeout " << start << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            start = start-1;
        }

        while (duration > 0) {
            std::cout << "etsStubImpl::triggerEventUINT8Multicast:" << (uint32_t)counter << std::endl;
            fireTestEventUINT8MulticastEvent(counter);
            duration = duration-debounce;
            counter = counter+1;
            std::this_thread::sleep_for(std::chrono::seconds(debounce));
        }
    });

    return;
}

void etsStubImpl::echoUINT8E2E(const std::shared_ptr<CommonAPI::ClientId> _client, uint16_t _echoUINT8E2E_ReqArg1,
                            uint16_t _echoUINT8E2E_ReqArg2, uint32_t _echoUINT8E2E_ReqArg3, uint32_t _echoUINT8E2E_ReqArg4,
                            uint8_t _echoUINT8E2E_ReqArg5, echoUINT8E2EReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    uint16_t echoUINT8E2E_ResArg1 = _echoUINT8E2E_ReqArg1;
    uint16_t echoUINT8E2E_ResArg2 = _echoUINT8E2E_ReqArg2;
    uint32_t echoUINT8E2E_ResArg3 = _echoUINT8E2E_ReqArg3;
    uint32_t echoUINT8E2E_ResArg4 = _echoUINT8E2E_ReqArg4;
    uint8_t echoUINT8E2E_ResArg5 = _echoUINT8E2E_ReqArg5;
    _reply(echoUINT8E2E_ResArg1, echoUINT8E2E_ResArg2, echoUINT8E2E_ResArg3, echoUINT8E2E_ResArg4, echoUINT8E2E_ResArg5);
    return;
}

void etsStubImpl::echoUINT8ArrayLengthTP(const std::shared_ptr<CommonAPI::ClientId> _client, std::vector< uint8_t > _inUINT8Array_ReqArg1, echoUINT8ArrayLengthTPReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    std::vector< uint8_t > outUINT8Array_ResArg1;
    outUINT8Array_ResArg1.assign(_inUINT8Array_ReqArg1.begin(), _inUINT8Array_ReqArg1.end());
    _reply(outUINT8Array_ResArg1);
    return;
}


void etsStubImpl::echoUINT8ArrayLengthInTP(const std::shared_ptr<CommonAPI::ClientId> _client, std::vector< uint8_t > _inUINT8Array_ReqArg1, echoUINT8ArrayLengthInTPReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << " Data size:" << std::dec << (uint32_t)_inUINT8Array_ReqArg1.size() << std::endl;
    uint32_t outUINT32_ResArg1 = _inUINT8Array_ReqArg1.size();
    for (uint32_t idx = 0; idx < outUINT32_ResArg1; idx++) {
        std::cout << std::hex << (uint32_t)_inUINT8Array_ReqArg1[idx];
    }
    std::cout << '\n';
    _reply(outUINT32_ResArg1);
    return;
}

void etsStubImpl::echoUINT8ArrayLengthOutTP(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _inUINT32_ReqArg1, echoUINT8ArrayLengthOutTPReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << " _inUINT32_ReqArg1:" << std::dec << _inUINT32_ReqArg1 << std::endl;
    std::vector< uint8_t > outUINT8Array_ResArg1;
    for (uint32_t idx=0; idx<_inUINT32_ReqArg1; idx++) {
        outUINT8Array_ResArg1.push_back(0xab);
    }
    _reply(outUINT8Array_ResArg1);
    return;
}

void etsStubImpl::triggerEventUINT8ArrayTP(const std::shared_ptr<CommonAPI::ClientId> _client, std::vector< uint8_t > _triggerEventUINT8ArrayTP_ReqArg1) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    std::vector< uint8_t > outUINT8EventArray;
    outUINT8EventArray.assign(_triggerEventUINT8ArrayTP_ReqArg1.begin(), _triggerEventUINT8ArrayTP_ReqArg1.end());
    fireTestEventUINT8ArrayTPEvent(outUINT8EventArray);
    return;
}

void etsStubImpl::echoUINT8ArrayLengthTPNoResponse(const std::shared_ptr<CommonAPI::ClientId> _client, std::vector< uint8_t > _echoUINT8ArrayLengthTPNoResponse_ReqArg1) {
    std::cout << "etsStubImpl::" << __func__ << " Data size:" << std::dec << (uint32_t)_echoUINT8ArrayLengthTPNoResponse_ReqArg1.size() << std::endl;
    uint32_t UINT8ArrayTP_ReqArg1Size = _echoUINT8ArrayLengthTPNoResponse_ReqArg1.size();
    for (uint32_t idx = 0; idx < UINT8ArrayTP_ReqArg1Size; idx++) {
        std::cout << std::hex << (uint32_t)_echoUINT8ArrayLengthTPNoResponse_ReqArg1[idx];
    }
    std::cout << '\n';
    return;
}

void etsStubImpl::triggerEventUINT8ArrayTPNoReqTPPayload(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT32_ReqArg1) {
    std::cout << "etsStubImpl::" << __func__ << " _triggerEventUINT32_ReqArg1:" << std::dec << _triggerEventUINT32_ReqArg1 << std::endl;
    std::vector< uint8_t > outINT8EventArray;
    for (uint32_t idx=0; idx<_triggerEventUINT32_ReqArg1; idx++) {
        outINT8EventArray.push_back(0xab);
    }
    fireTestEventUINT8ArrayTPEvent(outINT8EventArray);
    return; 
}

void etsStubImpl::triggerEventUINT32Periodic(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT32_ReqArg1) {
    std::cout << "etsStubImpl::" << __func__ << " _triggerEventUINT32_ReqArg1:" << _triggerEventUINT32_ReqArg1 << std::endl;
    fireTestEventUINT32PeriodicEvent(_triggerEventUINT32_ReqArg1);
    return;
}

void etsStubImpl::triggerEventUINT32UpdateOnChange(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT32_ReqArg1) {
    std::cout << "etsStubImpl::" << __func__ << " _triggerEventUINT32_ReqArg1:" << _triggerEventUINT32_ReqArg1 << std::endl;
    fireTestEventUINT32UpdateOnChangeEvent(_triggerEventUINT32_ReqArg1);
    return;
}

void etsStubImpl::activateTestSerivce(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _activateTestSerivce_ReqArg1, uint32_t _activateTestSerivce_ReqArg2) {
    std::cout << "etsStubImpl::" << __func__ << " _activateTestSerivce_ReqArg1:" << _activateTestSerivce_ReqArg1
        << " _activateTestSerivce_ReqArg2:" << _activateTestSerivce_ReqArg2 << std::endl;
    int retry_counter = 0;
    std::string domain = "local";
    if (false == TestService1Context1Registered && 259 == _activateTestSerivce_ReqArg1 && 1 == _activateTestSerivce_ReqArg2) {
        std::string instance = "someip.testability.ETSTestService1Context1";
        std::string connection = "TestService1Context1";
        std::shared_ptr<etsStubImplService1> testService = std::make_shared<etsStubImplService1>();
        TestService1Context1Registered = CommonAPI::Runtime::get()->registerService(domain, instance, testService, connection);
        while (!TestService1Context1Registered && retry_counter<10) {
            std::cout << "Register Service failed, trying again in 100 milliseconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            TestService1Context1Registered = CommonAPI::Runtime::get()->registerService(domain, instance, testService, connection);
            ++retry_counter;
        }
        if (retry_counter<10) {
            std::cout << "etsStubImpl::" << __func__ << "Successfully Registered ETSTestService1 Service for Context1!" << std::endl;
        }
        else {
            std::cout << "etsStubImpl::" << __func__ << "Registration Of ETSTestService1 Service Failed for Context1!" << std::endl;
        }
    }
    else if (false == TestService1Context2Registered && 259 == _activateTestSerivce_ReqArg1 && 2 == _activateTestSerivce_ReqArg2) {
        std::string instance = "someip.testability.ETSTestService1Context2";
        std::string connection = "TestService1Context2";
        std::shared_ptr<etsStubImplService1> testService = std::make_shared<etsStubImplService1>();
        TestService1Context2Registered = CommonAPI::Runtime::get()->registerService(domain, instance, testService, connection);
        while (!TestService1Context2Registered && retry_counter<10) {
            std::cout << "Register Service failed, trying again in 100 milliseconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            TestService1Context2Registered = CommonAPI::Runtime::get()->registerService(domain, instance, testService, connection);
            ++retry_counter;
        }
        if (retry_counter<10) {
            std::cout << "etsStubImpl::" << __func__ << "Successfully Registered ETSTestService1 Service for Context2!" << std::endl;
        }
        else {
            std::cout << "etsStubImpl::" << __func__ << "Registration Of ETSTestService1 Service Failed for Context2!" << std::endl;
        }
    }
    else if (false == TestService2Context1Registered && 260 == _activateTestSerivce_ReqArg1 && 1 == _activateTestSerivce_ReqArg2) {
        std::string instance = "someip.testability.ETSTestService2Context1";
        std::string connection = "TestService2Context1";
        std::shared_ptr<etsStubImplService2> testService = std::make_shared<etsStubImplService2>();
        TestService2Context1Registered = CommonAPI::Runtime::get()->registerService(domain, instance, testService, connection);
        while (!TestService2Context1Registered && retry_counter<10) {
            std::cout << "Register Service failed, trying again in 100 milliseconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            TestService2Context1Registered = CommonAPI::Runtime::get()->registerService(domain, instance, testService, connection);
            ++retry_counter;
        }
        if (retry_counter<10) {
            std::cout << "etsStubImpl::" << __func__ << "Successfully Registered ETSTestService2 Service!" << std::endl;
        }
        else {
            std::cout << "etsStubImpl::" << __func__ << "Registration Of ETSTestService2 Service Failed!" << std::endl;
        }
    }
    else {
        std::cerr << "etsStubImpl::" << __func__ << " Invalid service info" << std::endl;
    }
}

void etsStubImpl::deactivateTestSerivce(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _activateTestSerivce_ReqArg1, uint32_t _activateTestSerivce_ReqArg2) {
    std::cout << "etsStubImpl::" << __func__ << " _activateTestSerivce_ReqArg1:" << _activateTestSerivce_ReqArg1
        << " _activateTestSerivce_ReqArg2:" << _activateTestSerivce_ReqArg2 << std::endl;
    int retry_counter = 0;
    std::string domain = "local";
    if (true == TestService1Context1Registered && 259 == _activateTestSerivce_ReqArg1 && 1 == _activateTestSerivce_ReqArg2) {
        std::string instance = "someip.testability.ETSTestService1Context1";
        std::string connection = "TestService1Context1";
        bool successfullydeRegistered = CommonAPI::Runtime::get()->unregisterService(domain, ETSTestService1::getInterface(), instance);
        while (!successfullydeRegistered && retry_counter<10) {
            std::cout << "etsStubImpl::" << __func__ << "Service Deregistration failed, trying again in 100 milliseconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            successfullydeRegistered = CommonAPI::Runtime::get()->unregisterService(domain, ETSTestService1::getInterface(), instance);
            ++retry_counter;
        }
        if (retry_counter<10) {
            std::cout << "etsStubImpl::" << __func__ << "Successfully DeRegistered ETSTestService1 Service!" << std::endl;
            TestService1Context1Registered = false;
        }
        else {
            std::cout << "etsStubImpl::" << __func__ << "DeRegistration Of ETSTestService1 Service Failed!" << std::endl;
        }
    }
    else if (true == TestService1Context2Registered && 259 == _activateTestSerivce_ReqArg1 && 2 == _activateTestSerivce_ReqArg2) {
        std::string instance = "someip.testability.ETSTestService1Context2";
        std::string connection = "TestService1Context2";
        bool successfullydeRegistered = CommonAPI::Runtime::get()->unregisterService(domain, ETSTestService1::getInterface(), instance);
        while (!successfullydeRegistered && retry_counter<10) {
            std::cout << "etsStubImpl::" << __func__ << "Service Deregistration failed, trying again in 100 milliseconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            successfullydeRegistered = CommonAPI::Runtime::get()->unregisterService(domain, ETSTestService1::getInterface(), instance);
            ++retry_counter;
        }
        if (retry_counter<10) {
            std::cout << "etsStubImpl::" << __func__ << "Successfully DeRegistered ETSTestService1 Service!" << std::endl;
            TestService1Context2Registered = false;
        }
        else {
            std::cout << "etsStubImpl::" << __func__ << "DeRegistration Of ETSTestService1 Service Failed!" << std::endl;
        }
    }
    else if (true == TestService2Context1Registered && 260 == _activateTestSerivce_ReqArg1 && 1 == _activateTestSerivce_ReqArg2) {
        std::string instance = "someip.testability.ETSTestService2Context1";
        std::string connection = "TestService2Context1";
        bool successfullydeRegistered = CommonAPI::Runtime::get()->unregisterService(domain, ETSTestService2::getInterface(), instance);
        while (!successfullydeRegistered && retry_counter<10) {
            std::cout << "etsStubImpl::" << __func__ << "Service Deregistration failed, trying again in 100 milliseconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            successfullydeRegistered = CommonAPI::Runtime::get()->unregisterService(domain, ETSTestService2::getInterface(), instance);
            ++retry_counter;
        }
        if (retry_counter<10) {
            std::cout << "etsStubImpl::" << __func__ << "Successfully DeRegistered ETSTestService2 Service!" << std::endl;
            TestService2Context1Registered = false;
        }
        else {
            std::cout << "etsStubImpl::" << __func__ << "DeRegistration Of ETSTestService2 Service Failed!" << std::endl;
        }
    }
    else {
        std::cerr << "etsStubImpl::" << __func__ << " Invalid service info" << std::endl;
    }
}

void etsStubImplService1::echoUINT32(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _inUINT32_ReqArg1, echoUINT32Reply_t _reply) {
    std::cout << "etsStubImplService1::" << __func__ << " _inUINT32_ReqArg1:" << _inUINT32_ReqArg1 << std::endl;
    _reply(_inUINT32_ReqArg1);
    return;
}

void etsStubImplService2::echoUINT32(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _inUINT32_ReqArg1, echoUINT32Reply_t _reply) {
    std::cout << "etsStubImplService2::" << __func__ << " _inUINT32_ReqArg1:" << _inUINT32_ReqArg1 << std::endl;
    _reply(_inUINT32_ReqArg1);
    return;
}

void etsStubImpl::echoUINT8RELIABLE(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _echoUINT8RELIABLE_ReqArg1, echoUINT8RELIABLEReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    uint8_t _echoUINT8RELIABLE_ResArg1 = _echoUINT8RELIABLE_ReqArg1;
    _reply(_echoUINT8RELIABLE_ResArg1);
    return;
}

void etsStubImpl::triggerEventUINT8Reliable(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _triggerEventUINT8Reliable_ReqArg1, uint32_t _triggerEventUINT8Reliable_ReqArg2, uint32_t _triggerEventUINT8Reliable_ReqArg3) {
    std::cout << "etsStubImpl::" << __func__ << std::endl;
    startTimeout = _triggerEventUINT8Reliable_ReqArg1;
    durationTimeout = _triggerEventUINT8Reliable_ReqArg2;
    debounceTimeout = _triggerEventUINT8Reliable_ReqArg3;

    appThreadPool.emplace_back([&]{
        uint32_t start = startTimeout;
        uint32_t duration = durationTimeout;
        uint32_t debounce = debounceTimeout;
        uint8_t testEventUINT8Val = 0;
        std::cout << "etsStubImpl::triggerEventUINT8Reliable start timeout:" << start << std::endl;
        while (start > 0) {
            std::cout << "etsStubImpl::triggerEventUINT8Reliable wait for start timeout " << start << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            start = start-1;
        }

        while (duration > 0) {
            std::cout << "etsStubImpl::triggerEventUINT8Reliable:" << (uint32_t)testEventUINT8Val << std::endl;
            fireTestEventUINT8ReliableEvent(testEventUINT8Val);
            duration = duration-debounce;
            testEventUINT8Val = testEventUINT8Val+(uint8_t)1;
            std::this_thread::sleep_for(std::chrono::seconds(debounce));
        }
    });

    return;
}

void etsStubImpl::clientServiceGetLastValueOfEventTCP(const std::shared_ptr<CommonAPI::ClientId> _client, clientServiceGetLastValueOfEventTCPReply_t _reply) {
    std::cout << "etsStubImpl::" << __func__ << " lastuINT8ValueReliable:" << std::hex << (uint32_t)lastuINT8ValueReliable << std::endl;
    uint8_t lastval = lastuINT8ValueReliable;
    _reply(lastval);
    return;
}