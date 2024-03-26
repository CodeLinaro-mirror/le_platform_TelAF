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
            void Init(const char *dbName);

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
            int32_t ReadDtcCountByStatus(int32_t status);
            le_result_t ReadDtcByStatus(int32_t status, taf_DataAccess_DTCStatusRec_t *dtcStatusPtr);
            le_result_t ReadStatusByDtc(int32_t dtc, int32_t& status);
            le_result_t ReadStatusByDtc(int32_t dtc, int32_t& status, int32_t& occurCounter);
            le_result_t ReadFaultDetectionCounter(taf_DataAccess_FDCInfoRec_t *fdcInfoRecPtr);

            le_result_t WriteStatusByDtc(int32_t dtc, int32_t status);
            le_result_t WriteStatusByDtc(int32_t dtc, int32_t status, int32_t occurCounter);
            le_result_t WriteFaultDetectionCounterByDtc(int32_t dtc, int32_t fdc);

            le_result_t ClearDtcRecord();
            le_result_t ClearDtcRecord(int32_t dtc);
        private:
            le_result_t CreateTable(std::shared_ptr<IOHandler<DtcEntity, int32_t>> handler,
                    bool ifNotExists);
            le_result_t DropTable(std::shared_ptr<IOHandler<DtcEntity, int32_t>> handler,
                    bool ifExists);

            std::vector<std::string> columnList;
            std::vector<std::string> pkList;
            taf_DataAcsNVMType_t storageType = NVM_TYPE_SQLITE3;

            le_mem_PoolRef_t dtcStatusPool;
            le_mem_PoolRef_t fdcPool;
    };
}
}
#endif