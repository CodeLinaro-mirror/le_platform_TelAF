/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TAFSOMEIPGWSVC_HPP
#define TAFSOMEIPGWSVC_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"

#include <chrono>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <vsomeip/vsomeip.hpp>

//--------------------------------------------------------------------------------------------------
/**
 * Get the vsomeip routing manager instance by routing ID.
 */
//--------------------------------------------------------------------------------------------------
std::shared_ptr<vsomeip::application>& someip_GetRoutingManager
(
    uint8_t id
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the vsomeip client ID by routing ID.
 */
//--------------------------------------------------------------------------------------------------
uint16_t someip_GetClientId
(
    uint8_t id
);

#endif /* #ifndef TAFSOMEIPGWSVC_HPP */
