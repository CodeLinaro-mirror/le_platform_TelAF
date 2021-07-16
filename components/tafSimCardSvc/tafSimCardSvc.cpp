/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "legato.h"
#include "interfaces.h"
#include <iostream>
#include <string>
#include <memory>
#include <telux/tel/PhoneFactory.hpp>
#include "tafSimCard.hpp"

using namespace telux::tel;
using namespace telux::common;
using namespace telux::tafsvc;


COMPONENT_INIT
{
    LE_INFO("tafSimcard Service Init...\n");
    auto &sim = taf_sim::GetInstance();
    sim.Init();
    LE_INFO(" Sim Card service Ready...\n");

}

taf_sim_NewStateHandlerRef_t taf_sim_AddNewStateHandler(taf_sim_NewStateHandlerFunc_t handlerPtr,
        void* contextPtr){

    le_event_HandlerRef_t handlerRef;
    auto &sim = taf_sim::GetInstance();
    handlerRef = (le_event_HandlerRef_t)sim.AddStateHandler(handlerPtr, contextPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_sim_NewStateHandlerRef_t)(handlerRef);

}

void taf_sim_RemoveNewStateHandler(taf_sim_NewStateHandlerRef_t handlerRef)
{
    auto &sim = taf_sim::GetInstance();
    sim.RemoveStateHandler(handlerRef);
}

taf_sim_States_t taf_sim_GetState (taf_sim_Id_t slotId) {
    LE_INFO("tafSimCard getState \n");
    auto &sim = taf_sim::GetInstance();
    return (taf_sim_States_t)sim.getState(slotId);
}

bool taf_sim_IsPresent(taf_sim_Id_t slotId) {
    auto &sim = taf_sim::GetInstance();
    taf_sim_States_t state = sim.getState(slotId);
    if ((state == TAF_SIM_PRESENT) ||
            (state == TAF_SIM_READY) ||
            (state == TAF_SIM_RESTRICTED))
    {
        return true;
    } else {
        return false;
    }

}

bool taf_sim_IsReady(taf_sim_Id_t slotId) {
    auto &sim = taf_sim::GetInstance();
    taf_sim_States_t state = sim.getState(slotId);
    if (state == TAF_SIM_READY) {
        return true;
    } else {
        return false;
    }
}

le_result_t taf_sim_GetICCID( taf_sim_Id_t slotId, char * iccidPtr,
        size_t iccidLen) {
    TAF_ERROR_IF_RET_VAL(iccidPtr == NULL, LE_BAD_PARAMETER, "iccidPtr is NULL");
    TAF_ERROR_IF_RET_VAL(iccidLen < TAF_SIM_ICCID_BYTES, LE_OVERFLOW, "Incorrect buffer size");
    LE_INFO("tafSimCard getICCID \n");
    auto &sim = taf_sim::GetInstance();
    return sim.getICCID(slotId, iccidPtr, iccidLen);
}

le_result_t taf_sim_GetIMSI( taf_sim_Id_t slotId, char * imsiPtr,
        size_t imsiLen) {
    TAF_ERROR_IF_RET_VAL(imsiPtr == NULL, LE_BAD_PARAMETER, "imsiPtr is NULL");
    TAF_ERROR_IF_RET_VAL(imsiLen < TAF_SIM_IMSI_BYTES, LE_OVERFLOW, "Incorrect buffer size");
    LE_INFO("tafSimCard getIMSI \n");
    auto &sim = taf_sim::GetInstance();
    return sim.getIMSI(slotId, imsiPtr, imsiLen);
}

le_result_t taf_sim_GetSubscriberPhoneNumber( taf_sim_Id_t slotId, char * phoneNumberPtr,
        size_t phoneNumberLen) {
    TAF_ERROR_IF_RET_VAL(phoneNumberPtr == NULL, LE_BAD_PARAMETER, "iccidPtr is NULL");
    TAF_ERROR_IF_RET_VAL(phoneNumberLen < TAF_SIM_PHONE_NUM_MAX_BYTES, LE_OVERFLOW,
            "Incorrect buffer size");
    LE_INFO("tafSimCard taf_sim_GetSubscriberPhoneNumber \n");
    auto &sim = taf_sim::GetInstance();
    return sim.getSubscriberPhoneNumber(slotId, phoneNumberPtr, phoneNumberLen);
}

