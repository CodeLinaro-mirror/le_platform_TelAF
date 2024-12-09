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

    // Add boot KPI marker
    const char *kpi_file = "/sys/kernel/boot_kpi/kpi_values";
    const char *kpi_marker = "L - TelAF eCall service is ready";
    FILE *file = fopen(kpi_file, "w");
    if (file == NULL)
    {
        LE_ERROR("%s does not exist", kpi_file);
        return;
    }

    ecall.eCallInf = (eCall_Inf_t *)taf_devMgr_LoadDrv(TAF_ECALL_MODULE_NAME, nullptr);
    if(ecall.eCallInf == nullptr)
    {
        LE_ERROR("Can not load the driver %s", TAF_ECALL_MODULE_NAME);
        ecall.isDrvPresent = false;
    }
    else // successfully loaded
    {
        LE_INFO("Driver loaded successfully....");
        ecall.isDrvPresent = true;
        // init VHAL module first
        (*(ecall.eCallInf->InitHAL))();
    }

    if (LE_OK != ecall.UpdateMsdVehicleInfo())
    {
        LE_ERROR("Unable to update verhicle info via VHAL");
    }

    if (fwrite(kpi_marker, sizeof(char), strlen(kpi_marker), file) != strlen(kpi_marker))
    {
        LE_ERROR("failed to write %s to %s", kpi_marker, kpi_file);
    }
    fclose(file);
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

 PARAMETERS      [IN] phoneId: Phone ID

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_ForceOnlyMode
(
 uint8_t phoneId
)
{
    auto &ecall = taf_ecall::GetInstance();

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_MODEMSERVICE_ECALL_PATH );
    le_cfg_SetInt(iteratorRef, CFG_NODE_OPMODE, (int) TAF_ECALL_MODE_ECALL);
    le_cfg_CommitTxn(iteratorRef);

    return ecall.SetECallOperatingMode(phoneId, TAF_ECALL_MODE_ECALL);
}

