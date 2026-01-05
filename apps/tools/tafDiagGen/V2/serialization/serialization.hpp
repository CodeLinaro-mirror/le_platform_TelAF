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

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & session;
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
    std::string execution_authorization_pattern;
    Access access;
    std::map<std::string, SubFunction> sub_functions;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & supported;
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
    std::string execution_authorization_pattern_io;
    std::vector<std::string> execution_authentication_pattern_io;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & execution_authorization_pattern_read;
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

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & identification;
        ar & implementation;
        ar & supported_functions;
        ar & did_accessibility;
        ar & io_role;
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

struct DiagConf {
    std::map<int, DidEntry> did_all;
    std::map<std::string, ServiceEntry> services_all;
    std::map<std::string, FreezeFrameEntry> freeze_frames;
    std::map<std::string, DataIdSetEntry> dataid_set;
    std::map<std::string, DatasEntry> datas;
    std::map<std::string, AuthAntiConfEntry> auth_anti_conf;
    std::map<std::string, SessionSecurLvlEntry> session_secur_level;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & did_all;
        ar & services_all;
        ar & freeze_frames;
        ar & dataid_set;
        ar & datas;
        ar & auth_anti_conf;
        ar & session_secur_level;
    }
};




#endif
