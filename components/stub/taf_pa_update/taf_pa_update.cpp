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

#include "legato.h"
#include "interfaces.h"
#include "taf_pa_update.hpp"

/*======================================================================
 FUNCTION        taf_pa_update_GetSession
 DESCRIPTION     Get download session
 PARAMETERS      void
 RETURN VALUE    taf_update_SessionRef_t : Session reference
======================================================================*/
LE_SHARED taf_update_SessionRef_t taf_pa_update_GetSession()
{
    return nullptr;
}

/*======================================================================
 FUNCTION        taf_pa_update_DeleteSession
 DESCRIPTION     Delete download session
 PARAMETERS      [IN] sessionRef : Session reference
 RETURN VALUE    void
======================================================================*/
LE_SHARED void taf_pa_update_DeleteSession(taf_update_SessionRef_t sessionRef)
{
    return;
}

/*======================================================================
 FUNCTION        taf_pa_update_Download
 DESCRIPTION     Download update package
 PARAMETERS      [IN] sessionRef : Session reference
 RETURN VALUE    int : 0 - On success, -1 - On failure.
======================================================================*/
LE_SHARED int taf_pa_update_Download(taf_update_SessionRef_t sessionRef)
{
    return 0;
}

/*======================================================================
 FUNCTION        taf_pa_update_GetProgress
 DESCRIPTION     Download update package
 PARAMETERS      [OUT] state : Download progress state
                 [OUT] percent : Download percent
 RETURN VALUE    int : 0 - On success, -1 - On failure.
======================================================================*/
LE_SHARED int taf_pa_update_GetProgress(taf_update_ProgressState_t* state, int* percent)
{
    return 0;
}

/*======================================================================
 FUNCTION        taf_pa_update_Report
 DESCRIPTION     Report update state
 PARAMETERS      [IN] sessionRef : Session reference
                 [IN] state : Report state
 RETURN VALUE    int : 0 - On success, -1 - On failure.
======================================================================*/
LE_SHARED int taf_pa_update_Report(taf_update_SessionRef_t sessionRef, taf_update_ReportState_t state)
{
    return 0;
}

/*======================================================================
 FUNCTION        COMPONENT_INIT
 DESCRIPTION     Component initialization
 PARAMETERS      void
 RETURN VALUE    void
======================================================================*/
COMPONENT_INIT
{
    LE_INFO("taf_pa_update stub");
}
