/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"
#include "LocationApisTest.h"
#include "AudioApisTest.h"
#include "ECallApisTest.h"
#include "SimApisTest.h"
#include "KeyStoreApisTest.h"
#include "SmsApisTest.h"
#include "GpioApisTest.h"
#include "PMApisTest.h"
#include "RadioApisTest.h"
#include "UpdateApisTest.h"
#include "VoicecallApisTest.h"
#include "CanApisTest.h"
#include "SensorApisTest.h"
#include "FSCryptApisTest.h"
#include "MRCApisTest.h"
#include "HMSApisTest.h"
#include "ThermalApisTest.h"
#include "TimeApisTest.h"
#include "WlanApisTest.h"
#include "RSimApisTest.h"
#include "VerInfoApisTest.h"
#include "FlashAccessApisTest.h"
#include "SomeIPGWApisTest.h"
#include "NetApisTest.h"
#include "DcsApisTest.h"
#include "DevInfoApisTest.h"
#include "DiagApisTest.h"

COMPONENT_INIT
{
   locRetTest_RunApis();
   audioRetTest_RunApis();
   ecallRetTest_RunApis();
   simRetTest_RunApis();
   keystoreRetTest_RunApis();
   smsRetTest_RunApis();
   gpioRetTest_RunApis();
   pmRetTest_RunApis();
   radioRetTest_RunApis();
   updateRetTest_RunApis();
   voicecallRetTest_RunApis();
   canRetTest_RunApis();
   sensorRetTest_RunApis();
   fscryptRetTest_RunApis();
   mrcRetTest_RunApis();
   hmsRetTest_RunApis();
   thermRetTest_RunApis();
   timeRetTest_RunApis();
   wlanRetTest_RunApis();
   rsimRetTest_RunApis();
   verinfoRetTest_RunApis();
   someipgwRetTest_RunApis();
   flashaccessRetTest_RunApis();
   netRetTest_RunApis();
   dcsRetTest_RunApis();
   devinfoRetTest_RunApis();
   diagRetTest_RunApis();
   LE_TEST_INFO("======== LE_TEST_EXIT  ========");
   LE_TEST_EXIT;
}
