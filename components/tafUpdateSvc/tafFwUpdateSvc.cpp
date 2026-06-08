/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include <fstream>

#include "tafUpdate.hpp"
#include "tafFwUpdate.hpp"

using namespace tafsvc;

//--------------------------------------------------------------------------------------------------
/**
 * Reboot the system to the active bank.
 */
//--------------------------------------------------------------------------------------------------
void taf_fwupdate_RebootToActive
(
    void
)
{
    if (reboot(RB_AUTOBOOT) == -1)
    {
        LE_ERROR("Fail to reboot. Errno = %s.", LE_ERRNO_TXT(errno));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get current firmware version.
 *
 * @return
 *  - LE_OK             The firmware version string was retrieved successfully.
 *  - LE_BAD_PARAMETER  versionPtr is null.
 *  - LE_FAULT          The service-level request failed because taf_FwUpdate::GetFirmwareVersion()
 *                      could not parse or obtain the current firmware version.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_fwupdate_GetFirmwareVersion
(
    char* versionPtr,         ///< [OUT] Firmware version string.
    size_t versionNumElements ///< [IN] The number of characters in version.
)
{
    TAF_ERROR_IF_RET_VAL(versionPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(versionPtr)");

    auto &tafFwUpdate = taf_FwUpdate::GetInstance();

    if (tafFwUpdate.GetFirmwareVersion(versionPtr) != LE_OK)
    {
        LE_ERROR("Fail to get firmware version.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Install firmware.
 *
 * @return
 *  - LE_OK    The firmware installation request was queued successfully.
 *
 * @note This API reports synchronous request-queuing success only. Final install success or failure
 *       is delivered asynchronously through update state indications driven by the firmware update
 *       workflow.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_fwupdate_Install
(
    void
)
{
    taf_FwUpdateReq_t fwupdateReq;

    fwupdateReq.event = TAF_FWUPDATE_EV_START_INSTALL;
    le_utf8_Copy(fwupdateReq.filePath, TAF_FWUPDATE_LOCAL_PACAKAGE_PATH,
        TAF_UPDATE_FILE_PATH_LEN, NULL);
    le_event_Report(taf_FwUpdate::fwUpdateEvId, &fwupdateReq, sizeof(taf_FwUpdateReq_t));

    return LE_OK;
}