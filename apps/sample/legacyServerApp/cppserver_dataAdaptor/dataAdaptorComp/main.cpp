/**
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>

#include "dataAdaptor.h"
#include "pthread.h"
extern "C" {
    #include "taf_dataAdaptor_server.h"
}
/**
 * Dumps the available data profiles
 */
void taf_dataAdaptor_DumpProfile(void)
{
    auto &da = DataAdaptor::GetInstance();
    da.DumpDataProfile();
}


/**
 * Main thread for the application
 */
int main(int argc, char** argv)
{
    auto &da = DataAdaptor::GetInstance();

    DataAdaptor::Connect();
    taf_dataAdaptor_AdvertiseService();

    le_msg_AddServiceOpenHandler(taf_dataAdaptor_GetServiceRef(), DataAdaptor::OnClientSessionConnected, NULL);
    le_msg_AddServiceCloseHandler(taf_dataAdaptor_GetServiceRef(), DataAdaptor::OnClientSessionDisconnected, NULL);

    le_result_t result;
    taf_dcs_Pdp_t ipType = TAF_DCS_PDP_IPV4V6;

    da.DumpDataProfile();

    result = da.StartDataCallOnDefaultProfile(ipType);

    da.RegisterEventLoop();

    return 0;
}
