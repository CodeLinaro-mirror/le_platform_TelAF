/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/* @file       tafTherm.hpp
* @brief      Interface for Thermal Service object. The functions
*             in this file are impletmented internally.
*/

#include "legato.h"
#include "interfaces.h"
#include <memory>
#include "tafSvcIF.hpp"
#include <map>
#include <string>
#include "le_singlyLinkedList.h"
#include "telux/common/CommonDefines.hpp"
#include <telux/therm/ThermalDefines.hpp>
#include <telux/therm/ThermalFactory.hpp>
#include <telux/therm/ThermalListener.hpp>
#include <telux/therm/ThermalManager.hpp>

#define TAF_THERM_MAX_LIST_POOL_SIZE 100
#define TAF_THERM_MAX_ZONE_POOL_SIZE 50
#define TAF_THERM_ZONE_TYPE_MAX_SIZE 32

typedef struct {
    uint32_t coolingDeviceId;
    uint32_t tripPointListSize;
     le_sls_List_t TripPointList;
     le_sls_Link_t link;
     le_sls_Link_t* currPtr;
     taf_therm_BoundCoolingDeviceRef_t ref;
}taf_BoundCoolingDevice_t;

typedef struct
{
    taf_therm_TripType_t tripType;
    uint32_t threshold;
    uint32_t hysteresis;
    uint32_t tripId;    //supported in sa525m
    uint32_t tZoneId;   //supported in sa525m
    le_sls_Link_t link;
    taf_therm_TripPointRef_t ref;
} taf_TripPoint_t;

typedef struct
{
    uint32_t tZoneId;
    uint32_t currTemp;
    uint32_t passiveTemp;
    char Type[50];
    uint32_t tripPointListSize;
    uint32_t boundCoolingDeviceListSize;
    le_sls_List_t TripPointList;
    le_sls_List_t BoundCoolingDeviceList;
    le_sls_Link_t link;
    le_sls_Link_t* currPtr;
    taf_therm_ThermalZoneRef_t ref;
} taf_ThermalZone_t;

typedef struct
{
    uint32_t thermalZoneListSize;
    le_sls_List_t ThermalZoneList;
    le_msg_SessionRef_t sessionRef;
    le_sls_Link_t* currPtr;
    taf_therm_ThermalZoneListRef_t ref;
}taf_ThermalZoneList_t;

typedef struct
{
    uint32_t cDevId;
    uint32_t maxCoolingLevel;
    uint32_t currentCoolingLevel;
    char description[50];
    le_sls_Link_t link;
    taf_therm_CoolingDeviceRef_t ref;
} taf_CoolingDevice_t;

typedef struct
{
    uint32_t coolingDeviceListSize;
    le_sls_List_t CoolingDeviceList;
    le_msg_SessionRef_t sessionRef;
    le_sls_Link_t* currPtr;
    taf_therm_CoolingDeviceListRef_t ref;
}taf_CoolingDeviceList_t;

typedef struct
{
   taf_TripPoint_t* tripPoint;
   taf_therm_TripEventType_t tripEvent;
}taf_TripEventInfo_t;


typedef struct
{
    taf_CoolingDevice_t* cDevice;
}coolingLevelChangeInfo_t;

namespace telux {
    namespace tafsvc {

        class taf_ThermServiceListener : public telux::therm::IThermalListener {
        public:
            virtual void onServiceStatusChange(telux::common::ServiceStatus serviceStatus) override;
            void onTripEvent(std::shared_ptr<telux::therm::ITripPoint> tripPoint,
                    telux::therm::TripEvent tripEvent) override;
            void onCoolingDeviceLevelChange(
                   std::shared_ptr<telux::therm::ICoolingDevice> coolingDevice) override;
        };

        class taf_Therm : public ITafSvc {
        public:
            taf_Therm() {};
            ~taf_Therm() {};

            static taf_Therm& GetInstance();
            void Init();

            le_mem_PoolRef_t cDevListPool;
            le_mem_PoolRef_t cDevPool;
            le_mem_PoolRef_t tZoneListPool;
            le_mem_PoolRef_t tZonePool;
            le_mem_PoolRef_t tripPointPool;
            le_mem_PoolRef_t boundCDPool;
            le_mem_PoolRef_t boundTripPointCDPool;

            le_ref_MapRef_t cDevListRefMap;
            le_ref_MapRef_t cDevRefMap;
            le_ref_MapRef_t tZoneListRefMap;
            le_ref_MapRef_t tZoneRefMap;
            le_ref_MapRef_t tripPointRefMap;
            le_ref_MapRef_t boundCDRefMap;
            le_ref_MapRef_t boundTripPointRefMap;

            le_event_Id_t stateChangeEvent;
            le_event_Id_t onCoolingLevelChangeEvent;

            std::shared_ptr<telux::therm::IThermalManager> thermalManager;
            std::shared_ptr<taf_ThermServiceListener> thermalListener;
            std::shared_ptr<taf_ThermServiceListener> thermalServiceListener;
            std::map <std::string, int> zoneNameToIdMap;
            std::map <std::string, int> CDevNameToIdMap;

            le_result_t ReleaseTripEventRef(taf_therm_TripPointRef_t tripEventRef);
            le_result_t ReleaseCoolingDeviceRef(taf_therm_CoolingDeviceRef_t cDevRef);

            static void EventChanged(void* reportPtr, void* SecondLayeredHandlerFunc);
            taf_therm_TripEventHandlerRef_t AddTripEventHandler (
                    taf_therm_TripEventHandlerFunc_t handlerPtr, void* contextPtr);
            void RemoveTripEventHandler(taf_therm_TripEventHandlerRef_t handlerRef);
            const char* TripEventToString(telux::therm::TripEvent state);

