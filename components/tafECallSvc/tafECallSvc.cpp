/*
* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following
* disclaimer in the documentation and/or other materials provided
* with the distribution.
*
* * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
* contributors may be used to endorse or promote products derived
* from this software without specific prior written permission.
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

#include "tafECall.hpp"

using namespace telux::tel;
using namespace telux::common;
using namespace telux::tafsvc;
using namespace std;

COMPONENT_INIT
{
    LE_INFO("tafECall service Init...\n");
    auto &ecall = taf_ecall::GetInstance();
    ecall.Init();
    LE_INFO(" tafECall service Ready...\n");
}

/*======================================================================

 FUNCTION        taf_ecall_AddStateChangeHandler

 DESCRIPTION     Add handler to receive change in ecall state

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS

 RETURN VALUE

 SIDE EFFECTS

======================================================================*/
taf_ecall_StateChangeHandlerRef_t taf_ecall_AddStateChangeHandler (taf_ecall_StateChangeHandlerFunc_t handlerPtr,
        void* contextPtr){

    le_event_HandlerRef_t handlerRef;
    auto &ecall = taf_ecall::GetInstance();
    handlerRef = (le_event_HandlerRef_t)ecall.AddStateChangeHandler(handlerPtr, contextPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_ecall_StateChangeHandlerRef_t)(handlerRef);

}

/*======================================================================

 FUNCTION        taf_ecall_RemoveStateChangeHandler

 DESCRIPTION

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS

 RETURN VALUE

 SIDE EFFECTS

======================================================================*/
void taf_ecall_RemoveStateChangeHandler (taf_ecall_StateChangeHandlerRef_t handlerRef)
{
    auto &eCall = taf_ecall::GetInstance();
    eCall.RemoveStateChangeHandler(handlerRef);
}

/*======================================================================

 FUNCTION        taf_ecall_Create

 DESCRIPTION     Create eCall reference

 DEPENDENCIES    Initialization of Ecall Service

 PARAMETERS      NA

 RETURN VALUE   taf_ecall_CallRef_t ecall reference

 SIDE EFFECTS

======================================================================*/
taf_ecall_CallRef_t taf_ecall_Create()
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.CreateECallReference();
}

/*======================================================================

 FUNCTION        taf_ecall_Delete

 DESCRIPTION     Call to free up a call reference.

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      callRef: ECall reference

 RETURN VALUE    NA

 SIDE EFFECTS

======================================================================*/
void taf_ecall_Delete(taf_ecall_CallRef_t callRef)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.Delete(callRef);
}

