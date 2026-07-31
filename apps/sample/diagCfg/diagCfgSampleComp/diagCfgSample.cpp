/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "configuration.hpp"
#include <iostream>

using namespace tafsvc;
using namespace std;

static void examples()
{
    std::cout << "\n--- EXAMPLES USING SERIALIZATION-BASED API ---\n" << std::endl;

    // Example 1: Get a specific DTC Entry and access its members
    try
    {
        cout << "--- Example: Get a single DTC Entry ---\n";
        // Use the new, type-safe getter for a specific DTC.
        const DTCEntry& dtc = cfg::get_dtc_entry(0xAB0000);

        cout << "Eg. get_dtc_entry ... code: " << dtc.identification.code << endl;
        cout << "Eg. get_dtc_entry ... first event ID: " << dtc.events.front() << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "Example failed: " << e.what() << endl;
    }
    cout << "-------------------------------------------\n" << endl;

    // Example 2: Get a specific Event Entry
    try
    {
        cout << "--- Example: Get a single Event Entry ---\n";
        const EventEntry& ev = cfg::get_event_entry(0x0001);
        cout << "Eg. get_event_entry ... debounce_algorithm: " << ev.debounce_algorithm << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "Example failed: " << e.what() << endl;
    }
    cout << "-------------------------------------------\n" << endl;


    // Example 3: Get all Event IDs for a specific DTC
    try
    {
        cout << "--- Example: Get all Event IDs for a DTC ---\n";

        vector<uint16_t> eid_list = cfg::get_event_ids(0xAB0000);
        cout << "Eg. get_event_ids for DTC 0xAB0000:" << endl;
        for (const auto& eid : eid_list)
        {
            cout << "  - Event ID: " << eid << endl;
        }
    }
    catch (const std::exception& e)
    {
        cerr << "Example failed: " << e.what() << endl;
    }
    cout << "-------------------------------------------\n" << endl;

    // Example 4: Iterate over all configured DTCs
    try
    {
        cout << "--- Example: Iterate over all DTCs ---\n";

        const auto& dtc_map = cfg::get_all_dtc_entries();
        cout << "Eg. get_all_dtc_entries:" << endl;
        for (const auto& dtc_pair : dtc_map)
        {
            cout << "  - DTC Code: " << dtc_pair.first
                 << ", Fault Type: " << dtc_pair.second.identification.fault_type << endl;
        }
    }
    catch (const std::exception& e)
    {
        cerr << "Example failed: " << e.what() << endl;
    }
    cout << "-------------------------------------------\n" << endl;

    // Example 5: Get a specific Debounce Algorithm
    try
    {
        cout << "--- Example: Get Debounce Algorithm details ---\n";
        // Debounce algorithms are stored inside the main diagConf object.
        // We can get them via a specific getter.
        const DebounceAlgorithm& algos = cfg::get_debounce_algorithms();
        cout << "Eg. get_debounce_algorithms ... Counter_1 passed_threshold: "
             << algos.counter_based.counter_passed_threshold << endl;

        cout << "Eg. get_debounce_algorithms ... Time_1 failed_threshold: "
             << algos.time_based.time_failed_threshold << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "Example failed: " << e.what() << endl;
    }
    cout << "-------------------------------------------\n" << endl;

    // Example 6: Get a specific Freeze Frame Entry
    try
    {
        cout << "--- Example: Get a single Freeze Frame ---\n";
        const FreezeFrameEntry& ff = cfg::get_freeze_frame_entry("lastOccurrence");
        cout << "Eg. get_freeze_frame_entry 'lastOccurrence' ... trigger: " << ff.trigger << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "Example failed: " << e.what() << endl;
    }
    cout << "-------------------------------------------\n" << endl;
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
        // Initialize the configuration from the JSON file.
        cfg::diag_config_init(CFG_JSON_FILE_NAME);

        // Run the serialization examples.
        examples();

    } catch (const std::exception& e){
        std::cerr << "CRITICAL FAILURE: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
