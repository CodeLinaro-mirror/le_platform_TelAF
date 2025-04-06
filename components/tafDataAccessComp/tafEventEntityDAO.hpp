/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_EVENT_ENTITY_DAO_HPP
#define TAF_EVENT_ENTITY_DAO_HPP

#include <string>
#include <memory>
#include <vector>
#include "legato.h"
#include "interfaces.h"
#include "tafDataAccessComp.h"
#include "tafBaseDAO.hpp"
#include "tafEventEntity.hpp"
#include "tafIOFactory.hpp"
#include "tafIOHandler.hpp"
#include "tafEventEntity.hpp"

namespace taf{
namespace dataAccess{
    #define EVENT_TABLE_NAME  "event_entity"
    #define EVENT_TIME_BUF_SIZE   64
    #define TEST_FAILED_COUNTER_MAX (65535)

    class EventEntityDao : public BaseDao<EventEntity, int32_t> {
        public:
            EventEntityDao();
            ~EventEntityDao();

            static EventEntityDao &GetInstance();
            using BaseDao<EventEntity, int32_t>::Init;
            void Init(const char *dbName);

            void BindValues(DataStatement &statement, EventEntity &entity) override;
            void BindKeyValue(DataStatement &statement, EventEntity &entity) override;
            void BindKeyValue(DataStatement &statement, int index, int32_t key) override;

            // Get value from storage file.
            void ReadEntity(DataStatement &statement, EventEntity &entity) override;
            int32_t ReadKey(DataStatement &statement) override;

            int32_t GetKey(EventEntity &entity) override;
            bool HasKey(EventEntity &entity) override;

            std::vector<std::string> GetColumnsName() override;
            std::vector<std::string> GetPrimaryKeysName() override;

            int32_t ReadEventCountByEventId(int32_t eventId);
            int32_t ReadEventCountByDtc(int32_t dtc);
            int32_t ReadEventCountByStatus(int32_t status);
            int32_t ReadEventStatusByEventId(int32_t eventId);
            le_result_t ReadEventInfoByEventId(int32_t eventId,
                    taf_DataAccess_EventInfo_t *eventInfoPtr);
            int32_t ReadFailedCounterByEventId(int32_t eventId);

            le_result_t WriteStatusAndDtcByEventId(int32_t eventId, int32_t status, int32_t dtc);
            le_result_t WriteFailedCounterByEventId(int32_t eventId, int32_t counter);

            le_result_t ClearEventRecord();
            le_result_t ClearEventRecord(int32_t eventId);
            le_result_t ClearEventRecordByDtc(int32_t dtc);
        private:
            le_result_t CreateTable(std::shared_ptr<IOHandler<EventEntity, int32_t>> handler,
                    bool ifNotExists);
            le_result_t DropTable(std::shared_ptr<IOHandler<EventEntity, int32_t>> handler,
                    bool ifExists);

            std::vector<std::string> columnList;
            std::vector<std::string> pkList;
            taf_DataAcsNVMType_t storageType = NVM_TYPE_SQLITE3;
    };
}
}
#endif