/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

/**
 * @page c_tafDiagDoIP Diag DoIP Service
 *
 * @ref taf_diagDoIP_interface.h "API Reference"
 *
 * <HR>
 *
 * The diag DoIP service APIs are part of the diagnostic service. The diag DoIP service
 * provides functionality to access diag DoIP stack at runtime, including DoIP configuration
 * and event notification. The configuration APIs can be used to set VIN, EID and GID. The
 * event notification APIs can be used to get the client connection and disconnection event.
 *
 * @section taf_diagDoIP_binding IPC interfaces binding
 *
 * The functions of this API are provided by the @b tafDiagSvc platform service.
 *
 * The following example illustrates how to bind to the diag DoIP service.
 *
 * @verbatim
   bindings:
   {
        clientExe.clientComponent.taf_diagDoIP -> tafDiagSvc.taf_diagDoIP
   }
   @endverbatim
 *
 * @section c_taf_diagDoIP_server_API Server Service APIs
 *
 * A diag DoIP service reference can be got using taf_diagDoIP_GetService().
 * Use the returned reference for subsequent operations.
 *
 * The following example illustrates how to set up a diag DoIP server-service-instance.
 *
 * @code
 *   taf_diagDoIP_ServiceRef_t svcRef;    // Service reference
 *
 *   // Get the service reference.
 *   svcRef = taf_diagDoIP_GetService(sa);
 *   LE_ASSERT(svcRef != NULL);
 *   @endcode
 *
 * @section c_taf_diag_doip_config DoIP stack configuration
 *
 * an application can call taf_diagDoIP_SetVIN()/taf_diagDoIP_GetVIN()
 * to set/get vehicle Identification Number, call taf_diagDoIP_SetEID()
 * /taf_diagDoIP_GetEID() to set/get entity Identification, call
 * taf_diagDoIP_SetGID()/taf_diagDoIP_GetGID() to set/get group Identification.
 *
 * @code
 *   le_result_t result;
 *   char vin[TAF_DIAGDOIP_VIN_SIZE+1] = "xxxxxxxxxxxxxxxxx";
 *   char eid[TAF_DIAGDOIP_EID_SIZE+1] = "xxxxxxxxxxxx";
 *   char gid[TAF_DIAGDOIP_GID_SIZE+1] = "xxxxxxxxxxxx";
 *
 *   result = taf_diagDoIP_SetVIN(vin);
 *   if (result != LE_OK)
 *   {
 *      LE_ERROR("Fail to set vin.");
 *      return result;
 *   }
 *
 *   result = taf_diagDoIP_SetEID(eid);
 *   if (result != LE_OK)
 *   {
 *      LE_ERROR("Fail to set eid.");
 *      return result;
 *   }
 *
 *   result = taf_diagDoIP_SetGID(gid);
 *   if (result != LE_OK)
 *   {
 *      LE_ERROR("Fail to set gid.");
 *      return result;
 *   }
 *
 *   result = taf_diagDoIP_GetVIN(vin, TAF_DIAGDOIP_VIN_SIZE+1);
 *   if (result != LE_OK)
 *   {
 *      LE_ERROR("Fail to get vin.");
 *      return result;
 *   }
 *
 *   result = taf_diagDoIP_SetEID(eid, TAF_DIAGDOIP_EID_SIZE+1);
 *   if (result != LE_OK)
 *   {
 *      LE_ERROR("Fail to get eid.");
 *      return result;
 *   }
 *
 *   result = taf_diagDoIP_GetGID(gid, TAF_DIAGDOIP_GID_SIZE+1);
 *   if (result != LE_OK)
 *   {
 *      LE_ERROR("Fail to get gid.");
 *      return result;
 *   }
 *   @endcode
 *
 * @section c_taf_diag_doip_event DoIP entity event notification
 *
 * After getting the service reference, an application can call
 * taf_diagDoIP_AddEventHandler() to register an event handler for DoIP stack.
 * Once a connection event occurs, the handler will be called with the event reference,
 * event type and remote logical address.
 *
 * @code
 *   // Rx event handler function for DoIP service.
 *   void EventHandler
 *   (
 *        taf_diagDoIP_ServiceRef_t svcRef,
 *        taf_diagDoIP_EventType_t eventType,
 *        uint16_t remoteLogicAddr,
 *        void* contextPtr
 *   )
 *   {
 *        // Process after succesfully event notify
 *   }
 *
 *   // Register the event handler.
 *   taf_diagDoIP_EventHandlerRef_t eventHandlerRef =
 *       taf_diagDoIP_AddEventHandler(SvcRef,
 *            (taf_diagDoIP_EventHandlerFunc_t)EventHandler, NULL);
 *   LE_ASSERT(eventHandlerRef != NULL);
 *
 *   // To remove the handler function.
 *   taf_diagDoIP_RemoveEventHandler(eventHandlerRef);
 *   @endcode
 *
 * Finally call taf_diagDoIP_RemoveSvc() to remove the created service.
 *
 * <HR>
 *
 */


