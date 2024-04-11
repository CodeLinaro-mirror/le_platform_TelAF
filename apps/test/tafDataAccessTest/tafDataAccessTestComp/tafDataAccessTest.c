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
#include "legato.h"
#include "tafDataAccessComp.h"
#include "tafDataAccessTest.h"

static void SignalHandler
(
    int sigNum
)
{
    LE_TEST_INFO("=== telaf Data Access test END ===");
    LE_TEST_EXIT;
}

static void DEMTableReset
(
)
{
    le_result_t ret;
    ret = taf_DataAccess_DeleteAllData();
    LE_TEST_ASSERT(ret == LE_OK, "DTCReset");
}

__attribute__((unused)) void TestDTCInsert
(
    void
)
{
    LE_TEST_INFO("DTC insert testing");
    le_result_t ret;
    uint8_t status;

    status = 1;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC0, status, 1);
    LE_TEST_ASSERT(ret == LE_OK, "Test SetDTCStatus");

    taf_DataAccess_NumOfDTC_t numOfDtc;
    ret = taf_DataAccess_GetNumOfDtcByStatusMask(status, &numOfDtc);
    LE_TEST_ASSERT(ret == LE_OK, "Test GetNumOfDtcByStatusMask");
    LE_TEST_ASSERT(numOfDtc.dtcCnt == 1, "Test DTC Count");

    taf_DataAccess_DTCStatusRec_t dtcStatusRec;
    ret = taf_DataAccess_GetDtcByStatusMask(status, &dtcStatusRec);
    LE_TEST_ASSERT(ret == LE_OK, "Test GetDtcByStatusMask");
    LE_DEBUG("Available mask: 0x%x", dtcStatusRec.availableMask);

    taf_DataAccess_DTCStatus_t *statusPtr;
    le_dls_Link_t* linkPtr = le_dls_Pop(&dtcStatusRec.dtcStatusRecList);
    while (linkPtr != NULL)
    {
        statusPtr = CONTAINER_OF(linkPtr, taf_DataAccess_DTCStatus_t, link);
        if (statusPtr != NULL)
        {
            LE_TEST_INFO("DTC: 0x%x", statusPtr->dtc);
            LE_TEST_INFO("Status: 0x%x", statusPtr->status);
            le_mem_Release(statusPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&dtcStatusRec.dtcStatusRecList);
    }

    status = 0x55;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC0, status, 2);
    LE_TEST_ASSERT(ret == LE_OK, "Test SetDTCStatus");

    ret = taf_DataAccess_GetNumOfDtcByStatusMask(status, &numOfDtc);
    LE_TEST_ASSERT(ret == LE_OK, "Test GetNumOfDtcByStatusMask");
    LE_TEST_ASSERT(numOfDtc.dtcCnt == 1, "Test DTC Count");

    ret = taf_DataAccess_GetDtcByStatusMask(status, &dtcStatusRec);
    LE_TEST_ASSERT(ret == LE_OK, "Test GetDtcByStatusMask");
    LE_DEBUG("Available mask: 0x%x", dtcStatusRec.availableMask);

    linkPtr = le_dls_Pop(&dtcStatusRec.dtcStatusRecList);
    while (linkPtr != NULL)
    {
        statusPtr = CONTAINER_OF(linkPtr, taf_DataAccess_DTCStatus_t, link);
        if (statusPtr != NULL)
        {
            LE_TEST_INFO("DTC: 0x%x", statusPtr->dtc);
            LE_TEST_INFO("Status: 0x%x", statusPtr->status);
            LE_TEST_ASSERT(statusPtr->status == status, "Test DTC status read");
            LE_TEST_ASSERT(statusPtr->dtc == DATA_ACCESS_TEST_DTC0, "Test DTC status read");
            le_mem_Release(statusPtr);
        }

        // Process next node.
        linkPtr = le_dls_Pop(&dtcStatusRec.dtcStatusRecList);
    }

    LE_TEST_INFO("TestDTCInsert Exit...");
}

