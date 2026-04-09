/*
* Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"
#include "tafHMS.hpp"

using namespace tafsvc;
using namespace std;


//--------------------------------------------------------------------------------------------------
/**
 ** Gets the total cpu usage from " /proc/stat ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetCpuLoad
(
    double* cpuCurrentLoadPtr
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetCpuLoad(cpuCurrentLoadPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 ** Gets number of CPU core from " /proc/cpuinfo ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_hms_GetCpuCoreNum
(
    void
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetCpuCoreNum();
}

//--------------------------------------------------------------------------------------------------
/**
 ** Gets the CPU usage of each core from " /proc/stat ".
 ** The API provides total CPU usage of each core ranging from 0 to 100 %.
 ** The coreID range can be 0 - (cpuCoreNum-1), the result value can be LE_OK.
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetIndvCoreUsage
(
    uint32_t coreID,
        ///< [IN] Core ID
    double* cpuUsagePtr
        ///< [OUT] cpuUsage
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetIndvCoreUsage(coreID, cpuUsagePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 ** Gets the meminfo value from " /proc/meminfo ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetRamMemInfo
(
    uint32_t* ramTotalMemPtr,
    uint32_t* ramUsedMemPtr,
    uint32_t* ramFreeMemPtr
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetRamMemInfo(ramTotalMemPtr, ramUsedMemPtr, ramFreeMemPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 ** Gets the list of available UBI device information.
 **
 ** @return
 *  - NULL                            No information found.
 *  - taf_hms_ubiDevInfoListRef    The UBI device Info list object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_UbiDevInfoListRef_t taf_hms_GetUbiDevInfoList
(
    void
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetUbiDevInfoList();
}
//--------------------------------------------------------------------------------------------------
/**
 ** Deletes the UbiDevInfoList list retrieved with taf_hms_GetUbiDevInfoList().
 **
 ** @return
 **  - LE_BAD_PARAMETER -- Bad parameters.
 **  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_DeleteUbiDevInfoList
(
    taf_hms_UbiDevInfoListRef_t ubiDevInfoListRef
        ///< [IN] UBI device list reference
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.DeleteUbiDevInfoList(ubiDevInfoListRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the first UBI device Info object reference in the list of the
 * ubiDevInfoList retrieved with taf_hms_GetUBIDevInfoList().
 *
 * @return
 *  - NULL                          No information found.
 *  - taf_hms_ubiDevInfoListRef      The UBI device Info object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_UbiDevInfoRef_t taf_hms_GetFirstUbiDevInfo
(
    taf_hms_UbiDevInfoListRef_t ubiDevInfoListRef
        ///< [IN] UBI device list reference
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetFirstUbiDevInfo(ubiDevInfoListRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the next UBI device Info object reference in the list of the
 * UBIInfoList retrieved with taf_hms_GetUBIInfoList().
 *
 * @return
 *  - NULL                          No information found.
 *  - taf_hms_ubiDevInfoListRef      The UBI device Info object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_UbiDevInfoRef_t taf_hms_GetNextUbiDevInfo
(
    taf_hms_UbiDevInfoListRef_t ubiDevInfoListRef
        ///< [IN] UBI device list reference
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetNextUbiDevInfo(ubiDevInfoListRef);
}
//--------------------------------------------------------------------------------------------------
/**
 ** Gets UBI device ID
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetUbiDevId
(
    taf_hms_UbiDevInfoRef_t ubiDevInfoRef,
        ///< [IN] UBI device Info reference.
    uint32_t* ubiDevIdPtr
        ///< [OUT] UBI device ID
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetUbiDevId(ubiDevInfoRef, ubiDevIdPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 ** Get UBI information for erase count from " /sys/class/ubi/ubi%d/ ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetUbiDevMaxEraseCnt
(
    taf_hms_UbiDevInfoRef_t ubiDevInfoRef,
        ///< [IN] UBI device Info reference.
    uint32_t* ubiEraseCntPtr
        ///< [OUT] Erase count.
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetUbiDevMaxEraseCnt(ubiDevInfoRef, ubiEraseCntPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 ** Get UBI information for bad block count from " /sys/class/ubi/ubi%d/ ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetUbiDevBadBlkCnt
(
    taf_hms_UbiDevInfoRef_t ubiDevInfoRef,
        ///< [IN] UBI device Info reference.
    uint32_t* ubiBbCntPtr
        ///< [OUT] Bad Block count.
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetUbiDevBadBlkCnt(ubiDevInfoRef, ubiBbCntPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the first UBI volume Info object reference in the list of the
 * ubiVolInfoList retrieved with taf_hms_GetUbiVolInfoList().
 *
 * @return
 *  - NULL                          No information found.
 *  - taf_hms_UbiVolInfoListRef      The UBI device Info object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_UbiVolInfoRef_t taf_hms_GetFirstUbiVolInfo
(
    taf_hms_UbiDevInfoRef_t ubiDevInfoRef
        ///< [IN] UBI device list reference
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetFirstUbiVolInfo(ubiDevInfoRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the next UBI volume Info object reference in the list of the
 * ubiVolInfoList retrieved with taf_hms_GetUbiVolInfoList().
 *
 * @return
 *  - NULL                          No information found.
 *  - taf_hms_UbiVolInfoListRef      The UBI volume Info object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_UbiVolInfoRef_t taf_hms_GetNextUbiVolInfo
(
    taf_hms_UbiDevInfoRef_t ubiDevInfoRef
        ///< [IN] UBI device list reference
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetNextUbiVolInfo(ubiDevInfoRef);
}
//--------------------------------------------------------------------------------------------------
/**
 ** Gets UBI volume ID.
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetUbiVolId
(
    taf_hms_UbiVolInfoRef_t ubiVolInfoRef,
        ///< [IN]
    uint32_t* ubiVolIdPtr
        ///< [OUT]
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetUbiVolId(ubiVolInfoRef, ubiVolIdPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 ** Get name of UBI volume.
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetUbiVolName
(
    taf_hms_UbiVolInfoRef_t ubiVolInfoRef,
        ///< [IN] UBI device Info reference.
    char* ubiVolName,
        ///< [OUT] UBI volume name.
    size_t ubiVolNameSize
        ///< [IN]
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetUbiVolName(ubiVolInfoRef, ubiVolName, ubiVolNameSize);
}
//--------------------------------------------------------------------------------------------------
/**
 ** Get size of UBI volume.
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetUbiVolSize
(
    taf_hms_UbiVolInfoRef_t ubiVolInfoRef,
        ///< [IN] UBI device Info reference.
    uint32_t* ubiVolSizePtr
        ///< [OUT] UBI volume size.
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetUbiVolSize(ubiVolInfoRef, ubiVolSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 ** Gets the list of available MTD Node.
 **
 ** @return
 *  - NULL                            No information found.
 *  - taf_hms_mtdDevInfoListRef    The mtdInfo list object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_MtdDevInfoListRef_t taf_hms_GetMtdDevInfoList
(
    void
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetMtdDevInfoList();
}
//--------------------------------------------------------------------------------------------------
/**
 ** Deletes the MtdInfoList list retrieved with taf_hms_GetMtdDevInfoList().
 **
 ** @return
 **  - LE_BAD_PARAMETER -- Bad parameters.
 **  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_DeleteMtdDevInfoList
(
    taf_hms_MtdDevInfoListRef_t mtdDevInfoListRef
        ///< [IN] MTD device list reference
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.DeleteMtdDevInfoList(mtdDevInfoListRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the first mtdInfo object reference in the list of the
 * mtdInfoList retrieved with taf_hms_GetMtdDevInfoList().
 *
 * @return
 *  - NULL                          No information found.
 *  - taf_hms_mtdDevInfoListRef      The mtdInfo object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_MtdDevInfoRef_t taf_hms_GetFirstMtdDevInfo
(
    taf_hms_MtdDevInfoListRef_t mtdDevInfoListRef
        ///< [IN] MTD node list reference
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetFirstMtdDevInfo(mtdDevInfoListRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the next mtdInfo object reference in the list of the
 * mtdInfoList retrieved with taf_hms_GetMtdDevInfoList().
 *
 * @return
 *  - NULL                          No information found.
 *  - taf_hms_mtdDevInfoListRef      The mtdInfo object reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_MtdDevInfoRef_t taf_hms_GetNextMtdDevInfo
(
    taf_hms_MtdDevInfoListRef_t mtdDevInfoListRef
        ///< [IN] MTD node list reference
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetNextMtdDevInfo(mtdDevInfoListRef);
}

//--------------------------------------------------------------------------------------------------
/**
 ** Get MTD information for name from " /sys/class/mtd/mtd%d/ ".
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetMtdDevName
(
    taf_hms_MtdDevInfoRef_t mtdDevInfoRef,
        ///< [IN] UBIInfo reference.
    char* mtdName,
        ///< [OUT] MTD Name.
    size_t mtdNameSize
        ///< [IN]
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetMtdDevName(mtdDevInfoRef, mtdName, mtdNameSize);
}


//--------------------------------------------------------------------------------------------------
/**
 ** Get MTD information for block size.
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetMtdDevBlkSize
(
    taf_hms_MtdDevInfoRef_t mtdDevInfoRef,
        ///< [IN] UBIInfo reference.
    uint32_t* mtdBlkSizePtr
        ///< [OUT] Bad Block count.
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetMtdDevBlkSize(mtdDevInfoRef, mtdBlkSizePtr);
}
//--------------------------------------------------------------------------------------------------
/**
 ** Gets MTD information for block Id.
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetMtdDevId
(
    taf_hms_MtdDevInfoRef_t mtdDevInfoRef,
        ///< [IN] UBIInfo reference.
    uint32_t* mtdDevIdPtr
        ///< [OUT] Bad Block count.
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetMtdDevId(mtdDevInfoRef, mtdDevIdPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 ** Gets MTD information for block count.
 **
 ** @return
 ** - LE_FAULT         Failed.
 ** - LE_OK            Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetMtdDevBlkCnt
(
    taf_hms_MtdDevInfoRef_t mtdDevInfoRef,
        ///< [IN] UBIInfo reference.
    uint32_t* mtdBlkCntPtr
        ///< [OUT] Bad Block count.
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.GetMtdDevBlkCnt(mtdDevInfoRef, mtdBlkCntPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler for the modem status change and its reference object.
 *
 *
 * PARAMETERS      [IN] Handler function pointer.
 *                 [IN] Handler context.
 *
 * @return
 *  - taf_hms_ModemEventHandlerRef_t Handler reference.
 */
