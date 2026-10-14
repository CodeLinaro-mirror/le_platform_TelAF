#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import copy, os, sys
from collections import OrderedDict
import oyaml as yaml
import shutil
from pprint import pprint
from .logger import logger

# Don't support the same session id with different session names
def convert_diagnostic_session(final_yaml):

    duplicate_session = OrderedDict()

    for session_name, session_body in final_yaml['diagnostic_session'].items():
        session_id = session_body['id']

        # Convert p2_server_max & p2_start_server_max to float type
        session_body['p2_server_max'] = float(session_body['p2_server_max'])

        logger.info("Change the typo from 'p2_start_server_max' to 'p2_star_server_max'")
        session_body['p2_star_server_max'] = float(session_body['p2_start_server_max'])

        # Convert the value of 'short_name' to the key-name, and backup the short name
        session_body['origin_short_name'] = session_body['short_name']
        session_body['short_name'] = session_name # key-name -> short_name

        if session_id not in duplicate_session:
            duplicate_session[session_id] = list()
            duplicate_session[session_id].append(session_name)
        else:
            duplicate_session[session_id].append(session_name)

    for session_id, session_name_list in duplicate_session.items():
        if len(session_name_list) > 1:
            logger.error(f"Same session-id ({hex(session_id)}) with different names:")
            for sess_name in session_name_list: logger.error(f"  -> {sess_name}")
            sys.exit()


def add_default_info_to_services_all(final_yaml):
    # Conversion -> services_all
    logger.info("Conversion -> services_all")
    # Mark the absence of 'supported' as false
    # Mark the absence of 'IDPS_supported' as false
    for sid, svc in final_yaml['services_all'].items():
        if 'supported' not in svc.keys(): # Service layer
            svc['supported'] = False
            logger.debug(f"@mark services_all -> {sid} supported = false")
        if 'IDPS_supported' not in svc.keys(): # Service layer
            svc['IDPS_supported'] = False
            logger.debug(f"@mark services_all -> {sid} IDPS_supported = false")
        if 'functional_addressed' not in svc.keys(): # Service layer
            svc['functional_addressed'] = False
            logger.debug(f"@mark services_all -> {sid} functional_addressed = false")
        if 'sub_functions' in svc.keys():
            for sub_id, sub in svc['sub_functions'].items():
                if 'supported' not in sub.keys():
                    sub['supported'] = False
                    logger.debug(f"@mark services_all -> {sid} -> sub_functions -> {sub_id} -> supported = false")


def convert_patterns_in_service_all(final_yaml, final_pattern):
    # execution_authorization_pattern -> For drc_service (only service)
    logger.info("Extend execution_authorization_pattern to services_all")
    for svc_id, svc_item in final_yaml['services_all'].items():
        if 'execution_authorization_pattern' in svc_item.keys():
            logger.debug(f"@convert pattern to 'access' in [{hex(svc_id)}] svc")
            svc_item['access'] = OrderedDict()
            svc_item['access']['session'] = [
                item[0]
                for item in final_pattern[svc_item['execution_authorization_pattern']]['session']
            ] # (name, id)

            # Check if the security is needed for the current service
            if final_pattern[svc_item['execution_authorization_pattern']]['security_level'] and svc_id != 0x27: # Skip security itself
                svc_item['access']['security_type'] = 0x27
                svc_item['access']['security_level'] = [
                    item[0]
                    for item in final_pattern[svc_item['execution_authorization_pattern']]['security_level']
                ] # item <= (name, id)
            else:
                if svc_id == 0x27:
                    logger.info(f"Skip security access service itself!")
                else:
                    logger.debug(f"security_level is empty for pattern [{final_pattern[svc_item['execution_authorization_pattern']]}]")

        # For drc_service (sub-function)
        if 'sub_functions' in svc_item.keys():
            for sub_id, sub_item in svc_item['sub_functions'].items():
                if 'execution_authorization_pattern' in sub_item.keys():
                    logger.debug(f"@convert pattern to 'access' in [{hex(svc_id)}/{hex(sub_id)}] svc/sub")
                    sub_item['access'] = OrderedDict()
                    sub_item['access']['session'] = [
                        item[0]
                        for item in final_pattern[sub_item['execution_authorization_pattern']]['session']
                    ]

                    # Check if the security is needed for the current service subfunction
                    if final_pattern[sub_item['execution_authorization_pattern']]['security_level'] and svc_id != 0x27:
                        sub_item['access']['security_type'] = 0x27
                        sub_item['access']['security_level'] = [
                            item[0]
                            for item in final_pattern[sub_item['execution_authorization_pattern']]['security_level']
                        ]
                    else:
                        if svc_id == 0x27:
                            logger.info(f"Skip security access service subfunction itself!")
                        else:
                            logger.debug(f"security_level is empty for pattern [{final_pattern[sub_item['execution_authorization_pattern']]}]")


