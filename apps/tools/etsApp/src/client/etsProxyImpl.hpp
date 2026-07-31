/* 
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __ETS_PROXYIMPL_HPP__
#define __ETS_PROXYIMPL_HPP__

#include <vsomeip/vsomeip.hpp>
#include <CommonAPI/CommonAPI.hpp>
#include <v1/someip/testability/ETSProxy.hpp>
#include <v1/someip/testability/ETSSecondaryServiceStubDefault.hpp>
#include <v1/someip/testability/ETSTestService1Proxy.hpp>
#include <v1/someip/testability/ETSTestService2Proxy.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <boost/asio.hpp>
#include <iomanip>
#include <string>
#include <map>

using namespace ::v1::someip::testability;

class secServiceStubImpl: public ETSSecondaryServiceStubDefault {
};

class etsProxyImpl {
    private:
        std::shared_ptr<ETSProxy<>> etsProxy;
        bool isAvailable;
        std::shared_ptr<ETSTestService1Proxy<>> testSvcProxy1;
        std::shared_ptr<ETSTestService1Proxy<>> testSvcProxy2;
        std::shared_ptr<ETSTestService2Proxy<>> testSvcProxy3;
        std::mutex mtx;
        std::condition_variable cond;
        bool method_response_received;
        std::vector<std::thread> appThreadPool;
        std::shared_ptr<secServiceStubImpl> secService;
        bool secondaryServiceActive;
        uint32_t startTimeout;
        uint32_t stopTimeout;
        uint32_t durationTimeout;
        uint32_t uINT8Valuesubscription;
        uint32_t uINT8E2Esubscription;
        uint32_t uINT8Multicastsubscription;
        uint32_t uINT8Arraysubscription;
        uint32_t TestEventUINT8TPsubscription;
        uint32_t TestEventUINT32Periodicsubscription;
        uint32_t TestEventUINT32UpdateOnChangesubscription;
        uint32_t InterfaceVersionsubscription;
        uint32_t TestFieldUINT8subscription;
        uint32_t TestFieldUINT8Arraysubscription;
        uint32_t TestFieldUINT8Reliablesubscription;
        uint32_t uINT8ValuesubscriptionReliable;
    public:
        etsProxyImpl();
        ~etsProxyImpl();
        std::string callstatusToString(CommonAPI::CallStatus status);
        std::string returnCodeToString(vsomeip::return_code_e return_code);
        void createProxy(std::string appname);
        void tester_send_offer();
        void tester_send_stop_offer(int without_offer);
        void tester_send_unicast_event(int eventval);
        void tester_send_multicast_event(int eventval);
        void tester_send_session_reset();
        void tester_update_service_unreliable_port();
        void tester_get_interface_version(int service_id, int instance_id);
        void tester_subscribe_TestEventUINT8();
        void tester_unsubscribe_TestEventUINT8();
        void tester_subscribe_TestEventUINT8Array();
        void tester_unsubscribe_TestEventUINT8Array();
        void tester_subscribe_TestEventUINT8Multicast();
        void tester_unsubscribe_TestEventUINT8Multicast();
        void tester_subscribe_TestEventUINT8E2E();
        void tester_unsubscribe_TestEventUINT8E2E();
        void tester_subscribe_TestEventUINT8TP();
        void tester_unsubscribe_TestEventUINT8TP();
        void tester_subscribe_TestEventUINT32PeriodicEvent();
        void tester_unsubscribe_TestEventUINT32PeriodicEvent();
        void tester_subscribe_TestEventUINT32UpdateOnChangeEvent();
        void tester_unsubscribe_TestEventUINT32UpdateOnChangeEvent();
        void tester_requestTestService(uint32_t service_id, uint32_t instance_id);
        void invoke_requestTestServiceMethod(uint32_t service_id, uint32_t instance_id);
        void invoke_checkByteOrder();
        void invoke_clientServiceActivate(uint32_t start_timeout);
        void invoke_clientServiceDeactivate(int stop_timeout);
        void invoke_clientServiceGetLastValueOfEventUDPMulticast();
        void invoke_clientServiceGetLastValueOfEventUDPUnicast();
        void invoke_clientServiceSubscribeEventgroup(uint32_t start_timeout, uint32_t subscription_duration);
        void invoke_echoBitfields();
        void invoke_echoCommonDatatypes();
        void invoke_echoENUM();
        void invoke_echoFLOAT64();
        void invoke_echoINT8();
        void invoke_echoStaticUINT8Array();
        void invoke_echoUINT8();
        void invoke_echoUINT8Array();
        void invoke_echoUINT8Array16BitLength();
        void invoke_echoUINT8Array2Dim();
        void invoke_echoUINT8Array8BitLength();
        void invoke_echoUINT8ArrayMinSize();
        void invoke_echoUINT8ArrayMinSize_too_short(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_echoUTF16DYNAMIC();
        void invoke_echoUTF16FIXED();
        void invoke_echoUTF8DYNAMIC();
        void invoke_echoUTF8FIXED();
        void invoke_resetInterface();
        void invoke_suspendInterface(uint32_t start, uint32_t duration);
        void invoke_triggerEventUINT8(uint32_t start, uint32_t duration, uint32_t debounce);
        void invoke_triggerEventUINT8Array(uint32_t start, uint32_t duration, uint32_t debounce);
        void invoke_triggerEventUINT8Multicast(uint32_t start, uint32_t duration, uint32_t debounce);
        void invoke_echoUINT8ArrayLengthTP(int array_length);
        void invoke_echoUINT8ArrayLengthInTP(int array_length);
        void invoke_echoUINT8ArrayLengthOutTP(int array_length);
        void invoke_triggerEventUINT8ArrayTP(int array_length);
        void invoke_echoUINT8ArrayLengthTPNoResponse(int array_length);
        void invoke_triggerEventUINT8ArrayTPNoReqTPPayload(int array_length);
        void invoke_triggerEventUINT8E2E(uint32_t start, uint32_t duration, uint32_t debounce);
        void invoke_echoUINT8E2E();
        void invoke_triggerEventUINT32Periodic(uint32_t event_value);
        void invoke_triggerEventUINT32UpdateOnChange(uint32_t event_value);
        void invoke_array_length_longer_as_message_length_allows_it();
        void invoke_array_length_too_long();
        void invoke_array_length_too_short_strips_payload();
        void invoke_burst_test(int no_of_iteration); 
        void invoke_echoUTF16DYNAMIC_length_too_long_for_string();
        void invoke_echoUTF16DYNAMIC_length_too_short_for_malformed_string();
        void invoke_echoUTF16DYNAMIC_length_too_short_for_string();
        void invoke_echoUTF16DYNAMIC_odd_number_before_termination();
        void invoke_echoUTF16DYNAMIC_with_odd_number_after_termination();
        void invoke_echoUTF16DYNAMIC_wrong_BOM();
        void invoke_echoUTF8DYNAMIC_length_too_long_for_string();
        void invoke_echoUTF8DYNAMIC_length_too_short_for_malformed_string();
        void invoke_echoUTF8DYNAMIC_length_too_short_for_string();
        void invoke_echoUTF8DYNAMIC_wrong_BOM();
        void invoke_echoUTF16FIXED_with_odd_number();
        void invoke_string_UTF16FIXED_too_long();
        void invoke_string_UTF16FIXED_too_short();
        void invoke_string_UTF8FIXED_too_long();
        void invoke_string_UTF8FIXED_too_short();
        void invoke_Wrong_Interface_Version();
        void invoke_Wrong_Message_Type();
        void invoke_Wrong_Method_ID();
        void invoke_Fire_And_Forget_Wrong_Method_ID();
        void invoke_Wrong_Service_ID(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_Wrong_SOMEIP_Protocol_Version(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_Wrong_Return_Code(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_Length_equals_0_Test(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_Length_smaller_than_8_Test(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_Length_way_too_long(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Discover_Port_and_IP();
        void invoke_Sending_two_SOMEIP_Messages_in_a_row(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_UINT8Array_with_Length_0_strips_Payload();
        void invoke_Unaligned_SOMEIP_Messages_overUDP(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Answer_multiple_subscribes_together(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Check_Reaction_to_a_Subscribe_with_ttl_0(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Consider_Entries_Order(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Do_not_specify_a_port(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Do_not_specify_IPv4_Adress(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Empty_Entries_Array(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Empty_Option(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Empty_Options_Array(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Entries_Length_wrong_combined(std::string remote6_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Options_Array_too_short(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Request_non_existing_EventgroupID(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Request_non_existing_InstanceID(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Request_non_existing_Major_Version(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Request_non_existing_ServiceID(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Reserved_Field_Endpoint_Option_set(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_SOMEIP_Length_shorter_as_expected(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Specify_an_unexisting_IPv4_Address(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Subscribe_after_StopSubscribe(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_SubscribeEventgroup_with_unallowed_option_ip(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_SubscribeEventgroup_with_unallowed_option_ip_2(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Unknown_Option_type(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Unreferenced_option(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Unused_data_after_Options_Array(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Unused_data_after_Options_Array_wrong_length(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_Subscribe_using_wrong_SOMEIP_MessageID(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_Eventgroup_EventsAndFieldsUnreliable_5(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Calling_same_ports_before_and_after_suspendInterface(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Check_Reboot_Detection_separate_multicast_and_unicast(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Check_Reboot_Detection_Server_Side(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Check_subscribe_eventgroup_ttl_expired(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Deregister_from_Eventgroup(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_ResetInterface();
        void invoke_SD_SuspendInterface();
        void invoke_SD_Send_triggerEventUINT8_Eventgroup_2(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Send_triggerEventUINT8Array_Eventgroup_2(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Send_triggerEventUINT8E2E_Eventgroup_2(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Send_triggerEventUINT8Multicast_Eventgroup_6(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_SD_Interface_Version(std::string remote_address, uint16_t multicast_port, std::string local_address, uint16_t local_port);
        void invoke_activateTestSerivce(uint32_t service_id, uint32_t instance_id);
        void invoke_deactivateTestSerivce(uint32_t service_id, uint32_t instance_id);
        void invoke_TP_Verify_ErrorDuringReception(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_TP_Verify_ReceptionBufferManagement(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_TP_Verify_OffsetCalculationDuringReception(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_TP_Verify_MissingFrameDuringReception(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_TP_Verify_DuplicateFrameDuringReception(std::string remote_address, uint16_t remote_port, std::string local_address, uint16_t local_port);
        void invoke_ResetInterface_wrong_Fire_and_forget_package_get_No_Error_back();
        void tester_subscribe_TestFieldUINT8();
        void tester_set_TestFieldUINT8(uint32_t value);
        void tester_get_TestFieldUINT8();
        void tester_unsubscribe_TestFieldUINT8();
        void tester_subscribe_TestFieldUINT8Array();
        void tester_set_TestFieldUINT8Array();
        void tester_get_TestFieldUINT8Array();
        void tester_unsubscribe_TestFieldUINT8Array();
        void tester_subscribe_TestFieldUINT8Reliable();
        void tester_set_TestFieldUINT8Reliable(uint32_t value);
        void tester_get_TestFieldUINT8Reliable();
        void tester_unsubscribe_TestFieldUINT8Reliable();
        void tester_subscribe_ETSInterfaceVersion();
        void tester_get_ETSInterfaceVersion();
        void tester_unsubscribe_ETSInterfaceVersion();
        void invoke_echoUINT8RELIABLE();
        void tester_subscribe_TestEventUINT8Reliable();
        void tester_unsubscribe_TestEventUINT8Reliable();
        void invoke_triggerEventUINT8Reliable(uint32_t start, uint32_t duration, uint32_t debounce);
        void tester_send_reliable_event(int eventval);
        void invoke_clientServiceGetLastValueOfEventTCP();
};

#endif