/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_BASE_DAO_HPP
#define TAF_BASE_DAO_HPP

#include <vector>
#include <string>
#include <ctime>

#include "legato.h"
#include "interfaces.h"
#include "tafIOHandler.hpp"

namespace taf{
namespace dataAccess{

    // Forward declaration.
    template <typename T, typename K>
    class IOHandler;

    template <typename T, typename K>  // Template for entity type and key type
    class BaseDao {
        public:
            BaseDao(const BaseDao&) = delete;
            BaseDao& operator=(const BaseDao&) = delete;

            void Init(std::string  fileName, std::shared_ptr<IOHandler<T, K>> handler)
            {
                    mFileName = fileName;
                    mHandler = handler;
            }

            std::string mFileName;   // Storage entity(Table or file) name
            std::shared_ptr<IOHandler<T, K>> mHandler;
            std::shared_ptr<IOHandler<T, K>> GetDaoHandler()
            {
                return mHandler;
            }

            virtual void BindValues(DataStatement &statement, T &entity) = 0;
            virtual void BindKeyValue(DataStatement &statement, T &entity) = 0;
            virtual void BindKeyValue(DataStatement &statement, int index, K key) = 0;

            // Get value from storage file.
            virtual void ReadEntity(DataStatement &statement, T &entity) = 0;
            virtual K ReadKey(DataStatement &statement) = 0;

            virtual K GetKey(T &entity) = 0;
            virtual bool HasKey(T &entity) = 0;

            virtual std::vector<std::string> GetColumnsName() = 0; // Return column name list.
            virtual std::vector<std::string> GetPrimaryKeysName() = 0;

            // Access the DAO storage.
            le_result_t Add(T &entity)
            {
                return mHandler->Add(entity);
            }

            le_result_t Remove(T &entity)
            {
                return mHandler->Remove(entity);
            }

            le_result_t Update(T &entity)
            {
                return mHandler->Update(entity);
            }

            le_result_t QueryByKey(T &entity)
            {
                return mHandler->QueryByKey(entity);
            }

            DataStatement Query()
            {
                return mHandler->Query();
            }

            DataStatement Query(std::string &where)
            {
                return mHandler->Query(where);
            }

            DataStatement QueryCount(std::string &where)
            {
                return mHandler->QueryCount(where);
            }

            std::time_t String2Time(const char *timeBuf)
            {
                std::tm local;
                std::time_t t;

                strptime(timeBuf, "%Y-%m-%d %H:%M:%S", &local);
                local.tm_isdst = -1;
                t  = mktime(&local);

                return t;
            }

            void Time2String(std::time_t t, char *timeBuf, size_t timeBufSize)
            {
                std::tm *tmPtr;

                tmPtr = std::localtime(&t);
                std::strftime(timeBuf, timeBufSize, "%Y-%m-%d %H:%M:%S", tmPtr);
            }
        protected:
            BaseDao() {}
        private:
    };
}
}
#endif