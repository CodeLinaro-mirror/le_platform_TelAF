#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import os
import ast
import yaml
import fnmatch
import pdb
from pathlib import Path

from .checker import Checker
from .exceptions import *
from .constants import *
from .logger import checking_backtrace, trace_message, logc

def is_valid_value(value):
    if not isinstance(value, str) and not isinstance(value, dict):
        return False
    return True

class FuncLister(ast.NodeVisitor):
    def __init__(self):
        self.expr = ""

    def visit_Call(self, node):

        if not isinstance(node.func, ast.Name):
            raise Schema_SyntaxError(f"Bad function name: [ {node.func.value} ], " +
                                     f"its type is {type(node.func)}")

        func_name = node.func.id

         # the wrapped function name (callable), as in: hexa -> self.checker["hexa"]
        func_name = f"self.checker['{func_name}']"

        self.expr += func_name + "("

        for arg in node.args:
            if isinstance(arg, ast.Call):
                self.visit_Call(arg)
            elif isinstance(arg, ast.Constant):
                self.expr += repr(arg.value)
            elif isinstance(arg, ast.Name):
                raise Schema_SyntaxError(f"In schema rule line, should be --> {arg.id}(), not only symbol: {arg.id}")
            else:
                raise Schema_SyntaxError(f"Positional parameters, expected :[ast.Call, ast.Constant], " +
                                         f"but got: [{type(arg)}] => [ {arg.id} ]")
            self.expr += ","

        if len(node.args) != 0:
            self.expr = self.expr[:-1]

        if len(node.args) != 0 and len(node.keywords) != 0:
            self.expr += ","

        for kw in node.keywords:
            if isinstance(kw.value, ast.Constant):
                self.expr += kw.arg + "=" + repr(kw.value.value)
            elif isinstance(kw.value, ast.Call):
                self.expr += kw.arg + "="
                self.visit_Call(kw.value)
            elif isinstance(kw.value, ast.UnaryOp):
                raise Schema_SyntaxError(f"'UnaryOp' is not supported (op: {type(kw.value.op).__name__}/{kw.value.operand.value})")
            elif isinstance(kw.value, ast.Name):
                raise Schema_SyntaxError(f"ast.Name/{kw.value.id} is not allowed in schema-line, expected:[ast.Call, ast.Constant]")
            else:
                raise Schema_SyntaxError(f"Keyword parameters, expected :[ast.Call, ast.Constant], " +
                                f"but got: [{type(kw.value)}]")
            self.expr += ","

        if len(node.keywords) != 0:
            self.expr = self.expr[:-1]

        self.expr += ")"

class Schema:
    def __init__(self, *stream):
        self.streams = list(*stream)
        self.top_nodes = {} # contains all top-level 'Node' for checking
        self.extended = False
        self.checker = Checker()

    def _construct_leaf_schema(self, leaf):
        try:
            # Checking the python-like syntax for original schema statment
            tree = ast.parse(leaf.line_rule, mode='eval')
            logc.debug(ast.dump(tree))
        except SyntaxError as e:
            raise ConstructLeafLineSchemaError("ast.parse", e, leaf)

        call = FuncLister()
        call.visit(tree)

        if not call.expr.strip():
            raise FailToTransform(leaf)

        logc.debug(call.expr)

        try:
            real_schema = eval(call.expr) # return the wrapper function
        except SyntaxError as e:
            raise ConstructLeafLineSchemaError("eval", e, leaf)

        leaf.update_schema(real_schema, self.top_nodes)

    def _transform(self, node):
        if isinstance(node, Leaf):
            self._construct_leaf_schema(node)
        elif isinstance(node, Branch):
            for n in node.values():
                if isinstance(n, Branch):
                    self._transform(n)
                elif isinstance(n, Leaf):
                    self._construct_leaf_schema(n)
                else:
                    raise Exception(f"Why the type {type(n)} of node is NOT in [Leaf,Branch] ?")
        else:
            raise Exception(f"Why the type {type(n)} of node is NOT in [Leaf,Branch] ?")

    def transform(self):
        if self.extended is False:
            logc.warning("Maybe the [built-in] checker's syntax mapping table is NOT enough.")
        for stream in self.streams:
            for doc in stream:
                for node in doc:
                    self._transform(node.node)
                    if node.label in self.top_nodes:
                        raise Exception(f"In all schema files, Make sure the top-node name is unique! "
                                        + f"You have defined the key=[ {node.label} ] repeatedly")
                    self.top_nodes.update({node.label : node})

    def extend(self, ex_syn_path):
        self.checker.extend(ex_syn_path)
        self.extended = True

    def _check(self, cus_top_node):
        for key, value in cus_top_node.items():
            try:
                top_node = self.top_nodes[key]
            except KeyError:
                raise Exception(f"Unknow top node key: [{key}], should be in {list(self.top_nodes.keys())}")
            else:
                pnode = PendingNode(key, value)
                with checking_backtrace(f"Checking-Node: [ {key} ]", first_top_node=True):
                    top_node.check(pnode, is_first = True)

    def check(self, customer):
        for fpath, cus_top_node in customer.data.items():
            logc.info(f"[Checking] custom file: {fpath}")
            self._check(cus_top_node)

    def __getattr__(self, attr):
        return getattr(self.streams, attr)

    def _repr(self, level = 0):
        show = (INDENT * level) + f"[{self.__class__.__name__}: size: {len(self.streams)}]" + "\n"
        for stream in self.streams:
            show += stream._repr(level + 1)
        return show

    def __repr__(self):
        return self._repr()

