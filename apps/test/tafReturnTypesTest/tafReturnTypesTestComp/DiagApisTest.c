/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate Diag Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void diagRetTest_RunApis
(
  void
)
{
    le_result_t res;
    LE_TEST_INFO("diagRetTest_RunApis");

    //1.taf_diag_SetEnableCondition LE_OK scenario
    res = taf_diag_SetEnableCondition(5,false);
    LE_TEST_OK(res == LE_OK,"taf_diag_SetEnableCondition-LE_OK");

    //2.taf_diag_GetEnableConditionStatus false scenario
    bool enable;
    enable = taf_diag_GetEnableConditionStatus(5);
    LE_TEST_OK(enable == false,"taf_diag_GetEnableConditionStatus-false");

    //3.taf_diag_SetVlanId LE_BAD_PARAMETER scenario
    res = taf_diag_SetVlanId(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diag_SetVlanId-LE_BAD_PARAMETER");

    //4.taf_diag_ReleaseTesterStateMsg LE_OK scenario
    res = taf_diag_ReleaseTesterStateMsg(NULL);
    LE_TEST_OK(res == LE_OK,"taf_diag_ReleaseTesterStateMsg-LE_OK");

    //5.taf_diag_SelectTargetVlanID LE_BAD_PARAMETER scenario
    res = taf_diag_SelectTargetVlanID(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diag_SelectTargetVlanID-LE_BAD_PARAMETER");

    //6.taf_diag_Pause LE_BAD_PARAMETER scenario
    res = taf_diag_Pause(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diag_Pause-LE_BAD_PARAMETER");

    //7.taf_diag_Resume LE_BAD_PARAMETER scenario
    res = taf_diag_Resume(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diag_Resume-LE_BAD_PARAMETER");

    //8.taf_diag_RemoveSvc LE_BAD_PARAMETER scenario
    res = taf_diag_RemoveSvc(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diag_RemoveSvc-LE_BAD_PARAMETER");

    //9.taf_diagAuth_SetVlanId LE_BAD_PARAMETER scenario
    res = taf_diagAuth_SetVlanId(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_SetVlanId-LE_BAD_PARAMETER");

    //10.taf_diagAuth_GetCommConf LE_BAD_PARAMETER scenario
    res = taf_diagAuth_GetCommConf(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_GetCommConf-LE_BAD_PARAMETER");

    //11.taf_diagAuth_GetCertSize LE_BAD_PARAMETER scenario
    res = taf_diagAuth_GetCertSize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_GetCertSize-LE_BAD_PARAMETER");

    //12.taf_diagAuth_GetCert LE_BAD_PARAMETER scenario
    res = taf_diagAuth_GetCert(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_GetCert-LE_BAD_PARAMETER");

    //13.taf_diagAuth_GetChallengeSize LE_BAD_PARAMETER scenario
    res = taf_diagAuth_GetChallengeSize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_GetChallengeSize-LE_BAD_PARAMETER");

    //14.taf_diagAuth_GetChallenge LE_BAD_PARAMETER scenario
    res = taf_diagAuth_GetChallenge(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_GetChallenge-LE_BAD_PARAMETER");

    //15.taf_diagAuth_GetPOWNSize LE_BAD_PARAMETER scenario
    res = taf_diagAuth_GetPOWNSize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_GetPOWNSize-LE_BAD_PARAMETER");

    //16.taf_diagAuth_GetPOWN LE_BAD_PARAMETER scenario
    res = taf_diagAuth_GetPOWN(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_GetPOWN-LE_BAD_PARAMETER");

    //17.taf_diagAuth_GetPublicKeySize LE_BAD_PARAMETER scenario
    res = taf_diagAuth_GetPublicKeySize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_GetPublicKeySize-LE_BAD_PARAMETER");

    //18.taf_diagAuth_GetPublicKey LE_BAD_PARAMETER scenario
    res = taf_diagAuth_GetPublicKey(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_GetPublicKey-LE_BAD_PARAMETER");

    //19.taf_diagAuth_SetRole LE_BAD_PARAMETER scenario
    res = taf_diagAuth_SetRole(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_SetRole-LE_BAD_PARAMETER");

    //20.taf_diagAuth_GetCertEvalId LE_BAD_PARAMETER scenario
    res = taf_diagAuth_GetCertEvalId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_GetCertEvalId-LE_BAD_PARAMETER");

    //21.taf_diagAuth_GetVlanIdFromMsg LE_BAD_PARAMETER scenario
    res = taf_diagAuth_GetVlanIdFromMsg(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_GetVlanIdFromMsg-LE_BAD_PARAMETER");

    //22.taf_diagAuth_SetChallenge LE_BAD_PARAMETER scenario
    static uint8_t challengeSvr[100];
    res = taf_diagAuth_SetChallenge(NULL,challengeSvr,sizeof(challengeSvr));
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_SetChallenge-LE_BAD_PARAMETER");

    //23.taf_diagAuth_SetPublicKey LE_BAD_PARAMETER scenario
    res = taf_diagAuth_SetPublicKey(NULL,challengeSvr,sizeof(challengeSvr));
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_SetPublicKey-LE_BAD_PARAMETER");

    //24.taf_diagAuth_SetSessKeyInfo LE_BAD_PARAMETER scenario
    res = taf_diagAuth_SetSessKeyInfo(NULL,challengeSvr,sizeof(challengeSvr));
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_SetSessKeyInfo-LE_BAD_PARAMETER");

    //25.taf_diagAuth_SendResp LE_BAD_PARAMETER scenario
    res = taf_diagAuth_SendResp(NULL,0,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_SendResp-LE_BAD_PARAMETER");

    //25.taf_diagAuth_ReleaseAuthExpMsg LE_NOT_IMPLEMENTED scenario
    res = taf_diagAuth_ReleaseAuthExpMsg(NULL);
    LE_TEST_OK(res == LE_NOT_IMPLEMENTED,"taf_diagAuth_ReleaseAuthExpMsg-LE_NOT_IMPLEMENTED");

    //26.taf_diagAuth_RemoveSvc LE_BAD_PARAMETER scenario
    res = taf_diagAuth_RemoveSvc(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagAuth_RemoveSvc-LE_BAD_PARAMETER");

    //27.taf_diagDataID_SetVlanId LE_UNSUPPORTED scenario
    res = taf_diagDataID_SetVlanId(NULL,0);
    LE_TEST_OK(res == LE_UNSUPPORTED,"taf_diagDataID_SetVlanId-LE_UNSUPPORTED");

    //28.taf_diagDataID_SendReadDIDResp LE_BAD_PARAMETER scenario
    res = taf_diagDataID_SendReadDIDResp(NULL,0,challengeSvr,sizeof(challengeSvr));
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDataID_SendReadDIDResp-LE_BAD_PARAMETER");

    //29.taf_diagDataID_GetWriteDataRecord LE_BAD_PARAMETER scenario
    res = taf_diagDataID_GetWriteDataRecord(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDataID_GetWriteDataRecord-LE_BAD_PARAMETER");

    //29.taf_diagDataID_SendWriteDIDResp LE_BAD_PARAMETER scenario
    res = taf_diagDataID_SendWriteDIDResp(NULL,0,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDataID_SendWriteDIDResp-LE_BAD_PARAMETER");

    //30.taf_diagDataID_GetVlanIdFromMsg LE_BAD_PARAMETER scenario
    res = taf_diagDataID_GetVlanIdFromMsg(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDataID_GetVlanIdFromMsg-LE_BAD_PARAMETER");

    //31.taf_diagDataID_RemoveSvc LE_BAD_PARAMETER scenario
    res = taf_diagDataID_RemoveSvc(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDataID_RemoveSvc-LE_BAD_PARAMETER");

    //32.taf_diagDOIP_GetVIN LE_BAD_PARAMETER scenario
    res = taf_diagDoIP_GetVIN(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDOIP_GetVIN-LE_BAD_PARAMETER");

    //33.taf_diagDOIP_GetEID LE_BAD_PARAMETER scenario
    res = taf_diagDoIP_GetEID(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDOIP_GetEID-LE_BAD_PARAMETER");

    //34.taf_diagDOIP_GetGID LE_BAD_PARAMETER scenario
    res = taf_diagDoIP_GetGID(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDOIP_GetGID-LE_BAD_PARAMETER");

    //35.taf_diagDOIP_RemoveSvc LE_FAULT scenario
    res = taf_diagDoIP_RemoveSvc(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_diagDOIP_RemoveSvc-LE_FAULT");

    //36.taf_diagDTC_GetCode LE_BAD_PARAMETER scenario
    res = taf_diagDTC_GetCode(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDTC_GetCode-LE_BAD_PARAMETER");

    //37.taf_diagDTC_GetFaultDetectionCounter LE_BAD_PARAMETER scenario
    res = taf_diagDTC_GetFaultDetectionCounter(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDTC_GetFaultDetectionCounter-LE_BAD_PARAMETER");

    //38.taf_diagDTC_ReadStatus LE_BAD_PARAMETER scenario
    res = taf_diagDTC_ReadStatus(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDTC_ReadStatus-LE_BAD_PARAMETER");

    //39.taf_diagDTC_SetActivationStatus LE_BAD_PARAMETER scenario
    res = taf_diagDTC_SetActivationStatus(NULL,TAF_DIAGDTC_INACTIVE);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDTC_SetActivationStatus-LE_BAD_PARAMETER");

    //40.taf_diagDTC_GetActivationStatus LE_BAD_PARAMETER scenario
    res = taf_diagDTC_GetActivationStatus(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDTC_GetActivationStatus-LE_BAD_PARAMETER");

    //41.taf_diagDTC_SetSuppression LE_BAD_PARAMETER scenario
    res = taf_diagDTC_SetSuppression(NULL,false);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDTC_SetSuppression-LE_BAD_PARAMETER");

    //42.taf_diagDTC_GetSuppression LE_BAD_PARAMETER scenario
    res = taf_diagDTC_GetSuppression(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDTC_GetSuppression-LE_BAD_PARAMETER");

    //43.taf_diagDTC_ClearInfo LE_BAD_PARAMETER scenario
    res = taf_diagDTC_ClearInfo(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDTC_ClearInfo-LE_BAD_PARAMETER");

    //44.taf_diagDTC_GetDataList NULL scenario
    taf_diagDTC_DataListRef_t dataList;
    dataList = taf_diagDTC_GetDataList(NULL);
    LE_TEST_OK(dataList == NULL,"taf_diagDTC_GetDataList-NULL");

    //45.taf_diagDTC_GetFirstData NULL scenario
    taf_diagDTC_DataRef_t firstData;
    firstData = taf_diagDTC_GetFirstData(NULL);
    LE_TEST_OK(firstData == NULL,"taf_diagDTC_GetFirstData-NULL");

    //46.taf_diagDTC_GetNextData NULL scenario
    firstData = taf_diagDTC_GetNextData(NULL);
    LE_TEST_OK(firstData == NULL,"taf_diagDTC_GetNextData-NULL");

    //47.taf_diagDTC_DeleteDataList LE_BAD_PARAMETER scenario
    res = taf_diagDTC_DeleteDataList(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDTC_DeleteDataList-LE_BAD_PARAMETER");

    //48.taf_diagDTC_GetDataDtcCode LE_NOT_FOUND scenario
    res = taf_diagDTC_GetDataDtcCode(NULL,NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_diagDTC_GetDataDtcCode-LE_NOT_FOUND");

    //49.taf_diagDTC_GetDataType LE_NOT_FOUND scenario
    res = taf_diagDTC_GetDataType(NULL,NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_diagDTC_GetDataType-LE_NOT_FOUND");

    //50.taf_diagDTC_GetRecordNumber LE_NOT_FOUND scenario
    res = taf_diagDTC_GetRecordNumber(NULL,NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_diagDTC_GetRecordNumber-LE_NOT_FOUND");

    //51.taf_diagDTC_GetDataId LE_NOT_FOUND scenario
    res = taf_diagDTC_GetDataId(NULL,NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_diagDTC_GetDataId-LE_NOT_FOUND");

    //52.taf_diagDTC_GetDataValue LE_NOT_FOUND scenario
    res = taf_diagDTC_GetDataValue(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_NOT_FOUND,"taf_diagDTC_GetDataValue-LE_NOT_FOUND");

    //53.taf_diagDTC_RemoveSvc LE_FAULT scenario
    res = taf_diagDTC_RemoveSvc(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_diagDTC_RemoveSvc-LE_FAULT");

    //54.taf_diagDTC_ClearAllInfo LE_BAD_PARAMETER scenario
    res = taf_diagDTC_ClearAllInfo(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDTC_ClearAllInfo-LE_BAD_PARAMETER");

    //55.taf_diagDTC_SetAllSuppression LE_BAD_PARAMETER scenario
    res = taf_diagDTC_SetAllSuppression(NULL,false);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagDTC_SetAllSuppression-LE_BAD_PARAMETER");

    //56.taf_diagDTC_RemoveAllSvc LE_FAULT scenario
    res = taf_diagDTC_RemoveAllSvc(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_diagDTC_RemoveAllSvc-LE_FAULT");

    //57.taf_diagEvent_GetId LE_BAD_PARAMETER scenario
    res = taf_diagEvent_GetId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagEvent_GetId-LE_BAD_PARAMETER");

    //58.taf_diagEvent_SetStatus LE_BAD_PARAMETER scenario
    res = taf_diagEvent_SetStatus(NULL,TAF_DIAGEVENT_PASSED);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagEvent_SetStatus-LE_BAD_PARAMETER");

    //59.taf_diagEvent_SetStatusWithSupplierFaultCode LE_BAD_PARAMETER scenario
    uint8_t supplierFaultCode[5]={0x33, 0x34, 0x35, 0x36, 0x37};
    res = taf_diagEvent_SetStatusWithSupplierFaultCode(NULL,TAF_DIAGEVENT_FAILED,supplierFaultCode,5);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagEvent_SetStatusWithSupplierFaultCode-LE_BAD_PARAMETER");

    //60.taf_diagEvent_GetUdsStatus LE_BAD_PARAMETER scenario
    res = taf_diagEvent_GetUdsStatus(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagEvent_GetUdsStatus-LE_BAD_PARAMETER");

    //61.taf_diagEvent_GetOperationCycleId LE_BAD_PARAMETER scenario
    res = taf_diagEvent_GetOperationCycleId(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagEvent_GetOperationCycleId-LE_BAD_PARAMETER");

    //62.taf_diagEvent_GetDTCCode LE_BAD_PARAMETER scenario
    res = taf_diagEvent_GetDTCCode(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagEvent_GetDTCCode-LE_BAD_PARAMETER");

    //63.taf_diagEvent_GetEnableCondState LE_BAD_PARAMETER scenario
    res = taf_diagEvent_GetEnableCondState(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagEvent_GetEnableCondState-LE_BAD_PARAMETER");

    //64.taf_diagEvent_ResetDebounceStatus LE_BAD_PARAMETER scenario
    res = taf_diagEvent_ResetDebounceStatus(NULL,TAF_DIAGEVENT_DEBOUNCE_RESET);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagEvent_ResetDebounceStatus-LE_BAD_PARAMETER");

    //65.taf_diagEvent_RemoveSvc LE_FAULT scenario
    res = taf_diagEvent_RemoveSvc(NULL);
    LE_TEST_OK(res == LE_FAULT,"taf_diagEvent_RemoveSvc-LE_FAULT");

    //66.taf_diagEvent_SetOpCycleState LE_BAD_PARAMETER scenario
    res = taf_diagEvent_SetOpCycleState(NULL,TAF_DIAGEVENT_CYCLE_START);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagEvent_SetOpCycleState-LE_BAD_PARAMETER");

    //67.taf_diagEvent_GetOpCycleState LE_BAD_PARAMETER scenario
    res = taf_diagEvent_GetOpCycleState(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagEvent_GetOpCycleState-LE_BAD_PARAMETER");

    //68.taf_diagEvent_GetOpCycleIdByRef LE_BAD_PARAMETER scenario
    res = taf_diagEvent_GetOpCycleIdByRef(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagEvent_GetOpCycleIdByRef-LE_BAD_PARAMETER");

    //69.taf_diagEvent_RemoveOpCycle LE_BAD_PARAMETER scenario
    res = taf_diagEvent_RemoveOpCycle(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagEvent_RemoveOpCycle-LE_BAD_PARAMETER");

    //70.taf_diagIOCtrl_SetVlanId LE_BAD_PARAMETER scenario
    res = taf_diagIOCtrl_SetVlanId(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagIOCtrl_SetVlanId-LE_BAD_PARAMETER");

    //71.taf_diagIOCtrl_GetCtrlState LE_BAD_PARAMETER scenario
    res = taf_diagIOCtrl_GetCtrlState(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagIOCtrl_GetCtrlState-LE_BAD_PARAMETER");

    //72.taf_diagIOCtrl_GetCtrlEnableMaskRecd LE_BAD_PARAMETER scenario
    res = taf_diagIOCtrl_GetCtrlEnableMaskRecd(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagIOCtrl_GetCtrlEnableMaskRecd-LE_BAD_PARAMETER");

    //73.taf_diagIOCtrl_GetVlanIdFromMsg LE_BAD_PARAMETER scenario
    res = taf_diagIOCtrl_GetVlanIdFromMsg(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagIOCtrl_GetVlanIdFromMsg-LE_BAD_PARAMETER");

    //74.taf_diagIOCtrl_GetVlanIdFromMsg LE_BAD_PARAMETER scenario
    res = taf_diagIOCtrl_SendResp(NULL, TAF_DIAGIOCTRL_CONDITIONS_NOT_CORRECT, NULL, 0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagIOCtrl_GetVlanIdFromMsg-LE_BAD_PARAMETER");

    //75.taf_diagIOCtrl_RemoveSvc LE_BAD_PARAMETER scenario
    res = taf_diagIOCtrl_RemoveSvc(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagIOCtrl_RemoveSvc-LE_BAD_PARAMETER");

    //76.taf_diagReset_SetVlanId LE_BAD_PARAMETER scenario
    res = taf_diagReset_SetVlanId(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagReset_SetVlanId-LE_BAD_PARAMETER");

    //77.taf_diagReset_GetVlanIdFromMsg LE_BAD_PARAMETER scenario
    res = taf_diagReset_GetVlanIdFromMsg(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagReset_GetVlanIdFromMsg-LE_BAD_PARAMETER");

    //78.taf_diagReset_SendResp LE_BAD_PARAMETER scenario
    res = taf_diagReset_SendResp(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagReset_SendResp-LE_BAD_PARAMETER");

    //79.taf_diagReset_RemoveSvc LE_BAD_PARAMETER scenario
    res = taf_diagReset_RemoveSvc(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagReset_RemoveSvc-LE_BAD_PARAMETER");

    //80.taf_diagRoutineCtrl_SetVlanId LE_BAD_PARAMETER scenario
    res = taf_diagRoutineCtrl_SetVlanId(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagRoutineCtrl_SetVlanId-LE_BAD_PARAMETER");

    //81.taf_diagRoutineCtrl_GetRoutineCtrlRec LE_BAD_PARAMETER scenario
    res = taf_diagRoutineCtrl_GetRoutineCtrlRec(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagRoutineCtrl_GetRoutineCtrlRec-LE_BAD_PARAMETER");

    //82.taf_diagRoutineCtrl_GetVlanIdFromMsg LE_BAD_PARAMETER scenario
    res = taf_diagRoutineCtrl_GetVlanIdFromMsg(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagRoutineCtrl_GetVlanIdFromMsg-LE_BAD_PARAMETER");

    //83.taf_diagRoutineCtrl_SendResp LE_BAD_PARAMETER scenario
    res = taf_diagRoutineCtrl_SendResp(NULL,TAF_DIAGROUTINECTRL_CONDITIONS_NOT_CORRECT,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagRoutineCtrl_SendResp-LE_BAD_PARAMETER");

    //84.taf_diagRoutineCtrl_RemoveSvc LE_BAD_PARAMETER scenario
    res = taf_diagRoutineCtrl_RemoveSvc(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagRoutineCtrl_RemoveSvc-LE_BAD_PARAMETER");

    //85.taf_diagSecurity_SetVlanId LE_BAD_PARAMETER scenario
    res = taf_diagSecurity_SetVlanId(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagSecurity_SetVlanId-LE_BAD_PARAMETER");

    //86.taf_diagSecurity_SendSesTypeCheckResp LE_BAD_PARAMETER scenario
    res = taf_diagSecurity_SendSesTypeCheckResp(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagSecurity_SendSesTypeCheckResp-LE_BAD_PARAMETER");

    //87.taf_diagSecurity_SelectTargetVlanID LE_BAD_PARAMETER scenario
    res = taf_diagSecurity_SelectTargetVlanID(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagSecurity_SelectTargetVlanID-LE_BAD_PARAMETER");

    //88.taf_diagSecurity_GetCurrentSesType LE_BAD_PARAMETER scenario
    res = taf_diagSecurity_GetCurrentSesType(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagSecurity_GetCurrentSesType-LE_BAD_PARAMETER");

    //89.taf_diagSecurity_ReleaseSesChangeMsg LE_BAD_PARAMETER scenario
    res = taf_diagSecurity_ReleaseSesChangeMsg(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagSecurity_ReleaseSesChangeMsg-LE_BAD_PARAMETER");

    //90.taf_diagSecurity_GetSecAccessPayloadLen LE_BAD_PARAMETER scenario
    res = taf_diagSecurity_GetSecAccessPayloadLen(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagSecurity_GetSecAccessPayloadLen-LE_BAD_PARAMETER");

    //91.taf_diagSecurity_GetSecAccessPayload LE_BAD_PARAMETER scenario
    res = taf_diagSecurity_GetSecAccessPayload(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagSecurity_GetSecAccessPayload-LE_BAD_PARAMETER");

    //92.taf_diagSecurity_SendSecAccessResp LE_BAD_PARAMETER scenario
    res = taf_diagSecurity_SendSecAccessResp(NULL,0,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagSecurity_SendSecAccessResp-LE_BAD_PARAMETER");

    //93.taf_diagSecurity_GetVlanIdFromMsg LE_BAD_PARAMETER scenario
    res = taf_diagSecurity_GetVlanIdFromMsg(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagSecurity_GetVlanIdFromMsg-LE_BAD_PARAMETER");

    //94.taf_diagSecurity_RemoveSvc LE_BAD_PARAMETER scenario
    res = taf_diagSecurity_RemoveSvc(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagSecurity_RemoveSvc-LE_BAD_PARAMETER");

    //95.taf_diagUpdate_SetVlanId LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_SetVlanId(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_SetVlanId-LE_BAD_PARAMETER");

    //96.taf_diagUpdate_GetFilePathAndName LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_GetFilePathAndName(NULL,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_GetFilePathAndName-LE_BAD_PARAMETER");

    //97.taf_diagUpdate_GetDataFormatID LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_GetDataFormatID(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_GetDataFormatID-LE_BAD_PARAMETER");

    //98.taf_diagUpdate_GetUnCompFileSize LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_GetUnCompFileSize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_GetUnCompFileSize-LE_BAD_PARAMETER");

    //99.taf_diagUpdate_GetCompFileSize LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_GetCompFileSize(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_GetCompFileSize-LE_BAD_PARAMETER");

    //100.taf_diagUpdate_SetFilePosition LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_SetFilePosition(NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_SetFilePosition-LE_BAD_PARAMETER");

    //101.taf_diagUpdate_SetFileSizeOrDirInfoLength LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_SetFileSizeOrDirInfoLength(NULL,0,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_SetFileSizeOrDirInfoLength-LE_BAD_PARAMETER");

    //102.taf_diagUpdate_SendFileXferResp LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_SendFileXferResp(NULL,TAF_DIAGUPDATE_FILE_XFER_NO_ERROR);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_SendFileXferResp-LE_BAD_PARAMETER");

    //103.taf_diagUpdate_GetblockSeqCount LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_GetblockSeqCount(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_GetblockSeqCount-LE_BAD_PARAMETER");

    //104.taf_diagUpdate_GetXferDataParamRecLen LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_GetXferDataParamRecLen(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_GetXferDataParamRecLen-LE_BAD_PARAMETER");

    //105.taf_diagUpdate_GetXferDataParamRec LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_GetXferDataParamRec(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_GetXferDataParamRec-LE_BAD_PARAMETER");

    //106.taf_diagUpdate_SendXferDataResp LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_SendXferDataResp(NULL,0,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_SendXferDataResp-LE_BAD_PARAMETER");

    //107.taf_diagUpdate_GetXferExitParamRecLen LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_GetXferExitParamRecLen(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_GetXferExitParamRecLen-LE_BAD_PARAMETER");

    //108.taf_diagUpdate_GetXferExitParamRec LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_GetXferExitParamRec(NULL,NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_GetXferExitParamRec-LE_BAD_PARAMETER");

    //109.taf_diagUpdate_SendXferExitResp LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_SendXferExitResp(NULL,0,NULL,0);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_SendXferExitResp-LE_BAD_PARAMETER");

    //110.taf_diagUpdate_GetVlanIdFromMsg LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_GetVlanIdFromMsg(NULL,NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_GetVlanIdFromMsg-LE_BAD_PARAMETER");

    //111.taf_diagUpdate_RemoveSvc LE_BAD_PARAMETER scenario
    res = taf_diagUpdate_RemoveSvc(NULL);
    LE_TEST_OK(res == LE_BAD_PARAMETER,"taf_diagUpdate_RemoveSvc-LE_BAD_PARAMETER");

}