# Create the 'security_binding' top_node
def create_security_binding(final_yaml, final_pattern):

    logger.info("Create top-node: security_binding based on diagnostic_session_security_level patterns")
    final_yaml['security_binding'] = OrderedDict()

    sess_exist = [] # All session ids recorded.
    for sess_name, sess_node in final_yaml['diagnostic_session'].items():

        # Security Access Service is not available in the default session
        # We don't create the default-session in security_binding
        if sess_name == "default_session": continue

        # Checked the diagnostic_session already.
        final_yaml['security_binding'][sess_name] = OrderedDict({'session_id': sess_node['id'], 'security_level': []})

    # Construct the 'security_binding' from 'diagnostic_session_security_level' instead of 'drc_service'
    for lvl_name, lvl_item in final_yaml['diagnostic_session_security_level'].items():

        lvl_pattern = final_pattern[lvl_item['execution_authorization_pattern']]

        for (sess_name, sess_id) in lvl_pattern['session']:

            if sess_name == "default_session": continue # skip default session

            assert sess_name in final_yaml['security_binding'].keys(), \
                f"Why new session in execution_authorization_pattern list" + \
                f"(pattern: {lvl_item['execution_authorization_pattern']}) ? [{sess_name}] <--"

            logger.info(f"Bind lvl:{lvl_name} to sess-id:{sess_id}")
            final_yaml['security_binding'][sess_name]['security_level'].append(lvl_name) # key-name


