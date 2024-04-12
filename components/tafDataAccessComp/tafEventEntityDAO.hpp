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