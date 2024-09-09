/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <mtd/ubi-user.h>

#include "legato.h"
#include "interfaces.h"

#include "tafPiUA.h"

#include "uaPlugin.h"

extern ua_InfoTab_t TAF_HAL_INFO_TAB;

#define UA_SESSION_NUM 1

//--------------------------------------------------------------------------------------------------
/**
 * Update session.
 */
//--------------------------------------------------------------------------------------------------
taf_pi_ua_Session session;

//--------------------------------------------------------------------------------------------------
/**
 * Update session reference.
 */
//--------------------------------------------------------------------------------------------------
taf_pi_ua_SessionRef_t sessionRef;

//--------------------------------------------------------------------------------------------------
/**
 * Update session reference map.
 */
//--------------------------------------------------------------------------------------------------
le_ref_MapRef_t sessionRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Update command event.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t updateCmdEv;

//--------------------------------------------------------------------------------------------------
/**
 * Update session process event.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t sessProcEv;

//--------------------------------------------------------------------------------------------------
/**
 * Update status.
 */
//--------------------------------------------------------------------------------------------------
taf_pi_ua_Status_t updateStatus = TAF_PI_UA_STATUS_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Estimated total time for update.
 */
//--------------------------------------------------------------------------------------------------
int totalTime = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Elpased time for update.
 */
//--------------------------------------------------------------------------------------------------
int elapsedTime = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Percentage for update.
 */
//--------------------------------------------------------------------------------------------------
int updatePercent = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Update timer reference.
 */
//--------------------------------------------------------------------------------------------------
le_timer_Ref_t updateTimerRef;

//--------------------------------------------------------------------------------------------------
/**
 * Static map for update session reference.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(sessionRefMap, UA_SESSION_NUM);

//--------------------------------------------------------------------------------------------------
/**
 * Get update agent information table.
 *
 * @return
 * - NULL   -- Failed.
 * - Others -- UA module information.
 */
//--------------------------------------------------------------------------------------------------
void* taf_hal_GetModInf()
{
    LE_INFO("Get UA module information table.");

    return &(TAF_HAL_INFO_TAB.uaInf);
}

//--------------------------------------------------------------------------------------------------
/**
 * Update timer handler.
 */
