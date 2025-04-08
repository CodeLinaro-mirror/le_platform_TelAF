#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

from ..logger import logger


def forbidden_feature_check(only_yaml_with_default):

    logger.info("Forbidden Check ---- Start")

    for name,node in only_yaml_with_default['did_all'].items():
        if not node['supported_functions']['write_did']:
            continue       # skip check for write_did attribute value False DID's
        else:

            assert 'did_size' in node['implementation'].keys(), f"did_size attribute didn't found in did_all.implementation"
            assert 'endianness' in node['implementation'].keys(), f"endianness attribute didn't found in did_all.implementation"
            assert node['implementation']['endianness'] == "MSB", f"endianness attribute didn't found in did_all.implementation"

            did_size =  node['implementation']['did_size']
            total_bits_size = did_size*8
            computed_bits_size = 0

            assert 'bytes' in node['implementation'].keys(), f"bytes attribute didn't found in did_all.implementation"

            for byte,data in node['implementation']['bytes'].items():
                if len(data) == 0:
                    assert False, f"{name} bytes data is empty"

                for _key,_val in data.items():

                    assert len(_val) != 0, f"{name}: {byte} byte data is empty"
                    assert _val['data_ref_mnemonic'] in only_yaml_with_default['datas'].keys(), f"{name} {byte} {_val['data_ref_mnemonic']} -------> Invalid data_ref_mnemonic"

                    for d_key,d_val in only_yaml_with_default['datas'].items():
                        if d_key == _val['data_ref_mnemonic']:
                            if 'functional_definition' in d_val.keys():
                                attribute_list = []
                                for x in d_val['functional_definition'].keys():
                                    attribute_list.append(x)

                                for subKey, subVal in d_val['functional_definition'].items():
                                    if subKey == "bit_size":
                                        computed_bits_size += subVal

                                    if subKey == "data_type":
                                        if subVal == "Not numeric":
                                            if attribute_list.count('forbidden_characters') == 0 and attribute_list.count('forbidden_values') == 0:
                                                pass

                                        if subVal == "Numeric list":
                                            assert attribute_list.count('coding') != 0, f"For {_val['data_ref_mnemonic']} coding attribute didn't found"

                                            if isinstance(d_val['functional_definition']['coding'], dict):
                                                assert len(d_val['functional_definition']['coding']) != 0, f"For {_val['data_ref_mnemonic']} coding attribute data is empty"

                                                for x,y in d_val['functional_definition']['coding'].items():
                                                    bits_allowed = pow(2,d_val['functional_definition']['bit_size'])
                                                    if x < 0 or x >= bits_allowed:
                                                        assert False,f"Invalid bit value found:{x}"

                                        if subVal == "Numeric":
                                            if attribute_list.count('min') == 0 and attribute_list.count('max') == 0 and attribute_list.count('coding') == 0:
                                                assert False, f"For {_val['data_ref_mnemonic']} range attribute min/max didn't found"

            if total_bits_size < computed_bits_size:
                assert False, f"computed did_size:{computed_did_size} exceeded did_size:{total_did_size}"

    logger.info("Forbidden Check ---- End")
