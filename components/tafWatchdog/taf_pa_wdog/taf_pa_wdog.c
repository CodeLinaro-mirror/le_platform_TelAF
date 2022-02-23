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
#include "pa_wdog.h"

/**
 * File descriptor of the external watchdog.
 */
static int tafWdogFd = -1;

/**
 * SIGTERM event handler which will stop the watchdog device
 */
static void TafSigTermEventHandler(int tafSigNum)
{
    LE_INFO("TAF PA: SIGTERM event, SigNum: %d, fd: %d", tafSigNum, tafWdogFd);
    if (tafWdogFd > 0) {
        if (le_fd_Write(tafWdogFd, "V", 1) != 1)
        {
            LE_WARN("TAF PA: Write to watchdog is failed!!! fd: %d", tafWdogFd);
        }

        if (le_fd_Close(tafWdogFd) < 0)
        {
            LE_WARN("TAF PA: Failed to close watchdog device. fd: %d", tafWdogFd);
        }

        tafWdogFd = -1;

        LE_INFO("TAF PA: Watchdog is stopped.");
    }
    exit(-tafSigNum);
}


/**
 * Shutdown action can be taken if service is not kicking
 */
void pa_wdog_Shutdown(void)
{
    LE_FATAL("TAF PA: Watchdog timer is expired. Restart the device.");
}


/**
 * Kick external watchdog if any
 */
void pa_wdog_Kick(void)
{
    // Todo: Kick external watchdog if available. Currently don't have it.
}


/**
 * Initialize TAF PA watchdog
 **/
void pa_wdog_Init(void)
{
    // Todo: Configure external watchdog if available. Now don't have.
}


COMPONENT_INIT
{
    // Block the signal
    le_sig_Block(SIGTERM);

    // Setup signal's event handler.
    le_sig_SetEventHandler(SIGTERM, TafSigTermEventHandler);
    LE_INFO("TAF PA: COMPONENT_INIT success, watchdog PA is ready.");
}
