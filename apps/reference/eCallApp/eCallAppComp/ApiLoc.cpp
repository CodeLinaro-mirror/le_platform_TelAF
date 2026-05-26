/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ApiLoc.hpp"

#include "EcallEventBus.hpp"

#include <cmath>
#include <cstdio>
#include <ctime>

namespace ecall
{
namespace
{
struct PendingLocFix
{
    ApiLoc* self;
    LocFix  fix;
};
}

// Format epoch milliseconds (UTC) as "YYYY/MM/DD HH:MM:SS:mmm".
static void FormatTimeYMDHMS(uint64_t epochMs, char* buf, size_t len)
{
    time_t s = static_cast<time_t>(epochMs / 1000ULL);
    struct tm tmv{};
    gmtime_r(&s, &tmv);
    int ms = static_cast<int>(epochMs % 1000ULL);

    std::snprintf(buf,
                  len,
                  "%04d/%02d/%02d %02d:%02d:%02d:%03d",
                  tmv.tm_year + 1900,
                  tmv.tm_mon + 1,
                  tmv.tm_mday,
                  tmv.tm_hour,
                  tmv.tm_min,
                  tmv.tm_sec,
                  ms);
}

// static
ApiLoc& ApiLoc::GetInstance()
{
    static ApiLoc inst;
    return inst;
}

// Constructor: spawn the GNSS worker thread.
// All taf_locGnss_* calls are made on that thread so that the IPC
// connection and the Legato RunLoop are co-located, matching the
// pattern required by the taf_locGnss service.
ApiLoc::ApiLoc()
{
    mainThreadRef_ = le_thread_GetCurrent();
    LE_ASSERT(mainThreadRef_ != nullptr);

    workerThreadRef_ = le_thread_Create("ApiLocGnssWorker",
                                        &ApiLoc::GnssWorkerThreadFn,
                                        this);
    LE_ASSERT(workerThreadRef_ != nullptr);
    le_thread_Start(workerThreadRef_);

    LE_INFO("[ApiLoc] GNSS worker thread started (MAS/deg=%d agedThrMs=%u)",
            MAS_PER_DEGREE,
            static_cast<unsigned>(AGED_THR_MS));
}

ApiLoc::~ApiLoc()
{
    // In practice the singleton lives for the process lifetime,
    // but clean up properly in case of an orderly shutdown.
    if (posHandlerRef_)
    {
        taf_locGnss_RemovePositionHandler(posHandlerRef_);
        posHandlerRef_ = nullptr;
    }
    (void)taf_locGnss_Stop();
}

// static
void* ApiLoc::GnssWorkerThreadFn(void* ctx)
{
    auto* self = static_cast<ApiLoc*>(ctx);
    if (!self)
    {
        return nullptr;
    }

    self->InitOnWorkerThread();

    // Run the Legato event loop; position callbacks are delivered here.
    le_event_RunLoop();

    LE_INFO("[ApiLoc] GNSS worker RunLoop exited");
    return nullptr;
}

void ApiLoc::InitOnWorkerThread()
{
        // Connect to the GNSS IPC service on this thread.
    // NOTE: GNSS Start and handler registration are deferred to StartGnss(),
    // which is called from EcallMgrEcall::StartEcall() when an eCall begins.
    taf_locGnss_ConnectService();
    LE_INFO("[ApiLoc] GNSS service connected; waiting for StartGnss() call");
}

void ApiLoc::StartGnss()
{
    // Must be called on the GNSS worker thread so that the IPC connection
    // and the position handler are co-located with the RunLoop.
    le_event_QueueFunctionToThread(workerThreadRef_,
                                   &ApiLoc::StartGnssOnWorkerThread,
                                   this,
                                   nullptr);
}

void ApiLoc::StopGnss()
{
    // Dispatch to the worker thread where the IPC connection lives.
    le_event_QueueFunctionToThread(workerThreadRef_,
                                   &ApiLoc::StopGnssOnWorkerThread,
                                   this,
                                   nullptr);
}

// static
void ApiLoc::StopGnssOnWorkerThread(void* ctx, void* /*param2*/)
{
    auto* self = static_cast<ApiLoc*>(ctx);
    if (!self) return;

    // Remove position handler first to stop receiving callbacks.
    if (self->posHandlerRef_)
    {
        taf_locGnss_RemovePositionHandler(self->posHandlerRef_);
        self->posHandlerRef_ = nullptr;
        LE_INFO("[ApiLoc] StopGnss: position handler removed");
    }
    else
    {
        LE_INFO("[ApiLoc] StopGnss: no handler registered, skip remove");
    }

    // Stop GNSS engine.
    le_result_t rc = taf_locGnss_Stop();
    if (rc != LE_OK)
    {
        LE_WARN("[ApiLoc] StopGnss: taf_locGnss_Stop failed rc=%d", rc);
    }
    else
    {
        LE_INFO("[ApiLoc] StopGnss: GNSS stopped");
    }
}

// static
void ApiLoc::StartGnssOnWorkerThread(void* ctx, void* /*param2*/)
{
    auto* self = static_cast<ApiLoc*>(ctx);
    if (!self) return;

    // Avoid double-start if already registered.
    if (self->posHandlerRef_)
    {
        LE_INFO("[ApiLoc] StartGnss: already started, skip");
        return;
    }

    // Configure 1 Hz acquisition rate.
    le_result_t rc = taf_locGnss_SetAcquisitionRate(1000);
    if (rc != LE_OK)
    {
        LE_WARN("[ApiLoc] SetAcquisitionRate(1000) failed rc=%d", rc);
    }

    // Start GNSS positioning.
    rc = taf_locGnss_Start();
    if (rc != LE_OK)
    {
        LE_ERROR("[ApiLoc] taf_locGnss_Start failed rc=%d – no position fixes will be received", rc);
        return;
    }

    // Register position callback; store the ref for later removal.
    self->posHandlerRef_ = taf_locGnss_AddPositionHandler(&ApiLoc::GnssCallback, self);
    if (!self->posHandlerRef_)
    {
        LE_ERROR("[ApiLoc] taf_locGnss_AddPositionHandler returned NULL");
    }
    else
    {
        LE_INFO("[ApiLoc] GNSS started and position handler registered (%p)", self->posHandlerRef_);
    }
}

void ApiLoc::ReportFix(uint32_t locValidMask,
                       double   latDeg,
                       double   lonDeg,
                       double   hAccM,
                       float    headingDeg,
                       float    magDevDeg,
                       uint8_t  satUsed,
                       uint64_t epochMs)
{
    auto* pending = new (std::nothrow) PendingLocFix{
        this,
        LocFix{
            latDeg,
            lonDeg,
            hAccM,
            headingDeg,
            magDevDeg,
            locValidMask,
            satUsed,
            epochMs
        }
    };

    if (!pending)
    {
        LE_ERROR("[ApiLoc] Failed to allocate PendingLocFix");
        return;
    }

    le_event_QueueFunctionToThread(mainThreadRef_,
                                   &ApiLoc::DeliverFixOnMainThread,
                                   pending,
                                   nullptr);
}

// static
void ApiLoc::DeliverFixOnMainThread(void* ctx, void* /*param2*/)
{
    auto* pending = static_cast<PendingLocFix*>(ctx);
    if (!pending)
    {
        return;
    }

    ApiLoc* self = pending->self;
    const LocFix fix = pending->fix;
    delete pending;

    if (!self)
    {
        return;
    }

    EcallEventBus::GetInstance().PublishLocFix(fix);
    self->lastFix_ = fix;

    const uint64_t nowMs = NowUtcMs();
    const bool aged = (fix.epochMs &&
                       nowMs > fix.epochMs &&
                       (nowMs - fix.epochMs) > AGED_THR_MS);

    const char* sourceStr = aged ? "invalid" : "live";

    char tbuf[64] = {0};
    FormatTimeYMDHMS(fix.epochMs ? fix.epochMs : nowMs, tbuf, sizeof(tbuf));

    LE_DEBUG(
        "[ApiLoc][%09llu]: GNSS fix datum:WGS84 source:%s "
        "satellites_used:%u latitude:%.6f longitude:%.6f "
        "hacc:%.2f m heading:%.1f deg valid_mask:0x%08X mag_dev:%.1f deg timestamp:%s",
        static_cast<unsigned long long>(nowMs),
        sourceStr,
        fix.satUsed,
        fix.latDeg,
        fix.lonDeg,
        fix.hAccM,
        fix.headingDeg,
        fix.locValidMask,
        fix.magDevDeg,
        tbuf);
}

// static
void ApiLoc::GnssCallback(taf_locGnss_SampleRef_t ref, void* ctx)
{
    if (!ref)
    {
        LE_WARN("[ApiLoc] GnssCallback: null sample ref");
        return;
    }

    int32_t  latE6        = 0;
    int32_t  lonE6        = 0;
    int32_t  hAccE2       = 0;
    uint32_t locValidMask = 0;
    uint32_t headingE1    = 0;
    int32_t  magDevE1     = 0;
    uint8_t  satUsed      = 0;
    uint8_t  satInView    = 0;
    uint8_t  satTracking  = 0;
    uint64_t epochMs      = 0;

    // Read all fields from the sample ref before releasing it.
    (void)taf_locGnss_GetLocation(ref, &latE6, &lonE6, &hAccE2);
    (void)taf_locGnss_GetLocationInfoValidity(ref, &locValidMask, nullptr);
    (void)taf_locGnss_GetDirection(ref, &headingE1, nullptr);
    (void)taf_locGnss_GetMagneticDeviation(ref, &magDevE1);
    (void)taf_locGnss_GetSatellitesStatus(ref, &satInView, &satTracking, &satUsed);
    (void)taf_locGnss_GetEpochTime(ref, &epochMs);

    // IMPORTANT: release the sample ref as soon as all data has been read.
    // Failing to call this leaks the reference inside the GNSS service.
    taf_locGnss_ReleaseSampleRef(ref);

    double latDeg     = static_cast<double>(latE6)  / 1e6;
    double lonDeg     = static_cast<double>(lonE6)  / 1e6;
    double hAccM      = static_cast<double>(hAccE2) / 1e2;
    float  headingDeg = static_cast<float>(headingE1) / 1e1f;
    float  magDevDeg  = static_cast<float>(magDevE1)  / 1e1f;

    auto* self = static_cast<ApiLoc*>(ctx);
    if (!self)
    {
        return;
    }

    self->ReportFix(locValidMask,
                    latDeg,
                    lonDeg,
                    hAccM,
                    headingDeg,
                    magDevDeg,
                    satUsed,
                    epochMs);
}

} // namespace ecall