class Stream:
    def __init__(self, s_file, *docs):
        file_path = Path(s_file)
        if not file_path.exists():
            raise Exception(f"Not found: {file_path}")
        self.name = file_path.resolve()
        self.docs = list(*docs)

    def __getattr__(self, attr):
        return getattr(self.docs, attr)

    def __iter__(self):
        return iter(self.docs)

    def _repr(self, level = 0):
        show = (INDENT * level) + f"[{self.__class__.__name__}: {self.name}]" + "\n"
        for doc in self.docs:
            show += doc._repr(level + 1)
        return show

    def __repr__(self):
        return self._repr()

class Document:
    def __init__(self, *nodes):
        self.nodes = list(*nodes)

    def __getattr__(self, attr):
        return getattr(self.nodes, attr)

    def __iter__(self):
        return iter(self.nodes)

    def _repr(self, level = 0):
        show = (INDENT * level) + f"[{self.__class__.__name__}: size: {len(self.nodes)}]" + "\n"
        for node in self.nodes:
            show += node._repr(level + 1)
        return show

    def __repr__(self):
        return self._repr()


class Pair:
    def __init__(self, leaf, pending_node):
        self.leaf = leaf # schema-ruler
        self.pending_node = pending_node # pending-data

    def __getattr__(self, attr):
        return getattr(self.pending_node, attr)


class PendingNode:
    "NOTE: Only Schema-Class can instantiate this Class."

    class InternalNode:
        def __init__(self, key, value):
            self.key = key
            self.value = value
            self.kv_combo = {key:value}

    def __init__(self, key, value):
        self.pnode_stack = []
        self.pnode_stack.append(PendingNode.InternalNode(key,value))

    def push(self, key, value):
        self.pnode_stack.append(PendingNode.InternalNode(key, value))

    def pop(self):
        self.pnode_stack.pop()

    @property
    def node(self):
        return self.pnode_stack[-1]

    current = node

    def peek_node(self, idx):
        return self.pnode_stack[idx]

    def show_stack(self):
        for node in self.pnode_stack:
            print(f"{node.key} -> {node.value}") # to console for debugging

class Node:
    # Note: Schema nodes only include 'Leaf'(str) & 'Branch'(dict)
    #       but for the pending-value is not, such as: list...
    def __init__(self, top_node_label, top_node_rule):

        # schema line: label (key)
        self.label = top_node_label

        # schema line: rule (value)
        self.rule = top_node_rule

        # build real backend for this top node.
        self.node = self._build_node(self.label, self.rule)

    def _build_node(self, label, rule):
        if isinstance(rule, dict):
            branch = Branch(label)
            for _label, _rule in rule.items():
                branch.update({_label : self._build_node(_label, _rule)})
            return branch
        elif isinstance(rule, str):
            return Leaf(label, rule)
        else:
            raise Exception(f"Bad type for schemas: {type(rule).__name__} is not in [dict, str]")

    def check(self, pending_node, is_first = False):
        if is_first is True:
            if isinstance(self.node, Leaf):
                if self.node.schema_fn.required is False: # CASE_05 <--
                    raise Schema_SemanticError(f"The first top node '{self.node.line_label}' shouldn't be marked as 'required=False'")

        self.node.check(pending_node)

    def _repr(self, level = 0):
        show = (INDENT * level) + f"[{self.__class__.__name__}: {self.label}]" + "\n"
        show += self.node._repr(level + 1)
        return show

    def __repr__(self):
        return self._repr()