/*======================================================================

 FUNCTION        taf_ecall_ForcePersistentOnlyMode

 DESCRIPTION     Switch to mode in which only eCall is allowed and it
                 persists over power cycles.

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [IN] phoneId: Phone ID

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_ForcePersistentOnlyMode
(
 uint8_t phoneId
)
{
    auto &ecall = taf_ecall::GetInstance();

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_MODEMSERVICE_ECALL_PATH );
    le_cfg_SetInt(iteratorRef, CFG_NODE_OPMODE, (int) TAF_ECALL_MODE_FORCED_PERSISTENT_ONLY);
    le_cfg_CommitTxn(iteratorRef);

    return ecall.SetECallOperatingMode(phoneId, TAF_ECALL_MODE_ECALL);
}

/*======================================================================

 FUNCTION        taf_ecall_ExitOnlyMode

 DESCRIPTION     Exits from ECALL mode and switch to NORMAL mode.

 DEPENDENCIE     Initialization of ECall Service

 PARAMETERS      [IN] phoneId: Phone ID

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_ExitOnlyMode
(
 uint8_t phoneId
)
{
    auto &ecall = taf_ecall::GetInstance();

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_MODEMSERVICE_ECALL_PATH );
    le_cfg_SetInt(iteratorRef, CFG_NODE_OPMODE, (int) TAF_ECALL_MODE_NORMAL);
    le_cfg_CommitTxn(iteratorRef);

    return ecall.SetECallOperatingMode(phoneId, TAF_ECALL_MODE_NORMAL);
}

/*======================================================================

 FUNCTION        taf_ecall_GetConfiguredOperationMode

 DESCRIPTION     Get the ecall operating mode.

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [IN] phoneId: Phone ID
                 [OUT] opModePtr: pointer of type Opmode

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetConfiguredOperationMode
(
 uint8_t            phoneId,
 taf_ecall_OpMode_t* opModePtr
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.GetECallOperatingMode(phoneId, opModePtr);
}

/*======================================================================

 FUNCTION        taf_ecall_SetMsdVersion

 DESCRIPTION     Set MSD version. Only supports MSD version two and three.

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [IN] msdVersion: msd version value

 RETURN VALUE    le_result_t
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetMsdVersion
(
    uint32_t msdVersion
)
{
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_V3)
    if ((msdVersion != MSD_VERSION_TWO) && (msdVersion != MSD_VERSION_THREE))
    {
        LE_ERROR("MsdVersion is set wrong value %d", msdVersion);
        return LE_FAULT;
    }

    uint32_t msdVersionRead = 0;
    if ((taf_ecall_GetMsdVersion(&msdVersionRead) == LE_OK) && (msdVersionRead == msdVersion))
    {
        LE_DEBUG("MsdVersion is set to the same value as the current one %d", msdVersion);
        return LE_OK;
    }

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_MODEMSERVICE_ECALL_PATH );

    le_cfg_SetInt(iteratorRef, CFG_NODE_MSDVERSION, msdVersion);
    le_cfg_CommitTxn(iteratorRef);

    LE_DEBUG("Set MsdVersion to %d", msdVersion);


    return LE_OK;
#else
    return LE_UNSUPPORTED;
#endif
}

/*======================================================================

 FUNCTION        taf_ecall_GetMsdVersion

 DESCRIPTION     Get Msd version.

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [OUT] msdVersion: ptr to save msd version. Only
                 supports MSD version two and three.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetMsdVersion
(
    uint32_t* msdVersion
)
{
    TAF_ERROR_IF_RET_VAL(msdVersion == NULL, LE_BAD_PARAMETER, "msdVersion pointer is NULL");
#if defined(LE_CONFIG_ENABLE_ECALL_MSD_V3)
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn( CFG_MODEMSERVICE_ECALL_PATH );

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_MSDVERSION))
    {
        *msdVersion = le_cfg_GetInt(iteratorRef, CFG_NODE_MSDVERSION, 0);
        LE_DEBUG("MSD version is %d", *msdVersion);
    } else {
        *msdVersion = 2;
        LE_WARN("Unable to get the MSD version from config tree and use the default value %d", *msdVersion);
    }

    le_cfg_CancelTxn(iteratorRef);
    return LE_OK;
#else
    *msdVersion = 2; //Currently we support only msdVersion 2.
    return LE_OK;
#endif
}

/*======================================================================

 FUNCTION        taf_ecall_SetVehicleType

 DESCRIPTION     Set vehicle type

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [IN] vehicleType: vehicle type

 RETURN VALUE    le_result_t
                     LE_FAULT:             Failed.
                     LE_OK:                Successed.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetVehicleType (taf_ecall_MsdVehicleType_t vehicleType)
{
    auto &ecall = taf_ecall::GetInstance();
    if (ecall.isDrvPresent == true)
    {
        LE_ERROR("Unable to set as eCall VHAL is running");
        return LE_FAULT;
    }

    if ((vehicleType < TAF_ECALL_PASSENGER_VEHICLE_CLASS_M1) ||
        (vehicleType > TAF_ECALL_OTHER_VEHICLE_CLASS))
    {
        LE_ERROR("VehicleType is wrong %d", vehicleType);
        return LE_FAULT;
    }
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
                     LE_FAULT:             Failed.
                     LE_OK:                Successed.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetVehicleType
(
    taf_ecall_MsdVehicleType_t* vehicleTypePtr
)
{
    auto &ecall = taf_ecall::GetInstance();
    if (ecall.isDrvPresent == true)
    {
        LE_ERROR("Unable to set as eCall VHAL is running");
        return LE_FAULT;
    }

    TAF_ERROR_IF_RET_VAL(vehicleTypePtr == NULL, LE_BAD_PARAMETER,
            "vehicleTypePtr pointer is NULL");

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn( CFG_MODEMSERVICE_ECALL_PATH );

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_MSDVEHTYPE))
    {
        *vehicleTypePtr = (taf_ecall_MsdVehicleType_t)le_cfg_GetInt(iteratorRef, CFG_NODE_MSDVEHTYPE, 0);
        LE_DEBUG(" vehicleType =  %d", *vehicleTypePtr);
        le_cfg_CancelTxn(iteratorRef);
        return LE_OK;
    } else {
        LE_WARN("Unable to get the vehicle Type from config tree");
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
                     LE_FAULT:             Failed.
                     LE_OK:                Successed.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetVIN
(
    const char* vin
)
{
    auto &ecall = taf_ecall::GetInstance();
    if (ecall.isDrvPresent == true)
    {
        LE_ERROR("Unable to set as eCall VHAL is running");
        return LE_FAULT;
    }

    TAF_ERROR_IF_RET_VAL(strlen(vin) != TAF_ECALL_MAX_VIN_LENGTH, LE_FAULT,
            "VIN length is wrong");

    if (ecall.CheckVIN((char *)vin))
    {
        return LE_BAD_PARAMETER;
    }
    LE_INFO(" vehicle idendification number =  %s", vin);
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
                     LE_FAULT:             Failed.
                     LE_OK:                Succeeded.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetVIN
(
    char* vin,
    size_t vinNumElements
)
{
    auto &ecall = taf_ecall::GetInstance();
    if (ecall.isDrvPresent == true)
    {
        LE_ERROR("Unable to set as eCall VHAL is running");
        return LE_FAULT;
    }

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
    } else {
        LE_WARN("Unable to get the vin from config tree");
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
                     LE_FAULT:             Failed.
                     LE_OK:                Succeeded.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetPropulsionType
(
        taf_ecall_PropulsionStorageType_t propulsionType
)
{
    auto &ecall = taf_ecall::GetInstance();
    if (ecall.isDrvPresent == true)
    {
        LE_ERROR("Unable to set as eCall VHAL is running");
        return LE_FAULT;
    }

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn(CFG_ECALL_PROPULSIONTYPE_PATH);

    if (TAF_ECALL_PROP_TYPE_GASOLINE_TANK & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_GASOLINE, true);
    }
    else
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_GASOLINE, false);
    }

    if (TAF_ECALL_PROP_TYPE_DIESEL_TANK & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_DIESEL, true);
    }
    else
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_DIESEL, false);
    }

    if (TAF_ECALL_PROP_TYPE_COMPRESSED_NATURALGAS & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_NATURALGAS, true);
    }
    else
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_NATURALGAS, false);
    }

    if (TAF_ECALL_PROP_TYPE_PROPANE_GAS & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_PROPANE, true);
    }
    else
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_PROPANE, false);
    }

    if (TAF_ECALL_PROP_TYPE_ELECTRIC & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_ELECTRIC, true);
    }
    else
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_ELECTRIC, false);
    }

    if (TAF_ECALL_PROP_TYPE_HYDROGEN & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_HYDROGEN, true);
    }
    else
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_HYDROGEN, false);
    }

    if (TAF_ECALL_PROP_TYPE_OTHER & propulsionType)
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_OTHER, true);
    }
    else
    {
        le_cfg_SetBool(iteratorRef, CFG_NODE_PROPULSION_OTHER, false);
    }

    le_cfg_CommitTxn(iteratorRef);
    return LE_OK;
}

/*======================================================================

 FUNCTION        taf_ecall_GetPropulsionType

 DESCRIPTION    Read vehicle propulsion storage type.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS      [OUT]propulsionStorageType vehicle propulsion storage type

 RETURN VALUE    le_result_t
                     LE_FAULT:             Failed.
                     LE_OK:                Succeeded.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetPropulsionType
(
    taf_ecall_PropulsionStorageType_t* propulsionStorageType
)
{
    auto &ecall = taf_ecall::GetInstance();
    if (ecall.isDrvPresent == true)
    {
        LE_ERROR("Unable to set as eCall VHAL is running");
        return LE_FAULT;
    }

    if (propulsionStorageType == NULL) {
        LE_ERROR("propulsionStorageType is null.");
        return LE_FAULT;
    }

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
                     LE_FAULT:             Failed.
                     LE_OK:                Succeeded.
                     LE_DUPLICATE:         The MSD has already been imported.

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
    return ecall.SetMsdPosition(ecallRef, isTrusted,  latitude, longitude, direction);
}

/*======================================================================

 FUNCTION       taf_ecall_SetMsdPositionN1

 DESCRIPTION    Sets the position delta N-1 for MSD transmission.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS     [IN]ecallRef : reference for ecall
                [IN]latitudeDeltaN1: change in latitude value
                                     < 1 Unit = 100 miliarcseconds, which is approximately 3m
                                     < maximum value: 511 = 0 0'51.100'' (±1580m)
                                     < minimum value: -512 = -0 0'51.200'' (± -1583m)
                [IN]longitudeDeltaN1: change longitude value
                                     < 1 Unit = 100 miliarcseconds, which is approximately 3m
                                     < maximum value: 511 = 0 0'51.100'' (±1580m)
                                     < minimum value: -512 = -0 0'51.200'' (± -1583m)

 RETURN VALUE   le_result_t
                    LE_BAD_PARAMETER:     Bad eCall reference.
                    LE_FAULT:             Failed.
                    LE_OK:                Succeeded.
                    LE_DUPLICATE:         The MSD has already been imported.

 NOTE           The process exits when an invalid eCall reference is given.

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
    return ecall.SetMsdPositionN1(ecallRef, latitudeDeltaN1, longitudeDeltaN1);
}

/*======================================================================

 FUNCTION       taf_ecall_SetMsdPositionN2

 DESCRIPTION    Sets the position delta N-2 for MSD transmission.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS     [IN]ecallRef : reference for ecall
                [IN]latitudeDeltaN2: change in latitude value
                                     < 1 Unit = 100 miliarcseconds, which is approximately 3m
                                     < maximum value: 511 = 0 0'51.100'' (±1580m)
                                     < minimum value: -512 = -0 0'51.200'' (± -1583m)
                [IN]longitudeDeltaN2: change longitude value
                                     < 1 Unit = 100 miliarcseconds, which is approximately 3m
                                     < maximum value: 511 = 0 0'51.100'' (±1580m)
                                     < minimum value: -512 = -0 0'51.200'' (± -1583m)

 RETURN VALUE   le_result_t
                    LE_BAD_PARAMETER:     Bad eCall reference.
                    LE_FAULT:             Failed.
                    LE_OK:                Succeeded.
                    LE_DUPLICATE:         The MSD has already been imported.

 NOTE           The process exits when an invalid eCall reference is given.

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
    return ecall.SetMsdPositionN2(ecallRef, latitudeDeltaN2, longitudeDeltaN2);
}

/*======================================================================

 FUNCTION        taf_ecall_SetMsdPassengersCount

 DESCRIPTION     Set number of passenger.

 DEPENDENCIESA   Initialization of ECall service

 PARAMETERS      [IN] ecallRef: reference for ecall
                 [IN] passengerCount: number of passenger

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Bad eCall reference.
                     LE_FAULT:             Failed.
                     LE_OK:                Succeeded.
                     LE_DUPLICATE:         The MSD has already been imported.

 NOTE           The process exits when an invalid eCall reference is given.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetMsdPassengersCount
(
    taf_ecall_CallRef_t  ecallRef,
    uint32_t passengerCount
)
{
    auto &ecall = taf_ecall::GetInstance();

    if (ecall.isDrvPresent == true)
    {
        LE_ERROR("Unable to set as eCall VHAL is running");
        return LE_FAULT;
    }

    return ecall.SetMsdPassengersCount(ecallRef, passengerCount);
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
    return LE_UNSUPPORTED;

#if 0
    auto &ecall = taf_ecall::GetInstance();
    return ecall.SetMsdTxMode(txMode);
#endif
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
    return LE_UNSUPPORTED;

#if 0
    TAF_KILL_CLIENT_IF_RET_VAL(modePtr == NULL, LE_FAULT," TxMode pointer is NULL");

    auto &ecall = taf_ecall::GetInstance();
    return ecall.GetMsdTxMode(modePtr);
#endif
}

/*======================================================================

 FUNCTION       taf_ecall_SetMsdAdditionalData

 DESCRIPTION    Sets the optional additional data for MSD transmission.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS     [IN] ecallRef : reference for ecall
                [IN] oid : RELATIVE-OID
                [IN] data : optional data
                [IN] dataLength : the length of optional data

 RETURN VALUE   le_result_t
                    LE_BAD_PARAMETER:     Bad eCall reference.
                    LE_FAULT:             Failed.
                    LE_OK:                Succeeded.
                    LE_DUPLICATE:         The MSD has already been imported.
 NOTE           The process exits when an invalid eCall reference is given.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetMsdAdditionalData
(
    taf_ecall_CallRef_t ecallRef,
    const char* oid,
    const uint8_t* data,
    size_t dataLength
)
{
    TAF_ERROR_IF_RET_VAL(oid == NULL, LE_BAD_PARAMETER," oid pointer is NULL");
    TAF_ERROR_IF_RET_VAL(data == NULL, LE_BAD_PARAMETER," data pointer is NULL");
    TAF_ERROR_IF_RET_VAL(dataLength > TAF_ECALL_MAX_DATA_LENGTH,
                    LE_FAULT, "optional data length exceeds max length")
    auto &ecall = taf_ecall::GetInstance();

    if (ecall.isDrvPresent == true)
    {
        LE_ERROR("Unable to set as eCall VHAL is running");
        return LE_FAULT;
    }

    return ecall.SetMsdAdditionalData(ecallRef, oid, data, dataLength);
}

/*======================================================================

 FUNCTION       taf_ecall_ResetMsdAdditionalData

 DESCRIPTION    Resets the optional additional data for MSD transmission.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS     [IN] ecallRef : reference for ecall

 RETURN VALUE   le_result_t
                    LE_BAD_PARAMETER:     Bad eCall reference.
                    LE_FAULT:             Failed.
                    LE_OK:                Succeeded.
                    LE_DUPLICATE:         The MSD has already been imported.
 NOTE           The process exits when an invalid eCall reference is given.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_ResetMsdAdditionalData
(
    taf_ecall_CallRef_t ecallRef
)
{
    auto &ecall = taf_ecall::GetInstance();

    if (ecall.isDrvPresent == true)
    {
        LE_ERROR("Unable to set as eCall VHAL is running");
        return LE_FAULT;
    }

    return ecall.ResetMsdAdditionalData(ecallRef);
}

/*======================================================================

 FUNCTION       taf_ecall_SetMsdEuroNCAPLocationOfImpact

 DESCRIPTION    Sets the location of impact for Euro NCAP additional data.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS     [IN] ecallRef : reference for ecall
                [IN] iiLocations : location of impact

 RETURN VALUE   le_result_t
                    LE_BAD_PARAMETER:     Bad eCall reference.
                    LE_FAULT:             Failed.
                    LE_OK:                Succeeded.
                    LE_DUPLICATE:         The MSD has already been imported.
 NOTE           The process exits when an invalid eCall reference is given.

 SIDE EFFECTS

======================================================================*/

