#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

from .top_node_common_props import schema__common_props
from .top_node_data_identifier_set import schema__data_identifier_set
from .top_node_datas import schema__datas
from .top_node_datas_enable_conditions import schema__datas_enable_conditions
from .top_node_debounce_algorithm import schema__debounce_algorithm
from .top_node_diagnostic_session import schema__diagnostic_session
from .top_node_diagnostic_session_security_level import schema__diagnostic_session_security_level
from .top_node_did_all import schema__did_all
from .top_node_dtc_all import schema__dtc_all
from .top_node_execution_authorization_pattern import schema__execution_authorization_pattern
from .top_node_extended_data_records import schema__extended_data_records
from .top_node_freeze_frames import schema__freeze_frames
from .top_node_operation_cycle import schema__operation_cycle
from .top_node_reset_all import schema__reset_all
from .top_node_routine_parameters_all import schema__routine_parameters_all
from .top_node_routines_all import schema__routines_all
from .top_node_services_all import schema__services_all
from .top_node_authentication_antibruteforce import schema__authentication_antibruteforce
from .top_node_authentication_timeout import schema__authentication_timeout

# All schemas to be checked for all top nodes.
schema_mapping = {
    # drc_datas
    "datas" : schema__datas,

    # drc_data_identifiers
    "did_all" : schema__did_all,

    # drc_ecu_reset
    'reset_all' : schema__reset_all,

    # drc_platform
    'common_props' : schema__common_props,
    'diagnostic_session' : schema__diagnostic_session,
    'diagnostic_session_security_level': schema__diagnostic_session_security_level,
    'execution_authorization_pattern' : schema__execution_authorization_pattern,
    'operation_cycle' : schema__operation_cycle,
    'datas_enable_conditions' : schema__datas_enable_conditions,
    'debounce_algorithm' : schema__debounce_algorithm,
    'extended_data_records' : schema__extended_data_records,
    'authentication_antibruteforce' : schema__authentication_antibruteforce,
    'authentication_timeout' : schema__authentication_timeout,

    # drc_services
    'services_all' : schema__services_all,

    # drc_snapshots
    'data_identifier_set' : schema__data_identifier_set,
    'freeze_frames': schema__freeze_frames,

    # drc_dtcs
    'dtc_all': schema__dtc_all,

    # drc_routines
    'routines_all': schema__routines_all,

    # drc_routine_parameters
    'routine_parameters_all': schema__routine_parameters_all,
}