//--------------------------------------------------------------------------------------------------
taf_hms_ModemEvtHandlerRef_t taf_hms_AddModemEvtHandler
(
    taf_hms_ModemEvtBitmask_t     reqEventBits,
    taf_hms_ModemEvtHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.AddModemEvtHandler(reqEventBits, handlerPtr, contextPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for the modem status change.
 *
 *
 * PARAMETERS      [IN] Handler reference.

 */
//--------------------------------------------------------------------------------------------------
void taf_hms_RemoveModemEvtHandler(taf_hms_ModemEvtHandlerRef_t handlerRef)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.RemoveModemEvtHandler(handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove event reference for the modem status change.
 *
 *
 * PARAMETERS      [IN] event reference.
 *
 * @return
 * - LE_FAULT         Failed.
 * - LE_OK            Succeeded.

 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_ReleaseModemEvt(taf_hms_ModemEventRef_t eventRef)
{
    auto &hms = taf_Hms::GetInstance();
    return hms.ReleaseModemEvt(eventRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the last reset information reason
 *
 * @return
 *      - LE_OK          on success
 *      - LE_UNSUPPORTED if it is not supported by the platform
 *        LE_OVERFLOW    specific reset information length exceeds the maximum length.
 *      - LE_FAULT       for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_hms_GetResetInformation
(
    taf_hms_Reset_t* resetPtr,       ///< [OUT] Reset information
    char* resetSpecificInfoStr,      ///< [OUT] Reset specific information
    size_t resetSpecificInfoStrSize  ///< [IN]
)
{
    auto &hms = taf_Hms::GetInstance();
    if (resetPtr == NULL)
    {
        LE_ERROR("resetPtr is NULL !");
        return LE_FAULT;
    }
    if (resetSpecificInfoStr == NULL)
    {
        LE_ERROR("resetSpecificInfoStr is NULL !");
        return LE_FAULT;
    }

    return hms.GetResetInformation(resetPtr, resetSpecificInfoStr, resetSpecificInfoStrSize);
}

/**
 * The initialization of TelAF Health Monitor component.
*/
COMPONENT_INIT
{
    LE_INFO("TelAF Health Monitor Service init Started...");
    auto &hms = taf_Hms::GetInstance();
    hms.Init();
    LE_INFO("TelAF Health Monitor Service init completed and service advertised...");
}

