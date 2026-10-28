/*
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*
 * Tests of the taf_hmsEcall API (eCall recovery records in tafHMSvc).
 *
 * The restart of the eCall service itself cannot be simulated in a single process, so the
 * recovery path is tested by getting the record again with taf_hmsEcall_GetRecord(), which is
 * exactly what the eCall service does after it restarts.
 */

#include "legato.h"
#include "interfaces.h"

#define PHONE_ID_1 1
#define PHONE_ID_2 2

//--------------------------------------------------------------------------------------------------
/**
 * A new record has no eCall type; SetType/GetType round trip for every valid type.
 */
//--------------------------------------------------------------------------------------------------
static void TestSetGetType(void)
{
    LE_TEST_INFO("===== TestSetGetType =====");

    taf_hmsEcall_RecordRef_t ref = taf_hmsEcall_GetRecord(PHONE_ID_1);
    LE_TEST_OK(ref != NULL, "taf_hmsEcall_GetRecord - record created");
    if (ref == NULL)
    {
        return;
    }

    taf_hmsEcall_Type_t type = TAF_HMSECALL_TYPE_UNKNOWN;
    LE_TEST_OK(taf_hmsEcall_GetType(ref, &type) == LE_NOT_FOUND,
        "taf_hmsEcall_GetType on new record - LE_NOT_FOUND");

    const taf_hmsEcall_Type_t types[] =
    {
        TAF_HMSECALL_TYPE_TEST,
        TAF_HMSECALL_TYPE_AUTO,
        TAF_HMSECALL_TYPE_MANUAL,
        TAF_HMSECALL_TYPE_PRIVATE
    };

    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++)
    {
        type = TAF_HMSECALL_TYPE_UNKNOWN;
        LE_TEST_OK(taf_hmsEcall_SetType(ref, types[i]) == LE_OK,
            "taf_hmsEcall_SetType(%d) - LE_OK", types[i]);
        LE_TEST_OK(taf_hmsEcall_GetType(ref, &type) == LE_OK && type == types[i],
            "taf_hmsEcall_GetType - %d", types[i]);
    }

    LE_TEST_OK(taf_hmsEcall_Delete(ref) == LE_OK, "taf_hmsEcall_Delete - LE_OK");
}

//--------------------------------------------------------------------------------------------------
/**
 * Getting the record of a phone again returns the same record with the stored type. This is the
 * recovery path after the eCall service restarts.
 */
//--------------------------------------------------------------------------------------------------
static void TestGetRecordAgain(void)
{
    LE_TEST_INFO("===== TestGetRecordAgain =====");

    taf_hmsEcall_RecordRef_t ref = taf_hmsEcall_GetRecord(PHONE_ID_1);
    LE_TEST_OK(ref != NULL, "taf_hmsEcall_GetRecord - record created");
    if (ref == NULL)
    {
        return;
    }
    LE_TEST_OK(taf_hmsEcall_SetType(ref, TAF_HMSECALL_TYPE_MANUAL) == LE_OK,
        "taf_hmsEcall_SetType(MANUAL) - LE_OK");

    // The reference value is lost on a restart; get the record by phone ID.
    taf_hmsEcall_RecordRef_t ref2 = taf_hmsEcall_GetRecord(PHONE_ID_1);
    LE_TEST_OK(ref2 == ref, "taf_hmsEcall_GetRecord again - same reference");

    taf_hmsEcall_Type_t type = TAF_HMSECALL_TYPE_UNKNOWN;
    LE_TEST_OK(taf_hmsEcall_GetType(ref2, &type) == LE_OK && type == TAF_HMSECALL_TYPE_MANUAL,
        "taf_hmsEcall_GetType - MANUAL still stored");

    taf_hmsEcall_Delete(ref);
}

//--------------------------------------------------------------------------------------------------
/**
 * Records of different phones are independent.
 */