# execution_authorization_pattern -> For drc_data_identifiers (did_all)
def convert_patterns_in_did_all(final_yaml, final_pattern):
    logger.info("Extend executilon_authorization_pattern to did_all")

    # Don't care the 'access' attribute anymore !!

    for did_name, did_node in final_yaml['did_all'].items():
        # Mark the absence of 'functional_addressed' as false (DID layer)
        if 'functional_addressed' not in did_node.keys():
            did_node['functional_addressed'] = False
            logger.debug(f"@mark did_all -> {hex(did_name)} functional_addressed = false")

        if 'did_accessibility' not in did_node.keys():
            logger.warning(f"Not found 'did_accessibility' in DID [{hex(did_name) }]")
            continue

        did_accessibility = did_node['did_accessibility']

        if did_accessibility is None: # Case for 'allow_empty'
            logger.info(f"'did_accessibility' is empty in DID {hex(did_name)}")
            continue

        # In schema checking stage, we can ensure the:
        #   - diagnostic_session_and_security_level
        #   - diagnostic_session
        # at least one of them exists !!

        generated_diagnostic_session = OrderedDict()

        if 'diagnostic_session_and_security_level' in did_accessibility.keys():

            diagnostic_session_and_security_level = did_accessibility['diagnostic_session_and_security_level']

            if 'diagnostic_session' not in did_accessibility.keys(): # Only generate the sercue parts

                logger.info(f"Found 'diagnostic_session_and_security_level' alone -> DID: {hex(did_name)}")

                for pattern_attr_name, pattern in diagnostic_session_and_security_level.items():

                    if type(pattern) is not list:

                        if not final_pattern[pattern]['session']:
                            logger.error(f"Why doest pattern '{pattern}' have the empty-session-list ?")
                            sys.exit(1)

                        for s_name, s_id in final_pattern[pattern]['session']:

                            if s_name not in generated_diagnostic_session.keys():
                                generated_diagnostic_session[s_name] = OrderedDict()
                            else:
                                pass # Already created for this session

                            if pattern_attr_name == "execution_authorization_pattern_read":
                                this_attr = 'R'
                            elif pattern_attr_name == "execution_authorization_pattern_write":
                                this_attr = 'W'
                            elif pattern_attr_name == "execution_authorization_pattern_io":
                                this_attr = 'IO'

                            attr_dict = generated_diagnostic_session[s_name][this_attr] = OrderedDict()

                            if final_pattern[pattern]['security_level']: # non-empty
                                attr_dict['security'] = True
                                attr_dict['security_level'] = [ lvl[0] for lvl in final_pattern[pattern]['security_level'] ] # names
                            else: # level list is empty ? OK, mark it as non-secure
                                attr_dict['security'] = False
                                attr_dict['security_level'] = list()
                    else:
                        pass # Found authentication pattern list skip it

            else: # Both 'diagnostic_session' & 'diagnostic_session_and_security_level'

                logger.debug(f"Found both 'diagnostic_session' & 'diagnostic_session_and_security_level' -> DID: {hex(did_name)}")

                # Backup the original 'diagnostic_session'
                did_accessibility['origin_diagnostic_session'] = did_accessibility['diagnostic_session']

                # Construct the default 'diagnostic_session' without any security info first...
                for sess_name, attr_list in did_accessibility['origin_diagnostic_session'].items():

                    assert sess_name in final_yaml['diagnostic_session'].keys(), \
                        f"{sess_name} <-- not a valid session name"

                    # sess_name can't be duplicated
                    generated_diagnostic_session[sess_name] = OrderedDict()

                    for attr in attr_list:
                        attr_dict = generated_diagnostic_session[sess_name][attr] = OrderedDict()
                        attr_dict['security'] = False
                        attr_dict['security_level'] = list()

                for pattern_attr_name, pattern in diagnostic_session_and_security_level.items():

                    if type(pattern) is not list:

                        for s_name, s_id in final_pattern[pattern]['session']:

                            assert s_name in final_yaml['diagnostic_session'].keys(), \
                                f"{s_name} <-- not a valid session name"

                            if s_name not in generated_diagnostic_session.keys():
                                generated_diagnostic_session[s_name] = OrderedDict()
                            else:
                                pass # Already created for this session

                            if pattern_attr_name == "execution_authorization_pattern_read":
                                this_attr = 'R'
                            elif pattern_attr_name == "execution_authorization_pattern_write":
                                this_attr = 'W'
                            elif pattern_attr_name == "execution_authorization_pattern_io":
                                this_attr = 'IO'

                            if this_attr not in generated_diagnostic_session[s_name].keys():
                                attr_dict = generated_diagnostic_session[s_name][this_attr] = OrderedDict()
                            else:
                                attr_dict = generated_diagnostic_session[s_name][this_attr] # Alread created for this session

                            if final_pattern[pattern]['security_level']: # non-empty
                                attr_dict['security'] = True
                                attr_dict['security_level'] = [ lvl[0] for lvl in final_pattern[pattern]['security_level'] ]
                            else: # Need to cover all cases
                                attr_dict['security'] = False
                                attr_dict['security_level'] = list()

                    else:
                        pass # skip it as authentication pattern list found

        else: # 'diagnostic_session_and_security_level' is not preset, only 'diagnostic_session'
            logger.debug(f"Found 'diagnostic_session' alone -> DID: {hex(did_name)}")

            assert 'diagnostic_session' in did_accessibility.keys()

            for sess_name, attr_list in did_accessibility['diagnostic_session'].items():

                assert sess_name in final_yaml['diagnostic_session'].keys(), \
                    f"{sess_name} <-- not a valid session name"

                this_session = generated_diagnostic_session[sess_name] = OrderedDict()

                # Only generate the existing attributes
                for attr in attr_list:
                    attr_dict = this_session[attr] = OrderedDict()
                    attr_dict['security'] = False # No security checking
                    attr_dict['security_level'] = list()

            # When 'diagnostic_session' exists, backup it
            did_accessibility['origin_diagnostic_session'] = did_accessibility['diagnostic_session']

        # Update the 'diagnostic_session' after the conversion
        did_accessibility['diagnostic_session'] = generated_diagnostic_session


