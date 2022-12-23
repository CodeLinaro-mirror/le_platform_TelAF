/**
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"



COMPONENT_INIT
{
    LE_INFO("Data adaptor client init");

    taf_dataAdaptor_DumpProfile();

}