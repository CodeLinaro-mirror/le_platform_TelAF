/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <arpa/inet.h>
#include "legato.h"
#include "tafDoIPStack.h"

#include "tafDoIPStackUnitTest.h"
#include "tafDoIPNodeTest.h"

static uint32_t CurrConnectionCnt;
static taf_doip_PowerModeQueryHandlerRef_t PmQueryRef = NULL;
static taf_doip_DiagIndicationHandlerRef_t IndicationRef = NULL;
static taf_doip_DiagConfirmHandlerRef_t DiagConfirmRef = NULL;
static taf_doip_Ref_t  DoipEntityRef = NULL;

static le_result_t DoipEntityStart
(
    void
);
static void DoipEntityStop
(
    void
);

static taf_doip_PowerMode_t PowerModeQueryHandler
(
    void* userPtr
)
{
    LE_TEST_INFO("Enter taf_doip_PowerModeQuery.");

    return TAF_DOIP_POWER_MODE_READY;
}

static void ConnectionEventHandler
(
    uint16_t            sa,
    taf_doip_Result_t   result
)
{
    if (result == TAF_DOIP_RESULT_SA_REGISTERED)
    {
        LE_TEST_INFO("%d connection registered", sa);
    }
    else if (result == TAF_DOIP_RESULT_SA_DEREGISTERED)
    {
        LE_TEST_INFO("%d connection deregistered", sa);
        CurrConnectionCnt++;
        if (CurrConnectionCnt == UT_CONNECTION_COUNT)
        {
            // Diagnostic unit test is completed.
            DoipEntityStop();
            LE_TEST_EXIT;
        }
    }
    else
    {
        LE_ERROR("Unknown connection event result.");
    }
}

