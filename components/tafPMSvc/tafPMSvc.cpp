/*
 * Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       tafPMSvc.cpp
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
* RETURN VALUES: LE_OK on success, LE_FAULT for all errors
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
* RETURN VALUES: LE_OK on success, LE_FAULT for all errors
*/
le_result_t taf_pm_Relax( taf_pm_WakeupSourceRef_t w)
{
    LE_DEBUG("taf_pm_Relax \n");
    auto &power = taf_PM::GetInstance();
    return  power.Relax(w);
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
* RETURN VALUES: handlerRef if registered successfully or else NULL
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

/**
 * FUNCTION     : AddConsolidatedAckInfoHandler
 * DESCRIPTION  : send ConsolidatedAck Info notification
 * DEPENDECY    :
 * PARAMETERS   :
 * RETURN VALUES: handlerRef if registered successfully or else NULL
 */
taf_pm_ConsolidatedAckInfoHandlerRef_t taf_pm_AddConsolidatedAckInfoHandler
(taf_pm_ConsolidatedAckInfoHandlerFunc_t handlerPtr, void* contextPtr)
{
    LE_DEBUG("ConsolidatedAckInfoHandler in Service class");
    auto &power = taf_PM::GetInstance();
    return power.AddConsolidatedAckInfoHandler(handlerPtr, contextPtr);
}

/**
 * FUNCTION     : RemoveConsolidatedAckInfoHandler
 * DESCRIPTION  : remove ConsolidatedAck Info handler
 * DEPENDECY    :
 * PARAMETERS   : ConsolidatedAck Info Handler reference
 * RETURN VALUES:
 */
void taf_pm_RemoveConsolidatedAckInfoHandler(taf_pm_ConsolidatedAckInfoHandlerRef_t handlerRef)
{
   LE_DEBUG("taf_pm_RemoveConsolidatedAckInfoHandler");
   auto &power = taf_PM::GetInstance();
   power.RemoveConsolidatedAckInfoHandler(handlerRef);
}

/**
* FUNCTION     : SetAllVMPowerState
* DESCRIPTION  : Sets the power state to all Virtual Machines
* DEPENDECY    :
* PARAMETERS   : state to be set
* RETURN VALUES: result of state change in SA525M, LE_UNSUPPORTED for others
*/
le_result_t taf_pm_SetAllVMPowerState(taf_pm_State_t state)
{
   LE_DEBUG("taf_pm_SetAllVMPowerState");
#if defined(LE_CONFIG_ENABLE_MULTI_VM_SUPPORT)
   auto &power = taf_PM::GetInstance();
   return power.SetPowerState(state, "ALL_MACHINES");
#endif
   return LE_UNSUPPORTED;
}

/**
* FUNCTION     : SetVMPowerState
* DESCRIPTION  : Sets the power state to the Virtual Machine
* DEPENDECY    :
* PARAMETERS   : state to be set, Virtual machine name for which state to be changed
* RETURN VALUES: result of state change in SA525M, LE_UNSUPPORTED for others
*/
le_result_t taf_pm_SetVMPowerState(taf_pm_State_t state, const char *machineName)
{
   LE_DEBUG("taf_pm_SetVMPowerState");
#if defined(LE_CONFIG_ENABLE_MULTI_VM_SUPPORT)
   auto &power = taf_PM::GetInstance();
   return power.SetPowerState(state, machineName);
#endif
   return LE_UNSUPPORTED;
}

/**
* FUNCTION     : GetMachineList
* DESCRIPTION  : Gets the Virtual Machine List
* DEPENDECY    :
* PARAMETERS   :
* RETURN VALUES: Reference to the Virtual Machine List
*/
taf_pm_VMListRef_t taf_pm_GetMachineList( )
{
   LE_DEBUG("taf_pm_GetVirtualMachineList");
#if defined(LE_CONFIG_ENABLE_MULTI_VM_SUPPORT)
   auto &power = taf_PM::GetInstance();
   return power.GetMachineList();
#endif
   return NULL;
}

/**
* FUNCTION     : GetFirstMachineName
* DESCRIPTION  : Gets the first Virtual Machine Name in the List
* DEPENDECY    :
* PARAMETERS   : [IN] Virtual Machine List reference
                 [OUT] char* : The Virtual Machine Name.
                 [IN] size_t : Virtual Machine Name length.
* RETURN VALUES: LE_OK on success, LE_BAD_PARAMETER on bad param,
                 LE_NOT_FOUND if no machine, LE_UNSUPPORTED if not supported.
*/
le_result_t taf_pm_GetFirstMachineName( taf_pm_VMListRef_t vmListRef,
char* vmNamePtr, size_t vmNamePtrSize )
{
   LE_DEBUG("taf_pm_GetFirstMachineName");
#if defined(LE_CONFIG_ENABLE_MULTI_VM_SUPPORT)
   auto &power = taf_PM::GetInstance();
   return power.GetFirstMachineName(vmListRef, vmNamePtr, vmNamePtrSize);
#endif
   return LE_UNSUPPORTED;
}

/**
* FUNCTION     : GetNextMachineName
* DESCRIPTION  : Gets the next Virtual Machine Name from the List based on current Virtual Machine
* DEPENDECY    :
* PARAMETERS   : Virtual Machine List reference
                 [OUT] char* : The Virtual Machine Name.
                 [IN] size_t : Virtual Machine Name length.
* RETURN VALUES: LE_OK on success, LE_BAD_PARAMETER on bad param,
                 LE_NOT_FOUND on end of list, LE_UNSUPPORTED if not supported.
*/
le_result_t taf_pm_GetNextMachineName( taf_pm_VMListRef_t vmListRef,
char* vmNamePtr, size_t vmNamePtrSize )
{
   LE_DEBUG("taf_pm_GetNextMachineName");
#if defined(LE_CONFIG_ENABLE_MULTI_VM_SUPPORT)
   auto &power = taf_PM::GetInstance();
   return power.GetNextMachineName(vmListRef, vmNamePtr, vmNamePtrSize);
#endif
   return LE_UNSUPPORTED;
}

/**
* FUNCTION     : DeleteMachineList
* DESCRIPTION  : Delete the Virtual Machine List
* DEPENDECY    :
* PARAMETERS   : [IN] taf_pm_VMInfoRef_t : The Virtual Machine Info reference.
                 [OUT] char* : The Virtual Machine Name.
                 [IN] size_t : Virtual Machine Name length.
* RETURN VALUES: LE_OK on success, LE_FAULT on failure, LE_UNSUPPORTED if not supported.
*/
le_result_t taf_pm_DeleteMachineList( taf_pm_VMListRef_t vmListRef )
{
   LE_DEBUG("taf_pm_DeleteMachineList");
#if defined(LE_CONFIG_ENABLE_MULTI_VM_SUPPORT)
   auto &power = taf_PM::GetInstance();
   return power.DeleteMachineList(vmListRef);
#endif
   return LE_UNSUPPORTED;
}

/**
* FUNCTION     : SendStateChangeAck
* DESCRIPTION  : Receives the acktype and ack state from clients
* DEPENDECY    :
* PARAMETERS   : [IN] taf_pm_State_t : Changed State notified to client for ack.
                 [IN] taf_pm_ClientAck_t : Ack type from client for the changed state.
*/
void taf_pm_SendStateChangeAck(taf_pm_PowerStateRef_t powerStateRef,
taf_pm_State_t state, taf_pm_NadVm_t vm_id, taf_pm_ClientAck_t ackType )
{
   LE_INFO("taf_pm_SendStateChangeAck");
#if defined(LE_CONFIG_ENABLE_MULTI_VM_SUPPORT)
   auto &power = taf_PM::GetInstance();
   power.SendStateChangeAck(powerStateRef,state,vm_id,ackType);
#endif
}

/**
* FUNCTION     : AddStateChangeExHandler
* DESCRIPTION  : send state change notification
* DEPENDECY    :
* PARAMETERS   : handlerPtr to be called after state change and contextPtr
* RETURN VALUES: handlerRef if registered successfully or else NULL
*/
taf_pm_StateChangeExHandlerRef_t taf_pm_AddStateChangeExHandler
        (taf_pm_StateChangeExHandlerFunc_t handlerPtr,void* contextPtr)
{
    LE_DEBUG("AddStateChangeExHandler in Service class");
#if defined(LE_CONFIG_ENABLE_MULTI_VM_SUPPORT)
    auto &power = taf_PM::GetInstance();
    return power.AddStateChangeExHandler(handlerPtr, contextPtr);
#endif
    return NULL;
}


/**
 * FUNCTION     : RemoveStateChangeExHandler
 * DESCRIPTION  : remove state change handler
 * DEPENDECY    :
 * PARAMETERS   : state change handler reference to be removed
 * RETURN VALUES:
 */
void taf_pm_RemoveStateChangeExHandler(taf_pm_StateChangeExHandlerRef_t handlerRef)
{
   LE_DEBUG("RemoveStateChangeExHandler");
#if defined(LE_CONFIG_ENABLE_MULTI_VM_SUPPORT)
   auto &power = taf_PM::GetInstance();
   power.RemoveStateChangeExHandler(handlerRef);
#endif
}
/**
 * FUNCTION     : GetNackClientInfo
 * DESCRIPTION  : Gets the info of nacked clients
 * DEPENDECY    :
 * PARAMETERS   : Info reference as input, nackClientPtr and nackClientSize as the output
 * RETURN VALUES:
 */
le_result_t taf_pm_GetNackClientInfo
(
    taf_pm_ConsolidatedAckInfoRef_t consolidatedAckInfoRef,
        ///< [IN] The reference of the consolidated
        ///< acknowledgement information.
    taf_pm_ClientInfo_t* nackClientsPtr,
        ///< [OUT] The nacked clients.
    size_t* nackClientsSizePtr
        ///< [INOUT]
)
{
    auto &pmInstance = taf_PM::GetInstance();
    return pmInstance.GetNackClientInfo(consolidatedAckInfoRef, nackClientsPtr, nackClientsSizePtr);
}
/**
 * FUNCTION     : GetUnrespClientInfo
 * DESCRIPTION  : Gets the info of unresponsive clients
 * DEPENDECY    :
 * PARAMETERS   : Info reference as input, unrespClientPtr and unrespClientSize as the output
 * RETURN VALUES:
 */
le_result_t taf_pm_GetUnrespClientInfo
(
    taf_pm_ConsolidatedAckInfoRef_t consolidatedAckInfoRef,
        ///< [IN] The reference of the consolidated
        ///< acknowledgement information.
    taf_pm_ClientInfo_t* unrespClientsPtr,
        ///< [OUT] The unresponsive clients.
    size_t* unrespClientsSizePtr
        ///< [INOUT]
)
{
    auto &pmInstance = taf_PM::GetInstance();
    return pmInstance.GetUnrespClientInfo(consolidatedAckInfoRef,
                            unrespClientsPtr, unrespClientsSizePtr);
}