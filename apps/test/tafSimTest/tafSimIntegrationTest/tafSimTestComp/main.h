/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#include "legato.h"
#include "interfaces.h"

char* SimStateToString ( taf_sim_States_t state);
void tafSimTest_state( taf_sim_Id_t slot );
void tafSimTest_allState();
void tafSimTest_info( taf_sim_Id_t slot );
void tafSimTest_allInfo();
void tafSimTest_selection( taf_sim_Id_t slot);
void tafSimTest_enterPin( taf_sim_Id_t simId, taf_sim_LockType_t lockType,
        const char*  pinPtr);
void tafSimTest_setLock( taf_sim_Id_t simId, taf_sim_LockType_t lockType,
        const char*  pinPtr, bool lock);
void tafSimTest_Change_pin( taf_sim_Id_t simId, taf_sim_LockType_t lockType,
        const char* oldpinPtr, const char* newpinPtr);
void tafSimTest_unblock_puk( taf_sim_Id_t simId, taf_sim_LockType_t lockType,
        const char* pukPtr, const char* newpinPtr);
void tafSimTest_GetAppTypes(taf_sim_Id_t simId);
void tafSimTest_sim_access( taf_sim_Id_t simId);
void tafSimTest_sim_openLogicalChannel(taf_sim_Id_t simId, const char* aid);
void tafSimTest_sim_closeLogicalChannel(taf_sim_Id_t simId, uint8_t channelId);
void tafSimTest_SetPowerCheck(taf_sim_Id_t simId, le_onoff_t powerStatus);
void tafSimTest_sim_isEmergency(taf_sim_Id_t simId);
void tafSimTest_swapToEmergencyAndBack(taf_sim_Id_t simId, taf_sim_Manufacturer_t manufacturer);
void tafSimTest_fplmnList_test(taf_sim_Id_t simId);
void tafSimTest_createFplmnList_test(taf_sim_Id_t simId);
void tafSimTest_addFplmnOperator_test(taf_sim_Id_t simId, const char* mcc, const char* mnc);
void tafSimTest_writeFplmnList_test(taf_sim_Id_t simId, const char* mcc, const char* mnc);
void tafSimTest_writeFplmnLists_test(taf_sim_Id_t simId);

void tafSimTest_getFirstFplmnOperator_test(taf_sim_Id_t simId);
void tafSimTest_getNextFplmnOperator_test(taf_sim_Id_t simId);
void tafSimTest_deleteFplmnList_test(taf_sim_Id_t simId);
