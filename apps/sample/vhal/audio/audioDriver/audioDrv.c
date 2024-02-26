/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafHalAudio.h"

static le_result_t taf_hal_CtlSetAudioStatus(bool status,
     uint32_t route, taf_hal_audio_Mode mode)
{
    LE_INFO("AudioTestDrv: %s", __FUNCTION__);
    LE_INFO("status : %s route : %d mode : %d", status ? "true" : "false", route, mode);
    return LE_OK;
}

static void taf_hal_PowerOn()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return;
}

static void taf_hal_PowerOff()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return;
}

static int taf_hal_HwInit()
{
    LE_INFO("TestDrv: %s", __FUNCTION__);
    return 0;
}

static int taf_hal_SelfTest()
{
    LE_INFO("AudioTestDrv: %s", __FUNCTION__);
    return 0;
}

static void* taf_hal_GetModInf(void)
{
    LE_INFO("AudioTestDrv: %s", __FUNCTION__);
    return &(TAF_HAL_INFO_TAB.audioInf);
}

static void Init(void)
{
    LE_INFO("Audio Test Driver init");
}

LE_SHARED audio_InfoTab_t TAF_HAL_INFO_TAB = {
    // always come first
    .mgrInf = {
        .name = TAF_AUDIO_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .hwInitInf = taf_hal_HwInit,
        .powerOffInf = taf_hal_PowerOff,
        .powerOnInf = taf_hal_PowerOn,
        .selfTest = taf_hal_SelfTest, // tafModule will send test command
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

    .audioInf = {
        .InitHAL = Init,
        .CtlSetAudioStatus = taf_hal_CtlSetAudioStatus,
    },
};

// DOT NOT USE COMPONET_INIT define module init for your service/apps
COMPONENT_INIT
{
    // Do not put your specfici init in here, define your init
    LE_INFO("Audio Drv is loading\n");
}