static void DiagIncationHandler
(
    const taf_doip_AddrInfo_t*  addrInfoPtr,
    const taf_doip_DiagMsg_t*   diagMsgPtr,
    taf_doip_Result_t result,
    void* userPtr
)
{
    LE_TEST_INFO("Enter DiagIncationHandler.");

    le_result_t         ret;
    taf_doip_AddrInfo_t respAddrInfo;
    taf_doip_DiagMsg_t  respDiagMsg;

    LE_TEST_ASSERT(addrInfoPtr != NULL, "Address information is invalid");

    if ((result == TAF_DOIP_RESULT_SA_REGISTERED) || (result == TAF_DOIP_RESULT_SA_DEREGISTERED))
    {
        // Connection registed event handler.
        ConnectionEventHandler(addrInfoPtr->sa, result);
        return;
    }

    // Diagnostic message handler.
    LE_TEST_ASSERT(diagMsgPtr != NULL, "Diagnostic message is invalid");

    LE_TEST_INFO("UDS requst is from 0x%x to 0x%x", addrInfoPtr->sa, addrInfoPtr->ta);
    LE_TEST_INFO("[length: %"PRIuS"]pack data is ", diagMsgPtr->dataLen);

    uint32_t i;
    for (i = 0; i < diagMsgPtr->dataLen; ++i)
    {
        LE_DEBUG("0x%02X", diagMsgPtr->dataPtr[i]);
    }
    LE_DEBUG("\n");

    if (result != TAF_DOIP_RESULT_OK)
    {
        LE_TEST_INFO("Diag message invalid.");
        return;
    }

    // We simultate uds stack to respond UDS message.
    uint8_t sendData[1024];
    size_t length = 0;
    uint8_t subfunction;

    // Response data: SID, Sub function, parameters
    LE_DEBUG("UDS SID is 0x%x, sub function is 0x%x",
        diagMsgPtr->dataPtr[0], diagMsgPtr->dataPtr[1]);

    switch (diagMsgPtr->dataPtr[0])
    {
        case 0x10: // Diagnostic Session Control
        {
            if (diagMsgPtr->dataLen != 2)
            {
                // Respond incorrect message length.
                sendData[0] = 0x7F;
                sendData[1] = 0x10;
                sendData[2] = UDS_NRC_INCORRECT_LENGTH;
                length = 3;
                break;
            }

            subfunction = diagMsgPtr->dataPtr[1] & 0x7F;
            if (subfunction < 1 || subfunction > 4)
            {
                // Respond incorrect message length.
                sendData[0] = 0x7F;
                sendData[1] = 0x10;
                sendData[2] = UDS_NRC_SUBFUNCTION_UNSUPPORTED;
                length = 3;
                break;
            }

            sendData[0] = 0x50;
            sendData[1] = subfunction;

            uint16_t p2ServerMax = 0x32;
            uint16_t p2EnhanceServerMax = 0x01F4;

            p2ServerMax = htons(p2ServerMax);
            memcpy(sendData + 2, &p2ServerMax, 2);

            p2EnhanceServerMax = htons(p2EnhanceServerMax);
            memcpy(sendData + 4, &p2EnhanceServerMax, 2);
            length = 6;
        }
        break;
        case 0x11: // ECU Reset
        {
            if (diagMsgPtr->dataLen != 2)
            {
                // Respond incorrect message length.
                sendData[0] = 0x7F;
                sendData[1] = 0x11;
                sendData[2] = UDS_NRC_INCORRECT_LENGTH;
                length = 3;
                break;
            }

            subfunction = diagMsgPtr->dataPtr[1] & 0x7F;
            if (subfunction < 1 || subfunction > 5)
            {
                // Respond subfunction unsuported.
                sendData[0] = 0x7F;
                sendData[1] = 0x11;
                sendData[2] = UDS_NRC_SUBFUNCTION_UNSUPPORTED;
                length = 3;
                break;
            }
            sendData[0] = 0x51;
            sendData[1] = subfunction;
            sendData[2] = 0xB4;  // power down time(second)
            length = 3;
        }
        break;
        case 0x19: // Read DTC information
        {
            if (diagMsgPtr->dataLen < 2)
            {
                // Respond incorrect message length.
                sendData[0] = 0x7F;
                sendData[1] = 0x11;
                sendData[2] = UDS_NRC_INCORRECT_LENGTH;
                length = 3;
                break;
            }

            subfunction = diagMsgPtr->dataPtr[1] & 0x7F;
            if ((subfunction == 0) || (subfunction >= 0x1B && subfunction <= 0x41)
                || (subfunction >= 0x43 && subfunction <= 0x54)
                || (subfunction >= 0x57 && subfunction <= 0x7F))
            {
                // Respond subfunction unsuported.
                sendData[0] = 0x7F;
                sendData[1] = 0x19;
                sendData[2] = UDS_NRC_SUBFUNCTION_UNSUPPORTED;
                length = 3;
                break;
            }

            if (subfunction == 0x01 || subfunction == 0x07
                || subfunction == 0x11 || subfunction == 0x12)
            {
                // reportNumberOfDTCByStatusMask/reportNumberOfDTCBySeverityMaskRecord/
                // reportNumberOfMirrorMemoryDTCByStatusMask/
                // reportNumberOfEmissionsRelatedOBDDTCByStatusMask
                sendData[0] = 0x59;
                sendData[1] = subfunction;
                sendData[2] = 0x2F;
                sendData[3] = 0x01;  // ISO14229-1DTCF
                sendData[4] = 0x00;
                sendData[5] = 0x01;
                length = 6;
            }
            else
            {
                // other subfunction isnot supported yet.
                sendData[0] = 0x7F;
                sendData[1] = 0x19;
                sendData[2] = UDS_NRC_SUBFUNCTION_UNSUPPORTED;
                length = 3;
            }
        }
        break;
        case 0x3E:  // Tester Present
        {
            if (diagMsgPtr->dataLen != 2)
            {
                // Respond incorrect message length.
                sendData[0] = 0x7F;
                sendData[1] = 0x3E;
                sendData[2] = UDS_NRC_INCORRECT_LENGTH;
                length = 3;
                break;
            }

            subfunction = diagMsgPtr->dataPtr[1] & 0x7F;
            if (subfunction != 0)
            {
                // Respond subfunction unsuported.
                sendData[0] = 0x7F;
                sendData[1] = 0x3E;
                sendData[2] = UDS_NRC_SUBFUNCTION_UNSUPPORTED;
                length = 3;
                break;
            }

            if ((diagMsgPtr->dataPtr[1] & 0x80) == 0)
            {
                sendData[0] = 0x7E;
                sendData[1] = 0x00;
                length = 2;
            }
            else
            {
                length = 0;  // There is no response sent by the server.
            }
        }
        break;
        default:
        {
            // Unsupported service.
            sendData[0] = 0x7F;
            sendData[1] = diagMsgPtr->dataPtr[0];
            sendData[2] = UDS_NRC_SERVICE_UNSUPPORTED;
            length = 3;
        }
        break;
    }

    if (length != 0)
    {
        // Send diagnostic response.
        respAddrInfo.sa = addrInfoPtr->ta;
        respAddrInfo.ta = addrInfoPtr->sa;
        respAddrInfo.taType = TAF_DOIP_TA_TYPE_PHYSICAL;
        respDiagMsg.dataPtr = sendData;
        respDiagMsg.dataLen = length;

        ret = taf_doip_DiagRequest(&respAddrInfo, &respDiagMsg);
        LE_TEST_ASSERT(ret == LE_OK, "Diagnostic response sending");
    }

    LE_TEST_INFO("***********Handler for Rx UDS Message (End)******");

    return;
}