le_result_t taf_ecall_SetMsdEuroNCAPLocationOfImpact
(
    taf_ecall_CallRef_t ecallRef,
    taf_ecall_IILocations_t iiLocations
)
{
    auto &ecall = taf_ecall::GetInstance();

    if (ecall.isDrvPresent == true)
    {
        LE_ERROR("Unable to set as eCall VHAL is running");
        return LE_FAULT;
    }

    return ecall.SetMsdEuroNCAPLocationOfImpact(ecallRef, iiLocations);
}

/*======================================================================

 FUNCTION       taf_ecall_SetMsdEuroNCAPRolloverDetected

 DESCRIPTION    Sets the rollover detected for Euro NCAP additional data.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS     [IN] ecallRef : reference for ecall
                [IN] rolloverDetected: rollover detected or not

 RETURN VALUE   le_result_t
                    LE_BAD_PARAMETER:     Bad eCall reference.
                    LE_FAULT:             Failed.
                    LE_OK:                Succeeded.
                    LE_DUPLICATE:         The MSD has already been imported.
 NOTE           The process exits when an invalid eCall reference is given.

 SIDE EFFECTS

======================================================================*/

le_result_t taf_ecall_SetMsdEuroNCAPRolloverDetected
(
    taf_ecall_CallRef_t ecallRef,
    bool   rolloverDetected
)
{
    auto &ecall = taf_ecall::GetInstance();

    if (ecall.isDrvPresent == true)
    {
        LE_ERROR("Unable to set as eCall VHAL is running");
        return LE_FAULT;
    }

    return ecall.SetMsdEuroNCAPRolloverDetected(ecallRef, rolloverDetected);
}
/*======================================================================

 FUNCTION       taf_ecall_ResetMsdEuroNCAPRolloverDetected

 DESCRIPTION    Resets the rollover detected for Euro NCAP additional data.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS     [IN] ecallRef : reference for ecall

 RETURN VALUE   le_result_t
                    LE_BAD_PARAMETER:     Bad eCall reference.
                    LE_FAULT:             Failed.
                    LE_OK:                Succeeded.
                    LE_DUPLICATE:         The MSD has already been imported.
 NOTE           The process exits when an invalid eCall reference is given.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_ResetMsdEuroNCAPRolloverDetected
(
    taf_ecall_CallRef_t ecallRef
)
{
    auto &ecall = taf_ecall::GetInstance();

    if (ecall.isDrvPresent == true)
    {
        LE_ERROR("Unable to set as eCall VHAL is running");
        return LE_FAULT;
    }

    return ecall.ResetMsdEuroNCAPRolloverDetected(ecallRef);
}

/*======================================================================

 FUNCTION       taf_ecall_SetMsdEuroNCAPIIDeltaV

 DESCRIPTION    Sets the IIDeltaV for Euro NCAP additional data.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS     [IN] ecallRef : reference for ecall
                [IN] rangeLimit : unsigned upper limit of the detection range
                     for delta-v
                [IN] deltaVX : delta-v measured over the x-axis of the
                     coordinate system of the vehicle
                [IN] deltaVY : delta-v measured over the y-axis of the
                     coordinate system of the vehicle

 RETURN VALUE   le_result_t
                    LE_BAD_PARAMETER:     Bad eCall reference.
                    LE_FAULT:             Failed.
                    LE_OK:                Succeeded.
                    LE_DUPLICATE:         The MSD has already been imported.
 NOTE           The process exits when an invalid eCall reference is given.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetMsdEuroNCAPIIDeltaV
(
    taf_ecall_CallRef_t ecallRef,
    uint8_t   rangeLimit,
    int16_t   deltaVX,
    int16_t   deltaVY
)
{
    auto &ecall = taf_ecall::GetInstance();

    if (ecall.isDrvPresent == true)
    {
        LE_ERROR("Unable to set as eCall VHAL is running");
        return LE_FAULT;
    }

    return ecall.SetMsdEuroNCAPIIDeltaV(ecallRef, rangeLimit, deltaVX, deltaVY);
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
                     LE_BUSY:              eCall session is already in progress.

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
                     LE_BUSY:              eCall session is already in progress.

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
                     LE_BUSY:              eCall session is already in progress.

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

 FUNCTION        taf_ecall_StartPrivate

 DESCRIPTION    Initiate a private eCall

 DEPENDENCIESA   Initialization of ECall service

 PARAMETERS      [IN] ecallRef: ecall Reference

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.
                     LE_BUSY:              eCall session is already in progress.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_StartPrivate
(
    taf_ecall_CallRef_t ecallRef,
    const char * psapNumber,
    const char * contentType,
    const char * acceptInfo
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.StartPrivate(ecallRef, psapNumber, contentType, acceptInfo);
}

/*======================================================================

 FUNCTION        taf_ecall_End

 DESCRIPTION     End the ongoing ecall.

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

 FUNCTION        taf_ecall_Answer

 DESCRIPTION     Answer the incoming ecall.

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS      [IN] ecallRef: Reference

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_Answer
(
    taf_ecall_CallRef_t ecallRef
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.AnswerECall(ecallRef);
}

/*======================================================================

 FUNCTION        taf_ecall_ImportMsd

 DESCRIPTION    Import the PDU MSD

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS      [IN] ecallRef: Reference
                 [IN] pdumsd: msd in pdu format
                 [IN] msdLength: length of msd in pdu format

 RETURN VALUE    le_result_t
                     LE_OK:                Success.
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_OVERFLOW:          The size of the MSD buffer is wrong.

 NOTE            The process exits if an invalid eCall reference is passed.

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
                     LE_OK:                Success.
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_OVERFLOW:          The size of the MSD buffer is wrong.
                     LE_NOT_FOUND:         The MSD is not imported or updated.
                     LE_FAULT:             Fail.

 NOTE            The process exits if an invalid eCall reference is passed.

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

 FUNCTION       taf_eCall_GetType

 DESCRIPTION    Get the current type of the given eCall

 DEPENDENCIES   Initialization of ECall service

 PARAMETERS     [IN] ecallRef: ecall reference

 RETURN VALUE   The current type of the given eCall

 SIDE EFFECTS

======================================================================*/