le_result_t taf_sim_GetHomeNetworkOperator( taf_sim_Id_t slotId, char * namePtr,
        size_t nameLen) {
    TAF_ERROR_IF_RET_VAL(namePtr == NULL, LE_BAD_PARAMETER, "iccidPtr is NULL");
    LE_INFO("tafSimCard taf_sim_GetHomeNetworkOperator \n");
    auto &sim = taf_sim::GetInstance();
    return sim.getHomeNetworkOperator(slotId, namePtr, nameLen);
}

le_result_t taf_sim_GetHomeNetworkMccMnc( taf_sim_Id_t slotId,char * mccPtr,
        size_t mccLen, char * mncPtr, size_t mncLen) {
    TAF_ERROR_IF_RET_VAL(mccPtr == NULL, LE_BAD_PARAMETER, "mccPtr is NULL");
    TAF_ERROR_IF_RET_VAL(mncPtr == NULL, LE_BAD_PARAMETER, "mncPtr is NULL");
    LE_INFO("tafSimCard taf_sim_GetHomeNetworkMccMnc \n");
    auto &sim = taf_sim::GetInstance();
    return sim.getHomeNetworkMccMnc(slotId, mccPtr, mccLen, mncPtr, mncLen);
}

le_result_t taf_sim_SelectCard( taf_sim_Id_t slotId) {
    auto &sim = taf_sim::GetInstance();
    // Select the SIM card
    if (sim.selectSimSlot(slotId) != LE_OK) {
        LE_ERROR("Unable to select Sim Card slot %d !", slotId);
        return LE_FAULT;
    }
    return LE_OK;
}

taf_sim_Id_t taf_sim_GetSelectedCard( void) {
    auto &sim = taf_sim::GetInstance();
    return (taf_sim_Id_t)sim.slot;
}

le_result_t taf_sim_EnterPIN( taf_sim_Id_t  slotId, taf_sim_LockType_t lockType,
        const char* pinPtr) {
    TAF_ERROR_IF_RET_VAL(pinPtr == NULL, LE_BAD_PARAMETER, "pinPtr is NULL");
    TAF_ERROR_IF_RET_VAL(strlen(pinPtr) < TAF_SIM_PIN_MIN_LEN,
                        LE_UNDERFLOW , "pin length is not enough");
    TAF_ERROR_IF_RET_VAL(strlen(pinPtr) > TAF_SIM_PIN_MAX_LEN,
                        LE_BAD_PARAMETER, "pin length is too long");
    LE_INFO("tafSimCard taf_sim_EnterPin \n");
    auto &sim = taf_sim::GetInstance();
    return sim.UnlockCardByPin(slotId, lockType, pinPtr);
}

le_result_t taf_sim_ChangePIN(taf_sim_Id_t slotId, taf_sim_LockType_t lockType,
        const char* oldpinPtr, const char* newpinPtr) {
    TAF_ERROR_IF_RET_VAL(oldpinPtr == NULL, LE_BAD_PARAMETER, "oldpinPtr is NULL");
    TAF_ERROR_IF_RET_VAL(newpinPtr == NULL, LE_BAD_PARAMETER, "newpinPtr is NULL");
    TAF_ERROR_IF_RET_VAL(strlen(oldpinPtr) < TAF_SIM_PIN_MIN_LEN
                        || strlen(newpinPtr) < TAF_SIM_PIN_MIN_LEN,
                        LE_UNDERFLOW , "pin length is not enough");
    LE_INFO("tafSimCard taf_sim_EnterPin \n");
    auto &sim = taf_sim::GetInstance();
    return sim.ChangeCardPin(slotId, lockType, oldpinPtr, newpinPtr);
}

