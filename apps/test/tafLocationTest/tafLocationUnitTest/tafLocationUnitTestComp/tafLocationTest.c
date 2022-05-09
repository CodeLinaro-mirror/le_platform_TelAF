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
#include "main.h"

void tafLocationTest_DR(taf_gnss_DrParams_t* drParams)
{
    LE_INFO("======tafLocationTest_DR function: Configure Dead Reckoning======");
    le_result_t result = taf_gnss_SetDRConfig(drParams);
    switch (result)
    {
        case LE_OK:
            printf("\nSuccessfully set Dead Reckoning!\n");
            break;
        case LE_FAULT:
            printf("\nFailed to set Dead Reckoning. See logs for details\n");
            break;
        case LE_BAD_PARAMETER:
            printf("\nFailed to set Dead Reckoning, incompatible bit mask\n");
            break;
        case LE_BUSY:
            printf("\nFailed to set Dead Reckoning, service is busy\n");
            break;
        case LE_TIMEOUT:
            printf("\nFailed to set Dead Reckoning, timeout error\n");
            break;
        default:
            printf("Failed to set Dead Reckoning, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }
    le_mem_Release(drParams);
}