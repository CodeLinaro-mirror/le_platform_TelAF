/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SERIALIZATION_HPP__
#define __SERIALIZATION_HPP__

#include <string>
#include <vector>
#include <map>
#include <boost/serialization/access.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#define SERIALIZATION_INPUT_JSON "diag_template.yaml.json"
#define SERIALIZATION_OUTPUT_FILE "tree_data"
inline std::string tree_data_md5 = "";

// Structures for freeze_frames
struct FreezeFrameEntry {
    std::string short_name;
    int record_number;
    std::string trigger;
    bool update;
    std::string custom_trigger;
    std::string origin_short_name;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & short_name;
        ar & record_number;
        ar & trigger;
        ar & update;
        ar & custom_trigger;
        ar & origin_short_name;
    }
};

struct Access {
    std::vector<std::string> session;
    uint8_t security_type = 0;
    std::vector<std::string> security_level;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & session;
        ar & security_type;
        ar & security_level;
    }
};

struct SubFunction {
    bool supported = false;
    std::string execution_authorization_pattern;
    Access access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & supported;
        ar & execution_authorization_pattern;
        ar & access;
    }
};

struct ServiceEntry {
    bool supported = false;
    bool authentication = false;
    std::string execution_authorization_pattern;
    Access access;
    std::map<std::string, SubFunction> sub_functions;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & supported;
        ar & authentication;
        ar & execution_authorization_pattern;
        ar & access;
        ar & sub_functions;
    }
};

struct IoControlDescription {
    bool return_control_to_ecu = false;
    bool reset_to_default = false;
    bool freeze_current_state = false;
    bool short_term_adjustment = false;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & return_control_to_ecu;
        ar & reset_to_default;
        ar & freeze_current_state;
        ar & short_term_adjustment;
    }
};

struct SupportedFunctions {
    bool write_did = false;
    bool read_did = false;
    bool snapshot = false;
    bool io_control = false;
    bool routine_did = false;
    IoControlDescription io_control_description;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & write_did;
        ar & read_did;
        ar & snapshot;
        ar & io_control;
        ar & routine_did;
        ar & io_control_description;
    }
};

struct ByteBit {
    std::string data_ref_mnemonic;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & data_ref_mnemonic;
    }
};

struct Implementation {
    int did_size = 0;
    std::string endianness;
    std::map<int, std::map<int, ByteBit>> bytes;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & did_size;
        ar & endianness;
        ar & bytes;
    }
};

struct Identification {
    int code = 0;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & code;
    }
};

struct SecurityLevel {
    bool security = false;
    std::vector<std::string> security_level;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & security;
        ar & security_level;
    }
};

struct DiagnosticSession {
    std::map<std::string, SecurityLevel> R;
    std::map<std::string, SecurityLevel> W;
    std::map<std::string, SecurityLevel> IO;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & R;
        ar & W;
        ar & IO;
    }
};

struct DiagnosticSessionAndSecurityLevel {
    std::string execution_authorization_pattern_read;
    std::string execution_authorization_pattern_write;
    std::string execution_authorization_pattern_io;
    std::vector<std::string> execution_authentication_pattern_io;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & execution_authorization_pattern_read;
        ar & execution_authorization_pattern_write;
        ar & execution_authorization_pattern_io;
        ar & execution_authentication_pattern_io;
    }
};

struct DidAccessibility {
    DiagnosticSessionAndSecurityLevel diagnostic_session_and_security_level;
    std::map<std::string, DiagnosticSession> diagnostic_session;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & diagnostic_session_and_security_level;
        ar & diagnostic_session;
    }
};

struct DidEntry {
    Identification identification;
    Implementation implementation;
    SupportedFunctions supported_functions;
    DidAccessibility did_accessibility;
    std::vector<std::string> io_role;
    std::vector<std::string> read_role;
    std::vector<std::string> write_role;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & identification;
        ar & implementation;
        ar & supported_functions;
        ar & did_accessibility;
        ar & io_role;
        ar & read_role;
        ar & write_role;
    }
};

struct DataIdSetEntry {
    std::vector<int> dataId;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & dataId;
    }
};

