/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_DID_DATA_ACCESS_COMP_H
#define TAF_DID_DATA_ACCESS_COMP_H

#include "legato.h"

#ifdef  __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------
/**
 * Initialize Data Access component.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DIDDataAccess_Init
(
);

//-------------------------------------------------------------------------------------------------
/**
 * Get event status from storage media.
 *
 * @return
 *  - The status of the event. If not exist, will return 0.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DIDDataAccess_ReadDID
(
    uint16_t did,     ///< [IN]
    uint8_t* value,   ///< [OUT]
    size_t *len      ///< [OUT]
);

//-------------------------------------------------------------------------------------------------
/**
 * Get event status from storage media by event name.
 *
 * @return
 *  - The status of the event. If not exist, will return 0.
 */
//-------------------------------------------------------------------------------------------------
LE_SHARED le_result_t taf_DIDDataAccess_WriteDID
(
    uint16_t did,    ///< [IN]
    uint8_t* value,       ///< [IN]
    size_t len       ///< [IN]
);

#ifdef  __cplusplus
}
#endif

#endif  // TAF_DID_DATA_ACCESS_COMP_H