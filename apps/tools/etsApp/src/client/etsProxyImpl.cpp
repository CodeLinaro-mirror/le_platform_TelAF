/* 
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "etsProxyImpl.hpp"

etsProxyImpl::etsProxyImpl() {
    secondaryServiceActive = false;
    method_response_received = false;
    uINT8Valuesubscription = 0;
    uINT8E2Esubscription = 0;
    uINT8Multicastsubscription = 0;
    uINT8Arraysubscription = 0;
    TestEventUINT8TPsubscription = 0;
    TestEventUINT32Periodicsubscription = 0;
    TestEventUINT32UpdateOnChangesubscription = 0;
    uINT8ValuesubscriptionReliable = 0;
}

etsProxyImpl::~etsProxyImpl() {
    for (auto& thread : appThreadPool) {
        thread.join();
    }
}

std::string etsProxyImpl::callstatusToString(CommonAPI::CallStatus status) {
    uint8_t status_ = static_cast<uint8_t>(status);
    switch(status_) {
        case static_cast<uint8_t>(CommonAPI::CallStatus::SUCCESS): return "SUCCESS";
        case static_cast<uint8_t>(CommonAPI::CallStatus::OUT_OF_MEMORY): return "OUT_OF_MEMORY";
        case static_cast<uint8_t>(CommonAPI::CallStatus::NOT_AVAILABLE): return "NOT_AVAILABLE";
        case static_cast<uint8_t>(CommonAPI::CallStatus::CONNECTION_FAILED): return "CONNECTION_FAILED";
        case static_cast<uint8_t>(CommonAPI::CallStatus::REMOTE_ERROR): return "REMOTE_ERROR";
        case static_cast<uint8_t>(CommonAPI::CallStatus::UNKNOWN): return "UNKNOWN";
        case static_cast<uint8_t>(CommonAPI::CallStatus::INVALID_VALUE): return "INVALID_VALUE";
        case static_cast<uint8_t>(CommonAPI::CallStatus::SUBSCRIPTION_REFUSED): return "SUBSCRIPTION_REFUSED";
        case static_cast<uint8_t>(CommonAPI::CallStatus::SERIALIZATION_ERROR): return "SERIALIZATION_ERROR";
        default: return "UNDEFINED";
    }
}

std::string etsProxyImpl::returnCodeToString(vsomeip::return_code_e return_code) {
    uint8_t return_code_ = static_cast<uint8_t>(return_code);
    switch(return_code_) {
        case static_cast<uint8_t>(vsomeip::return_code_e::E_OK): return "E_OK";
        case static_cast<uint8_t>(vsomeip::return_code_e::E_NOT_OK): return "E_NOT_OK";
        case static_cast<uint8_t>(vsomeip::return_code_e::E_UNKNOWN_SERVICE): return "E_UNKNOWN_SERVICE";
        case static_cast<uint8_t>(vsomeip::return_code_e::E_UNKNOWN_METHOD): return "E_UNKNOWN_METHOD";
        case static_cast<uint8_t>(vsomeip::return_code_e::E_NOT_READY): return "E_NOT_READY";
        case static_cast<uint8_t>(vsomeip::return_code_e::E_NOT_REACHABLE): return "E_NOT_REACHABLE";
        case static_cast<uint8_t>(vsomeip::return_code_e::E_TIMEOUT): return "E_TIMEOUT";
        case static_cast<uint8_t>(vsomeip::return_code_e::E_WRONG_PROTOCOL_VERSION): return "E_WRONG_PROTOCOL_VERSION";
        case static_cast<uint8_t>(vsomeip::return_code_e::E_WRONG_INTERFACE_VERSION): return "E_WRONG_INTERFACE_VERSION";
        case static_cast<uint8_t>(vsomeip::return_code_e::E_MALFORMED_MESSAGE): return "E_MALFORMED_MESSAGE";
        case static_cast<uint8_t>(vsomeip::return_code_e::E_WRONG_MESSAGE_TYPE): return "E_WRONG_MESSAGE_TYPE";
        case static_cast<uint8_t>(vsomeip::return_code_e::E_UNKNOWN): return "E_UNKNOWN";
        default: return "UNDEFINED";
    }
}

void etsProxyImpl::createProxy(std::string appname) {
    CommonAPI::Runtime::setProperty("LogContext", "ETS01C");
    CommonAPI::Runtime::setProperty("LibraryBase", "ETS");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "someip.testability.ETS";
    std::string connection = appname;

    std::cout << "etsProxyImpl::" << __func__ << "[app:" << connection << "]" << std::endl;

    etsProxy = runtime->buildProxy<ETSProxy>(domain, instance, connection);

    etsProxy->getProxyStatusEvent().subscribe([&](const CommonAPI::AvailabilityStatus& val) {
        if (val == CommonAPI::AvailabilityStatus::AVAILABLE) {
            isAvailable = true;
            std::cout << "ETS Service available" << std::endl;
        }
        else {
            isAvailable = false;
            std::cout << "ETS Service not available" << std::endl;
        }
    });

    std::cout << "Checking availability!" << std::endl;
    while (!etsProxy->isAvailable()) {
        std::cout << "Service not available, trying again in 100 milliseconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "ETS Service Available..." << std::endl;
}

void etsProxyImpl::tester_send_offer() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    appThreadPool.emplace_back([&]{
        std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
        std::string domain = "local";
        std::string instance = "someip.testability.ETSSecondaryService";
        std::string connection = "ets_secondary_service";
        int retry_counter = 5;

        if (!secondaryServiceActive) {
            std::cout << "etsProxyImpl::tester_send_offer() start sending offer for Secondary Service" << std::endl;
            secService = std::make_shared<secServiceStubImpl>();
            bool successfullyRegistered = runtime->registerService(domain, instance, secService, connection);

            while (!successfullyRegistered && (retry_counter>0)) {
                std::cout << "Register Secondary Service failed, trying again in 100 milliseconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                successfullyRegistered = runtime->registerService(domain, instance, secService, connection);
                retry_counter=retry_counter-1;
            }

            if (retry_counter > 0) {
                std::cout << "Successfully Registered Secondary Service!" << std::endl;
                secondaryServiceActive = true;
                while (secondaryServiceActive) {
                    std::cout << "Secondary Service Waiting for calls..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(60));
                }
            }
            else {
                std::cout << "Secondary Service Registration failed" << std::endl;
            }
        }
        else {
            std::cout << "Secondary Service Already Registered!" << std::endl;
        }
    });
}

void etsProxyImpl::tester_send_unicast_event(int eventval) {
    uint8_t uINT8Value = 0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (secondaryServiceActive) {
        uINT8Value = (uint8_t)eventval;
        std::cout << "etsProxyImpl::" << __func__ << " value:" << std::hex << (uint32_t)uINT8Value << std::endl;
        secService->fireSecondaryEventUINT8Event(uINT8Value);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " Service not yet offered" << std::endl;
    }

    return;
}
void etsProxyImpl::tester_send_multicast_event(int eventval) {
    uint8_t uINT8Value = 0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (secondaryServiceActive) {
        uINT8Value = (uint8_t)eventval;
        std::cout << "etsProxyImpl::" << __func__ << " value:" << std::hex << (uint32_t)uINT8Value << std::endl;
        secService->fireSecondaryMulticastEventUINT8Event(uINT8Value);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " Service not yet offered" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_send_stop_offer(int without_offer) {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (2 == without_offer) {
        std::cout << "etsProxyImpl::" << __func__ << " Send Stop Offer without starting Offer" << std::endl;
        secondaryServiceActive = true;
    }

    appThreadPool.emplace_back([&]{
        std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
        std::string domain = "local";
        std::string instance = "someip.testability.ETSSecondaryService";
        std::string connection = "ets_secondary_service";
        int retry_counter = 5;

        if (secondaryServiceActive) {
            secondaryServiceActive = false;
            std::cout << "etsProxyImpl::tester_send_stop_offer DeRegistering Secondary Service" << std::endl;

            bool successfullydeRegistered = runtime->unregisterService(domain, ETSSecondaryService::getInterface(), instance);

            while (!successfullydeRegistered && (retry_counter>0)) {
                std::cout << "DeRegister Secondary Service failed, trying again in 100 milliseconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                successfullydeRegistered = runtime->unregisterService(domain, ETSSecondaryService::getInterface(), instance);
                retry_counter=retry_counter-1;
            }
            if (retry_counter > 0) {
                std::cout << "Successfully DeRegistered Secondary Service!" << std::endl;
            }
            else {
                std::cout << "Secondary Service De Registration failed!" << std::endl;
            }
        }
        else {
            std::cout << "Secondary Service Already DeRegistered!" << std::endl;
        }
    });
}

void etsProxyImpl::tester_send_session_reset() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;
    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");

    if (_app) {
#if 0
        _app->reset_sd_session();
#endif
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_update_service_unreliable_port() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;
#if 0
    vsomeip::service_t service = 258;
    vsomeip::instance_t instance = 244;
    vsomeip::port_t unreliable = 30520;
#endif
    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");

    if (_app) {
#if 0
        _app->update_service_unreliable_port(service, instance, unreliable);
#endif
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_get_interface_version(int service_id, int instance_id) {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (isAvailable) {
        vsomeip::application::available_t are_available;
        vsomeip::service_t service = (vsomeip::service_t)service_id;
        vsomeip::instance_t instance = (vsomeip::instance_t)instance_id;
        std::map<vsomeip::major_version_t, vsomeip::minor_version_t> version; 
        std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
        if (_app) {
            _app->are_available(are_available, vsomeip::ANY_SERVICE, vsomeip::ANY_INSTANCE, vsomeip::ANY_MAJOR, vsomeip::ANY_MINOR);
            auto found_service = are_available.find(service);
            if(found_service != are_available.end()) {
                auto found_instance = found_service->second.find(instance);
                if(found_instance != found_service->second.end()) {
                    version = are_available[service][instance];
                    for (auto& it : version) {
                        std::cout << "Service[" << (uint32_t)service << "." << (uint32_t)instance << "]"
                            << ": Version[" << (uint32_t)it.first << "." << (uint32_t)it.second << "]" << std::endl;
                    }
                }
            }
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_subscribe_TestEventUINT8() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        uINT8Valuesubscription = etsProxy->getTestEventUINT8Event().subscribe([&](const uint8_t& uINT8Value) {
            std::cout << "etsProxyImpl::tester_subscribe_TestEventUINT8 TestEventUINT8 Notification:" << std::hex << (uint32_t)uINT8Value << std::endl;
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_unsubscribe_TestEventUINT8() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestEventUINT8Event().unsubscribe(uINT8Valuesubscription);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_subscribe_TestEventUINT8Array() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        uINT8Arraysubscription = etsProxy->getTestEventUINT8ArrayEvent().subscribe([&](const std::vector< uint8_t > &_uINT8Array) {
            std::cout << "etsProxyImpl::tester_subscribe_TestEventUINT8Array TestEventUINT8Array Notification [size:"
                << std::dec << (uint32_t)_uINT8Array.size() << "]" << std::endl;
            for (auto &it : _uINT8Array) {
                std::cout << std::hex << (uint32_t)it;
            }
            std::cout << '\n';
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_unsubscribe_TestEventUINT8Array() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestEventUINT8ArrayEvent().unsubscribe(uINT8Arraysubscription);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_subscribe_TestEventUINT8Multicast() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        uINT8Multicastsubscription = etsProxy->getTestEventUINT8MulticastEvent().subscribe([&](const uint8_t &_uINT8Value) {
            std::cout << "etsProxyImpl::tester_subscribe_TestEventUINT8Multicast TestEventUINT8Multicast Notification:" << std::hex << (uint32_t)_uINT8Value << std::endl;
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_unsubscribe_TestEventUINT8Multicast() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestEventUINT8MulticastEvent().unsubscribe(uINT8Multicastsubscription);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_subscribe_TestEventUINT8E2E() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        uINT8E2Esubscription = etsProxy->getTestEventUINT8E2EEvent().subscribe([&](const uint16_t &_length,
                 const uint16_t &_counter, const uint32_t &_dataID, const uint32_t &_CRC, const uint8_t &_uINT8Value) {
            std::cout << "etsProxyImpl::tester_subscribe_TestEventUINT8E2E TestEventUINT8E2E Notification:" << std::endl;
            std::cout << "length:" << (uint32_t)_length << std::endl;
            std::cout << "counter:" << (uint32_t)_counter << std::endl;
            std::cout << "data id:" << _dataID << std::endl;
            std::cout << "CRC value:" << _CRC << std::endl;
            std::cout << "payload:" << (uint32_t)_uINT8Value << std::endl;
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_unsubscribe_TestEventUINT8E2E() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestEventUINT8E2EEvent().unsubscribe(uINT8E2Esubscription);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_subscribe_TestEventUINT8TP() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        TestEventUINT8TPsubscription = etsProxy->getTestEventUINT8ArrayTPEvent().subscribe([&](const std::vector< uint8_t > &_outUINT8EventArray) {
            std::cout << "etsProxyImpl::tester_subscribe_TestEventUINT8TP TestEventUINT8TP Notification [size:"
                << std::dec << (uint32_t)_outUINT8EventArray.size() << "]" << std::endl;
            for (auto &it : _outUINT8EventArray) {
                std::cout << std::hex << (uint32_t)it;
            }
            std::cout << " Done" << std::endl;
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_unsubscribe_TestEventUINT8TP() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestEventUINT8ArrayTPEvent().unsubscribe(TestEventUINT8TPsubscription);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_subscribe_TestEventUINT32PeriodicEvent() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        TestEventUINT32Periodicsubscription = etsProxy->getTestEventUINT32PeriodicEvent().subscribe([&](const uint32_t &_uINT32Value) {
            std::cout << "etsProxyImpl::tester_subscribe_TestEventUINT32PeriodicEvent TestEventUINT32PeriodicEvent:" << _uINT32Value << std::endl;
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_unsubscribe_TestEventUINT32PeriodicEvent() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestEventUINT32PeriodicEvent().unsubscribe(TestEventUINT32Periodicsubscription);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_subscribe_TestEventUINT32UpdateOnChangeEvent() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        TestEventUINT32UpdateOnChangesubscription = etsProxy->getTestEventUINT32UpdateOnChangeEvent().subscribe([&](const uint32_t &_uINT32Value) {
            std::cout << "etsProxyImpl::tester_subscribe_TestEventUINT32UpdateOnChangeEvent TestEventUINT32UpdateOnChangeEvent:" << _uINT32Value << std::endl;
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_unsubscribe_TestEventUINT32UpdateOnChangeEvent() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestEventUINT32UpdateOnChangeEvent().unsubscribe(TestEventUINT32UpdateOnChangesubscription);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_requestTestService(uint32_t service_id, uint32_t instance_id) {
    std::string domain = "local";
    int retry_counter = 0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (259 == service_id && 1 == instance_id) {
        std::string instance = "someip.testability.ETSTestService1Context1";
        std::string connection = "TestService1Context1Client";
        testSvcProxy1 = CommonAPI::Runtime::get()->buildProxy<ETSTestService1Proxy>(domain, instance, connection);
        std::cout << "Checking availability!" << std::endl;
        while (!testSvcProxy1->isAvailable() && retry_counter<10) {
            std::cout << "Service not available, trying again in 100 milliseconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ++retry_counter;
        }
        if (retry_counter<10) {
            std::cout << "TestService1 Service Available..." << std::endl;
        }
        else {
            std::cout << "TestService1 Service Not Available..." << std::endl;
        }
    }
    else if (259 == service_id && 2 == instance_id) {
        std::string instance = "someip.testability.ETSTestService1Context2";
        std::string connection = "TestService1Context2Client";
        testSvcProxy2 = CommonAPI::Runtime::get()->buildProxy<ETSTestService1Proxy>(domain, instance, connection);
        std::cout << "Checking availability!" << std::endl;
        while (!testSvcProxy2->isAvailable() && retry_counter<10) {
            std::cout << "Service not available, trying again in 100 milliseconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ++retry_counter;
        }
        if (retry_counter<10) {
            std::cout << "TestService2 Service Available..." << std::endl;
        }
        else {
            std::cout << "TestService2 Service Not Available..." << std::endl;
        }
    }
    else if (260 == service_id && 1 == instance_id) {
        std::string instance = "someip.testability.ETSTestService2Context1";
        std::string connection = "TestService2Context1Client";
        testSvcProxy3 = CommonAPI::Runtime::get()->buildProxy<ETSTestService2Proxy>(domain, instance, connection);
        std::cout << "Checking availability!" << std::endl;
        while (!testSvcProxy3->isAvailable() && retry_counter<10) {
            std::cout << "Service not available, trying again in 100 milliseconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ++retry_counter;
        }
        if (retry_counter<10) {
            std::cout << "TestService3 Service Available..." << std::endl;
        }
        else {
            std::cout << "TestService3 Service Not Available..." << std::endl;
        }
    }
    else {
       std::cerr << "Invalid service info" << std::endl; 
    }
}

void etsProxyImpl::invoke_requestTestServiceMethod(uint32_t service_id, uint32_t instance_id) {
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint32_t inUINT32_ReqArg1;
    uint32_t outUINT32_ResArg1;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;
    if (259 == service_id && 1 == instance_id) {
        if (testSvcProxy1 && testSvcProxy1->isAvailable()) {
            inUINT32_ReqArg1 = service_id + instance_id;
            testSvcProxy1->echoUINT32(inUINT32_ReqArg1, callStatus, outUINT32_ResArg1);
            if (callStatus != CommonAPI::CallStatus::SUCCESS) {
                std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
            }
            else {
                std::cout << "outUINT32_ResArg1:" << outUINT32_ResArg1 << std::endl;
            }
        }
        else {
            std::cout << "testSvcProxy1 Not Available" << std::endl;
        }
    }
    else if (259 == service_id && 2 == instance_id) {
        if (testSvcProxy1 && testSvcProxy1->isAvailable()) {
            inUINT32_ReqArg1 = service_id + instance_id;
            testSvcProxy2->echoUINT32(inUINT32_ReqArg1, callStatus, outUINT32_ResArg1);
            if (callStatus != CommonAPI::CallStatus::SUCCESS) {
                std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
            }
            else {
                std::cout << "outUINT32_ResArg1:" << outUINT32_ResArg1 << std::endl;
            }
        }
        else {
            std::cout << "testSvcProxy1 Not Available" << std::endl;
        }
    }
    else if (260 == service_id && 1 == instance_id) {
        if (testSvcProxy1 && testSvcProxy1->isAvailable()) {
            inUINT32_ReqArg1 = service_id + instance_id;
            testSvcProxy3->echoUINT32(inUINT32_ReqArg1, callStatus, outUINT32_ResArg1);
            if (callStatus != CommonAPI::CallStatus::SUCCESS) {
                std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
            }
            else {
                std::cout << "outUINT32_ResArg1:" << outUINT32_ResArg1 << std::endl;
            }
        }
        else {
            std::cout << "testSvcProxy1 Not Available" << std::endl;
        }
    }
    else {
        std::cerr << "Invalid service info" << std::endl;
    }
}

void etsProxyImpl::invoke_checkByteOrder() {
    uint8_t checkByteOrder_ReqArg1 = 0x12;
    uint16_t checkByteOrder_ReqArg2 = 0x1234;
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    uint32_t checkByteOrder_ResArg1 = 0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->checkByteOrder(checkByteOrder_ReqArg1, checkByteOrder_ReqArg2, callStatus, checkByteOrder_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "checkByteOrder_ResArg1:" << std::hex << checkByteOrder_ResArg1 << std::endl;
            if (checkByteOrder_ResArg1 == ((uint32_t)checkByteOrder_ReqArg1 + (uint32_t)checkByteOrder_ReqArg2)) {
                std::cout << "SOMEIP_ETS_005: checkByteOrder PASS" << std::endl;
            }
            else {
                std::cout << "SOMEIP_ETS_005: checkByteOrder FAIL" << std::endl;
            }
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_clientServiceActivate(uint32_t start_timeout) {
    uint8_t clientServiceActivate_ReqArg1 = 0;
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    clientServiceActivate_ReqArg1 = (uint8_t)start_timeout;
    if (etsProxy && isAvailable) {
        etsProxy->clientServiceActivate(clientServiceActivate_ReqArg1, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_clientServiceDeactivate(int stop_timeout) {
    uint8_t clientServiceDeactivate_ReqArg1=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;
    clientServiceDeactivate_ReqArg1 = (uint8_t)stop_timeout;

    if (etsProxy && isAvailable) {
        etsProxy->clientServiceDeactivate(clientServiceDeactivate_ReqArg1, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_clientServiceGetLastValueOfEventUDPMulticast() {
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint8_t clientServiceGetLastValueOfEventUDPMulticast_ResArg1=0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->clientServiceGetLastValueOfEventUDPMulticast(callStatus, clientServiceGetLastValueOfEventUDPMulticast_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " Last Multicast Event Value:" << std::hex << (uint32_t)clientServiceGetLastValueOfEventUDPMulticast_ResArg1 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_clientServiceGetLastValueOfEventUDPUnicast() {
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint8_t clientServiceGetLastValueOfEventUDPUnicast_ResArg1=0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->clientServiceGetLastValueOfEventUDPUnicast(callStatus, clientServiceGetLastValueOfEventUDPUnicast_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " Last Unicast Event Value:" << std::hex << (uint32_t)clientServiceGetLastValueOfEventUDPUnicast_ResArg1 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_clientServiceSubscribeEventgroup(uint32_t start_timeout, uint32_t subscription_duration) {
    uint32_t clientServiceSubscribeEventgroup_ReqArg1=0;
    uint32_t clientServiceSubscribeEventgroup_ReqArg2=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    clientServiceSubscribeEventgroup_ReqArg1 = (uint8_t)start_timeout;
    clientServiceSubscribeEventgroup_ReqArg2 = (uint8_t)subscription_duration;
    if (etsProxy && isAvailable) {
        etsProxy->clientServiceSubscribeEventgroup(clientServiceSubscribeEventgroup_ReqArg1, clientServiceSubscribeEventgroup_ReqArg2, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoBitfields() {
    uint8_t echoBitfields_ReqArg1 = 0x12;
    uint16_t echoBitfields_ReqArg2 = 0x1234;
    uint32_t echoBitfields_ReqArg3 = 0x12345678;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint8_t echoBitfields_ResArg1=0;
    uint16_t echoBitfields_ResArg2=0;
    uint32_t echoBitfields_ResArg3=0;
    std::cout << "etsProxyImpl::" << __func__ << " echoBitfields_ReqArg1:" << std::hex << (uint32_t)echoBitfields_ReqArg1
            << " echoBitfields_ReqArg2:" << std::hex << (uint32_t)echoBitfields_ReqArg2
            << " echoBitfields_ReqArg3:" << std::hex << (uint32_t)echoBitfields_ReqArg3 << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->echoBitfields(echoBitfields_ReqArg1, echoBitfields_ReqArg2, echoBitfields_ReqArg3,
            callStatus, echoBitfields_ResArg1, echoBitfields_ResArg2, echoBitfields_ResArg3);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoBitfields_ResArg1:" << std::hex << (uint32_t)echoBitfields_ResArg1
                    << " echoBitfields_ResArg2:" << std::hex << (uint32_t)echoBitfields_ResArg2
                    << " echoBitfields_ResArg3:" << std::hex << (uint32_t)echoBitfields_ResArg3 << std::endl;
    
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoCommonDatatypes() {
    bool EchoCommonDatatypes_ReqArg1 = true;
    uint8_t EchoCommonDatatypes_ReqArg2 = 0x12;
    uint16_t EchoCommonDatatypes_ReqArg3 = 0x1234;
    uint32_t EchoCommonDatatypes_ReqArg4 = 0x12345678;
    int8_t EchoCommonDatatypes_ReqArg5 = 0x21;
    int16_t _EchoCommonDatatypes_ReqArg6 = 0x4321;
    int32_t _EchoCommonDatatypes_ReqArg7 = 0x76543211;
    float EchoCommonDatatypes_ReqArg8 = 8.146723;
    double EchoCommonDatatypes_ReqArg9 = 11.98765432152346;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    double EchoCommonDatatypes_ResArg1 = 0;
    float EchoCommonDatatypes_ResArg2 = 0;
    int32_t EchoCommonDatatypes_ResArg3 = 0;
    int16_t EchoCommonDatatypes_ResArg4 = 0;
    int8_t EchoCommonDatatypes_ResArg5 = 0;
    uint32_t EchoCommonDatatypes_ResArg6 = 0;
    uint16_t EchoCommonDatatypes_ResArg7 = 0;
    uint8_t EchoCommonDatatypes_ResArg8 = 0;
    bool EchoCommonDatatypes_ResArg9 = 0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->echoCommonDatatypes(EchoCommonDatatypes_ReqArg1, EchoCommonDatatypes_ReqArg2, EchoCommonDatatypes_ReqArg3,
            EchoCommonDatatypes_ReqArg4, EchoCommonDatatypes_ReqArg5, _EchoCommonDatatypes_ReqArg6, _EchoCommonDatatypes_ReqArg7,
            EchoCommonDatatypes_ReqArg8, EchoCommonDatatypes_ReqArg9, callStatus, EchoCommonDatatypes_ResArg1, EchoCommonDatatypes_ResArg2,
            EchoCommonDatatypes_ResArg3, EchoCommonDatatypes_ResArg4, EchoCommonDatatypes_ResArg5, EchoCommonDatatypes_ResArg6,
            EchoCommonDatatypes_ResArg7, EchoCommonDatatypes_ResArg8, EchoCommonDatatypes_ResArg9);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " EchoCommonDatatypes_ResArg1:"
                << std::setprecision(15) << EchoCommonDatatypes_ResArg1
                << " EchoCommonDatatypes_ResArg2:" << std::setprecision(7) << EchoCommonDatatypes_ResArg2
                << " EchoCommonDatatypes_ResArg3:" << EchoCommonDatatypes_ResArg3
                << " EchoCommonDatatypes_ResArg4:" << (int32_t)EchoCommonDatatypes_ResArg4
                << " EchoCommonDatatypes_ResArg5:" << (int32_t)EchoCommonDatatypes_ResArg5
                << " EchoCommonDatatypes_ResArg6:" << EchoCommonDatatypes_ResArg6
                << " EchoCommonDatatypes_ResArg7:" << (uint32_t)EchoCommonDatatypes_ResArg7
                << " EchoCommonDatatypes_ResArg8:" << (uint32_t)EchoCommonDatatypes_ResArg8
                << " EchoCommonDatatypes_ResArg9:" << EchoCommonDatatypes_ResArg9 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoENUM() {
    ETS::Enum echoENUM_ReqArg1 = ETS::Enum::ONE;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    ETS::Enum echoENUM_ResArg1=ETS::Enum::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->echoENUM(echoENUM_ReqArg1, callStatus, echoENUM_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoENUM_ResArg1:" << echoENUM_ResArg1.toString() << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoFLOAT64() {
    double echoFLOAT64_ReqArg1 = 5.98765432152346;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    double echoFLOAT64_ResArg1 = 0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->echoFLOAT64(echoFLOAT64_ReqArg1, callStatus, echoFLOAT64_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoFLOAT64_ResArg1:" << std::setprecision(15) << echoFLOAT64_ResArg1 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoINT8() {
    int8_t echoINT8_ReqArg1 = 12;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    int8_t echoINT8_ResArg1=0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->echoINT8(echoINT8_ReqArg1, callStatus, echoINT8_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoINT8_ResArg1:" << std::hex << (int32_t)echoINT8_ResArg1 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoStaticUINT8Array() {
    std::vector<uint8_t> echoStaticUINT8Array_ReqArg1;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::vector<uint8_t> echoStaticUINT8Array_ResArg1;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    echoStaticUINT8Array_ReqArg1.push_back(0x12);
    echoStaticUINT8Array_ReqArg1.push_back(0x34);
    echoStaticUINT8Array_ReqArg1.push_back(0x56);
    echoStaticUINT8Array_ReqArg1.push_back(0x78);
    echoStaticUINT8Array_ReqArg1.push_back(0x9a);

    if (etsProxy && isAvailable) {
        etsProxy->echoStaticUINT8Array(echoStaticUINT8Array_ReqArg1, callStatus, echoStaticUINT8Array_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoStaticUINT8Array_ResArg1:";
            for (auto &it : echoStaticUINT8Array_ResArg1) {
                std::cout << std::hex << (uint32_t)it;
            }
            std::cout << '\n';
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUINT8() {
    uint8_t echoUINT8_ReqArg1 = 17;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint8_t echoUINT8_ResArg1=0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->echoUINT8(echoUINT8_ReqArg1, callStatus, echoUINT8_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoUINT8_ResArg1:" << std::hex << (uint32_t)echoUINT8_ResArg1 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUINT8Array() {
    uint32_t echoUINT8Array_ReqArg1=0;
    std::vector<uint8_t> echoUINT8Array_ReqArg2;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint32_t echoUINT8Array_ResArg1=0;
    std::vector<uint8_t> echoUINT8Array_ResArg2;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    echoUINT8Array_ReqArg1 = 4;
    echoUINT8Array_ReqArg2.push_back(0x12);
    echoUINT8Array_ReqArg2.push_back(0x34);
    echoUINT8Array_ReqArg2.push_back(0x56);
    echoUINT8Array_ReqArg2.push_back(0x78);

    if (etsProxy && isAvailable) {
        etsProxy->echoUINT8Array(echoUINT8Array_ReqArg1, echoUINT8Array_ReqArg2, callStatus, echoUINT8Array_ResArg1, echoUINT8Array_ResArg2);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoUINT8Array_ResArg1:" << std::hex << (uint32_t)echoUINT8Array_ResArg1 << std::endl;
            for (auto &it : echoUINT8Array_ResArg2) {
                std::cout << std::hex << (uint32_t)it << " ";
            }
            std::cout << '\n';
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUINT8Array16BitLength() {
    uint16_t echoUINT8Array16BitLength_ReqArg1=0;
    std::vector<uint8_t> echoUINT8Array16BitLength_ReqArg2;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint16_t echoUINT8Array16BitLength_ResArg1=0;
    std::vector<uint8_t> echoUINT8Array16BitLength_ResArg2;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    echoUINT8Array16BitLength_ReqArg1 = 4;
    echoUINT8Array16BitLength_ReqArg2.push_back(0x12);
    echoUINT8Array16BitLength_ReqArg2.push_back(0x34);
    echoUINT8Array16BitLength_ReqArg2.push_back(0x56);
    echoUINT8Array16BitLength_ReqArg2.push_back(0x78);

    if (etsProxy && isAvailable) {
        etsProxy->echoUINT8Array16BitLength(echoUINT8Array16BitLength_ReqArg1, echoUINT8Array16BitLength_ReqArg2, callStatus, echoUINT8Array16BitLength_ResArg1, echoUINT8Array16BitLength_ResArg2);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoUINT8Array16BitLength_ResArg1:" << std::hex << (uint32_t)echoUINT8Array16BitLength_ResArg1 << std::endl;
            for (auto &it : echoUINT8Array16BitLength_ResArg2) {
                std::cout << std::hex << (uint32_t)it << " ";
            }
            std::cout << '\n';
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUINT8Array2Dim() {
    uint32_t echoUINT8Array2Dim_ReqArg1=0;
    std::vector< ETS::uint8ArrayArray > echoUINT8Array2Dim_ReqArg2;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint32_t echoUINT8Array2Dim_ResArg1=0;
    std::vector< ETS::uint8ArrayReturnArray > echoUINT8Array2Dim_ResArg2;
    ETS::uint8ArrayArray echoUINT8Array1Dim_ReqArg1;
    ETS::uint8ArrayArray echoUINT8Array1Dim_ReqArg2;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;
    echoUINT8Array2Dim_ReqArg1 = 6;
    echoUINT8Array1Dim_ReqArg1.push_back(0x12);
    echoUINT8Array1Dim_ReqArg1.push_back(0x34);
    echoUINT8Array1Dim_ReqArg1.push_back(0x56);
    echoUINT8Array1Dim_ReqArg2.push_back(0x78);
    echoUINT8Array1Dim_ReqArg2.push_back(0x9a);
    echoUINT8Array1Dim_ReqArg2.push_back(0xbc);
    echoUINT8Array2Dim_ReqArg2.push_back(echoUINT8Array1Dim_ReqArg1);
    echoUINT8Array2Dim_ReqArg2.push_back(echoUINT8Array1Dim_ReqArg2);
    if (etsProxy && isAvailable) {
        etsProxy->echoUINT8Array2Dim(echoUINT8Array2Dim_ReqArg1, echoUINT8Array2Dim_ReqArg2, callStatus, echoUINT8Array2Dim_ResArg1, echoUINT8Array2Dim_ResArg2);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoUINT8Array2Dim_ReqArg1:" << std::hex << (uint32_t)echoUINT8Array2Dim_ReqArg1 << std::endl;
            for (auto &arr1d : echoUINT8Array2Dim_ResArg2) {
                for (auto &it : arr1d) {
                    std::cout << std::hex << (uint32_t)it << " ";
                }
                std::cout << '\n';
            }
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUINT8Array8BitLength() {
    uint8_t echoUINT8Array8BitLength_ReqArg1=0;
    std::vector<uint8_t> echoUINT8Array8BitLength_ReqArg2;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint8_t echoUINT8Array8BitLength_ResArg1=0;
    std::vector<uint8_t> echoUINT8Array8BitLength_ResArg2;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    echoUINT8Array8BitLength_ReqArg1 = 4;
    echoUINT8Array8BitLength_ReqArg2.push_back(0x12);
    echoUINT8Array8BitLength_ReqArg2.push_back(0x34);
    echoUINT8Array8BitLength_ReqArg2.push_back(0x56);
    echoUINT8Array8BitLength_ReqArg2.push_back(0x78);

    if (etsProxy && isAvailable) {
        etsProxy->echoUINT8Array8BitLength(echoUINT8Array8BitLength_ReqArg1, echoUINT8Array8BitLength_ReqArg2, callStatus, echoUINT8Array8BitLength_ResArg1, echoUINT8Array8BitLength_ResArg2);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoUINT8Array8BitLength_ResArg1:" << std::hex << (uint32_t)echoUINT8Array8BitLength_ResArg1 << std::endl;
            for (auto &it : echoUINT8Array8BitLength_ResArg2) {
                std::cout << std::hex << (uint32_t)it << " ";
            }
            std::cout << '\n';
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUINT8ArrayMinSize() {
    uint32_t echoUINT8ArrayMinSize_ReqArg1=0;
    std::vector<uint8_t> echoUINT8ArrayMinSize_ReqArg2;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint32_t echoUINT8ArrayMinSize_ResArg1=0;
    std::vector<uint8_t> echoUINT8ArrayMinSize_ResArg2;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    echoUINT8ArrayMinSize_ReqArg1 = 3;
    echoUINT8ArrayMinSize_ReqArg2.push_back(0x12);
    echoUINT8ArrayMinSize_ReqArg2.push_back(0x34);
    echoUINT8ArrayMinSize_ReqArg2.push_back(0x56);

    if (etsProxy && isAvailable) {
        etsProxy->echoUINT8ArrayMinSize(echoUINT8ArrayMinSize_ReqArg1, echoUINT8ArrayMinSize_ReqArg2, callStatus, echoUINT8ArrayMinSize_ResArg1, echoUINT8ArrayMinSize_ResArg2);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoUINT8ArrayMinSize_ResArg1:" << std::hex << (uint32_t)echoUINT8ArrayMinSize_ResArg1 << std::endl;
            for (auto &it : echoUINT8ArrayMinSize_ResArg2) {
                std::cout << std::hex << (uint32_t)it << " ";
            }
            std::cout << '\n';
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUINT8ArrayMinSize_too_short(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << " remote_addresss:" << remote_address
        << " remote port:" << (uint32_t)remote_port << " local_address:" << local_address
        << " local_port:" << (uint32_t)local_port << std::endl;

    /* malformed message with too short length */
    const uint8_t request_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x37,   	        /* method id */
        0x00, 0x00, 0x00, 0x11,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01, 		    /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x00, 			        /* request type */
        0x00,			        /* return code */
        0x00,			        /* payload */
        0x00,			        /* payload */
        0x00,			        /* payload */
        0x01,			        /* payload */
        0x00,			        /* payload */
        0x00,			        /* payload */
        0x00,			        /* payload */
        0x01,			        /* payload */
        0x12			        /* payload */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(request_data, sizeof(request_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF16DYNAMIC() {
    uint32_t echoUTF16DYNAMIC_ReqArg1=0;
    std::string echoUTF16DYNAMIC_ReqArg2;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint32_t echoUTF16DYNAMIC_ResArg1=0;
    std::string echoUTF16DYNAMIC_ResArg2;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    echoUTF16DYNAMIC_ReqArg1 = 4;
    echoUTF16DYNAMIC_ReqArg2 = "abcd";

    if (etsProxy && isAvailable) {
        etsProxy->echoUTF16DYNAMIC(echoUTF16DYNAMIC_ReqArg1, echoUTF16DYNAMIC_ReqArg2, callStatus, echoUTF16DYNAMIC_ResArg1, echoUTF16DYNAMIC_ResArg2);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoUTF16DYNAMIC_ResArg1:" << echoUTF16DYNAMIC_ResArg1
                    << " utf16 string:" << echoUTF16DYNAMIC_ResArg2 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF16FIXED() {
    std::string echoUTF16FIXED_ReqArg1(30, 'a');
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::string echoUTF16FIXED_ResArg1;
    std::cout << "etsProxyImpl::" << __func__ << " echoUTF16FIXED_ReqArg1:" << echoUTF16FIXED_ReqArg1 << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->echoUTF16FIXED(echoUTF16FIXED_ReqArg1, callStatus, echoUTF16FIXED_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoUTF16FIXED_ResArg1 utf8:" << echoUTF16FIXED_ResArg1 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF8DYNAMIC() {
    uint32_t echoUTF8DYNAMIC_ReqArg1=0;
    std::string echoUTF8DYNAMIC_ReqArg2;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint32_t echoUTF8DYNAMIC_ResArg1=0;
    std::string echoUTF8DYNAMIC_ResArg2;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    echoUTF8DYNAMIC_ReqArg1 = 4;
    echoUTF8DYNAMIC_ReqArg2 = "abcd";

    if (etsProxy && isAvailable) {
        etsProxy->echoUTF8DYNAMIC(echoUTF8DYNAMIC_ReqArg1, echoUTF8DYNAMIC_ReqArg2, callStatus, echoUTF8DYNAMIC_ResArg1, echoUTF8DYNAMIC_ResArg2);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoUTF8DYNAMIC_ResArg1:" << echoUTF8DYNAMIC_ResArg1
                 << " utf8 string:" << echoUTF8DYNAMIC_ResArg2 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF8FIXED() {
    std::string echoUTF8FIXED_ReqArg1(60, 'a');
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::string echoUTF8FIXED_ResArg1;
    std::cout << "etsProxyImpl::" << __func__ << " echoUTF8FIXED_ReqArg1:" << echoUTF8FIXED_ReqArg1 << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->echoUTF8FIXED(echoUTF8FIXED_ReqArg1, callStatus, echoUTF8FIXED_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoUTF8FIXED_ResArg1:" << echoUTF8FIXED_ResArg1 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_resetInterface() {
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
    etsProxy->resetInterface(callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
                std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_suspendInterface(uint32_t start, uint32_t duration) {
    uint32_t suspendInterface_ReqArg1=0;
    uint32_t suspendInterface_ReqArg2=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    suspendInterface_ReqArg1 = start;
    suspendInterface_ReqArg2 = duration;
    if (etsProxy && isAvailable) {
    etsProxy->suspendInterface(suspendInterface_ReqArg1, suspendInterface_ReqArg2, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
                std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_triggerEventUINT8(uint32_t start, uint32_t duration, uint32_t debounce) {
    uint32_t triggerEventUINT8_ReqArg1=0;
    uint32_t triggerEventUINT8_ReqArg2=0;
    uint32_t triggerEventUINT8_ReqArg3=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    triggerEventUINT8_ReqArg1 = start;
    triggerEventUINT8_ReqArg2 = duration;
    triggerEventUINT8_ReqArg3 = debounce;
    if (etsProxy && isAvailable) {
        etsProxy->triggerEventUINT8(triggerEventUINT8_ReqArg1, triggerEventUINT8_ReqArg2, triggerEventUINT8_ReqArg3, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_triggerEventUINT8Array(uint32_t start, uint32_t duration, uint32_t debounce) {
    uint32_t triggerEventUINT8Array_ReqArg1=0;
    uint32_t triggerEventUINT8Array_ReqArg2=0;
    uint32_t triggerEventUINT8Array_ReqArg3=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    triggerEventUINT8Array_ReqArg1 = start;
    triggerEventUINT8Array_ReqArg2 = duration;
    triggerEventUINT8Array_ReqArg3 = debounce;
    if (etsProxy && isAvailable) {
        etsProxy->triggerEventUINT8Array(triggerEventUINT8Array_ReqArg1, triggerEventUINT8Array_ReqArg2, triggerEventUINT8Array_ReqArg3, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_triggerEventUINT8E2E(uint32_t start, uint32_t duration, uint32_t debounce) {
    uint32_t triggerEventUINT8E2E_ReqArg1=0;
    uint32_t triggerEventUINT8E2E_ReqArg2=0;
    uint32_t triggerEventUINT8E2E_ReqArg3=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    triggerEventUINT8E2E_ReqArg1 = start;
    triggerEventUINT8E2E_ReqArg2 = duration;
    triggerEventUINT8E2E_ReqArg3 = debounce;
    if (etsProxy && isAvailable) {
        etsProxy->triggerEventUINT8E2E(triggerEventUINT8E2E_ReqArg1, triggerEventUINT8E2E_ReqArg2, triggerEventUINT8E2E_ReqArg3, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_triggerEventUINT8Multicast(uint32_t start, uint32_t duration, uint32_t debounce) {
    uint32_t triggerEventUINT8Multicast_ReqArg1=0;
    uint32_t triggerEventUINT8Multicast_ReqArg2=0;
    uint32_t triggerEventUINT8Multicast_ReqArg3=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    triggerEventUINT8Multicast_ReqArg1 = start;
    triggerEventUINT8Multicast_ReqArg2 = duration;
    triggerEventUINT8Multicast_ReqArg3 = debounce;
    if (etsProxy && isAvailable) {
        etsProxy->triggerEventUINT8Multicast(triggerEventUINT8Multicast_ReqArg1, triggerEventUINT8Multicast_ReqArg2, triggerEventUINT8Multicast_ReqArg3, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUINT8E2E() {
    uint16_t echoUINT8E2E_ReqArg1 = 0x0;
    uint16_t echoUINT8E2E_ReqArg2 = 0x0;
    uint32_t echoUINT8E2E_ReqArg3 = 0x0;
    uint32_t echoUINT8E2E_ReqArg4 = 0x0;
    uint8_t echoUINT8E2E_ReqArg5 = 0x9a;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint16_t echoUINT8E2E_ResArg1=0;
    uint16_t echoUINT8E2E_ResArg2=0;
    uint32_t echoUINT8E2E_ResArg3=0;
    uint32_t echoUINT8E2E_ResArg4=0;
    uint8_t echoUINT8E2E_ResArg5=0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->echoUINT8E2E(echoUINT8E2E_ReqArg1, echoUINT8E2E_ReqArg2, echoUINT8E2E_ReqArg3, echoUINT8E2E_ReqArg4, echoUINT8E2E_ReqArg5, 
            callStatus, echoUINT8E2E_ResArg1, echoUINT8E2E_ResArg2, echoUINT8E2E_ResArg3, echoUINT8E2E_ResArg4, echoUINT8E2E_ResArg5);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoUINT8E2E_ResArg1:" << std::hex << (uint16_t)echoUINT8E2E_ResArg1
                << " echoUINT8E2E_ResArg2:" << std::hex << (uint16_t)echoUINT8E2E_ResArg2
                << " echoUINT8E2E_ResArg3:" << std::hex << echoUINT8E2E_ResArg3
                << " echoUINT8E2E_ResArg4:" << std::hex << echoUINT8E2E_ResArg4
                << " echoUINT8E2E_ResArg5:" << std::hex << (uint32_t)echoUINT8E2E_ResArg5;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUINT8ArrayLengthTP(int array_length) {
    std::vector<uint8_t> inUINT8Array_ReqArg1 = {};
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::vector<uint8_t> outUINT8Array_ResArg1 = {};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    for (int idx=0; idx<array_length; idx++) {
        inUINT8Array_ReqArg1.push_back(0xab);
    }

    if (etsProxy && isAvailable) {
        etsProxy->echoUINT8ArrayLengthTP(inUINT8Array_ReqArg1, callStatus, outUINT8Array_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:"
                << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " Response:" << std::endl;
            uint32_t outUINT8Array_ResArg1Size = outUINT8Array_ResArg1.size();
            for (uint32_t idx = 0; idx < outUINT8Array_ResArg1Size; idx++) {
                std::cout << std::hex << (uint32_t)outUINT8Array_ResArg1[idx];
            }
            std::cout << '\n';
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUINT8ArrayLengthInTP(int array_length) {
    std::vector<uint8_t> inUINT8Array_ReqArg1 = {};
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint32_t outUINT32_ResArg1=0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    for (int idx=0; idx<array_length; idx++) {
        inUINT8Array_ReqArg1.push_back(0xab);
    }

    if (etsProxy && isAvailable) {
        etsProxy->echoUINT8ArrayLengthInTP(inUINT8Array_ReqArg1, callStatus, outUINT32_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:"
                << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " outUINT32_ResArg1:" << std::dec << outUINT32_ResArg1 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUINT8ArrayLengthOutTP(int array_length) {
    uint32_t inUINT32_ReqArg1;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::vector<uint8_t> outUINT8Array_ResArg1 = {};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    inUINT32_ReqArg1 = (uint32_t)array_length;

    if (etsProxy && isAvailable) {
        etsProxy->echoUINT8ArrayLengthOutTP(inUINT32_ReqArg1, callStatus, outUINT8Array_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:"
                << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " Response:" << std::endl;
            uint32_t outUINT8Array_ResArg1Size = outUINT8Array_ResArg1.size();
            for (uint32_t idx = 0; idx < outUINT8Array_ResArg1Size; idx++) {
                std::cout << std::hex << (uint32_t)outUINT8Array_ResArg1[idx];
            }
            std::cout << '\n';
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_triggerEventUINT8ArrayTP(int array_length) {
    std::vector< uint8_t > triggerEventUINT8ArrayTP_ReqArg1;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    for (int idx=0; idx<array_length; idx++) {
        triggerEventUINT8ArrayTP_ReqArg1.push_back(0xab);
    }

    if (etsProxy && isAvailable) {
        etsProxy->triggerEventUINT8ArrayTP(triggerEventUINT8ArrayTP_ReqArg1, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUINT8ArrayLengthTPNoResponse(int array_length) {
    std::vector< uint8_t > echoUINT8ArrayLengthTPNoResponse_ReqArg1;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    for (int idx=0; idx<array_length; idx++) {
        echoUINT8ArrayLengthTPNoResponse_ReqArg1.push_back(0xab);
    }

    if (etsProxy && isAvailable) {
        etsProxy->echoUINT8ArrayLengthTPNoResponse(echoUINT8ArrayLengthTPNoResponse_ReqArg1, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_triggerEventUINT8ArrayTPNoReqTPPayload(int array_length) {
    uint32_t triggerEventUINT32_ReqArg1=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    triggerEventUINT32_ReqArg1 = (uint32_t)array_length;

    if (etsProxy && isAvailable) {
        etsProxy->triggerEventUINT8ArrayTPNoReqTPPayload(triggerEventUINT32_ReqArg1, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:"
                << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_triggerEventUINT32Periodic(uint32_t event_value) {
    uint32_t triggerEventUINT32_ReqArg1=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    triggerEventUINT32_ReqArg1 = event_value;

    if (etsProxy && isAvailable) {
        etsProxy->triggerEventUINT32Periodic(triggerEventUINT32_ReqArg1, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_triggerEventUINT32UpdateOnChange(uint32_t event_value) {
    uint32_t triggerEventUINT32_ReqArg1=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    triggerEventUINT32_ReqArg1 = event_value;

    if (etsProxy && isAvailable) {
        etsProxy->triggerEventUINT32UpdateOnChange(triggerEventUINT32_ReqArg1, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_array_length_longer_as_message_length_allows_it() {
    /* echoStaticUINT8Array details*/
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x9;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
                << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
                << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
                << " Type:" << (uint32_t)msgType << std::endl;
            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << " bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUINT8Array_ReqArg1= 1376 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x5);
            _payload_data.push_back(0x58);
            /* Update Array length field */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x5);
            _payload_data.push_back(0x58);
            /* echoUINT8Array_ReqArg2 */
            /* PRS_SOMEIP_00730 UDP packet length 1400 bytes
               8 bytes UDP header + 16 bytes SOMEIP header + 1376 bytes SOMEIP payload
            */
            for (int idx=0; idx<1368; idx++) {
                _payload_data.push_back(0xab);
            }
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_array_length_too_long() {
    /* echoStaticUINT8Array details*/
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x9;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUINT8Array_ReqArg1= 20 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x14);
            /* Update Array length field */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x14);
            /* echoUINT8Array_ReqArg2 */
            /* PRS_SOMEIP_00730 UDP packet length 1400 bytes
               8 bytes UDP header + 16 bytes SOMEIP header + 20 bytes SOMEIP payload
            */
            for (int idx=0; idx<5; idx++) {
                _payload_data.push_back(0xab);
            }
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_array_length_too_short_strips_payload() {
    /* echoStaticUINT8Array details*/
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x9;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUINT8Array_ReqArg1=5*/
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x5);
            /* Update Array length field */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x5);
            /* echoUINT8Array_ReqArg2 */
            /* PRS_SOMEIP_00730 UDP packet length 1400 bytes
               8 bytes UDP header + 16 bytes SOMEIP header + 10 bytes SOMEIP payload
            */
            for (int idx=0; idx<10; idx++) {
                _payload_data.push_back(0xab);
            }
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_burst_test(int no_of_iteration) {
    uint8_t echoUINT8_ReqArg1=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint8_t echoUINT8_ResArg1=0;
    std::cout << "etsProxyImpl::" << __func__ << "no_of_iteration:" << no_of_iteration << std::endl;

    if (etsProxy && isAvailable) {
        for (int idx=1; idx<=no_of_iteration; idx++) {
            echoUINT8_ReqArg1 = (uint8_t)idx;
            etsProxy->echoUINT8(echoUINT8_ReqArg1, callStatus, echoUINT8_ResArg1);
            if (callStatus != CommonAPI::CallStatus::SUCCESS) {
                std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
            }
            else {
                std::cout << "etsProxyImpl::" << __func__ << " echoUINT8_ResArg1:" << (uint32_t)echoUINT8_ResArg1 << std::endl;
            }
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::invoke_echoUTF16DYNAMIC_length_too_long_for_string() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x16;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF16DYNAMIC_ReqArg1=4 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x4);
            /* echoUTF16DYNAMIC_ReqArg2=abcd
               payload:BOM(fe-ff-00)+2*string_length(UTF16)+null_termination=12bytes
               Update String length field more than actual length (>12bytes)
            */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x14);
            /* Update payload */
            _payload_data.push_back(0xfe);
            _payload_data.push_back(0xff);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x61);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x62);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x63);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x64);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF16DYNAMIC_length_too_short_for_malformed_string() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x16;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF16DYNAMIC_ReqArg1=4 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x4);
            /* echoUTF16DYNAMIC_ReqArg2=abcd
               payload:BOM(fe-ff-00)+2*string_length(UTF16)+null_termination=12bytes
               Update String length field less than actual length (<12bytes)
            */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x2);
            /* Update payload */
            _payload_data.push_back(0xfe);
            _payload_data.push_back(0xff);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x61);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x62);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x63);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x64);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF16DYNAMIC_length_too_short_for_string() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x16;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF16DYNAMIC_ReqArg1=4 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x4);
            /* echoUTF16DYNAMIC_ReqArg2=abcd
               payload:BOM(fe-ff-00)+2*string_length(UTF16)+null_termination=12bytes
               Update String length field less than actual length but greater than BOM length(<12bytes)
            */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x5);
            /* Update payload */
            _payload_data.push_back(0xfe);
            _payload_data.push_back(0xff);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x61);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x62);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x63);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x64);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF16DYNAMIC_odd_number_before_termination() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x16;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF16DYNAMIC_ReqArg1=4 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x4);
            /* echoUTF16DYNAMIC_ReqArg2=abcd
               payload:BOM(fe-ff-00)+2*string_length(UTF16)+null_termination=12bytes
               Update String with odd number before termination
            */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0xc);
            /* Update payload */
            _payload_data.push_back(0xfe);
            _payload_data.push_back(0xff);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x61);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x62);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x63);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x64);
            _payload_data.push_back(0x0);
            /* Added odd number before termination for string */
            _payload_data.push_back(0x3);
            _payload_data.push_back(0x0);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF16DYNAMIC_with_odd_number_after_termination() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x16;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF16DYNAMIC_ReqArg1=4 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x4);
            /* echoUTF16DYNAMIC_ReqArg2=abcd
               payload:BOM(fe-ff-00)+2*string_length(UTF16)+null_termination=12bytes
               Update String with odd number after termination
            */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0xc);
            /* Update payload */
            _payload_data.push_back(0xfe);
            _payload_data.push_back(0xff);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x61);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x62);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x63);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x64);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            /* Added odd number before termination for string */
            _payload_data.push_back(0x3);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF16DYNAMIC_wrong_BOM() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x16;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF16DYNAMIC_ReqArg1=4 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x4);
            /* echoUTF16DYNAMIC_ReqArg2=abcd
               payload:BOM(fe-ff-00)+2*string_length(UTF16)+null_termination=12bytes
               Update String with wrong BOM
            */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0xc);
            /* Update payload */
            _payload_data.push_back(0xff);
            _payload_data.push_back(0xff);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x61);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x62);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x63);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x64);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF8DYNAMIC_length_too_long_for_string() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x15;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF8DYNAMIC_ReqArg1=4 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x4);
            /* echoUTF8DYNAMIC_ReqArg2=abcd
               payload:BOM(ef-bb-bf)+string_length(UTF8)+null_termination=8bytes
               Update String length field more than actual length (>8bytes)
            */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x14);
            /* Update payload */
            _payload_data.push_back(0xef);
            _payload_data.push_back(0xbb);
            _payload_data.push_back(0xbf);
            _payload_data.push_back(0x61);
            _payload_data.push_back(0x62);
            _payload_data.push_back(0x63);
            _payload_data.push_back(0x64);
            _payload_data.push_back(0x0);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF8DYNAMIC_length_too_short_for_malformed_string() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x15;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF8DYNAMIC_ReqArg1=4 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x4);
            /* echoUTF8DYNAMIC_ReqArg2=abcd
               payload:BOM(ef-bb-bf)+string_length(UTF8)+null_termination=8bytes
               Update String length field less than actual length (<8bytes)
            */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x2);
            /* Update payload */
            _payload_data.push_back(0xef);
            _payload_data.push_back(0xbb);
            _payload_data.push_back(0xbf);
            _payload_data.push_back(0x61);
            _payload_data.push_back(0x62);
            _payload_data.push_back(0x63);
            _payload_data.push_back(0x64);
            _payload_data.push_back(0x0);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF8DYNAMIC_length_too_short_for_string() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x15;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF8DYNAMIC_ReqArg1=4 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x4);
            /* echoUTF8DYNAMIC_ReqArg2=abcd
               payload:BOM(ef-bb-bf)+string_length(UTF8)+null_termination=8bytes
               Update String length field less than actual length but greater than BOM length(<8bytes)
            */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x5);
            /* Update payload */
            _payload_data.push_back(0xef);
            _payload_data.push_back(0xbb);
            _payload_data.push_back(0xbf);
            _payload_data.push_back(0x61);
            _payload_data.push_back(0x62);
            _payload_data.push_back(0x63);
            _payload_data.push_back(0x64);
            _payload_data.push_back(0x0);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF8DYNAMIC_wrong_BOM() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x15;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF8DYNAMIC_ReqArg1=4 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x4);
            /* echoUTF8DYNAMIC_ReqArg2=abcd
               payload:BOM(ef-bb-bf)+string_length(UTF8)+null_termination=8bytes
               Update String with wrong BOM
            */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x8);
            /* Update payload */
            _payload_data.push_back(0xff);
            _payload_data.push_back(0xbb);
            _payload_data.push_back(0xbf);
            _payload_data.push_back(0x61);
            _payload_data.push_back(0x62);
            _payload_data.push_back(0x63);
            _payload_data.push_back(0x64);
            _payload_data.push_back(0x0);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUTF16FIXED_with_odd_number() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x14;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF16STATIC_ReqArg1=string of 64 bytes
               payload:BOM(fe-ff-00)+ 30 bytes string_length(UTF16)+null_termination=64bytes
               Update String with odd number after termination
            */
            _payload_data.push_back(0xfe);
            _payload_data.push_back(0xff);
            _payload_data.push_back(0x0);
            for (int idx=0; idx<30; idx++) {
                _payload_data.push_back(0x61);
                _payload_data.push_back(0x0);
            }
            _payload_data.push_back(0x0);
            /* Added odd number before termination for string */
            _payload_data.push_back(0x3);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_string_UTF16FIXED_too_long() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x14;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF16STATIC_ReqArg1=string of 64 bytes
               payload:BOM(fe-ff-00)+ 30 bytes string_length(UTF16)+null_termination=64bytes
               Update String with length > 64 bytes
            */
            _payload_data.push_back(0xfe);
            _payload_data.push_back(0xff);
            _payload_data.push_back(0x0);
            for (int idx=0; idx<70; idx++) {
                _payload_data.push_back(0x61);
                _payload_data.push_back(0x0);
            }
            _payload_data.push_back(0x0);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_string_UTF16FIXED_too_short() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x14;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF16STATIC_ReqArg1=string of 64 bytes
               payload:BOM(fe-ff-00)+30 bytes string_length(UTF16)+null_termination=64bytes
               Update String with length < 64 bytes
            */
            _payload_data.push_back(0xfe);
            _payload_data.push_back(0xff);
            _payload_data.push_back(0x0);
            for (int idx=0; idx<10; idx++) {
                _payload_data.push_back(0x61);
                _payload_data.push_back(0x0);
            }
            _payload_data.push_back(0x0);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_string_UTF8FIXED_too_long() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x13;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
            << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
            << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
            << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF8FIXED_ReqArg1=string of 64 bytes
               payload:BOM(ef-bb-bf)+60 bytes string_length(UTF16)+null_termination=64bytes
               Update String with length > 64 bytes
            */
            _payload_data.push_back(0xef);
            _payload_data.push_back(0xbb);
            _payload_data.push_back(0xbf);
            for (int idx=0; idx<70; idx++) {
                _payload_data.push_back(0x61);
            }
            _payload_data.push_back(0x0);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_string_UTF8FIXED_too_short() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x13;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
                << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
                << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
                << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUTF8FIXED_ReqArg1=string of 64 bytes
               payload:BOM(ef-bb-bf)+60 bytes string_length(UTF16)+null_termination=64bytes
               Update String with length < 64 bytes
            */
            _payload_data.push_back(0xef);
            _payload_data.push_back(0xbb);
            _payload_data.push_back(0xbf);
            for (int idx=0; idx<10; idx++) {
                _payload_data.push_back(0x61);
            }
            _payload_data.push_back(0x0);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_Wrong_Interface_Version() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x8;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
                << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
                << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
                << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            /* wrong interface version */
            _request->set_interface_version(0x2);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUINT8_ReqArg1=0x12 */
            _payload_data.push_back(0x12);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

/* Checking for Fire and Forget Method */
void etsProxyImpl::invoke_Wrong_Message_Type() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x3;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
                << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
                << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
                << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            /* wrong message type */
            _request->set_message_type(vsomeip::message_type_e::MT_REQUEST);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* triggerEventUINT8_ReqArg1=0x2 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x2);
            /* triggerEventUINT8_ReqArg1=0x4 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x4);
            /* triggerEventUINT8_ReqArg1=0x2 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x2);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_Wrong_Method_ID() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    /* wrong method id */
    vsomeip::method_t method_id = 0x1001;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
                << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
                << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
                << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUINT8_ReqArg1=0x12 */
            _payload_data.push_back(0x12);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_Fire_And_Forget_Wrong_Method_ID() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    /* wrong method id */
    vsomeip::method_t method_id = 0x1001;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
                << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
                << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
                << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            _request->set_message_type(vsomeip::message_type_e::MT_REQUEST_NO_RETURN);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUINT8_ReqArg1=0x12 */
            _payload_data.push_back(0x12);
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_Wrong_Service_ID(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << " remote_addresss:" << remote_address
        << " remote port:" << (uint32_t)remote_port << " local_address:" << local_address
        << " local_port:" << (uint32_t)local_port << std::endl;

    /* malformed message with wrong service id */
    const uint8_t request_data[] = {
        0x01, 0xff,   	        /* service id */
        0x00, 0x08,   	        /* method id */
        0x00, 0x00, 0x00, 0x09,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01, 		    /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x00, 			        /* request type */
        0x00,			        /* return code */
        0x12			        /* payload */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(request_data, sizeof(request_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;

        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_Wrong_SOMEIP_Protocol_Version(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << " remote_addresss:" << remote_address
        << " remote port:" << (uint32_t)remote_port << " local_address:" << local_address
        << " local_port:" << (uint32_t)local_port << std::endl;

    /* malformed message with wrong protocol version */
    const uint8_t request_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x08,   	        /* method id */
        0x00, 0x00, 0x00, 0x09,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01, 		    /* session id */
        0xfe, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x00, 			        /* request type */
        0x00,			        /* return code */
        0x12			        /* payload */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(request_data, sizeof(request_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

/*  Do not reply to messages already carrying an error */
void etsProxyImpl::invoke_Wrong_Return_Code(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    std::cout << "etsProxyImpl::" << __func__ << " remote_addresss:" << remote_address
        << " remote port:" << (uint32_t)remote_port << " local_address:" << local_address
        << " local_port:" << (uint32_t)local_port << std::endl;

    /* malformed message with unknown return code */
    const uint8_t request_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x08,   	        /* method id */
        0x00, 0x00, 0x00, 0x09,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01, 		    /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x00, 			        /* request type */
        0x1f,			        /* unknown return code */
        0x12			        /* payload */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(request_data, sizeof(request_data)), udp_server_endpoint);

        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_Length_equals_0_Test(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << " remote_addresss:" << remote_address
        << " remote port:" << (uint32_t)remote_port << " local_address:" << local_address
        << " local_port:" << (uint32_t)local_port << std::endl;

    /* malformed message with zero length */
    const uint8_t request_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x08,   	        /* method id */
        0x00, 0x00, 0x00, 0x00,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01, 		    /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x00, 			        /* request type */
        0x00,			        /* return code */
        0x12			        /* payload */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(request_data, sizeof(request_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_Length_smaller_than_8_Test(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << " remote_addresss:" << remote_address
        << " remote port:" << (uint32_t)remote_port << " local_address:" << local_address
        << " local_port:" << (uint32_t)local_port << std::endl;

    /* malformed message with smaller than 8 byte length */
    const uint8_t request_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x08,   	        /* method id */
        0x00, 0x00, 0x00, 0x06,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01, 		    /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x00, 			        /* request type */
        0x00,			        /* return code */
        0x12			        /* payload */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(request_data, sizeof(request_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}
void etsProxyImpl::invoke_Length_way_too_long(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << " remote_addresss:" << remote_address
        << " remote port:" << (uint32_t)remote_port << " local_address:" << local_address
        << " local_port:" << (uint32_t)local_port << std::endl;

    /* malformed message with larger length */
    const uint8_t request_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x08,   	        /* method id */
        0x00, 0xff, 0xff, 0xff,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01, 		    /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x00, 			        /* request type */
        0x00,			        /* return code */
        0x12			        /* payload */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(request_data, sizeof(request_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}
void etsProxyImpl::invoke_SD_Discover_Port_and_IP() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::major_version_t major = 0x1;
    vsomeip::minor_version_t minor = 0x0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->request_service(service_id, instance_id, major, minor);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}
void etsProxyImpl::invoke_Sending_two_SOMEIP_Messages_in_a_row(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* three message in a single udp packet */
    const uint8_t request_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x08,   	        /* method id */
        0x00, 0x00, 0x00, 0x09,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01, 		    /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x00, 			        /* request type */
        0x00,			        /* return code */
        0x10,			        /* payload */
        0x01, 0x01,   	        /* service id */
        0x00, 0x08,   	        /* method id */
        0x00, 0x00, 0x00, 0x09,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x02, 		    /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x00, 			        /* request type */
        0x00,			        /* return code */
        0x11,			        /* payload */
        0x01, 0x01,   	        /* service id */
        0x00, 0x08,   	        /* method id */
        0x00, 0x00, 0x00, 0x09,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x03, 		    /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x00, 			        /* request type */
        0x00,			        /* return code */
        0x12			        /* payload */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(request_data, sizeof(request_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}
void etsProxyImpl::invoke_UINT8Array_with_Length_0_strips_Payload() {
    /* echoStaticUINT8Array details*/
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x9;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
                << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
                << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
                << " Type:" << (uint32_t)msgType << std::endl;
            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x1);
            _request->set_reliable(false);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            /* echoUINT8Array_ReqArg1= 0 */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            /* Update Array length field */
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            _payload_data.push_back(0x0);
            /* echoUINT8Array_ReqArg2 */
            /* PRS_SOMEIP_00730 UDP packet length 1400 bytes
               8 bytes UDP header + 16 bytes SOMEIP header + (<=1376 bytes) SOMEIP payload
            */
            for (int idx=0; idx<4; idx++) {
                _payload_data.push_back(0xab);
            }
            _payload->set_data(_payload_data);
            _request->set_payload(_payload);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}
void etsProxyImpl::invoke_Unaligned_SOMEIP_Messages_overUDP(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* three message in a single udp packet */
    const uint8_t request_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x09,   	        /* method id */
        0x00, 0x00, 0x00, 0x11,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01, 		    /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x00, 			        /* request type */
        0x00,			        /* return code */
        0x00, 0x00, 0x00, 0x01,	/* payload */
        0x00, 0x00, 0x00, 0x01,	/* payload */
        0x10,			        /* payload */
        0x01, 0x01,   	        /* service id */
        0x00, 0x09,   	        /* method id */
        0x00, 0x00, 0x00, 0x14,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x02, 		    /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x00, 			        /* request type */
        0x00,			        /* return code */
        0x00, 0x00, 0x00, 0x04,	/* payload */
        0x00, 0x00, 0x00, 0x04,	/* payload */
        0x11, 0x12, 0x13, 0x14,	/* payload */
        0x01, 0x01,   	        /* service id */
        0x00, 0x09,   	        /* method id */
        0x00, 0x00, 0x00, 0x14,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x03, 		    /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x00, 			        /* request type */
        0x00,			        /* return code */
        0x00, 0x00, 0x00, 0x04,	/* payload */
        0x00, 0x00, 0x00, 0x04,	/* payload */
        0x15, 0x16, 0x17, 0x18,	/* payload */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(request_data, sizeof(request_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Answer_multiple_subscribes_together(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* three message in a single udp packet */
    uint8_t request_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x50, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x30, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x02,             /* event group */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x06,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        std::memcpy(&request_data[sizeof(request_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        request_data[sizeof(request_data)-2] = (local_port>>8) & 0xff;
        request_data[sizeof(request_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(request_data, sizeof(request_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Check_Reaction_to_a_Subscribe_with_ttl_0(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with TTL=0 */
    uint8_t request_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x00,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x02,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        std::memcpy(&request_data[sizeof(request_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        request_data[sizeof(request_data)-2] = (local_port>>8) & 0xff;
        request_data[sizeof(request_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(request_data, sizeof(request_data)), udp_server_endpoint);

        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Consider_Entries_Order(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    /* stop subscribe(TTL=0) and subscribe event group */
    uint8_t stop_subscribe_subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x40, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x20, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x00,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        /* send stop subscribe and subscribe */
        std::memcpy(&stop_subscribe_subscribe_data[sizeof(stop_subscribe_subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        stop_subscribe_subscribe_data[sizeof(stop_subscribe_subscribe_data)-2] = (local_port>>8) & 0xff;
        stop_subscribe_subscribe_data[sizeof(stop_subscribe_subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(stop_subscribe_subscribe_data, sizeof(stop_subscribe_subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Do_not_specify_a_port(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with udp port missing in endpoint options */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Do_not_specify_IPv4_Adress(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with ip4 address missing */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x20, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Empty_Entries_Array(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with empty entry array */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x00, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Empty_Option(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with empty option length */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x00,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Empty_Options_Array(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with empty option array */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x00, /* option length */
        0x00, 0x00,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Entries_Length_wrong_combined(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with wrong entry array length */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x40, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* shortened entry array length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x02,             /* event group */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option array length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Options_Array_too_short(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with option array too short */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x02, /* option array length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Request_non_existing_EventgroupID(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with unknown event group */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x15,             /* unknown event group */
        0x00, 0x00, 0x00, 0x0c, /* option array length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Request_non_existing_InstanceID(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with unknown instance id */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x11, 0x11,             /* unknown instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option array length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Request_non_existing_Major_Version(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with unknown major version */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x11,                   /* unknown major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Request_non_existing_ServiceID(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group unknown service id */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x11, 0x11,             /* unknown service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Reserved_Field_Endpoint_Option_set(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with reserved field set */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option array length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x01,                   /* reserved field set */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x01,                   /* reserved field set */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_SOMEIP_Length_shorter_as_expected(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with shorter length */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x20, /* wrong length - truncate option array */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Specify_an_unexisting_IPv4_Address(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with incorrect endpoint ip address */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0xff, 0xff, 0xff, 0xff, /* incorrect endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Subscribe_after_StopSubscribe(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* stop subscribe(TTL=0) and subscribe event group */
    uint8_t stop_subscribe_subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x40, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x20, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x00,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send stop subscribe and subscribe */
        std::memcpy(&stop_subscribe_subscribe_data[sizeof(stop_subscribe_subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        stop_subscribe_subscribe_data[sizeof(stop_subscribe_subscribe_data)-2] = (local_port>>8) & 0xff;
        stop_subscribe_subscribe_data[sizeof(stop_subscribe_subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(stop_subscribe_subscribe_data, sizeof(stop_subscribe_subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_SubscribeEventgroup_with_unallowed_option_ip(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        /* set DUT's IP address as endpoint IP address */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_server_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_SubscribeEventgroup_with_unallowed_option_ip_2(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group invalid endpoint address */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x6f, 0x6f, 0x6f, 0x6f, /* invalid endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Unknown_Option_type(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group with unknown option type */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option array length */
        0x00, 0x09,             /* option length */
        0xff,                   /* unknown option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Unreferenced_option(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x48, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00,             /* udp port */
        0x00, 0x15,             /* ipv6 option length */
        0x06,                   /* ipv6 option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* ipv6 endpoint address */
        0x00, 0x00, 0x00, 0x00, /* ipv6 endpoint address */
        0x00, 0x00, 0x00, 0x00, /* ipv6 endpoint address */
        0x00, 0x00, 0x00, 0x00, /* ipv6 endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-32], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-26] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-25] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Unused_data_after_Options_Array(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x35, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00,              /* udp port */
        0x30, 0x30, 0x3a, 0x30, 0x31 /* unused data */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-13], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-7] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-6] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Unused_data_after_Options_Array_wrong_length(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00,              /* udp port */
        0x30, 0x30, 0x3a, 0x30, 0x31 /* unused data */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-13], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-7] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-6] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_Subscribe_using_wrong_SOMEIP_MessageID(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id (sd service id) */
        0xff, 0xff,             /* method id (sd wrong method id) */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_Eventgroup_EventsAndFieldsUnreliable_5(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x05,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;


        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Calling_same_ports_before_and_after_suspendInterface(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    return;
}

void etsProxyImpl::invoke_SD_Check_Reboot_Detection_separate_multicast_and_unicast(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    return;
}

void etsProxyImpl::invoke_SD_Check_Reboot_Detection_Server_Side(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    return;
}

void etsProxyImpl::invoke_SD_Check_subscribe_eventgroup_ttl_expired(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint32_t triggerEventUINT8_ReqArg1=0;
    uint32_t triggerEventUINT8_ReqArg2=0;
    uint32_t triggerEventUINT8_ReqArg3=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        uINT8Valuesubscription = etsProxy->getTestEventUINT8Event().subscribe([&](const uint8_t& uINT8Value) {
            std::cout << "etsProxyImpl::invoke_SD_Check_subscribe_eventgroup_ttl_expired TestEventUINT8 Notification:" << std::hex << (uint32_t)uINT8Value << std::endl;
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    /* wait for subscription expiry (TTL=3sec)*/
    sleep(4);

    triggerEventUINT8_ReqArg1 = 2;
    triggerEventUINT8_ReqArg2 = 4;
    triggerEventUINT8_ReqArg3 = 2;
    if (etsProxy && isAvailable) {
        etsProxy->triggerEventUINT8(triggerEventUINT8_ReqArg1, triggerEventUINT8_ReqArg2, triggerEventUINT8_ReqArg3, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Deregister_from_Eventgroup(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint32_t triggerEventUINT8_ReqArg1=0;
    uint32_t triggerEventUINT8_ReqArg2=0;
    uint32_t triggerEventUINT8_ReqArg3=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        uINT8Valuesubscription = etsProxy->getTestEventUINT8Event().subscribe([&](const uint8_t& uINT8Value) {
            std::cout << "etsProxyImpl::invoke_SD_Check_subscribe_eventgroup_ttl_expired TestEventUINT8 Notification:" << std::hex << (uint32_t)uINT8Value << std::endl;
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    if (etsProxy && isAvailable) {
        etsProxy->getTestEventUINT8Event().unsubscribe(uINT8Valuesubscription);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    triggerEventUINT8_ReqArg1 = 2;
    triggerEventUINT8_ReqArg2 = 4;
    triggerEventUINT8_ReqArg3 = 2;
    if (etsProxy && isAvailable) {
        etsProxy->triggerEventUINT8(triggerEventUINT8_ReqArg1, triggerEventUINT8_ReqArg2, triggerEventUINT8_ReqArg3, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_ResetInterface() {
    uint32_t value = 5;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    tester_get_TestFieldUINT8();
    sleep(1);
    tester_set_TestFieldUINT8(value);
    sleep(1);
    invoke_resetInterface();
    sleep(1);
    tester_get_TestFieldUINT8();

    return;
}

void etsProxyImpl::invoke_SD_SuspendInterface() {
    uint32_t start = 1;
    uint32_t suspend_duration = 4;
    uint32_t value = 10;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    tester_get_TestFieldUINT8();
    sleep(1);
    tester_set_TestFieldUINT8(value);
    sleep(1);
    invoke_suspendInterface(start, suspend_duration);
    sleep(start+suspend_duration+1);
    tester_get_TestFieldUINT8();

    return;
}

void etsProxyImpl::invoke_SD_Send_triggerEventUINT8_Eventgroup_2(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    uint32_t triggerEventUINT8_ReqArg1=0;
    uint32_t triggerEventUINT8_ReqArg2=0;
    uint32_t triggerEventUINT8_ReqArg3=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x02,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;

    
        triggerEventUINT8_ReqArg1 = 2;
        triggerEventUINT8_ReqArg2 = 4;
        triggerEventUINT8_ReqArg3 = 2;
        if (etsProxy && isAvailable) {
            etsProxy->triggerEventUINT8(triggerEventUINT8_ReqArg1, triggerEventUINT8_ReqArg2, triggerEventUINT8_ReqArg3, callStatus);
            if (callStatus != CommonAPI::CallStatus::SUCCESS) {
                std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Send_triggerEventUINT8Array_Eventgroup_2(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    uint32_t triggerEventUINT8Array_ReqArg1=0;
    uint32_t triggerEventUINT8Array_ReqArg2=0;
    uint32_t triggerEventUINT8Array_ReqArg3=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x02,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;

    
        triggerEventUINT8Array_ReqArg1 = 2;
        triggerEventUINT8Array_ReqArg2 = 4;
        triggerEventUINT8Array_ReqArg3 = 2;
        if (etsProxy && isAvailable) {
            etsProxy->triggerEventUINT8Array(triggerEventUINT8Array_ReqArg1, triggerEventUINT8Array_ReqArg2, triggerEventUINT8Array_ReqArg3, callStatus);
            if (callStatus != CommonAPI::CallStatus::SUCCESS) {
                std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Send_triggerEventUINT8E2E_Eventgroup_2(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    uint32_t triggerEventUINT8E2E_ReqArg1=0;
    uint32_t triggerEventUINT8E2E_ReqArg2=0;
    uint32_t triggerEventUINT8E2E_ReqArg3=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x02,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;

    
        triggerEventUINT8E2E_ReqArg1 = 2;
        triggerEventUINT8E2E_ReqArg2 = 4;
        triggerEventUINT8E2E_ReqArg3 = 2;
        if (etsProxy && isAvailable) {
            etsProxy->triggerEventUINT8E2E(triggerEventUINT8E2E_ReqArg1, triggerEventUINT8E2E_ReqArg2, triggerEventUINT8E2E_ReqArg3, callStatus);
            if (callStatus != CommonAPI::CallStatus::SUCCESS) {
                std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Send_triggerEventUINT8Multicast_Eventgroup_6(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    uint32_t triggerEventUINT8Multicast_ReqArg1=0;
    uint32_t triggerEventUINT8Multicast_ReqArg2=0;
    uint32_t triggerEventUINT8Multicast_ReqArg3=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    /* subscribe event group */
    uint8_t subscribe_data[] = {
        0xff, 0xff,             /* service id */
        0x81, 0x00,             /* method id */
        0x00, 0x00, 0x00, 0x30, /* length */
        0x00, 0x00,             /* client id */
        0x00, 0x01,             /* session id */
        0x01,                   /* protocol version */
        0x01,                   /* interface version */
        0x02,                   /* message type */
        0x00,                   /* return code */
        0xc0,                   /* flag */
        0x00, 0x00, 0x00,       /* reserved */
        0x00, 0x00, 0x00, 0x10, /* entry length */
        0x06,                   /* type subscribe event group */
        0x00, 0x00, 0x10,       /* index */
        0x01, 0x01,             /* service id */
        0x00, 0x01,             /* instance id */
        0x01,                   /* major version */
        0x00, 0x00, 0x03,       /* TTL */
        0x00,                   /* reserved */
        0x00,                   /* initial data request */
        0x00, 0x06,             /* event group */
        0x00, 0x00, 0x00, 0x0c, /* option length */
        0x00, 0x09,             /* option length */
        0x04,                   /* option type */
        0x00,                   /* reserved */
        0x00, 0x00, 0x00, 0x00, /* endpoint address */
        0x00,                   /* reserved */
        0x11,                   /* udp protocol */
        0x00, 0x00              /* udp port */
    };

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), multicast_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), multicast_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        udp_client_sock.open(udp_client_endpoint.protocol(), err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to open socket:" << err_code.message() << std::endl;
        }
        udp_client_sock.set_option(boost::asio::socket_base::reuse_address(true));
        udp_client_sock.set_option(boost::asio::socket_base::linger(true, 0));
        int optval = 1;
        setsockopt(udp_client_sock.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        udp_client_sock.bind(udp_client_endpoint, err_code);
        if (err_code) {
            std::cout << "etsProxyImpl::" << __func__ << " Failed to bind local address:" << err_code.message() << std::endl;
        }

        io_ctx.run();

        /* send subscribe */
        std::memcpy(&subscribe_data[sizeof(subscribe_data)-8], &udp_client_endpoint.address().to_v4().to_bytes()[0], 4);
        subscribe_data[sizeof(subscribe_data)-2] = (local_port>>8) & 0xff;
        subscribe_data[sizeof(subscribe_data)-1] = local_port & 0xff;
        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(subscribe_data, sizeof(subscribe_data)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;

    
        triggerEventUINT8Multicast_ReqArg1 = 2;
        triggerEventUINT8Multicast_ReqArg2 = 4;
        triggerEventUINT8Multicast_ReqArg3 = 2;
        if (etsProxy && isAvailable) {
            etsProxy->triggerEventUINT8Multicast(triggerEventUINT8Multicast_ReqArg1, triggerEventUINT8Multicast_ReqArg2, triggerEventUINT8Multicast_ReqArg3, callStatus);
            if (callStatus != CommonAPI::CallStatus::SUCCESS) {
                std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
            }
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_SD_Interface_Version(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port) {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;
    return;
}

void etsProxyImpl::invoke_activateTestSerivce(uint32_t service_id, uint32_t instance_id) {
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;
    if (etsProxy && isAvailable) {
        etsProxy->activateTestSerivce(service_id, instance_id, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::invoke_deactivateTestSerivce(uint32_t service_id, uint32_t instance_id) {
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;
    if (etsProxy && isAvailable) {
        etsProxy->deactivateTestSerivce(service_id, instance_id, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::invoke_TP_Verify_ErrorDuringReception(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    /* payload data type array
       number of element sent in the array 4000 bytes
       array length field is 4 bytes
       so, actual payload to vsomeip send buffer is 4004 bytes.
    */
    uint8_t send_buffer_segment1[1412] = {0};
    int segment1_payload_length = 1392+4+16;
    uint8_t send_buffer_segment2[1412] = {0};
    int segment2_payload_length = 1392+4+16;
    uint8_t send_buffer_segment3[1240] = {0};
    int segment3_payload_length = 1220+4+16;
    int offset = 0;
    std::cout << "etsProxyImpl::" << __func__ << " remote_addresss:" << remote_address
        << " remote port:" << (uint32_t)remote_port << " local_address:" << local_address
        << " local_port:" << (uint32_t)local_port << std::endl;

    /* someip header data */
    const uint8_t segment1_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x05, 0x7c,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    /* Incorrect Message ID(service id + method id) and Request ID(client id + session id) */
    const uint8_t segment2_header_data[] = {
        0x00, 0x00,   	        /* service id */
        0x00, 0x02,   	        /* method id */
        0x00, 0x00, 0x05, 0x7c,	/* length */
        0x00, 0x00,		        /* client id */
        0x00, 0x02,		        /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    /* Incorrect Message ID(service id + method id) and Request ID(client id + session id) */
    const uint8_t segment3_header_data[] = {
        0x00, 0x00,   	        /* service id */
        0x00, 0x02,   	        /* method id */
        0x00, 0x00, 0x04, 0xd0,	/* length */
        0x00, 0x00,		        /* client id */
        0x00, 0x02,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    /* TP header (offset(28bit) + reserved(3bit) + more segment(1bit)) */
    const uint8_t tp_hader_data_segment1[] = {0x00, 0x00, 0x00, 0x01};
    const uint8_t tp_hader_data_segment2[] = {0x00, 0x00, 0x05, 0x71};
    const uint8_t tp_hader_data_segment3[] = {0x00, 0x00, 0x0a, 0xe0};
    const uint8_t array_payload_length[] = {0x00, 0x00, 0x0f, 0xa0}; /* 0xfa0 -> 4000 (array payload length) */

    /* prepare first segment */
    memset(send_buffer_segment1, 0, 1412);
    memcpy(&send_buffer_segment1[0], &segment1_header_data[0], sizeof(segment1_header_data));
    offset = sizeof(segment1_header_data);
    memcpy(&send_buffer_segment1[offset], &tp_hader_data_segment1[0], sizeof(tp_hader_data_segment1));
    offset += sizeof(tp_hader_data_segment1);
    memcpy(&send_buffer_segment1[offset], &array_payload_length[0], sizeof(array_payload_length));
    offset += sizeof(array_payload_length);
    for (int idx=offset; idx<segment1_payload_length; idx++) {
        send_buffer_segment1[idx]=0xab;
    }
    /* prepare second segment */
    offset = 0;
    memset(send_buffer_segment2, 0, 1412);
    memcpy(&send_buffer_segment2[0], &segment2_header_data[0], sizeof(segment2_header_data));
    offset = sizeof(segment2_header_data);
    memcpy(&send_buffer_segment2[offset], &tp_hader_data_segment2[0], sizeof(tp_hader_data_segment2));
    offset += sizeof(tp_hader_data_segment2);
    for (int idx=offset; idx<segment2_payload_length; idx++) {
        send_buffer_segment2[idx]=0xab;
    }
    /* prepare third segment */
    offset = 0;
    memset(send_buffer_segment3, 0, 1240);
    memcpy(&send_buffer_segment3[0], &segment3_header_data[0], sizeof(segment3_header_data));
    offset = sizeof(segment3_header_data);
    memcpy(&send_buffer_segment3[offset], &tp_hader_data_segment3[0], sizeof(tp_hader_data_segment3));
    offset += sizeof(tp_hader_data_segment3);
    for (int idx=offset; idx<segment3_payload_length; idx++) {
        send_buffer_segment3[idx]=0xab;
    }

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment1, sizeof(send_buffer_segment1)), udp_server_endpoint);
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment2, sizeof(send_buffer_segment2)), udp_server_endpoint);
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment3, sizeof(send_buffer_segment3)), udp_server_endpoint);

        sleep(5);
        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_TP_Verify_ReceptionBufferManagement(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    /* payload data type array
       number of element sent in the array 5000 bytes
       array length field is 4 bytes
       so, actual payload to vsomeip send buffer is 5004 bytes.
    */
    uint8_t send_buffer_segment1[1412] = {0};
    int segment1_payload_length = 1392+4+16;
    uint8_t send_buffer_segment2[1412] = {0};
    int segment2_payload_length = 1392+4+16;
    uint8_t send_buffer_segment3[1412] = {0};
    int segment3_payload_length = 1392+4+16;
    uint8_t send_buffer_segment4[848] = {0};
    int segment4_payload_length = 828+4+16;
    int offset = 0;
    std::cout << "etsProxyImpl::" << __func__ << " remote_addresss:" << remote_address
        << " remote port:" << (uint32_t)remote_port << " local_address:" << local_address
        << " local_port:" << (uint32_t)local_port << std::endl;

    /* someip header data */
    const uint8_t segment1_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x05, 0x7c,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    const uint8_t segment2_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x05, 0x7c,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    const uint8_t segment3_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x05, 0x7c,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    const uint8_t segment4_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x03, 0x48,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    /* TP header (offset(28bit) + reserved(3bit) + more segment(1bit)) */
    const uint8_t tp_hader_data_segment1[] = {0x00, 0x00, 0x00, 0x01};
    const uint8_t tp_hader_data_segment2[] = {0x00, 0x00, 0x05, 0x71};
    const uint8_t tp_hader_data_segment3[] = {0x00, 0x00, 0x0a, 0xe1};
    const uint8_t tp_hader_data_segment4[] = {0x00, 0x00, 0x10, 0x50};
    const uint8_t array_payload_length[] = {0x00, 0x00, 0x13, 0x88}; /* 0x1388 -> 5000 (array payload length) */

    /* prepare first segment */
    memset(send_buffer_segment1, 0, 1412);
    memcpy(&send_buffer_segment1[0], &segment1_header_data[0], sizeof(segment1_header_data));
    offset = sizeof(segment1_header_data);
    memcpy(&send_buffer_segment1[offset], &tp_hader_data_segment1[0], sizeof(tp_hader_data_segment1));
    offset += sizeof(tp_hader_data_segment1);
    memcpy(&send_buffer_segment1[offset], &array_payload_length[0], sizeof(array_payload_length));
    offset += sizeof(array_payload_length);
    for (int idx=offset; idx<segment1_payload_length; idx++) {
        send_buffer_segment1[idx]=0xab;
    }
    /* prepare second segment */
    offset = 0;
    memset(send_buffer_segment2, 0, 1412);
    memcpy(&send_buffer_segment2[0], &segment2_header_data[0], sizeof(segment2_header_data));
    offset = sizeof(segment2_header_data);
    memcpy(&send_buffer_segment2[offset], &tp_hader_data_segment2[0], sizeof(tp_hader_data_segment2));
    offset += sizeof(tp_hader_data_segment2);
    for (int idx=offset; idx<segment2_payload_length; idx++) {
        send_buffer_segment2[idx]=0xab;
    }
    /* prepare third segment */
    offset = 0;
    memset(send_buffer_segment3, 0, 1412);
    memcpy(&send_buffer_segment3[0], &segment3_header_data[0], sizeof(segment3_header_data));
    offset = sizeof(segment3_header_data);
    memcpy(&send_buffer_segment3[offset], &tp_hader_data_segment3[0], sizeof(tp_hader_data_segment3));
    offset += sizeof(tp_hader_data_segment3);
    for (int idx=offset; idx<segment3_payload_length; idx++) {
        send_buffer_segment3[idx]=0xab;
    }
    /* prepare fourth segment */
    offset = 0;
    memset(send_buffer_segment4, 0, 848);
    memcpy(&send_buffer_segment4[0], &segment4_header_data[0], sizeof(segment4_header_data));
    offset = sizeof(segment4_header_data);
    memcpy(&send_buffer_segment4[offset], &tp_hader_data_segment4[0], sizeof(tp_hader_data_segment4));
    offset += sizeof(tp_hader_data_segment4);
    for (int idx=offset; idx<segment4_payload_length; idx++) {
        send_buffer_segment4[idx]=0xab;
    }

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment1, sizeof(send_buffer_segment1)), udp_server_endpoint);
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment2, sizeof(send_buffer_segment2)), udp_server_endpoint);
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment3, sizeof(send_buffer_segment3)), udp_server_endpoint);
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment4, sizeof(send_buffer_segment4)), udp_server_endpoint);

        sleep(5);
        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_TP_Verify_OffsetCalculationDuringReception(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    /* payload data type array
       number of element sent in the array 4000 bytes
       array length field is 4 bytes
       so, actual payload to vsomeip send buffer is 4004 bytes.
    */
    uint8_t send_buffer_segment1[1412] = {0};
    int segment1_payload_length = 1392+4+16;
    uint8_t send_buffer_segment2[1412] = {0};
    int segment2_payload_length = 1392+4+16;
    uint8_t send_buffer_segment3[1240] = {0};
    int segment3_payload_length = 1220+4+16;
    int offset = 0;
    std::cout << "etsProxyImpl::" << __func__ << " remote_addresss:" << remote_address
        << " remote port:" << (uint32_t)remote_port << " local_address:" << local_address
        << " local_port:" << (uint32_t)local_port << std::endl;

    /* someip header data */
    const uint8_t segment1_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x05, 0x7c,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    const uint8_t segment2_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x05, 0x7c,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    const uint8_t segment3_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x04, 0xd0,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    /* TP header (offset(28bit) + reserved(3bit) + more segment(1bit)) */
    const uint8_t tp_hader_data_segment1[] = {0x00, 0x00, 0x00, 0x01};
    const uint8_t tp_hader_data_segment2[] = {0x00, 0x00, 0x00, 0x31};
    const uint8_t tp_hader_data_segment3[] = {0x00, 0x00, 0x0a, 0xe0};
    const uint8_t array_payload_length[] = {0x00, 0x00, 0x0f, 0xa0}; /* 0xfa0 -> 4000 (array payload length) */

    /* prepare first segment */
    memset(send_buffer_segment1, 0, 1412);
    memcpy(&send_buffer_segment1[0], &segment1_header_data[0], sizeof(segment1_header_data));
    offset = sizeof(segment1_header_data);
    memcpy(&send_buffer_segment1[offset], &tp_hader_data_segment1[0], sizeof(tp_hader_data_segment1));
    offset += sizeof(tp_hader_data_segment1);
    memcpy(&send_buffer_segment1[offset], &array_payload_length[0], sizeof(array_payload_length));
    offset += sizeof(array_payload_length);
    for (int idx=offset; idx<segment1_payload_length; idx++) {
        send_buffer_segment1[idx]=0xab;
    }
    /* prepare second segment */
    offset = 0;
    memset(send_buffer_segment2, 0, 1412);
    memcpy(&send_buffer_segment2[0], &segment2_header_data[0], sizeof(segment2_header_data));
    offset = sizeof(segment2_header_data);
    memcpy(&send_buffer_segment2[offset], &tp_hader_data_segment2[0], sizeof(tp_hader_data_segment2));
    offset += sizeof(tp_hader_data_segment2);
    for (int idx=offset; idx<segment2_payload_length; idx++) {
        send_buffer_segment2[idx]=0xab;
    }
    /* prepare third segment */
    offset = 0;
    memset(send_buffer_segment3, 0, 1240);
    memcpy(&send_buffer_segment3[0], &segment3_header_data[0], sizeof(segment3_header_data));
    offset = sizeof(segment3_header_data);
    memcpy(&send_buffer_segment3[offset], &tp_hader_data_segment3[0], sizeof(tp_hader_data_segment3));
    offset += sizeof(tp_hader_data_segment3);
    for (int idx=offset; idx<segment3_payload_length; idx++) {
        send_buffer_segment3[idx]=0xab;
    }

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment1, sizeof(send_buffer_segment1)), udp_server_endpoint);
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment2, sizeof(send_buffer_segment2)), udp_server_endpoint);
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment3, sizeof(send_buffer_segment3)), udp_server_endpoint);

        sleep(5);
        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_TP_Verify_MissingFrameDuringReception(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    /* payload data type array
       number of element sent in the array 4000 bytes
       array length field is 4 bytes
       so, actual payload to vsomeip send buffer is 4004 bytes.
    */
    uint8_t send_buffer_segment1[1412] = {0};
    int segment1_payload_length = 1392+4+16;
    uint8_t send_buffer_segment2[1412] = {0};
    int segment2_payload_length = 1392+4+16;
    uint8_t send_buffer_segment3[1240] = {0};
    int segment3_payload_length = 1220+4+16;
    int offset = 0;
    std::cout << "etsProxyImpl::" << __func__ << " remote_addresss:" << remote_address
        << " remote port:" << (uint32_t)remote_port << " local_address:" << local_address
        << " local_port:" << (uint32_t)local_port << std::endl;

    /* someip header data */
    const uint8_t segment1_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x05, 0x7c,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    const uint8_t segment2_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x05, 0x7c,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    const uint8_t segment3_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x04, 0xd0,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    /* TP header (offset(28bit) + reserved(3bit) + more segment(1bit)) */
    const uint8_t tp_hader_data_segment1[] = {0x00, 0x00, 0x00, 0x01};
    const uint8_t tp_hader_data_segment2[] = {0x00, 0x00, 0x05, 0x71};
    const uint8_t tp_hader_data_segment3[] = {0x00, 0x00, 0x0a, 0xe0};
    const uint8_t array_payload_length[] = {0x00, 0x00, 0x0f, 0xa0}; /* 0xfa0 -> 4000 (array payload length) */

    /* prepare first segment */
    memset(send_buffer_segment1, 0, 1412);
    memcpy(&send_buffer_segment1[0], &segment1_header_data[0], sizeof(segment1_header_data));
    offset = sizeof(segment1_header_data);
    memcpy(&send_buffer_segment1[offset], &tp_hader_data_segment1[0], sizeof(tp_hader_data_segment1));
    offset += sizeof(tp_hader_data_segment1);
    memcpy(&send_buffer_segment1[offset], &array_payload_length[0], sizeof(array_payload_length));
    offset += sizeof(array_payload_length);
    for (int idx=offset; idx<segment1_payload_length; idx++) {
        send_buffer_segment1[idx]=0xab;
    }
    /* prepare second segment */
    offset = 0;
    memset(send_buffer_segment2, 0, 1412);
    memcpy(&send_buffer_segment2[0], &segment2_header_data[0], sizeof(segment2_header_data));
    offset = sizeof(segment2_header_data);
    memcpy(&send_buffer_segment2[offset], &tp_hader_data_segment2[0], sizeof(tp_hader_data_segment2));
    offset += sizeof(tp_hader_data_segment2);
    for (int idx=offset; idx<segment2_payload_length; idx++) {
        send_buffer_segment2[idx]=0xab;
    }
    /* prepare third segment */
    offset = 0;
    memset(send_buffer_segment3, 0, 1240);
    memcpy(&send_buffer_segment3[0], &segment3_header_data[0], sizeof(segment3_header_data));
    offset = sizeof(segment3_header_data);
    memcpy(&send_buffer_segment3[offset], &tp_hader_data_segment3[0], sizeof(tp_hader_data_segment3));
    offset += sizeof(tp_hader_data_segment3);
    for (int idx=offset; idx<segment3_payload_length; idx++) {
        send_buffer_segment3[idx]=0xab;
    }

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment1, sizeof(send_buffer_segment1)), udp_server_endpoint);
        /* Don't send second segment
        *  udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment2, sizeof(send_buffer_segment2)), udp_server_endpoint);
        */
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment3, sizeof(send_buffer_segment3)), udp_server_endpoint);

        sleep(5);
        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_TP_Verify_DuplicateFrameDuringReception(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port) {
    uint8_t recv_buffer[1400] = {0};
    /* payload data type array
       number of element sent in the array 4000 bytes
       array length field is 4 bytes
       so, actual payload to vsomeip send buffer is 4004 bytes.
    */
    uint8_t send_buffer_segment1[1412] = {0};
    int segment1_payload_length = 1392+4+16;
    uint8_t send_buffer_segment2[1412] = {0};
    uint8_t send_duplicate_buffer_segment2[1412] = {0};
    int segment2_payload_length = 1392+4+16;
    uint8_t send_buffer_segment3[1240] = {0};
    int segment3_payload_length = 1220+4+16;
    int offset = 0;
    std::cout << "etsProxyImpl::" << __func__ << " remote_addresss:" << remote_address
        << " remote port:" << (uint32_t)remote_port << " local_address:" << local_address
        << " local_port:" << (uint32_t)local_port << std::endl;

    /* someip header data */
    const uint8_t segment1_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x05, 0x7c,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    const uint8_t segment2_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x05, 0x7c,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    const uint8_t segment3_header_data[] = {
        0x01, 0x01,   	        /* service id */
        0x00, 0x71,   	        /* method id */
        0x00, 0x00, 0x04, 0xd0,	/* length */
        0x55, 0x45,		        /* client id */
        0x00, 0x01,             /* session id */
        0x01, 			        /* protocol version */
        0x01, 			        /* interface version */
        0x20, 			        /* request type */
        0x00			        /* return code */
    };
    /* TP header (offset(28bit) + reserved(3bit) + more segment(1bit)) */
    const uint8_t tp_hader_data_segment1[] = {0x00, 0x00, 0x00, 0x01};
    const uint8_t tp_hader_data_segment2[] = {0x00, 0x00, 0x05, 0x71};
    const uint8_t tp_hader_data_segment3[] = {0x00, 0x00, 0x0a, 0xe0};
    const uint8_t array_payload_length[] = {0x00, 0x00, 0x0f, 0xa0}; /* 0xfa0 -> 4000 (array payload length) */

    /* prepare first segment */
    memset(send_buffer_segment1, 0, 1412);
    memcpy(&send_buffer_segment1[0], &segment1_header_data[0], sizeof(segment1_header_data));
    offset = sizeof(segment1_header_data);
    memcpy(&send_buffer_segment1[offset], &tp_hader_data_segment1[0], sizeof(tp_hader_data_segment1));
    offset += sizeof(tp_hader_data_segment1);
    memcpy(&send_buffer_segment1[offset], &array_payload_length[0], sizeof(array_payload_length));
    offset += sizeof(array_payload_length);
    for (int idx=offset; idx<segment1_payload_length; idx++) {
        send_buffer_segment1[idx]=0xab;
    }
    /* prepare second segment */
    offset = 0;
    memset(send_buffer_segment2, 0, 1412);
    memset(send_duplicate_buffer_segment2, 0, 1412);
    memcpy(&send_buffer_segment2[0], &segment2_header_data[0], sizeof(segment2_header_data));
    memcpy(&send_duplicate_buffer_segment2[0], &segment2_header_data[0], sizeof(segment2_header_data));
    offset = sizeof(segment2_header_data);
    memcpy(&send_buffer_segment2[offset], &tp_hader_data_segment2[0], sizeof(tp_hader_data_segment2));
    memcpy(&send_duplicate_buffer_segment2[offset], &tp_hader_data_segment2[0], sizeof(tp_hader_data_segment2));
    offset += sizeof(tp_hader_data_segment2);
    for (int idx=offset; idx<segment2_payload_length; idx++) {
        send_buffer_segment2[idx]=0xab;
        /* duplicate frame with different payload */
        send_duplicate_buffer_segment2[idx]=0xaa;
    }
    /* prepare third segment */
    offset = 0;
    memset(send_buffer_segment3, 0, 1240);
    memcpy(&send_buffer_segment3[0], &segment3_header_data[0], sizeof(segment3_header_data));
    offset = sizeof(segment3_header_data);
    memcpy(&send_buffer_segment3[offset], &tp_hader_data_segment3[0], sizeof(tp_hader_data_segment3));
    offset += sizeof(tp_hader_data_segment3);
    for (int idx=offset; idx<segment3_payload_length; idx++) {
        send_buffer_segment3[idx]=0xab;
    }

    if (isAvailable) {
        boost::asio::io_service io_ctx;
        boost::system::error_code err_code;
        boost::asio::ip::udp::socket::endpoint_type udp_client_endpoint(boost::asio::ip::address::from_string(local_address), local_port);
        boost::asio::ip::udp::socket::endpoint_type udp_server_endpoint(boost::asio::ip::address::from_string(remote_address), remote_port);
        boost::asio::ip::udp::socket udp_client_sock(io_ctx, udp_client_endpoint);
        boost::asio::ip::udp::socket::endpoint_type remote_endpoint;

        io_ctx.run();

        std::cout << "etsProxyImpl::" << __func__ << " Sending data..." << std::endl;
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment1, sizeof(send_buffer_segment1)), udp_server_endpoint);
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment2, sizeof(send_buffer_segment2)), udp_server_endpoint);
        /* send duplicate frame */
        udp_client_sock.send_to(boost::asio::buffer(send_duplicate_buffer_segment2, sizeof(send_duplicate_buffer_segment2)), udp_server_endpoint);
        udp_client_sock.send_to(boost::asio::buffer(send_buffer_segment3, sizeof(send_buffer_segment3)), udp_server_endpoint);

        udp_client_sock.receive_from(boost::asio::buffer(recv_buffer, 1400), remote_endpoint);
        std::cout << "etsProxyImpl::" << __func__ << " Received response[header]:" << std::endl;
        for (int idx=0; idx<16; idx++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)recv_buffer[idx] << " ";
            if ((idx+1)%4 == 0) {
                std::cout << '\n';
            }
        }
        std::cout << "etsProxyImpl::" << __func__ << " Return Code:"
             << returnCodeToString(static_cast<vsomeip::return_code_e>(recv_buffer[15])) << std::endl;

        io_ctx.stop();

        udp_client_sock.shutdown(boost::asio::socket_base::shutdown_both, err_code);
        udp_client_sock.close(err_code);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_ResetInterface_wrong_Fire_and_forget_package_get_No_Error_back() {
    vsomeip::service_t service_id = 0x101;
    vsomeip::instance_t instance_id = 0x1;
    vsomeip::method_t method_id = 0x1;
    uint32_t value = 15;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    std::shared_ptr<vsomeip::application> _app = vsomeip::runtime::get()->get_application("ets_default_client");
    if (_app) {
        _app->register_message_handler(service_id, instance_id, method_id, [&](const std::shared_ptr<vsomeip::message> &_response)
        {
            vsomeip::message_type_e msgType = _response->get_message_type();
            vsomeip::service_t serviceId = _response->get_service();
            vsomeip::instance_t instanceId = _response->get_instance();
            vsomeip::method_t methodId = _response->get_method();
            vsomeip::major_version_t majorVersion = _response->get_interface_version();
            vsomeip::return_code_e returnCode = _response->get_return_code();

            std::cout << "etsProxyImpl::on_message:[" << std::hex << (uint32_t)serviceId << "."
                << std::hex << (uint32_t)instanceId << "." << std::hex << (uint32_t)methodId << "."
                << std::hex << (uint32_t)majorVersion << "] Session:" << _response->get_session()
                << " Type:" << (uint32_t)msgType << std::endl;

            if (vsomeip::return_code_e::E_OK == returnCode) {
                std::shared_ptr<vsomeip::payload> payload = _response->get_payload();
                vsomeip::length_t payload_length = _response->get_payload()->get_length();
                vsomeip::byte_t* payload_data = _response->get_payload()->get_data();
                std::cout << "Received[" << (uint32_t)payload_length << "bytes]:" << std::endl;
                for (uint32_t idx=0; idx<payload_length; idx++) {
                    std::cout << std::hex << (uint32_t)payload_data[idx];
                }
                std::cout << "Done" << std::endl;
            }
            std::cout << "Return code:" << std::hex << (uint32_t)returnCode << ":" << returnCodeToString(returnCode) << std::endl;
            {
                std::unique_lock<std::mutex> lk(mtx);
                this->method_response_received = true;
                cond.notify_one();
            }
        });

        if (isAvailable) {
            method_response_received = false;
            std::shared_ptr<vsomeip::message> _request = vsomeip::runtime::get()->create_request();
            _request->set_service(service_id);
            _request->set_instance(instance_id);
            _request->set_method(method_id);
            _request->set_interface_version(0x15);
            _request->set_reliable(false);
            _request->set_message_type(vsomeip::message_type_e::MT_REQUEST_NO_RETURN);
            std::shared_ptr<vsomeip::payload> _payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> _payload_data;
            _request->set_payload(_payload);

            tester_get_TestFieldUINT8();
            sleep(1);
            tester_set_TestFieldUINT8(value);
            sleep(1);

            std::cout << "etsProxyImpl::" << __func__ << " Sending payload" << std::endl;
            _app->send(_request);
            {
                std::unique_lock<std::mutex> lk(mtx);
                cond.wait_for(lk, std::chrono::seconds(5), [&]{
                    return this->method_response_received;
                });
            }

            sleep(1);
            tester_get_TestFieldUINT8();
        }
        else {
            std::cout << "ETS Service Not Available" << std::endl;
        }

        _app->unregister_message_handler(service_id, instance_id, method_id);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " ets_default_client app not present" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_subscribe_TestFieldUINT8() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        TestFieldUINT8subscription = etsProxy->getTestFieldUINT8Attribute().getChangedEvent().subscribe([&](const uint8_t &TestFieldUINT8) {
            std::cout << "etsProxyImpl::tester_subscribe_TestFieldUINT8 TestFieldUINT8:" << (uint32_t)TestFieldUINT8 << std::endl;
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_set_TestFieldUINT8(uint32_t value) {
    uint8_t TestFieldUINT8 = 0;
    uint8_t TestFieldUINT8Resp = 0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        TestFieldUINT8 = (uint8_t)value;
        etsProxy->getTestFieldUINT8Attribute().setValue(TestFieldUINT8, callStatus, TestFieldUINT8Resp);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " TestFieldUINT8Resp:" << (uint32_t)TestFieldUINT8Resp << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;    
}

void etsProxyImpl::tester_get_TestFieldUINT8() {
    uint8_t TestFieldUINT8 = 0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestFieldUINT8Attribute().getValue(callStatus, TestFieldUINT8);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " TestFieldUINT8:" << (uint32_t)TestFieldUINT8 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_unsubscribe_TestFieldUINT8() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestFieldUINT8Attribute().getChangedEvent().unsubscribe(TestFieldUINT8subscription);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_subscribe_TestFieldUINT8Array() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        TestFieldUINT8Arraysubscription = etsProxy->getTestFieldUINT8ArrayAttribute().getChangedEvent().subscribe([&](const std::vector< uint8_t > &TestFieldUINT8Array) {
            std::cout << "etsProxyImpl::tester_subscribe_TestFieldUINT8Array:" << std::endl;
            for (auto &it : TestFieldUINT8Array) {
                std::cout << it << " ";
            }
            std::cout << std::endl;
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_set_TestFieldUINT8Array() {
    std::vector< uint8_t > TestFieldUINT8Array;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::vector< uint8_t > TestFieldUINT8ArrayResp;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    for (uint32_t idx=0; idx<5; idx++) {
        TestFieldUINT8Array.push_back(0xab);
    }

    if (etsProxy && isAvailable) {
        etsProxy->getTestFieldUINT8ArrayAttribute().setValue(TestFieldUINT8Array, callStatus, TestFieldUINT8ArrayResp);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " TestFieldUINT8ArrayResp:" << std::endl;
            for (auto &it : TestFieldUINT8ArrayResp) {
                std::cout << it << " ";
            }
            std::cout << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_get_TestFieldUINT8Array() {
    std::vector< uint8_t > TestFieldUINT8Array;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestFieldUINT8ArrayAttribute().getValue(callStatus, TestFieldUINT8Array);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " TestFieldUINT8Array:" << std::endl;
            for (auto &it : TestFieldUINT8Array) {
                std::cout << it << " ";
            }
            std::cout << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_unsubscribe_TestFieldUINT8Array() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestFieldUINT8ArrayAttribute().getChangedEvent().unsubscribe(TestFieldUINT8Arraysubscription);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_subscribe_TestFieldUINT8Reliable() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        TestFieldUINT8Reliablesubscription = etsProxy->getTestFieldUINT8ReliableAttribute().getChangedEvent().subscribe([&](const uint8_t &TestFieldUINT8Reliable) {
            std::cout << "etsProxyImpl::tester_subscribe_TestFieldUINT8Reliable TestFieldUINT8Reliable:" << TestFieldUINT8Reliable << std::endl;
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_set_TestFieldUINT8Reliable(uint32_t value) {
    uint8_t TestFieldUINT8Reliable = 0;
    uint8_t TestFieldUINT8ReliableResp = 0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    TestFieldUINT8Reliable = (uint8_t)value;
    if (etsProxy && isAvailable) {
        etsProxy->getTestFieldUINT8ReliableAttribute().setValue(TestFieldUINT8Reliable, callStatus, TestFieldUINT8ReliableResp);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " TestFieldUINT8ReliableResp:" << (uint32_t)TestFieldUINT8ReliableResp << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_get_TestFieldUINT8Reliable() {
    uint8_t TestFieldUINT8Reliable = 0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestFieldUINT8ReliableAttribute().getValue(callStatus, TestFieldUINT8Reliable);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " TestFieldUINT8Reliable:" << (uint32_t)TestFieldUINT8Reliable << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_unsubscribe_TestFieldUINT8Reliable() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestFieldUINT8ReliableAttribute().getChangedEvent().unsubscribe(TestFieldUINT8Reliablesubscription);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_subscribe_ETSInterfaceVersion() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        InterfaceVersionsubscription = etsProxy->getETSInterfaceVersionAttribute().getChangedEvent().subscribe([&](const ETS::VersionType &ETSInterfaceVersion) {
            std::cout << "etsProxyImpl::tester_subscribe_ETSInterfaceVersion ETSInterfaceVersion["
                << (uint32_t)ETSInterfaceVersion.getMajorVersion() << "." << ETSInterfaceVersion.getMinorVersion() << "]" << std::endl;
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_get_ETSInterfaceVersion() {
    ETS::VersionType ETSInterfaceVersion = {};
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getETSInterfaceVersionAttribute().getValue(callStatus, ETSInterfaceVersion);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << "ETSInterfaceVersion["
                << (uint32_t)ETSInterfaceVersion.getMajorVersion() << "." << ETSInterfaceVersion.getMinorVersion() << "]" << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_unsubscribe_ETSInterfaceVersion() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getETSInterfaceVersionAttribute().getChangedEvent().unsubscribe(InterfaceVersionsubscription);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::invoke_echoUINT8RELIABLE() {
    uint8_t echoUINT8RELIABLE_ReqArg1 = 18;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint8_t echoUINT8RELIABLE_ResArg1=0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->echoUINT8RELIABLE(echoUINT8RELIABLE_ReqArg1, callStatus, echoUINT8RELIABLE_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " echoUINT8RELIABLE_ResArg1:"
                << std::hex << (uint32_t)echoUINT8RELIABLE_ResArg1 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_subscribe_TestEventUINT8Reliable() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        uINT8ValuesubscriptionReliable = etsProxy->getTestEventUINT8ReliableEvent().subscribe([&](const uint8_t& uINT8Value) {
            std::cout << "etsProxyImpl::tester_subscribe_TestEventUINT8Reliable TestEventUINT8Reliable Notification:" << std::hex << (uint32_t)uINT8Value << std::endl;
        });
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::tester_unsubscribe_TestEventUINT8Reliable() {
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->getTestEventUINT8ReliableEvent().unsubscribe(uINT8ValuesubscriptionReliable);
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }
}

void etsProxyImpl::invoke_triggerEventUINT8Reliable(uint32_t start, uint32_t duration, uint32_t debounce) {
    uint32_t triggerEventUINT8Reliable_ReqArg1=0;
    uint32_t triggerEventUINT8Reliable_ReqArg2=0;
    uint32_t triggerEventUINT8Reliable_ReqArg3=0;
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    triggerEventUINT8Reliable_ReqArg1 = start;
    triggerEventUINT8Reliable_ReqArg2 = duration;
    triggerEventUINT8Reliable_ReqArg3 = debounce;
    if (etsProxy && isAvailable) {
        etsProxy->triggerEventUINT8Reliable(triggerEventUINT8Reliable_ReqArg1, triggerEventUINT8Reliable_ReqArg2, triggerEventUINT8Reliable_ReqArg3, callStatus);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}

void etsProxyImpl::tester_send_reliable_event(int eventval) {
    uint8_t uINT8Value = 0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (secondaryServiceActive) {
        uINT8Value = (uint8_t)eventval;
        std::cout << "etsProxyImpl::" << __func__ << " value:" << std::hex << (uint32_t)uINT8Value << std::endl;
        secService->fireSecondaryEventUINT8ReliableEvent(uINT8Value);
    }
    else {
        std::cout << "etsProxyImpl::" << __func__ << " Service not yet offered" << std::endl;
    }
}

void etsProxyImpl::invoke_clientServiceGetLastValueOfEventTCP() {
    CommonAPI::CallStatus callStatus=CommonAPI::CallStatus::UNKNOWN;
    uint8_t clientServiceGetLastValueOfEventTCP_ResArg1=0;
    std::cout << "etsProxyImpl::" << __func__ << std::endl;

    if (etsProxy && isAvailable) {
        etsProxy->clientServiceGetLastValueOfEventTCP(callStatus, clientServiceGetLastValueOfEventTCP_ResArg1);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "etsProxyImpl::" << __func__ << " Failed status:" << callstatusToString(callStatus) << std::endl;
        }
        else {
            std::cout << "etsProxyImpl::" << __func__ << " Last TCP Event Value:" << std::hex << (uint32_t)clientServiceGetLastValueOfEventTCP_ResArg1 << std::endl;
        }
    }
    else {
        std::cout << "ETS Service Not Available" << std::endl;
    }

    return;
}