taf_ecall_Type_t taf_ecall_GetType
(
    taf_ecall_CallRef_t    ecallRef
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.GetType(ecallRef);
}

/*======================================================================

 FUNCTION        taf_ecall_SetPsapNumber

 DESCRIPTION     Set the Public Safely Answering Point telephone number.

 @note The PSAP number is applicable in manually or automatically dialed eCalls. It is also
   applicable to test eCalls that are dialed intentionally for testing, validating, or certifying.

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

 @note The PSAP number is applicable in manually or automatically dialed eCalls. It is also
   applicable to test eCalls that are dialed intentionally for testing, validating, or certifying.

 @warning This function doesn't read the U/SIM content.

 DEPENDENCIES    Initialization of ECall Service

 PARAMETERS      [OUT] psapNumber: Ptr to save PSAP number

 RETURN VALUE             le_result_t
    - LE_OK               On success
    - LE_FAULT            On failures or if le_ecall_SetPsapNumber() has never been called before
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

int32_t taf_ecall_GetPlatformSpecificTerminationCode
(
    taf_ecall_CallRef_t ecallRef
)
{
    LE_WARN("Not supported.");
    return 0;
}

/*======================================================================

 FUNCTION      taf_ecall_SetNadClearDownFallbackTime

 Sets the eCall clear down fallback time of the NAD. If the NAD doesn't receive a clear
 down indication from network or a clear down message (AL-ACK) from the PSAP during
 an ecall, then will trigger an automatic call end when clear down fallback time out.

 @return
  - LE_OK         On success
  - LE_BUSY       An eCall session is in progress
  - LE_FAULT      On failures

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetNadClearDownFallbackTime
(
    uint16_t ccftTime //NAD (network access device) clear down fallback time in minutes
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.SetNadClearDownFallbackTime(ccftTime);
}

/*======================================================================

 FUNCTION      taf_ecall_GetNadClearDownFallbackTime

 Gets the eCall clear down fallback time of the NAD (network access device) .

 @return
  - LE_OK          On success
  - LE_FAULT       On failures

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetNadClearDownFallbackTime
(
    uint16_t* ccftTime //NAD (network access device) clear down fallback time in minutes
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.GetNadClearDownFallbackTime(ccftTime);
}

/*======================================================================

 FUNCTION      taf_ecall_SetNadMinNetworkRegistrationTime

 Sets the eCall minimum network registration time of the NAD. After an eCall ends, the
 NAD shall remain registered on the serving network. During this time, the NAD can
 automatically receive calls from the PSAP.

 @return
  - LE_OK         On success
  - LE_BUSY       An eCall session is in progress
  - LE_FAULT      On failures

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetNadMinNetworkRegistrationTime
(
    uint16_t minNwRegTime //NAD (network access device) minimum network registration time in minutes
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.SetNadMinNetworkRegistrationTime(minNwRegTime);
}

/*======================================================================

 FUNCTION      taf_ecall_GetNadMinNetworkRegistrationTime

 Gets the eCall minimum network registration time of the NAD.

 @return
  - LE_OK          On success
  - LE_FAULT       On failures

 SIDE EFFECTS

======================================================================*/

