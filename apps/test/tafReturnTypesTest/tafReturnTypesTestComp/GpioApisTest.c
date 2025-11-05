/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Function to validate GPIO Service APIs.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) void gpioRetTest_RunApis
(
    void
)
{
    le_result_t result;
    #define TAF_GPIO_PIN_NAME_MAX_BYTE 32
    LE_TEST_INFO("gpioRetTest_RunApis");

    //1.taf_gpio_GetName - LE_BAD_PARAMETER  scenario
    result = taf_gpio_GetName(1,NULL, TAF_GPIO_PIN_NAME_MAX_BYTE);
    LE_TEST_OK(result==LE_BAD_PARAMETER ,"taf_gpio_GetName-LE_BAD_PARAMETER ");

    //2.taf_gpio_GetName - LE_OVERFLOW  scenario
    char pinName[TAF_GPIO_PIN_NAME_MAX_BYTE] = {};
    result = taf_gpio_GetName(1,pinName, TAF_GPIO_PIN_NAME_MAX_BYTE+1);
    LE_TEST_OK(result==LE_OVERFLOW ,"taf_gpio_GetName-LE_OVERFLOW ");

    //3.taf_gpio_GetName -  LE_OUT_OF_RANGE  scenario
    result = taf_gpio_GetName(-1,NULL, TAF_GPIO_PIN_NAME_MAX_BYTE);
    LE_TEST_OK(result== LE_OUT_OF_RANGE ,"taf_gpio_GetName- LE_OUT_OF_RANGE");
}