class Branch:
    def __init__(self, br_label, **leaves):
        self.br_label = br_label
        self.leaves = leaves

    def check(self, pending_node):
        if pending_node.current.value is None:
            raise Exception(f"Branch section: [{pending_node.current.key} / {self.br_label}], shouldn't be empty !")

        # Required-Checking:
        # CASE_01: (required) True  + (value.existness) YES --> OK, checking
        # CASE_02: (required) True  + (value.existness) NO  --> NOK, raise exception
        # CASE_03: (required) False + (value.existness) YES --> OK, checking
        # CASE_04: (required) False + (value.existness) NO  --> ignore
        # Note that: CASE_05
        # if the top_node is one instance of Class-Leaf, the 'required=False' is NOT allowd!
        for label, leaf_or_branch in self.leaves.items():
            if not isinstance(leaf_or_branch, Leaf):
                continue
            if leaf_or_branch.schema_fn.required is True: # get the 'required' info from the checker itself
                if type(pending_node.current.value) is not dict:
                    pdb.set_trace()
                if label not in pending_node.current.value.keys(): # CASE_02 <--
                    raise Schema_CheckingError_10(f"'{label}' was NOT found in '{pending_node.current.key}/{self.br_label}', that's required")
                else: # CASE_01 <--
                    pass
            else:
                if label in pending_node.current.value.keys(): # CASE_03 <--
                    trace_message(f"{label} marked as 'required=False', but existence, check it anyway")
                else: # CASE_04 <--
                    pass

        with checking_backtrace(f"Branch: {self.br_label}") as wc:
            for k,v in pending_node.current.value.items():
                try:
                    leaf = self.leaves[k]
                except KeyError:
                    raise Schema_CheckingError_11(f"During check, unknow key: [{k}] in parent section: [{pending_node.current.key} / {self.br_label}]")
                else:
                    pending_node.push(k,v)
                    leaf.check(pending_node)
                    pending_node.pop()

    def __getattr__(self, attr):
        return getattr(self.leaves, attr)

    def __iter__(self):
        return iter(self.leaves)

    def _repr(self, level = 0):
        show = (INDENT * level) + f"[{self.__class__.__name__}: {self.key} contains {len(self.leaves)}]" + "\n"
        for leaf in self.leaves.values():
            show += leaf._repr(level + 1)
        return show

    def __repr__(self):
        return self._repr()


class Leaf:
    def __init__(self, line_label, line_rule):
        self.line_label = line_label
        self.line_rule = line_rule

        self.schema_fn = None
        self.top_nodes = None

    def update_schema(self, schema_fn, top_nodes):
        self.schema_fn = schema_fn
        self.top_nodes = top_nodes # reflect to Schema's top_nodes

    def check(self, pending_node):
        trace_message(f"Leaf: {self.line_label}/{self.line_rule}")
        # import pdb
        # pdb.set_trace()
        self.schema_fn( Pair(self, pending_node) )

    def _repr(self, level = 0):
        show = (INDENT * level) + f"[{self.__class__.__name__}: {self.line_label} -> {self.line_rule}]" + "\n"
        return show

    def __repr__(self):
        return self._repr()


# Type transfer : https://stackoverflow.com/questions/50045617/yaml-load-force-dict-keys-to-strings
class ExLoader(yaml.SafeLoader):
    def construct_mapping(self, *args, **kwargs):
        mapping = super().construct_mapping(*args, **kwargs)

        for key in list(mapping.keys()):
            # bool is a subclass of int
            if not isinstance(key, bool) and isinstance(key, (int, float)):
                mapping[str(key)] = mapping.pop(key)

        return mapping


def build_stream_tree(file_path):
    with open(file_path, 'r') as f:
        stream = Stream(file_path)
        # Add one Loader by 'load_all' not 'safe_load_all'
        # Reference: https://pyyaml.org/wiki/PyYAMLDocumentation
        datas = yaml.load_all(f, Loader=ExLoader)
        for data in datas:
            doc = Document()
            try:
                for key, value in data.items():
                    doc.append( Node(key, value) ) # add the top-level Nodes
            except AttributeError as e:
                # the string alone is support by YAML, but dengine isn't
                if isinstance(data, str):
                    print(e)
                    raise Exception(f"Please check your YAML file {file_path}, only contains 'string' alone ?")
                elif data is None:
                    logc.warning(f"Why empty DOC is it configured in schema-yaml [{file_path}] ? Ignore")
                    continue # Ignore the empty document, but prompt
                else:
                    raise
            else:
                stream.append(doc)
        return stream


def build_schema_tree(x_path):
    sc = Schema()
    if os.path.isdir(x_path):
        for root, _, files in os.walk(x_path):
            for file in fnmatch.filter(files, "*" + SUFFIX):
                logc.info(f"Handling file -> {os.path.join(root, file)}")
                stream = build_stream_tree(os.path.join(root, file))
                sc.append(stream)
    else: # file
        logc.info(f"Handling single file -> {x_path}")
        stream = build_stream_tree(x_path)
        sc.append(stream)
    return sc