__attribute__((unused)) void TestEventInsert
(
    void
)
{
    LE_TEST_INFO("Event insert testing");
    le_result_t ret;
    uint8_t status;

    status = 0x40;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV1, status);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    uint8_t rdStatus;
    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(rdStatus == status, "Test taf_DataAccess_GetEventStatus");

    status = 0x40;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV2, status);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    status = 0x55;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC1_EV1, status);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");
    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC1_EV1);
    LE_TEST_ASSERT(rdStatus == status, "Test taf_DataAccess_GetEventStatus");

    LE_TEST_INFO("TestEventInsert Exit...");
}

__attribute__((unused)) void TestDTCOperation
(
    void
)
{
    LE_TEST_INFO("TestDTCOperation testing");
    le_result_t ret;
    uint8_t status1, status2;
    uint8_t occCounter1, occCounter2;

    // Insert
    status1 = 0x40;
    occCounter1 = 2;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC0, status1, occCounter1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    status2 = 0x33;
    occCounter2 = 3;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC1, status2, occCounter2);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    uint8_t rdStat = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdStat == status1, "Test taf_DataAccess_SetDTCStatus");

    rdStat = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC1);
    LE_TEST_ASSERT(rdStat == status2, "Test taf_DataAccess_SetDTCStatus");

    uint8_t rdOccCounter = taf_DataAccess_GetDTCOccurrenceCounter(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdOccCounter == occCounter1, "Test taf_DataAccess_GetDTCOccurrenceCounter");

    rdOccCounter = taf_DataAccess_GetDTCOccurrenceCounter(DATA_ACCESS_TEST_DTC1);
    LE_TEST_ASSERT(rdOccCounter == occCounter2, "Test taf_DataAccess_GetDTCOccurrenceCounter");

    // Update
    status2 = 0x55;
    occCounter2 = 7;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC1, status2, occCounter2);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    rdStat = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdStat == status1, "Test taf_DataAccess_SetDTCStatus");

    rdStat = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC1);
    LE_TEST_ASSERT(rdStat == status2, "Test taf_DataAccess_SetDTCStatus");

    rdOccCounter = taf_DataAccess_GetDTCOccurrenceCounter(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdOccCounter == occCounter1, "Test taf_DataAccess_GetDTCOccurrenceCounter");

    rdOccCounter = taf_DataAccess_GetDTCOccurrenceCounter(DATA_ACCESS_TEST_DTC1);
    LE_TEST_ASSERT(rdOccCounter == occCounter2, "Test taf_DataAccess_GetDTCOccurrenceCounter");

    // Delete
    ret = taf_DataAccess_DeleteData(DATA_ACCESS_TEST_DTC1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_DeleteData");

    rdStat = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC1);
    LE_TEST_ASSERT(rdStat == 0, "Test taf_DataAccess_SetDTCStatus");

    rdOccCounter = taf_DataAccess_GetDTCOccurrenceCounter(DATA_ACCESS_TEST_DTC1);
    LE_TEST_ASSERT(rdOccCounter == 0, "Test taf_DataAccess_GetDTCOccurrenceCounter");

    // Query
    rdStat = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdStat == status1, "Test taf_DataAccess_SetDTCStatus");

    rdOccCounter = taf_DataAccess_GetDTCOccurrenceCounter(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdOccCounter == occCounter1, "Test taf_DataAccess_GetDTCOccurrenceCounter");

    LE_TEST_INFO("TestDTCOperation Exit...");
}

__attribute__((unused)) void TestEventOperation
(
    void
)
{
    LE_TEST_INFO("TestEventOperation testing");
    le_result_t ret;
    uint8_t status1, status2, status3;

    // Insert
    status1 = 0x40;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV1, status1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    status2 = 0x55;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV2, status2);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    status3 = 0x55;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC1_EV1, status3);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    uint8_t rdStatus;
    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(rdStatus == status1, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV2);
    LE_TEST_ASSERT(rdStatus == status2, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC1_EV1);
    LE_TEST_ASSERT(rdStatus == status3, "Test taf_DataAccess_GetEventStatus");

    // Upate
    status2 = 0x22;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV2, status2);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(rdStatus == status1, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV2);
    LE_TEST_ASSERT(rdStatus == status2, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC1_EV1);
    LE_TEST_ASSERT(rdStatus == status3, "Test taf_DataAccess_GetEventStatus");

    // Delete
    ret = taf_DataAccess_DeleteData(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_DeleteData");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV2);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    // Query
    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC1_EV1);
    LE_TEST_ASSERT(rdStatus == status3, "Test taf_DataAccess_GetEventStatus");

    LE_TEST_INFO("TestEventOperation Exit...");
}

