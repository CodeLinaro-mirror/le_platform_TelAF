#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import os, re
import json, hashlib
from jinja2 import Environment, FileSystemLoader
from collections import OrderedDict
from .logger import logger
from datetime import datetime

JSON_FNAME = "diag_template.yaml.json"

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

def f_to_data_item(mnemonic, root):
    data_item = root['datas'][mnemonic]
    return data_item

def f_coding_list_conversion(coding_list):

    assert isinstance(coding_list, dict)

    statement = "{"
    for codev, desc in coding_list.items():
        # coding item size <= uint32_t
        assert (type(codev) is int) and (codev < 2**32)
        statement += f"0x{codev:X}, "
    statement += "}"
    return statement

def iterate_hex_string(hex_str):
    if hex_str.startswith("0x"):
        hex_str = hex_str[2:]
    for i in range(0, len(hex_str), 2):
        yield hex_str[i:i+2]

def format_hex(value):
    hex_str = f"{value:X}"
    if len(hex_str) % 2 != 0:
        hex_str = "0" + hex_str
    return f"0x{hex_str}"

def f_forbidden_values_conversion(forbidden_values, data_item):
    assert data_item['functional_definition']['bit_size'] % 8 == 0
    assert isinstance(forbidden_values, list)
    assert len(forbidden_values) > 0

    # Usage for below format:
    # auto outer_vector = std::make_shared<std::vector<std::shared_ptr<std::vector<uint8_t>>>>(
    #     std::initializer_list<std::shared_ptr<std::vector<uint8_t>>>{
    #         std::make_shared<std::vector<uint8_t>>(std::initializer_list<uint8_t>{1, 2, 3}),
    #         std::make_shared<std::vector<uint8_t>>(std::initializer_list<uint8_t>{4, 5, 6}),
    #         std::make_shared<std::vector<uint8_t>>(std::initializer_list<uint8_t>{7, 8, 9})
    #     }
    # );

    byte_size = data_item['functional_definition']['bit_size'] // 8

    statement = "{"

    for byte_queue in forbidden_values:

        each_forbidden_block = "std::make_shared<std::vector<uint8_t>>(std::initializer_list<uint8_t>{"

        if byte_queue == 0:
            # For 0, generate byte_size number of 0x00
            byte_list = ["0x00"] * byte_size
            each_forbidden_block += ", ".join(byte_list)
            each_forbidden_block += "}),"
        else:

            value = format_hex(byte_queue)
            for byte in iterate_hex_string(value):
                each_forbidden_block += "0x" + byte
                each_forbidden_block += ", "

            each_forbidden_block += "}),"

        statement += each_forbidden_block

    statement += "}"
    return statement

range_match = re.compile(r"\s*\((0x[0-9A-Fa-f]+)\.\.(0x[0-9A-Fa-f]+)\)\s*")

def f_forbidden_characters_conversion(forbidden_characters, data_item):
    assert data_item['functional_definition']['bit_size'] % 8 == 0
    assert isinstance(forbidden_characters, list)

    statement = "{"
    for forbidden_range in forbidden_characters:
        hit = range_match.match(forbidden_range)
        if hit:
            start_hex, stop_hex = hit.groups()
            assert stop_hex >= start_hex

            # (0x00..0x1F)  -> closed-range [0x00:0x1F]
            for val in range(int(start_hex,16), int(stop_hex,16) + 1):
                statement += hex(val) + ","
        else:
            raise Exception(f"Bad forbidden_characters range provided: {forbidden_range}!")
    statement += "}"
    return statement

def signed_range(bits):
    max_signed = (1 << (bits - 1)) - 1   # 2^(n-1) - 1
    min_signed = -(1 << (bits - 1))      # -2^(n-1)
    return min_signed, max_signed

def unsigned_range(bits):
    max_unsigned = (1 << bits) - 1   # 2^n - 1
    min_unsigned = 0                 # 0
    return min_unsigned, max_unsigned