//--------------------------------------------------------------------------------------------------
static void TestTwoPhones(void)
{
    LE_TEST_INFO("===== TestTwoPhones =====");

    taf_hmsEcall_RecordRef_t ref1 = taf_hmsEcall_GetRecord(PHONE_ID_1);
    taf_hmsEcall_RecordRef_t ref2 = taf_hmsEcall_GetRecord(PHONE_ID_2);
    LE_TEST_OK(ref1 != NULL && ref2 != NULL && ref1 != ref2,
        "taf_hmsEcall_GetRecord - different references for phone 1 and 2");
    if (ref1 == NULL || ref2 == NULL)
    {
        if (ref1 != NULL)
        {
            taf_hmsEcall_Delete(ref1);
        }

        if (ref2 != NULL)
        {
            taf_hmsEcall_Delete(ref2);
        }

        return;
    }

    taf_hmsEcall_Type_t type = TAF_HMSECALL_TYPE_UNKNOWN;

    LE_TEST_OK(taf_hmsEcall_SetType(ref1, TAF_HMSECALL_TYPE_TEST) == LE_OK,
        "taf_hmsEcall_SetType(phone 1, TEST) - LE_OK");
    LE_TEST_OK(taf_hmsEcall_GetType(ref2, &type) == LE_NOT_FOUND,
        "taf_hmsEcall_GetType(phone 2) - LE_NOT_FOUND");

    LE_TEST_OK(taf_hmsEcall_SetType(ref2, TAF_HMSECALL_TYPE_PRIVATE) == LE_OK,
        "taf_hmsEcall_SetType(phone 2, PRIVATE) - LE_OK");
    LE_TEST_OK(taf_hmsEcall_GetType(ref1, &type) == LE_OK && type == TAF_HMSECALL_TYPE_TEST,
        "taf_hmsEcall_GetType(phone 1) - TEST");
    LE_TEST_OK(taf_hmsEcall_GetType(ref2, &type) == LE_OK && type == TAF_HMSECALL_TYPE_PRIVATE,
        "taf_hmsEcall_GetType(phone 2) - PRIVATE");

    LE_TEST_OK(taf_hmsEcall_Delete(ref1) == LE_OK, "taf_hmsEcall_Delete(phone 1) - LE_OK");
    LE_TEST_OK(taf_hmsEcall_GetType(ref2, &type) == LE_OK && type == TAF_HMSECALL_TYPE_PRIVATE,
        "taf_hmsEcall_GetType(phone 2) - still PRIVATE after deleting phone 1");
    taf_hmsEcall_Delete(ref2);
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete releases the record: the reference becomes invalid and the next GetRecord gives a new,
 * empty record.
 */
//--------------------------------------------------------------------------------------------------
static void TestDelete(void)
{
    LE_TEST_INFO("===== TestDelete =====");

    taf_hmsEcall_RecordRef_t ref = taf_hmsEcall_GetRecord(PHONE_ID_1);
    LE_TEST_OK(ref != NULL, "taf_hmsEcall_GetRecord - record created");
    if (ref == NULL)
    {
        return;
    }
    LE_TEST_OK(taf_hmsEcall_SetType(ref, TAF_HMSECALL_TYPE_AUTO) == LE_OK,
        "taf_hmsEcall_SetType(AUTO) - LE_OK");

    LE_TEST_OK(taf_hmsEcall_Delete(ref) == LE_OK, "taf_hmsEcall_Delete - LE_OK");

    taf_hmsEcall_Type_t type = TAF_HMSECALL_TYPE_UNKNOWN;

    // A new eCall starts from a clean record.
    taf_hmsEcall_RecordRef_t ref2 = taf_hmsEcall_GetRecord(PHONE_ID_1);
    LE_TEST_OK(ref2 != NULL, "taf_hmsEcall_GetRecord after delete - new record");
    LE_TEST_OK(ref2 != NULL && taf_hmsEcall_GetType(ref2, &type) == LE_NOT_FOUND,
        "taf_hmsEcall_GetType on new record - LE_NOT_FOUND (no stale type)");

    taf_hmsEcall_Delete(ref2);
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);
    LE_INFO("---------- TelAF Health Monitor eCall Recovery Tests Start ----------");

    TestSetGetType();
    TestGetRecordAgain();
    TestTwoPhones();
    TestDelete();

    LE_INFO("---------- All Tests Complete ----------");
    LE_TEST_EXIT;
}
