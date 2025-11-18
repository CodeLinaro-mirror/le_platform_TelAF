/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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

__attribute__((unused)) static void DEMTableReset
(
)
{
    le_result_t ret;
    LE_TEST_INFO("Reset db table");
    ret = taf_DataAccess_ResetDTCStorage();
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
    LE_TEST_INFO("numOfDtc.dtcCnt=%d", numOfDtc.dtcCnt);
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

    LE_TEST_INFO("TestEventInsert Exit...");
}

__attribute__((unused)) void TestDTCOperation
(
    void
)
{
    LE_TEST_INFO("TestDTCOperation testing");
    le_result_t ret;
    uint8_t status1;
    uint8_t occCounter1;

    // Insert
    status1 = 0x40;
    occCounter1 = 2;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC0, status1, occCounter1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    uint8_t rdStat = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdStat == status1, "Test taf_DataAccess_SetDTCStatus");

    uint8_t rdOccCounter = taf_DataAccess_GetDTCOccurrenceCounter(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdOccCounter == occCounter1, "Test taf_DataAccess_GetDTCOccurrenceCounter");

    rdStat = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdStat == status1, "Test taf_DataAccess_SetDTCStatus");

    rdOccCounter = taf_DataAccess_GetDTCOccurrenceCounter(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdOccCounter == occCounter1, "Test taf_DataAccess_GetDTCOccurrenceCounter");

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
    uint8_t status1, status2;

    // Insert
    status1 = 0x40;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV1, status1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    status2 = 0x55;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV2, status2);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    uint8_t rdStatus;
    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(rdStatus == status1, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV2);
    LE_TEST_ASSERT(rdStatus == status2, "Test taf_DataAccess_GetEventStatus");

    // Upate
    status2 = 0x22;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV2, status2);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(rdStatus == status1, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV2);
    LE_TEST_ASSERT(rdStatus == status2, "Test taf_DataAccess_GetEventStatus");

    // Delete
    ret = taf_DataAccess_DeleteData(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_DeleteData");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV2);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    // Test failed counter.
    uint8_t testFailedCounter = 6;
    uint8_t readFailedCounter;
    readFailedCounter = taf_DataAccess_GetEventFailedCounter(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(readFailedCounter == 0, "Test taf_DataAccess_GetEventFailedCounter");

    ret = taf_DataAccess_SetEventFailedCounter(DATA_ACCESS_TEST_DTC0_EV1, testFailedCounter);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventFailedCounter");

    readFailedCounter = taf_DataAccess_GetEventFailedCounter(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(readFailedCounter == 6, "Test taf_DataAccess_GetEventFailedCounter");

    taf_DataAccess_DeleteAllData();
    readFailedCounter = taf_DataAccess_GetEventFailedCounter(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(readFailedCounter == 6, "Test taf_DataAccess_GetEventFailedCounter");   

    LE_TEST_INFO("TestEventOperation Exit...");
}

__attribute__((unused)) void TestClearDTC
(
    void
)
{
    LE_TEST_INFO("TestClearDTC testing");
    le_result_t ret;
    uint8_t status1;
    uint8_t occCounter1;
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

    // Delete DTC1
    ret = taf_DataAccess_DeleteData(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_DeleteData");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV2);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_SetDTCStatus");

    // Delete All DTCs with no suppression
    ret = taf_DataAccess_DeleteAllData();
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_DeleteAllData");

    rdStatus = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_SetDTCStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    // Delete All DTCs with suppression
    // DTC1
    status1 = 0x40;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV1, status1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    occCounter1 = 2;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC0, status1, occCounter1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV2, 0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    ret = taf_DataAccess_SetAllDTCSuppression(1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetAllDTCSuppression");

    ret = taf_DataAccess_DeleteAllData();
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_DeleteAllData");

    rdStatus = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdStatus == status1, "Test taf_DataAccess_GetDTCStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(rdStatus == status1, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV2);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    ret = taf_DataAccess_SetDTCSuppression(DATA_ACCESS_TEST_DTC0, 0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCSuppression");

    ret = taf_DataAccess_DeleteAllData();
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_DeleteAllData");

    rdStatus = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetDTCStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV2);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    ret = taf_DataAccess_SetAllDTCSuppression(0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetAllDTCSuppression");

    ret = taf_DataAccess_DeleteAllData();
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_DeleteAllData");

    rdStatus = taf_DataAccess_GetDTCStatus(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetDTCStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV1);
    LE_TEST_ASSERT(rdStatus == 0, "Test taf_DataAccess_GetEventStatus");

    rdStatus = taf_DataAccess_GetEventStatus(DATA_ACCESS_TEST_DTC0_EV2);
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
    uint8_t status1;
    uint8_t occCounter1;

    // DTC1
    status1 = 0x40;
    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV1, status1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    occCounter1 = 2;
    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC0, status1, occCounter1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    ret = taf_DataAccess_SetEventStatus(DATA_ACCESS_TEST_DTC0_EV2, 0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetEventStatus");

    taf_DataAccess_NumOfDTC_t numOfDtc;
    ret = taf_DataAccess_GetNumOfDtcByStatusMask(status1, &numOfDtc);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_GetNumOfDtcByStatusMask");
    LE_TEST_ASSERT(numOfDtc.dtcCnt == 1, "Test DTC Count");

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
#ifdef LE_CONFIG_DIAG_FEATURE_A
    LE_TEST_ASSERT(formatType == DTC_FORMAT_IDENTIFIER_SAE_J2012_04,
        "taf_DataAccess_GetDtcFormatId");
#else
    LE_TEST_ASSERT(formatType == DTC_FORMAT_IDENTIFIER_ISO_14229,
        "taf_DataAccess_GetDtcFormatId");
#endif
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
            LE_TEST_ASSERT(statusPtr->dtc == DATA_ACCESS_TEST_DTC0,
                "taf_DataAccess_GetDtcFormatId");
            le_mem_Release(statusPtr);
        }
        count++;
        // Process next node.
        linkPtr = le_dls_Pop(&dtcStaRec.dtcStatusRecList);
    }

    LE_TEST_ASSERT(count == 1, "taf_DataAccess_GetDtcByStatusMask");

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
            LE_TEST_ASSERT(statusPtr->dtc == DATA_ACCESS_TEST_DTC0,
                "taf_DataAccess_GetDtcFormatId");
            le_mem_Release(statusPtr);
        }
        count++;
        // Process next node.
        linkPtr = le_dls_Pop(&dtcStaRec.dtcStatusRecList);
    }

    LE_TEST_ASSERT(count == 1, "taf_DataAccess_GetDtcByStatusMask");


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
#ifdef LE_CONFIG_DIAG_FEATURE_A
            LE_TEST_ASSERT(statusPtr->dtc == DATA_ACCESS_TEST_DTC0 || 
                statusPtr->dtc == DATA_ACCESS_TEST_DTC1,
                "TestGetSupportedDTC");
#else
            LE_TEST_ASSERT(statusPtr->dtc == DATA_ACCESS_TEST_DTC0,
                "TestGetSupportedDTC");
#endif
            le_mem_Release(statusPtr);
        }
        count++;
        // Process next node.
        linkPtr = le_dls_Pop(&dtcStaRec.dtcStatusRecList);
    }