struct FunctionalDefinition {
    std::string data_type;
    std::string data_type_encoding;
    std::string value_type;
    int bit_size;
    int default_value;
    std::vector<std::string> forbidden_values;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & data_type;
        ar & data_type_encoding;
        ar & value_type;
        ar & bit_size;
        ar & default_value;
        ar & forbidden_values;
    }

};

struct DatasEntry {
    std::string mnemonic;
    FunctionalDefinition functional_definition;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & mnemonic;
        ar & functional_definition;
    }
};

struct AuthAntiConfEntry {
    int antiBruteForceCounterMaxValue;
    int delayTimerInvokingValueInit;
    int delayTimerInvokingValueMax;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & antiBruteForceCounterMaxValue;
        ar & delayTimerInvokingValueInit;
        ar & delayTimerInvokingValueMax;
    }
};

struct SessionSecurLvlEntry {
    std::string short_name;
    int request_seed_id;
    int key_size;
    int num_failed_security_access;
    int security_delay_time;
    int seed_size;
    std::string execution_authorization_pattern;
    bool static_seed;
    int level_id;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & short_name;
        ar & request_seed_id;
        ar & key_size;
        ar & num_failed_security_access;
        ar & security_delay_time;
        ar & seed_size;
        ar & execution_authorization_pattern;
        ar & static_seed;
        ar & level_id;
    }
};

struct CommonProps {
    int max_number_of_rcrrp;
    std::string occurrence_counter_processing;
    double s3_server_max;
    bool ignore_request_for_hardreset;
    int dtc_status_availability_mask;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & max_number_of_rcrrp;
        ar & occurrence_counter_processing;
        ar & s3_server_max;
        ar & ignore_request_for_hardreset;
        ar & dtc_status_availability_mask;
    }
};

struct ExtendedDataRecordEntry {
    std::string short_name;
    int record_element_bit_off_set;
    std::string base_type;
    int record_number;
    std::string data_provider;
    std::string trigger;
    bool update;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & short_name;
        ar & record_element_bit_off_set;
        ar & base_type;
        ar & record_number;
        ar & data_provider;
        ar & trigger;
        ar & update;
    }
};

struct ExecAuthPatternEntry {
    std::string class_;
    std::string short_name;
    bool base;
    bool did_read_app;
    bool secured_configuration;
    bool did_write;
    bool io_control;
    bool routine_control;
    bool expert_read;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & class_;
        ar & short_name;
        ar & base;
        ar & did_read_app;
        ar & secured_configuration;
        ar & did_write;
        ar & io_control;
        ar & routine_control;
        ar & expert_read;
    }
};

struct DiagSessionEntry {
    std::string short_name;
    int id;
    double p2_server_max;
    double p2_start_server_max;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & short_name;
        ar & id;
        ar & p2_server_max;
        ar & p2_start_server_max;
    }
};

struct DebounceCounterBasedAlgorithm {
    std::string short_name;
    std::string base;
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

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & short_name;
        ar & base;
        ar & debounce_behavior;
        ar & counter_decrement_step_size;
        ar & counter_passed_threshold;
        ar & counter_increment_step_size;
        ar & counter_failed_threshold;
         ar & counter_jump_down_value;
         ar & counter_jump_up_value;//issue
        ar & counter_jump_up;
        ar & counter_jump_down;
        ar & counter_fdc_threshold; //no issue
    }

};

struct DebounceTimeBasedAlgorithm {
    std::string short_name;
    double time_failed_threshold;
    double time_passed_threshold;
    std::string base;
    double time_fdc_threshold;
    std::string debounce_behavior;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & short_name;
        ar & time_failed_threshold;
        ar & time_passed_threshold;
        ar & base;
        ar & time_fdc_threshold;
        ar & debounce_behavior;
    }

};

struct DebounceCustom {
    std::string short_name;
    std::string base;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & short_name;
        ar & base;
    }
};

struct DebounceAlgorithm {
    std::map<std::string, DebounceCounterBasedAlgorithm> counter_based;
    std::map<std::string, DebounceTimeBasedAlgorithm>    time_based;
    std::map<std::string, DebounceCustom>                custom;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & counter_based;
        ar & time_based;
        ar & custom;
    }
};

