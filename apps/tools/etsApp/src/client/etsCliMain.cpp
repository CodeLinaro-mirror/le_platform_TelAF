/* 
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "etsProxyImpl.hpp"

int main(int argc, char *argv[]) {
    int indx = 0;
    uint32_t startTimeout = 0;
    uint32_t stopTimeout = 0;
    uint32_t subscriptionDuration = 0;
    int withoutOffer = 0;
    int service_id  = 0;
    int instance_id = 0;
    int array_length = 0;
    uint32_t start = 0;
    uint32_t duration = 0;
    uint32_t debounce = 0;
    int eventval = 0;
    int meventval = 0;
    int iteration = 0;
    std::string remote_unicast_ipaddr;
    int remote_udp_port = 0;
    std::string local_unicast_ipaddr;
    int local_udp_port = 0;
    std::string multicast_ipaddr;
    int multicast_port = 0;
    uint32_t fieldval = 0;

    if (argc != 2) {
        std::cout << "help:" << std::endl;
        std::cout << "For default client run: someipETSClient ets_default_client" << std::endl;
        std::cout << "For duplicate client run: someipETSClient ets_duplicate_client" << std::endl;
        return 0;
    }

    std::cout << __func__ << " Starting ETS Client App " << argv[1] << std::endl;
    std::shared_ptr<etsProxyImpl> proxyPtr = std::make_shared<etsProxyImpl>();
    if (!proxyPtr) {
        std::cerr << __func__ << "proxyPtr (nullptr)... Exit" << std::endl;
        return 0;
    }

    proxyPtr->createProxy(argv[1]);

    while(true) {
        std::cout << "Case 0: invoke_all_test_cases" << std::endl;
        std::cout << "Case 1: invoke_checkByteOrder" << std::endl;
        std::cout << "Case 2: invoke_clientServiceActivate" << std::endl;
        std::cout << "Case 3: tester send Offer Service" << std::endl;
        std::cout << "Case 4: tester send Stop Offer Service" << std::endl;
        std::cout << "Case 5: invoke_clientServiceSubscribeEventgroup" << std::endl;
        std::cout << "Case 6: tester send unicast event" << std::endl;
        std::cout << "Case 7: invoke_clientServiceGetLastValueOfEventUDPUnicast" << std::endl;
        std::cout << "Case 8: tester send multicast event" << std::endl;
        std::cout << "Case 9: invoke_clientServiceGetLastValueOfEventUDPMulticast" << std::endl;
        std::cout << "Case 10: invoke_clientServiceDeactivate" << std::endl;
        std::cout << "Case 11: tester_send_session_reset" << std::endl;
        std::cout << "Case 12: tester_update_service_unreliable_port" << std::endl;
        std::cout << "Case 13: tester_get_interface_version" << std::endl;
        std::cout << "Case 14: invoke_echoBitfields" << std::endl;
        std::cout << "Case 15: invoke_echoCommonDatatypes" << std::endl;
        std::cout << "Case 16: invoke_echoUINT8ArrayLengthTP" << std::endl;
        std::cout << "Case 17: tester_subscribe_TestEventUINT8TP" << std::endl;
        std::cout << "Case 18: invoke_triggerEventUINT8ArrayTP" << std::endl;
        std::cout << "Case 19: tester_unsubscribe_TestEventUINT8TP" << std::endl;
        std::cout << "Case 20: invoke_echoFLOAT64" << std::endl;
        std::cout << "Case 21: invoke_echoINT8" << std::endl;
        std::cout << "Case 22: invoke_echoStaticUINT8Array" << std::endl;
        std::cout << "Case 23: invoke_echoUINT8" << std::endl;
        std::cout << "Case 24: invoke_echoUINT8Array" << std::endl;
        std::cout << "Case 25: invoke_echoUINT8Array16BitLength" << std::endl;
        std::cout << "Case 26: invoke_echoUINT8Array2Dim" << std::endl;
        std::cout << "Case 27: invoke_echoUINT8Array8BitLength" << std::endl;
        std::cout << "Case 28: invoke_echoUINT8ArrayMinSize" << std::endl;
        std::cout << "Case 29: invoke_echoUTF16DYNAMIC" << std::endl;
        std::cout << "Case 30: invoke_echoUTF16FIXED" << std::endl;
        std::cout << "Case 31: invoke_echoUTF8DYNAMIC" << std::endl;
        std::cout << "Case 32: invoke_echoUTF8FIXED" << std::endl;
        std::cout << "Case 33: tester_subscribe_TestEventUINT8" << std::endl;
        std::cout << "Case 34: invoke_triggerEventUINT8" << std::endl;
        std::cout << "Case 35: tester_unsubscribe_TestEventUINT8" << std::endl;
        std::cout << "Case 36: tester_subscribe_TestEventUINT8Array" << std::endl;
        std::cout << "Case 37: invoke_triggerEventUINT8Array" << std::endl;
        std::cout << "Case 38: tester_unsubscribe_TestEventUINT8Array" << std::endl;
        std::cout << "Case 39: tester_subscribe_TestEventUINT8Multicast" << std::endl;
        std::cout << "Case 40: invoke_triggerEventUINT8Multicast" << std::endl;
        std::cout << "Case 41: tester_unsubscribe_TestEventUINT8Multicast" << std::endl;
        std::cout << "Case 42: tester_subscribe_TestEventUINT8E2E" << std::endl;
        std::cout << "Case 43: invoke_triggerEventUINT8E2E" << std::endl;
        std::cout << "Case 44: tester_unsubscribe_TestEventUINT8E2E" << std::endl;
        std::cout << "Case 45: invoke_echoUINT8E2E" << std::endl;
        std::cout << "Case 46: tester_subscribe_TestEventUINT32PeriodicEvent" << std::endl;
        std::cout << "Case 47: invoke_triggerEventUINT32Periodic" << std::endl;
        std::cout << "Case 48: tester_unsubscribe_TestEventUINT32PeriodicEvent" << std::endl;
        std::cout << "Case 49: tester_subscribe_TestEventUINT32UpdateOnChangeEvent" << std::endl;
        std::cout << "Case 50: invoke_triggerEventUINT32UpdateOnChange" << std::endl;
        std::cout << "Case 51: tester_unsubscribe_TestEventUINT32UpdateOnChangeEvent" << std::endl;
        std::cout << "Case 52: invoke_array_length_longer_as_message_length_allows_it" << std::endl;
        std::cout << "Case 53: invoke_array_length_too_long" << std::endl;
        std::cout << "Case 54: invoke_array_length_too_short_strips_payload" << std::endl;
        std::cout << "Case 55: invoke_burst_test" << std::endl;
        std::cout << "Case 56: invoke_echoUTF16DYNAMIC_length_too_long_for_string" << std::endl;
        std::cout << "Case 57: invoke_echoUTF16DYNAMIC_length_too_short_for_malformed_string" << std::endl;
        std::cout << "Case 58: invoke_echoUTF16DYNAMIC_length_too_short_for_string" << std::endl;
        std::cout << "Case 59: invoke_echoUTF16DYNAMIC_odd_number_before_termination" << std::endl;
        std::cout << "Case 60: invoke_echoUTF16DYNAMIC_with_odd_number_after_termination" << std::endl;
        std::cout << "Case 61: invoke_echoUTF16DYNAMIC_wrong_BOM" << std::endl;
        std::cout << "Case 62: invoke_echoUTF8DYNAMIC_length_too_long_for_string" << std::endl;
        std::cout << "Case 63: invoke_echoUTF8DYNAMIC_length_too_short_for_malformed_string" << std::endl;
        std::cout << "Case 64: invoke_echoUTF8DYNAMIC_length_too_short_for_string" << std::endl;
        std::cout << "Case 65: invoke_echoUTF8DYNAMIC_wrong_BOM" << std::endl;
        std::cout << "Case 66: invoke_echoUTF16FIXED_with_odd_number" << std::endl;
        std::cout << "Case 67: invoke_string_UTF16FIXED_too_long" << std::endl;
        std::cout << "Case 68: invoke_string_UTF16FIXED_too_short" << std::endl;
        std::cout << "Case 69: invoke_string_UTF8FIXED_too_long" << std::endl;
        std::cout << "Case 70: invoke_string_UTF8FIXED_too_short" << std::endl;
        std::cout << "Case 71: invoke_Wrong_Interface_Version" << std::endl;
        std::cout << "Case 72: invoke_Wrong_Message_Type" << std::endl;
        std::cout << "Case 73: invoke_Wrong_Method_ID" << std::endl;
        std::cout << "Case 74: invoke_Wrong_Service_ID" << std::endl;
        std::cout << "Case 75: invoke_Wrong_SOMEIP_Protocol_Version" << std::endl;
        std::cout << "Case 76: invoke_Length_equals_0_Test" << std::endl;
        std::cout << "Case 77: invoke_Length_smaller_than_8_Test" << std::endl;
        std::cout << "Case 78: invoke_Length_way_too_long" << std::endl;
        std::cout << "Case 79: invoke_SD_Discover_Port_and_IP" << std::endl;
        std::cout << "Case 80: invoke_Sending_two_SOMEIP_Messages_in_a_row" << std::endl;
        std::cout << "Case 81: invoke_UINT8Array_with_Length_0_strips_Payload" << std::endl;
        std::cout << "Case 82: invoke_Unaligned_SOMEIP_Messages_overUDP" << std::endl;
        std::cout << "Case 83: invoke_SD_Answer_multiple_subscribes_together" << std::endl;
        std::cout << "Case 84: invoke_SD_Check_Reaction_to_a_Subscribe_with_ttl_0" << std::endl;
        std::cout << "Case 85: invoke_SD_Consider_Entries_Order" << std::endl;
        std::cout << "Case 86: invoke_SD_Do_not_specify_a_port" << std::endl;
        std::cout << "Case 87: invoke_SD_Do_not_specify_IPv4_Adress" << std::endl;
        std::cout << "Case 88: invoke_SD_Empty_Entries_Array" << std::endl;
        std::cout << "Case 89: invoke_SD_Empty_Option" << std::endl;
        std::cout << "Case 90: invoke_SD_Empty_Options_Array" << std::endl;
        std::cout << "Case 91: invoke_SD_Entries_Length_wrong_combined" << std::endl;
        std::cout << "Case 92: invoke_SD_Options_Array_too_short" << std::endl;
        std::cout << "Case 93: invoke_SD_Request_non_existing_EventgroupID" << std::endl;
        std::cout << "Case 94: invoke_SD_Request_non_existing_InstanceID" << std::endl;
        std::cout << "Case 95: invoke_SD_Request_non_existing_Major_Version" << std::endl;
        std::cout << "Case 96: invoke_SD_Request_non_existing_ServiceID" << std::endl;
        std::cout << "Case 97: invoke_SD_Reserved_Field_Endpoint_Option_set" << std::endl;
        std::cout << "Case 98: invoke_SD_SOMEIP_Length_shorter_as_expected" << std::endl;
        std::cout << "Case 99: invoke_SD_Specify_an_unexisting_IPv4_Address" << std::endl;
        std::cout << "Case 100: invoke_SD_Subscribe_after_StopSubscribe" << std::endl;
        std::cout << "Case 101: invoke_SD_SubscribeEventgroup_with_unallowed_option_ip" << std::endl;
        std::cout << "Case 102: invoke_SD_SubscribeEventgroup_with_unallowed_option_ip_2" << std::endl;
        std::cout << "Case 103: invoke_SD_Unknown_Option_type" << std::endl;
        std::cout << "Case 104: invoke_SD_Unreferenced_option" << std::endl;
        std::cout << "Case 105: invoke_SD_Unused_data_after_Options_Array" << std::endl;
        std::cout << "Case 106: invoke_SD_Unused_data_after_Options_Array_wrong_length" << std::endl;
        std::cout << "Case 107: invoke_Subscribe_using_wrong_SOMEIP_MessageID" << std::endl;
        std::cout << "Case 108: invoke_ResetInterface_wrong_Fire_and_forget_package_get_No_Error_back" << std::endl;
        std::cout << "Case 109: invoke_Eventgroup_EventsAndFieldsUnreliable_5" << std::endl;
        std::cout << "Case 110: invoke_SD_Calling_same_ports_before_and_after_suspendInterface" << std::endl;
        std::cout << "Case 111: invoke_SD_Check_Reboot_Detection_separate_multicast_and_unicast" << std::endl;
        std::cout << "Case 112: invoke_SD_Check_Reboot_Detection_Server_Side" << std::endl;
        std::cout << "Case 113: invoke_SD_Check_subscribe_eventgroup_ttl_expired" << std::endl;
        std::cout << "Case 114: invoke_SD_Deregister_from_Eventgroup" << std::endl;
        std::cout << "Case 115: invoke_SD_ResetInterface" << std::endl;
        std::cout << "Case 116: invoke_SD_Send_triggerEventUINT8_Eventgroup_2" << std::endl;
        std::cout << "Case 117: invoke_SD_Send_triggerEventUINT8Array_Eventgroup_2" << std::endl;
        std::cout << "Case 118: invoke_SD_Send_triggerEventUINT8E2E_Eventgroup_2" << std::endl;
        std::cout << "Case 119: invoke_SD_Send_triggerEventUINT8Multicast_Eventgroup_6" << std::endl;
        std::cout << "Case 120: invoke_SD_Interface_Version" << std::endl;
        std::cout << "Case 121: invoke_Fire_And_Forget_Wrong_Method_ID" << std::endl;
        std::cout << "Case 122: invoke_echoENUM" << std::endl;
        std::cout << "Case 123: invoke_activateTestSerivce" << std::endl;
        std::cout << "Case 124: tester_requestTestService" << std::endl;
        std::cout << "Case 125: invoke_requestTestServiceMethod" << std::endl;
        std::cout << "Case 126: invoke_deactivateTestSerivce" << std::endl;
        std::cout << "Case 127: invoke_Wrong_Return_Code" << std::endl;
        std::cout << "Case 128: invoke_echoUINT8ArrayMinSize_too_short" << std::endl;
        std::cout << "Case 129: Verify IN OUT TP data transfer in parallel" << std::endl;
        std::cout << "Case 130: invoke_TP_Verify_ErrorDuringReception" << std::endl;
        std::cout << "Case 131: invoke_TP_Verify_ReceptionBufferManagement" << std::endl;
        std::cout << "Case 132: invoke_TP_Verify_OffsetCalculationDuringReception" << std::endl;
        std::cout << "Case 133: invoke_TP_Verify_MissingFrameDuringReception" << std::endl;
        std::cout << "Case 134: invoke_TP_Verify_DuplicateFrameDuringReception" << std::endl;
        std::cout << "Case 135: invoke_resetInterface" << std::endl;
        std::cout << "Case 136: invoke_suspendInterface" << std::endl;
        std::cout << "Case 137: invoke_SD_SuspendInterface" << std::endl;
        std::cout << "Case 138: tester_subscribe_TestFieldUINT8" << std::endl;
        std::cout << "Case 139: tester_set_TestFieldUINT8" << std::endl;
        std::cout << "Case 140: tester_get_TestFieldUINT8" << std::endl;
        std::cout << "Case 141: tester_unsubscribe_TestFieldUINT8" << std::endl;
        std::cout << "Case 142: tester_subscribe_TestFieldUINT8Array" << std::endl;
        std::cout << "Case 143: tester_set_TestFieldUINT8Array" << std::endl;
        std::cout << "Case 144: tester_get_TestFieldUINT8Array" << std::endl;
        std::cout << "Case 145: tester_unsubscribe_TestFieldUINT8Array" << std::endl;
        std::cout << "Case 146: tester_subscribe_TestFieldUINT8Reliable" << std::endl;
        std::cout << "Case 147: tester_set_TestFieldUINT8Reliable" << std::endl;
        std::cout << "Case 148: tester_get_TestFieldUINT8Reliable" << std::endl;
        std::cout << "Case 149: tester_unsubscribe_TestFieldUINT8Reliable" << std::endl;
        std::cout << "Case 150: tester_subscribe_ETSInterfaceVersion" << std::endl;
        std::cout << "Case 151: tester_get_ETSInterfaceVersion" << std::endl;
        std::cout << "Case 152: tester_unsubscribe_ETSInterfaceVersion" << std::endl;
        std::cout << "Case 153: invoke_echoUINT8RELIABLE" << std::endl;
        std::cout << "Case 154: tester_subscribe_TestEventUINT8Reliable" << std::endl;
        std::cout << "Case 155: invoke_triggerEventUINT8Reliable" << std::endl;
        std::cout << "Case 156: tester_unsubscribe_TestEventUINT8Reliable" << std::endl;
        std::cout << "Case 157: tester_send_reliable_event" << std::endl;
        std::cout << "Case 158: invoke_clientServiceGetLastValueOfEventTCP" << std::endl;
        std::cout << "Enter case no:";
        std::cin >> indx;
        switch(indx) {
            case 0:
            {
                array_length = 4000;
                start = 2;
                duration = 4;
                debounce = 2;
                eventval = 8;
                meventval = 9;
                iteration = 100;
                startTimeout = 2;
                stopTimeout = 2;
                subscriptionDuration = 30;

                remote_unicast_ipaddr = "192.168.225.1";               
                remote_udp_port = 30515;

                local_unicast_ipaddr = "192.168.225.35";               
                local_udp_port = 30701;
                multicast_ipaddr = "224.0.0.1";
                multicast_port = 30490;
                proxyPtr->invoke_checkByteOrder();
                sleep(2);
                proxyPtr->invoke_echoBitfields();
                sleep(2);
                proxyPtr->invoke_echoCommonDatatypes();
                sleep(2);
                array_length = 1395;
                proxyPtr->invoke_echoUINT8ArrayLengthTP(array_length);
                sleep(2);
                array_length = 1396;
                proxyPtr->invoke_echoUINT8ArrayLengthTP(array_length);
                sleep(2);
                array_length = 1397;
                proxyPtr->invoke_echoUINT8ArrayLengthTP(array_length);
                sleep(2);
                array_length = 1395;
                proxyPtr->invoke_echoUINT8ArrayLengthInTP(array_length);
                sleep(2);
                array_length = 1396;
                proxyPtr->invoke_echoUINT8ArrayLengthInTP(array_length);
                sleep(2);
                array_length = 1397;
                proxyPtr->invoke_echoUINT8ArrayLengthInTP(array_length);
                sleep(2);
                array_length = 1395;
                proxyPtr->invoke_echoUINT8ArrayLengthOutTP(array_length);
                sleep(2);
                array_length = 1396;
                proxyPtr->invoke_echoUINT8ArrayLengthOutTP(array_length);
                sleep(2);
                array_length = 1397;
                proxyPtr->invoke_echoUINT8ArrayLengthOutTP(array_length);
                sleep(2);
                array_length = 1395;
                proxyPtr->invoke_echoUINT8ArrayLengthTPNoResponse(array_length);
                sleep(2);
                array_length = 1396;
                proxyPtr->invoke_echoUINT8ArrayLengthTPNoResponse(array_length);
                sleep(2);
                array_length = 1397;
                proxyPtr->invoke_echoUINT8ArrayLengthTPNoResponse(array_length);
                sleep(2);
                proxyPtr->tester_subscribe_TestEventUINT8TP();
                sleep(2);
                array_length = 1395;
                proxyPtr->invoke_triggerEventUINT8ArrayTP(array_length);
                sleep(2);
                array_length = 1396;
                proxyPtr->invoke_triggerEventUINT8ArrayTP(array_length);
                sleep(2);
                array_length = 1397;
                proxyPtr->invoke_triggerEventUINT8ArrayTP(array_length);
                sleep(2);
                array_length = 1395;
                proxyPtr->invoke_triggerEventUINT8ArrayTPNoReqTPPayload(array_length);
                sleep(2);
                array_length = 1396;
                proxyPtr->invoke_triggerEventUINT8ArrayTPNoReqTPPayload(array_length);
                sleep(2);
                array_length = 1397;
                proxyPtr->invoke_triggerEventUINT8ArrayTPNoReqTPPayload(array_length);
                sleep(2);
                /* To verify IN OUT TP processing in parallel */
                proxyPtr->invoke_triggerEventUINT8ArrayTPNoReqTPPayload(array_length);
                proxyPtr->invoke_echoUINT8ArrayLengthTPNoResponse(array_length);
                sleep(2);
                proxyPtr->tester_unsubscribe_TestEventUINT8TP();
                sleep(2);
                proxyPtr->invoke_TP_Verify_ErrorDuringReception(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_TP_Verify_ReceptionBufferManagement(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_TP_Verify_OffsetCalculationDuringReception(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_TP_Verify_MissingFrameDuringReception(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_TP_Verify_DuplicateFrameDuringReception(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_echoFLOAT64();
                sleep(2);
                proxyPtr->invoke_echoINT8();
                sleep(2);
                proxyPtr->invoke_echoStaticUINT8Array();
                sleep(2);
                proxyPtr->invoke_echoUINT8();
                sleep(2);
                proxyPtr->invoke_echoUINT8Array();
                sleep(2);
                proxyPtr->invoke_echoUINT8Array16BitLength();
                sleep(2);
                proxyPtr->invoke_echoUINT8Array2Dim();
                sleep(2);
                proxyPtr->invoke_echoUINT8Array8BitLength();
                sleep(2);
                proxyPtr->invoke_echoUINT8ArrayMinSize();
                sleep(2);
                proxyPtr->invoke_echoUTF16DYNAMIC();
                sleep(2);
                proxyPtr->invoke_echoUTF16FIXED();
                sleep(2);
                proxyPtr->invoke_echoUTF8DYNAMIC();
                sleep(2);
                proxyPtr->invoke_echoUTF8FIXED();
                sleep(2);
                proxyPtr->tester_subscribe_TestEventUINT8();
                sleep(2);
                proxyPtr->invoke_triggerEventUINT8(start, duration, debounce);
                sleep(5);
                proxyPtr->tester_unsubscribe_TestEventUINT8();
                sleep(2);
                proxyPtr->tester_subscribe_TestEventUINT8Array();
                sleep(2);
                proxyPtr->invoke_triggerEventUINT8Array(start, duration, debounce);
                sleep(5);
                proxyPtr->tester_unsubscribe_TestEventUINT8Array();
                sleep(2);
                proxyPtr->tester_subscribe_TestEventUINT8Multicast();
                sleep(2);
                proxyPtr->invoke_triggerEventUINT8Multicast(start, duration, debounce);
                sleep(5);
                proxyPtr->tester_unsubscribe_TestEventUINT8Multicast();
                sleep(2);
                proxyPtr->tester_subscribe_TestEventUINT8E2E();
                sleep(2);
                proxyPtr->invoke_triggerEventUINT8E2E(start, duration, debounce);
                sleep(5);
                proxyPtr->tester_unsubscribe_TestEventUINT8E2E();
                sleep(2);
                proxyPtr->invoke_echoUINT8E2E();
                sleep(2);
                proxyPtr->tester_subscribe_TestEventUINT32PeriodicEvent();
                sleep(2);
                proxyPtr->invoke_triggerEventUINT32Periodic(eventval);
                sleep(5);
                proxyPtr->tester_unsubscribe_TestEventUINT32PeriodicEvent();
                sleep(2);
                proxyPtr->tester_subscribe_TestEventUINT32UpdateOnChangeEvent();
                sleep(2);
                proxyPtr->invoke_triggerEventUINT32UpdateOnChange(eventval);
                sleep(5);
                proxyPtr->tester_unsubscribe_TestEventUINT32UpdateOnChangeEvent();
                sleep(2);
                proxyPtr->invoke_array_length_longer_as_message_length_allows_it();
                sleep(2);
                proxyPtr->invoke_array_length_too_long();
                sleep(2);
                proxyPtr->invoke_array_length_too_short_strips_payload();
                sleep(2);
                proxyPtr->invoke_burst_test(iteration);
                sleep(2);
                proxyPtr->invoke_echoUTF16DYNAMIC_length_too_long_for_string();
                sleep(2);
                proxyPtr->invoke_echoUTF16DYNAMIC_length_too_short_for_malformed_string();
                sleep(2);
                proxyPtr->invoke_echoUTF16DYNAMIC_length_too_short_for_string();
                sleep(2);
                proxyPtr->invoke_echoUTF16DYNAMIC_odd_number_before_termination();
                sleep(2);
                proxyPtr->invoke_echoUTF16DYNAMIC_with_odd_number_after_termination();
                sleep(2);
                proxyPtr->invoke_echoUTF16DYNAMIC_wrong_BOM();
                sleep(2);
                proxyPtr->invoke_echoUTF8DYNAMIC_length_too_long_for_string();
                sleep(2);
                proxyPtr->invoke_echoUTF8DYNAMIC_length_too_short_for_malformed_string();
                sleep(2);
                proxyPtr->invoke_echoUTF8DYNAMIC_length_too_short_for_string();
                sleep(2);
                proxyPtr->invoke_echoUTF8DYNAMIC_wrong_BOM();
                sleep(2);
                proxyPtr->invoke_echoUTF16FIXED_with_odd_number();
                sleep(2);
                proxyPtr->invoke_string_UTF16FIXED_too_long();
                sleep(2);
                proxyPtr->invoke_string_UTF16FIXED_too_short();
                sleep(2);
                proxyPtr->invoke_string_UTF8FIXED_too_long();
                sleep(2);
                proxyPtr->invoke_string_UTF8FIXED_too_short();
                sleep(2);
                proxyPtr->invoke_Wrong_Interface_Version();
                sleep(2);
                proxyPtr->invoke_Wrong_Message_Type();
                sleep(2);
                proxyPtr->invoke_Wrong_Method_ID();
                sleep(2);
                proxyPtr->invoke_Fire_And_Forget_Wrong_Method_ID();
                sleep(2);
                proxyPtr->invoke_echoENUM();
                sleep(2);
                proxyPtr->invoke_Wrong_Service_ID(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_Wrong_SOMEIP_Protocol_Version(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_Wrong_Return_Code(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_Length_equals_0_Test(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_Length_smaller_than_8_Test(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_Length_way_too_long(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Discover_Port_and_IP();
                sleep(2);
                proxyPtr->invoke_Sending_two_SOMEIP_Messages_in_a_row(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_UINT8Array_with_Length_0_strips_Payload();
                sleep(2);
                proxyPtr->invoke_Unaligned_SOMEIP_Messages_overUDP(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Answer_multiple_subscribes_together(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Check_Reaction_to_a_Subscribe_with_ttl_0(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Consider_Entries_Order(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Do_not_specify_a_port(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Do_not_specify_IPv4_Adress(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Empty_Entries_Array(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Empty_Option(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Empty_Options_Array(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Entries_Length_wrong_combined(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Options_Array_too_short(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Request_non_existing_EventgroupID(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Request_non_existing_InstanceID(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Request_non_existing_Major_Version(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Request_non_existing_ServiceID(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Reserved_Field_Endpoint_Option_set(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_SOMEIP_Length_shorter_as_expected(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Specify_an_unexisting_IPv4_Address(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Subscribe_after_StopSubscribe(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_SubscribeEventgroup_with_unallowed_option_ip(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_SubscribeEventgroup_with_unallowed_option_ip_2(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Unknown_Option_type(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Unreferenced_option(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Unused_data_after_Options_Array(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Unused_data_after_Options_Array_wrong_length(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_Subscribe_using_wrong_SOMEIP_MessageID(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_Eventgroup_EventsAndFieldsUnreliable_5(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Check_subscribe_eventgroup_ttl_expired(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(5);
                proxyPtr->invoke_SD_Deregister_from_Eventgroup(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Send_triggerEventUINT8_Eventgroup_2(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Send_triggerEventUINT8Array_Eventgroup_2(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Send_triggerEventUINT8E2E_Eventgroup_2(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_SD_Send_triggerEventUINT8Multicast_Eventgroup_6(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                proxyPtr->invoke_echoUINT8ArrayMinSize_too_short(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                sleep(2);
                service_id = 259;
                instance_id = 1;
                proxyPtr->invoke_activateTestSerivce(service_id, instance_id);
                sleep(2);
                service_id = 260;
                instance_id = 1;
                proxyPtr->invoke_activateTestSerivce(service_id, instance_id);
                sleep(2);
                service_id = 259;
                instance_id = 1;
                proxyPtr->tester_requestTestService(service_id, instance_id);
                sleep(2);
                service_id = 260;
                instance_id = 1;
                proxyPtr->tester_requestTestService(service_id, instance_id);
                sleep(2);
                service_id = 259;
                instance_id = 1;
                proxyPtr->invoke_requestTestServiceMethod(service_id, instance_id);
                sleep(2);
                service_id = 260;
                instance_id = 1;
                proxyPtr->invoke_requestTestServiceMethod(service_id, instance_id);
                sleep(2);
                service_id = 259;
                instance_id = 1;
                proxyPtr->invoke_deactivateTestSerivce(service_id, instance_id);
                sleep(2);
                service_id = 260;
                instance_id = 1;
                proxyPtr->invoke_deactivateTestSerivce(service_id, instance_id);
                sleep(2);
                service_id = 259;
                instance_id = 1;
                proxyPtr->invoke_activateTestSerivce(service_id, instance_id);
                sleep(2);
                service_id = 259;
                instance_id = 2;
                proxyPtr->invoke_activateTestSerivce(service_id, instance_id);
                sleep(2);
                service_id = 259;
                instance_id = 1;
                proxyPtr->tester_requestTestService(service_id, instance_id);
                sleep(2);
                service_id = 259;
                instance_id = 2;
                proxyPtr->tester_requestTestService(service_id, instance_id);
                sleep(2);
                service_id = 259;
                instance_id = 1;
                proxyPtr->invoke_requestTestServiceMethod(service_id, instance_id);
                sleep(2);
                service_id = 259;
                instance_id = 2;
                proxyPtr->invoke_requestTestServiceMethod(service_id, instance_id);
                sleep(2);
                service_id = 259;
                instance_id = 1;
                proxyPtr->invoke_deactivateTestSerivce(service_id, instance_id);
                sleep(2);
                service_id = 259;
                instance_id = 2;
                proxyPtr->invoke_deactivateTestSerivce(service_id, instance_id);
                sleep(2);
                proxyPtr->invoke_clientServiceActivate(startTimeout);
                sleep(4);
                proxyPtr->tester_send_offer();
                sleep(2);
                proxyPtr->invoke_clientServiceSubscribeEventgroup(startTimeout, subscriptionDuration);
                sleep(4);
                proxyPtr->tester_send_unicast_event(eventval);
                sleep(2);
                proxyPtr->invoke_clientServiceGetLastValueOfEventUDPUnicast();
                sleep(2);
                proxyPtr->tester_send_multicast_event(meventval);
                sleep(2);
                proxyPtr->invoke_clientServiceGetLastValueOfEventUDPMulticast();
                sleep(2);
                proxyPtr->invoke_clientServiceDeactivate(stopTimeout);
                sleep(2);
                proxyPtr->invoke_SD_ResetInterface();
                sleep(2);
                proxyPtr->invoke_ResetInterface_wrong_Fire_and_forget_package_get_No_Error_back();
                sleep(2);
                proxyPtr->invoke_SD_SuspendInterface();
                sleep(2);
                break;
            }
            case 1:
            {
                proxyPtr->invoke_checkByteOrder();
                break;
            }
            case 2:
            {
                std::cout << "Enter Start Timeout value in seconds:";
                std::cin >> startTimeout;
                proxyPtr->invoke_clientServiceActivate(startTimeout);
                break;
            }
            case 3:
            {
                proxyPtr->tester_send_offer();
                break;
            }
            case 4:
            {
                std::cout << "Enter Start Timeout value in seconds[1->with offer, 2->without offer]:";
                std::cin >> withoutOffer;
                proxyPtr->tester_send_stop_offer(withoutOffer);
                break;
            }
            case 5:
            {
                std::cout << "Enter Start Timeout value in seconds:";
                std::cin >> startTimeout;
                std::cout << "Enter Subscription Duration value in seconds:";
                std::cin >> subscriptionDuration;
                proxyPtr->invoke_clientServiceSubscribeEventgroup(startTimeout, subscriptionDuration);
                break;
            }
            case 6:
            {
                std::cout << "Enter Unicast Event value:";
                std::cin >> eventval;
                proxyPtr->tester_send_unicast_event(eventval);
                break;
            }
            case 7:
            {
                proxyPtr->invoke_clientServiceGetLastValueOfEventUDPUnicast();
                break;
            }
            case 8:
            {
                std::cout << "Enter Multicast Event value:";
                std::cin >> eventval;
                proxyPtr->tester_send_multicast_event(eventval);
                break;
            }
            case 9:
            {
                proxyPtr->invoke_clientServiceGetLastValueOfEventUDPMulticast();
                break;
            }
            case 10:
            {
                std::cout << "Enter Stop Timeout value in seconds:";
                std::cin >> stopTimeout;
                proxyPtr->invoke_clientServiceDeactivate(stopTimeout);
                break;
            }
            case 11:
            {
                std::cout << "Reset Secondary Service Session Id" << std::endl;
                proxyPtr->tester_send_session_reset();
                break;
            }
            case 12:
            {
                std::cout << "Update Remote Service Unreliable Port" << std::endl;
                proxyPtr->tester_update_service_unreliable_port();
                break;
            }
            case 13:
            {
                std::cout << "Get Interface version[257.1]" << std::endl;
                std::cout << "Enter service id:";
                std::cin >> service_id;
                std::cout << "Enter instance id:";
                std::cin >> instance_id;
                proxyPtr->tester_get_interface_version(service_id, instance_id);
                break;
            }
            case 14:
            {
                std::cout << "Reverse Bit Fileds" << std::endl;
                proxyPtr->invoke_echoBitfields();
                break;
            }
            case 15:
            {
                std::cout << "Echo Common Data Types" << std::endl;
                proxyPtr->invoke_echoCommonDatatypes();
                break;
            }
            case 16:
            {
                std::cout << "Send TP data over method call" << std::endl;
                std::cout << "Enter TP Array length(range:0-4996):";
                std::cin >> array_length;
                std::cout << "1. IN-TP OUT-TP" << std::endl;
                std::cout << "2. Only IN-TP" << std::endl;
                std::cout << "3. Only OUT-TP" << std::endl;
                std::cout << "4. Fire and Forget TP" << std::endl;
                std::cin >> indx;
                if (1==indx) {
                    proxyPtr->invoke_echoUINT8ArrayLengthTP(array_length);
                }
                else if (2==indx) {
                    proxyPtr->invoke_echoUINT8ArrayLengthInTP(array_length);
                }
                else if (3==indx) {
                    proxyPtr->invoke_echoUINT8ArrayLengthOutTP(array_length);
                }
                else if (4==indx) {
                    proxyPtr->invoke_echoUINT8ArrayLengthTPNoResponse(array_length);
                }
                break;
            }
            case 17:
            {
                std::cout << "Subscribe TP Event" << std::endl;
                proxyPtr->tester_subscribe_TestEventUINT8TP();
                break;
            }
            case 18:
            {
                std::cout << "TP broadcast event" << std::endl;
                std::cout << "Enter TP Array length(range:0-4996):";
                std::cin >> array_length;
                std::cout << "1. With TP Payload as Input" << std::endl;
                std::cout << "2. With TP Payload Size as Input" << std::endl;
                std::cin >> indx;
                if (1==indx) {
                    proxyPtr->invoke_triggerEventUINT8ArrayTP(array_length);
                }
                else if (2==indx) {
                   proxyPtr->invoke_triggerEventUINT8ArrayTPNoReqTPPayload(array_length);
                }
                break;
            }
            case 19:
            {
                std::cout << "Unsubscribe TP Event" << std::endl;
                proxyPtr->tester_unsubscribe_TestEventUINT8TP();
                break;
            }
            case 20:
            {
                std::cout << "echo double" << std::endl;
                proxyPtr->invoke_echoFLOAT64();
                break;
            }
            case 21:
            {
                std::cout << "echo signed byte" << std::endl;
                proxyPtr->invoke_echoINT8();
                break;
            }
            case 22:
            {
                std::cout << "echo static uint8 array" << std::endl;
                proxyPtr->invoke_echoStaticUINT8Array();
                break;
            }
            case 23:
            {
                std::cout << "echo uint8" << std::endl;
                proxyPtr->invoke_echoUINT8();
                break;
            }
            case 24:
            {
                std::cout << "echo uint8 array" << std::endl;
                proxyPtr->invoke_echoUINT8Array();
                break;
            }
            case 25:
            {
                std::cout << "echo uint8 array of 16bit length" << std::endl;
                proxyPtr->invoke_echoUINT8Array16BitLength();
                break;
            }
            case 26:
            {
                std::cout << "echo 2d uint8 array" << std::endl;
                proxyPtr->invoke_echoUINT8Array2Dim();
                break;
            }
            case 27:
            {
                std::cout << "echo uint8 array of 8bit length" << std::endl;
                proxyPtr->invoke_echoUINT8Array8BitLength();
                break;
            }
            case 28:
            {
                std::cout << "echo uint8 array of min size" << std::endl;
                proxyPtr->invoke_echoUINT8ArrayMinSize();
                break;
            }
            case 29:
            {
                std::cout << "echo utf16 dynamic" << std::endl;
                proxyPtr->invoke_echoUTF16DYNAMIC();
                break;
            }
            case 30:
            {
                std::cout << "echo utf16 fixed" << std::endl;
                proxyPtr->invoke_echoUTF16FIXED();
                break;
            }
            case 31:
            {
                std::cout << "echo utf8 dynamic" << std::endl;
                proxyPtr->invoke_echoUTF8DYNAMIC();
                break;
            }
            case 32:
            {
                std::cout << "echo utf8 fixed" << std::endl;
                proxyPtr->invoke_echoUTF8FIXED();
                break;
            }
            case 33:
            {
                std::cout << "Subscribe to TestEventUINT8" << std::endl;
                proxyPtr->tester_subscribe_TestEventUINT8();
                break;
            }
            case 34:
            {
                std::cout << "trigger uint8 event" << std::endl;
                std::cout << "Enter Start Timeout value in seconds:";
                std::cin >> start;
                std::cout << "Enter trigger event duration in seconds:";
                std::cin >> duration;
                std::cout << "Enter debounce time in seconds:";
                std::cin >> debounce;
                proxyPtr->invoke_triggerEventUINT8(start, duration, debounce);
                break;
            }
            case 35:
            {
                std::cout << "Unsubscribe TestEventUINT8" << std::endl;
                proxyPtr->tester_unsubscribe_TestEventUINT8();
                break;
            }
            case 36:
            {
                std::cout << "Subscribe to TestEventUINT8Array" << std::endl;
                proxyPtr->tester_subscribe_TestEventUINT8Array();
                break;
            }
            case 37:
            {
                std::cout << "trigger uint8 array event" << std::endl;
                std::cout << "Event payload range is set by duration*debounce rule" << std::endl;
                std::cout << "For payload = 1400 bytes, preferable input: duration:140 debounce:10" << std::endl;
                std::cout << "For payload > 1400 bytes, preferable input: duration:140 debounce:14" << std::endl;
                std::cout << "For payload < 1400 bytes, preferable input: duration:140 debounce:7" << std::endl;
                std::cout << "Enter Start Timeout value in seconds:";
                std::cin >> start;
                std::cout << "Enter trigger event duration in seconds:";
                std::cin >> duration;
                std::cout << "Enter debounce time in seconds:";
                std::cin >> debounce;
                proxyPtr->invoke_triggerEventUINT8Array(start, duration, debounce);
                break;
            }
            case 38:
            {
                std::cout << "Unsubscribe TestEventUINT8Array" << std::endl;
                proxyPtr->tester_unsubscribe_TestEventUINT8Array();
                break;
            }
            case 39:
            {
                std::cout << "Subscribe to TestEventUINT8Multicast" << std::endl;
                proxyPtr->tester_subscribe_TestEventUINT8Multicast();
                break;
            }
            case 40:
            {
                std::cout << "trigger uint8 multicast event" << std::endl;
                std::cout << "Enter Start Timeout value in seconds:";
                std::cin >> start;
                std::cout << "Enter trigger event duration in seconds:";
                std::cin >> duration;
                std::cout << "Enter debounce time in seconds:";
                std::cin >> debounce;
                proxyPtr->invoke_triggerEventUINT8Multicast(start, duration, debounce);
                break;
            }
            case 41:
            {
                std::cout << "Unsubscribe TestEventUINT8Multicast" << std::endl;
                proxyPtr->tester_unsubscribe_TestEventUINT8Multicast();
                break;
            }
            case 42:
            {
                std::cout << "Subscribe to TestEventUINT8E2E" << std::endl;
                proxyPtr->tester_subscribe_TestEventUINT8E2E();
                break;
            }
            case 43:
            {
                std::cout << "trigger uint8 e2e event" << std::endl;
                std::cout << "Enter Start Timeout value in seconds:";
                std::cin >> start;
                std::cout << "Enter trigger event duration in seconds:";
                std::cin >> duration;
                std::cout << "Enter debounce time in seconds:";
                std::cin >> debounce;
                proxyPtr->invoke_triggerEventUINT8E2E(start, duration, debounce);
                break;
            }
            case 44:
            {
                std::cout << "Unsubscribe TestEventUINT8E2E" << std::endl;
                proxyPtr->tester_unsubscribe_TestEventUINT8E2E();
                break;
            }
            case 45:
            {
                std::cout << "echo uint8 e2e" << std::endl;
                proxyPtr->invoke_echoUINT8E2E();
                break;
            }
            case 46:
            {
                std::cout << "suscribe periodic event:";
                proxyPtr->tester_subscribe_TestEventUINT32PeriodicEvent();
                break;
            }
            case 47:
            {
                std::cout << "enter periodic event value:";
                std::cin >> eventval;
                proxyPtr->invoke_triggerEventUINT32Periodic(eventval);
                break;
            }
            case 48:
            {
                std::cout << "unsuscribe periodic event:";
                proxyPtr->tester_unsubscribe_TestEventUINT32PeriodicEvent();
                break;
            }
            case 49:
            {
                std::cout << "suscribe on change event:";
                proxyPtr->tester_subscribe_TestEventUINT32UpdateOnChangeEvent();
                break;
            }
            case 50:
            {
                std::cout << "enter on change event value:";
                std::cin >> eventval;
                proxyPtr->invoke_triggerEventUINT32UpdateOnChange(eventval);
                break;
            }
            case 51:
            {
                std::cout << "unsuscribe on change event:";
                proxyPtr->tester_unsubscribe_TestEventUINT32UpdateOnChangeEvent();
                break;
            }
            case 52:
            {
                std::cout << "array length longer as message length:";
                proxyPtr->invoke_array_length_longer_as_message_length_allows_it();
                break;
            }
            case 53:
            {
                std::cout << "array length too long:";
                proxyPtr->invoke_array_length_too_long();
                break;
            }
            case 54:
            {
                std::cout << "array length too short strip payload:";
                proxyPtr->invoke_array_length_too_short_strips_payload();
                break;
            }
            case 55:
            {
                std::cout << "burst test with iteration:";
                std::cin >> iteration;
                proxyPtr->invoke_burst_test(iteration);
                break;
            }
            case 56:
            {
                std::cout << "invoke_echoUTF16DYNAMIC_length_too_long_for_string";
                proxyPtr->invoke_echoUTF16DYNAMIC_length_too_long_for_string();
                break;
            }
            case 57:
            {
                std::cout << "invoke_echoUTF16DYNAMIC_length_too_short_for_malformed_string";
                proxyPtr->invoke_echoUTF16DYNAMIC_length_too_short_for_malformed_string();
                break;
            }
            case 58:
            {
                std::cout << "invoke_echoUTF16DYNAMIC_length_too_short_for_string";
                proxyPtr->invoke_echoUTF16DYNAMIC_length_too_short_for_string();
                break;
            }
            case 59:
            {
                std::cout << "invoke_echoUTF16DYNAMIC_odd_number_before_termination";
                proxyPtr->invoke_echoUTF16DYNAMIC_odd_number_before_termination();
                break;
            }
            case 60:
            {
                std::cout << "invoke_echoUTF16DYNAMIC_with_odd_number_after_termination";
                proxyPtr->invoke_echoUTF16DYNAMIC_with_odd_number_after_termination();
                break;
            }
            case 61:
            {
                std::cout << "invoke_echoUTF16DYNAMIC_wrong_BOM";
                proxyPtr->invoke_echoUTF16DYNAMIC_wrong_BOM();
                break;
            }
            case 62:
            {
                std::cout << "invoke_echoUTF8DYNAMIC_length_too_long_for_string";
                proxyPtr->invoke_echoUTF8DYNAMIC_length_too_long_for_string();
                break;
            }
            case 63:
            {
                std::cout << "invoke_echoUTF8DYNAMIC_length_too_short_for_malformed_string";
                proxyPtr->invoke_echoUTF8DYNAMIC_length_too_short_for_malformed_string();
                break;
            }
            case 64:
            {
                std::cout << "invoke_echoUTF8DYNAMIC_length_too_short_for_string";
                proxyPtr->invoke_echoUTF8DYNAMIC_length_too_short_for_string();
                break;
            }
            case 65:
            {
                std::cout << "invoke_echoUTF8DYNAMIC_wrong_BOM";
                proxyPtr->invoke_echoUTF8DYNAMIC_wrong_BOM();
                break;
            }
            case 66:
            {
                std::cout << "invoke_echoUTF16FIXED_with_odd_number";
                proxyPtr->invoke_echoUTF16FIXED_with_odd_number();
                break;
            }
            case 67:
            {
                std::cout << "invoke_string_UTF16FIXED_too_long";
                proxyPtr->invoke_string_UTF16FIXED_too_long();
                break;
            }
            case 68:
            {
                std::cout << "invoke_string_UTF16FIXED_too_short";
                proxyPtr->invoke_string_UTF16FIXED_too_short();
                break;
            }
            case 69:
            {
                std::cout << "invoke_string_UTF8FIXED_too_long";
                proxyPtr->invoke_string_UTF8FIXED_too_long();
                break;
            }
            case 70:
            {
                std::cout << "invoke_string_UTF8FIXED_too_short";
                proxyPtr->invoke_string_UTF8FIXED_too_short();
                break;
            }
            case 71:
            {
                std::cout << "invoke_Wrong_Interface_Version";
                proxyPtr->invoke_Wrong_Interface_Version();
                break;
            }
            case 72:
            {
                std::cout << "invoke_Wrong_Message_Type";
                proxyPtr->invoke_Wrong_Message_Type();
                break;
            }
            case 73:
            {
                std::cout << "invoke_Wrong_Method_ID";
                proxyPtr->invoke_Wrong_Method_ID();
                break;
            }
            case 74:
            {
                std::cout << "invoke_Wrong_Service_ID" << std::endl;
                std::cout << "invoke_Length_equals_0_Test";
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_Wrong_Service_ID(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 75:
            {
                std::cout << "invoke_Wrong_SOMEIP_Protocol_Version" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_Wrong_SOMEIP_Protocol_Version(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 76:
            {
                std::cout << "invoke_Length_equals_0_Test" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_Length_equals_0_Test(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 77:
            {
                std::cout << "invoke_Length_smaller_than_8_Test" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_Length_smaller_than_8_Test(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 78:
            {
                std::cout << "invoke_Length_way_too_long" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_Length_way_too_long(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 79:
            {
                std::cout << "invoke_SD_Discover_Port_and_IP" << std::endl;
                proxyPtr->invoke_SD_Discover_Port_and_IP();
                break;
            }
            case 80:
            {
                std::cout << "invoke_Sending_two_SOMEIP_Messages_in_a_row" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_Sending_two_SOMEIP_Messages_in_a_row(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 81:
            {
                std::cout << "invoke_UINT8Array_with_Length_0_strips_Payload" << std::endl;
                proxyPtr->invoke_UINT8Array_with_Length_0_strips_Payload();
                break;
            }
            case 82:
            {
                std::cout << "invoke_Unaligned_SOMEIP_Messages_overUDP" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_Unaligned_SOMEIP_Messages_overUDP(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 83:
            {
                std::cout << "invoke_SD_Answer_multiple_subscribes_together" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Answer_multiple_subscribes_together(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 84:
            {
                std::cout << "invoke_SD_Check_Reaction_to_a_Subscribe_with_ttl_0" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Check_Reaction_to_a_Subscribe_with_ttl_0(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 85:
            {
                std::cout << "invoke_SD_Consider_Entries_Order" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Consider_Entries_Order(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 86:
            {
                std::cout << "invoke_SD_Do_not_specify_a_port" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Do_not_specify_a_port(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 87:
            {
                std::cout << "invoke_SD_Do_not_specify_IPv4_Adress" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Do_not_specify_IPv4_Adress(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 88:
            {
                std::cout << "invoke_SD_Empty_Entries_Array" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Empty_Entries_Array(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 89:
            {
                std::cout << "invoke_SD_Empty_Option" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Empty_Option(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 90:
            {
                std::cout << "invoke_SD_Empty_Options_Array" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Empty_Options_Array(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 91:
            {
                std::cout << "invoke_SD_Entries_Length_wrong_combined" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Entries_Length_wrong_combined(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 92:
            {
                std::cout << "invoke_SD_Options_Array_too_short" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Options_Array_too_short(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 93:
            {
                std::cout << "invoke_SD_Request_non_existing_EventgroupID" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Request_non_existing_EventgroupID(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 94:
            {
                std::cout << "invoke_SD_Request_non_existing_InstanceID" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Request_non_existing_InstanceID(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 95:
            {
                std::cout << "invoke_SD_Request_non_existing_Major_Version" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Request_non_existing_Major_Version(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 96:
            {
                std::cout << "invoke_SD_Request_non_existing_ServiceID" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Request_non_existing_ServiceID(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 97:
            {
                std::cout << "invoke_SD_Reserved_Field_Endpoint_Option_set" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Reserved_Field_Endpoint_Option_set(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 98:
            {
                std::cout << "invoke_SD_SOMEIP_Length_shorter_as_expected" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_SOMEIP_Length_shorter_as_expected(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 99:
            {
                std::cout << "invoke_SD_Specify_an_unexisting_IPv4_Address" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Specify_an_unexisting_IPv4_Address(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 100:
            {
                std::cout << "invoke_SD_Subscribe_after_StopSubscribe" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Subscribe_after_StopSubscribe(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 101:
            {
                std::cout << "invoke_SD_SubscribeEventgroup_with_unallowed_option_ip" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_SubscribeEventgroup_with_unallowed_option_ip(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 102:
            {
                std::cout << "invoke_SD_SubscribeEventgroup_with_unallowed_option_ip_2" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_SubscribeEventgroup_with_unallowed_option_ip_2(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 103:
            {
                std::cout << "invoke_SD_Unknown_Option_type" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Unknown_Option_type(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 104:
            {
                std::cout << "invoke_SD_Unreferenced_option" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Unreferenced_option(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 105:
            {
                std::cout << "invoke_SD_Unused_data_after_Options_Array" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Unused_data_after_Options_Array(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 106:
            {
                std::cout << "invoke_SD_Unused_data_after_Options_Array_wrong_length" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Unused_data_after_Options_Array_wrong_length(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 107:
            {
                std::cout << "invoke_Subscribe_using_wrong_SOMEIP_MessageID" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_Subscribe_using_wrong_SOMEIP_MessageID(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 108:
            {
                proxyPtr->invoke_ResetInterface_wrong_Fire_and_forget_package_get_No_Error_back();
                break;
            }
            case 109:
            {
                std::cout << "invoke_Eventgroup_EventsAndFieldsUnreliable_5" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_Eventgroup_EventsAndFieldsUnreliable_5(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 110:
            {
                std::cout << "invoke_SD_Calling_same_ports_before_and_after_suspendInterface" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Calling_same_ports_before_and_after_suspendInterface(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 111:
            {
                std::cout << "invoke_SD_Check_Reboot_Detection_separate_multicast_and_unicast" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Check_Reboot_Detection_separate_multicast_and_unicast(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 112:
            {
                std::cout << "invoke_SD_Check_Reboot_Detection_Server_Side" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Check_Reboot_Detection_Server_Side(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 113:
            {
                std::cout << "invoke_SD_Check_subscribe_eventgroup_ttl_expired" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Check_subscribe_eventgroup_ttl_expired(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 114:
            {
                std::cout << "invoke_SD_Deregister_from_Eventgroup" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Deregister_from_Eventgroup(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 115:
            {
                std::cout << "invoke_SD_ResetInterface" << std::endl;
                proxyPtr->invoke_SD_ResetInterface();
                break;
            }
            case 116:
            {
                std::cout << "invoke_SD_Send_triggerEventUINT8_Eventgroup_2" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Send_triggerEventUINT8_Eventgroup_2(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 117:
            {
                std::cout << "invoke_SD_Send_triggerEventUINT8Array_Eventgroup_2" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Send_triggerEventUINT8Array_Eventgroup_2(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 118:
            {
                std::cout << "invoke_SD_Send_triggerEventUINT8E2E_Eventgroup_2" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Send_triggerEventUINT8E2E_Eventgroup_2(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 119:
            {
                std::cout << "invoke_SD_Send_triggerEventUINT8Multicast_Eventgroup_6" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Send_triggerEventUINT8Multicast_Eventgroup_6(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 120:
            {
                std::cout << "invoke_SD_Interface_Version" << std::endl;
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter multicast port:";
                std::cin >> multicast_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_SD_Interface_Version(remote_unicast_ipaddr, (uint16_t)multicast_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 121:
            {
                std::cout << "invoke_Fire_And_Forget_Wrong_Method_ID" << std::endl;
                proxyPtr->invoke_Fire_And_Forget_Wrong_Method_ID();
                break;
            }
            case 122:
            {
                std::cout << "invoke_echoENUM" << std::endl;
                proxyPtr->invoke_echoENUM();
                break;
            }
            case 123:
            {
                std::cout << "Enter service id:";
                std::cin >> service_id;
                std::cout << "Enter instance id:";
                std::cin >> instance_id;
                proxyPtr->invoke_activateTestSerivce(service_id, instance_id);
                break;
            }
            case 124:
            {
                std::cout << "Enter service id:";
                std::cin >> service_id;
                std::cout << "Enter instance id:";
                std::cin >> instance_id;
                proxyPtr->tester_requestTestService(service_id, instance_id);
                break;
            }
            case 125:
            {
                std::cout << "Enter service id:";
                std::cin >> service_id;
                std::cout << "Enter instance id:";
                std::cin >> instance_id;
                proxyPtr->invoke_requestTestServiceMethod(service_id, instance_id);
                break;
            }
            case 126:
            {
                std::cout << "Enter service id:";
                std::cin >> service_id;
                std::cout << "Enter instance id:";
                std::cin >> instance_id;
                proxyPtr->invoke_deactivateTestSerivce(service_id, instance_id);
                break;
            }
            case 127:
            {
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_Wrong_Return_Code(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 128:
            {
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_echoUINT8ArrayMinSize_too_short(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 129:
            {
                std::cout << "Enter TP Array length(range:0-4996):";
                std::cin >> array_length;
                proxyPtr->tester_subscribe_TestEventUINT8TP();
                sleep(2);
                proxyPtr->invoke_triggerEventUINT8ArrayTPNoReqTPPayload(array_length);
                proxyPtr->invoke_echoUINT8ArrayLengthTPNoResponse(array_length);
                sleep(2);
                proxyPtr->tester_unsubscribe_TestEventUINT8TP();
                break;
            }
            case 130:
            {
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_TP_Verify_ErrorDuringReception(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 131:
            {
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_TP_Verify_ReceptionBufferManagement(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 132:
            {
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_TP_Verify_OffsetCalculationDuringReception(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 133:
            {
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_TP_Verify_MissingFrameDuringReception(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 134:
            {
                std::cout << "Enter remote unicast ipaddr:";
                std::cin >> remote_unicast_ipaddr;
                std::cout << "Enter remote udp port:";
                std::cin >> remote_udp_port;
                std::cout << "Enter local unicast ipaddr:";
                std::cin >> local_unicast_ipaddr;
                std::cout << "Enter local port:";
                std::cin >> local_udp_port;
                proxyPtr->invoke_TP_Verify_DuplicateFrameDuringReception(remote_unicast_ipaddr, (uint16_t)remote_udp_port, local_unicast_ipaddr, (uint16_t)local_udp_port);
                break;
            }
            case 135:
            {
                proxyPtr->invoke_resetInterface();
                break;
            }
            case 136:
            {
                std::cout << "Enter Start Timeout value in seconds:";
                std::cin >> start;
                std::cout << "Enter trigger event duration in seconds:";
                std::cin >> duration;
                proxyPtr->invoke_suspendInterface(start, duration);
                break;
            }
            case 137:
            {
                std::cout << "invoke_SD_SuspendInterface" << std::endl;
                proxyPtr->invoke_SD_SuspendInterface();
                break;
            }
            case 138:
            {
                std::cout << "tester_subscribe_TestFieldUINT8" << std::endl;
                proxyPtr->tester_subscribe_TestFieldUINT8();
                break;
            }
            case 139:
            {
                std::cout << "tester_set_TestFieldUINT8" << std::endl;
                std::cout << "Enter Field value:";
                std::cin >> fieldval;
                proxyPtr->tester_set_TestFieldUINT8(fieldval);
                break;
            }
            case 140:
            {
                std::cout << "tester_get_TestFieldUINT8" << std::endl;
                proxyPtr->tester_get_TestFieldUINT8();
                break;
            }
            case 141:
            {
                std::cout << "tester_unsubscribe_TestFieldUINT8" << std::endl;
                proxyPtr->tester_unsubscribe_TestFieldUINT8();
                break;
            }
            case 142:
            {
                std::cout << "tester_subscribe_TestFieldUINT8Array" << std::endl;
                proxyPtr->tester_subscribe_TestFieldUINT8Array();
                break;
            }
            case 143:
            {
                std::cout << "tester_set_TestFieldUINT8Array" << std::endl;
                proxyPtr->tester_set_TestFieldUINT8Array();
                break;
            }
            case 144:
            {
                std::cout << "tester_get_TestFieldUINT8Array" << std::endl;
                proxyPtr->tester_get_TestFieldUINT8Array();
                break;
            }
            case 145:
            {
                std::cout << "tester_unsubscribe_TestFieldUINT8Array" << std::endl;
                proxyPtr->tester_unsubscribe_TestFieldUINT8Array();
                break;
            }
            case 146:
            {
                std::cout << "tester_subscribe_TestFieldUINT8Reliable" << std::endl;
                proxyPtr->tester_subscribe_TestFieldUINT8Reliable();
                break;
            }
            case 147:
            {
                std::cout << "tester_set_TestFieldUINT8Reliable" << std::endl;
                std::cout << "Enter Field value:";
                std::cin >> fieldval;
                proxyPtr->tester_set_TestFieldUINT8Reliable(fieldval);
                break;
            }
            case 148:
            {
                std::cout << "tester_get_TestFieldUINT8Reliable" << std::endl;
                proxyPtr->tester_get_TestFieldUINT8Reliable();
                break;
            }
            case 149:
            {
                std::cout << "tester_unsubscribe_TestFieldUINT8Reliable" << std::endl;
                proxyPtr->tester_unsubscribe_TestFieldUINT8Reliable();
                break;
            }
            case 150:
            {
                std::cout << "tester_subscribe_ETSInterfaceVersion" << std::endl;
                proxyPtr->tester_subscribe_ETSInterfaceVersion();
                break;
            }
            case 151:
            {
                std::cout << "tester_get_ETSInterfaceVersion" << std::endl;
                proxyPtr->tester_get_ETSInterfaceVersion();
                break;
            }
            case 152:
            {
                std::cout << "tester_unsubscribe_ETSInterfaceVersion" << std::endl;
                proxyPtr->tester_unsubscribe_ETSInterfaceVersion();
                break;
            }
            case 153:
            {
                std::cout << "invoke_echoUINT8RELIABLE" << std::endl;
                proxyPtr->invoke_echoUINT8RELIABLE();
                break;
            }
            case 154:
            {
                std::cout << "tester_subscribe_TestEventUINT8Reliable" << std::endl;
                proxyPtr->tester_subscribe_TestEventUINT8Reliable();
                break;
            }
            case 155:
            {
                std::cout << "invoke_triggerEventUINT8Reliable" << std::endl;
                std::cout << "Enter Start Timeout value in seconds:";
                std::cin >> start;
                std::cout << "Enter trigger event duration in seconds:";
                std::cin >> duration;
                std::cout << "Enter debounce time in seconds:";
                std::cin >> debounce;
                proxyPtr->invoke_triggerEventUINT8Reliable(start, duration, debounce);
                break;
            }
            case 156:
            {
                std::cout << "tester_unsubscribe_TestEventUINT8Reliable" << std::endl;
                proxyPtr->tester_unsubscribe_TestEventUINT8Reliable();
                break;
            }
            case 157:
            {
                std::cout << "Enter Reliable Event value:";
                std::cin >> eventval;
                proxyPtr->tester_send_reliable_event(eventval);
                break;
            }
            case 158:
            {
                std::cout << "invoke_clientServiceGetLastValueOfEventTCP" << std::endl;
                proxyPtr->invoke_clientServiceGetLastValueOfEventTCP();
                break;
            }
            default:
            {
                std::cout << "Not a valid case" << std::endl;
                break;
            }
        }
    }

    return 0;
}
