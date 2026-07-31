/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

extern "C"
{
#include "legato.h"
#include "interfaces.h"
}

#include "EcallEventBus.hpp"
#include "ApiEcall.hpp"
#include "ApiLoc.hpp"
#include "ApiRadio.hpp"
#include "ApiHms.hpp"
#include "ApiDiag.hpp"
#include "ApiAudio.hpp"
#include "ApiMngdPm.hpp"
#include "EcallController.hpp"

#include "EcallMgrEcall.hpp"
#include "EcallMgrConfigJsonLoader.hpp"
#include "EcallMgrConfig.hpp"
#include "EcallMgrLocation.hpp"
#include "EcallMgrNetwork.hpp"
#include "EcallMgrDiag.hpp"
#include "EcallMgrPower.hpp"
#include "EcallMgrDataLogger.hpp"

using namespace ecall;

//------------------------------------------------------------------------------
// COMPONENT_INIT
//
// Legato/TelAF component entry point.
//
// Initialization order:
//   1) Global event bus
//   2) Low-level IPC adapters (Api*)
//   3) Business logic managers (EcallMgr*)
//   4) EcallController — IPC service handlers are registered above as
//      ctrlEcall_* free functions; EcallController is initialized here
//      so it is ready before any IPC call arrives.
//------------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// Non-exit disconnect handlers for services used on the main thread.
//
// Legato auto-connects required services on the main thread at startup.
// By default, if a server disconnects the client process exits immediately.
// Registering a non-exit handler overrides that behaviour so the app stays
// alive and the per-worker reconnect timers can recover the connection.
// -----------------------------------------------------------------------------
static void OnMainThreadAudioDisconnect(void* ctx)
{
    (void)ctx;
    LE_WARN("eCallApp[main]: taf_audio disconnected; app will stay alive");
}

COMPONENT_INIT
{
    LE_INFO("eCall app starting");

    // Override default exit-on-disconnect for services that have a worker-thread
    // reconnect path.  Must be called before any worker thread ConnectService.
    taf_audio_SetNonExitServerDisconnectHandler(OnMainThreadAudioDisconnect, nullptr);

    // 1) Global event bus.
    EcallEventBus::GetInstance();

    // 2) Low-level IPC adapters.
    ApiEcall::GetInstance();
    ApiLoc::GetInstance();
    ApiRadio::GetInstance();
    ApiDiag::GetInstance();
    ApiAudio::GetInstance();
    ApiHms::GetInstance();
    ApiMngdPm::GetInstance();

    // 3) Business logic managers.
    //    Config must be loaded before EcallMgrEcall starts.
    EcallMgrConfigJsonLoader::GetInstance();
    EcallMgrConfig::GetInstance();
    EcallMgrLocation::GetInstance();
    EcallMgrNetwork::GetInstance();
    EcallMgrPower::GetInstance();
    EcallMgrDiag::GetInstance();
    EcallMgrDataLogger::GetInstance();
    EcallMgrEcall::GetInstance();

    // 4) IPC service entry point.
    EcallController::GetInstance();

    LE_INFO("eCall app ready");
}
