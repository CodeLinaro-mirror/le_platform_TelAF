/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear 
 */

#pragma once

extern "C"
{
    #include "legato.h"
    #include "interfaces.h"
}

#include "EcallDefs.hpp"
#include <optional>
#include <cstdint>

namespace ecall
{

// ApiLoc
// ------
// Wrapper around taf_locGnss_* service.
//
// Lifecycle:
//   - GetInstance() returns the singleton; the GNSS worker thread is started
//     on first access and runs for the lifetime of the process.
//   - The worker thread owns the IPC connection (taf_locGnss_ConnectService),
//     the GNSS session (Start/Stop) and the position handler registration.
//   - Each position callback releases the sample ref after reading all fields.
//   - Publishes LocFix events on EcallEventBus and caches the latest fix.
class ApiLoc
{
public:
    // Get the singleton instance.
    // Starts the GNSS worker thread on first call.
    static ApiLoc& GetInstance();

    // Get the last GNSS fix if available.
    // Returns std::nullopt if no valid fix has been received yet.
    std::optional<LocFix> GetLastFix() const
    {
        return lastFix_;
    }

        // Called from EcallMgrEcall::StartEcall() to start GNSS positioning.
    // Dispatches to the worker thread where the IPC connection lives.
    void StartGnss();

    // Called when T9 timer expires (callback window ends) to stop GNSS.
    // Dispatches to the worker thread where the IPC connection lives.
    void StopGnss();

private:
    ApiLoc();
    ~ApiLoc();

    ApiLoc(const ApiLoc&) = delete;
    ApiLoc& operator=(const ApiLoc&) = delete;

    // Worker thread entry point.
    // Owns the IPC connection, GNSS session and handler registration.
    static void* GnssWorkerThreadFn(void* ctx);

    // Called on the worker thread after IPC connect: only connects service.
    void InitOnWorkerThread();

        // Trampoline queued onto the worker thread by StartGnss().
    static void StartGnssOnWorkerThread(void* ctx, void* param2);

    // Trampoline queued onto the worker thread by StopGnss().
    static void StopGnssOnWorkerThread(void* ctx, void* param2);

        // Report one GNSS fix from the worker thread.
    // Queues delivery onto the main thread, where the event bus publish and
    // lastFix_ update happen so main-thread subscribers receive the event.
    void ReportFix(uint32_t locValidMask,
                   double   latDeg,
                   double   lonDeg,
                   double   hAccM,
                   float    headingDeg,
                   float    magDevDeg,
                   uint8_t  satUsed,
                   uint64_t epochMs);

    // Main-thread delivery trampoline used by ReportFix().
    static void DeliverFixOnMainThread(void* ctx, void* param2);


    // GNSS position callback registered with taf_locGnss_AddPositionHandler.
    // Reads all fields from the sample ref, releases it, then calls ReportFix.
    static void GnssCallback(taf_locGnss_SampleRef_t ref, void* ctx);

        // Main thread that owns the in-process event bus subscribers.
    le_thread_Ref_t mainThreadRef_{nullptr};

    // Worker thread reference (kept for potential future join/stop).
    le_thread_Ref_t workerThreadRef_{nullptr};


    // Handler reference returned by taf_locGnss_AddPositionHandler.
    // Stored so it can be removed on teardown.
    taf_locGnss_PositionHandlerRef_t posHandlerRef_{nullptr};

    // Last known location fix.
    std::optional<LocFix> lastFix_;
};

} // namespace ecall