# Conversion -> diagnostic_session_security_level
def convert_diagnostic_session_security_level(final_yaml):
    logger.info("Conversion -> diagnostic_session_security_level")
    for sec_name, sec_node in final_yaml['diagnostic_session_security_level'].items():
        sec_node['static_seed'] = False
        logger.debug(f"add static_seed to {sec_name} (security level)")

        sec_node['level_id'] = sec_node['request_seed_id']
        logger.debug(f"add level_id based on 'request_seed_id'")

        sec_node['origin_short_name'] = sec_node['short_name']
        sec_node['short_name'] = sec_name # key-name -> short_name
        logger.debug(f"replace the short_name with key-name for security_level")

def extend_roles_in_authentication(final_yaml):
    logger.info("Conversion -> extend_roles_in_authentication")

    final_yaml["authentication_timeout_extended"] = OrderedDict()

    for key, value in final_yaml["authentication_timeout"].items():
        bit_position = int(key[1:])     # Extract numeric part from key (e.g., "b0" → 0, "b1" → 1)
        final_yaml["authentication_timeout_extended"][key] = {
            "name": key,
            "value": 1 << bit_position,  # Calculate 2^bit_position
            **value,  # Retain existing attributes
        }
    del final_yaml["authentication_timeout"]

def extend_role_to_did_all(final_yaml):
    logger.info("Conversion -> extend_role_to_did_all")

    for did_name,did_node in final_yaml["did_all"].items():
        if 'did_accessibility' in did_node.keys():
            if 'diagnostic_session_and_security_level' in did_node['did_accessibility'].keys():
                node_sec_level = did_node['did_accessibility']['diagnostic_session_and_security_level']
                if 'execution_authentication_pattern_read' in node_sec_level.keys():
                    did_node['read_role'] = node_sec_level['execution_authentication_pattern_read']
                if 'execution_authentication_pattern_write' in node_sec_level.keys():
                    did_node['write_role'] = node_sec_level['execution_authentication_pattern_write']
                if 'execution_authentication_pattern_io' in node_sec_level.keys():
                    did_node['io_role'] = node_sec_level['execution_authentication_pattern_io']

def extend_role_to_routines_all(final_yaml):
    logger.info("Conversion -> extend_role_to_routines_all")

    for key,value in final_yaml["routines_all"].items():
        if 'execution_authentication_pattern' in value.keys():
            value['routine_role'] = value['execution_authentication_pattern']

def convert_extended_data_records(final_yaml):
    logger.info(f"Conversion -> extended_data_records")
    for key_name, ex_data in final_yaml['extended_data_records'].items():
        ex_data['origin_short_name'] = ex_data['short_name']
        ex_data['short_name'] = key_name # key-name -> short_name