//--------------------------------------------------------------------------------------------------
void UpdateTimerHandler
(
    le_timer_Ref_t timerRef ///< [IN] Download timer reference.
)
{
    elapsedTime++;
    int percent = 0;

    switch (updateStatus)
    {
        case TAF_PI_UA_STATUS_INIT:
            LE_INFO("Update initiated.");
            break;
        case TAF_PI_UA_STATUS_UPDATING:
            if (totalTime != 0)
            {
                LE_INFO("Updating....");
                percent = elapsedTime * 100 / totalTime;
                if (percent > 100)
                {
                    updatePercent = 100;
                    LE_WARN("Wait more time to complete the update.");
                }
                else
                {
                    updatePercent = percent;
                }
            }
            break;
        case TAF_PI_UA_STATUS_FINISH:
            LE_INFO("Update finished.");
            percent = 100;
            le_timer_Stop(timerRef);
            break;
        case TAF_PI_UA_STATUS_PAUSED:
            LE_INFO("Update paused.");
            le_timer_Stop(timerRef);
            break;
        case TAF_PI_UA_STATUS_ERROR:
        default:
            LE_INFO("Update error.");
            le_timer_Stop(timerRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Send command.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SendCmd
(
    const char* cmd ///< [IN] Command.
)
{
    FILE* pfp = popen(cmd, "w");
    if (pfp == NULL)
    {
        LE_ERROR("send %s command failed.", cmd);
        return LE_FAULT;
    }
    pclose(pfp);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open UBI volume.
 */
//--------------------------------------------------------------------------------------------------
le_result_t UbiOpen
(
    const char* volName, ///< [IN] UBI volume name.
    int* fd,             ///< [OUT] UBI file descriptor.
    uint32_t* volSize    ///< [OUT] UBI volume size.
)
{
    LE_INFO("Open volume %s ...", volName);
    // 1. Ger volume count.
    int volCount = 0;
    FILE* fp = fopen(UBI_VOL_COUNT_PATH, "r");
    if (fp == NULL)
    {
        LE_ERROR("Fail to get volume count.");
        return LE_FAULT;
    }
    int rc = fscanf(fp, "%d\n", &volCount);
    fclose(fp);

    // 2. Get volume ID.
    int id = 0;
    while (id < volCount)
    {
        char namePath[FILE_PATH_MAX_BYTES] = {0};
        char volume[VOL_NAME_MAX_BYTES] = {0};
        snprintf(namePath, sizeof(namePath) - 1, UBI_VOL_NAME_PATH, id);
        fp = fopen(namePath, "r");
        if (fp == NULL)
        {
            LE_ERROR("Fail to get %d volume name.", id);
            return LE_FAULT;
        }
        rc = fscanf(fp, "%s\n", volume);
        fclose(fp);
        if (strncmp(volName, volume, strlen(volName)) == 0)
        {
            char vsPath[FILE_PATH_MAX_BYTES] = {0};
            snprintf(vsPath, sizeof(vsPath) - 1, UBI_VOL_SIZE_PATH, id);
            fp = fopen(vsPath, "r");
            if (fp == NULL)
            {
                LE_ERROR("Fail to get %d volume name.", id);
                return LE_FAULT;
            }
            rc = fscanf(fp, "%d\n", volSize);
            LE_INFO("volName %s size : %d", volName, *volSize);
            LE_DEBUG("RC : %d", rc);
            fclose(fp);
            break;
        }

        id++;
    }
    if (id >= volCount)
    {
        LE_ERROR("Fail to get volume ID for %s.", volName);
        return LE_FAULT;
    }

    // 3. Get volume FD.
    char devPath[FILE_PATH_MAX_BYTES] = {0};
    snprintf(devPath, sizeof(devPath) - 1, UBI_VOL_DEV_PATH, id);
    *fd = open(devPath, O_RDWR);
    if (*fd < 0)
    {
        LE_ERROR("Fail to open %s.", devPath);
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read UBI volume.
 */
//--------------------------------------------------------------------------------------------------
le_result_t UbiRead
(
    const char* file, ///< [IN] UBI source file.
    int fd,           ///< [IN] UBI file descriptor.
    uint32_t volSize  ///< [IN] UBI volume size.
)
{
    char buffer[PAGE_SIZE];
    lseek(fd, 0, SEEK_SET);

    FILE *fp = fopen(file, "ab+");
    if (fp == NULL)
    {
        LE_ERROR("Error opening file %s.", file);
        return LE_FAULT;
    }

    int i;
    int loop = volSize / PAGE_SIZE;
    LE_INFO("Creating %d pages for %s.", loop, file);
    for (i = 0; i < loop; i++)
    {
        int ret = read(fd, buffer, PAGE_SIZE);
        if (ret < 0)
        {
            LE_ERROR("Read UBI failed, ret = %d errno=%d", ret, errno);
            fclose(fp);
            return LE_FAULT;
        }

        ret = fwrite(buffer, 1, PAGE_SIZE, fp);
        if (ret != PAGE_SIZE)
        {
            LE_ERROR("Write UBI data page error, ret = %d", ret);
            fclose(fp);
            return LE_FAULT;
        }
    }

   (void)SendCmd(SYNC_FS);

    fclose(fp);
    close(fd);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write UBI volume.
 */
//--------------------------------------------------------------------------------------------------
le_result_t UbiWrite
(
    int fd,           ///< [IN] UBI file descriptor.
    uint32_t volSize, ///< [IN] UBI volume size.
    const char* file  ///< [IN] UBI target file.
)
{
    char buffer[PAGE_SIZE];
    lseek(fd, 0, SEEK_SET);

    // 1. Set write size.
    if (ioctl(fd, UBI_IOCVOLUP, &volSize))
    {
        LE_ERROR("Fail to set write size, fd = %d.", fd);
        return LE_FAULT;
    }

    FILE *fp = NULL;
    fp = fopen(file, "r");
    if (fp == NULL)
    {
        LE_ERROR("Fail to get output %s from diff engine.", file);
        return LE_FAULT;
    }

    // 2. Start writing.
    int i;
    int loop = volSize / PAGE_SIZE;
    LE_INFO("Creating %d pages for %s.", loop, file);
    for (i = 0; i < loop; i++)
    {
        int ret = fread(buffer, 1, PAGE_SIZE, fp);
        if (ret != PAGE_SIZE)
        {
            LE_ERROR("Read UBI data page error, ret = %d", ret);
            fclose(fp);
            return LE_FAULT;
        }

        ret = write(fd, buffer, PAGE_SIZE);
        if (ret <= 0)
        {
            LE_ERROR("Fail to write fd = %d.", fd);
            fclose(fp);
            return LE_FAULT;
        }
    }

   (void)SendCmd(SYNC_FS);

    fclose(fp);
    close(fd);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize UBI volume information table.
 */
//--------------------------------------------------------------------------------------------------
void InitUbiInfoTab
(
    const char* volName,             ///< [IN] Source UBI volume name.
    bool bootA,                      ///< [IN] If boot on system A.
    taf_pi_ua_PartitionInfo* infoPtr ///< [OUT] Partition information.
)
{
    if (bootA)
    {
        snprintf(infoPtr->srcPartName, sizeof(infoPtr->srcPartName) - 1, "%s_%s", volName, "a");
        snprintf(infoPtr->tgtPartName, sizeof(infoPtr->tgtPartName) - 1, "%s_%s", volName, "b");
        snprintf(infoPtr->srcPath, sizeof(infoPtr->srcPath) - 1, "/data/%s_%s", volName, "a");
        snprintf(infoPtr->tgtPath, sizeof(infoPtr->tgtPath) - 1, "/data/%s_%s", volName, "b");
    }
    else
    {
        snprintf(infoPtr->srcPartName, sizeof(infoPtr->srcPartName) - 1, "%s_%s", volName, "b");
        snprintf(infoPtr->tgtPartName, sizeof(infoPtr->tgtPartName) - 1, "%s_%s", volName, "a");
        snprintf(infoPtr->srcPath, sizeof(infoPtr->srcPath) - 1, "/data/%s_%s", volName, "b");
        snprintf(infoPtr->tgtPath, sizeof(infoPtr->tgtPath) - 1, "/data/%s_%s", volName, "a");
    }

    snprintf(infoPtr->patchPath, sizeof(infoPtr->patchPath) - 1, "/data/%s.patch", volName);
    if (access(infoPtr->patchPath, 0) == 0)
    {
        LE_INFO("%s is to be updated.", infoPtr->patchPath);
        infoPtr->validPatch = true;
    }
    else
    {
        LE_INFO("%s is not to be updated.", infoPtr->patchPath);
        infoPtr->validPatch = false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Apply UBI patch.
 */
//--------------------------------------------------------------------------------------------------
le_result_t ApplyUbiPatch
(
    taf_pi_ua_PartitionInfo* infoPtr ///< [IN] Partition information.
)
{
    le_result_t result = LE_OK;
    int rc = 0;

    if (infoPtr->validPatch)
    {
        // 1. Read data from source partittions.
        result = UbiOpen(infoPtr->srcPartName, &infoPtr->srcFd, &infoPtr->volSize);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to open source %s volume.", infoPtr->srcPartName);
            return LE_FAULT;
        }

        UbiRead(infoPtr->srcPath, infoPtr->srcFd, infoPtr->volSize);

        // 2. Apply patch on the source partittions.
        char deltaCmd[STR_MAX_BYTES];
        snprintf(deltaCmd, sizeof(deltaCmd), PERFORM_DELTA, infoPtr->srcPath, infoPtr->patchPath,
            infoPtr->tgtPath);
        if (SendCmd(deltaCmd) != LE_OK)
        {
             LE_ERROR("Fail to perform delta.");
             return LE_FAULT;
        }

        // 3. Access on unpacked file.
        if (SendCmd(SYNC_FS) != LE_OK)
        {
             LE_ERROR("Fail to sync file system.");
             return LE_FAULT;
        }

        // 4. Flash the target partitions.
        result = UbiOpen(infoPtr->tgtPartName, &infoPtr->tgtFd, &infoPtr->volSize);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to open target %s volume.", infoPtr->tgtPartName);
            return LE_FAULT;
        }

        result = UbiWrite(infoPtr->tgtFd, infoPtr->volSize, infoPtr->tgtPath);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to write %s volume.", infoPtr->tgtPartName);
            return LE_FAULT;
        }

        // 5. Clean up temporary files.
        rc = unlink(infoPtr->srcPath);
        rc = unlink(infoPtr->tgtPath);
        rc = unlink(infoPtr->patchPath);
        LE_DEBUG("RC : %d.", rc);
    }
    else
    {
        LE_INFO("No patch to be applied on %s volume.", infoPtr->srcPartName);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Unpack package.
 */
//--------------------------------------------------------------------------------------------------
void UnpackPackage
(
    char* filePath ///< [IN] Update package file path.
)
{
    char unpackCmd[STR_MAX_BYTES];
    snprintf(unpackCmd, sizeof(unpackCmd), UNZIP_TO_DATA, filePath);
    if (SendCmd(unpackCmd) != LE_OK)
    {
        LE_ERROR("Unpack package %s failed.", filePath);
        updateStatus = TAF_PI_UA_STATUS_ERROR;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get boot slot.
 */
//--------------------------------------------------------------------------------------------------
void GetBootSlot
(
    bool* bootA ///< [OUT] If boot on system A.
)
{
    char res[STR_MAX_BYTES];
    FILE* fp = popen(GET_ACTIVE_SLOT, "r");
    if (fp == NULL || fgets(res, sizeof(res), fp) == NULL)
    {
        LE_ERROR("Fail to get boot slot.");
        updateStatus = TAF_PI_UA_STATUS_ERROR;
        return;
    }

    if (strncmp(res, "_a", 2) == 0)
    {
        LE_INFO("Boot on A slot.");
        *bootA = true;
    }
    else if (strncmp(res, "_b", 2) == 0)
    {
        LE_INFO("Boot on B slot.");
        *bootA = false;
    }
	else
    {
        LE_ERROR("Fail to get boot slot.");
        updateStatus = TAF_PI_UA_STATUS_ERROR;
    }

    pclose(fp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initiate flash information.
 */
//--------------------------------------------------------------------------------------------------
void InitFlashInfo
(
    taf_pi_ua_FlashInfo* flashInfo ///< [OUT] Flash information.
)
{
    InitUbiInfoTab("telaf", flashInfo->isBootOnA, &flashInfo->telaf);
    InitUbiInfoTab("rootfs", flashInfo->isBootOnA, &flashInfo->rootfs);

    // Estimate time for applying patches
    if (flashInfo->telaf.validPatch)
    {
       totalTime += UPDATE_TELAF_TIME;
    }
    if (flashInfo->rootfs.validPatch)
    {
       totalTime += UPDATE_ROOTFS_TIME;
    }

    if (totalTime == 0)
    {
        LE_ERROR("No partitions to be updated.");
        updateStatus = TAF_PI_UA_STATUS_ERROR;
        return;
    }
    else
    {
        totalTime += UNPACK_TIME;
        LE_INFO("Estimate %ds for updating patches.", totalTime);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set active slot.
 */
//--------------------------------------------------------------------------------------------------
void SetActiveSlot
(
    int slot ///< [IN] Slot to be set as active.
)
{
    char activeCmd[STR_MAX_BYTES];
    snprintf(activeCmd, sizeof(activeCmd), SET_ACTIVE_SLOT, slot);
    if (SendCmd(activeCmd) != LE_OK)
    {
        LE_ERROR("AB switch failed.");
        updateStatus = TAF_PI_UA_STATUS_ERROR;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set pause action
 */
//--------------------------------------------------------------------------------------------------
void SetPauseAction
(
    bool action ///< [IN] Pause action
)
{
    int rc;
    if (action)
    {
        rc = open(PAUSE_ACTION, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR);
        if (rc > 0)
        {
            close(rc);
        }
    }
    else
	{
        rc = unlink(PAUSE_ACTION);
    }
    LE_DEBUG("RC : %d.", rc);

    (void)SendCmd(SYNC_FS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get pause action
 */
//--------------------------------------------------------------------------------------------------
bool GetPauseAction
(
    void
)
{
    if (access(PAUSE_ACTION, 0) == 0)
        return true;

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Update command handler.
 *
 * @return NULL
 */
//--------------------------------------------------------------------------------------------------
void UpdateCmdHandler
(
    void* cmdPtr ///< [IN] Update command.
)
{
    ua_Command_t* uaCmdPtr = (ua_Command_t*)cmdPtr;

    switch (uaCmdPtr->action)
    {
        case START_INSTALL:
            if (updateStatus == TAF_PI_UA_STATUS_UPDATING ||
                updateStatus == TAF_PI_UA_STATUS_PAUSED)
            {
                LE_WARN("Installation is in progress.");
            }
            else
            {
                session.cmd.action = START_INSTALL;
                le_utf8_Copy(session.cmd.filePath, uaCmdPtr->filePath, FILE_PATH_MAX_BYTES, NULL);
                session.process = INIT_SESSION;
                le_timer_Start(updateTimerRef);
                le_event_Report(sessProcEv, &session.process, sizeof(taf_pi_ua_Process));
            }
            break;
        case PAUSE_INSTALL:
            if (updateStatus == TAF_PI_UA_STATUS_PAUSED)
            {
                LE_WARN("Installation is already pauesd.");
            }
            else if (updateStatus == TAF_PI_UA_STATUS_UPDATING || updateStatus == TAF_PI_UA_STATUS_INIT)
            {
                SetPauseAction(true);
            }
            break;
        case RESUME_INSTALL:
            if (updateStatus == TAF_PI_UA_STATUS_PAUSED)
            {
                SetPauseAction(false);
                session.cmd.action = RESUME_INSTALL;
                le_timer_Start(updateTimerRef);
                le_event_Report(sessProcEv, &session.process, sizeof(taf_pi_ua_Process));
            }
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Update thread.
 *
 * @return NULL
 */
//--------------------------------------------------------------------------------------------------
void* UpdateThread
(
    void* contextPtr ///< [IN] Context.
)
{
    le_event_AddHandler("UpdateCmdHandler", updateCmdEv, UpdateCmdHandler);

    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Session process handler.
 *
 * @return NULL
 */
//--------------------------------------------------------------------------------------------------
void SessProcHandler
(
    void* processPtr ///< [IN] Session process context.
)
{
    taf_pi_ua_Process* procPtr = (taf_pi_ua_Process*)processPtr;
    switch (*procPtr)
    {
        case INIT_SESSION:
            updateStatus = TAF_PI_UA_STATUS_INIT;
            totalTime = 0;
            elapsedTime = 0;
            updatePercent = 0;

            session.process = UNPACK_PACKAGE;
            if (!GetPauseAction())
            {
                le_event_Report(sessProcEv, &session.process, sizeof(taf_pi_ua_Process));
            }
            else
            {
                updateStatus = TAF_PI_UA_STATUS_PAUSED;
            }
            break;
        case UNPACK_PACKAGE:
            updateStatus = TAF_PI_UA_STATUS_INIT;

            UnpackPackage(session.cmd.filePath);

            if (updateStatus != TAF_PI_UA_STATUS_ERROR)
            {
                session.process = GET_BOOT_SLOT;
                if (!GetPauseAction())
                {
                    le_event_Report(sessProcEv, &session.process, sizeof(taf_pi_ua_Process));
                }
                else
                {
                    updateStatus = TAF_PI_UA_STATUS_PAUSED;
                }
            }
            break;
        case GET_BOOT_SLOT:
            updateStatus = TAF_PI_UA_STATUS_INIT;

            GetBootSlot(&session.flashInfo.isBootOnA);
            if (updateStatus != TAF_PI_UA_STATUS_ERROR)
            {
                session.process = INIT_FLASH_INFO;
                if (!GetPauseAction())
                {
                    le_event_Report(sessProcEv, &session.process, sizeof(taf_pi_ua_Process));
                }
                else
                {
                    updateStatus = TAF_PI_UA_STATUS_PAUSED;
                }
            }
            break;
        case INIT_FLASH_INFO:
            updateStatus = TAF_PI_UA_STATUS_INIT;

            InitFlashInfo(&session.flashInfo);
            if (updateStatus != TAF_PI_UA_STATUS_ERROR)
            {
                session.process = APPLY_TELAF_PATCH;
                if (!GetPauseAction())
                {
                    le_event_Report(sessProcEv, &session.process, sizeof(taf_pi_ua_Process));
                }
                else
                {
                    updateStatus = TAF_PI_UA_STATUS_PAUSED;
                }
            }
            break;
        case APPLY_TELAF_PATCH:
            updateStatus = TAF_PI_UA_STATUS_UPDATING;

            if (ApplyUbiPatch(&session.flashInfo.telaf) != LE_OK)
            {
                LE_ERROR("Applying telaf patch failed.");
                updateStatus = TAF_PI_UA_STATUS_ERROR;
            }
            else
            {
                session.process = APPLY_ROOTFS_PATCH;
                if (!GetPauseAction())
                {
                    le_event_Report(sessProcEv, &session.process, sizeof(taf_pi_ua_Process));
                }
                else
                {
                    updateStatus = TAF_PI_UA_STATUS_PAUSED;
                }
            }
            break;
        case APPLY_ROOTFS_PATCH:
            updateStatus = TAF_PI_UA_STATUS_UPDATING;

            if (ApplyUbiPatch(&session.flashInfo.rootfs) != LE_OK)
            {
                LE_ERROR("Applying rootfs patch failed.");
                updateStatus = TAF_PI_UA_STATUS_ERROR;
            }
            else
            {
                session.process = SET_ACTIVE;
                if (!GetPauseAction())
                {
                    le_event_Report(sessProcEv, &session.process, sizeof(taf_pi_ua_Process));
                }
                else
                {
                    updateStatus = TAF_PI_UA_STATUS_PAUSED;
                }
            }
            break;
        case SET_ACTIVE:
            SetActiveSlot((int)session.flashInfo.isBootOnA);
            if (updateStatus != TAF_PI_UA_STATUS_ERROR)
            {
                updateStatus = TAF_PI_UA_STATUS_FINISH;
            }
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Session process thread.
 *
 * @return NULL
 */
//--------------------------------------------------------------------------------------------------
void* SessProcThread
(
    void* contextPtr ///< [IN] Context.
)
{
    le_event_AddHandler("SessProcHandler", sessProcEv, SessProcHandler);

    le_sem_Post((le_sem_Ref_t)contextPtr);

    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize update agent.
 */
//--------------------------------------------------------------------------------------------------
void taf_pi_ua_Init()
{
    LE_INFO("UA Plug-In Init...");

    sessionRefMap = le_ref_InitStaticMap(sessionRefMap, UA_SESSION_NUM);

    le_sem_Ref_t semaphore = le_sem_Create("updateSem", 0);
    updateCmdEv = le_event_CreateId("updateCmdEv", sizeof(ua_Command_t));
    le_thread_Ref_t threadRef = le_thread_Create("updateThread", UpdateThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, 0x20000);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);

    sessProcEv = le_event_CreateId("sessProcEv", sizeof(taf_pi_ua_Process));
    threadRef = le_thread_Create("sessProcThread", SessProcThread, (void*)semaphore);
    le_thread_SetStackSize(threadRef, 0x20000);
    le_thread_Start(threadRef);
    le_sem_Wait(semaphore);

    updateTimerRef = le_timer_Create("updateTimer");
    le_timer_SetMsInterval(updateTimerRef, 1000);
    le_timer_SetRepeat(updateTimerRef, 0);
    le_timer_SetHandler(updateTimerRef, UpdateTimerHandler);

    LE_INFO("UA Plug-In Ready...");
}

//--------------------------------------------------------------------------------------------------
/**
 * Create update agent session.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
int taf_pi_ua_GetSession
(
    const char* cfgFile,            ///< [IN]  Update session configuration file.
    taf_pi_ua_SessionRef_t* sessRef ///< [OUT] Update session reference.
)
{
    LE_INFO("UA Plug-In get update session.");

    FILE* file = fopen(cfgFile, "r");
    if (file == NULL)
    {
        LE_ERROR("Fail to open file %s", cfgFile);
        return -1;
    }

    if (fgets(session.deltaPath, sizeof(session.deltaPath), file) == NULL)
    {
        LE_ERROR("Fail to get delta decoder.");
        fclose(file);
        return -1;
    }

    if (sessionRef == NULL)
    {
        sessionRef = (taf_pi_ua_SessionRef_t)le_ref_CreateRef(sessionRefMap, (void*)&session);
    }

    *sessRef = sessionRef;
    fclose(file);

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start install.
 *
 * @return
 *  - 0      On success.
 *  - Others On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_pi_ua_StartInstall
(
    taf_pi_ua_SessionRef_t sessRef, ///< [IN] Update session reference.
    const char* filePath            ///< [IN] Update package.
)
{
    LE_INFO("UA Plug-In start install...");

    taf_pi_ua_Session* sessPtr = (taf_pi_ua_Session*)le_ref_Lookup(sessionRefMap, sessRef);
    if (sessPtr == NULL)
    {
        LE_ERROR("Can not find update session.");
        return -1;
    }

    ua_Command_t cmd;
    cmd.action = START_INSTALL;
    le_utf8_Copy(cmd.filePath, filePath, FILE_PATH_MAX_BYTES, NULL);
    le_event_Report(updateCmdEv, &cmd, sizeof(ua_Command_t));

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Pause install.
 *
 * @return
 *  - 0      On success.
 *  - Others On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_pi_ua_PauseInstall
(
    taf_pi_ua_SessionRef_t sessRef ///< [IN] Update session reference.
)
{
    LE_INFO("UA Plug-In pause install...");

    taf_pi_ua_Session* sessPtr = (taf_pi_ua_Session*)le_ref_Lookup(sessionRefMap, sessRef);
    if (sessPtr == NULL)
    {
        LE_ERROR("Can not find update session.");
        return -1;
    }

    ua_Command_t cmd;
    cmd.action = PAUSE_INSTALL;
    le_event_Report(updateCmdEv, &cmd, sizeof(ua_Command_t));

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume install.
 *
 * @return
 *  - 0      On success.
 *  - Others On failure.
 */
//--------------------------------------------------------------------------------------------------
int taf_pi_ua_ResumeInstall
(
    taf_pi_ua_SessionRef_t sessRef ///< [IN] Update session reference.
)
{
    LE_INFO("UA Plug-In resume install...");

    taf_pi_ua_Session* sessPtr = (taf_pi_ua_Session*)le_ref_Lookup(sessionRefMap, sessRef);
    if (sessPtr == NULL)
    {
        LE_ERROR("Can not find update session.");
        return -1;
    }

    ua_Command_t cmd;
    cmd.action = RESUME_INSTALL;
    le_event_Report(updateCmdEv, &cmd, sizeof(ua_Command_t));

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get update progress.
 *
 * @return
 * - 0      -- Succeeded.
 * - Others -- Failed.
 */
//--------------------------------------------------------------------------------------------------
int taf_pi_ua_GetProgress
(
    taf_pi_ua_SessionRef_t sessRef, ///< [IN]  Update session reference.
    taf_pi_ua_Status_t* status,     ///< [OUT] Update status.
    int* percent,                   ///< [OUT] Update percentage.
    int* error                      ///< [OUT] Error code in update process.
)
{
    taf_pi_ua_Session* sessPtr = (taf_pi_ua_Session*)le_ref_Lookup(sessionRefMap, sessRef);
    if (sessPtr == NULL)
    {
        LE_ERROR("Can not find update session.");
        return -1;
    }

    *status = updateStatus;
    *percent = updatePercent;

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Update agent information table.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED ua_InfoTab_t TAF_HAL_INFO_TAB =
{
    .mgrInf =
    {
        .name = TAF_UA_MODULE_NAME,
        .majorVer = 1,
        .minorVer = 0,
        .vendor = "QCT",
        .moduleType = TAF_MODULETYPE_PLUG_IN,
        .getModInf = taf_hal_GetModInf,
        .res = { 0 },
    },

    .uaInf =
    {
        .init = taf_pi_ua_Init,
        .getSess = taf_pi_ua_GetSession,
        .startInstall = taf_pi_ua_StartInstall,
        .pauseInstall = taf_pi_ua_PauseInstall,
        .resumeInstall = taf_pi_ua_ResumeInstall,
        .getProgress = taf_pi_ua_GetProgress,
    },
};

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}
