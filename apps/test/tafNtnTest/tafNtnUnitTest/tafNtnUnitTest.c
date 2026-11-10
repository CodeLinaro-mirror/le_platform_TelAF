/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#define DEFAULT_PHONE_ID 2
#define INVALID_PHONE_ID 0

//--------------------------------------------------------------------------------------------------
/**
 * Returns true when a PA-backed API result is acceptable on the default PA. Session, list, location
 * fix and packet objects are managed in the service layer and therefore succeed without a modem;
 * the operations that reach the PA return LE_NOT_IMPLEMENTED (or LE_UNAVAILABLE) on the default PA.
 */
//--------------------------------------------------------------------------------------------------
static bool PaResultAcceptable
(
    le_result_t result ///< [IN] Result to check.
)
{
    return (result == LE_OK) || (result == LE_NOT_IMPLEMENTED) || (result == LE_UNAVAILABLE) ||
        (result == LE_UNSUPPORTED);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for NTN state change.
 */
//--------------------------------------------------------------------------------------------------
static void StateChangeHandler
(
    taf_ntn_SessionRef_t sessionRef, ///< [IN] NTN session reference.
    taf_ntn_State_t state,           ///< [IN] NTN state.
    void* contextPtr                 ///< [IN] Handler context.
)
{
    LE_INFO("NTN state change : %d", state);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for NTN capability change.
 */
//--------------------------------------------------------------------------------------------------
static void CapabilitiesChangeHandler
(
    taf_ntn_SessionRef_t sessionRef, ///< [IN] NTN session reference.
    void* contextPtr                 ///< [IN] Handler context.
)
{
    LE_INFO("NTN capabilities changed.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for NTN data acknowledgement.
 */
//--------------------------------------------------------------------------------------------------
static void DataAckHandler
(
    taf_ntn_SessionRef_t sessionRef, ///< [IN] NTN session reference.
    taf_ntn_DataAckResult_t result,  ///< [IN] Data acknowledgement result.
    uint64_t transactionId,          ///< [IN] Transaction ID.
    void* contextPtr                 ///< [IN] Handler context.
)
{
    LE_INFO("NTN data ack : result %d transactionId %" PRIu64, result, transactionId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for incoming NTN data.
 */
//--------------------------------------------------------------------------------------------------
static void IncomingDataHandler
(
    taf_ntn_SessionRef_t sessionRef,  ///< [IN] NTN session reference.
    taf_ntn_DataPacketRef_t packetRef,///< [IN] Incoming data packet reference.
    void* contextPtr                  ///< [IN] Handler context.
)
{
    LE_INFO("NTN incoming data packet received.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for terrestrial cellular coverage.
 */
//--------------------------------------------------------------------------------------------------
static void CellularCoverageHandler
(
    taf_ntn_SessionRef_t sessionRef, ///< [IN] NTN session reference.
    bool available,                  ///< [IN] True if cellular coverage is available.
    void* contextPtr                 ///< [IN] Handler context.
)
{
    LE_INFO("NTN cellular coverage available : %d", available);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for location fix request.
 */
//--------------------------------------------------------------------------------------------------
static void LocationFixRequestHandler
(
    taf_ntn_SessionRef_t sessionRef,           ///< [IN] NTN session reference.
    taf_ntn_LocationFixRequestReason_t reason, ///< [IN] Location fix request reason.
    void* contextPtr                           ///< [IN] Handler context.
)
{
    LE_INFO("NTN location fix requested : reason %d", reason);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for NTN band update.
 */
//--------------------------------------------------------------------------------------------------
static void BandUpdateHandler
(
    taf_ntn_SessionRef_t sessionRef, ///< [IN] NTN session reference.
    uint64_t band,                   ///< [IN] NTN operating band.
    void* contextPtr                 ///< [IN] Handler context.
)
{
    LE_INFO("NTN band update : %" PRIu64, band);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test add/remove of every NTN event handler (BEFORE enabling NTN).
 */
//--------------------------------------------------------------------------------------------------
static void TestNtnHandlers
(
    void
)
{
    LE_TEST_INFO("--- Setting up event listeners BEFORE enabling NTN ---");

    // Test core functionality - add handlers BEFORE NTN is enabled
    taf_ntn_StateChangeHandlerRef_t stateRef =
        taf_ntn_AddStateChangeHandler(StateChangeHandler, NULL);
    LE_TEST_OK(stateRef != NULL, "taf_ntn_AddStateChangeHandler - !NULL");
    LE_INFO("Added StateChangeHandler: stateRef=%p", stateRef);

    taf_ntn_CapabilitiesChangeHandlerRef_t capRef =
        taf_ntn_AddCapabilitiesChangeHandler(CapabilitiesChangeHandler, NULL);
    LE_TEST_OK(capRef != NULL, "taf_ntn_AddCapabilitiesChangeHandler - !NULL");
    LE_INFO("Added CapabilitiesChangeHandler: capRef=%p", capRef);

    taf_ntn_DataAckHandlerRef_t dataAckRef = taf_ntn_AddDataAckHandler(DataAckHandler, NULL);
    LE_TEST_OK(dataAckRef != NULL, "taf_ntn_AddDataAckHandler - !NULL");
    LE_INFO("Added DataAckHandler: dataAckRef=%p", dataAckRef);

    taf_ntn_IncomingDataHandlerRef_t incomingRef =
        taf_ntn_AddIncomingDataHandler(IncomingDataHandler, NULL);
    LE_TEST_OK(incomingRef != NULL, "taf_ntn_AddIncomingDataHandler - !NULL");
    LE_INFO("Added IncomingDataHandler: incomingRef=%p", incomingRef);

    taf_ntn_CellularCoverageHandlerRef_t coverageRef =
        taf_ntn_AddCellularCoverageHandler(CellularCoverageHandler, NULL);
    LE_TEST_OK(coverageRef != NULL, "taf_ntn_AddCellularCoverageHandler - !NULL");
    LE_INFO("Added CellularCoverageHandler: coverageRef=%p", coverageRef);

    taf_ntn_LocationFixRequestHandlerRef_t locReqRef =
        taf_ntn_AddLocationFixRequestHandler(LocationFixRequestHandler, NULL);
    LE_TEST_OK(locReqRef != NULL, "taf_ntn_AddLocationFixRequestHandler - !NULL");
    LE_INFO("Added LocationFixRequestHandler: locReqRef=%p", locReqRef);

    taf_ntn_BandUpdateHandlerRef_t bandRef = taf_ntn_AddBandUpdateHandler(BandUpdateHandler, NULL);
    LE_TEST_OK(bandRef != NULL, "taf_ntn_AddBandUpdateHandler - !NULL");
    LE_INFO("Added BandUpdateHandler: bandRef=%p", bandRef);

    LE_TEST_INFO("--- Event listeners setup complete, ready for NTN operations ---");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test NTN support, session lifecycle, and basic operations.
 * Returns a session reference (or NULL) that the caller must delete.
 */
//--------------------------------------------------------------------------------------------------
static taf_ntn_SessionRef_t TestNtnSupportAndSession
(
    uint8_t phoneId ///< [IN] Phone ID to test against.
)
{
    // Test core functionality first
    bool supported = false;
    le_result_t result = taf_ntn_IsSupported(&supported);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_IsSupported - result %d", result);
    LE_INFO("IsSupported result: supported=%d", supported);

    taf_ntn_SessionRef_t sessionRef = NULL;
    result = taf_ntn_CreateSession(phoneId, &sessionRef);
    LE_TEST_OK(result == LE_OK && sessionRef != NULL, "taf_ntn_CreateSession - LE_OK");
    LE_INFO("Created sessionRef: %p", sessionRef);

    taf_ntn_State_t state = TAF_NTN_STATE_DISABLED;
    result = taf_ntn_GetState(sessionRef, &state);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_GetState - result %d", result);
    LE_INFO("GetState result: state=%d (TAF_NTN_STATE_DISABLED=0)", state);

    uint64_t maxDataSize = 0;
    result = taf_ntn_GetCapabilityMaxDataSize(sessionRef, &maxDataSize);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_GetCapabilityMaxDataSize - result %d", result);
    LE_INFO("GetCapabilityMaxDataSize result: maxDataSize=%" PRIu64, maxDataSize);

    // Test parameter validation
    result = taf_ntn_IsSupported(NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_ntn_IsSupported null - LE_BAD_PARAMETER");
    LE_INFO("IsSupported NULL check result: %d", result);

    result = taf_ntn_CreateSession(phoneId, NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_ntn_CreateSession null out - LE_BAD_PARAMETER");
    LE_INFO("CreateSession NULL out check result: %d", result);

    taf_ntn_SessionRef_t badSessionRef = NULL;
    result = taf_ntn_CreateSession(INVALID_PHONE_ID, &badSessionRef);
    LE_TEST_OK(result == LE_BAD_PARAMETER,
        "taf_ntn_CreateSession invalid phoneId - LE_BAD_PARAMETER");
    LE_INFO("CreateSession invalid phoneId result: %d", result);

    result = taf_ntn_GetState(NULL, &state);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_ntn_GetState null session - LE_BAD_PARAMETER");
    LE_INFO("GetState NULL session check result: %d", result);

    result = taf_ntn_SetNtnMode(NULL, true, false);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_ntn_SetNtnMode null - LE_BAD_PARAMETER");
    LE_INFO("SetNtnMode NULL session check result: %d", result);

    return sessionRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test NTN mode operations (REQUIRES NTN to be ACTIVE).
 */
//--------------------------------------------------------------------------------------------------
static void TestNtnModeOperations
(
    taf_ntn_SessionRef_t sessionRef ///< [IN] NTN session reference.
)
{
    LE_TEST_INFO("--- Testing NTN mode operations (NTN MUST be ACTIVE) ---");

    // Enable NTN mode
    le_result_t result = taf_ntn_SetNtnMode(sessionRef, true, false);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_SetNtnMode enable - result %d", result);
    LE_INFO("SetNtnMode enable result: %d", result);

    // Enable cellular scan (requires NTN to be active)
    result = taf_ntn_SetCellularScan(sessionRef, true);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_SetCellularScan enable - result %d", result);
    LE_INFO("SetCellularScan enable result: %d", result);

    // Disable cellular scan
    result = taf_ntn_SetCellularScan(sessionRef, false);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_SetCellularScan disable - result %d", result);
    LE_INFO("SetCellularScan disable result: %d", result);

    // Disable NTN mode
    result = taf_ntn_SetNtnMode(sessionRef, false, false);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_SetNtnMode disable - result %d", result);
    LE_INFO("SetNtnMode disable result: %d", result);

    LE_TEST_INFO("--- NTN mode operations complete ---");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test NTN data operations (REQUIRES NTN to be ACTIVE).
 */
//--------------------------------------------------------------------------------------------------
static void TestNtnData
(
    taf_ntn_SessionRef_t sessionRef ///< [IN] NTN session reference.
)
{
    LE_TEST_INFO("--- Testing NTN data operations (NTN MUST be ACTIVE) ---");

    // Enable NTN mode first
    le_result_t result = taf_ntn_SetNtnMode(sessionRef, true, false);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_SetNtnMode enable - result %d", result);
    LE_INFO("SetNtnMode enable result: %d", result);

    // Test core functionality - send data (requires NTN to be active)
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    uint64_t transactionId = 0;
    result = taf_ntn_SendData(sessionRef, data, sizeof(data), false, &transactionId);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_SendData - result %d", result);
    LE_INFO("SendData result: transactionId=%" PRIu64 ", dataSize=%zu", transactionId, sizeof(data));

    // Abort data (requires NTN to be active)
    result = taf_ntn_AbortData(sessionRef);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_AbortData - result %d", result);
    LE_INFO("AbortData result: %d", result);

    // Disable NTN mode
    result = taf_ntn_SetNtnMode(sessionRef, false, false);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_SetNtnMode disable - result %d", result);
    LE_INFO("SetNtnMode disable result: %d", result);

    // Test parameter validation
    result = taf_ntn_SendData(NULL, data, sizeof(data), false, &transactionId);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_ntn_SendData null session - LE_BAD_PARAMETER");
    LE_INFO("SendData NULL session check result: %d", result);

    uint8_t rxBuf[TAF_NTN_MAX_DATA_BYTES];
    size_t rxLen = sizeof(rxBuf);
    result = taf_ntn_ReadDataPacket(NULL, rxBuf, &rxLen);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_ntn_ReadDataPacket null - LE_BAD_PARAMETER");
    LE_INFO("ReadDataPacket NULL packet check result: %d", result);

    result = taf_ntn_DeleteDataPacket(NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_ntn_DeleteDataPacket null - LE_BAD_PARAMETER");
    LE_INFO("DeleteDataPacket NULL packet check result: %d", result);

    LE_TEST_INFO("--- NTN data operations complete ---");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the system selection specifier list builder and its getters.
 */
//--------------------------------------------------------------------------------------------------
static void TestNtnSystemSelection
(
    taf_ntn_SessionRef_t sessionRef ///< [IN] NTN session reference.
)
{
    LE_TEST_INFO("--- Testing NTN system selection (setup BEFORE enabling NTN) ---");

    // Test core functionality first
    taf_ntn_SystemSelectionListRef_t listRef = NULL;
    le_result_t result = taf_ntn_CreateSystemSelectionList(sessionRef, &listRef);
    LE_TEST_OK(result == LE_OK && listRef != NULL, "taf_ntn_CreateSystemSelectionList - LE_OK");
    LE_INFO("Created listRef: %p", listRef);

    taf_ntn_SystemSelectionSpecifierRef_t specRef = NULL;
    result = taf_ntn_AddSystemSelectionSpecifier(listRef, &specRef);
    LE_TEST_OK(result == LE_OK && specRef != NULL, "taf_ntn_AddSystemSelectionSpecifier - LE_OK");
    LE_INFO("Created specRef: %p", specRef);

    result = taf_ntn_AddSystemSelectionPlmn(specRef, "310", "410");
    LE_TEST_OK(result == LE_OK, "taf_ntn_AddSystemSelectionPlmn - LE_OK");
    LE_INFO("Added PLMN to specRef: mcc=310, mnc=410");

    uint64_t bandMask[TAF_NTN_NTN_BAND_GROUP_NUM] = {0};
    bandMask[0] = 0x5; // bands 1 and 3.
    result = taf_ntn_AddSystemSelectionBands(specRef, bandMask, TAF_NTN_NTN_BAND_GROUP_NUM);
    LE_TEST_OK(result == LE_OK, "taf_ntn_AddSystemSelectionBands - LE_OK");
    LE_INFO("Added bands to specRef: bandMask[0]=0x%" PRIx64, bandMask[0]);

    uint64_t earfcns[] = {100, 200, 300};
    result = taf_ntn_AddSystemSelectionEarfcns(specRef, earfcns,
        sizeof(earfcns) / sizeof(earfcns[0]));
    LE_TEST_OK(result == LE_OK, "taf_ntn_AddSystemSelectionEarfcns - LE_OK");
    LE_INFO("Added EARFCNs to specRef: count=%zu", sizeof(earfcns) / sizeof(earfcns[0]));

    // Traverse and read back.
    taf_ntn_SystemSelectionSpecifierRef_t firstRef = NULL;
    result = taf_ntn_GetFirstSystemSelectionSpecifier(listRef, &firstRef);
    LE_TEST_OK(result == LE_OK && firstRef != NULL,
        "taf_ntn_GetFirstSystemSelectionSpecifier - LE_OK");
    LE_INFO("Retrieved firstRef: %p", firstRef);

    char mcc[TAF_NTN_MCC_BYTES] = {0};
    char mnc[TAF_NTN_MNC_BYTES] = {0};
    result = taf_ntn_GetSystemSelectionPlmn(firstRef, mcc, sizeof(mcc), mnc, sizeof(mnc));
    LE_TEST_OK(result == LE_OK, "taf_ntn_GetSystemSelectionPlmn - LE_OK");
    LE_TEST_OK(strcmp(mcc, "310") == 0 && strcmp(mnc, "410") == 0,
        "taf_ntn_GetSystemSelectionPlmn values - mcc=%s mnc=%s", mcc, mnc);
    LE_INFO("Retrieved PLMN from firstRef: mcc=%s, mnc=%s", mcc, mnc);

    uint64_t outBands[TAF_NTN_NTN_BAND_GROUP_NUM] = {0};
    size_t outBandsSize = TAF_NTN_NTN_BAND_GROUP_NUM;
    result = taf_ntn_GetSystemSelectionBands(firstRef, outBands, &outBandsSize);
    LE_TEST_OK(result == LE_OK && outBands[0] == 0x5,
        "taf_ntn_GetSystemSelectionBands - LE_OK");
    LE_INFO("Retrieved bands from firstRef: bandMask[0]=0x%" PRIx64, outBands[0]);

    uint64_t outEarfcns[TAF_NTN_MAX_SYSTEM_SELECTION_EARFCNS] = {0};
    size_t outEarfcnsSize = TAF_NTN_MAX_SYSTEM_SELECTION_EARFCNS;
    result = taf_ntn_GetSystemSelectionEarfcns(firstRef, outEarfcns, &outEarfcnsSize);
    LE_TEST_OK(result == LE_OK && outEarfcnsSize == 3,
        "taf_ntn_GetSystemSelectionEarfcns - LE_OK count=%" PRIuS, outEarfcnsSize);
    LE_INFO("Retrieved EARFCNs from firstRef: count=%zu, earfcns=[%" PRIu64 ", %" PRIu64 ", %" PRIu64 "]",
        outEarfcnsSize, outEarfcns[0], outEarfcns[1], outEarfcns[2]);

    // Test edge cases before deletion
    taf_ntn_SystemSelectionSpecifierRef_t nextRef = NULL;
    result = taf_ntn_GetNextSystemSelectionSpecifier(listRef, &nextRef);
    LE_TEST_OK(result == LE_NOT_FOUND,
        "taf_ntn_GetNextSystemSelectionSpecifier end - LE_NOT_FOUND");
    LE_INFO("GetNextSystemSelectionSpecifier result: nextRef=%p (expected NULL)", nextRef);

    // A specifier accepts PLMN only once.
    result = taf_ntn_AddSystemSelectionPlmn(specRef, "311", "480");
    LE_TEST_OK(result == LE_DUPLICATE, "taf_ntn_AddSystemSelectionPlmn duplicate - LE_DUPLICATE");
    LE_INFO("AddSystemSelectionPlmn duplicate check result: %d", result);

    // Write system selection list (should be done BEFORE enabling NTN)
    result = taf_ntn_WriteSystemSelectionList(listRef);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_WriteSystemSelectionList - result %d", result);
    LE_INFO("WriteSystemSelectionList result: %d", result);

    result = taf_ntn_DeleteSystemSelectionList(listRef);
    LE_TEST_OK(result == LE_OK, "taf_ntn_DeleteSystemSelectionList - LE_OK");
    LE_INFO("Deleted listRef: %p", listRef);

    // Test parameter validation
    result = taf_ntn_CreateSystemSelectionList(NULL, &listRef);
    LE_TEST_OK(result == LE_BAD_PARAMETER,
        "taf_ntn_CreateSystemSelectionList null session - LE_BAD_PARAMETER");
    LE_INFO("CreateSystemSelectionList NULL session check result: %d", result);

    result = taf_ntn_DeleteSystemSelectionList(NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER,
        "taf_ntn_DeleteSystemSelectionList null - LE_BAD_PARAMETER");
    LE_INFO("DeleteSystemSelectionList NULL list check result: %d", result);

    LE_TEST_INFO("--- NTN system selection complete ---");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the location fix builder, setters and update/response.
 */
//--------------------------------------------------------------------------------------------------
static void TestNtnLocationFix
(
    taf_ntn_SessionRef_t sessionRef ///< [IN] NTN session reference.
)
{
    LE_TEST_INFO("--- Testing NTN location fix (setup BEFORE enabling NTN) ---");

    // Test core functionality first
    taf_ntn_LocationFixRef_t locationRef = NULL;
    le_result_t result = taf_ntn_CreateLocationFix(sessionRef, &locationRef);
    LE_TEST_OK(result == LE_OK && locationRef != NULL, "taf_ntn_CreateLocationFix - LE_OK");
    LE_INFO("Created locationRef: %p", locationRef);

    result = taf_ntn_SetLocationCoordinates(locationRef, 37.42, -122.08, 30.0, 10);
    LE_TEST_OK(result == LE_OK, "taf_ntn_SetLocationCoordinates - LE_OK");
    LE_INFO("SetLocationCoordinates: lat=37.42, lon=-122.08, alt=30.0, uncertainty=10");

    result = taf_ntn_SetLocationVelocityEnu(locationRef, true, 1.0, 2.0, 3.0, true, 0.1, 0.2, 0.3);
    LE_TEST_OK(result == LE_OK, "taf_ntn_SetLocationVelocityEnu - LE_OK");
    LE_INFO("SetLocationVelocityEnu: east=1.0, north=2.0, up=3.0");

    result = taf_ntn_SetLocationHeading(locationRef, true, 90, true, 5);
    LE_TEST_OK(result == LE_OK, "taf_ntn_SetLocationHeading - LE_OK");
    LE_INFO("SetLocationHeading: heading=90, uncertainty=5");

    result = taf_ntn_SetLocationConfidence(locationRef, true, 68);
    LE_TEST_OK(result == LE_OK, "taf_ntn_SetLocationConfidence - LE_OK");
    LE_INFO("SetLocationConfidence: confidence=68");

    result = taf_ntn_UpdateLocationFix(locationRef);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_UpdateLocationFix - result %d", result);
    LE_INFO("UpdateLocationFix result: %d", result);

    result = taf_ntn_SendLocationFixResponse(sessionRef, TAF_NTN_LOCATION_STATUS_SUCCESS, 0);
    LE_TEST_OK(PaResultAcceptable(result), "taf_ntn_SendLocationFixResponse - result %d", result);
    LE_INFO("SendLocationFixResponse result: %d", result);

    result = taf_ntn_DeleteLocationFix(locationRef);
    LE_TEST_OK(result == LE_OK, "taf_ntn_DeleteLocationFix - LE_OK");
    LE_INFO("Deleted locationRef: %p", locationRef);

    // Test parameter validation and boundary conditions
    result = taf_ntn_SetLocationCoordinates(locationRef, 91.0, 0.0, 0.0, 10);
    LE_TEST_OK(result == LE_OUT_OF_RANGE,
        "taf_ntn_SetLocationCoordinates bad lat - LE_OUT_OF_RANGE");
    LE_INFO("SetLocationCoordinates invalid latitude check result: %d", result);

    result = taf_ntn_SetLocationHeading(locationRef, true, 361, false, 0);
    LE_TEST_OK(result == LE_OUT_OF_RANGE, "taf_ntn_SetLocationHeading bad - LE_OUT_OF_RANGE");
    LE_INFO("SetLocationHeading invalid heading check result: %d", result);

    result = taf_ntn_SetLocationConfidence(locationRef, true, 101);
    LE_TEST_OK(result == LE_OUT_OF_RANGE, "taf_ntn_SetLocationConfidence bad - LE_OUT_OF_RANGE");
    LE_INFO("SetLocationConfidence invalid confidence check result: %d", result);

    result = taf_ntn_DeleteLocationFix(NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_ntn_DeleteLocationFix null - LE_BAD_PARAMETER");
    LE_INFO("DeleteLocationFix NULL check result: %d", result);

    LE_TEST_INFO("--- NTN location fix complete ---");
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    uint8_t phoneId = DEFAULT_PHONE_ID;
    LE_TEST_INFO("Using phoneId %d", phoneId);

    LE_TEST_INFO("======== PHASE 1: Setup Event Listeners (BEFORE enabling NTN) ========");
    TestNtnHandlers();

    LE_TEST_INFO("======== PHASE 2: Create Session and Basic Checks ========");
    taf_ntn_SessionRef_t sessionRef = TestNtnSupportAndSession(phoneId);

    LE_TEST_INFO("======== PHASE 3: Setup System Selection (BEFORE enabling NTN) ========");
    TestNtnSystemSelection(sessionRef);

    LE_TEST_INFO("======== PHASE 4: Setup Location Fix (BEFORE enabling NTN) ========");
    TestNtnLocationFix(sessionRef);

    LE_TEST_INFO("======== PHASE 5: Test Operations (REQUIRES NTN ACTIVE) ========");
    TestNtnModeOperations(sessionRef);

    LE_TEST_INFO("======== PHASE 6: Test Data Operations (REQUIRES NTN ACTIVE) ========");
    TestNtnData(sessionRef);

    LE_TEST_INFO("======== PHASE 7: Session Cleanup ========");
    le_result_t result = taf_ntn_DeleteSession(sessionRef);
    LE_TEST_OK(result == LE_OK, "taf_ntn_DeleteSession - LE_OK");
    LE_INFO("Deleted sessionRef: %p, result=%d", sessionRef, result);

    result = taf_ntn_DeleteSession(NULL);
    LE_TEST_OK(result == LE_BAD_PARAMETER, "taf_ntn_DeleteSession null - LE_BAD_PARAMETER");
    LE_INFO("DeleteSession NULL check result: %d", result);

    LE_TEST_EXIT;
}