__attribute__((unused)) void TestClearDTC
(
    void
)
{
    LE_TEST_INFO("TestClearDTC testing");
    le_result_t ret;
    uint8_t status1, status2;
    uint8_t occCounter1, occCounter2;
    uint8_t rdStatus;

    // DTC1
    status1 = 0x40;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV1, status1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    occCounter1 = 2;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC0, status1, occCounter1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV2, 0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    // DTC2
    status2 = 0x33;
    occCounter2 = 3;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC1, status2, occCounter2);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC1_EV1, status2);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    // Delete DTC1
    ret = taf_DataAccess_DeleteData(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_DeleteData");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC1_EV1);
    LE_TEST_ASSERT(rdStatus == status2, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV2);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_SetDTCStatus");

    ret = taf_DataAccess_DeleteAllData();
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_DeleteAllData");

    rdStatus = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC1);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_SetDTCStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC1_EV1);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    LE_TEST_INFO("TestClearDTC Exit...");
}

__attribute__((unused)) void TestGetNumOfDtcByStatusMask
(
    void
)
{
    LE_TEST_INFO("TestGetNumOfDtcByStatusMask testing");
    le_result_t ret;
    uint8_t status1, status2;
    uint8_t occCounter1, occCounter2;

    // DTC1
    status1 = 0x40;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV1, status1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    occCounter1 = 2;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC0, status1, occCounter1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV2, 0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    // DTC2
    status2 = 0x33;
    occCounter2 = 3;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC1, status2, occCounter2);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC1_EV1, status2);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    taf_DataAccess_NumOfDTC_t numOfDtc;
    ret = taf_DataAccess_GetNumOfDtcByStatusMask(status1, &numOfDtc);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_GetNumOfDtcByStatusMask");
    LE_TEST_ASSERT(numOfDtc.dtcCnt == 1, "Test DTC Count");

    // The same status
    status2 = 0x40;
    occCounter2 = 4;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC1, status2, occCounter2);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    ret = taf_DataAccess_GetNumOfDtcByStatusMask(status1, &numOfDtc);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_GetNumOfDtcByStatusMask");
    LE_TEST_ASSERT(numOfDtc.dtcCnt == 2, "Test DTC Count");

    // Different status. But the status mask contains all the DTC status
    status2 = 0x33;
    occCounter2 = 4;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC1, status2, occCounter2);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    ret = taf_DataAccess_GetNumOfDtcByStatusMask(0x7F, &numOfDtc);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_GetNumOfDtcByStatusMask");
    LE_TEST_ASSERT(numOfDtc.dtcCnt == 2, "Test DTC Count");

    ret = taf_DataAccess_GetNumOfDtcByStatusMask(0xC, &numOfDtc);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_GetNumOfDtcByStatusMask");
    LE_TEST_ASSERT(numOfDtc.dtcCnt == 0, "Test DTC Count");

    ret = taf_DataAccess_GetNumOfDtcByStatusMask(0x41, &numOfDtc);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_GetNumOfDtcByStatusMask");
    LE_TEST_ASSERT(numOfDtc.dtcCnt == 2, "Test DTC Count");

    LE_DEBUG("Available mask: 0x%x, formate=%d",
        numOfDtc.availableMask, numOfDtc.formatIdentifier);

    LE_TEST_INFO("TestGetNumOfDtcByStatusMask Exit...");
}

__attribute__((unused)) void TestGetAvailableStatusMask
(
    void
)
{
    LE_TEST_INFO("TestGetAvailableStatusMask testing");

    uint8_t availMsk = taf_DataAccess_GetAvailableStatusMask();
    LE_TEST_ASSERT(availMsk == 0x7F, "taf_DataAccess_GetAvailableStatusMask");

    LE_TEST_INFO("TestGetAvailableStatusMask Exit...");
}

