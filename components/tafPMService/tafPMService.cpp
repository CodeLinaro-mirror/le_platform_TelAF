/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * @file       tafPMService.cpp
 * @brief      This file provides the taf power manager service as interfaces described
 *             in taf_pm.api. The power manager service will be started automatically.
 */
#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include "tafPM.hpp"

using namespace telux::common;
using namespace telux::power;
using namespace telux::tafsvc;

COMPONENT_INIT
{
    auto &power = taf_PM::GetInstance();
    power.Init();

    // install the handler
    taf_Handler myHandler;
}

/**
* FUNCTION     : NewWakeupSource
* DESCRIPTION  : Creates wakeup source
* DEPENDECY    :
* PARAMETERS   : Wakeup source options and wakeup source tag
* RETURN VALUES: wakeup source reference
*/
taf_pm_WakeupSourceRef_t taf_pm_NewWakeupSource(uint32_t opts, const char *tag)
{
    LE_DEBUG("taf_pm_NewWakeupSource\n");
    auto &power = taf_PM::GetInstance();
    return  power.NewWakeupSource(opts, tag);
}

/**
* FUNCTION     : StayAwake
* DESCRIPTION  : Acquires the wakeup source
* DEPENDECY    :
* PARAMETERS   : wakeup source reference
* RETURN VALUES: LE_OK on success, LE_NO_MEMORY on reaching the limit, LE_FAULT for all errors
*/
le_result_t taf_pm_StayAwake( taf_pm_WakeupSourceRef_t w)
{
    LE_DEBUG("taf_pm_StayAwake \n");
    auto &power = taf_PM::GetInstance();
    return  power.StayAwake(w);
}

/**
* FUNCTION     : Relax
* DESCRIPTION  : Releases the previously acquired reference
* DEPENDECY    :
* PARAMETERS   : wakeup source reference
* RETURN VALUES: LE_OK on success, LE_NOT_FOUND if wakeup source is not acquired currently,
*                LE_FAULT for all errors
*/
le_result_t taf_pm_Relax( taf_pm_WakeupSourceRef_t w)
{
    LE_DEBUG("taf_pm_Relax \n");
    auto &power = taf_PM::GetInstance();
    return  power.Relax(w);
}

/**
* FUNCTION     : ForceRelaxAndDestroyAllWakeupSource
* DESCRIPTION  : Release and destroy all acquired wakeup source, kill all clients
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_NOT_PERMITTED if taf_powermgr_StayAwake has not failed with
*                LE_NO_MEMORY, LE_FAULT for all errors
*/
le_result_t taf_pm_ForceRelaxAndDestroyAllWakeupSource()
{
    LE_DEBUG("taf_pm_ForceRelaxAndDestroyAllWakeupSource");
    auto &power = taf_PM::GetInstance();
    return  power.ForceRelaxAndDestroyAllWakeupSource();
}

/**
* FUNCTION     : GetPowerState
* DESCRIPTION  : gives the current TCU state
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: TCU state
*/
taf_pm_State_t taf_pm_GetPowerState()
{
    LE_DEBUG("taf_GetPowerState \n");
    auto &power = taf_PM::GetInstance();
    taf_pm_State_t state = power.GetPowerState();
    return state;
}

/**
* FUNCTION     : AddStateChangeHandle
* DESCRIPTION  : send state change notification
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: LE_OK on success, LE_FAULT for all errors
*/
taf_pm_StateChangeHandlerRef_t taf_pm_AddStateChangeHandler
(taf_pm_StateChangeHandlerFunc_t handlerPtr, void* contextPtr)
{
    LE_DEBUG("AddStateChangeHandler in Service class");
    auto &power = taf_PM::GetInstance();
    return power.AddStateChangeHandler(handlerPtr, contextPtr);
}

/**
* FUNCTION     : RemoveStateChangeHandler
* DESCRIPTION  : remove state change handler
* DEPENDECY    :
* PARAMETERS   : state change handler reference
* RETURN VALUES:
*/
void taf_pm_RemoveStateChangeHandler(taf_pm_StateChangeHandlerRef_t handlerRef)
{
   LE_DEBUG("taf_pm_RemoveStateChangeHandler");
   auto &power = taf_PM::GetInstance();
   power.RemoveStateChangeHandler(handlerRef);
}

