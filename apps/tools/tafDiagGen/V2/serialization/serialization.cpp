/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <fstream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/serialization/map.hpp>

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <map> // std::map is a Red-Black Tree
#include "serialization.hpp"

using boost::property_tree::ptree;

void save_tree(DiagConf& tree, const std::string& filename) {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        throw std::runtime_error("Failed to open output file: " + filename);
    }
    try {
        boost::archive::text_oarchive oa(ofs);
        oa << tree;
    } catch (const boost::archive::archive_exception& e) {
        throw std::runtime_error("Failed to serialize data: " + std::string(e.what()));
    }
}

void serialize_didAll(std::map<int, DidEntry>& didAll, ptree& root)
{
    ptree did_all = root.get_child("did_all");

    for (const auto& did_pair : did_all) {
        int did_code;
        try {
            did_code = std::stoi(did_pair.first);
        } catch (const std::exception& e) {
            std::cerr << "Invalid DID code: " << did_pair.first << std::endl;
            continue;
        }
        const ptree& did_data = did_pair.second;

        DidEntry entry;
        entry.identification.code = did_data.get<int>("identification.code", 0);

        const ptree& impl = did_data.get_child("implementation");
        entry.implementation.did_size = impl.get<int>("did_size", 0);
        entry.implementation.endianness = impl.get<std::string>("endianness", "");

        if (auto bytes_node = impl.get_child_optional("bytes")) {
            for (const auto& byte : *bytes_node) {
                int byte_index = std::stoi(byte.first);
                for (const auto& bit : byte.second) {
                    int bit_index = std::stoi(bit.first);
                    ByteBit bb;
                    bb.data_ref_mnemonic = bit.second.get<std::string>("data_ref_mnemonic", "");
                    entry.implementation.bytes[byte_index][bit_index] = bb;
                }
            }
        }

        const ptree& sf = did_data.get_child("supported_functions");
        entry.supported_functions.write_did = sf.get<bool>("write_did", false);
        entry.supported_functions.read_did = sf.get<bool>("read_did", false);
        entry.supported_functions.snapshot = sf.get<bool>("snapshot", false);
        entry.supported_functions.io_control = sf.get<bool>("io_control", false);
        entry.supported_functions.routine_did = sf.get<bool>("routine_did", false);

        if (auto io_desc = sf.get_child_optional("io_control_description")) {
            entry.supported_functions.io_control_description.return_control_to_ecu =
                io_desc->get<bool>("return_control_to_ecu", false);
            entry.supported_functions.io_control_description.reset_to_default =
                io_desc->get<bool>("reset_to_default", false);
            entry.supported_functions.io_control_description.freeze_current_state =
                io_desc->get<bool>("freeze_current_state", false);
            entry.supported_functions.io_control_description.short_term_adjustment =
                io_desc->get<bool>("short_term_adjustment", false);
        }

        if (auto acc = did_data.
            get_child_optional("did_accessibility.diagnostic_session_and_security_level")) {
            entry.did_accessibility.diagnostic_session_and_security_level.
                execution_authorization_pattern_read =
                acc->get<std::string>("execution_authorization_pattern_read", "");
            entry.did_accessibility.diagnostic_session_and_security_level.
                execution_authorization_pattern_io =
                    acc->get<std::string>("execution_authorization_pattern_io", "");
            if (auto auth_io_node = acc->get_child_optional("execution_authentication_pattern_io"))
            {
                for (const auto& role : *auth_io_node) {
                    entry.did_accessibility.diagnostic_session_and_security_level.
                        execution_authentication_pattern_io.push_back(
                        role.second.get_value<std::string>());
                }
            }
        }

        if (auto diag_sessions = did_data.
            get_child_optional("did_accessibility.diagnostic_session")) {
            for (const auto& session : *diag_sessions) {
                const std::string& session_name = session.first;
                for (const auto& access_type : session.second) {
                    const std::string& access = access_type.first;
                    SecurityLevel sec;
                    sec.security = access_type.second.get<bool>("security", false);
                    for (const auto& level : access_type.second.get_child("security_level",
                        ptree{})) {
                        sec.security_level.push_back(level.second.get_value<std::string>());
                    }
                    if (access == "R")
                        entry.did_accessibility.diagnostic_session[session_name].R[access] = sec;
                    else if (access == "W")
                        entry.did_accessibility.diagnostic_session[session_name].W[access] = sec;
                    else if (access == "IO")
                        entry.did_accessibility.diagnostic_session[session_name].IO[access] = sec;
                }
            }
        }

        if (auto io_role_node = did_data.get_child_optional("io_role")) {
            for (const auto& role : *io_role_node) {
                entry.io_role.push_back(role.second.get_value<std::string>());
            }
        }

        didAll[did_code] = entry;
    }
}