__attribute__((unused)) void TestGetDtcFormatId
(
    void
)
{
    LE_TEST_INFO("TestGetAvailableStatusMask testing");

    uint8_t formatType = taf_DataAccess_GetDtcFormatId();
    LE_TEST_ASSERT(formatType == DTC_FORMAT_IDENTIFIER_ISO_14229, "taf_DataAccess_GetDtcFormatId");

    LE_TEST_INFO("TestGetDtcFormatId Exit...");
}

__attribute__((unused)) void TestGetDtcByStatus
(
    void
)
{
    LE_TEST_INFO("TestGetDtcByStatus testing");
    uint8_t status = 0x40;
    le_result_t ret;
    taf_DataAccess_DTCStatusRec_t dtcStaRec;

    // DTC1
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV1, status);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC0, status, 1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV2, status);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    // DTC2
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC1, status, 1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC1_EV1, status);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    // Get Result
    ret = taf_DataAccess_GetDtcByStatusMask(status, &dtcStaRec);
    LE_TEST_ASSERT(ret == LE_OK, "taf_DataAccess_GetDtcByStatusMask");
    LE_TEST_ASSERT(dtcStaRec.availableMask == 0x7F, "taf_DataAccess_GetDtcByStatusMask");

    taf_DataAccess_DTCStatus_t *statusPtr;
    int count = 0;
    le_dls_Link_t* linkPtr = le_dls_Pop(&dtcStaRec.dtcStatusRecList);
    while (linkPtr != NULL)
    {
        statusPtr = CONTAINER_OF(linkPtr, taf_DataAccess_DTCStatus_t, link);
        if (statusPtr != NULL)
        {
            LE_TEST_INFO("DTC: 0x%x", statusPtr->dtc);
            LE_TEST_INFO("Status: 0x%x", statusPtr->status);
            LE_TEST_ASSERT(statusPtr->dtc == DATA_ACCESS_TEST_DTC0
                || statusPtr->dtc == DATA_ACCESS_TEST_DTC1,
                "taf_DataAccess_GetDtcFormatId");
            le_mem_Release(statusPtr);
        }
        count++;
        // Process next node.
        linkPtr = le_dls_Pop(&dtcStaRec.dtcStatusRecList);
    }

    LE_TEST_ASSERT(count == 2, "taf_DataAccess_GetDtcByStatusMask");

    ret = taf_DataAccess_GetDtcByStatusMask(0x7F, &dtcStaRec);
    LE_TEST_ASSERT(ret == LE_OK, "taf_DataAccess_GetDtcByStatusMask");
    LE_TEST_ASSERT(dtcStaRec.availableMask == 0x7F, "taf_DataAccess_GetDtcByStatusMask");

    count = 0;
    linkPtr = le_dls_Pop(&dtcStaRec.dtcStatusRecList);
    while (linkPtr != NULL)
    {
        statusPtr = CONTAINER_OF(linkPtr, taf_DataAccess_DTCStatus_t, link);
        if (statusPtr != NULL)
        {
            LE_TEST_INFO("DTC: 0x%x", statusPtr->dtc);
            LE_TEST_INFO("Status: 0x%x", statusPtr->status);
            LE_TEST_ASSERT(statusPtr->dtc == DATA_ACCESS_TEST_DTC0
                || statusPtr->dtc == DATA_ACCESS_TEST_DTC1,
                "taf_DataAccess_GetDtcFormatId");
            le_mem_Release(statusPtr);
        }
        count++;
        // Process next node.
        linkPtr = le_dls_Pop(&dtcStaRec.dtcStatusRecList);
    }

    LE_TEST_ASSERT(count == 2, "taf_DataAccess_GetDtcByStatusMask");


    LE_TEST_INFO("TestGetDtcByStatus Exit...");
}

