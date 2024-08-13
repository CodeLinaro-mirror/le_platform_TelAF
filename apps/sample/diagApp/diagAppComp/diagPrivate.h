/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DIAGPRIVATE_H
#define DIAGPRIVATE_H

#include "legato.h"
#include "interfaces.h"

// Enable the following macro for multi-vlan testing
// #define DIAG_MULTIVLAN_TEST
#define TEST_VLAN_ID_0      10
#define TEST_VLAN_ID_1      110

#define SYSTEM_COMMAND_STR_LENGTH 1030
#define DELETE_SYSTEM_CMD_FORMAT "rm %s"

void diagRFT_DeactivateProgrammingByVlanId(uint32_t vlanId);
void diagRFT_DeactivateProgramming();

uint8_t writeDIDToConfigTree
(
    const uint16_t dataId,
    const uint8_t* dataPtr,
    uint16_t dataSize
);

uint8_t readDIDFromConfigTree
(
    const uint16_t dataId,
    uint8_t* sendBuf,
    size_t* sendBufLen
);

taf_update_State_t diagRoutineCtrl_GetUpdateState();

#endif /* DIAGPRIVATE_H */