            static void CoolingLevelChanged(void* reportPtr, void* SecondLayeredHandlerFunc);
            taf_therm_CoolingLevelChangeEventHandlerRef_t AddCoolingLevelChangeEventHandler
            (taf_therm_CoolingLevelChangeEventHandlerFunc_t handlerPtr, void* contextPtr);
            void RemoveCoolingLevelChangeEventHandler(
                    taf_therm_CoolingLevelChangeEventHandlerRef_t handlerRef);

            taf_therm_ThermalZoneListRef_t GetThermalZonesList();
            taf_therm_ThermalZoneRef_t GetFirstThermalZone(taf_therm_ThermalZoneListRef_t);
            taf_therm_ThermalZoneRef_t GetNextThermalZone(taf_therm_ThermalZoneListRef_t);
            taf_therm_TripPointRef_t GetFirstTripPoint(taf_therm_ThermalZoneRef_t);
            taf_therm_TripPointRef_t GetNextTripPoint(taf_therm_ThermalZoneRef_t);
            taf_therm_BoundCoolingDeviceRef_t GetFirstBoundCDev(taf_therm_ThermalZoneRef_t);
            taf_therm_BoundCoolingDeviceRef_t GetNextBoundCDev(taf_therm_ThermalZoneRef_t);
            taf_therm_TripPointRef_t GetFirstBoundTripPoint(taf_therm_BoundCoolingDeviceRef_t);
            taf_therm_TripPointRef_t GetNextBoundTripPoint(taf_therm_BoundCoolingDeviceRef_t);
            le_result_t DeleteThermalZoneList(taf_therm_ThermalZoneListRef_t);

            le_result_t GetThermalZonesListSize(taf_therm_ThermalZoneListRef_t,uint32_t* listSize);
            le_result_t GetTripPointListSize(taf_therm_ThermalZoneRef_t, uint32_t* listSize);
            le_result_t GetBoundCoolingDeviceListSize(taf_therm_ThermalZoneRef_t,
                    uint32_t* listSize);

            le_result_t GetThermalZoneID(taf_therm_ThermalZoneRef_t, uint32_t* thermalZoneID);
            le_result_t GetThermalZoneCurrentTemp(taf_therm_ThermalZoneRef_t, uint32_t* currTemp);
            le_result_t GetThermalZonePassiveTemp(taf_therm_ThermalZoneRef_t,
                    uint32_t* passiveTemp);
            le_result_t GetThermalZoneType(taf_therm_ThermalZoneRef_t, char*, size_t );
            le_result_t GetTripPointType(taf_therm_TripPointRef_t,  char*,size_t );
            le_result_t GetTripPointThreshold(taf_therm_TripPointRef_t, uint32_t* threshold);
            le_result_t GetTripPointHysterisis(taf_therm_TripPointRef_t, uint32_t* hysterisis);

            #if defined(LE_CONFIG_ENABLE_THERMAL_GET_TRIP_ID)
            le_result_t GetTripPointTripID(taf_therm_TripPointRef_t, uint32_t* tripID);
            le_result_t GetBoundTripPointTripID(taf_therm_TripPointRef_t, uint32_t* boundTripID);
            #endif
            #if defined(LE_CONFIG_ENABLE_THERMAL_GET_ZONE_ID)
            le_result_t GetTripPointThermalZoneID(taf_therm_TripPointRef_t, uint32_t* tZoneID);
            le_result_t GetBoundTripPointThermalZoneID(taf_therm_TripPointRef_t,
                    uint32_t* boundTZoneID);
            #endif

            le_result_t GetBoundTripPointType(taf_therm_TripPointRef_t, char*, size_t );
            le_result_t GetBoundTripPointThreshold(taf_therm_TripPointRef_t,
                    uint32_t* boundThreshold);
            le_result_t GetBoundTripPointHysterisis(taf_therm_TripPointRef_t,
                    uint32_t* boundHysterisis);
            le_result_t GetBoundCoolingId(taf_therm_BoundCoolingDeviceRef_t,
                    uint32_t* boundCoolingId);
            le_result_t GetBoundTripPointListSize(taf_therm_BoundCoolingDeviceRef_t,
                    uint32_t* listSize);

            taf_therm_CoolingDeviceListRef_t GetCoolingDeviceList();
            taf_therm_CoolingDeviceRef_t GetFirstCoolingDevice(taf_therm_CoolingDeviceListRef_t);
            taf_therm_CoolingDeviceRef_t GetNextCoolingDevice(taf_therm_CoolingDeviceListRef_t);
            le_result_t DeleteCoolingDeviceList(taf_therm_CoolingDeviceListRef_t);

            le_result_t GetCoolingDeviceListSize(taf_therm_CoolingDeviceListRef_t,
                     uint32_t* listSize);
            le_result_t GetCDevID(taf_therm_CoolingDeviceRef_t, uint32_t* cDevID);
            le_result_t GetCDevDescription(taf_therm_CoolingDeviceRef_t, char*, size_t);
            le_result_t GetCDevMaxCoolingLevel(taf_therm_CoolingDeviceRef_t,
                     uint32_t* maxCoolingLevel);
            le_result_t GetCDevCurrentCoolingLevel(taf_therm_CoolingDeviceRef_t,
                     uint32_t* currentCoolingLevel);

            taf_therm_ThermalZoneRef_t GetThermalZoneByName(const char*);
            taf_therm_CoolingDeviceRef_t GetCoolingDeviceByName(const char*);
            int MapThermalZonetoId(const char*);
            int MapCDevtoId(const char*);
        };
        class taf_Handler : public ITafSvc {
        public:
         void Init(void);
         taf_Handler();
         ~taf_Handler();
        //taf service handlers
         static void OnClientDisconnection(le_msg_SessionRef_t sessionRef, void* contextPtr);
        };
    }
}