/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "tafIvssMngdSvc.hpp"

using namespace v0::com::qualcomm::qti::modem;

//--------------------------------------------------------------------------------------------------
/**
 * Get the single instance of TelAF Ivss Mngd server.
 */
//--------------------------------------------------------------------------------------------------
std::shared_ptr<tafIvssMngdSvc> tafIvssMngdSvc::GetInstance()
{
    static std::shared_ptr<tafIvssMngdSvc> instance = std::make_shared<tafIvssMngdSvc>();
    return instance;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization.
 */
//--------------------------------------------------------------------------------------------------
void tafIvssMngdSvc::Init
(
    void
)
{
    // Init the memory pool
    EventPool = le_mem_CreatePool("Ivss Mngd EventPool", sizeof(taf_IvssMngd_Ind_t));

    LE_INFO("tafIvssMngdSvc Service initialized");
};
