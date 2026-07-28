/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_PROP_PMS_H
#define TAF_PROP_PMS_H

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------------------------
/**
 * The reference type for no-ship pms Mpss active-object
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_prop_pms_Mpss * taf_prop_pms_MpssRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Error codes for callback function
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    /* QMI service is unavailable */
    TAF_PROP_PMS_ERR_SVC_GONE = 0,

} taf_prop_pms_ErrCode_t;

//--------------------------------------------------------------------------------------------------
/**
 * The collection of bitset about Modem wakeup sources
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    MODEM_WS_INCOMING_SMS     = 0x0001,
    MODEM_WS_INCOMING_VCALL   = 0x0002,
    MODEM_WS_SIM_PROFILE_SWAP = 0x0004,
    MODEM_WS_NAS_SYS_INFO     = 0x0008,
} taf_prop_pms_ModemWakeupSource_t;

//--------------------------------------------------------------------------------------------------
/**
 * Parse the command-line arguments for options.
 */
//--------------------------------------------------------------------------------------------------
typedef void ( * taf_prop_pms_ErrCallback )
(
    taf_prop_pms_ErrCode_t errCode,
    void * errCbCtx
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the dependent QMI client and return the reference handle for other APIs.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  Bad parameters.
 *  - LE_DUPLICATE      Duplicated to call this function.
 *  - LE_FAULT          Failed to initialize the QMI client for remote QMI service.
 *
 * @b NOTE: DO NOT call taf_prop_pms_XXX APIs in callback function, otherwise deadlock will occur!
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_pms_Init
(
    taf_prop_pms_MpssRef_t * mpssRefPtr,  ///< [IN/OUT] Reference handle for active-object.
    taf_prop_pms_ErrCallback errCbFn,     ///< [IN] Error notification callback function.
    void * errCbCtx                         ///< [IN] Callback function context.
);

//--------------------------------------------------------------------------------------------------
/**
 * Deinitialize the dependent QMI client and release the passed reference pointer.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  Bad parameters.
 *  - LE_NOT_PERMITTED  Can not perform this function due to some bad preconditions.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_pms_Deinit
(
    taf_prop_pms_MpssRef_t * mpssRefPtr   ///< [IN] Pointer for the recorded reference handle
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the wakeup source filter(bitset) for specific MPSS services.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  Bad parameters.
 *  - LE_NOT_PERMITTED  Can not perform this function due to some bad preconditions.
 *  - LE_TIMEOUT        Not received the indication from the QMI service.
 *  - LE_FAULT          Failed to set the bitset to the remote service.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_pms_SetWsFilter
(
    taf_prop_pms_MpssRef_t mpssRef,         ///< [IN] Recorded reference from _Init API.
    taf_prop_pms_ModemWakeupSource_t bitset ///< [IN] Filter bitset for special indications.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the wakeup source filter(bitset) from remote QMI PDC service.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  Bad parameters.
 *  - LE_NOT_PERMITTED  Can not perform this function due to some bad preconditions.
 *  - LE_TIMEOUT        Not received the indication from the QMI service.
 *  - LE_FAULT          Failt to get the bitset from the remote service.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_pms_GetWsFilter
(
    taf_prop_pms_MpssRef_t mpssRef,          ///< [IN]  Recorded reference from _Init API.
    taf_prop_pms_ModemWakeupSource_t *bitset ///< [OUT] Configured filter bitset in remote sevice
);

//--------------------------------------------------------------------------------------------------
/**
 * Enable all wakeup source indications for MPSS.
 *
 * @return
 *  - LE_OK             Function succeeded.
 *  - LE_BAD_PARAMETER  Bad parameters.
 *  - LE_NOT_PERMITTED  Can not perform this function due to some bad preconditions.
 *  - LE_TIMEOUT        Not received the indication from the QMI service.
 *  - LE_FAULT          Failed to enable all indications for MPSS side.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_prop_pms_EnableAllWs
(
    taf_prop_pms_MpssRef_t mpssRef
);

#ifdef __cplusplus
}
#endif

#endif // TAF_PROP_PMS_H