void serialize_servicesAll(std::map<std::string, ServiceEntry>& servicesAll, ptree& root)
{
    ptree services = root.get_child("services_all");

    for (const auto& service : services) {
        const std::string& service_id = service.first;
        const ptree& service_data = service.second;

        ServiceEntry serviceEntry;
        serviceEntry.supported = service_data.get<bool>("supported", false);
        serviceEntry.execution_authorization_pattern =
            service_data.get<std::string>("execution_authorization_pattern", "");

        if (auto access_node = service_data.get_child_optional("access.session")) {
            for (const auto& s : *access_node) {
                serviceEntry.access.session.push_back(s.second.get_value<std::string>());
            }
        }

        if (auto sub_node = service_data.get_child_optional("sub_functions")) {
            for (const auto& sub : *sub_node) {
                const std::string& sub_id = sub.first;
                const ptree& sub_data = sub.second;

                SubFunction sub_func;
                sub_func.supported = sub_data.get<bool>("supported", false);
                sub_func.execution_authorization_pattern =
                    sub_data.get<std::string>("execution_authorization_pattern", "");

                if (auto sub_access = sub_data.get_child_optional("access.session")) {
                    for (const auto& s : *sub_access) {
                        sub_func.access.session.push_back(s.second.get_value<std::string>());
                    }
                }

                serviceEntry.sub_functions[sub_id] = sub_func;
            }
        }

        servicesAll[service_id] = serviceEntry;
    }
}

void serialize_freezeFrame(std::map<std::string, FreezeFrameEntry>& freezeFrames, ptree& root)
{
    // Parse freeze_frames
    ptree frames = root.get_child("freeze_frames");
    for (const auto& frame : frames) {
        const std::string& frame_id = frame.first;
        const ptree& frame_data = frame.second;

        FreezeFrameEntry ffE;
        ffE.short_name = frame_data.get<std::string>("short_name", "");
        ffE.record_number = frame_data.get<int>("record_number", 0);
        ffE.trigger = frame_data.get<std::string>("trigger", "");
        ffE.update = frame_data.get<bool>("update", false);
        ffE.custom_trigger = frame_data.get<std::string>("custom_trigger", "");
        ffE.origin_short_name = frame_data.get<std::string>("origin_short_name", "");

        freezeFrames[frame_id] = ffE;
    }
}

void serialize_dataIdSet(std::map<std::string, DataIdSetEntry>& dataIdSet, ptree& root)
{
    ptree dataIds = root.get_child("data_identifier_set");
    for (const auto& pair : dataIds) {
        const std::string& setName = pair.first;
        const ptree& dataIdList = pair.second;

        DataIdSetEntry dataIdSetEntry;

        for (const auto& item : dataIdList) {
            dataIdSetEntry.dataId.push_back(item.second.get_value<int>());
        }

        dataIdSet[setName] = dataIdSetEntry;
    }
}

