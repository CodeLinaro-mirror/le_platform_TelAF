/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Process-wide NTN handles reused across menu operations. The session reference is created once at
 * startup (its value is printed) and passed to every session-based API; the location fix and system
 * selection list references are created on demand by their respective menu actions.
 */
//--------------------------------------------------------------------------------------------------
static taf_ntn_SessionRef_t            g_sessionRef = NULL;
static taf_ntn_LocationFixRef_t        g_locationRef = NULL;
static taf_ntn_SystemSelectionListRef_t g_listRef = NULL;
static uint8_t                         g_phoneId = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Reads a line of input from stdin into buf, stripping the trailing newline.
 *
 * @return 0 on success, -1 on EOF/error.
 */
//--------------------------------------------------------------------------------------------------
static int ReadLine
(
    char* buf,     ///< [OUT] Output buffer.
    size_t bufSize ///< [IN] Output buffer size.
)
{
    if (fgets(buf, bufSize, stdin) == NULL)
    {
        return -1;
    }
    buf[strcspn(buf, "\r\n")] = '\0';
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Prompts with a label and reads an unsigned 64-bit value.
 */
//--------------------------------------------------------------------------------------------------
static uint64_t PromptU64
(
    const char* label, ///< [IN] Prompt label.
    int base           ///< [IN] Numeric base (10 or 16).
)
{
    char line[64] = {0};
    printf("%s", label);
    fflush(stdout);
    if (ReadLine(line, sizeof(line)) != 0 || line[0] == '\0')
    {
        return 0;
    }
    return (uint64_t)strtoull(line, NULL, base);
}

//--------------------------------------------------------------------------------------------------
/**
 * Prompts with a label and reads a boolean (1/true/yes -> true).
 */
//--------------------------------------------------------------------------------------------------
static bool PromptBool
(
    const char* label ///< [IN] Prompt label.
)
{
    char line[16] = {0};
    printf("%s (0/1): ", label);
    fflush(stdout);
    if (ReadLine(line, sizeof(line)) != 0)
    {
        return false;
    }
    return (line[0] == '1' || line[0] == 'y' || line[0] == 'Y' || line[0] == 't' || line[0] == 'T');
}

//--------------------------------------------------------------------------------------------------
/**
 * Prompts with a label and reads a double value.
 */
//--------------------------------------------------------------------------------------------------
static double PromptDouble
(
    const char* label ///< [IN] Prompt label.
)
{
    char line[64] = {0};
    printf("%s", label);
    fflush(stdout);
    if (ReadLine(line, sizeof(line)) != 0 || line[0] == '\0')
    {
        return 0.0;
    }
    return strtod(line, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Parses a space-separated list of numbers into a uint64_t array.
 *
 * @return Number of values parsed.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t ParseU64List
(
    const char* line,  ///< [IN] Space-separated values.
    int base,          ///< [IN] Numeric base (10 or 16).
    uint64_t* out,     ///< [OUT] Output array.
    uint32_t maxCount  ///< [IN] Maximum number of values.
)
{
    uint32_t count = 0;
    const char* p = line;
    while (count < maxCount)
    {
        while (*p == ' ' || *p == '\t')
        {
            p++;
        }
        if (*p == '\0')
        {
            break;
        }
        char* end = NULL;
        uint64_t val = (uint64_t)strtoull(p, &end, base);
        if (end == p)
        {
            break;
        }
        out[count++] = val;
        p = end;
    }
    return count;
}

//--------------------------------------------------------------------------------------------------
/**
 * Converts an even-length hex string into bytes.
 *
 * @return Number of bytes written to buf.
 */
//--------------------------------------------------------------------------------------------------
static size_t HexToBytes
(
    const char* hex, ///< [IN] Null-terminated hex string.
    uint8_t* buf,    ///< [OUT] Output buffer.
    size_t bufSize   ///< [IN] Output buffer size.
)
{
    size_t hexLen = strlen(hex);
    size_t count = 0;
    for (size_t i = 0; (i + 1) < hexLen && count < bufSize; i += 2)
    {
        char byteStr[3] = { hex[i], hex[i + 1], '\0' };
        buf[count++] = (uint8_t)strtoul(byteStr, NULL, 16);
    }
    return count;
}

//--------------------------------------------------------------------------------------------------
// NTN event handlers - registered once at startup so that any event produced by any subsequent
// operation is printed live while the menu is being driven.
//--------------------------------------------------------------------------------------------------

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
    LE_INFO("[EVENT] NTN state change : ref=%p state=%d", sessionRef, state);
    printf("\n[EVENT] NTN state change : state=%d\n", state);
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
    LE_INFO("[EVENT] NTN capabilities changed : ref=%p", sessionRef);
    printf("\n[EVENT] NTN capabilities changed\n");
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
    LE_INFO("[EVENT] NTN data ack : result=%d transactionId=%" PRIu64, result, transactionId);
    printf("\n[EVENT] NTN data ack : result=%d transactionId=%" PRIu64 "\n", result, transactionId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for incoming NTN data. Reads and releases the packet.
 */
//--------------------------------------------------------------------------------------------------
static void IncomingDataHandler
(
    taf_ntn_SessionRef_t sessionRef,  ///< [IN] NTN session reference.
    taf_ntn_DataPacketRef_t packetRef,///< [IN] Incoming data packet reference.
    void* contextPtr                  ///< [IN] Handler context.
)
{
    uint8_t data[TAF_NTN_MAX_DATA_BYTES];
    size_t dataSize = sizeof(data);
    le_result_t result = taf_ntn_ReadDataPacket(packetRef, data, &dataSize);
    LE_INFO("[EVENT] NTN incoming data : %" PRIuS " bytes (read result %d)", dataSize, result);
    printf("\n[EVENT] NTN incoming data : %" PRIuS " bytes\n", dataSize);
    taf_ntn_DeleteDataPacket(packetRef);
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
    LE_INFO("[EVENT] NTN cellular coverage available : %d", available);
    printf("\n[EVENT] NTN cellular coverage available : %d\n", available);
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
    LE_INFO("[EVENT] NTN location fix requested : reason=%d", reason);
    printf("\n[EVENT] NTN location fix requested : reason=%d\n", reason);
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
    LE_INFO("[EVENT] NTN band update : %" PRIu64, band);
    printf("\n[EVENT] NTN band update : %" PRIu64 "\n", band);
}

//--------------------------------------------------------------------------------------------------
/**
 * Registers every NTN event handler on the calling thread.
 */
//--------------------------------------------------------------------------------------------------
static void AddAllHandlers
(
    void
)
{
    taf_ntn_AddStateChangeHandler(StateChangeHandler, NULL);
    taf_ntn_AddCapabilitiesChangeHandler(CapabilitiesChangeHandler, NULL);
    taf_ntn_AddDataAckHandler(DataAckHandler, NULL);
    taf_ntn_AddIncomingDataHandler(IncomingDataHandler, NULL);
    taf_ntn_AddCellularCoverageHandler(CellularCoverageHandler, NULL);
    taf_ntn_AddLocationFixRequestHandler(LocationFixRequestHandler, NULL);
    taf_ntn_AddBandUpdateHandler(BandUpdateHandler, NULL);
    LE_INFO("All NTN event handlers registered.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Prints the interactive menu.
 */
//--------------------------------------------------------------------------------------------------
static void PrintMenu
(
    void
)
{
    printf(
        "\n===================== tafNtnIntTest =====================\n"
        "  phoneId=%d  sessionRef=%p\n"
        "  1  - IsSupported (re-query, prompts phoneId)\n"
        "  2  - GetState + GetCapabilityMaxDataSize\n"
        "  3  - SetNtnMode (prompts enable, emergency)\n"
        "  4  - SendData (prompts hex payload, emergency)\n"
        "  5  - AbortData\n"
        "  6  - SetCellularScan (prompts enable)\n"
        "  7  - Location fix (prompts coords/velocity/heading/confidence)\n"
        "  8  - System selection (prompts PLMN/bands/EARFCNs)\n"
        "  9  - Print current references\n"
        "  0  - Exit\n"
        "=========================================================\n"
        "NTN > ",
        g_phoneId, g_sessionRef);
    fflush(stdout);
}

//--------------------------------------------------------------------------------------------------
/**
 * Menu action: query IsSupported for a prompted phone ID.
 */
//--------------------------------------------------------------------------------------------------
static void DoIsSupported
(
    void
)
{
    bool supported = false;
    le_result_t result = taf_ntn_IsSupported(&supported);
    printf("taf_ntn_IsSupported() : result=%s, supported=%d\n",
        LE_RESULT_TXT(result), supported);
}

//--------------------------------------------------------------------------------------------------
/**
 * Menu action: get NTN state and maximum data size.
 */
//--------------------------------------------------------------------------------------------------
static void DoGetState
(
    void
)
{
    taf_ntn_State_t state = TAF_NTN_STATE_DISABLED;
    le_result_t result = taf_ntn_GetState(g_sessionRef, &state);
    printf("taf_ntn_GetState : result=%s, state=%d\n", LE_RESULT_TXT(result), state);

    uint64_t maxDataSize = 0;
    result = taf_ntn_GetCapabilityMaxDataSize(g_sessionRef, &maxDataSize);
    printf("taf_ntn_GetCapabilityMaxDataSize : result=%s, maxDataSize=%" PRIu64 "\n",
        LE_RESULT_TXT(result), maxDataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Menu action: set NTN mode.
 */
//--------------------------------------------------------------------------------------------------
static void DoSetNtnMode
(
    void
)
{
    bool enable = PromptBool("Enable NTN");
    bool isEmergency = PromptBool("Emergency");
    le_result_t result = taf_ntn_SetNtnMode(g_sessionRef, enable, isEmergency);
    printf("taf_ntn_SetNtnMode(enable=%d, emergency=%d) : result=%s\n",
        enable, isEmergency, LE_RESULT_TXT(result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Menu action: send data.
 */
//--------------------------------------------------------------------------------------------------
static void DoSendData
(
    void
)
{
    char hex[2 * TAF_NTN_MAX_DATA_BYTES + 2] = {0};
    printf("Enter data (hex, e.g. 01020304): ");
    fflush(stdout);
    if (ReadLine(hex, sizeof(hex)) != 0 || hex[0] == '\0')
    {
        printf("No data entered.\n");
        return;
    }
    bool isEmergency = PromptBool("Emergency");

    uint8_t data[TAF_NTN_MAX_DATA_BYTES];
    size_t dataLen = HexToBytes(hex, data, sizeof(data));
    uint64_t transactionId = 0;
    le_result_t result = taf_ntn_SendData(g_sessionRef, data, dataLen, isEmergency, &transactionId);
    printf("taf_ntn_SendData(%" PRIuS " bytes, emergency=%d) : result=%s, transactionId=%" PRIu64
        "\n", dataLen, isEmergency, LE_RESULT_TXT(result), transactionId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Menu action: abort queued data.
 */
//--------------------------------------------------------------------------------------------------
static void DoAbortData
(
    void
)
{
    le_result_t result = taf_ntn_AbortData(g_sessionRef);
    printf("taf_ntn_AbortData : result=%s\n", LE_RESULT_TXT(result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Menu action: enable/disable terrestrial cellular scan.
 */
//--------------------------------------------------------------------------------------------------
static void DoSetCellularScan
(
    void
)
{
    bool enable = PromptBool("Enable cellular scan");
    le_result_t result = taf_ntn_SetCellularScan(g_sessionRef, enable);
    printf("taf_ntn_SetCellularScan(enable=%d) : result=%s\n", enable, LE_RESULT_TXT(result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Menu action: build and update a location fix from user-supplied values, then send a response.
 */
//--------------------------------------------------------------------------------------------------
static void DoLocationFix
(
    void
)
{
    le_result_t result;

    if (g_locationRef != NULL)
    {
        taf_ntn_DeleteLocationFix(g_locationRef);
        g_locationRef = NULL;
    }

    result = taf_ntn_CreateLocationFix(g_sessionRef, &g_locationRef);
    printf("taf_ntn_CreateLocationFix : result=%s, ref=%p\n", LE_RESULT_TXT(result), g_locationRef);
    if (result != LE_OK || g_locationRef == NULL)
    {
        return;
    }

    double lat = PromptDouble("Latitude (deg): ");
    double lon = PromptDouble("Longitude (deg): ");
    double alt = PromptDouble("Altitude (m): ");
    uint32_t horizUnc = (uint32_t)PromptU64("Horizontal uncertainty (m): ", 10);
    result = taf_ntn_SetLocationCoordinates(g_locationRef, lat, lon, alt, horizUnc);
    printf("taf_ntn_SetLocationCoordinates : result=%s\n", LE_RESULT_TXT(result));

    bool velValid = PromptBool("Velocity valid");
    if (velValid)
    {
        double east = PromptDouble("East velocity (m/s): ");
        double north = PromptDouble("North velocity (m/s): ");
        double up = PromptDouble("Up velocity (m/s): ");
        bool uncValid = PromptBool("Velocity uncertainty valid");
        double eastUnc = 0.0, northUnc = 0.0, upUnc = 0.0;
        if (uncValid)
        {
            eastUnc = PromptDouble("East uncertainty (m/s): ");
            northUnc = PromptDouble("North uncertainty (m/s): ");
            upUnc = PromptDouble("Up uncertainty (m/s): ");
        }
        result = taf_ntn_SetLocationVelocityEnu(g_locationRef, velValid, east, north, up,
            uncValid, eastUnc, northUnc, upUnc);
        printf("taf_ntn_SetLocationVelocityEnu : result=%s\n", LE_RESULT_TXT(result));
    }

    bool headingValid = PromptBool("Heading valid");
    if (headingValid)
    {
        uint32_t heading = (uint32_t)PromptU64("Heading (deg): ", 10);
        bool uncValid = PromptBool("Heading uncertainty valid");
        uint32_t headingUnc = uncValid ? (uint32_t)PromptU64("Heading uncertainty (deg): ", 10) : 0;
        result = taf_ntn_SetLocationHeading(g_locationRef, headingValid, heading, uncValid,
            headingUnc);
        printf("taf_ntn_SetLocationHeading : result=%s\n", LE_RESULT_TXT(result));
    }

    bool confValid = PromptBool("Confidence valid");
    if (confValid)
    {
        uint32_t confidence = (uint32_t)PromptU64("Confidence (%): ", 10);
        result = taf_ntn_SetLocationConfidence(g_locationRef, confValid, confidence);
        printf("taf_ntn_SetLocationConfidence : result=%s\n", LE_RESULT_TXT(result));
    }

    result = taf_ntn_UpdateLocationFix(g_locationRef);
    printf("taf_ntn_UpdateLocationFix : result=%s\n", LE_RESULT_TXT(result));

    if (PromptBool("Send location fix response"))
    {
        uint32_t status = (uint32_t)PromptU64(
            "Status (0=SUCCESS 1=INVALID_ARG 2=INTERNAL_ERR 3=NOT_SUPPORTED 4=RETRY 5=FAILED): ",
            10);
        uint64_t waitMs = PromptU64("Retry wait time (ms): ", 10);
        result = taf_ntn_SendLocationFixResponse(g_sessionRef, (taf_ntn_LocationStatus_t)status,
            waitMs);
        printf("taf_ntn_SendLocationFixResponse : result=%s\n", LE_RESULT_TXT(result));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Menu action: build a system selection list from user input, write it, then read it back.
 */
//--------------------------------------------------------------------------------------------------
static void DoSystemSelection
(
    void
)
{
    le_result_t result;

    if (g_listRef != NULL)
    {
        taf_ntn_DeleteSystemSelectionList(g_listRef);
        g_listRef = NULL;
    }

    result = taf_ntn_CreateSystemSelectionList(g_sessionRef, &g_listRef);
    printf("taf_ntn_CreateSystemSelectionList : result=%s, ref=%p\n", LE_RESULT_TXT(result),
        g_listRef);
    if (result != LE_OK || g_listRef == NULL)
    {
        return;
    }

    uint32_t specCount = (uint32_t)PromptU64("Number of specifiers: ", 10);
    for (uint32_t i = 0; i < specCount; i++)
    {
        printf("--- Specifier %u ---\n", i + 1);
        taf_ntn_SystemSelectionSpecifierRef_t specRef = NULL;
        result = taf_ntn_AddSystemSelectionSpecifier(g_listRef, &specRef);
        printf("taf_ntn_AddSystemSelectionSpecifier : result=%s\n", LE_RESULT_TXT(result));
        if (result != LE_OK)
        {
            continue;
        }

        char mcc[TAF_NTN_MCC_BYTES] = {0};
        char mnc[TAF_NTN_MNC_BYTES] = {0};
        char inbuf[32] = {0};
        printf("Enter MCC (blank to skip PLMN): ");
        fflush(stdout);
        ReadLine(inbuf, sizeof(inbuf));
        if (inbuf[0] != '\0')
        {
            le_utf8_Copy(mcc, inbuf, sizeof(mcc), NULL);
            printf("Enter MNC: ");
            fflush(stdout);
            ReadLine(inbuf, sizeof(inbuf));
            le_utf8_Copy(mnc, inbuf, sizeof(mnc), NULL);
            result = taf_ntn_AddSystemSelectionPlmn(specRef, mcc, mnc);
            printf("taf_ntn_AddSystemSelectionPlmn(%s/%s) : result=%s\n", mcc, mnc,
                LE_RESULT_TXT(result));
        }

        char bandLine[128] = {0};
        printf("Enter band bitmask words (hex, up to %d, space-separated, blank to skip): ",
            TAF_NTN_NTN_BAND_GROUP_NUM);
        fflush(stdout);
        ReadLine(bandLine, sizeof(bandLine));
        if (bandLine[0] != '\0')
        {
            uint64_t bandMask[TAF_NTN_NTN_BAND_GROUP_NUM] = {0};
            ParseU64List(bandLine, 16, bandMask, TAF_NTN_NTN_BAND_GROUP_NUM);
            result = taf_ntn_AddSystemSelectionBands(specRef, bandMask,
                TAF_NTN_NTN_BAND_GROUP_NUM);
            printf("taf_ntn_AddSystemSelectionBands : result=%s\n", LE_RESULT_TXT(result));
        }

        char earfcnLine[256] = {0};
        printf("Enter EARFCNs (decimal, up to %d, space-separated, blank to skip): ",
            TAF_NTN_MAX_SYSTEM_SELECTION_EARFCNS);
        fflush(stdout);
        ReadLine(earfcnLine, sizeof(earfcnLine));
        if (earfcnLine[0] != '\0')
        {
            uint64_t earfcns[TAF_NTN_MAX_SYSTEM_SELECTION_EARFCNS] = {0};
            uint32_t n = ParseU64List(earfcnLine, 10, earfcns,
                TAF_NTN_MAX_SYSTEM_SELECTION_EARFCNS);
            result = taf_ntn_AddSystemSelectionEarfcns(specRef, earfcns, n);
            printf("taf_ntn_AddSystemSelectionEarfcns(%u) : result=%s\n", n, LE_RESULT_TXT(result));
        }
    }

    result = taf_ntn_WriteSystemSelectionList(g_listRef);
    printf("taf_ntn_WriteSystemSelectionList : result=%s\n", LE_RESULT_TXT(result));

    // Read back the specifiers that were added.
    taf_ntn_SystemSelectionSpecifierRef_t specRef = NULL;
    result = taf_ntn_GetFirstSystemSelectionSpecifier(g_listRef, &specRef);
    while (result == LE_OK && specRef != NULL)
    {
        char mcc[TAF_NTN_MCC_BYTES] = {0};
        char mnc[TAF_NTN_MNC_BYTES] = {0};
        if (taf_ntn_GetSystemSelectionPlmn(specRef, mcc, sizeof(mcc), mnc, sizeof(mnc)) == LE_OK)
        {
            printf("  read-back PLMN mcc=%s mnc=%s\n", mcc, mnc);
        }

        uint64_t bandMask[TAF_NTN_NTN_BAND_GROUP_NUM] = {0};
        size_t bandCount = TAF_NTN_NTN_BAND_GROUP_NUM;
        if (taf_ntn_GetSystemSelectionBands(specRef, bandMask, &bandCount) == LE_OK)
        {
            printf("  read-back bandMask[0]=0x%" PRIx64 "\n", bandMask[0]);
        }

        uint64_t earfcns[TAF_NTN_MAX_SYSTEM_SELECTION_EARFCNS] = {0};
        size_t earfcnCount = TAF_NTN_MAX_SYSTEM_SELECTION_EARFCNS;
        if (taf_ntn_GetSystemSelectionEarfcns(specRef, earfcns, &earfcnCount) == LE_OK)
        {
            printf("  read-back EARFCN count=%" PRIuS "\n", earfcnCount);
        }

        result = taf_ntn_GetNextSystemSelectionSpecifier(g_listRef, &specRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Menu action: print the currently held references.
 */
//--------------------------------------------------------------------------------------------------
static void DoPrintRefs
(
    void
)
{
    printf("phoneId     = %d\n", g_phoneId);
    printf("sessionRef  = %p\n", g_sessionRef);
    printf("locationRef = %p\n", g_locationRef);
    printf("listRef     = %p\n", g_listRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Menu driver state. Input is serviced by an fd monitor on the event-loop thread (see
 * COMPONENT_INIT) so that the same thread both reads stdin and dispatches NTN indications to the
 * registered handlers. A blocking read loop on a separate thread would starve that thread's event
 * loop and the handlers would never fire.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    MENU_STATE_PHONE_ID = 0, ///< Waiting for the initial phoneId line.
    MENU_STATE_MENU          ///< Waiting for a menu selection.
} MenuState_t;

static MenuState_t g_menuState = MENU_STATE_PHONE_ID;

//--------------------------------------------------------------------------------------------------
/**
 * Handles the initial phoneId line: queries support, registers nothing new (handlers are already
 * registered in COMPONENT_INIT), creates the session, then shows the menu.
 */
//--------------------------------------------------------------------------------------------------
static void HandlePhoneIdInput
(
    const char* line ///< [IN] Raw phoneId line.
)
{
    g_phoneId = (uint8_t)strtoull(line, NULL, 10);

    bool supported = false;
    le_result_t result = taf_ntn_IsSupported(&supported);
    printf("taf_ntn_IsSupported() : result=%s, supported=%d\n",
        LE_RESULT_TXT(result), supported);

    result = taf_ntn_CreateSession(g_phoneId, &g_sessionRef);
    printf("taf_ntn_CreateSession(phoneId=%d) : result=%s\n", g_phoneId, LE_RESULT_TXT(result));
    if (result != LE_OK || g_sessionRef == NULL)
    {
        printf("Failed to create session; exiting.\n");
        exit(EXIT_FAILURE);
    }
    // Print the session reference the operator reuses for all subsequent APIs.
    printf(">>> Session created. sessionRef = %p (reused by all operations below) <<<\n",
        g_sessionRef);
    LE_INFO("Session created: ref=%p phoneId=%d", g_sessionRef, g_phoneId);

    g_menuState = MENU_STATE_MENU;
    PrintMenu();
}

//--------------------------------------------------------------------------------------------------
/**
 * Handles a menu selection line.
 */
//--------------------------------------------------------------------------------------------------
static void HandleMenuInput
(
    const char* line ///< [IN] Raw menu selection line.
)
{
    switch (line[0])
    {
        case '1': DoIsSupported(); break;
        case '2': DoGetState(); break;
        case '3': DoSetNtnMode(); break;
        case '4': DoSendData(); break;
        case '5': DoAbortData(); break;
        case '6': DoSetCellularScan(); break;
        case '7': DoLocationFix(); break;
        case '8': DoSystemSelection(); break;
        case '9': DoPrintRefs(); break;
        case '0':
            if (g_locationRef != NULL)
            {
                taf_ntn_DeleteLocationFix(g_locationRef);
            }
            if (g_listRef != NULL)
            {
                taf_ntn_DeleteSystemSelectionList(g_listRef);
            }
            taf_ntn_DeleteSession(g_sessionRef);
            printf("Session deleted. Exiting.\n");
            exit(EXIT_SUCCESS);
        case '\0': break; // empty line, just re-print menu
        default:
            printf("Unknown option.\n");
            break;
    }
    PrintMenu();
}

//--------------------------------------------------------------------------------------------------
/**
 * stdin fd-monitor handler. Runs on the event-loop thread, so NTN indication handlers registered on
 * this same thread continue to dispatch between menu selections.
 */
//--------------------------------------------------------------------------------------------------
static void StdinHandler
(
    int fd,       ///< [IN] File descriptor (stdin).
    short events  ///< [IN] Event flags.
)
{
    LE_UNUSED(fd);

    if (events & (POLLHUP | POLLRDHUP | POLLERR))
    {
        printf("stdin closed. Exiting.\n");
        exit(EXIT_SUCCESS);
    }

    char line[64] = {0};
    if (ReadLine(line, sizeof(line)) != 0)
    {
        printf("stdin EOF. Exiting.\n");
        exit(EXIT_SUCCESS);
    }

    if (g_menuState == MENU_STATE_PHONE_ID)
    {
        HandlePhoneIdInput(line);
    }
    else
    {
        HandleMenuInput(line);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization. Connects to the service and registers all NTN handlers on this (main)
 * thread, then monitors stdin via an fd monitor. Because the handlers are registered on the same
 * thread that runs the event loop, NTN indications are dispatched to them while the menu is idle.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    taf_ntn_ConnectService();

    // Register handlers on the event-loop (main) thread so indications dispatch here.
    AddAllHandlers();

    // Drive stdin from the event loop rather than a blocking read loop, so the same thread can both
    // read menu input and dispatch NTN indications to the handlers registered above.
    le_fdMonitor_Create("NtnStdin", fileno(stdin), StdinHandler, POLLIN);

    printf("Enter phoneId to use for NTN: ");
    fflush(stdout);
}