def convert_freeze_frames(final_yaml):
    # Convert 'freeze_frames' trigger type from string to Enum
    logger.info("Convert 'freeze_frames' trigger type from string to Enum")
    for ff_name, ff_node in final_yaml['freeze_frames'].items():
        assert ff_node['trigger'] != "custom", "How to convert trigger type 'custom' ?"
        orig_ff_node_trigger = ff_node['trigger']
        if ff_node['trigger'] == "confirmed":
            ff_node['trigger'] = "DEM_TRIGGER_ON_CONFIRMED"
        elif ff_node['trigger'] == "testFailed":
            ff_node['trigger'] = "DEM_TRIGGER_ON_TEST_FAILED"
        elif ff_node['trigger'] == "pending":
            ff_node['trigger'] = "DEM_TRIGGER_ON_PENDING"

        ff_node['origin_short_name'] = ff_node['short_name']
        ff_node['short_name'] = ff_name # key-name -> short_name

        logger.debug(f"@Convert {ff_name} trigger type from '{orig_ff_node_trigger}' to '{ff_node['trigger']}'")

def convert_debounce_algorithm(final_yaml):
    # Conversion -> debounce_algorithm
    logger.info("Conversion -> debounce_algorithm")
    for aname, algo in final_yaml['debounce_algorithm'].items():
        if 'counter_based' in algo.keys():
            algo['base'] = 'Counter'
            logger.debug(f"@add debounce_algorithm -> {aname} -> base = Counter")

            if 'counter_fdc_threshold' not in algo.keys():
                logger.debug(f"@add counter_fdc_threshold to Counter based algo")
                algo['counter_fdc_threshold'] = 5 # default
            else:
                logger.warning(f"@Why the 'counter_fdc_threshold' was already present ?")

        if 'time_based' in algo.keys():
            algo['base'] = "Timer"
            logger.debug(f"@add debounce_algorithm -> {aname} -> base = Timer")

            if 'time_fdc_threshold' not in algo.keys():
                logger.debug(f"@add time_fdc_threshold to Time based algo")
                algo['time_fdc_threshold'] = 3.0 # default
            else:
                logger.warning(f"@Why the 'time_fdc_threshold' was already present ?")

            if 'debounce_behavior' not in algo.keys():
                logger.debug(f"@add debounce_behavior to Time based algo")
                algo['debounce_behavior'] = 'reset'
            else:
                logger.warning(f"@why debounce_behavior was already present ?")

            # Convert time_failed_threshold & time_passed_threshold to float type
            algo['time_failed_threshold'] = float(algo['time_failed_threshold'])
            algo['time_passed_threshold'] = float(algo['time_passed_threshold'])

        if 'monitor_internal' in algo.keys():
            algo['base'] = "Custom"
            logger.debug(f"@add debounce_algorithm -> {aname} -> base = Custom")

        # short_name in 'debounce_algorithm' can be different from key-name and can be refered by others!!


def convert_reset_all(final_yaml):
    logger.info(f"Conversion -> reset_all")

    for rst_name, rst_body in final_yaml['reset_all'].items():
        # Convert the 'SubFunctionNumber' to 'sub_function_identifier'
        rst_body['sub_function_identifier'] = rst_body['SubFunctionNumber']

    # Sort by 'sub_function_identifier'
    reset_all = OrderedDict()
    for rst_body in final_yaml['reset_all'].values():
        reset_all[ rst_body['sub_function_identifier'] ] = rst_body
    final_yaml['reset_all'] = reset_all # Update the changes


