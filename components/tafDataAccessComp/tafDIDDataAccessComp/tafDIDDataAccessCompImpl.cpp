/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include "tafDIDDataAccessComp.h"
#include "tafDIDEntityDAO.hpp"
#include "tafDIDDataAccessCompImpl.hpp"

using namespace std;
using namespace taf::dataAccess;

DidDataHandler &DidDataHandler::GetInstance
(
)
{
    static DidDataHandler instance;

    return instance;
}

DidDataHandler::DidDataHandler
(
)
{
}

DidDataHandler::~DidDataHandler
(
)
{
}

void DidDataHandler::Init
(
)
{
    auto &tafDIDDao = DIDEntityDAO::GetInstance();
    tafDIDDao.Init(DID_DATABASE_NAME, DID_DB_VERSION);

    tafDIDDao.GetDaoHandler()->SetVersion(DID_DB_VERSION);
    tafDIDDao.GetDaoHandler()->EnableWAL();
}

le_result_t DidDataHandler::Load
(
)
{
    auto &tafDIDDao = DIDEntityDAO::GetInstance();

    le_result_t ret = tafDIDDao.Load();
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to load DID table");
        return ret;
    }

    return LE_OK;
}

le_result_t DidDataHandler::WriteDID
(
    uint16_t did,
    const uint8_t *value,
    size_t len
)
{
    auto &tafDIDDao = DIDEntityDAO::GetInstance();

    LE_DEBUG("Write DID 0x%x to DB, len=%zu", did, len);
    if (value == nullptr)
    {
        LE_ERROR("Invalid input parameters");
        return LE_BAD_PARAMETER;
    }

    return tafDIDDao.WriteDID(did, value, len);
}

le_result_t DidDataHandler::ReadDID
(
    uint16_t did,
    uint8_t *value,
    size_t *len
)
{
    auto &tafDIDDao = DIDEntityDAO::GetInstance();

    LE_DEBUG("Read DID 0x%x from DB", did);
    if (value == nullptr || len == nullptr)
    {
        LE_ERROR("Invalid input parameters");
        return LE_BAD_PARAMETER;
    }

    return tafDIDDao.ReadDID(did, value, len);
}
