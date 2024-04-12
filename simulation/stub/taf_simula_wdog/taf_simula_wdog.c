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

#include "legato.h"
#include "pa_wdog.h"

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

#define TIMEOUT 120

static struct itimerval timer;

static void TafSigTermEventHandler(int tafSigNum)
{
    LE_CRIT("Terminated");
    exit(-tafSigNum);
}


void pa_wdog_Shutdown(void)
{
    LE_FATAL("TAF PA: Watchdog timer is expired. Restart the device.");
}


void pa_wdog_Kick(void)
{
    setitimer(ITIMER_REAL, &timer, NULL);
}

static void sig_handler(int signo)
{
    LE_FATAL("External wdog bite, stop telaf framework.");
}


void pa_wdog_Init(void)
{
    signal(SIGALRM, sig_handler);

    timer.it_value.tv_sec = TIMEOUT;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;

    pa_wdog_Kick();
}


COMPONENT_INIT
{
    // Block the signal
    le_sig_Block(SIGTERM);

    // Setup signal's event handler.
    le_sig_SetEventHandler(SIGTERM, TafSigTermEventHandler);
    LE_INFO("TAF PA: COMPONENT_INIT success, watchdog PA is ready.");
}