def create_io_all(final_yaml):
    # execution_authorization_pattern -> For drc_input_output (io_all):
    logger.info("Extend execution_authorization_pattern to IO_all")
    # Create new top_node for this case
    IO_all = final_yaml['IO_all'] = OrderedDict()
    for did_id, did_node in final_yaml['did_all'].items():
        if did_node['supported_functions']['io_control'] is False:
            continue

        logger.debug(f"@add new item [{hex(did_id)}] to IO_all TopNode")

        # io_control = True
        IO_all[did_id] = OrderedDict()
        IO_all[did_id]['identifier'] = did_node['identification']['code']
        IO_all[did_id]['request'] = {'control_option_record': OrderedDict()}
        control_option_record = IO_all[did_id]['request']['control_option_record']

        control_option_record['io_control_parameter'] = []
        if 'io_control_description' in did_node['supported_functions'].keys():
            for k,v in did_node['supported_functions']['io_control_description'].items():
                if k == 'return_control_to_ecu' and v is True:
                    control_option_record['io_control_parameter'].append(0x00)
                if k == 'reset_to_default' and v is True:
                    control_option_record['io_control_parameter'].append(0x01)
                if k == 'freeze_current_state' and v is True:
                    control_option_record['io_control_parameter'].append(0x02)
                if k == 'short_term_adjustment' and v is True:
                    control_option_record['io_control_parameter'].append(0x03)

        # Drop 'control_state' item from IO_all_item, add 'did_size' for checking
        control_option_record['did_size'] = did_node['implementation']['did_size'] # unit by byte
        control_option_record['bytes'] = did_node['implementation']['bytes'] # bytes info

        if 'access' in did_node.keys():
            IO_all[did_id]['access'] = copy.deepcopy(did_node['access'])

        # Don't use 'access' anymore, check pattern -> 'diagnostic_session' like DID item
        if 'did_accessibility' in did_node.keys() and did_node['did_accessibility']: # non-empty
            assert 'diagnostic_session' in did_node['did_accessibility'].keys()
            IO_all[did_id]['diagnostic_session'] = \
                        copy.deepcopy(did_node['did_accessibility']['diagnostic_session'])

        for tag_name in ['data_enable_condition']: # Maybe more
            if tag_name in did_node.keys():
                IO_all[did_id][tag_name] = copy.deepcopy(did_node[tag_name])

        if 'io_role' in did_node.keys():
            IO_all[did_id]['io_role'] = copy.deepcopy(did_node['io_role'])


def convert_routines_all(final_yaml, final_pattern):
    # execution_authorization_pattern -> For routines_all
    logger.info("Extend execution_authorization_pattern to routines_all")
    for rid_name, rid_node in final_yaml['routines_all'].items():
        logger.debug(f"@convert pattern to 'access' in rid [{rid_name}] ")
        if 'execution_authorization_pattern' in rid_node.keys():
            pattern = rid_node['execution_authorization_pattern']
            generated_rid_access = OrderedDict()
            generated_rid_access['session'] = [ s_name for s_name, s_id in final_pattern[pattern]['session'] ]
            if final_pattern[pattern]['security_level']: # non-empty
                generated_rid_access['security_type'] = 0x27
                generated_rid_access['security_level'] = [ l_name for l_name , s_id in final_pattern[pattern]['security_level'] ]
            else: # empty -> no security
                pass
            rid_node['access'] = generated_rid_access


def convert_dtc_all_and_events(final_yaml):
    # Conversion -> drc_dtcs (dtc_all & events)
    logger.info("Extend execution_authorization_pattern to dtc_all & events")
    dtc_events = OrderedDict()

    for dtc_code, dtc_node in final_yaml['dtc_all'].items():
        # Combine the new 'code' from original-code + fault_type
        logger.debug(f"@combine the 'code' & 'fault_type' to new 'code' in dtc [{hex(dtc_code)}]")
        new_code = (dtc_node['identification']['code'] << 8) | dtc_node['identification']['fault_type']
        dtc_node['identification']['origin_code'] = dtc_node['identification']['code']
        dtc_node['identification']['code'] = new_code

        dtc_node['events'] = list()
        assert 'functional_conditions' in dtc_node.keys()
        assert 'events' in dtc_node['functional_conditions'].keys()

        events = dtc_node['functional_conditions']['events']
        for evt in events:
            assert evt['mnemonic'] not in dtc_events
            dtc_events[evt['mnemonic']] = copy.deepcopy(evt)

    # By default, Alphabetical order should be used
    sorted_dtc_events = OrderedDict(
                            sorted(dtc_events.items(),
                                   key=lambda evt: evt[0])) # [0] = mnemonic
    # Event ID start from 0x01
    for idx, evt in enumerate(sorted_dtc_events.values(), start=1):
        evt['id'] = idx

    for dtc_node in final_yaml['dtc_all'].values():
        current_dtc_events = []
        for evt_in_dtc in dtc_node['functional_conditions']['events']:
            current_dtc_events.append(sorted_dtc_events[evt_in_dtc['mnemonic']]['id'])
        dtc_node['events'] = current_dtc_events
        del dtc_node['functional_conditions'] # Remove the original 'functional_conditions'.'events'

    def find_dup_event_id_mnemonic(events):
        element_count = {}
        duplicates = []

        # Already ensure the string type on schema-checking stage
        lst = [ ev['mnemonic'] for ev in events.values()]

        for item in lst:
            if item in element_count:
                element_count[item] += 1
            else:
                element_count[item] = 1

        for item, count in element_count.items():
            if count > 1:
                duplicates.append(item)

        return duplicates

    dup_evid_lst = find_dup_event_id_mnemonic(sorted_dtc_events)
    if len(dup_evid_lst) != 0: # Non-empty
        logger.error(f"Duplicate event-id mnemonic, please check {dup_evid_lst}")
        sys.exit(1)
    logger.info("All mnemonic of event ids are unique")

    final_yaml['events'] = sorted_dtc_events