le_result_t taf_sim_Lock( taf_sim_Id_t slotId, taf_sim_LockType_t lockType,
        const char* pinPtr) {
    TAF_ERROR_IF_RET_VAL(pinPtr == NULL, LE_BAD_PARAMETER, "pinPtr is NULL");
    LE_INFO("tafSimCard taf_sim_Lock \n");
    TAF_ERROR_IF_RET_VAL(strlen(pinPtr) < TAF_SIM_PIN_MIN_LEN,
                        LE_UNDERFLOW , "pin length is not enough");
    auto &sim = taf_sim::GetInstance();
    return sim.SetCardLock(slotId, lockType, pinPtr, true);

    return LE_OK;
}

le_result_t taf_sim_Unlock( taf_sim_Id_t slotId, taf_sim_LockType_t lockType,
        const char* pinPtr) {
    TAF_ERROR_IF_RET_VAL(pinPtr == NULL, LE_BAD_PARAMETER, "pinPtr is NULL");
    TAF_ERROR_IF_RET_VAL(strlen(pinPtr) < TAF_SIM_PIN_MIN_LEN,
                        LE_UNDERFLOW , "pin length is not enough");
    TAF_ERROR_IF_RET_VAL(strlen(pinPtr) < TAF_SIM_PIN_MIN_LEN,
                        LE_UNDERFLOW , "pin length is not enough");
    LE_INFO("tafSimCard taf_sim_UnLock \n");
    auto &sim = taf_sim::GetInstance();
    return sim.SetCardLock(slotId, lockType, pinPtr, false);
}

le_result_t taf_sim_Unblock( taf_sim_Id_t slotId, taf_sim_LockType_t lockType,
        const char* pukPtr, const char* newpinPtr) {
    TAF_ERROR_IF_RET_VAL(pukPtr == NULL, LE_BAD_PARAMETER, "pukPtr is NULL");
    TAF_ERROR_IF_RET_VAL(newpinPtr == NULL, LE_BAD_PARAMETER, "newpinPtr is NULL");
    TAF_ERROR_IF_RET_VAL(strlen(pukPtr) != TAF_SIM_PUK_MAX_LEN,
                        LE_OUT_OF_RANGE , "puk length is wrong");
    TAF_ERROR_IF_RET_VAL(strlen(newpinPtr) < TAF_SIM_PIN_MIN_LEN,
                        LE_UNDERFLOW , "pin length is not enough");
    LE_INFO("tafSimCard taf_sim_UnBlock \n");
    auto &sim = taf_sim::GetInstance();
    return sim.UnlockCardByPuk(slotId, lockType, pukPtr, newpinPtr);
}

int32_t taf_sim_GetRemainingPINTries( taf_sim_Id_t slotId) {
    auto &sim = taf_sim::GetInstance();
    return sim.GetRemainingPINTries(slotId);
}

le_result_t taf_sim_GetRemainingPUKTries( taf_sim_Id_t slotId,
        uint32_t* remainingPukTriesPtr) {
    auto &sim = taf_sim::GetInstance();
    return sim.GetRemainingPukTries(slotId, remainingPukTriesPtr);
}

taf_sim_AuthenticationResponseHandlerRef_t taf_sim_AddAuthenticationResponseHandler(
        taf_sim_AuthenticationResponseHandlerFunc_t handlerPtr, void* contextPtr) {

    le_event_HandlerRef_t handlerRef;
    auto &sim = taf_sim::GetInstance();
    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }
    handlerRef = (le_event_HandlerRef_t)sim.AddAuthenticationResponseHandler(handlerPtr,
            contextPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_sim_AuthenticationResponseHandlerRef_t)(handlerRef);

}

void taf_sim_RemoveAuthenticationResponseHandler(
        taf_sim_AuthenticationResponseHandlerRef_t  handlerRef) {
    auto &sim = taf_sim::GetInstance();
    sim.RemoveAuthenticationResponseHandler(handlerRef);
}

le_result_t  taf_sim_GetEID( taf_sim_Id_t slotId, char* eidPtr, size_t eidLen) {
    TAF_ERROR_IF_RET_VAL(eidPtr == NULL, LE_BAD_PARAMETER, "eidPtr is NULL");
    TAF_ERROR_IF_RET_VAL(eidLen < TAF_SIM_EID_BYTES, LE_OVERFLOW, "Incorrect buffer size");
    auto &sim = taf_sim::GetInstance();
    return sim.GetEID(slotId, eidPtr, eidLen);
}