/*======================================================================

 FUNCTION        taf_ecall_ForceOnlyMode

 DESCRIPTION     Switch to mode in which only eCall is allowed.

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [IN] slotId: slot ID

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_ForceOnlyMode
(
 taf_sim_Id_t slotId
)
{
    auto &ecall = taf_ecall::GetInstance();

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_MODEMSERVICE_ECALL_PATH );
    le_cfg_SetInt(iteratorRef, CFG_NODE_OPMODE, (int) TAF_ECALL_MODE_NORMAL);
    le_cfg_CommitTxn(iteratorRef);

    return ecall.SetECallOperatingMode(slotId, TAF_ECALL_MODE_ECALL);
}

/*======================================================================

 FUNCTION        taf_ecall_ForcePersistentOnlyMode

 DESCRIPTION     Switch to mode in which only eCall is allowed and it
                 persists over power cycles.

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [IN] slotId: slot ID

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_ForcePersistentOnlyMode
(
 taf_sim_Id_t slotId
)
{
    auto &ecall = taf_ecall::GetInstance();

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_MODEMSERVICE_ECALL_PATH );
    le_cfg_SetInt(iteratorRef, CFG_NODE_OPMODE, (int) TAF_ECALL_MODE_ECALL);
    le_cfg_CommitTxn(iteratorRef);

    return ecall.SetECallOperatingMode(slotId, TAF_ECALL_MODE_ECALL);
}

/*======================================================================

 FUNCTION        taf_ecall_ExitOnlyMode

 DESCRIPTION     Exits from ECALL mode and switch to NORMAL mode.

 DEPENDENCIE     Initialization of ECall Service

 PARAMETERS      [IN] slotId: slot ID

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_ExitOnlyMode
(
 taf_sim_Id_t slotId
)
{
    auto &ecall = taf_ecall::GetInstance();

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_MODEMSERVICE_ECALL_PATH );
    le_cfg_SetInt(iteratorRef, CFG_NODE_OPMODE, (int) TAF_ECALL_MODE_NORMAL);
    le_cfg_CommitTxn(iteratorRef);

    return ecall.SetECallOperatingMode(slotId, TAF_ECALL_MODE_NORMAL);
}

/*======================================================================

 FUNCTION        taf_ecall_GetConfiguredOperationMode

 DESCRIPTION     Get the ecall operating mode.

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [IN] slotId: slot ID
                 [OUT] opModePtr: pointer of type Opmode

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetConfiguredOperationMode
(
 taf_sim_Id_t slotId,
 taf_ecall_OpMode_t* opModePtr
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.GetECallOperatingMode(slotId, opModePtr);
}

/*======================================================================

 FUNCTION        taf_ecall_SetMsdVersion

 DESCRIPTION     Set MSD version.

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [IN] msdVersion: msd version value

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetMsdVersion
(
    uint32_t msdVersion
)
{
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_MODEMSERVICE_ECALL_PATH );

    le_cfg_SetInt(iteratorRef, CFG_NODE_MSDVERSION, msdVersion);
    le_cfg_CommitTxn(iteratorRef);

    LE_DEBUG("Set MsdVersion to %d", msdVersion);


    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_ecall_GetMsdVersion

 DESCRIPTION     Get Msd version

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [OUT] msdVersion: ptr to save msd version

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetMsdVersion
(
    uint32_t* msdVersion
)
{
    TAF_ERROR_IF_RET_VAL(msdVersion == NULL, LE_BAD_PARAMETER, "msdVersion pointer is NULL");

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn( CFG_MODEMSERVICE_ECALL_PATH );

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_MSDVERSION))
    {
        *msdVersion = le_cfg_GetInt(iteratorRef, CFG_NODE_MSDVERSION, 0);
        LE_DEBUG("MSD version read as %d", *msdVersion);
        le_cfg_CancelTxn(iteratorRef);
        return LE_OK;
    }

    le_cfg_CancelTxn(iteratorRef);
    return LE_FAULT;

}

/*======================================================================

 FUNCTION        taf_ecall_SetVehicleType

 DESCRIPTION     Set vehicle type

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [IN] vehicleType: vehicle type

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetVehicleType (taf_ecall_MsdVehicleType_t vehicleType)
{
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_MODEMSERVICE_ECALL_PATH );

    le_cfg_SetInt(iteratorRef, CFG_NODE_MSDVEHTYPE, vehicleType);
    le_cfg_CommitTxn(iteratorRef);

    LE_DEBUG("Set Vehicle type to %d", vehicleType);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_ecall_GetVehicleType

 DESCRIPTION     Get vehicle type

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [OUT] vehicleTypePtr: Vehicle type

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetVehicleType
(
    taf_ecall_MsdVehicleType_t* vehicleTypePtr
)
{
    TAF_ERROR_IF_RET_VAL(vehicleTypePtr == NULL, LE_BAD_PARAMETER,
            "vehicleTypePtr pointer is NULL");

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn( CFG_MODEMSERVICE_ECALL_PATH );

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_MSDVEHTYPE))
    {
        *vehicleTypePtr = (taf_ecall_MsdVehicleType_t)le_cfg_GetInt(iteratorRef, CFG_NODE_MSDVEHTYPE, 0);
        LE_DEBUG(" vehicleType =  %d", *vehicleTypePtr);
        le_cfg_CancelTxn(iteratorRef);
        return LE_OK;
    }
    le_cfg_CancelTxn(iteratorRef);
    return LE_FAULT;
}

/*======================================================================

 FUNCTION        taf_ecall_SetVIN

 DESCRIPTION     Set vehicle identification number.

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [IN] vin vehicle identification number

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetVIN
(
    const char* vin
)
{
    TAF_ERROR_IF_RET_VAL(strlen(vin) != TAF_ECALL_MAX_VIN_LENGTH, LE_FAULT,
            "VIN length is wrong");

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_MODEMSERVICE_ECALL_PATH );

    le_cfg_SetString(iteratorRef, CFG_NODE_MSDVIN, vin);

    le_cfg_CommitTxn(iteratorRef);

    LE_DEBUG("Set Vehicle Identification Number to %s", vin);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_ecall_GetVIN

 DESCRIPTION     Get vehicle Identification Number.

 DEPENDENCIES    Initialization of ECall service

 PARAMETERS      [IN] const char* vin: vehicle identification Number

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetVIN
(
    char* vin,
    size_t vinNumElements
)
{
    TAF_ERROR_IF_RET_VAL(vin == NULL, LE_BAD_PARAMETER, "vin pointer is NULL");

    TAF_ERROR_IF_RET_VAL(vinNumElements != TAF_ECALL_MAX_VIN_BYTES,
            LE_BAD_PARAMETER, "vin length is wrong");

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn( CFG_MODEMSERVICE_ECALL_PATH);

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_MSDVIN))
    {
        le_cfg_GetString(iteratorRef, CFG_NODE_MSDVIN, vin, TAF_ECALL_MAX_VIN_BYTES, "");

        TAF_ERROR_IF_RET_VAL(strlen(vin) != TAF_ECALL_MAX_VIN_LENGTH, LE_NOT_FOUND, "vin length is wrong");

        LE_DEBUG(" vehicle idendification number =  %s", vin);

        le_cfg_CancelTxn(iteratorRef);

        return LE_OK;
    }

    le_cfg_CancelTxn(iteratorRef);
    return LE_FAULT;
}

/*======================================================================

 FUNCTION        taf_ecall_SetPropulsionType

 DESCRIPTION     Set vehicle propulsion storage type.

 DEPENDENCIES    Initialization of ECall service

 PARAMETERS      [IN] propulsionType: propulsion storage type

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetPropulsionType
(
        taf_ecall_PropulsionStorageType_t propulsionType
)
{

    le_cfg_QuickDeleteNode( CFG_ECALL_PROPULSIONTYPE_PATH );

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn(CFG_ECALL_PROPULSIONTYPE_PATH);
    le_result_t res = LE_FAULT;

    if (TAF_ECALL_PROP_TYPE_GASOLINE_TANK & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_GASOLINE, true);
        res = LE_OK;
    }

    if (TAF_ECALL_PROP_TYPE_DIESEL_TANK & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_DIESEL, true);
        res = LE_OK;
    }

    if (TAF_ECALL_PROP_TYPE_COMPRESSED_NATURALGAS & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_NATURALGAS, true);
        res = LE_OK;
    }

    if (TAF_ECALL_PROP_TYPE_PROPANE_GAS & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_PROPANE, true);
        res = LE_OK;
    }

    if (TAF_ECALL_PROP_TYPE_ELECTRIC & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_ELECTRIC, true);
        res = LE_OK;
    }

    if (TAF_ECALL_PROP_TYPE_HYDROGEN & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_HYDROGEN, true);
        res = LE_OK;
    }

    if (TAF_ECALL_PROP_TYPE_OTHER & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_OTHER, true);
        res = LE_OK;
    }

    if (res == LE_OK)
    {
        le_cfg_CommitTxn(iteratorRef);
        return LE_OK;
    }
    le_cfg_CancelTxn( iteratorRef );
    return LE_FAULT;
}

/*======================================================================

 FUNCTION        taf_ecall_GetPropulsionType

 DESCRIPTION    Read vehicle propulsion storage type.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS      [OUT]propulsionStorageType vehicle propulsion storage type

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetPropulsionType
(
    taf_ecall_PropulsionStorageType_t* propulsionStorageType
)
{
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn( CFG_ECALL_PROPULSIONTYPE_PATH);
    le_result_t res = LE_FAULT;
    taf_ecall_PropulsionStorageType_t propulsionType = 0;

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_GASOLINE))
    {
        if (le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_GASOLINE, false))
        {
            propulsionType |=  TAF_ECALL_PROP_TYPE_GASOLINE_TANK;
            res = LE_OK;
        }
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_DIESEL))
    {
        if (le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_DIESEL, false))
        {
            propulsionType |=  TAF_ECALL_PROP_TYPE_DIESEL_TANK;
            res = LE_OK;
        }
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_NATURALGAS))
    {
        if (le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_NATURALGAS, false))
        {
            propulsionType |=  TAF_ECALL_PROP_TYPE_COMPRESSED_NATURALGAS;
            res = LE_OK;
        }
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_PROPANE))
    {
        if (le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_PROPANE, false))
        {
            propulsionType |=  TAF_ECALL_PROP_TYPE_PROPANE_GAS;
            res = LE_OK;
        }
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_ELECTRIC))
    {
        if (le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_ELECTRIC, false))
        {
            propulsionType |=  TAF_ECALL_PROP_TYPE_ELECTRIC;
            res = LE_OK;
        }
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_HYDROGEN))
    {
        if (le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_HYDROGEN, false))
        {
            propulsionType |=  TAF_ECALL_PROP_TYPE_HYDROGEN;
            res = LE_OK;
        }
    }

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_PROPULSION_OTHER))
    {
        if (le_cfg_GetBool(iteratorRef, CFG_NODE_PROPULSION_OTHER, false))
        {
            propulsionType |=  TAF_ECALL_PROP_TYPE_OTHER;
            res = LE_OK;
        }
    }

    *propulsionStorageType = propulsionType;

    le_cfg_CancelTxn(iteratorRef);
    return res;
}

/*======================================================================

 FUNCTION        taf_ecall_SetMsdPosition

 DESCRIPTION    Set the latitude and longitude.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS     [IN]ecallRef : reference for ecall
                [IN]isTrusted: trusted location or not
                [IN]latitude: latitude value
                [IN]longitude: longitude value
                [IN]direction: direction

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetMsdPosition
(
    taf_ecall_CallRef_t  ecallRef,
    bool                isTrusted,
    int32_t             latitude,
    int32_t             longitude,
    int32_t             direction
)
{
    auto &ecall = taf_ecall::GetInstance();
    ecall.SetMsdPosition(ecallRef, isTrusted,  latitude, longitude, direction);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_ecall_SetMsdPositionN1

 DESCRIPTION    Set the change in latitude and longitude compared
                to the last MSD transmission.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS     [IN]ecallRef : reference for ecall
                [IN]latitudeDeltaN1: change in latitude value
                [IN]longitudeDeltaN1: change longitude value

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetMsdPositionN1
(
    taf_ecall_CallRef_t  ecallRef,
    int32_t     latitudeDeltaN1,
    int32_t     longitudeDeltaN1
)
{
    auto &ecall = taf_ecall::GetInstance();
    ecall.SetMsdPositionN1(ecallRef, latitudeDeltaN1, longitudeDeltaN1);
    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_ecall_SetMsdPositionN2

 DESCRIPTION    Set the change in latitude and longitude compared
                to the last MSD transmission.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS     [IN]ecallRef : reference for ecall
                [IN]latitudeDeltaN2: change in latitude value
                [IN]longitudeDeltaN2: change longitude value

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetMsdPositionN2
(
    taf_ecall_CallRef_t ecallRef,
    int32_t latitudeDeltaN2,
    int32_t longitudeDeltaN2
)
{
    auto &ecall = taf_ecall::GetInstance();
    ecall.SetMsdPositionN2(ecallRef, latitudeDeltaN2, longitudeDeltaN2);
    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_ecall_SetMsdPassengersCount

 DESCRIPTION     Set number of passenger.

 DEPENDENCIESA   Initialization of ECall service

 PARAMETERS      [IN] ecallRef: reference for ecall
                 [IN] passengerCount: number of passenger

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetMsdPassengersCount
(
    taf_ecall_CallRef_t  ecallRef,
    uint32_t passengerCount
)
{
    auto &ecall = taf_ecall::GetInstance();
    ecall.SetMsdPassengersCount(ecallRef, passengerCount);

    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_ecall_SetMsdTxMode

 DESCRIPTION     Set MSD transmission mode.

 DEPENDENCIES    Initialization of ECall service

 PARAMETERS      [IN] TxMode: Transmission mode

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetMsdTxMode
(
    taf_ecall_MsdTransmissionMode_t txMode
)
{
    LE_DEBUG("Set MsdTransmission mode %d", txMode);
    auto &ecall = taf_ecall::GetInstance();
    return ecall.SetMsdTxMode(txMode);
}

/*======================================================================

 FUNCTION        taf_ecall_GetMsdTxMode

 DESCRIPTION    Get MSD transmission mode.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS      [OUT] modePtr: msd transmission mode

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetMsdTxMode
(
    taf_ecall_MsdTransmissionMode_t* modePtr
)
{
    TAF_KILL_CLIENT_IF_RET_VAL(modePtr == NULL, LE_FAULT," TxMode pointer is NULL");

    auto &ecall = taf_ecall::GetInstance();
    return ecall.GetMsdTxMode(modePtr);
}

/*======================================================================

 FUNCTION        taf_ecall_StartTest

 DESCRIPTION    Initiate a test voice eCall with a configured telephone
                number stored in the USIM

 DEPENDENCIESA   Initialization of ECall service

 PARAMETERS      [IN] ecallRef: ecall Reference

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_StartTest
(
    taf_ecall_CallRef_t ecallRef
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.StartECall(ECallCategory::VOICE_EMER_CAT_MANUAL,
                         ECallVariant::ECALL_TEST, ecallRef);
}

/*======================================================================

 FUNCTION        taf_ecall_StartManual

 DESCRIPTION    Start manual emergency call.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS      [IN] ecallRef: ecall reference

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_StartManual
(
    taf_ecall_CallRef_t ecallRef
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.StartECall(ECallCategory::VOICE_EMER_CAT_MANUAL,
                             ECallVariant::ECALL_EMERGENCY, ecallRef);
}

/*======================================================================

 FUNCTION        taf_ecall_StartAutomatic

 DESCRIPTION    Start automatic emergency call.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS      [IN] ecallRef: Reference

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_StartAutomatic
(
    taf_ecall_CallRef_t ecallRef
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.StartECall(ECallCategory::VOICE_EMER_CAT_AUTO_ECALL,
                            ECallVariant::ECALL_EMERGENCY, ecallRef);
}


/*======================================================================

 FUNCTION        taf_ecall_End

 DESCRIPTION     Stop the ongoing ecall session.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS      [IN] ecallRef: Reference

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_End
(
    taf_ecall_CallRef_t ecallRef
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.StopECall(ecallRef);
}


/*======================================================================

 FUNCTION        taf_ecall_ImportMsd

 DESCRIPTION    Import the PDU MSD

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS      [IN] ecallRef: Reference
                 [IN] pdumsd: msd in pdu format
                 [IN] msdLength: length of msd in pdu format

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_ImportMsd
(
    taf_ecall_CallRef_t  ecallRef,
    const uint8_t*       pdumsd,
    size_t               msdLength
)
{
    TAF_ERROR_IF_RET_VAL(pdumsd == NULL, LE_BAD_PARAMETER," msd pointer is NULL");

    TAF_ERROR_IF_RET_VAL(msdLength > TAF_ECALL_MAX_MSD_LENGTH,
                    LE_OVERFLOW, "msd length exceeds max length");

    auto &ecall = taf_ecall::GetInstance();
    return ecall.ImportMsd(ecallRef, pdumsd, msdLength);
}


/*======================================================================

 FUNCTION        taf_ecall_ExportMsd

 DESCRIPTION    Export the PDU MSD.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS      [IN] ecallRef: ecall reference

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_ExportMsd
(
    taf_ecall_CallRef_t    ecallRef,
    uint8_t*               pdumsd,
    size_t*                msdLength
)
{
    TAF_ERROR_IF_RET_VAL(pdumsd == NULL, LE_FAULT," msd pointer is NULL");

    auto &ecall = taf_ecall::GetInstance();
    return ecall.ExportMsd(ecallRef, pdumsd, msdLength);
}


/*======================================================================

 FUNCTION        taf_ecall_SendMsd

 DESCRIPTION    Update the eCall MSD in modem to be sent to Public Safety Answering Point
                (PSAP) when requested.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS      [IN] ecallRef: ecall reference

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SendMsd
(
    taf_ecall_CallRef_t    ecallRef
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.SendMsd(ecallRef);
}

/*======================================================================

 FUNCTION       taf_ecall_GetState

 DESCRIPTION    Get current ECall state

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS      [IN] ecallRef: ecall reference

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
taf_ecall_State_t taf_ecall_GetState
(
    taf_ecall_CallRef_t    ecallRef
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.GetState(ecallRef);
}

/*======================================================================

 FUNCTION       taf_ecall_GetTerminationReason

 DESCRIPTION    Get eCall terminate reason

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS     [IN] ecallRef: ecall reference

 RETURN VALUE   The termination reason

 SIDE EFFECTS

======================================================================*/
taf_ecall_TerminationReason_t taf_ecall_GetTerminationReason

