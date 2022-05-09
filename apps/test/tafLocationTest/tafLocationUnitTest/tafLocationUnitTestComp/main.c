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
#include "main.h"

static le_mem_PoolRef_t DrFramePool = NULL;

void DisplayAppUsage(void) {
    printf("tafLocationUnitTest -> To Configure DR Parameters\n");
    printf("rollOffset: valid ranges -180.0 to 180.0\n");
    printf("yawOffset:  valid ranges -180.0 to 180.0\n");
    printf("pitchOffset: valid ranges -180.0 to 180.0\n");
    printf("offsetUnc:  valid ranges from -180.0 to 180.0\n");
    printf("speedFactor: valid ranges from 0.9 to 1.0\n");
    printf("speedFactorUnc: valid ranges from 0.0 to 0.1\n");
    printf("gyroFactor: valid ranges from 0.9 to 1.1\n");
    printf("gyroFactorUnc: valid ranges from 0.0 to 0.1\n");
}


COMPONENT_INIT
{
    bool exitApplication = false;
    const char* paramType = "";
    char *end;

//Command Line Arguement Format :
//app runProc tafLocationUnitTest --exe=tafLocationUnitTest -- dr 1.2 2.3 3.4 180.0 1.0 0.0 1.0 0.0
//dr<dead reckoning> 1.2<rollOffset> 2.3<yawOffset> 3.4<pitchOffset> 180.0<offsetUnc> 1.0<speedFactor> 0.0<speedFactorUnc> 1.0<gyroFactor> 0.0<gyroFactorUnc>

    int NumberOfArgs = le_arg_NumArgs();
    LE_INFO("====== Strt Dead Reckoning test ======");
    if (NumberOfArgs >= 1) {
        paramType = le_arg_GetArg(0);
        if (NULL == paramType) {
            LE_ERROR("paramType is NULL");
            exit(EXIT_FAILURE);
        }
    }
    else //if no arguements passed in the command line argument
    {
        LE_ERROR("NumberOfArgs: %d", NumberOfArgs);
        printf("No Parameters passed, exiting the application\n");
        exit(EXIT_FAILURE);
    }

    DisplayAppUsage();
    if(strcmp(paramType,"dr") == 0) {
        taf_gnss_DrParams_t *drParamsPtr;
        DrFramePool = le_mem_CreatePool("DrframePool", sizeof(taf_gnss_DrParams_t));
        drParamsPtr = (taf_gnss_DrParams_t*) le_mem_ForceAlloc(DrFramePool);
        const char* rollOffset = le_arg_GetArg(1);
        if (NULL == rollOffset)
        {
            LE_ERROR("rollOffset is NULL");
            printf("\nrollOffset is NULL exiting the application\n");
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        double roll_Offset = strtod(rollOffset, &end);
        if ('\0' != end[0])
        {
           printf("Bad rollOffset: %s\n", rollOffset);
           le_mem_Release(drParamsPtr);
           exit(EXIT_FAILURE);
        }
        drParamsPtr->rollOffset = roll_Offset;
        LE_INFO("drParamsPtr->rollOffset : %lf", drParamsPtr->rollOffset);

        const char* yawOffset = le_arg_GetArg(2);
        if (NULL == yawOffset)
        {
            LE_ERROR("yawOffset is NULL");
            printf("\nyawOffset is NULL exiting the application\n");
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        double yaw_Offset = strtod(yawOffset, &end);
        if ('\0' != end[0])
        {
            printf("Bad yawOffset: %s\n", yawOffset);
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        drParamsPtr->yawOffset = yaw_Offset ;
        LE_INFO("drParamsPtr->yawOffset : %lf", drParamsPtr->yawOffset);

        const char* pitchOffset = le_arg_GetArg(3);
        if (NULL == pitchOffset)
        {
            LE_ERROR("pitchOffset is NULL");
            printf("\npitchOffset is NULL exiting the application\n");
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        double pitch_Offset = strtod(pitchOffset, &end);
        if ('\0' != end[0])
        {
            printf("Bad pitchOffset: %s\n", pitchOffset);
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        drParamsPtr->pitchOffset = pitch_Offset;
        LE_INFO("drParamsPtr->pitchOffse : %lf", drParamsPtr->pitchOffset);

        const char* offsetUnc = le_arg_GetArg(4);
        if (NULL == offsetUnc)
        {
            LE_ERROR("offsetUnc is NULL");
            printf("\noffsetUnc is NULL exiting the application\n");
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        double Offset_Unc = strtod(offsetUnc, &end);
        if ('\0' != end[0])
        {
            printf("Bad offsetUnc: %s\n", offsetUnc);
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        drParamsPtr->offsetUnc = Offset_Unc;
        LE_INFO("drParamsPtr->offsetUnc : %lf", drParamsPtr->offsetUnc);

        const char* speedFactor = le_arg_GetArg(5);
        if (NULL == speedFactor)
        {
            LE_ERROR("speedFactor is NULL");
            printf("\nspeedFactor is NULL exiting the application\n");
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        double speed_Factor = strtod(speedFactor, &end);
        if ('\0' != end[0])
        {
            printf("Bad speedFactor: %s\n", speedFactor);
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        drParamsPtr->speedFactor = speed_Factor;
        LE_INFO("drParamsPtr->speedFactor : %lf", drParamsPtr->speedFactor);

        const char* speedFactorUnc = le_arg_GetArg(6);
        if (NULL == speedFactorUnc)
        {
            LE_ERROR("speedFactorUnc is NULL");
            printf("\nspeedFactorUnc is NULL exiting the application\n");
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        double speed_FactorUnc = strtod(speedFactorUnc, &end);
        if ('\0' != end[0])
        {
            printf("Bad speedFactorUnc: %s\n", speedFactorUnc);
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        drParamsPtr->speedFactorUnc = speed_FactorUnc;
        LE_INFO("drParamsPtr->speedFactorUnc : %lf", drParamsPtr->speedFactorUnc);

        const char* gyroFactor = le_arg_GetArg(7);
        if (NULL == gyroFactor)
        {
            LE_ERROR("gyroFactor is NULL");
            printf("\ngyroFactor is NULL exiting the application\n");
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        double gyro_factor = strtod(gyroFactor, &end);
        if ('\0' != end[0])
        {
            printf("Bad gyroFactor: %s\n", gyroFactor);
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        drParamsPtr->gyroFactor = gyro_factor;
        LE_INFO("drParamsPtr->gyroFactor : %lf", drParamsPtr->gyroFactor);

        const char* gyroFactorUnc = le_arg_GetArg(8);
        if (NULL == gyroFactorUnc)
        {
            LE_ERROR("gyroFactorUnc is NULL");
            printf("\ngyroFactorUnc is NULL exiting the application\n");
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        double gyro_factorUnc = strtod(gyroFactorUnc, &end);
        if ('\0' != end[0])
        {
            printf("Bad gyroFactorUnc: %s\n", gyroFactorUnc);
            le_mem_Release(drParamsPtr);
            exit(EXIT_FAILURE);
        }
        drParamsPtr->gyroFactorUnc = gyro_factorUnc;
        LE_INFO("drParamsPtr->gyroFactorUnc : %lf", drParamsPtr->gyroFactorUnc);
        LE_INFO("tafLocationTest_DR is called\n");
        tafLocationTest_DR(drParamsPtr);
        exitApplication = true;
    } else {
        LE_INFO("header is not matching, exiting application\n");
        exit(EXIT_FAILURE);
    }

    if (exitApplication)
    {
        LE_INFO("Exit tafLocationUnitTest App\n");
        printf("\n==== Application exits =====\n");
        exit(EXIT_SUCCESS);
    }
}