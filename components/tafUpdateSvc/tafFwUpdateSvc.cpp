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
    taf_FwUpdateReq_t updateReq;
    updateReq.event = TAF_FWUPDATE_EV_REBOOT_TO_ACTIVE;
    le_event_Report(taf_FwUpdate::fwUpdateEvId, &updateReq, sizeof(taf_FwUpdateReq_t));
}

/*======================================================================
 FUNCTION        taf_fwupdate_GetFirmwareVersion
 DESCRIPTION     Get firmware version
 PARAMETERS      [OUT] versionPtr: Firmware version
                 [IN] versionNumElements: version size in bytes
 RETURN VALUE    void
======================================================================*/
le_result_t taf_fwupdate_GetFirmwareVersion(char* versionPtr, size_t versionNumElements)
{
    TAF_ERROR_IF_RET_VAL(versionPtr == nullptr, LE_BAD_PARAMETER, "Null ptr(versionPtr)");

    std::ifstream fin(TAF_FWUPDATE_VERSION_FILE);
    std::string verstr;
    getline(fin, verstr);
    le_utf8_Copy(versionPtr, verstr.c_str(), TAF_FWUPDATE_MAX_VERS_LEN, NULL);
    fin.close();

    return LE_OK;
}

/*======================================================================
 FUNCTION        taf_fwupdate_Install
 DESCRIPTION     Install firmware
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
le_result_t taf_fwupdate_Install()
{
    taf_FwUpdateReq_t fwupdateReq;
    fwupdateReq.event = TAF_FWUPDATE_EV_INSTALL;
    le_utf8_Copy(fwupdateReq.name, TAF_UPDATE_FOTA_PAKCAGE_FILE_PATH,
        TAF_UPDATE_MAX_PKG_NAME_LEN, NULL);
    le_event_Report(taf_FwUpdate::fwUpdateEvId, &fwupdateReq, sizeof(taf_FwUpdateReq_t));
    return LE_OK;
}