/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_DTC_ENTITY_DAO_HPP
#define TAF_DTC_ENTITY_DAO_HPP

#include <string>
#include <memory>
#include <vector>
#include "legato.h"
#include "interfaces.h"
#include "tafDataAccessComp.h"
#include "tafBaseDAO.hpp"
#include "tafDTCEntity.hpp"
#include "tafIOFactory.hpp"
#include "tafIOHandler.hpp"

namespace taf{
namespace dataAccess{
    #define DTC_TABLE_NAME  "dtc_entity"
    #define DTC_TIME_BUF_SIZE   64

    typedef struct
    {
        uint32_t    dtc;
        uint8_t     faultDetCounter;    // DTC Fault Detection Counter
        le_dls_Link_t link;
    }taf_DataAccess_FDCInfo_t;

    typedef struct
    {
        le_dls_List_t fdcInfoList;
    }taf_DataAccess_FDCInfoRec_t;

    class DtcEntityDao : public BaseDao<DtcEntity, int32_t> {
        public:
            DtcEntityDao();
            ~DtcEntityDao();

            static DtcEntityDao &GetInstance();
            using BaseDao<DtcEntity, int32_t>::Init;
            void Init(const char *dbName,   int expectedVer);
            le_result_t Load();

            void BindValues(DataStatement &statement, DtcEntity &entity) override;
            void BindKeyValue(DataStatement &statement, DtcEntity &entity) override;
            void BindKeyValue(DataStatement &statement, int index, int32_t key) override;

            // Get value from storage file.
            void ReadEntity(DataStatement &statement, DtcEntity &entity) override;
            int32_t ReadKey(DataStatement &statement) override;

            int32_t GetKey(DtcEntity &entity) override;
            bool HasKey(DtcEntity &entity) override;

            std::vector<std::string> GetColumnsName() override;
            std::vector<std::string> GetPrimaryKeysName() override;

            int32_t ReadDtcCountByDtc(int32_t dtc);
            int32_t ReadDtcCountByDtcWithSuppress(int32_t dtc);
            int32_t ReadDtcCountByStatus(int32_t status);
            int32_t ReadDtcCountByStatusWithSuppress(int32_t status);
            le_result_t ReadDtcByStatus(int32_t status, taf_DataAccess_DTCStatusRec_t *dtcStatusPtr);
            le_result_t ReadStatusByDtc(int32_t dtc, int32_t& status);
            le_result_t ReadStatusByDtc(int32_t dtc, int32_t& status, int32_t& occurCounter);
            le_result_t ReadStatusByDtc(int32_t dtc, int32_t& status, int32_t& occurCounter,
                    int32_t& activation, int32_t& suppression);
            le_result_t ReadFaultDetectionCounter(taf_DataAccess_FDCInfoRec_t *fdcInfoRecPtr);

            le_result_t WriteStatusByDtc(int32_t dtc, int32_t status);
            le_result_t WriteStatusByDtc(int32_t dtc, int32_t status, int32_t occurCounter);
            le_result_t WriteFaultDetectionCounterByDtc(int32_t dtc, int32_t fdc);

            le_result_t ReadActivationByDtc(int32_t dtc, int32_t& activation);
            le_result_t WriteActivationByDtc(int32_t dtc, int32_t activation);
            le_result_t ReadSuppressionByDtc(int32_t dtc, int32_t& suppression);
            le_result_t WriteSuppressionByDtc(int32_t dtc, int32_t suppression);
            le_result_t WriteAllSuppression(int32_t suppression);
            le_result_t ClearDtcRecord();
            le_result_t ClearDtcRecord(int32_t dtc);
            std::vector<int32_t> GetDTCWithUnsuppress();
            std::vector<int32_t> GetDTCWithSuppress();
        private:
            le_result_t CreateTable(std::shared_ptr<IOHandler<DtcEntity, int32_t>> handler,
                    bool ifNotExists);
            le_result_t DropTable(std::shared_ptr<IOHandler<DtcEntity, int32_t>> handler,
                    bool ifExists);
            le_result_t UpdateTable(std::shared_ptr<IOHandler<DtcEntity, int32_t>> handler,
                    int currentVer, int expectedVer);
            le_result_t InitTableWithConfig();

            std::vector<std::string> columnList;
            std::vector<std::string> pkList;
            taf_DataAcsNVMType_t storageType = NVM_TYPE_SQLITE3;

            le_mem_PoolRef_t dtcStatusPool;
            le_mem_PoolRef_t fdcPool;
    };
}
}
#endif