def f_check_min_max(data_item, _min, _max):
    assert isinstance(data_item, dict)
    assert 'value_type' in data_item['functional_definition']

    bit_size = data_item['functional_definition']['bit_size']
    sign = data_item['functional_definition']['value_type']

    if (sign == "sint8"
    or  sign == "sint16"
    or  sign == "sint32"):
        v_min, v_max = signed_range(bit_size)
        assert _min >= v_min
        assert _max <= v_max
    else:
        v_min, v_max = unsigned_range(bit_size)
        assert _min >= v_min
        assert _max <= v_max
    return ""

Filters = {
    'f_get_list_elm_type': f_get_list_elm_type,
    'f_get_value_type' : f_get_value_type,
    'f_to_hex_format' : f_to_hex_format,
    'f_to_data_item' : f_to_data_item,
    'f_coding_list_conversion' : f_coding_list_conversion,
    'f_forbidden_values_conversion' : f_forbidden_values_conversion,
    'f_forbidden_characters_conversion' : f_forbidden_characters_conversion,
    'f_check_min_max': f_check_min_max,
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
    return type(value) is dict or type(value) is OrderedDict

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

def generate_code(root_node, final_pattern, tmpls_layer, build_dir):
    env = Environment(loader=FileSystemLoader(tmpls_layer),
                      keep_trailing_newline = True,
                      trim_blocks = True,
                      lstrip_blocks = True)

    env.globals["raise_exception"] = raise_exception

    env.tests.update(Testers)
    env.filters.update(Filters)

    timestamp = datetime.timestamp(datetime.now())
    generated_time = datetime.fromtimestamp(timestamp).strftime('%Y_%m_%d__%H_%M_%S')

    orig_evid_h_tmpl = "diag_ids.h.jinja"
    evid_h_generated = get_generated_name(orig_evid_h_tmpl, build_dir)
    evid_h_template = env.get_template(orig_evid_h_tmpl)
    evid_h_code = evid_h_template.render(
                        root_node = root_node,
                        ev_id_name_max = max([len(ev['mnemonic']) for ev in root_node['events'].values()]),
                        cond_id_name_max = max([len(k) for k in root_node['datas_enable_conditions'].keys()]),
                        oc_id_name_max = max([len(k) for k in root_node['operation_cycle'].keys()]),
                        tool_version = root_node['version'],
                        generated_time = generated_time
                        )
    with open(evid_h_generated, 'w') as evid_h_generated_fd:
        evid_h_generated_fd.write(evid_h_code)
    logger.info("[Generate] Event id header file done.")

    evid_h_md5_str = compute_file_md5(evid_h_generated)
    logger.info(f"[MD5] Event ID header file: {evid_h_md5_str}")

    json_md5_str = compute_file_md5(os.path.join(build_dir, JSON_FNAME))
    logger.info(f"[MD5] Json file: {json_md5_str}")

    orig_cpp_tmpl = "configuration.cpp.jinja"
    orig_hpp_tmpl = "configuration.hpp.jinja"
    cpp_generated = get_generated_name(orig_cpp_tmpl, build_dir)
    hpp_generated = get_generated_name(orig_hpp_tmpl, build_dir)
    cpp_template = env.get_template(orig_cpp_tmpl)
    hpp_template = env.get_template(orig_hpp_tmpl)
    cpp_rendered_code = cpp_template.render(root = root_node, final_pattern = final_pattern)
    hpp_rendered_code = hpp_template.render(root = root_node,
                                            tool_version = root_node['version'],
                                            generated_time = generated_time,
                                            json_md5 = json_md5_str,
                                            evid_h_md5 = evid_h_md5_str)
    with open(cpp_generated, 'w') as out_cpp_fd, open(hpp_generated, 'w') as out_hpp_fd:
        out_cpp_fd.write(cpp_rendered_code)
        out_hpp_fd.write(hpp_rendered_code)

    logger.info("[Generate] Config-module source code done.")

def generate_json(root_node, build_dir):
    json_fname = os.path.join(build_dir, JSON_FNAME)
    with open(json_fname, 'w') as j_fd:
        j_data = json.dumps(root_node, indent=2)
        j_fd.write(j_data)

    logger.info("[Generate] Json done.")