def convert_datas_enable_conditions(final_yaml):
    # Conversion -> datas_enable_conditions
    logger.info("Conversion -> datas_enable_conditions")
    datas_enable_conditions_nrc_map = OrderedDict()
    for ec_name, ec_node in final_yaml['datas_enable_conditions'].items():
        # All checks have beed done in check-stage.
        for logic in ec_node.keys():
            if logic not in ('and', 'or'): continue

            for idx, node in enumerate(ec_node[logic]):
                for body in node.values():
                    datas_enable_conditions_nrc_map[ec_name] = \
                        { 'nrc' : body['nrc'], 'id' : len(datas_enable_conditions_nrc_map) + 1}
                    break
                break
            break
        # Continue ...

    final_yaml['origin_datas_enable_conditions'] = final_yaml['datas_enable_conditions'] # Rename to be recorded
    final_yaml['datas_enable_conditions'] = datas_enable_conditions_nrc_map # collected in checking

    # Conversion for all data_enable_condition in routines_all, reset_all, events, did_all, IO_all
    def convert_data_enable_condition(final_yaml, top_node_name):
        logger.info(f"Convert data_enable_condition in [{top_node_name}] ...")
        for i_name, i_node in final_yaml[top_node_name].items():
            if 'data_enable_condition' in i_node.keys():
                assert type(i_node['data_enable_condition']) is dict
                assert len(i_node['data_enable_condition']) == 1
                i_node['origin_data_enable_condition'] = OrderedDict()
                for logc_opt, cond_list in i_node['data_enable_condition'].items():
                    i_node['origin_data_enable_condition'][logc_opt] = []
                    for cond in cond_list:
                        assert cond in final_yaml['datas_enable_conditions'].keys(), f"Condition -> {cond} was not found in [{i_name}]"
                        i_node['origin_data_enable_condition'][logc_opt].append(final_yaml['datas_enable_conditions'][cond]['id'])
                # Swap them
                i_node['data_enable_condition'], i_node['origin_data_enable_condition'] \
                    = i_node['origin_data_enable_condition'], i_node['data_enable_condition']
        logger.info(f"Conversion [{top_node_name}] done.")

    convert_data_enable_condition(final_yaml, 'did_all')
    convert_data_enable_condition(final_yaml, 'reset_all')
    convert_data_enable_condition(final_yaml, 'events')
    convert_data_enable_condition(final_yaml, 'IO_all')
    convert_data_enable_condition(final_yaml, 'routines_all')

def extend_size_to_routine_parameters(final_yaml):
    for node_name, node_info in final_yaml['routine_parameters_all'].items():
        total_bits = 0
        reserved = False
        for bit_offset, element in node_info['bit_offset'].items():
            if element['dataElement'] != 'Reserved':
                for data,data_info in final_yaml['datas'].items():
                    if data == element['dataElement']:
                        if 'functional_definition' in data_info.keys():
                            total_bits += data_info['functional_definition']['bit_size']
            else:
                reserved = True

        if not reserved:
            node_info['size'] = total_bits//8

