#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

from ..logger import logger
import copy, os
from collections import OrderedDict
import oyaml as yaml
import shutil
import enum as Enum

def resolve_all_references(only_yaml_with_default):

    logger.info("Reference Check ---- Start")

    logger.info("data_enable_conditions reference check ---- Start")

    data_enable_condition_content = []
    and_list = []
    or_list = []

    for des in only_yaml_with_default['datas_enable_conditions'].items():
        for val in des:
            if type(val) == str:
                data_enable_condition_content.append(val)

    logger.info("[data_enable_conditions] reference check in [dtc_all] ---- Start")

    for name,node in only_yaml_with_default['dtc_all'].items():
        if 'functional_conditions' in node.keys():
            for event,data in node['functional_conditions'].items():
                for dat in data:
                    if 'data_enable_condition' in dat.keys():
                        for key,value in dat['data_enable_condition'].items():
                            if key == 'and':
                                for val in value:
                                    and_list.append(val)
                            if key == 'or':
                                for val in value:
                                    or_list.append(val)
    if and_list:
        for item in and_list:
            if item not in data_enable_condition_content:
                    assert data_enable_condition_content.count(item) == 1, f"Reference check failed as [{item}] not found."
    if or_list:
        for item in or_list:
            if item not in data_enable_condition_content:
                assert data_enable_condition_content.count(item) == 1, f"Reference check failed as [{item}] not found."

    logger.info("[data_enable_conditions] reference check in [dtc_all] ---- End")

    def data_enable_condition_reference_check(only_yaml_with_default, top_node_name):
        logger.info(f"[data_enable_conditions] reference check in [{top_node_name}] ---- Start")
        for name,node in only_yaml_with_default[top_node_name].items():
            if 'data_enable_condition' in node.keys():
                for logic,data in node['data_enable_condition'].items():
                    if data:
                        for item in data:
                            if item not in data_enable_condition_content:
                                assert data_enable_condition_content.count(item) == 1, f"Reference check failed as [{item}] not found."
        logger.info(f"[data_enable_conditions] reference check in [{top_node_name}] ---- End")

    data_enable_condition_reference_check(only_yaml_with_default, 'did_all')
    data_enable_condition_reference_check(only_yaml_with_default, 'routines_all')
    data_enable_condition_reference_check(only_yaml_with_default, 'reset_all')

    logger.info("data_enable_conditions reference check ---- End")

    def extended_records_freeze_frames_reference_check(only_yaml_with_default, reference_data_node, target_category_node, sub_section_node):
        logger.info(f"[{reference_data_node}] reference check in [{target_category_node}] ---- Start")
        extended_records_freeze_frames_content = []
        for name,node in only_yaml_with_default[reference_data_node].items():
            if 'short_name' in node.keys():
                extended_records_freeze_frames_content.append(node['short_name'])
        for name,node in only_yaml_with_default[target_category_node].items():
            if sub_section_node in node.keys():
                for record,data in node[sub_section_node].items():
                    if reference_data_node in record:
                        if data:
                            for item in data:
                                if item not in extended_records_freeze_frames_content:
                                    assert extended_records_freeze_frames_content.count(item) == 1, f"Reference check failed as [{item}] not found."
        logger.info(f"[{reference_data_node}] reference check in [{target_category_node}] ---- End")

    extended_records_freeze_frames_reference_check(only_yaml_with_default, 'extended_data_records', 'dtc_all', 'identification')
    extended_records_freeze_frames_reference_check(only_yaml_with_default, 'freeze_frames', 'dtc_all', 'snapshots')

    def operation_cycle_debounce_algorithm_reference_check(only_yaml_with_default, reference_data_node, target_category_node, sub_section_node):
        logger.info(f"[{reference_data_node}] reference check in [{target_category_node}] ---- Start")
        operation_cycle_debounce_algorithm_content = []
        for name,node in only_yaml_with_default[reference_data_node].items():
            if 'short_name' in node.keys():
                operation_cycle_debounce_algorithm_content.append(node['short_name'])
        for name,node in only_yaml_with_default[target_category_node].items():
            if sub_section_node in node.keys():
                for event,data in node[sub_section_node].items():
                    for dat in data:
                        if reference_data_node in dat.keys():
                            if dat[reference_data_node] not in operation_cycle_debounce_algorithm_content:
                                assert operation_cycle_debounce_algorithm_content.count(dat[reference_data_node]) == 1, f"Reference check failed as [{dat[reference_data_node]}] not found."

        logger.info(f"[{reference_data_node}] reference check in [{target_category_node}] ---- End")

    operation_cycle_debounce_algorithm_reference_check(only_yaml_with_default, 'operation_cycle', 'dtc_all', 'functional_conditions')
    operation_cycle_debounce_algorithm_reference_check(only_yaml_with_default, 'debounce_algorithm', 'dtc_all', 'functional_conditions')

    logger.info("[debounce_algorithm_type] reference check in [debounce_algorithm] ----- Start")

    debounce_algorithm_type_content = []

    for types in only_yaml_with_default['debounce_algorithm_type']:
        debounce_algorithm_type_content.append(types)

    for name,node in only_yaml_with_default['debounce_algorithm'].items():
        if 'counter_based' in node.keys() and 'time_based' not in node.keys() and 'monitor_internal' not in node.keys():
            if node['counter_based']:
                if 'Counter' not in debounce_algorithm_type_content:
                    assert debounce_algorithm_type_content.count("Counter") == 1, f"Reference check failed as Counter debounce type not found."
            else:
                assert node['counter_based'] == True, f"Reference check failed as [node['counter_based']] was found as false."

        elif 'time_based' in node.keys() and 'counter_based' not in node.keys() and 'monitor_internal' not in node.keys():
            if node['time_based']:
                if 'Timer' not in debounce_algorithm_type_content:
                    assert debounce_algorithm_type_content.count("Timer") == 1, f"Reference check failed as Timer debounce type not found."
            else:
                assert node['time_based'] == True, f"Reference check failed as [node['time_based']] was found as false."

        elif 'monitor_internal' in node.keys() and 'counter_based' not in node.keys() and 'time_based' not in node.keys():
            if node['monitor_internal']:
                if 'Custom' not in debounce_algorithm_type_content:
                    assert debounce_algorithm_type_content.count("Custom") == 1, f"Reference check failed as Custom debounce type not found."
            else:
                assert node['monitor_internal'] == True, f"Reference check failed as [node['monitor_internal']] was found as false."

        else:
            assert False, f"Reference check failed as debounce algorithm type was not found."

    logger.info("[debounce_algorithm_type] reference check in [debounce_algorithm]----- End")

    logger.info("[data_identifier_set] DID reference check in [did_all]----- Start")

    did_list_content = []

    for name,node in only_yaml_with_default['did_all'].items():
        if 'identification' in node.keys():
            for item,did in node['identification'].items():
                if 'code' in item:
                    did_list_content.append(did)

    for name,node in only_yaml_with_default['data_identifier_set'].items():
        if 'base' in name:
            for item in node:
                if item not in did_list_content:
                    assert did_list_content.count(item) == 1, f"Reference check failed as [{item}] not found."

    logger.info("[data_identifier_set] DID reference check in [did_all]----- End")

    logger.info("[freeze_frames.triger] reference check in [freeze_frame_trigger_type]----- Start")

    freeze_frames_type_content = ['confirmed', 'testFailed', 'pending', 'custom']

    for name,node in only_yaml_with_default['freeze_frames'].items():
        if 'trigger' in node.keys():
            if node['trigger'] not in freeze_frames_type_content:
               assert freeze_frames_type_content.count(node['trigger']) == 1, f"Reference check failed as [{node['trigger']}] not found."

    logger.info("[freeze_frames.triger] reference check in [freeze_frame_trigger_type]----- End")

    logger.info("[access.session] reference check in [did_all] ---- Start")

    diagnostic_session_content = []

    for name,node in only_yaml_with_default['diagnostic_session'].items():
        if 'short_name' in node.keys():
            diagnostic_session_content.append(node['short_name'])

    for svc_id,node in only_yaml_with_default['did_all'].items():
        if 'access' in node.keys():
            for node,val in node['access'].items():
                if 'session' in node:
                    for ses in val:
                        if ses not in diagnostic_session_content:
                            assert diagnostic_session_content.count(ses) == 1, f"Reference check failed as [{ses}] not found."

    logger.info("[access.session] reference check in [did_all] ---- End")

    logger.info("execution_authorization_pattern reference check ---- Start")

    execution_authorization_pattern_content = []

    for name,node in only_yaml_with_default['execution_authorization_pattern'].items():
        for tag,val in node.items():
            if tag not in execution_authorization_pattern_content and tag != 'class' and tag != 'short_name':
                execution_authorization_pattern_content.append(tag)

    logger.info("execution_authorization_pattern reference check in [routines_all]---- Start")

    for key,value in only_yaml_with_default['routines_all'].items():
        if 'execution_authorization_pattern' in value:
            if value['execution_authorization_pattern'] not in execution_authorization_pattern_content:
                assert execution_authorization_pattern_content.count(value['execution_authorization_pattern']) == 1, f"Reference check failed as [{value['execution_authorization_pattern']}] not found."

    logger.info("execution_authorization_pattern reference check in [routines_all]---- End")

    logger.info("execution_authorization_pattern reference check in [services_all]---- Start")

    for svc_id,node in only_yaml_with_default['services_all'].items():
        if 'execution_authorization_pattern' in node.keys():
            if node['execution_authorization_pattern'] not in execution_authorization_pattern_content:
                assert execution_authorization_pattern_content.count(node['execution_authorization_pattern']) == 1, f"Reference check failed as [{node['execution_authorization_pattern']}] service pattern not found."

        if 'sub_functions' in node.keys():
            for sub_func,data in node['sub_functions'].items():
                if 'execution_authorization_pattern' in data.keys():
                    assert execution_authorization_pattern_content.count(data['execution_authorization_pattern']) == 1, f"Reference check failed as [{data['execution_authorization_pattern']}] subfunction pattern not found."

    logger.info("execution_authorization_pattern reference check in [services_all]---- End")

    logger.info("execution_authorization_pattern reference check in [diagnostic_session]---- Start")

    for key,value in only_yaml_with_default['diagnostic_session'].items():
        if 'execution_authorization_pattern' in value:
            if value['execution_authorization_pattern'] not in execution_authorization_pattern_content:
                assert execution_authorization_pattern_content.count(value['execution_authorization_pattern']) == 1, f"Reference check failed as [{value['execution_authorization_pattern']}] not found."

    logger.info("execution_authorization_pattern reference check in [diagnostic_session]---- End")

    logger.info("execution_authorization_pattern reference check ---- End")

    logger.info("Reference Check ---- End")