#include "legato.h"
#include "interfaces.h"
#include "tafDiagDoIPSvr.hpp"

using namespace telux::tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference of a diag DoIP service. If there is no diag DoIP service, a new one will be
 * created.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to create the service.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
taf_diagDoIP_ServiceRef_t taf_diagDoIP_GetService
(
    uint16_t Identifier
        ///< [IN] For DoIP node, it is logical address.
)
{
    auto& doipSvc = taf_DiagDoIPSvr::GetInstance();

    return doipSvc.FindOrCreateService(Identifier);
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the VIN(Vehicle Identification Number) into DoIP stack.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid VIN size.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_FAULT -- Internal error.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDoIP_SetVIN
(
    const char* LE_NONNULL vin
        ///< [IN] VIN.
)
{
    auto& doipSvc = taf_DiagDoIPSvr::GetInstance();

    return doipSvc.SetVIN(vin);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the VIN(Vehicle Identification Number) from DoIP stack.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid VIN size.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_FAULT -- Internal error.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDoIP_GetVIN
(
    char* vin,
        ///< [OUT] VIN.
    size_t vinSize
        ///< [IN]
)
{
    auto& doipSvc = taf_DiagDoIPSvr::GetInstance();

    return doipSvc.GetVIN(vin, vinSize);
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the EID(Entify Identification) into DoIP stack.
 *
 * @note if the EID is "a2:8c:ed:3e:38:14", please input "a28ced3e3814".
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid EID size.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_FAULT -- Internal error.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDoIP_SetEID
(
    const char* LE_NONNULL eid
        ///< [IN] EID.
)
{
    auto& doipSvc = taf_DiagDoIPSvr::GetInstance();

    return doipSvc.SetEID(eid);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the EID(Entify Identification) from DoIP stack.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid EID size.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_FAULT -- Internal error.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDoIP_GetEID
(
    char* eid,
        ///< [OUT] EID.
    size_t eidSize
        ///< [IN]
)
{
    auto& doipSvc = taf_DiagDoIPSvr::GetInstance();

    return doipSvc.GetEID(eid, eidSize);
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the GID(Group Identification) into DoIP stack.
 *
 * @note if the GID is "a2:8c:ed:3e:38:14", please input "a28ced3e3814".
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid GID size.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_FAULT -- Internal error.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDoIP_SetGID
(
    const char* LE_NONNULL gid
        ///< [IN] GID.
)
{
    auto& doipSvc = taf_DiagDoIPSvr::GetInstance();

    return doipSvc.SetGID(gid);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the GID(Group Identification) from DoIP stack.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid GID size.
 *     - LE_NOT_FOUND -- Reference not found.
 *     - LE_FAULT -- Internal error.
 *
 * @note The process exits if an invalid reference is passed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDoIP_GetGID
(
    char* gid,
        ///< [OUT] GID.
    size_t gidSize
        ///< [IN]
)
{
    auto& doipSvc = taf_DiagDoIPSvr::GetInstance();

    return doipSvc.GetGID(gid, gidSize);
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_diagDoIP_Event'
 *
 * This event provides DoIP event on DoIP service.
 *
 * @note Only support to register one handler for each client.
 *
 */
//--------------------------------------------------------------------------------------------------
taf_diagDoIP_EventHandlerRef_t taf_diagDoIP_AddEventHandler
(
    taf_diagDoIP_ServiceRef_t svcRef,
        ///< [IN] Service reference.
    taf_diagDoIP_EventHandlerFunc_t handlerPtr,
        ///< [IN] DoIP event handler.
    void* contextPtr
        ///< [IN]
)
{
    auto& doipSvc = taf_DiagDoIPSvr::GetInstance();

    return doipSvc.AddEventHandler(svcRef, handlerPtr, contextPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_diagDoIP_Event'
 */
//--------------------------------------------------------------------------------------------------
void taf_diagDoIP_RemoveEventHandler
(
    taf_diagDoIP_EventHandlerRef_t handlerRef
        ///< [IN]
)
{
    auto& doipSvc = taf_DiagDoIPSvr::GetInstance();

    doipSvc.RemoveEventHandler(handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Removes the diag DoIP service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid svcRef.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_diagDoIP_RemoveSvc
(
    taf_diagDoIP_ServiceRef_t svcRef
        ///< [IN] Service reference.
)
{
    auto& doipSvc = taf_DiagDoIPSvr::GetInstance();

    return doipSvc.RemoveService(svcRef);
}