(
    taf_ecall_CallRef_t    ecallRef
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.GetTerminationReason(ecallRef);
}

/*======================================================================

 FUNCTION        taf_ecall_SetPsapNumber

 DESCRIPTION     Set the Public Safely Answering Point telephone number.

 @note That PSAP number is not applied to Manually or Automatically initiated eCall. For those
   modes, an emergency call is launched.

 @warning This function doesn't modify the U/SIM content.

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [IN] psapNumber: PSAP number

 RETURN VALUE    le_result_t
    - LE_OK            On success
    - LE_FAULT         For other failures

 @note If PSAP number is empty or too long (max TAF_SIM_PHONE_NUM_MAX_LEN digits), it is a
   fatal error, the function will not return.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetPsapNumber
(
  const char* psapNumber
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.SetPsapNumber(psapNumber);
}

/*======================================================================

 FUNCTION        taf_ecall_GetPsapNumber

 DESCRIPTION     Get the Public Safely Answering Point telephone number set with
   taf_ecall_SetPsapNumber() function.

 @note That PSAP number is not applied to Manually or Automatically initiated eCall. For those
   modes, an emergency call is launched.

 @warning This function doesn't read the U/SIM content.

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [OUT] psapNumber: Ptr to save PSAP number

 RETURN VALUE             le_result_t
    - LE_OK               On success
    - LE_FAULT            On failures or if le_ecall_SetPsapNumber() has never been called before
    - LE_OVERFLOW         Retrieved PSAP number is too long for the out parameter
    - LE_BAD_PARAMETER    If Psap number is null

 @note If the passed PSAP pointer is NULL, a fatal error is raised and the function will not
   return.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetPsapNumber
(
  char* psapNumber,
  size_t psapNumLength
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.GetPsapNumber(psapNumber, psapNumLength);
}

/*======================================================================

 FUNCTION      taf_ecall_UseUSimNumbers

 When modem is in ECALL_FORCED_PERSISTENT_ONLY_MODE or ECALL_ONLY_MODE, this function
 can be called to request the modem to read the number to dial from the FDN/SDN of the U/SIM.

 @note If FDN directory is updated with new dial numbers, be sure that the SIM card is refreshed.

  @return
   - LE_OK on success
   - LE_FAULT for other failures


 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_UseUSimNumbers
(
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.UseUSimNumbers();
}

/*======================================================================

 FUNCTION      taf_ecall_SetNadDeregistrationTime

 Set eCall deregistration time in NAD (network access device). After end of an emergency call
 the in-vehicle system stays registered in the network for a specific amount of time, defined
 by NAD (network access device) eCall deregistration time.

 @return
  - LE_OK         On success
  - LE_BUSY       An eCall session is in progress
  - LE_FAULT      On failures

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetNadDeregistrationTime
(
    uint16_t deregTime //NAD (network access device) deregistration time in minutes
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.SetNadDeregistrationTime(deregTime);
}

/*======================================================================

 FUNCTION      taf_ecall_GetNadDeregistrationTime

 Get eCall deregistration time of the NAD (network access device) .

 @return
  - LE_OK          On success
  - LE_FAULT       On failures

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetNadDeregistrationTime
(
    uint16_t* deregTime //NAD (network access device) deregistration time in minutes
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.GetNadDeregistrationTime(deregTime);
}

/*======================================================================

 FUNCTION      taf_ecall_TerminateRegistration

 Disconnect network access device (NAD) from the eCall network. After end of an emergency call
 the in-vehicle system continues registered on the network for a period of time to facilate get
 callback from Public Safely Answering Point (PSAP). But when NAD requires to deregister from
 the network this API can be used.

 @return
  - LE_OK         On success
  - LE_FAULT      On failures

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_TerminateRegistration
(
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.TerminateRegistration();
}

