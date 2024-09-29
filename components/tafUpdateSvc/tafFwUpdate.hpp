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

#ifndef TAFFWUPDATE_HPP
#define TAFFWUPDATE_HPP

#include <map>
#include "legato.h"
#include "interfaces.h"

#include "tafSvcIF.hpp"
#include "tafFlashAccess.hpp"

#define TAF_FWUPDATE_INSTALL_CMD_LEN 256
#define TAF_FWUPDATE_CMD_RESULT_LEN 32
// Data proccessing rate is about 3.84 MB/s.
#define TAF_FWUPDATE_PROC_DATA_RATE 4035394

#define TAF_FIRMWARE_VERSION_LINE_NUM 16
#define TAF_TELAF_VERSION_LEN 21

#define TAF_FWUPDATE_BYPASS_CHECK_TAG "NULL"

#define TAF_FWUPDATE_RECOVERY_LOG_FILE "/tmp/recovery.log"

#define TAF_TELAF_VERSION_FILE "/legato/systems/current/version"
#define TAF_ROOTFS_VERSION_FILE "/etc/version"
#define TAF_FIRMWARE_VERSION_FILE "/firmware/image/Ver_Info.txt"

#define TAF_FWUPDATE_FOTA_STATE "/data/le_fs/fotaState"
#define TAF_FWUPDATE_PREVIOUS_BANK "/data/le_fs/bank"
#define TAF_FWUPDATE_LOCAL_PACAKAGE_PATH "/data/images/firmware"

const size_t kPageSize = 4 * 1024; //4k

const std::string kAreBlocksErased = "ARE_BLOCKS_ERASED";
const std::string kIsMTDSynced = "IS_MTD_SYNCED";
const std::string kIsUBISynced = "IS_UBI_SYNCED";
const std::string kPagesSynced = "PAGES_SYNCED";
const std::string kTotalPages = "TOTAL_PAGES";

//--------------------------------------------------------------------------------------------------
/**
 * Flash page size.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_FWUPDATE_FLASH_PAGE_SIZE 0x1000

// Firmware update event
typedef enum {
    TAF_FWUPDATE_EV_INSTALL,
    TAF_FWUPDATE_EV_INSTALL_POST_CHECK,
    TAF_FWUPDATE_EV_REBOOT_TO_ACTIVE,
    TAF_FWUPDATE_EV_VERIFY_ACTIVATION,
    TAF_FWUPDATE_EV_SYNC,
    TAF_FWUPDATE_EV_START_SYNC,
    TAF_FWUPDATE_EV_PAUSE_SYNC,
    TAF_FWUPDATE_EV_RESUME_SYNC,
    TAF_FWUPDATE_EV_ROLLBACK
} taf_FwUpdateEvent_t;

// Timer options
typedef enum {
    TAF_FWUPDATE_TIMER_OP_INST_START,
    TAF_FWUPDATE_TIMER_OP_INST_STOP
} taf_FwUpdateTimerOp_t;

// Firmware update request
typedef struct {
    taf_FwUpdateEvent_t event;
    char filePath[TAF_UPDATE_FILE_PATH_LEN];
} taf_FwUpdateReq_t;

namespace telux {
namespace tafsvc {
    class taf_FwUpdate : public ITafSvc {
    public:
        taf_FwUpdate() {};
        ~taf_FwUpdate() {};

        static taf_FwUpdate &GetInstance();

        le_result_t SendPipeCmd(const char* cmd, const char* mod);

        taf_update_State_t GetState();
        void SetState(taf_update_State_t state);

        void ReportStatus(taf_update_State_t state, uint32_t percent, taf_update_Error_t error);
        void UpdateProgress(taf_update_State_t state);

        static void InstallTimerHandler(le_timer_Ref_t timerRef);

        void GetRootfsVersion(char* version);
        void GetTelafVersion(char* version);
        le_result_t GetFirmwareVersion(char* version);
        le_result_t GetMtdInformation(taf_lib_flash_Partition_t *partition, uint32_t* blocksNumber,
                uint32_t* badBlocksNumber, uint32_t* blockSize, uint32_t* pageSize);
        le_result_t GetUbiInformation(taf_lib_flash_Partition_t* partition, uint32_t* lebNumber,
                uint32_t* freeLebNumber, uint32_t* volumeSize);
        le_result_t InstallPreCheck(const char* manifest);
        void InstallFirmware(const char* filePath);
        le_result_t InstallPostCheck(const char* filePath);

        bool IsBankSwitched(void);
        le_result_t GetActiveBank(taf_update_Bank_t* bankPtr);
        le_result_t SetActiveBank(taf_update_Bank_t bank);
        le_result_t EraseBank(taf_update_Bank_t bank);
        le_result_t PerformBankSync(void);
        le_result_t Rollback(void);
        le_result_t VerifyActivation(const char* manifest);

        void Init(void);

        le_result_t CalculateTotalPages(void);
        bool CompareBinaryFiles(const std::string& filename1, const std::string& filename2);

        std::string FwUpdateStateToString(taf_update_State_t state);

        static void FwUpdateHandler(void* reqPtr);
        static void FwSyncHandler(void* reqPtr);
        static void FwStartSync(void* reqPtr);
        static void* FwUpdateThread(void* contextPtr);
        static void* FwSyncHandlerThread(void* contextPtr);
        static void* FwStartSyncThread(void* contextPtr);

        static void TimerOpHandler(void* contextPtr);
        static void* TimerThread(void* contextPtr);

        static le_event_Id_t fwUpdateEvId;
        static le_event_Id_t fwTimerEvId;
        static le_event_Id_t fwSyncHandlerEvId;
        static le_event_Id_t fwStartSyncEvId;

        le_timer_Ref_t instTimerRef;

        uint32_t percent = 0;
        uint32_t totalTime = 0;
        taf_update_Error_t error = TAF_UPDATE_NONE;
        bool isPartitionListInit = false;

    private:
        le_result_t InitPartitionList();
        le_result_t SyncMTD(taf_update_Bank_t activeBank);
        le_result_t EraseAllMTDBlocks(taf_update_Bank_t activeBank);
        le_result_t SyncUBI(taf_update_Bank_t activeBank);
        void PerformABSync();

        taf_lib_flash_PartitionList_t partitionList;
        std::map<std::string /* Partition name */, uint32_t /* Index*/> partitionMap;
    };
}
}

#endif
