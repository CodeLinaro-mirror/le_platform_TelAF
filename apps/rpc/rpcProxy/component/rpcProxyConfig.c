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
#include "interfaces.h"
#include "messagingMessage.h"
#include "rpcProxyConfig.h"

//--------------------------------------------------------------------------------------------------
/**
 * Indicates if the configuration is loaded.
 */
//--------------------------------------------------------------------------------------------------
static bool IsConfigurationLoaded = false;

//--------------------------------------------------------------------------------------------------
/**
 * Indicates if loading JSON configuration is inprogress.
 */
//--------------------------------------------------------------------------------------------------
static bool IsJSONParsingInProgress = false;

//--------------------------------------------------------------------------------------------------
/**
 * The full path/name of the JSON configuration file.
 */
//--------------------------------------------------------------------------------------------------
//static char ConfigFilePath[LIMIT_MAX_PATH_BYTES] = { 0 };

//--------------------------------------------------------------------------------------------------
/**
 * The static configuration data struct, can be overrided by the configuration in JSON file is a
 * JSON file is specified.
 */
//--------------------------------------------------------------------------------------------------
#ifdef RPC_SERVER_TEST
static RpcConfigData_t MyConfiguration =
{
    .mySystem = { 2, SERVER_SYSTEM_ID, "RPC SERVER SYSTEM" },    ///< My system info.
    .remoteSystems =                           ///< Remote system info list.
    {
        { 1, CLIENT_SYSTEM_ID, "RPC CLIENT SYSTEM" }
    },
    .remoteSystemCnt = 1,                      ///< Remote system count in the list.
    .requestServiceCnt = 0,                    ///< Request service count.
    .offerServiceCnt = 12,                     ///< Offer service count.
    .offerServices =                           ///< Offer service list.
    {
        {
            .service = { 0xED01, "telaf", "taf_radio", "ANY_VERSION" },
            .port = { true, 42001 },
            .clientSystems = { CLIENT_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED02, "telaf", "taf_mdc", "ANY_VERSION" },
            .port = { true, 42002 },
            .clientSystems = { CLIENT_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED03, "telaf", "taf_sms", "ANY_VERSION" },
            .port = { true, 42003 },
            .clientSystems = { CLIENT_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED04, "telaf", "taf_voicecall", "ANY_VERSION" },
            .port = { true, 42004 },
            .clientSystems = { CLIENT_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED05, "telaf", "taf_ecall", "ANY_VERSION" },
            .port = { true, 42005 },
            .clientSystems = { CLIENT_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED06, "telaf", "taf_net", "ANY_VERSION" },
            .port = { true, 42006 },
            .clientSystems = { CLIENT_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED07, "telaf", "taf_dcs", "ANY_VERSION" },
            .port = { true, 42007 },
            .clientSystems = { CLIENT_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED08, "telaf", "taf_pm", "ANY_VERSION" },
            .port = { true, 42008 },
            .clientSystems = { CLIENT_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED09, "telaf", "taf_gnss", "ANY_VERSION" },
            .port = { true, 42009 },
            .clientSystems = { CLIENT_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED0A, "telaf", "taf_pos", "ANY_VERSION" },
            .port = { true, 42010 },
            .clientSystems = { CLIENT_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED0B, "telaf", "taf_posCtrl", "ANY_VERSION" },
            .port = { true, 42011 },
            .clientSystems = { CLIENT_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED0C, "root", "printer", "ANY_VERSION" },
            .port = { true, 42012 },
            .clientSystems = { CLIENT_SYSTEM_ID },
            .systemCnt = 1
        }
    }
};
#endif

#ifdef RPC_CLIENT_TEST
static RpcConfigData_t MyConfiguration =
{
    .mySystem = { 1, CLIENT_SYSTEM_ID, "RPC CLIENT SYSTEM" },      ///< My system info.
    .remoteSystems =                           ///< Remote system info list.
    {
        { 2, SERVER_SYSTEM_ID, "RPC SERVER SYSTEM" },
    },
    .remoteSystemCnt = 1,                      ///< Remote system count in the list.
    .offerServiceCnt = 0,                      ///< Offer service count.
    .requestServiceCnt = 12,                    ///< Request service count.
    .requestServices =
    {
        {
            .service = { 0xED01, "telaf", "taf_radio", "ANY_VERSION" },
            .isReliable = true,
            .serverSystems = { SERVER_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED02, "telaf", "taf_mdc", "ANY_VERSION" },
            .isReliable = true,
            .serverSystems = { SERVER_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED03, "telaf", "taf_sms", "ANY_VERSION" },
            .isReliable = true,
            .serverSystems = { SERVER_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED04, "telaf", "taf_voicecall", "ANY_VERSION" },
            .isReliable = true,
            .serverSystems = { SERVER_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED05, "telaf", "taf_ecall", "ANY_VERSION" },
            .isReliable = true,
            .serverSystems = { SERVER_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED06, "telaf", "taf_net", "ANY_VERSION" },
            .isReliable = true,
            .serverSystems = { SERVER_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED07, "telaf", "taf_dcs", "ANY_VERSION" },
            .isReliable = true,
            .serverSystems = { SERVER_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED08, "telaf", "taf_pm", "ANY_VERSION" },
            .isReliable = true,
            .serverSystems = { SERVER_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED09, "telaf", "taf_gnss", "ANY_VERSION" },
            .isReliable = true,
            .serverSystems = { SERVER_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED0A, "telaf", "taf_pos", "ANY_VERSION" },
            .isReliable = true,
            .serverSystems = { SERVER_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED0B, "telaf", "taf_posCtrl", "ANY_VERSION" },
            .isReliable = true,
            .serverSystems = { SERVER_SYSTEM_ID },
            .systemCnt = 1
        },

        {
            .service = { 0xED0C, "root", "printer", "ANY_VERSION" },
            .isReliable = true,
            .serverSystems = { SERVER_SYSTEM_ID },
            .systemCnt = 1
        }
    }
};
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Load and parse RPC JSON file.
 */