void serialize_datas(std::map<std::string, DatasEntry>& datas, ptree& root)
{
    ptree datas_node = root.get_child("datas");

    for (const auto& item : datas_node) {
        const std::string& key = item.first;
        const ptree& entry = item.second;

        DatasEntry data_entry;
        data_entry.mnemonic = entry.get<std::string>("mnemonic", "");

        const ptree& fd = entry.get_child("functional_definition");

        FunctionalDefinition& def = data_entry.functional_definition;
        def.data_type = fd.get<std::string>("data_type", "");
        def.data_type_encoding = fd.get<std::string>("data_type_encoding", "");
        def.value_type = fd.get<std::string>("value_type", "");
        def.bit_size = fd.get<int>("bit_size", 0);
        def.default_value = fd.get<int>("default_value", 0);

        // 处理 forbidden_values（以字符串形式存储，避免超大整数溢出）
        if (auto fv = fd.get_child_optional("forbidden_values")) {
            for (const auto& val : *fv) {
                def.forbidden_values.push_back(val.second.get_value<std::string>());
            }
        }

        datas[key] = data_entry;
    }
}

void serialize_auth_anti_conf(std::map<std::string, AuthAntiConfEntry>& authAntiConf, ptree& root)
{
    ptree auth_anti_node = root.get_child("authentication_antibruteforce");

    for (const auto& item : auth_anti_node) {
        const std::string& key = item.first;
        const ptree& entry = item.second;

        AuthAntiConfEntry auth_anti_entry;
        auth_anti_entry.antiBruteForceCounterMaxValue =
            entry.get<int>("AntiBruteForceCounterMaxValue", 0);
        auth_anti_entry.delayTimerInvokingValueInit =
            entry.get<int>("DelayTimerInvokingValueInit", 0);
        auth_anti_entry.delayTimerInvokingValueMax =
            entry.get<int>("DelayTimerInvokingValueMax", 0);

        authAntiConf[key] = auth_anti_entry;
    }
}

void serialize_session_secur_lvl(
    std::map<std::string,
    SessionSecurLvlEntry>& sessionSecurLvl,
    ptree& root
)
{
    ptree session_secur_lvl_node = root.get_child("diagnostic_session_security_level");

    for (const auto& item : session_secur_lvl_node) {
        const std::string& key = item.first;
        const ptree& entry = item.second;

        SessionSecurLvlEntry session_secur_lvl_entry;
        session_secur_lvl_entry.short_name = entry.get<std::string>("short_name", "");
        session_secur_lvl_entry.request_seed_id = entry.get<int>("request_seed_id", 0);
        session_secur_lvl_entry.key_size = entry.get<int>("key_size", 0);
        session_secur_lvl_entry.num_failed_security_access =
            entry.get<int>("num_failed_security_access", 0);
        session_secur_lvl_entry.security_delay_time = entry.get<int>("security_delay_time", 0);
        session_secur_lvl_entry.seed_size = entry.get<int>("seed_size", 0);
        session_secur_lvl_entry.execution_authorization_pattern =
            entry.get<std::string>("execution_authorization_pattern", "");
        session_secur_lvl_entry.static_seed = entry.get<bool>("static_seed", 0);
        session_secur_lvl_entry.level_id = entry.get<int>("level_id", 0);

        sessionSecurLvl[key] = session_secur_lvl_entry;
    }
}

int main(int argc, char* argv[]) {


    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <output_directory>" << std::endl;
        return 1;
    }

    std::string path = argv[1];
    if (path.back() != '/' && path.back() != '\\') {
        path += '/';
    }

    std::string inPutFileName = path + SERIALIZATION_INPUT_JSON;
    std::string outPutFileName = path + SERIALIZATION_OUTPUT_FILE;


    ptree root;
    try {
        read_json(inPutFileName, root);
    } catch (const boost::property_tree::json_parser_error& e) {
        std::cerr << "Failed to parse JSON file: " << inPutFileName << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    DiagConf diagConf;

    serialize_didAll(diagConf.did_all, root);
    serialize_servicesAll(diagConf.services_all, root);
    serialize_freezeFrame(diagConf.freeze_frames, root);
    serialize_dataIdSet(diagConf.dataid_set, root);
    serialize_datas(diagConf.datas, root);
    //serialize_auth_anti_conf(diagConf.auth_anti_conf, root);
    serialize_session_secur_lvl(diagConf.session_secur_level, root);

    save_tree(diagConf, outPutFileName);

    std::cout <<"Save tree successfully"<< std::endl;

    return 0;
}
