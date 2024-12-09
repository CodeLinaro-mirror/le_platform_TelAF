/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __CONFIGURATION_HPP__
#define __CONFIGURATION_HPP__

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cstdint>
#include <memory>


#define EXPORT_SYM __attribute__((visibility ("default")))

namespace telux {
namespace tafsvc {
namespace cfg {

typedef boost::property_tree::ptree Node;

typedef enum operation_cycle_type
{
    IGNITION,
    DC,
    POWER,
    WUC
} operation_cycle_type_t;

typedef enum freeze_frame_trigger_type
{
    DEM_TRIGGER_ON_CONFIRMED,
    DEM_TRIGGER_ON_EVERY_TEST_FAILED,
    DEM_TRIGGER_ON_FDC_THRESHOLD,
    DEM_TRIGGER_ON_PENDING,
    DEM_TRIGGER_ON_TEST_FAILED,
    DEM_TRIGGER_ON_TEST_FAILED_THIS_OPERATION_CYCLE
} freeze_frame_trigger_type_t;

typedef enum freeze_frames_type
{
    firstOccurrence,
    lastOccurrence
} freeze_frames_type_t;

typedef enum extended_data_records_type
{
    OccurrenceCounter,
    CumulativeDistanceWithTestFailed
} extended_data_records_type_t;

typedef enum debounce_algorithm_type
{
    Counter,
    Timer,
    Custom
} debounce_algorithm_type_t;


typedef struct { /* <-- from [debounce_counter_based_algorithm] */
    std::string short_name;
    bool counter_based;
    bool debounce_counter_storage;
    std::string debounce_behavior;
    int counter_decrement_step_size;
    int counter_passed_threshold;
    int counter_increment_step_size;
    int counter_failed_threshold;
    int counter_jump_down_value;
    int counter_jump_up_value;
    bool counter_jump_up;
    bool counter_jump_down;
    std::string base;
    int counter_fdc_threshold;
} Counter_t;

typedef struct { /* <-- from [debounce_time_based_algorithm] */
    std::string short_name;
    bool time_based;
    float time_failed_threshold;
    float time_passed_threshold;
    std::string base;
    float time_fdc_threshold;
    std::string debounce_behavior;
} Timer_t;














typedef struct { /* <-- from [debounceCustom] */
    std::string short_name;
    bool monitor_internal;
    std::string base;
} Custom_t;

inline static operation_cycle_type_t s_to_operation_cycle_type(std::string s)
{ throw std::runtime_error("[tiny] to be implemented"); }

inline static freeze_frame_trigger_type_t s_to_freeze_frame_trigger_type(std::string s)
{ throw std::runtime_error("[tiny] to be implemented"); }

inline static extended_data_records_type_t s_to_extended_data_records_type(std::string s)
{ throw std::runtime_error("[tiny] to be implemented"); }

typedef struct { /* <-- from [diagnostic_session] */
    std::string short_name;
    int id;
    std::string jump_to_bootloader;
    float p2_server_max;
    float p2_start_server_max;
    std::string execution_authorization_pattern;
    float p2_star_server_max;
} diagnostic_session_item_t;


typedef struct { /* <-- from [operation_cycle] */
    std::string short_name;
    std::string type;
    std::string target_swc_service_dependency;
    std::string context_sw_component;
    std::string cycle_auto_start;
} operation_cycle_item_t;


typedef struct { /* <-- from [extended_data_records] */
    std::string short_name;
    int record_element_bit_off_set;
    std::string base_type;
    int record_number;
    std::string data_provider;
    std::string trigger;
    bool update;
} extended_data_records_item_t;


typedef struct { /* <-- from [freeze_frames] */
    std::string short_name;
    int record_number;
    std::string trigger;
    bool update;
    std::string custom_trigger;
} freeze_frames_item_t;


typedef struct { /* <-- from [events] */
    std::string failure_name;
    std::string mnemonic;
    std::string event_kind;
    std::string long_name;
    int confirmation_threshold;
    std::string operation_cycle;
    std::string debounce_algorithm;
    typedef struct {
        std::vector<int> or_;
    } data_enable_condition_t;
    data_enable_condition_t data_enable_condition;
    int id;
    typedef struct {
        std::vector<std::string> or_;
    } origin_data_enable_condition_t;
    origin_data_enable_condition_t origin_data_enable_condition;
} events_item_t;


typedef struct { /* <-- from [dtc_all] */
    typedef struct {
        int code;
        int fault_type;
        std::string description;
        std::string device_name;
        int priority;
        std::vector<std::string> extended_data_records;
        int origin_code;
    } identification_t;
    identification_t identification;
    typedef struct {
        std::string snapshot_record_content;
        std::vector<std::string> freeze_frames;
    } snapshots_t;
    snapshots_t snapshots;
    std::string feature;
    std::string functional_specification;
    std::string functional_requirement;
    std::vector<int> events;
} dtc_all_item_t;


typedef struct {
    float s3_server_max;
    bool ignore_request_for_hardreset;
    std::string occurrence_counter_processing;
    int security_delay_time_on_boot;
    int max_number_of_request_correctly_received_response_pending;
    bool response_on_second_declined_request;
    std::string status_bit_handling_test_failed_since_last_clear;
    std::string environment_data_capture;
    int max_number_of_event_entries;
    std::string event_displacement_strategy;
    std::string clear_dtc_limitation;
    std::string default_endianness;
    bool response_on_all_requests_ids;
    bool aging_requires_tested_cycle;
    bool status_bit_storage_test_failed;
    std::string type_of_dtc_supported;
    bool reset_confirmed_bit_on_overflow;
    int dtc_status_availability_mask;
    std::string memory_entry_storage_trigger;
} common_props_t;

static inline void diag_config_init(const char * cfg_path)
{ throw std::runtime_error("[tiny] to be implemented"); }

static inline bool get_init_status(void)
{ throw std::runtime_error("[tiny] to be implemented"); }

#define NEED_INITED()


template <typename T>
Node & match_item
(
    Node & item_container,
    const std::string & field_name,
    T & expected_value
)
{
    throw std::runtime_error("[tiny] to be implemented");
}

static inline Node & get_root_node(void)
{ throw std::runtime_error("[tiny] to be implemented"); }
static inline Node & get_dtc_node(uint32_t dtc_code)
{ throw std::runtime_error("[tiny] to be implemented"); }
static inline Node & get_dtc_node(uint16_t event_id)
{ throw std::runtime_error("[tiny] to be implemented"); }
static inline Node & get_event_node(uint16_t event_id)
{ throw std::runtime_error("[tiny] to be implemented"); }

static inline std::map<uint16_t, std::shared_ptr<Node>> get_event_nodes(uint32_t dtc_code)
{ throw std::runtime_error("[tiny] to be implemented"); }
static inline std::vector<uint16_t> get_event_ids(uint32_t dtc_code)
{ throw std::runtime_error("[tiny] to be implemented"); }
static inline std::map<uint32_t, std::shared_ptr<Node>> get_dtc_nodes(void)
{ throw std::runtime_error("[tiny] to be implemented"); }
static inline std::vector<uint32_t> get_dtc_codes(void)
{ throw std::runtime_error("[tiny] to be implemented"); }
static inline size_t get_did_value_size(uint16_t did_code)
{ throw std::runtime_error("[tiny] to be implemented"); }
static inline std::map<std::string, uint8_t> get_diagnostic_session_map(void)
{ throw std::runtime_error("[tiny] to be implemented"); }
static inline uint8_t get_nrc_by_condition_id(uint8_t cond_id)
{ throw std::runtime_error("[tiny] to be implemented"); }

template <typename T>
static inline void fill_list(Node & node, std::vector<T> & to_be_filled)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
Node & top_debounce_algorithm(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
void top_debounce_algorithm(std::string field_name, T expected_value, Counter_t* to_be_filled)
{
    throw std::runtime_error("[tiny] to be implemented");
}

template <typename T = std::string>
void top_debounce_algorithm(std::string field_name, T expected_value, Timer_t* to_be_filled)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
void top_debounce_algorithm(std::string field_name, T expected_value, Custom_t* to_be_filled)
{
    throw std::runtime_error("[tiny] to be implemented");
}



template <typename T = std::string>
Node & top_dtc_all(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
void top_dtc_all(std::string field_name, T expected_value, dtc_all_item_t* to_be_filled)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
Node & top_events(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
void top_events(std::string field_name, T expected_value, events_item_t* to_be_filled)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
Node & top_freeze_frames(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
void top_freeze_frames(std::string field_name, T expected_value, freeze_frames_item_t* to_be_filled)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
Node & top_diagnostic_session(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}

template <typename T = std::string>
void top_diagnostic_session(std::string field_name, T expected_value, diagnostic_session_item_t* to_be_filled)
{
    throw std::runtime_error("[tiny] to be implemented");
}

template <typename T = std::string>
Node & top_routines_all(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}

template <typename T = std::string>
Node & top_diagnostic_session_security_level(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}

template <typename T = std::string>
Node & top_routine_parameters_all(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}

template <typename T = std::string>
Node & top_datas(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}

template <typename T = std::string>
Node & top_reset_all(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}

template <typename T = std::string>
Node & top_IO_all(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}

template <typename T = std::string>
Node & top_did_all(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}



template <typename T = std::string>
Node & top_operation_cycle(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
void top_operation_cycle(std::string field_name, T expected_value, operation_cycle_item_t* to_be_filled)
{
    throw std::runtime_error("[tiny] to be implemented");
}



template <typename T = std::string>
Node & top_extended_data_records(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
void top_extended_data_records(std::string field_name, T expected_value, extended_data_records_item_t* to_be_filled)
{
    throw std::runtime_error("[tiny] to be implemented");
}

}
}
}

#undef EXPORT_SYM

#endif /* __CONFIGURATION_HPP__ */
