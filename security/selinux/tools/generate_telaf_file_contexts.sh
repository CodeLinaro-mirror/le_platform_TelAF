# Copyright (c) 2021 The Linux Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of The Linux Foundation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Changes from Qualcomm Innovation Center are provided under the following license:
# Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

#!/bin/bash
BASEDIR=$(dirname $(realpath $0))

SEPOLICY_SYS_DIR=$(dirname ${BASEDIR})/sepolicy
SEPOLICY_COMPONENT_DIR=$(dirname ${BASEDIR})/../../components

FILE_CONTEXTS_DIR=${SEPOLICY_SYS_DIR}/files
FILE_CONTEXTS_FILE=${FILE_CONTEXTS_DIR}/file_contexts

FC_FILES=$(find ${SEPOLICY_SYS_DIR} ${SEPOLICY_COMPONENT_DIR} -type f -name "*.fc")

if [ ! -z "$FC_FILES" ]
then
    if [ ! -d ${FILE_CONTEXTS_DIR} ]
    then
        mkdir ${FILE_CONTEXTS_DIR}
    fi
    if [ -e ${FILE_CONTEXTS_FILE} ]
    then
        rm ${FILE_CONTEXTS_FILE}
    fi
    touch ${FILE_CONTEXTS_FILE}
fi


for FC_FILE in ${FC_FILES}
do
    while read file rule; do
        if [[ ${file:0:7} == "/legato" ]]
        then
            #echo "file: $file rule: $rule ";
            rule=${rule//gen_context(/};
            rule=${rule//t,s/t:s};
            rule=${rule//)/};

            if [ -z "${file:7}" ]
            then
                echo "/    $rule" >> $FILE_CONTEXTS_FILE;
            else
                echo "${file:7}    $rule" >> $FILE_CONTEXTS_FILE;
            fi
        fi
    done < $FC_FILE
done
