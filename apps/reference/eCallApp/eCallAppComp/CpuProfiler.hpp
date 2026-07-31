/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <atomic>
#include <thread>
#include <cstddef>
#include <cstdint>

namespace ecall
{

// -----------------------------------------------------------------------------
// CpuProfiler
//
// Process-wide CPU profiling utility running in a background thread.
// Periodically scans /proc, computes per-(pid,tid) CPU usage and logs
// TOP-like statistics via LE_INFO.
//
// Previously named EcallMgrCpuProfiling.  Renamed because it is a generic
// debug tool with no dependency on eCall business logic.
//
// Usage:
//   CpuProfiler::GetInstance().Start();   // start with defaults
//   CpuProfiler::GetInstance().Stop();    // stop and join worker thread
//
// Start() is idempotent: if already running it logs and returns.
// -----------------------------------------------------------------------------
class CpuProfiler
{
public:
    static CpuProfiler& GetInstance();

    // Start profiler.
    //   periodMs       – sampling period in milliseconds
    //   topN           – number of top entries to report per cycle
    //   includeThreads – if true, profile individual threads too
    void Start(uint32_t    periodMs       = 10375,
               std::size_t topN           = 50,
               bool        includeThreads = true);

    // Stop profiler and wait for the worker thread to exit.
    // Safe to call multiple times.
    void Stop();

    bool IsRunning() const noexcept
    {
        return running_.load(std::memory_order_relaxed);
    }

private:
    CpuProfiler();
    ~CpuProfiler();

    CpuProfiler(const CpuProfiler&)            = delete;
    CpuProfiler& operator=(const CpuProfiler&) = delete;

    void RunLoop(uint32_t periodMs, std::size_t topN, bool includeThreads);

    std::atomic<bool> running_{false};
    std::thread       worker_{};
};

} // namespace ecall