def convert_to_specific_format(with_default, top_build_layer):

    final_yaml = copy.deepcopy(with_default)

    convert_diagnostic_session(final_yaml)

    convert_extended_data_records(final_yaml)

    convert_freeze_frames(final_yaml)

    add_default_info_to_services_all(final_yaml)

    convert_debounce_algorithm(final_yaml)

    convert_diagnostic_session_security_level(final_yaml)

    extend_roles_in_authentication(final_yaml)

    extend_role_to_did_all(final_yaml)

    extend_role_to_routines_all(final_yaml)

    extend_size_to_routine_parameters(final_yaml)

    # Conversion -> execution_authorization_pattern
    logger.info("Conversion -> execution_authorization_pattern")
    session_id_map = { k : v['id'] for k,v in final_yaml['diagnostic_session'].items() }
    sec_lvl_id_map = { k : v['request_seed_id'] for k,v in final_yaml['diagnostic_session_security_level'].items() }

    pattern_map = OrderedDict()
    pattern_map['session'] = OrderedDict()
    pattern_map['security_level'] = OrderedDict()

    pattern_tags = []

    for item in final_yaml['execution_authorization_pattern'].values():
        if 'diagnostic_session' == item['class']:
            pattern_map['session'][item['short_name']] = OrderedDict()
            this_session = pattern_map['session'][item['short_name']]

            this_session['id'] = session_id_map[item['short_name']]
            this_session['tags'] = OrderedDict()

            for pname, pval in item.items():
                if pname in ('class', 'short_name'): continue
                this_session['tags'].update({pname: pval})

                # Record the total pattern-tags
                if pname not in pattern_tags:
                    pattern_tags.append(pname)

        if 'diagnostic_session_security_level' == item['class']:
            pattern_map['security_level'][item['short_name']] = OrderedDict()
            this_level = pattern_map['security_level'][item['short_name']]

            this_level['id'] = sec_lvl_id_map[item['short_name']]
            this_level['tags'] = OrderedDict()
            for pname, pval in item.items():
                if pname in ('class', 'short_name'): continue
                this_level['tags'].update({pname: pval})

    # Format: final_pattern = {'base' : {'session': [(name, id) ...],
    #                                    'security_level': [(name, id) ...]} ... }
    final_pattern = OrderedDict()
    for p in pattern_tags:
        final_pattern[p] = OrderedDict()
        final_pattern[p]['session'] = list()
        final_pattern[p]['security_level'] = list()

    for s_name, s_node in pattern_map['session'].items():
        for tag, val in s_node['tags'].items():
            if val is True: # Only pick the True-Item
                final_pattern[tag]['session'].append( (s_name, s_node['id']) )
    for l_name, l_node in pattern_map['security_level'].items():
        for tag, val in l_node['tags'].items():
            if val is True: # Same as 'session'
                final_pattern[tag]['security_level'].append( (l_name, l_node['id']) )

    convert_patterns_in_service_all(final_yaml, final_pattern)

    convert_patterns_in_did_all(final_yaml, final_pattern)

    create_io_all(final_yaml)

    convert_routines_all(final_yaml, final_pattern)

    convert_dtc_all_and_events(final_yaml)

    convert_datas_enable_conditions(final_yaml)

    create_security_binding(final_yaml, final_pattern)

    convert_reset_all(final_yaml)

    with open(os.path.join(top_build_layer, 'final_version.yaml'), 'w') as f:
        yaml.dump(final_yaml, f, default_flow_style=False, sort_keys=False)

    shutil.copy(os.path.join(top_build_layer, 'final_version.yaml'),
                os.path.join(top_build_layer, 'diag_template.yaml'))

    return (final_yaml, final_pattern)
