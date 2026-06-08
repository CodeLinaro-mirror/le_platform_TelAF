/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EcallController.hpp"

#include "ApiEcall.hpp"
#include "EcallMgrEcall.hpp"
#include "CpuProfiler.hpp"

namespace ecall
{

EcallController& EcallController::GetInstance()
{
    static EcallController inst;
    return inst;
}

EcallController::EcallController()  = default;
EcallController::~EcallController() = default;

le_result_t EcallController::StartAutomatic()
{
    EcallMgrEcall::GetInstance().StartEcall(EcCallType::Automatic);
    return LE_OK;
}

le_result_t EcallController::StartManual()
{
    EcallMgrEcall::GetInstance().StartEcall(EcCallType::Manual);
    return LE_OK;
}

le_result_t EcallController::StartTest()
{
    EcallMgrEcall::GetInstance().StartEcall(EcCallType::Test);
    return LE_OK;
}

le_result_t EcallController::End()
{
    return ApiEcall::GetInstance().End();
}

le_result_t EcallController::CpuStart()
{
    StartCpuProfiler();
    return LE_OK;
}

le_result_t EcallController::CpuStop()
{
    StopCpuProfiler();
    return LE_OK;
}

void EcallController::StartCpuProfiler(uint32_t    periodMs,
                                       std::size_t topN,
                                       bool        includeThreads)
{
    CpuProfiler::GetInstance().Start(periodMs, topN, includeThreads);
}

void EcallController::StopCpuProfiler()
{
    CpuProfiler::GetInstance().Stop();
}

} // namespace ecall
