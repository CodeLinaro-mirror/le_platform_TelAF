/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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



static void SignalHandler( int sigNum)
{
    int status;
    LE_INFO("Stopping the LXC container on tafLxcInit app termination");
    status = system("sh /etc/lxc/telaf_lxc.sh stop");
    if(status == 0)
        LE_INFO("LXC container is stopped successfully");
    else
        LE_ERROR("Failed to stop LXC container");

    LE_INFO("End tafLxcInit sample test app");
    exit(EXIT_SUCCESS);
}

COMPONENT_INIT
{
    LE_INFO("tafLxcInit starting");
    int status;
    status = system("sh /etc/lxc/telaf_lxc.sh create");
    if(status == 0)
        LE_INFO("LXC container is created successfully");
    else
    {
        status = system("sh /etc/lxc/telaf_lxc.sh info");
        if(status == 0)
            LE_INFO("LXC container is already present");
        else
            LE_FATAL("Failed to create LXC container");
    }

    status = system("sh /etc/lxc/telaf_lxc.sh start");
    if(status == 0)
        LE_INFO("LXC container is started successfully");
    else
        LE_FATAL("Failed to start LXC container");

    le_sig_SetEventHandler(SIGTERM, SignalHandler);

}