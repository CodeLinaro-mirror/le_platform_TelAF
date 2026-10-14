/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include "tafDIDDataAccessComp.h"
#include "tafIOFactory.hpp"
#include "tafDIDDataAccessCompImpl.hpp"

using namespace taf::dataAccess;

//--------------------------------------------------------------------------------------------------
/**
 * Component once initializer.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT_ONCE
{
    LE_INFO("tafDIDDataAccessComp Init...");
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization function
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_DEBUG("DIDDataAccess component initializing");
}

//-------------------------------------------------------------------------------------------------
/**
 * Initialize Data Access component.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DIDDataAccess_Init
(
)
{
    auto &didHandler = DidDataHandler::GetInstance();
//Init function creates DB file. taf_DIDDataAccess_Init is called by tafDidStoresvc with telaf user.
    didHandler.Init();

    return didHandler.Load();
}

//-------------------------------------------------------------------------------------------------
/**
 * Set event status and save it in storage.
 *
 * @return
 *  - LE_OK             Funtion success.
 *  - LE_BAD_PARAMETER  Invalid parameter.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DIDDataAccess_WriteDID
(
    uint16_t did,    ///< [IN]
    uint8_t* value,       ///< [IN]
    size_t len       ///< [IN]
)
{
    auto &didDataHandler = DidDataHandler::GetInstance();

    return didDataHandler.WriteDID(did, static_cast<const uint8_t *>(value), len);
}

//-------------------------------------------------------------------------------------------------
/**
 * Get event status from storage media.
 *
 * @return
 *  - The status of the event. If not exist, will return 0.
 */
//-------------------------------------------------------------------------------------------------
le_result_t taf_DIDDataAccess_ReadDID
(
    uint16_t did,     ///< [IN]
    uint8_t* value,   ///< [OUT]
    size_t *len      ///< [OUT]
)
{
    auto &didDataHandler = DidDataHandler::GetInstance();

    return didDataHandler.ReadDID(did, value, len);
}
