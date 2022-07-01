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

void tafLocationTest_DR(taf_gnss_DrParams_t* drParams)
{
    LE_INFO("======tafLocationTest_DR function: Configure Dead Reckoning======");
    le_result_t result = taf_gnss_SetDRConfig(drParams);
    switch (result)
    {
        case LE_OK:
            printf("\nSuccessfully set Dead Reckoning!\n");
            break;
        case LE_FAULT:
            printf("\nFailed to set Dead Reckoning. See logs for details\n");
            break;
        case LE_BAD_PARAMETER:
            printf("\nFailed to set Dead Reckoning, incompatible bit mask\n");
            break;
        case LE_BUSY:
            printf("\nFailed to set Dead Reckoning, service is busy\n");
            break;
        case LE_TIMEOUT:
            printf("\nFailed to set Dead Reckoning, timeout error\n");
            break;
        case LE_NOT_PERMITTED:
            printf("\nGNSS is not in ready state!\n");
            break;
        default:
            printf("Failed to set Dead Reckoning, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }
    le_mem_Release(drParams);
}
void tafLocationTest_EngineState(int EngineType, int EngState)
{
    LE_INFO("======tafLocationTest_EngineState function: Configure Engine State======");
    le_result_t result = taf_gnss_ConfigureEngineState(EngineType,EngState);
    switch (result)
    {
        case LE_OK:
            printf("\nSuccessfully set Engine State!\n");
            break;
        case LE_FAULT:
            printf("\nFailed to set Engine State. See logs for details\n");
            break;
        case LE_BAD_PARAMETER:
            printf("\nFailed to set Engine State, incompatible bit mask\n");
            break;
        case LE_BUSY:
            printf("\nFailed to set Engine State, service is busy\n");
            break;
        case LE_TIMEOUT:
            printf("\nFailed to set Engine State, timeout error\n");
            break;
        case LE_NOT_PERMITTED:
            printf("\nGNSS is not in active state!\n");
            break;
        default:
            printf("Failed to set Engine State, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }
}

void tafLocationTest_ConfigureRobustLocation(int enable, int enabled911)
{
    LE_INFO("======tafLocationTest_ConfigureRobustLocation Configure Robust Location======");
    le_result_t result = taf_gnss_ConfigureRobustLocation(enable,enabled911);
    switch (result)
    {
        case LE_OK:
            printf("\nSuccessfully configured Robust Location!\n");
            break;
        case LE_FAULT:
            printf("\nFailed to configure Robust Location. See logs for details\n");
            break;
        case LE_BAD_PARAMETER:
            printf("\nFailed to configure Robust Location, incompatible bit mask\n");
            break;
        case LE_BUSY:
            printf("\nFailed to configure Robust Location, service is busy\n");
            break;
        case LE_TIMEOUT:
            printf("\nFailed to configure Robust Location, timeout error\n");
            break;
        case LE_NOT_PERMITTED:
            printf("\nGNSS is not in active state!\n");
            break;
        default:
            printf("Failed to configure Robust Location, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }
}

void tafLocationTest_RobustLocationInformation(void)
{
    LE_INFO("======tafLocationTest_RobustLocationInformation Robust Location Information======");
    uint8_t enable;
    uint8_t enabled911;
    uint8_t majorVersion;
    uint8_t minorVersion;
    le_result_t result = taf_gnss_RobustLocationInformation(&enable,&enabled911, &majorVersion,
            &minorVersion);
    LE_INFO("\n======tafLocationTest_RobustLocationInformation Robust Location Information enable:"
            "====== %d", enable);
    LE_INFO("\n======tafLocationTest_RobustLocationInformation Robust Location Information"
            "enabled911:====== %d", enabled911);
    LE_INFO("\n======tafLocationTest_RobustLocationInformation Robust Location Information"
            "majorVersion:====== %d", majorVersion);
    LE_INFO("\n======tafLocationTest_RobustLocationInformation Robust Location Information"
            "minorVersion:====== %d", minorVersion);
    switch (result)
    {
        case LE_OK:
            printf("\nSuccessfully received Robust Location Information!\n");
            printf("\nRobust Location Information Enable: %d", enable);
            printf("\nRobust Location Information enabled911: %d", enabled911);
            printf("\nRobust Location Information majorVersion number: %d", majorVersion);
            printf("\nRobust Location Information minorVersion number: %d", minorVersion);
            break;
        case LE_FAULT:
            printf("\nFailed to set Engine State. See logs for details\n");
            break;
        case LE_BAD_PARAMETER:
            printf("\nFailed to set Engine State, incompatible bit mask\n");
            break;
        case LE_BUSY:
            printf("\nFailed to set Engine State, service is busy\n");
            break;
        case LE_TIMEOUT:
            printf("\nFailed to set Engine State, timeout error\n");
            break;
        case LE_NOT_PERMITTED:
            printf("\nGNSS is not in active state!\n");
            break;
        default:
            printf("Failed to set Engine State, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }
}

void tafLocationTest_DefaultSecondaryBandConstellations(void)
{
    LE_INFO("======tafLocationTest_EmptySecondaryBandConstellation"
            "Empty Secondary Band Constellation======");
    le_result_t result = taf_gnss_DefaultSecondaryBandConstellations();
    switch (result)
    {
        case LE_OK:
            printf("\nSuccessfully configured Default Secondary Band Constellations!\n");
            break;
        case LE_FAULT:
            printf("\nFailed to configure Default Secondary Band Constellations."
                    "See logs for details\n");
            break;
        case LE_BAD_PARAMETER:
            printf("\nFailed to configure Default Secondary Band Constellations,"
                    "incompatible bit mask\n");
            break;
        case LE_BUSY:
            printf("\nFailed to configure Default Secondary Band Constellations,"
                    "service is busy\n");
            break;
        case LE_TIMEOUT:
            printf("\nFailed to configure Default Secondary Band Constellations,"
                    "timeout error\n");
            break;
        case LE_NOT_PERMITTED:
            printf("\nGNSS is not in ready state!\n");
            break;
        default:
            printf("Failed to configure Default Secondary Band Constellations,error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }
}
void tafLocationTest_RequestSecondaryBandConstellations(void)
{
    LE_INFO("======tafLocationTest_RequestSecondaryBandConstellations"
            "Get Secondary Band Constellations disabled======");
    int32_t constellationSb=0;
    le_result_t result = taf_gnss_RequestSecondaryBandConstellations(&constellationSb);
    printf("Disabled Secondary Band Constellation bit mask = 0x%04X\n", constellationSb);
    switch (result)
    {
        case LE_OK:
            printf("\nSuccesfully retrieved Disabled Secondary Band Constellations!\n");
            if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_GPS-1)))//1st bit
            {
                printf("\nGPS constellation is disabled \n");
            }
            if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_GALILEO-1)))//2nd bit
            {
                printf("\nGALILEO constellation is disabled \n");
            }
            if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_SBAS-1)))//3rd bit
            {
                printf("\nSBAS constellation is disabled \n");
            }
            if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_COMPASS-1)))//4th bit
            {
                printf("\nCOMPASS constellation is disabled \n");
            }
            if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_GLONASS-1))) //5th bit
            {
                printf("\nGLONASS constellation is disabled \n");
            }
            if(constellationSb &(1<<(TAF_GNSS_SB_CONSTELLATION_BDS-1))) //6th bit
            {
                printf("\nBDS constellation is disabled \n");
            }
            if(constellationSb & (1<<(TAF_GNSS_SB_CONSTELLATION_QZSS-1)))//7th bit
            {
                printf("\nQZAS constellation is disabled \n");
            }
            if(constellationSb &(1<<(TAF_GNSS_SB_CONSTELLATION_NAVIC-1)))//8th bit
            {
                printf("\nNAVIC constellation is disabled \n");
            }
            break;
        case LE_FAULT:
            printf("\nFailed to get disabled Secondary Band Constellation."
                    "See logs for details\n");
            break;
        case LE_BAD_PARAMETER:
            printf("\nFailed to get disabled Secondary Band Constellation,"
                    "incompatible bit mask\n");
            break;
        case LE_BUSY:
            printf("\nFailed to get disabled Secondary Band Constellation,"
                    "service is busy\n");
            break;
        case LE_TIMEOUT:
            printf("\nFailed to get disabled Secondary Band Constellation,"
                    "timeout error\n");
            break;
        case LE_NOT_PERMITTED:
            printf("\nGNSS is not in ready state!\n");
            break;
        default:
            printf("Failed to get disabled Secondary Band Constellation, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }
}
void tafLocationTest_DisplayConfigureSecondaryBandConstellations(uint32_t constellationSb)
{
    LE_INFO("======tafLocationTest_DisplaySecondaryBandConstellations"
            "set Secondary Band Constellation to be disabled======");
    le_result_t result = taf_gnss_ConfigureSecondaryBandConstellations(constellationSb);
    switch (result)
    {
        case LE_OK:
            printf("\nSuccesfully configured Secondary Band Constellation to be disabled!\n");
            break;
        case LE_FAULT:
            printf("\nFailed to configure Secondary Band Constellation. See logs for details\n");
            break;
        case LE_BAD_PARAMETER:
            printf("\nFailed to configure Secondary Band Constellation, incompatible bit mask\n");
            break;
        case LE_BUSY:
            printf("\nFailed to configure Secondary Band Constellation, service is busy\n");
            break;
        case LE_TIMEOUT:
            printf("\nFailed to configure Secondary Band Constellation, timeout error\n");
            break;
        case LE_NOT_PERMITTED:
            printf("\nGNSS is not in ready state!\n");
            break;
        default:
            printf("Failed to configure Secondary Band Constellation, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }

}