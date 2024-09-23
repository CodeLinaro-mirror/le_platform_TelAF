/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <fstream>

#include "tafUpdate.hpp"
#include "tafFwUpdate.hpp"

using namespace telux::tafsvc;

/*======================================================================
 FUNCTION        taf_fwupdate_RebootToActive
 DESCRIPTION     Reboot to active slot
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
void taf_fwupdate_RebootToActive()
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
 *  - LE_FAULT         On failure.
 *  - LE_OK            On success.
 *  - LE_BAD_PARAMETER Invalid parameters.
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
 *  - LE_FAULT On failure.
 *  - LE_OK    On success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_fwupdate_Install()
{
    taf_FwUpdateReq_t fwupdateReq;

    fwupdateReq.event = TAF_FWUPDATE_EV_START_INSTALL;
    le_utf8_Copy(fwupdateReq.filePath, TAF_FWUPDATE_LOCAL_PACAKAGE_PATH,
        TAF_UPDATE_FILE_PATH_LEN, NULL);
    le_event_Report(taf_FwUpdate::fwUpdateEvId, &fwupdateReq, sizeof(taf_FwUpdateReq_t));

    return LE_OK;
}