#ifdef LE_CONFIG_DIAG_FEATURE_A
    LE_TEST_ASSERT(count == 2, "taf_DataAccess_GetSupportedDtc");
#else
    LE_TEST_ASSERT(count == 1, "taf_DataAccess_GetSupportedDtc");
#endif
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

    // Get result.
    ret = taf_DataAccess_GetExtDataRecByDtc(DATA_ACCESS_TEST_DTC0, 1, &dtcExtDataRec);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_GetExtDataRecByDtc");
    LE_TEST_ASSERT(dtcExtDataRec.dtc == DATA_ACCESS_TEST_DTC0,
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

__attribute__((unused)) void TestDTCActivation
(
    void
)
{
    le_result_t ret;
    uint8_t act;

    ret = taf_DataAccess_SetDTCActivation(DATA_ACCESS_TEST_DTC0, 0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCActivation");

    act = taf_DataAccess_GetDTCActivation(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(ret == LE_OK && act == 0, "Test taf_DataAccess_GetDTCActivation");

    ret = taf_DataAccess_SetDTCActivation(DATA_ACCESS_TEST_DTC0, 1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCActivation");

    act = taf_DataAccess_GetDTCActivation(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(ret == LE_OK && act == 1, "Test taf_DataAccess_GetDTCActivation");
}

__attribute__((unused)) void TestDTCSuppression
(
    void
)
{
    le_result_t ret;
    uint8_t suppress;

    ret = taf_DataAccess_SetDTCSuppression(DATA_ACCESS_TEST_DTC0, 0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCSuppression");

    suppress = taf_DataAccess_GetDTCSuppression(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(ret == LE_OK && suppress == 0, "Test taf_DataAccess_GetDTCSuppression");

    ret = taf_DataAccess_SetDTCSuppression(DATA_ACCESS_TEST_DTC0, 1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCSuppression");

    suppress = taf_DataAccess_GetDTCSuppression(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(ret == LE_OK && suppress == 1, "Test taf_DataAccess_GetDTCSuppression");

    ret = taf_DataAccess_SetDTCSuppression(DATA_ACCESS_TEST_DTC0, 0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCSuppression");

    ret = taf_DataAccess_SetAllDTCSuppression(1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetAllDTCSuppression");

    suppress = taf_DataAccess_GetDTCSuppression(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(ret == LE_OK && suppress == 1, "Test taf_DataAccess_GetDTCSuppression");

    ret = taf_DataAccess_SetAllDTCSuppression(0);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetAllDTCSuppression");

    suppress = taf_DataAccess_GetDTCSuppression(DATA_ACCESS_TEST_DTC0);
    LE_TEST_ASSERT(ret == LE_OK && suppress == 0, "Test taf_DataAccess_GetDTCSuppression");
}

__attribute__((unused)) void TestGetDTCData
(
    void
)
{
    le_result_t ret;
    le_dls_List_t list;
    uint8_t status = 0x40;
    uint8_t buf[10] = {0x11, 0x22, 0x33, 0x44, 0x55};

    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC0, status, 1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    list = LE_DLS_LIST_INIT;
    taf_DataAccess_DidNode_t node;
    node.did = DATA_ACCESS_DID;
    node.len = 5;
    node.val = buf;
    node.link = LE_DLS_LINK_INIT;
    le_dls_Queue(&list, &node.link);

    ret = taf_DataAccess_SetSnapshotData(DATA_ACCESS_TEST_DTC0, &list, NULL);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetSnapshotData");

    le_dls_List_t dataList;
    taf_DataAccess_DtcDataInfo_t *dataPtr;
    ret = taf_DataAccess_GetDTCData(DATA_ACCESS_TEST_DTC0, &dataList);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_GetDTCData");

    int count = 0;
    le_dls_Link_t* linkPtr = le_dls_Peek(&dataList);
    while (linkPtr != NULL)
    {
        dataPtr = CONTAINER_OF(linkPtr, taf_DataAccess_DtcDataInfo_t, link);
        if (dataPtr != NULL)
        {
            LE_TEST_INFO("DTC code: 0x%x", dataPtr->dtcCode);
            LE_TEST_INFO("record number: 0x%x", dataPtr->recordNum);
            LE_TEST_INFO("Data type: 0x%x", dataPtr->dataType);

            for (int i = 0; i < dataPtr->dataLen; i++)
            {
                LE_TEST_INFO("0x%x", dataPtr->dataValue[i]);
                if (dataPtr->dataType == SNAPSHOT_DATA)
                {
                    LE_TEST_ASSERT(dataPtr->dataValue[i] == buf[i],
                        "Test DID value");
                }
            }
        }
        count++;
        // Process next node.
        linkPtr = le_dls_PeekNext(&dataList, linkPtr);
    }

    taf_DataAccess_ReleaseDTCData(&dataList);
    LE_TEST_ASSERT(count != 0, "TestGetDTCData");
}

__attribute__((unused)) void TestSnapshotIdentification
(
    void
)
{
    le_result_t ret;
    le_dls_List_t list;
    uint8_t status = 0x40;
    uint8_t buf[10] = {0x11, 0x22, 0x33, 0x44, 0x55};

    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC0, status, 1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    list = LE_DLS_LIST_INIT;
    taf_DataAccess_DidNode_t node;
    node.did = DATA_ACCESS_DID;
    node.len = 5;
    node.val = buf;
    node.link = LE_DLS_LINK_INIT;
    le_dls_Queue(&list, &node.link);

    ret = taf_DataAccess_SetSnapshotData(DATA_ACCESS_TEST_DTC0, &list, NULL);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetSnapshotData");

    taf_DataAccess_SnapshotInfoRec_t snapshotInfo;
    taf_DataAccess_SnapshotInfo_t *snapshotNodePtr;

    snapshotInfo.snapshotInfoList = LE_DLS_LIST_INIT;
    ret = taf_DataAccess_GetSnapshotIdentification(&snapshotInfo);

    int count = 0;
    le_dls_Link_t* linkPtr = le_dls_Pop(&snapshotInfo.snapshotInfoList);
    while (linkPtr != NULL)
    {
        snapshotNodePtr = CONTAINER_OF(linkPtr, taf_DataAccess_SnapshotInfo_t, link);
        if (snapshotNodePtr != NULL)
        {
            LE_TEST_INFO("DTC code: 0x%x", snapshotNodePtr->dtc);
            LE_TEST_INFO("record number: 0x%x", snapshotNodePtr->recNumber);
            LE_TEST_ASSERT(snapshotNodePtr->dtc == DATA_ACCESS_TEST_DTC0, "Test snapshot DTC");
            LE_TEST_ASSERT(snapshotNodePtr->recNumber == 1 || snapshotNodePtr->recNumber == 2,
                "Test snapshot record number");
            le_mem_Release(snapshotNodePtr);
        }
        count++;
        // Process next node.
        linkPtr = le_dls_Pop(&snapshotInfo.snapshotInfoList);
    }
    LE_TEST_ASSERT(count != 0, "TestSnapshotIdentification");
}

__attribute__((unused)) void TestGetSnapshotRecord
(
    void
)
{
    le_result_t ret;
    le_dls_List_t list;
    uint8_t status = 0x40;
    uint8_t buf[10] = {0x11, 0x22, 0x33, 0x44, 0x55};

    ret = taf_DataAccess_SetDTCStatus(DATA_ACCESS_TEST_DTC0, status, 1);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetDTCStatus");

    list = LE_DLS_LIST_INIT;
    taf_DataAccess_DidNode_t node;
    node.did = DATA_ACCESS_DID;
    node.len = 5;
    node.val = buf;
    node.link = LE_DLS_LINK_INIT;
    le_dls_Queue(&list, &node.link);

    ret = taf_DataAccess_SetSnapshotData(DATA_ACCESS_TEST_DTC0, &list, NULL);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_SetSnapshotData");

    taf_DataAccess_SnapshotDataRec_t snapshotRec;
    taf_DataAccess_SnapshotData_t *snapshotData;

    ret = taf_DataAccess_GetSnapshotRecByDtc(DATA_ACCESS_TEST_DTC0, 0xFF, &snapshotRec);
    LE_TEST_ASSERT(ret == LE_OK, "Test taf_DataAccess_GetSnapshotRecByDtc");
    LE_TEST_INFO("record count: %" PRIuS, le_dls_NumLinks(&snapshotRec.snapshotDataList));
#ifdef LE_CONFIG_DIAG_FEATURE_A
    LE_TEST_ASSERT(le_dls_NumLinks(&snapshotRec.snapshotDataList) == 1,
        "Test taf_DataAccess_GetSnapshotRecByDtc");  // Only last.
#else
    LE_TEST_ASSERT(le_dls_NumLinks(&snapshotRec.snapshotDataList) >= 2 &&
        le_dls_NumLinks(&snapshotRec.snapshotDataList) <= 3,
        "Test taf_DataAccess_GetSnapshotRecByDtc");  // At least 2. first and last.
#endif
    LE_TEST_INFO("DTC code: 0x%x", snapshotRec.dtc);
    LE_TEST_INFO("DTC status: 0x%x", snapshotRec.status);

    int count = 0;
    le_dls_Link_t* linkPtr = le_dls_Pop(&snapshotRec.snapshotDataList);
    while (linkPtr != NULL)
    {
        snapshotData = CONTAINER_OF(linkPtr, taf_DataAccess_SnapshotData_t, link);
        if (snapshotData != NULL)
        {
            LE_TEST_INFO("record number: 0x%x", snapshotData->recNumber);

            LE_TEST_ASSERT(snapshotData->recNumber == 1 || snapshotData->recNumber == 2,
                "Test snapshot record number");
            LE_TEST_ASSERT(snapshotData->recDidSize == 1, "Test snapshot record DID size");
            LE_TEST_ASSERT(snapshotData->didSet[0].did == DATA_ACCESS_DID, "Test snapshot record DID");
            LE_TEST_ASSERT(snapshotData->didSet[0].didDataSize == 5, "Test snapshot data size");
            le_mem_Release(snapshotData);
        }
        count++;
        // Process next node.
        linkPtr = le_dls_Pop(&snapshotRec.snapshotDataList);
    }
    LE_TEST_ASSERT(count != 0, "TestGetSnapshotRecord");
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

    taf_DataAccess_Init();

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

    DEMTableReset();
    TestDTCActivation();

    DEMTableReset();
    TestDTCSuppression();

    DEMTableReset();
    TestGetDTCData();

    DEMTableReset();
    TestSnapshotIdentification();

    DEMTableReset();
    TestGetSnapshotRecord();

    LE_TEST_INFO("=== telaf Data Access test END ===");
}