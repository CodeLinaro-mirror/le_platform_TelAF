/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "configuration.hpp"

using namespace tafsvc;
using namespace std;

static void examples(cfg::Node & node)
{
    {
        cfg::Node & dtc = cfg::get_dtc_node((uint32_t)0xAB0000);
        std::cout << "Eg. get_dtc_node 1 ..code: " << dtc.get<int>("identification.code") << std::endl;
        std::cout << "Eg. get_dtc_node 2 ..description: " << dtc.get<string>("identification.description") << std::endl;
    }

    {
        cfg::Node & dtc = cfg::get_dtc_node((uint16_t)0x0001);
        std::cout << "Eg. get_dtc_node 1 ..event id: " << dtc.get_child("events").begin()->second.get_value<int>() << std::endl;
    }

    {
        cfg::Node & ev = cfg::get_event_node(0x0001);
        std::cout << "Eg. get_event_node 1 ..name: " << ev.get<string>("long_name") << std::endl;

        cfg::Node & indicator = ev.get_child("connected_indicator.indicator");
        std::cout << "Eg. get_event_node 2 ..list item: " << indicator.begin()->second.data() << std::endl;
    }

    {
        std::map<uint16_t, std::shared_ptr<cfg::Node>> ev_map = cfg::get_event_nodes(0xAB0000);
        for (const auto & ev : ev_map)
        {
            std::cout << "Eg. get_event_node_list .. eid: " << ev.first << ", long_name: " << ev.second->get<string>("long_name") << std::endl;
        }
    }

    {
        vector<uint16_t> eid_list = cfg::get_event_ids(0xAB0000);
        for (auto & eid : eid_list)
        {
            std::cout << "Eg. get_event_id_list ..eid: " <<  eid << std::endl;
        }
    }

    {
        std::map<uint32_t, std::shared_ptr<cfg::Node>> dtc_map = cfg::get_dtc_nodes();
        for (const auto & dtc: dtc_map)
        {
            std::cout << "Eg. get_dtc_nodes .. dtc_code: " << dtc.first << ", description:" << dtc.second->get<string>("identification.description") << std::endl;
        }
    }

    {
        std::vector<uint32_t> dtc_code_list = cfg::get_dtc_codes();
        for (const auto & dtc_code: dtc_code_list)
        {
            std::cout << "Eg. get_dtc_codes .. code: " << dtc_code << std::endl;
        }
    }

    {
        cfg::Node & n = cfg::top_dtc_all<int>("identification.code", 0xAB0000);
        std::cout << "Eg. top_dtc_all 1..snapshot_record_content: " << n.get<string>("snapshots.snapshot_record_content") << std::endl;
    }

    {
        cfg::Node & n = cfg::top_events<int>("id", 0x0001);
        std::cout << "Eg. top_events ..long_name: " << n.get<string>("long_name") <<std::endl;

        cfg::Node & nn = cfg::top_events<int>("connected_indicator.indicator_failure_cycle_counter_threshold", 1);
        std::cout << "Eg. top_events 3..debounce_algorithm: " << nn.get<string>("debounce_algorithm") << std::endl;
    }

    {
        cfg::Node & n = cfg::top_debounce_algorithm<string>("short_name", "Counter_1");
        std::cout << "Eg. top_debounce_algorithm 1..name: " << n.get<string>("short_name") << std::endl;

        cfg::Counter_t temp;
        cfg::top_debounce_algorithm<string>("short_name", "Counter_1", &temp);
        std::cout << "Eg. top_debounce_algorithm 2..counter_passed_threshold: " << temp.counter_passed_threshold << std::endl;
    }

    {
        cfg::Timer_t temp;
        cfg::top_debounce_algorithm<string>("short_name", "Time_1", &temp);
        std::cout << "Eg. top_debounce_algorithm 3..time_failed_threshold: " << temp.time_failed_threshold << std::endl;
    }

    {
        cfg::Custom_t temp;
        cfg::top_debounce_algorithm<string>("short_name", "Custom", &temp);
        std::cout << "Eg. top_debounce_algorithm 4..monitor_internal: " << temp.monitor_internal << std::endl;
    }

    {
        cfg::Node & n = cfg::top_freeze_frames<string>("short_name", "lastOccurrence");
        std::cout << "Eg. top_freeze_frames 1 .. trigger: " << n.get<string>("trigger") << std::endl;
    }

    {
        cfg::Node & n = cfg::top_operation_cycle<string>("short_name", "POWER");
        std::cout << "Eg. top_operation_cycle 1 .. type: " << n.get<string>("type") << std::endl;
    }

    {
        cfg::Node & n = cfg::top_extended_data_records<string>("short_name", "IUMPRNumerator");
        std::cout << "Eg. top_extended_data_records 1 .. trigger: " << n.get<string>("trigger") << std::endl;
    }

}

COMPONENT_INIT
{

#define CFG_JSON_FILE_NAME "diag_template.yaml.json"

    if (access(CFG_JSON_FILE_NAME, F_OK) != 0)
    {
        std::cerr << "Please put the [" CFG_JSON_FILE_NAME "] into the sample-app root dir." << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        cfg::diag_config_init(CFG_JSON_FILE_NAME);
        cfg::Node & root = cfg::get_root_node();
        examples(root);
    } catch (const std::exception& e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    exit(EXIT_SUCCESS);
}
