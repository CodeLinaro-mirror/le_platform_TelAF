#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import os, sys
import json, hashlib
from jinja2 import Environment, FileSystemLoader

from .constants import JSON_FNAME
from .logger import logc
from . import __version__ as tool_version
from datetime import datetime

def f_get_list_elm_type(m_list):
    assert len(m_list) != 0, "Empty list."
    elm_type = type(m_list[0])

    same = all(type(e) == elm_type for e in m_list)
    assert same is True, "Different type in list"

    return f_get_value_type(m_list[0])


def f_get_value_type(value):
    if type(value) is str:
        return 'std::string'
    elif type(value) is float:
        return 'float'
    elif type(value) is int:
        return 'int'
    elif type(value) is bool:
        return 'bool'
    elif value is None:
        return "std::string"
    else:
        raise Exception("Bad element --> [{}]".format(value))

def f_to_hex_format(value, width=0):
    assert type(value) is int
    return f"0x{value:0{width}X}"

Filters = {
    'f_get_list_elm_type': f_get_list_elm_type,
    'f_get_value_type' : f_get_value_type,
    'f_to_hex_format' : f_to_hex_format,
}

def t_string(value):
    return type(value) is str

def t_int(value):
    return type(value) is int

def t_float(value):
    return type(value) is float

def t_bool(value):
    return type(value) is bool

def t_dict(value):
    return type(value) is dict

def t_list(value):
    return type(value) is list

def t_none(value):
    return value is None

Testers = {
    't_string' : t_string,
    't_int' :    t_int,
    't_bool' :   t_bool,
    't_float' :  t_float,
    't_dict' :   t_dict,
    't_list' :   t_list,
    't_none' :   t_none,
}

def raise_exception(msg):
    raise Exception(msg)

def get_generated_name(tmpl_file_name, build_dir):
    snippets = tmpl_file_name.split(".")
    return os.path.join(build_dir, ".".join(snippets[:2]))

def compute_file_md5(file_path):
    md5_hash = hashlib.md5()

    with open(file_path, 'rb') as file:
        for byte_block in iter(lambda: file.read(4096), b""):
            md5_hash.update(byte_block)

    return md5_hash.hexdigest()

def generate_code(custom, tmpls_layer, build_dir):
    env = Environment(loader=FileSystemLoader(tmpls_layer),
                      keep_trailing_newline = True,
                      trim_blocks = True,
                      lstrip_blocks = True)

    env.globals["raise_exception"] = raise_exception

    env.tests.update(Testers)
    env.filters.update(Filters)

    timestamp = datetime.timestamp(datetime.now())
    generated_time = datetime.fromtimestamp(timestamp).strftime('%Y_%m_%d__%H_%M_%S')

    root_node = custom.merged_data

    orig_evid_h_tmpl = "diag_ids.h.jinja"
    evid_h_generated = get_generated_name(orig_evid_h_tmpl, build_dir)
    evid_h_template = env.get_template(orig_evid_h_tmpl)
    evid_h_code = evid_h_template.render(
                        root_node = root_node,
                        ev_id_name_max = max([ev['id'] for ev in root_node['events'].values()]),
                        oc_id_name_max = max([len(k) for k in root_node['operation_cycle'].keys()]),
                        tool_version = tool_version + "_" + "customer",
                        generated_time = generated_time
                        )
    with open(evid_h_generated, 'w') as evid_h_generated_fd:
        evid_h_generated_fd.write(evid_h_code)
    logc.info("[Generate] Event id header file done.")

    evid_h_md5_str = compute_file_md5(evid_h_generated)
    logc.info(f"[MD5] Event ID header file: {evid_h_md5_str}")

    json_md5_str = compute_file_md5(os.path.join(build_dir, JSON_FNAME))
    logc.info(f"[MD5] Json file: {json_md5_str}")

    orig_cpp_tmpl = "configuration.cpp.jinja"
    orig_hpp_tmpl = "configuration.hpp.jinja"

    cpp_generated = get_generated_name(orig_cpp_tmpl, build_dir)
    hpp_generated = get_generated_name(orig_hpp_tmpl, build_dir)

    cpp_template = env.get_template(orig_cpp_tmpl)
    hpp_template = env.get_template(orig_hpp_tmpl)

    if not root_node: # Empty dict ?
        logc.error("Invalid 'root_node' for templates, empty 'customer' ? please check.")
        sys.exit(1)

    cpp_rendered_code = cpp_template.render(root = root_node)
    hpp_rendered_code = hpp_template.render(root = root_node,
                                                   tool_version = tool_version + "_" + "customer",
                                                   generated_time = generated_time,
                                                   json_md5 = json_md5_str,
                                                   evid_h_md5 = evid_h_md5_str)

    with open(cpp_generated, 'w') as out_cpp_fd, open(hpp_generated, 'w') as out_hpp_fd:
        out_cpp_fd.write(cpp_rendered_code)
        out_hpp_fd.write(hpp_rendered_code)
    logc.info("[Generate] Source code done.")

def generate_json(custom, build_dir):
    json_fname = os.path.join(build_dir, JSON_FNAME)
    with open(json_fname, 'w') as j_fd:
        j_data = json.dumps(custom.merged_data, indent=2)
        j_fd.write(j_data)

    logc.info("[Generate] Json done.")
