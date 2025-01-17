# Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys

def gen_dlt_appid(input_string):
    # Initialize hash value
    hash_value = 0

    # Go through the input string
    for char in input_string:
        # Update the hash value
        hash_value = (hash_value * 31 + ord(char)) & 0xFFFFFFFF

    # Define the expected charset
    charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_"
    base = len(charset)

    # Map the hash value to the APPID
    result = []
    for _ in range(4):
        result.append(charset[hash_value % base])
        hash_value //= base

    return ''.join(result)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python gen_dlt_appid.py <target string>")
        sys.exit(1)

    input_string = sys.argv[1]
    print("APPID:", gen_dlt_appid(input_string))