struct RoutineRequest {
    std::vector<int> sub_function;
    std::string start;
    std::string stop;
    std::string result;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & sub_function;
        ar & start;
        ar & stop;
        ar & result;
    }
};

struct RoutineEntry {
    int identifier;
    RoutineRequest request;
    Access access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & identifier;
        ar & request;
        ar & access;
    }
};

struct AuthRoleEntry {
    std::string name;
    int value;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & name;
        ar & value;
    }
};

struct SecurBindingEntry {
    int session_id;
    std::map<std::string, SessionSecurLvlEntry> security_level;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & session_id;
        ar & security_level;
    }
};

// Define helper struct
struct EnableConditionData {
    std::vector<uint8_t> and_conditions;
    std::vector<uint8_t> or_conditions;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & and_conditions;
        ar & or_conditions;
    }
};

struct EventEntry {
    int id;
    int confirmation_threshold;
    std::string operation_cycle;
    std::string debounce_algorithm;
    EnableConditionData enable_condition;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & id;
        ar & confirmation_threshold;
        ar & operation_cycle;
        ar & debounce_algorithm;
        ar & enable_condition;
    }
};

struct DTCIdentification {
    int code;
    int fault_type;
    std::vector<std::string> extended_data_records;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & code;
        ar & fault_type;
        ar & extended_data_records;
    }
};

struct DTCSnapshots {
    std::string snapshot_record_content;
    std::vector<std::string> freeze_frames;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & snapshot_record_content;
        ar & freeze_frames;
    }
};

struct DTCEntry {
    DTCIdentification identification;
    DTCSnapshots snapshots;
    Access access;
    std::vector<int> events;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & identification;
        ar & snapshots;
        ar & access;
        ar & events;
    }
};

struct ResetEntry {
    int sub_function_identifier;
    Access access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & sub_function_identifier;
        ar & access;
    }
};

struct IOSession {
    SecurityLevel IO;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & IO;
    }
};

struct IOControlOptionRecord {
    std::vector<int> io_control_parameter;
    int did_size;
    std::string control_state;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & io_control_parameter;
        ar & did_size;
        ar & control_state;
    }
};

struct IORequest {
    IOControlOptionRecord control_option_record;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & control_option_record;
    }
};

struct IOEntry {
    int identifier;
    IORequest request;
    std::map<std::string, SecurityLevel> diagnostic_session;
    Access access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & identifier;
        ar & request;
        ar & diagnostic_session;
        ar & access;
    }
};

struct DiagConf {
    std::map<int, DidEntry> did_all;
    std::map<std::string, ServiceEntry> services_all;
    std::map<std::string, FreezeFrameEntry> freeze_frames;
    std::map<std::string, DataIdSetEntry> dataid_set;
    std::map<std::string, DatasEntry> datas;
    std::map<std::string, AuthAntiConfEntry> auth_anti_conf;
    std::map<std::string, SessionSecurLvlEntry> session_secur_level;
    CommonProps common_props;
    std::map<std::string, ExtendedDataRecordEntry> extended_data_records;
    std::map<std::string, DiagSessionEntry> diag_session;
    DebounceAlgorithm debounce_algorithm;
    std::map<std::string, RoutineEntry> routines_all;
    std::map<std::string, AuthRoleEntry> auth_roles;
    std::map<std::string, ExecAuthPatternEntry> exec_auth_pattern;
    std::map<std::string, SecurBindingEntry> secur_binding;
    std::map<int, EventEntry> events;
    std::map<int, DTCEntry> dtc_all;
    std::map<int, ResetEntry> reset_all;
    std::map<int, IOEntry> io_all;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & did_all;
        ar & services_all;
        ar & freeze_frames;
        ar & dataid_set;
        ar & datas;
        ar & auth_anti_conf;
        ar & session_secur_level;
        ar & common_props;
        ar & extended_data_records;
        ar & diag_session;
        ar & debounce_algorithm;
        ar & routines_all;
        ar & auth_roles;
        ar & exec_auth_pattern;
        ar & secur_binding;
        ar & events;
        ar & dtc_all;
        ar & reset_all;
        ar & io_all;
    }
};

#endif
