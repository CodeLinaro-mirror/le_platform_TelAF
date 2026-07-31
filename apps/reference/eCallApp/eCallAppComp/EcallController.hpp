/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <cstddef>
#include <cstdint>

extern "C"
{
#include "legato.h"
}

namespace ecall
{

// -----------------------------------------------------------------------------
// EcallController
//
// High-level entry point for all eCall operations.
//
// Serves as the implementation of the ctrlEcall Legato IPC API:
//   ctrlEcall_StartAutomatic / StartManual / StartTest / End / Answer
// are registered in EcallAppMain as the IPC service handlers.
//
// Also exposes CpuStart/CpuStop for internal diagnostics.
//
// Singleton; use GetInstance() for access.
// -----------------------------------------------------------------------------
class EcallController
{
public:
    static EcallController& GetInstance();

    // ctrlEcall IPC service handlers (called by Legato IPC framework)
    le_result_t StartAutomatic();
    le_result_t StartManual();
    le_result_t StartTest();
    le_result_t End();
    le_result_t CpuStart();
    le_result_t CpuStop();

    // CPU profiling (debug / diagnostics)
    void StartCpuProfiler(uint32_t    periodMs       = 10375,
                          std::size_t topN           = 50,
                          bool        includeThreads = true);
    void StopCpuProfiler();

private:
    EcallController();
    ~EcallController();

    EcallController(const EcallController&)            = delete;
    EcallController& operator=(const EcallController&) = delete;
};

} // namespace ecall
