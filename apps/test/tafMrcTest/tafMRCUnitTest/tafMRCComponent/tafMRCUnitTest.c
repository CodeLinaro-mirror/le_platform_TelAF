/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "legato.h"
#include "interfaces.h"

void PrintHelp()
{
    LE_INFO("Please run \"app runProc tafMrcTestApp tafMrc -- number\"");
    LE_INFO("number:");
    LE_INFO("1: send ota start message to mrc daemon.");
    LE_INFO("2: perform fota upgrade, please rember to push update.zip under /data.");
    LE_INFO("3: send ota end success message to mrc daemon.");
    LE_INFO("4: send ota ab sync message to mrc daemon.");
    LE_INFO("5: send ota resume message to mrc daemon.");
}

COMPONENT_INIT
{
    long number = strtol(le_arg_GetArg(0), NULL, 10);
    switch (number) {
        case 1:
            LE_ASSERT(taf_mrc_SendOtaStartMsg() == LE_OK);
            break;
        case 2:
            LE_ASSERT(system("recovery --update_package=/data/update.zip") == 0);
            break;
        case 3:
            LE_ASSERT(taf_mrc_SendOtaEndMsg(TAF_MRC_OTA_OP_STATUS_SUCCESS) == LE_OK);
            break;
        case 4:
            LE_ASSERT(taf_mrc_SendOtaAbsyncMsg() == LE_OK);
            break;
        case 5:
            LE_ASSERT(taf_mrc_SendOtaResumeMsg() == LE_OK);
            break;
        default:
            PrintHelp();
            exit(EXIT_SUCCESS);
    }

    exit(EXIT_SUCCESS);
}