//--------------------------------------------------------------------------------------------------
static void StartParsingJSON
(
    const char* filePathPtr ///< [IN] The full path of the JSON file.
)
{
    LE_ASSERT(filePathPtr != NULL);

    // <TBD>
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * The async queue function for loading RPC configuration.
 */
//--------------------------------------------------------------------------------------------------
static void LoadRpcConfigFunc
(
    void* param1Ptr,    ///< [IN] param 1, user callback.
    void* param2Ptr     ///< [IN] param 2, config file path.
)
{
    le_result_t result;
    RpcConfigCallbackFunc_t userCallback = (RpcConfigCallbackFunc_t)param1Ptr;
    const char* filePathPtr = (const char*)param2Ptr;

    // Start parsing the JSON file if file path is provided.
    if (filePathPtr != NULL)
    {
        LE_INFO("Loading RPC configuration file: %s", filePathPtr);
        StartParsingJSON(filePathPtr);
        return;
    }

    // Using static coniguration if JSON file path is empty, and directly call the userCallback.
    IsConfigurationLoaded = true;
    result = LE_OK;
    userCallback(result, &MyConfiguration);

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Dump all RPC configurations.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyConfig_ShowConfiguration
(
    void
)
{
    int i, j;

    if (!IsConfigurationLoaded)
    {
        LE_ERROR("RPC configuration is not loaded yet.");
        return;
    }

    LE_INFO("=========================================================");
    LE_INFO("====================RPC configuration====================");
    LE_INFO("=========================================================");

    // Dump my system info.
    LE_INFO("My system index: %d", MyConfiguration.mySystem.index);
    LE_INFO("My system ID: 0x%x", MyConfiguration.mySystem.id);
    LE_INFO("My system name: '%s'", MyConfiguration.mySystem.name);
    LE_INFO(" ");
    LE_INFO(" ");

    LE_INFO("--------------------Remote system list-------------------");
    for (i = 0; i < MyConfiguration.remoteSystemCnt; i++)
    {
        LE_INFO("Remote system entry:");
        LE_INFO("    System index: %d", MyConfiguration.remoteSystems[i].index);
        LE_INFO("    System ID: 0x%x", MyConfiguration.remoteSystems[i].id);
        LE_INFO("    System name: '%s'", MyConfiguration.remoteSystems[i].name);
        LE_INFO(" ");
    }
    LE_INFO("---------------------------------------------------------");
    LE_INFO(" ");

    // Dump offer service list.
    LE_INFO("--------------------Offer service list-------------------");
    for (i = 0; i < MyConfiguration.offerServiceCnt; i++)
    {
        LE_INFO("Offer service entry:");
        LE_INFO("    Service ID: 0x%x", MyConfiguration.offerServices[i].service.id);
        LE_INFO("    Service name: '%s'", MyConfiguration.offerServices[i].service.name);
        LE_INFO("    Service user: '%s'", MyConfiguration.offerServices[i].service.user);
        LE_INFO("    Service version: '%s'", MyConfiguration.offerServices[i].service.protocolId);
        LE_INFO("    Network config: '%s' port %d",
            MyConfiguration.offerServices[i].port.isReliable ? "TCP" : "UDP",
            MyConfiguration.offerServices[i].port.number);
        LE_INFO("    Client system list:");
        for (j = 0; j < MyConfiguration.offerServices[i].systemCnt; j++)
        {
            LE_INFO("        System ID: 0x%x", MyConfiguration.offerServices[i].clientSystems[j]);
        }
        LE_INFO(" ");
    }
    LE_INFO("---------------------------------------------------------");
    LE_INFO(" ");

    // Dump request service list.
    LE_INFO("--------------------request service list-----------------");
    for (i = 0; i < MyConfiguration.requestServiceCnt; i++)
    {
        LE_INFO("request service entry:");
        LE_INFO("    Service ID: 0x%x", MyConfiguration.requestServices[i].service.id);
        LE_INFO("    Service name: '%s'", MyConfiguration.requestServices[i].service.name);
        LE_INFO("    Service user: '%s'", MyConfiguration.requestServices[i].service.user);
        LE_INFO("    Service version: '%s'", MyConfiguration.requestServices[i].service.protocolId);
        LE_INFO("    Server system list:");
        for (j = 0; j < MyConfiguration.requestServices[i].systemCnt; j++)
        {
            LE_INFO("        System ID: 0x%x", MyConfiguration.requestServices[i].serverSystems[j]);
        }
        LE_INFO(" ");
    }
    LE_INFO("--------------------------------------------------------");
    LE_INFO(" ");
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the RPC configuration from the JSON file. The configuration callback function will be called
 * once JSON file parsing is done.
 */
//--------------------------------------------------------------------------------------------------
void rpcProxyConfig_LoadConfiguration
(
    char* filePathPtr,                  ///< [IN] The full path of the JSON file.
    RpcConfigCallbackFunc_t funcPtr     ///< [IN] The callback function pointer.
)
{
    if ((IsConfigurationLoaded) || (IsJSONParsingInProgress))
    {
        LE_WARN("Configuration is already loaded or loading is in progress.");
        return;
    }

    if (funcPtr == NULL)
    {
        LE_FATAL("function callback is NULL.");
    }

    // Start loading configuration in aysnc mode.
    le_event_QueueFunction(LoadRpcConfigFunc, (void*)funcPtr, (void*)filePathPtr);

    if (filePathPtr != NULL)
    {
        // Loading JSON file is in progress.
        IsJSONParsingInProgress = true;
    }
}