le_result_t taf_ecall_GetNadMinNetworkRegistrationTime
(
    uint16_t* minNwRegTime //NAD (network access device) minimum network registration time in minutes
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.GetNadMinNetworkRegistrationTime(minNwRegTime);
}

/*======================================================================

 FUNCTION        taf_ecall_GetHlapTimerState

 DESCRIPTION     Get eCall hlap timer state

 DEPENDENCIES    Initialization of ECall service

 PARAMETERS      [IN] ecallRef: ecall reference

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_GetHlapTimerState
(
    taf_ecall_HlapTimerType_t timerType, //Hlap timer type
    taf_ecall_HlapTimerStatus_t* timerStatus, //The status of hlap timer
    uint16_t* elapsedTime //The elapsed time of hlap timer
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.GetHlapTimerState(timerType, timerStatus, elapsedTime);
}

/*======================================================================

 FUNCTION        taf_ecall_IsInProgress

 DESCRIPTION     Queries the eCall in progress or not.

 DEPENDENCIES    Initialization of ECall service

 PARAMETERS      [IN] ecallRef: ecall reference

 RETURN VALUE    le_result_t
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/

le_result_t taf_ecall_IsInProgress
(
    taf_ecall_CallRef_t ecallRef,
    bool* isInProgress
)
{
    auto &ecall = taf_ecall::GetInstance();
    return ecall.IsInProgress(ecallRef, isInProgress);
}

/*======================================================================

 FUNCTION        taf_ecall_SetInitialDialAttempts

 DESCRIPTION     Sets the number of dial attempts in case of call initiation failed for Europen regulatory eCall.

 DEPENDENCIES    Initialization of ECall service

 PARAMETERS      [IN] ecallRef: ecall reference

 RETURN VALUE    le_result_t
                     LE_OVERFLOW:          The size of the dial attempt length is wrong.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetInitialDialAttempts
(
    uint8_t attempts
)
{
    TAF_ERROR_IF_RET_VAL(attempts > TAF_ECALL_MAX_DIAL_ATTEMPTS_LENGTH || attempts == 0,
                    LE_OVERFLOW, "Wrong dial attempt length");

    auto &ecall = taf_ecall::GetInstance();
    return ecall.SetInitialDialAttempts(attempts);
}

/*======================================================================

 FUNCTION        taf_ecall_SetInitialDialIntervalBetweenDialAttempts

 DESCRIPTION     Sets the dial interval value between dial attempts in case of call initiation failed for Europen
                 regulatory eCall.

 DEPENDENCIES    Initialization of ECall service

 PARAMETERS      [IN] ecallRef: ecall reference

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER:     Invalid parameters.
                     LE_OVERFLOW:          The size of the dial interval length is wrong.
                     LE_FAULT:             Fail.
                     LE_OK:                Success.

 SIDE EFFECTS

======================================================================*/
le_result_t taf_ecall_SetInitialDialIntervalBetweenDialAttempts
(
    const uint16_t* interval,
    size_t intervalLength
)
{
    TAF_ERROR_IF_RET_VAL(interval == NULL, LE_BAD_PARAMETER," Dial interval is NULL");

    TAF_ERROR_IF_RET_VAL(intervalLength > TAF_ECALL_MAX_DIAL_ATTEMPTS_LENGTH || intervalLength == 0,
                    LE_OVERFLOW, "Wrong dial interval length");

    auto &ecall = taf_ecall::GetInstance();
    return ecall.SetInitialDialIntervalBetweenDialAttempts(interval, intervalLength);
}