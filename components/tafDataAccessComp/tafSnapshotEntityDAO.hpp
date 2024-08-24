/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
#ifndef TAF_SNAPSHOT_ENTITY_DAO_HPP
#define TAF_SNAPSHOT_ENTITY_DAO_HPP
#include <string>
#include <memory>
#include <vector>
#include "legato.h"
#include "interfaces.h"
#include "tafDataAccessComp.h"
#include "tafBaseDAO.hpp"
#include "tafSnapshotEntity.hpp"
#include "tafIOFactory.hpp"
#include "tafIOHandler.hpp"

namespace taf{
namespace dataAccess{
    #define SNAPSHOT_TABLE_NAME "snapshot_entity"
    #define FF_TIME_BUF_SIZE    64  // Freeze frame time buffer size

    struct taf_DataAccess_DIDInfo_t{
        int32_t did;
        int32_t len;
        uint8_t *val;

        taf_DataAccess_DIDInfo_t(int32_t _did, const uint8_t _val[], int32_t _len)
                : did(_did), len(_len), val(new uint8_t[_len])
        {
            std::copy(_val, _val + len, val);
        }

        ~taf_DataAccess_DIDInfo_t()
        {
            LE_DEBUG("DA release DID info resource!");
            delete[] val;
        }
    };

    using DIDInfoPtr = std::shared_ptr<taf_DataAccess_DIDInfo_t>;

    class SnapshotEntityDao : public BaseDao<SnapshotEntity, int32_t> {
        public:
            SnapshotEntityDao();
            ~SnapshotEntityDao();

            static SnapshotEntityDao &GetInstance();
            using BaseDao<SnapshotEntity, int32_t>::Init;
            void Init(const char *dbName);

            void BindValues(DataStatement &statement, SnapshotEntity &entity) override;
            void BindKeyValue(DataStatement &statement, SnapshotEntity &entity) override;
            void BindKeyValue(DataStatement &statement, int index, int32_t key) override;

            // Get value from storage file.
            void ReadEntity(DataStatement &statement, SnapshotEntity &entity) override;
            int32_t ReadKey(DataStatement &statement) override;

            int32_t GetKey(SnapshotEntity &entity) override;
            bool HasKey(SnapshotEntity &entity) override;

            std::vector<std::string> GetColumnsName() override;
            std::vector<std::string> GetPrimaryKeysName() override;

            int32_t ReadDIDCount(int32_t dtc, int32_t recordNum);

            // The interfaces for upper layer.
            std::vector<DIDInfoPtr> GetDIDRecord(int32_t dtc, int32_t recordNum);
            le_result_t SetDIDRecord(int32_t dtc, int32_t recordNum, std::vector<DIDInfoPtr> dids);

            void GetAllDTCRecord(le_dls_List_t *list);

            le_result_t ClearFFRecord();
            le_result_t ClearFFRecord(int32_t id);
            le_result_t ClearFFRecordByDtc(int32_t dtc);

        private:
            le_result_t CreateTable(std::shared_ptr<IOHandler<SnapshotEntity, int32_t>> handler,
                    bool ifNotExists);
            le_result_t DropTable(std::shared_ptr<IOHandler<SnapshotEntity, int32_t>> handler,
                    bool ifExists);

            le_mem_PoolRef_t dtcRecordPool;

            std::vector<std::string> columnList;
            std::vector<std::string> pkList;
            taf_DataAcsNVMType_t storageType = NVM_TYPE_SQLITE3;
    };
}
}
#endif