__attribute__((unused)) void TestGetSupportedDTC
(
    void
)
{
    LE_TEST_INFO("TestGetSupportedDTC testing");

    le_result_t ret;
    taf_DataAccess_DTCStatusRec_t dtcStaRec;

    ret = taf_DataAccess_GetSupportedDtc(&dtcStaRec);
    LE_TEST_ASSERT(ret == LE_OK, "taf_DataAccess_GetSupportedDtc");
    LE_TEST_ASSERT(dtcStaRec.availableMask == 0x7F, "taf_DataAccess_GetSupportedDtc");

    taf_DataAccess_DTCStatus_t *statusPtr;
    int count = 0;
    le_dls_Link_t* linkPtr = le_dls_Pop(&dtcStaRec.dtcStatusRecList);
    while (linkPtr != NULL)
    {
        statusPtr = CONTAINER_OF(linkPtr, taf_DataAccess_DTCStatus_t, link);
        if (statusPtr != NULL)
        {
            LE_TEST_INFO("DTC: 0x%x", statusPtr->dtc);
            LE_TEST_INFO("Status: 0x%x", statusPtr->status);
            LE_TEST_ASSERT(statusPtr->dtc == DATA_ACCESS_TEST_DTC0
                || statusPtr->dtc == DATA_ACCESS_TEST_DTC1,
                "taf_DataAccess_GetDtcFormatId");
            le_mem_Release(statusPtr);
        }
        count++;
        // Process next node.
        linkPtr = le_dls_Pop(&dtcStaRec.dtcStatusRecList);
    }

    LE_TEST_ASSERT(count == 2, "taf_DataAccess_GetSupportedDtc");

    LE_TEST_INFO("TestGetSupportedDTC Exit...");
}

__attribute__((unused)) void TestGetExtDataRecByDtc
(
    void
)
{
    LE_TEST_INFO("TestGetSupportedDTC testing");
    uint8_t status = 0x40;
    le_result_t ret;
    taf_DataAccess_ExtDataRec_t dtcExtDataRec;

    // DTC1
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV1, status);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC0, status, 1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV2, status);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    // DTC2
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC1, status, 2);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC1_EV1, status);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    // Get result.
    ret = taf_DataAccess_GetExtDataRecByDtc(DATA_ACCESS_TEST_DTC1, 1, &dtcExtDataRec);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_GetExtDataRecByDtc");
    LE_TEST_ASSERT(dtcExtDataRec.dtc == DATA_ACCESS_TEST_DTC1,
        "Test taf_DataAccess_GetExtDataRecByDtc");
    LE_TEST_ASSERT(dtcExtDataRec.status == status,
        "Test taf_DataAccess_GetExtDataRecByDtc");

    taf_DataAccess_ExtData_t *extDataPtr;
    int count = 0;
    le_dls_Link_t* linkPtr = le_dls_Pop(&dtcExtDataRec.extDataList);
    while (linkPtr != NULL)
    {
        extDataPtr = CONTAINER_OF(linkPtr, taf_DataAccess_ExtData_t, link);
        if (extDataPtr != NULL)
        {
            LE_TEST_INFO("record number: 0x%x", extDataPtr->recNumber);
            LE_TEST_INFO("extended data size: 0x%x", extDataPtr->extDataSize);
            LE_TEST_INFO("extended data: 0x%x", extDataPtr->extData[0]);
            LE_TEST_ASSERT(extDataPtr->recNumber == 1, "taf_DataAccess_GetExtDataRecByDtc");

            le_mem_Release(extDataPtr);
        }
        count++;
        // Process next node.
        linkPtr = le_dls_Pop(&dtcExtDataRec.extDataList);
    }

    LE_TEST_ASSERT(count == 1, "taf_DataAccess_GetExtDataRecByDtc");

    LE_TEST_INFO("TestGetExtDataRecByDtc Exit...");
}

/*======================================================================
 FUNCTION        COMPONENT_INIT
 DESCRIPTION     Component initialization
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("=== telaf Data Access test BEGIN ===");
    le_sig_SetEventHandler(SIGTERM, SignalHandler);

    ConfigModuleInit();

    DEMTableReset();
    TestDTCInsert();

    DEMTableReset();
    TestEventInsert();

    DEMTableReset();
    TestDTCOperation();

    DEMTableReset();
    TestEventOperation();

    DEMTableReset();
    TestClearDTC();

    DEMTableReset();
    TestGetNumOfDtcByStatusMask();

    DEMTableReset();
    TestGetAvailableStatusMask();

    DEMTableReset();
    TestGetDtcFormatId();

    DEMTableReset();
    TestGetDtcByStatus();

    DEMTableReset();
    TestGetSupportedDTC();

    DEMTableReset();
    TestGetExtDataRecByDtc();
}