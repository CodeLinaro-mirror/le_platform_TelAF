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

void DisplayDRUsage(void) {
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

void DisplayEngStateUsage(void) {

    printf("tafLocationUnitTest -> To Configure Engine State\n");
    printf("tafLocationUnitTest -> Enter 1->SPE 2->PPE 3->DRE 4->VPE\n");
    printf("tafLocationUnitTest -> Enter 1 to bring Engine to suspend state\n");
    printf("tafLocationUnitTest -> Enter 2 to bring engine to running state\n");
}

void DisplayConfigureRobustLocationUsage(void) {

    printf("tafLocationUnitTest -> To Configure Robust Location\n");
    printf("tafLocationUnitTest -> Enter 1 -> Enable 0 -> disable\n");
    printf("tafLocationUnitTest -> Enter 1 -> to enable EnableE911 0-> to disable EnableE911\n");
}

void DisplayRobustLocationInformation(void)
{
    printf("tafLocationUnitTest -> To get Robust Location Information\n");
    printf("tafLocationUnitTest -> Enable :1->enable 0-> disable\n");
    printf("tafLocationUnitTest -> EnableE911: 1->enable 0->disable\n");
    printf("tafLocationUnitTest -> Major version number :\n");
    printf("tafLocationUnitTest -> Minor version number :\n");
}

void DisplayDefaultSecondaryBandConstellations(void)
{
    printf("tafLocationUnitTest -> To set Default Secondary Band Constellations\n");
}

void DisplaySecondaryBandConstellations(void)
{
    printf("tafLocationUnitTest -> To Configure Secondary Band Constellations\n");
    printf("Enter the constellations whose secondary bands needs to be disabled \n");
    printf("1->disable 0->default ------> GPS\n");
    printf("1->disable 0->default ------> GALILEO\n");
    printf("1->disable 0->default ------> SBAS\n");
    printf("1->disable 0->default ------> COMPASS\n");
    printf("1->disable 0->default ------> GLONASS\n");
    printf("1->disable 0->default ------> BDS\n");
    printf("1->disable 0->default ------> QZSS\n");
    printf("1->disable 0->default ------> NAVIC\n");
}
void DisplayRequestSecondaryBandConstellations(void)
{
    printf("tafLocationUnitTest -> To Get Secondary band constellation disabled\n");
}

COMPONENT_INIT
{
    bool exitApplication = false;
    const char* paramType = "";
    char *end;
    int NumberOfArgs = le_arg_NumArgs();
    LE_INFO("Total NumberOfArgs: %d", NumberOfArgs);
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

   //DEAD RECKONING
//Command Line Arguement Format :
//app runProc tafLocationUnitTest --exe=tafLocationUnitTest -- dr 1.2 2.3 3.4 180.0 1.0 0.0 1.0 0.0
//dr<dead reckoning> 1.2<rollOffset> 2.3<yawOffset> 3.4<pitchOffset> 180.0<offsetUnc> 1.0<speedFactor> 0.0<speedFactorUnc> 1.0<gyroFactor> 0.0<gyroFactorUnc>
    if(strcmp(paramType,"dr") == 0) {
        DisplayDRUsage();
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
    }

     //ENGINE INTEGRITY RISK
     //app runProc tafLocationUnitTest --exe=tafLocationUnitTest -- EngState 1 1
    else if(strcmp(paramType,"EngState") == 0)
    {
        DisplayEngStateUsage();
        const char* engType = le_arg_GetArg(1);
        if (NULL == engType)
        {
            LE_ERROR("engType is NULL");
            printf("\engType is NULL exiting the application\n");
            exit(EXIT_FAILURE);
        }
       uint32_t engineType = strtoul(engType, &end, 10);

       if ('\0' != end[0])
       {
            printf("Bad engine type: %s\n", engType);
            exit(EXIT_FAILURE);
       }
       const char* engState = le_arg_GetArg(2);
       if (NULL == engState)
       {
            LE_ERROR("engState is NULL");
            printf("\engState is NULL exiting the application\n");
            exit(EXIT_FAILURE);
       }
       uint32_t engineState = strtoul(engState, &end, 10);
       if ('\0' != end[0])
       {
            printf("Bad engine state: %d\n", engineState);
            exit(EXIT_FAILURE);
       }
        tafLocationTest_EngineState((int)engineType,(int)engineState);
        LE_INFO("tafLocationTest_EngineState is called\n");
        exitApplication = true;
    }

    //ROBUST LOCATION
    //app runProc tafLocationUnitTest --exe=tafLocationUnitTest -- robloc 1 1
    else if(strcmp(paramType,"robloc") == 0)
    {
        DisplayConfigureRobustLocationUsage();
        const char* enable = le_arg_GetArg(1);
        if (NULL == enable)
        {
            LE_ERROR("Robust location enable is NULL");
            printf("\nRobust location enable is NULL exiting the application\n");
            exit(EXIT_FAILURE);
        }
       uint32_t enableRobust = strtoul(enable, &end, 10);

       if ('\0' != end[0])
       {
            printf("Bad Robust location enable type: %s\n", enable);
            exit(EXIT_FAILURE);
       }
        const char* enable911 = le_arg_GetArg(2);
        if (NULL == enable911)
        {
            LE_ERROR("Robust location enable911 is NULL");
            printf("\nRobust location enable911 is NULL exiting the application\n");
            exit(EXIT_FAILURE);
        }
       uint32_t enableRobust911 = strtoul(enable911, &end, 10);

       if ('\0' != end[0])
       {
            printf("Bad Robust location enable type: %s\n", enable911);
            exit(EXIT_FAILURE);
       }
        tafLocationTest_ConfigureRobustLocation(enableRobust,enableRobust911);
        LE_INFO("tafLocationTest_ConfigureRobustLocation is called\n");
        exitApplication = true;
    }

    //ROBUST LOCATION INFORMATION
    //app runProc tafLocationUnitTest --exe=tafLocationUnitTest -- roblocInfo
    else if(strcmp(paramType,"roblocInfo") == 0)
    {
        DisplayRobustLocationInformation();
        tafLocationTest_RobustLocationInformation();
        LE_INFO("tafLocationTest_RobustLocationInformation is called\n");
        exitApplication = true;
    }

    //EMPTY/DEFAULT SECONDARY BAND CONSTELLATIONS
    //app runProc tafLocationUnitTest --exe=tafLocationUnitTest -- defaultsbConst
    else if(strcmp(paramType,"defaultsbConst") == 0)
    {
        DisplayDefaultSecondaryBandConstellations();
        tafLocationTest_DefaultSecondaryBandConstellations();
        LE_INFO("tafLocationTest_EmptySecondaryBandConstellation is called\n");
        exitApplication = true;
    }

    //REQUEST SECONDARY BAND CONSTELLATIONS
    //app runProc tafLocationUnitTest --exe=tafLocationUnitTest -- requestsbConst
    else if(strcmp(paramType,"requestsbConst") == 0)
    {
        DisplayRequestSecondaryBandConstellations();
        tafLocationTest_RequestSecondaryBandConstellations();
        LE_INFO("tafLocationTest_RequestSecondaryBandConstellations is called\n");
        exitApplication = true;
    }

   //CONFIGURE SECONDARY BAND CONSTELLATIONS
   //app runProc tafLocationUnitTest --exe=tafLocationUnitTest -- secbandConst 1<GPS> 1<GALILEO> 1<SBAS> 1<COMPASS> 1<GLONASS> 1<BDS> 1<QZSS> 1<QZSS>
   //app runProc tafLocationUnitTest --exe=tafLocationUnitTest -- secbandConst 0<GPS> 0<GALILEO> 0<SBAS> 0<COMPASS> 0<GLONASS> 0<BDS> 0<QZSS> 0<QZSS>
    else if(strcmp(paramType,"secbandConst") == 0)
    {
        DisplaySecondaryBandConstellations();
        uint32_t constellationSb=0;
        const char* secGps = le_arg_GetArg(1);//GPS-1 (to disable enter ->1 or 0 to default)
        if (NULL == secGps)
        {
            LE_ERROR("secGps is NULL");
            printf("\nsecGps is NULL exiting the application\n");
            exit(EXIT_FAILURE);
        }
        uint32_t gpsConst = strtoul(secGps, &end, 10);

        if ('\0' != end[0])
        {
            printf("Bad constellation type: %s\n", secGps);
            exit(EXIT_FAILURE);
        }
        LE_INFO("gpsConst: %d", gpsConst);
        if((gpsConst == 1) || (gpsConst == 0))
        {
            constellationSb |= (gpsConst<<(TAF_GNSS_SB_CONSTELLATION_GPS-1));
        }
        else
        {
            printf("\nInvalid gpsConst input value! enter 1 or 0 for valid ranges,"
                    "exiting the application\n");
            exit(EXIT_FAILURE);
        }
        const char* secGali = le_arg_GetArg(2);//GALILEO-1 (to disable enter ->1 or 0 to default)
        if (NULL == secGali)
        {
            LE_ERROR("secGali is NULL");
            printf("\nsecGali is NULL exiting the application\n");
            exit(EXIT_FAILURE);
        }
        uint32_t galiConst = strtoul(secGali, &end, 10);

        if ('\0' != end[0])
        {
            printf("Bad constellation type: %s\n", secGali);
            exit(EXIT_FAILURE);
        }
        LE_INFO("galiConst: %d", galiConst);
        if((galiConst == 1) || (galiConst == 0))
        {
            constellationSb |= (galiConst<<(TAF_GNSS_SB_CONSTELLATION_GALILEO-1));
        }
        else
        {
            printf("\nInvalid galiConst input value! enter 1 or 0 for valid ranges,"
                    "exiting the application\n");
            exit(EXIT_FAILURE);
        }
        const char* secSbas = le_arg_GetArg(3);//SBAS-1 (to disable enter ->1 or 0 to default)
        if (NULL == secSbas)
        {
            LE_ERROR("secSbas is NULL");
            printf("\nsecSbas is NULL exiting the application\n");
            exit(EXIT_FAILURE);
        }
        uint32_t sbasConst = strtoul(secSbas, &end, 10);

        if ('\0' != end[0])
        {
            printf("Bad constellation type: %s\n", secSbas);
            exit(EXIT_FAILURE);
        }
        LE_INFO("sbasConst: %d", sbasConst);
        if((sbasConst == 1) || (sbasConst == 0))
        {
            constellationSb |= (sbasConst<<(TAF_GNSS_SB_CONSTELLATION_SBAS-1));
        }
        else
        {
            printf("\nInvalid sbasConst input value! enter 1 or 0 for valid ranges,"
                    "exiting the application\n");
            exit(EXIT_FAILURE);
        }
        const char* secComp = le_arg_GetArg(4);//COMPASS-1 (to disable enter ->1 or 0 to default)
        if (NULL == secComp)
        {
            LE_ERROR("secComp is NULL");
            printf("\nsecComp is NULL exiting the application\n");
            exit(EXIT_FAILURE);
        }
        uint32_t compConst = strtoul(secComp, &end, 10);

        if ('\0' != end[0])
        {
            printf("Bad constellation type: %s\n", secComp);
            exit(EXIT_FAILURE);
        }
        LE_INFO("compConst: %d", compConst);
        if((compConst == 1) || (compConst == 0))
        {
            constellationSb |= (compConst<<(TAF_GNSS_SB_CONSTELLATION_COMPASS-1));
        }
        else
        {
            printf("\nInvalid compConst input value! enter 1 or 0 for valid ranges,"
                    "exiting the application\n");
            exit(EXIT_FAILURE);
        }
        const char* secGlo = le_arg_GetArg(5);//GLONASS-1 (to disable enter ->1 or 0 to default)
        if (NULL == secGlo)
        {
            LE_ERROR("secGlo is NULL");
            printf("\nsecGlo is NULL exiting the application\n");
            exit(EXIT_FAILURE);
        }
        uint32_t gloConst = strtoul(secGlo, &end, 10);

        if ('\0' != end[0])
        {
            printf("Bad constellation type: %s\n", secGlo);
            exit(EXIT_FAILURE);
        }
        LE_INFO("gloConst: %d", gloConst);
        if((gloConst == 1) || (gloConst == 0))
        {
            constellationSb |= (gloConst<<(TAF_GNSS_SB_CONSTELLATION_GLONASS-1));
        }
        else
        {
            printf("\nInvalid gloConst input value! enter 1 or 0 for valid ranges,"
                    "exiting the application\n");
            exit(EXIT_FAILURE);
        }
        const char* secBdas = le_arg_GetArg(6);//BDAS-1 (to disable enter ->1 or 0 to default)
        if (NULL == secBdas)
        {
            LE_ERROR("secBdas is NULL");
            printf("\nsecBdas is NULL exiting the application\n");
            exit(EXIT_FAILURE);
        }
        uint32_t bdasConst = strtoul(secBdas, &end, 10);

        if ('\0' != end[0])
        {
            printf("Bad constellation type: %s\n", secBdas);
            exit(EXIT_FAILURE);
        }
        LE_INFO("bdasConst: %d", bdasConst);
        if((bdasConst == 1) || (bdasConst == 0))
        {
            constellationSb |= (bdasConst<<(TAF_GNSS_SB_CONSTELLATION_BDS-1));
        }
        else
        {
            printf("\nInvalid bdasConst input value! enter 1 or 0 for valid ranges,"
                    "exiting the application\n");
            exit(EXIT_FAILURE);
        }
        const char* secQzss = le_arg_GetArg(7);//QZSS-1 (to disable enter ->1 or 0 to default)
        if (NULL == secQzss)
        {
            LE_ERROR("secQzss is NULL");
            printf("\nsecQzss is NULL exiting the application\n");
            exit(EXIT_FAILURE);
        }
        uint32_t qzssConst = strtoul(secQzss, &end, 10);

        if ('\0' != end[0])
        {
            printf("Bad constellation type: %s\n", secQzss);
            exit(EXIT_FAILURE);
        }
        LE_INFO("qzssConst: %d", qzssConst);
        if((qzssConst == 1) || (qzssConst == 0))
        {
            constellationSb |= (qzssConst<<(TAF_GNSS_SB_CONSTELLATION_QZSS-1));
        }
        else
        {
            printf("\nInvalid qzssConst input value! enter 1 or 0 for valid ranges,"
                    "exiting the application\n");
            exit(EXIT_FAILURE);
        }
        const char* secNavic = le_arg_GetArg(8);//NAVIC-1 (to disable enter ->1 or 0 to default)
        if (NULL == secNavic)
        {
            LE_ERROR("secNavic is NULL");
            printf("\nsecNavic is NULL exiting the application\n");
            exit(EXIT_FAILURE);
        }
        uint32_t navicConst = strtoul(secNavic, &end, 10);

        if ('\0' != end[0])
        {
            printf("Bad constellation type: %s\n", secNavic);
            exit(EXIT_FAILURE);
        }
        LE_INFO("navicConst: %d", navicConst);
        if((navicConst == 1) || (navicConst == 0))
        {
            constellationSb |= (navicConst<<(TAF_GNSS_SB_CONSTELLATION_NAVIC-1));
        }
        else
        {
            printf("\nInvalid navicConst input value! enter 1 or 0 for valid ranges,"
                    "exiting the application\n");
            exit(EXIT_FAILURE);
        }
       tafLocationTest_DisplayConfigureSecondaryBandConstellations(constellationSb);
       LE_INFO("tafLocationTest_DisplaySecondaryBandConstellations is called\n");
       exitApplication = true;
    }
    else
    {
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