static void DiagConfirmHandler
(
    const taf_doip_AddrInfo_t* addrInfoPtr,
    taf_doip_Result_t result,
    void* userPtr
)
{
    LE_TEST_INFO("Enter taf_doip_DiagConfirm.");

    LE_TEST_ASSERT(addrInfoPtr != NULL, "Address information is invalid");

    LE_TEST_INFO("Diagnostic Confirmation Info: from 0x%x to 0x%x",
        addrInfoPtr->sa, addrInfoPtr->ta);

    if (result == TAF_DOIP_RESULT_OK)
    {
        LE_TEST_INFO("Diagnostic message is sent");
    }
    else
    {
        LE_ERROR("Failed to send diagnostic message");
    }
    LE_TEST_ASSERT(result == TAF_DOIP_RESULT_OK, "Diagnostic response confirm");
}

static le_result_t DoipEntityStart
(
    void
)
{
    le_result_t ret;

    DoipEntityRef = CreateDoipEntityRef();
    if (DoipEntityRef == NULL)
    {
        LE_ERROR("Failed to create doip entity");
        return LE_FAULT;
    }

    ret = taf_doip_SetVin(DoipEntityRef, DOIP_TEST_VIN);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set VIN");
        taf_doip_Delete(DoipEntityRef);
        return LE_FAULT;
    }

    ret = taf_doip_SetGid(DoipEntityRef, DOIP_TEST_GID);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to set GID");
        taf_doip_Delete(DoipEntityRef);
        return LE_FAULT;
    }

    ret = taf_doip_Start(DoipEntityRef);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to start doip entity");
        taf_doip_Delete(DoipEntityRef);
        return LE_FAULT;
    }

    PmQueryRef = taf_doip_AddPowerModeQueryHandler(DoipEntityRef,
        PowerModeQueryHandler, NULL);
    if (PmQueryRef == NULL)
    {
        LE_ERROR("Failed to register power mode query handler");
        goto errOut;
    }

    IndicationRef = taf_doip_AddDiagIndicationHandler(DoipEntityRef,
        DiagIncationHandler, NULL);
    if (IndicationRef == NULL)
    {
        LE_ERROR("Failed to register diag indication handler");
        goto errOut;
    }

    DiagConfirmRef = taf_doip_AddDiagConfirmHandler(DoipEntityRef,
        DiagConfirmHandler, NULL);
    if (DiagConfirmRef == NULL)
    {
        LE_ERROR("Failed to register diag confirmation handler");
        goto errOut;
    }

    return LE_OK;
errOut:
    if (DiagConfirmRef != NULL)
    {
        taf_doip_RemoveDiagConfirmHandler(DiagConfirmRef);
    }

    if (IndicationRef != NULL)
    {
        taf_doip_RemoveDiagIndicationHandler(IndicationRef);
    }

    if (PmQueryRef != NULL)
    {
        taf_doip_RemovePowerModeQueryHandler(PmQueryRef);
    }

    taf_doip_Stop(DoipEntityRef);
    taf_doip_Delete(DoipEntityRef);

    return LE_FAULT;
}

static void DoipEntityStop
(
    void
)
{
    if (DiagConfirmRef != NULL)
    {
        taf_doip_RemoveDiagConfirmHandler(DiagConfirmRef);
        DiagConfirmRef = NULL;
    }

    if (IndicationRef != NULL)
    {
        taf_doip_RemoveDiagIndicationHandler(IndicationRef);
        IndicationRef = NULL;
    }

    if (PmQueryRef != NULL)
    {
        taf_doip_RemovePowerModeQueryHandler(PmQueryRef);
        PmQueryRef = NULL;
    }

    if (DoipEntityRef != NULL)
    {
        taf_doip_Stop(DoipEntityRef);
        taf_doip_Delete(DoipEntityRef);
        DoipEntityRef = NULL;
    }
}

__attribute__((unused)) void DoipNodeTest_DiagMain
(
    void
)
{
    le_result_t ret;

    LE_TEST_INFO("Enter diagnostic loop.");

    CurrConnectionCnt = 0;
    ret = DoipEntityStart();
    LE_TEST_ASSERT(ret == LE_OK, "DoIP entity start");

    // Using a tester in another device to connect and send uds commands for testing.
}

