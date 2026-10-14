/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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