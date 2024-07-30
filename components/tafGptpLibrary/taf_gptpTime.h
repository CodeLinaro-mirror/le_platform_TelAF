/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_PTP_LIB_H
#define TAF_PTP_LIB_H

#include <time.h>
#include <legato.h>
#include <interfaces.h>


#ifdef __cplusplus
extern "C"
{
#endif

//-------------------------------------------------------------------------------------------------
/**
* Reference type for Gptp time.
*/
//-------------------------------------------------------------------------------------------------
typedef struct taf_gptpTime_Ref* taf_gptpTime_Ref_t;

/*-------------------------------------------------------------------------------------------------

 FUNCTION        taf_GptpTime_CreateRef

 DESCRIPTION     Creates the gptp time reference with the given device name.

 DEPENDENCIES    Initialization of Time Service

 PARAMETERS      [IN] const char* deviceName: Name of the gptp device.
                 Note: Device name can be found under "/dev" (E.g "/dev/ptp0")

 RETURN VALUE    taf_GptpTime_Ref_t: Gptp reference
                 NULL:               Fail.

 SIDE EFFECTS

-------------------------------------------------------------------------------------------------*/
LE_SHARED taf_gptpTime_Ref_t taf_gptpTime_CreateRef
(
    const char* deviceName
);

/*-------------------------------------------------------------------------------------------------

 FUNCTION        taf_GptpTime_GetTimeValue

 DESCRIPTION     Gets the gptp time with the given reference.

 DEPENDENCIES    Initialization of Time Service

 PARAMETERS      [IN] taf_GptpTime_Ref_t gptpTimeRef:gptp reference.
                 [OUT] timespec* gptpTimeValuePtr: gptp time

 RETURN VALUE    le_result_t
                 LE_FAULT:             Fail.
                 LE_OK:                Success.

 SIDE EFFECTS
 
-------------------------------------------------------------------------------------------------*/
LE_SHARED le_result_t taf_gptpTime_GetTimeValue
(
    taf_gptpTime_Ref_t gptpTimeRef,
    struct timespec* gptpTimeValuePtr
);

/*-------------------------------------------------------------------------------------------------

 FUNCTION        taf_GptpTime_DeleteRef

 DESCRIPTION     Deletes the gptp time reference.

 DEPENDENCIES    Initialization of Time Service

 PARAMETERS      [IN] taf_GptpTime_Ref_t gptpTimeRef:gptp reference.

 RETURN VALUE    le_result_t
                 LE_FAULT:             Fail.
                 LE_OK:                Success.

 SIDE EFFECTS
 
-------------------------------------------------------------------------------------------------*/
LE_SHARED le_result_t taf_gptpTime_DeleteRef
(
    taf_gptpTime_Ref_t gptpTimeRef
);

#ifdef __cplusplus
}
#endif

#endif // TAF_PTP_LIB_H

