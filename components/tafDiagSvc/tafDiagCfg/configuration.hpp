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
    WUC,
    Custom1
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

typedef enum storage_condition_type
{
    storage_condition_1,
    storage_condition_2
} storage_condition_type_t;

typedef enum fim_fid_type
{
    fim_fid_dma_accelpos_accel_pdl_grd_lim,
    fim_fid_dma_accelpos_alg_dgn_act,
    fim_fid_dma_accelpos_dbl_fail
} fim_fid_type_t;

typedef enum freeze_frames_type
{
    firstOccurrence,
    lastOccurrence,
    lastDisappearance
} freeze_frames_type_t;

typedef enum enable_condition_type
{
    Enable_1,
    Enable_2
} enable_condition_type_t;

typedef enum indicator_type
{
    G1,
    MIL,
    G2
} indicator_type_t;

typedef enum connected_indicator_behavior_type
{
    BLINK_MODE,
    BLINK_OR_CONTINUOUS_MODE_ON,
    CONTINUOUS_MODE_ON,
    FAST_FLASHING_MODE,
    SLOW_FLASHING_MODE
} connected_indicator_behavior_type_t;

typedef enum extended_data_records_type
{
    OccurenceCounter,
    CumulativeDistanceOCCWithTestFailed,
    IUMPRNumerator,
    IUMPRDenominator
} extended_data_records_type_t;

typedef enum debounce_algorithm_type
{
    Counter,
    Timer,
    Custom
} debounce_algorithm_type_t;


typedef struct { /* <-- from [debounce_counter_based_algorithm] */
    std::string short_name;
    std::string base;
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
    int counter_fdc_threshold;
} Counter_t;

typedef struct { /* <-- from [debounce_time_based_algorithm] */
    std::string short_name;
    std::string base;
    std::string debounce_behavior;
    float time_failed_threshold;
    float time_passed_threshold;
    float time_fdc_threshold;
} Timer_t;

typedef struct { /* <-- from [debounce_monitor_internal_algorithm] */
    std::string short_name;
    std::string base;
    bool monitor_internal;
} Custom_t;

inline static operation_cycle_type_t s_to_operation_cycle_type(std::string s)
{ throw std::runtime_error("[tiny] to be implemented"); }

inline static enable_condition_type_t s_to_enable_condition_type(std::string s)
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
    int p2_star_server_max;
} diagnostic_session_item_t;


typedef struct { /* <-- from [storage_condition] */
    std::string short_name;
    bool target_swc_service_dependency;
    std::string context_sw_component;
} storage_condition_item_t;


typedef struct { /* <-- from [enable_condition] */
    std::string short_name;
    bool target_swc_service_dependency;
    std::string context_sw_component;
} enable_condition_item_t;


typedef struct { /* <-- from [operation_cycle] */
    std::string short_name;
    std::string type;
    std::string target_swc_service_dependency;
    std::string context_sw_component;
    std::string cycle_auto_start;
} operation_cycle_item_t;


typedef struct { /* <-- from [indicator] */
    std::string short_name;
    std::string type;
} indicator_item_t;


typedef struct { /* <-- from [connected_indicator_behavior] */
    std::string indicator_mnemonic;
    std::string conditions;
} connected_indicator_behavior_item_t;


typedef struct { /* <-- from [extended_data_records] */
    std::string short_name;
    int record_element_bit_off_set;
    std::string base_type;
    int record_number;
    std::string data_provider;
    std::string trigger;
    bool update;
} extended_data_records_item_t;


typedef struct { /* <-- from [fim_all] */
    std::string diagnostic_function_identifier;
    std::string context_sw_component;
} fim_all_item_t;


typedef struct { /* <-- from [freeze_frames] */
    std::string short_name;
    int record_number;
    std::string trigger;
    bool update;
    std::string custom_trigger;
} freeze_frames_item_t;


typedef struct { /* <-- from [events] */
    int id;
    std::string event_kind;
    std::string long_name;
    std::string failure_name;
    int confirmation_threshold;
    std::string operation_cycle;
    typedef struct {
        std::vector<std::string> indicator;
        std::vector<std::string> behavior;
        int indicator_failure_cycle_counter_threshold;
        int healing_cycle_counter_threshold;
        std::string healing_cycle;
    } connected_indicator_t;
    connected_indicator_t connected_indicator;
    std::string debounce_algorithm;
    std::string enable_condition;
    std::string storage_condition;
} events_item_t;


typedef struct { /* <-- from [dtc_all] */
    typedef struct {
        int code;
        int priority;
        int fault_type;
        std::string description;
        std::vector<std::string> extended_data_records;
        int dtc_group_number;
    } identification_t;
    identification_t identification;
    typedef struct {
        std::string snapshot_record_content;
        std::vector<std::string> freeze_frames;
    } snapshots_t;
    snapshots_t snapshots;
    typedef struct {
        std::vector<std::string> session;
        int security_level;
    } access_t;
    access_t access;
    std::vector<int> events;
} dtc_all_item_t;


typedef struct {
    int max_number_of_event_entries;
    std::string memory_entry_storage_trigger;
    bool aging_requires_tested_cycle;
    std::string clear_dtc_limitation;
    std::string default_endianness;
    std::string environment_data_capture;
    std::string event_displacement_strategy;
    int max_number_of_request_correctly_received_response_pending;
    std::string occurrence_counter_processing;
    bool response_on_all_requests_ids;
    bool response_on_second_declined_request;
    std::string status_bit_handling_test_failed_since_last_clear;
    bool reset_confirmed_bit_on_overflow;
    bool status_bit_storage_test_failed;
    std::string type_of_dtc_supported;
    int security_delay_time_on_boot;
    float s3_server_max;
    bool ignore_request_for_hardreset;
    int dtc_status_availability_mask;
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
Node & top_fim_all(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
void top_fim_all(std::string field_name, T expected_value, fim_all_item_t* to_be_filled)
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
Node & top_security_level(std::string field_name, T expected_value)
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
Node & top_storage_condition(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
void top_storage_condition(std::string field_name, T expected_value, storage_condition_item_t* to_be_filled)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
Node & top_enable_condition(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
void top_enable_condition(std::string field_name, T expected_value, enable_condition_item_t* to_be_filled)
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
Node & top_indicator(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
void top_indicator(std::string field_name, T expected_value, indicator_item_t* to_be_filled)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
Node & top_connected_indicator_behavior(std::string field_name, T expected_value)
{
    throw std::runtime_error("[tiny] to be implemented");
}


template <typename T = std::string>
void top_connected_indicator_behavior(std::string field_name, T expected_value, connected_indicator_behavior_item_t* to_be_filled)
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
