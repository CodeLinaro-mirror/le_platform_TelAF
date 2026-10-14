/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

#include "legato.h"
#include "interfaces.h"
#include "tafDIDEntity.hpp"
#include "tafBaseDAO.hpp"
#include "tafIOFactory.hpp"
#include "tafIOHandler.hpp"

namespace taf{
namespace dataAccess {

#define DID_TABLE_NAME      "did_entity"
#define DID_TIME_BUF_SIZE   64

class DIDEntityDAO : public BaseDao<DIDEntity, int32_t> {
public:
    DIDEntityDAO() = default;
    ~DIDEntityDAO();

    static DIDEntityDAO &GetInstance();
    using BaseDao<DIDEntity, int32_t>::Init;
    void Init(const char *dbName, int expectedVer);
    le_result_t Load();

    void BindValues(DataStatement &statement, DIDEntity &entity) override;
    void BindKeyValue(DataStatement &statement, DIDEntity &entity) override;
    void BindKeyValue(DataStatement &statement, int index, int32_t key) override;

    void ReadEntity(DataStatement &statement, DIDEntity &entity) override;
    int32_t ReadKey(DataStatement &statement) override;

    int32_t GetKey(DIDEntity &entity) override;
    bool HasKey(DIDEntity &entity) override;

    std::vector<std::string> GetColumnsName() override;
    std::vector<std::string> GetPrimaryKeysName() override;

    le_result_t WriteDID(uint16_t did, const uint8_t *value, size_t len);
    le_result_t ReadDID(uint16_t did, uint8_t *value, size_t *len);
    bool RemoveDID(uint16_t did);
    std::vector<DIDEntity> FindAll();
    bool Clear();

private:
    le_result_t CreateTable(std::shared_ptr<IOHandler<DIDEntity, int32_t>> handler,
            bool ifNotExists);
    le_result_t DropTable(std::shared_ptr<IOHandler<DIDEntity, int32_t>> handler,
            bool ifExists);

    std::vector<std::string> columnList;
    std::vector<std::string> pkList;
    taf_DataAcsNVMType_t storageType = NVM_TYPE_SQLITE3;
};

} // namespace dataAccess
}