le_result_t taf_sim_SetAutomaticSelection( bool enable) {
    auto &sim = taf_sim::GetInstance();
    return sim.SetAutomaticSelection(enable);
}

le_result_t taf_sim_GetAutomaticSelection( bool* enablePtr) {
    TAF_ERROR_IF_RET_VAL(enablePtr == NULL, LE_BAD_PARAMETER, "enablePtr is NULL");
    auto &sim = taf_sim::GetInstance();
    return sim.GetAutomaticSelection(enablePtr);
}

le_result_t taf_sim_OpenLogicalChannel( taf_sim_Id_t slotId,
                taf_sim_AppType_t appType, uint8_t* channel) {
    TAF_ERROR_IF_RET_VAL(channel == NULL, LE_BAD_PARAMETER, "channelPtr is NULL");
    auto &sim = taf_sim::GetInstance();
    return sim.OpenLogicalChannel(slotId, appType, channel);
}

le_result_t taf_sim_CloseLogicalChannel( taf_sim_Id_t simId, uint8_t channel) {
    auto &sim = taf_sim::GetInstance();
    return sim.CloseLogicalChannel(simId, channel);
}

le_result_t taf_sim_SendApduOnChannel
(
    taf_sim_Id_t simId,
    uint8_t channel,
    const uint8_t* commandApduPtr,
    size_t commandApduNumElements,
    uint8_t* responseApduPtr,
    size_t* responseApduNumElementsPtr
)
{
    TAF_ERROR_IF_RET_VAL(commandApduPtr == NULL, LE_BAD_PARAMETER, "commandApduPtr is NULL");
    TAF_ERROR_IF_RET_VAL(responseApduPtr == NULL, LE_BAD_PARAMETER, "responseApduPtr is NULL");
    TAF_ERROR_IF_RET_VAL(responseApduNumElementsPtr == NULL, LE_BAD_PARAMETER, "responseApduNumElementsPtr is NULL");
    TAF_ERROR_IF_RET_VAL(commandApduNumElements > TAF_SIM_APDU_MAX_BYTES, LE_BAD_PARAMETER, "Too many elements ");
    TAF_ERROR_IF_RET_VAL(*responseApduNumElementsPtr > TAF_SIM_RESPONSE_MAX_BYTES, LE_BAD_PARAMETER, "Too many elements ");

    auto &sim = taf_sim::GetInstance();
    return sim.SendApduOnChannel(simId, channel, commandApduPtr,
             commandApduNumElements, responseApduPtr, responseApduNumElementsPtr);

}

le_result_t taf_sim_SendApdu
(
    taf_sim_Id_t simId,
    const uint8_t* commandApduPtr,
    size_t commandApduNumElements,
    uint8_t* responseApduPtr,
    size_t* responseApduNumElementsPtr
)
{
    TAF_ERROR_IF_RET_VAL(commandApduPtr == NULL, LE_BAD_PARAMETER, "commandApduPtr is NULL");
    TAF_ERROR_IF_RET_VAL(responseApduPtr == NULL, LE_BAD_PARAMETER, "responseApduPtr is NULL");
    TAF_ERROR_IF_RET_VAL(responseApduNumElementsPtr == NULL, LE_BAD_PARAMETER, "responseApduNumElementsPtr is NULL");
    TAF_ERROR_IF_RET_VAL(commandApduNumElements > TAF_SIM_APDU_MAX_BYTES, LE_BAD_PARAMETER, "Too many elements ");
    TAF_ERROR_IF_RET_VAL(*responseApduNumElementsPtr > TAF_SIM_RESPONSE_MAX_BYTES, LE_BAD_PARAMETER, "Too many elements ");

    auto &sim = taf_sim::GetInstance();
    return sim.SendApdu(simId, commandApduPtr, commandApduNumElements, responseApduPtr, responseApduNumElementsPtr);

}
