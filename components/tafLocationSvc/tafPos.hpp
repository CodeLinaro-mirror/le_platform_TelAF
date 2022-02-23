/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "legato.h"
#include "interfaces.h"
#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <telux/loc/LocationDefines.hpp>
#include <telux/loc/LocationManager.hpp>
#include <telux/loc/LocationConfigurator.hpp>
#include <telux/loc/LocationListener.hpp>
#include "telux/common/CommonDefines.hpp"
#include "tafSvcIF.hpp"

using namespace telux::loc;
using namespace telux::common;

#define TAF_POS_MAX_OBJ 1
#define POSITIONING_SAMPLE_MAX       1
#define HIGH_POS_HANDLER_COUNT       1
#define SUPPOSED_AVERAGE_SPEED       50
#define DEFAULT_ACQUISITION_RATE     1000
#define DEFAULT_POWER_STATE          true
#define SEC_TO_MSEC            1000
#define HOURS_TO_SEC           3600
#define CHECK_VALIDITY(_par_,_max_) (((_par_) == (_max_))? false : true)

namespace telux {
namespace tafsvc {

        typedef struct
        {
            bool     locationValid;
            bool     altitudeValid;
            int32_t  latitude;
            int32_t  longitude;
            int32_t  altitude;
            int32_t  vAccuracy;
            int32_t  hAccuracy;
        }
        PositionParam_t;

        typedef struct taf_pos_Sample
        {
            taf_gnss_FixState_t fixState;

            bool      latitudeValid;
            bool      longitudeValid;
            bool      hAccuracyValid;
            bool      altitudeValid;
            bool      vAccuracyValid;
            bool      hSpeedValid;
            bool      hSpeedAccuracyValid;
            bool      vSpeedValid;
            bool      vSpeedAccuracyValid;
            bool      headingValid;
            bool      headingAccuracyValid;
            bool      directionValid;
            bool      directionAccuracyValid;
            bool      dateValid;
            bool      timeValid;
            bool      leapSecondsValid;
            int32_t   latitude;
            int32_t   longitude;
            int32_t   hAccuracy;
            int32_t   altitude;
            int32_t   vAccuracy;
            int32_t   vSpeed;
            int32_t   vSpeedAccuracy;
            uint32_t  hSpeed;
            uint32_t  heading;
            uint32_t  direction;
            uint32_t  hSpeedAccuracy;
            uint32_t  headingAccuracy;
            uint32_t  directionAccuracy;
            uint16_t  day;
            uint16_t  month;
            uint16_t  year;
            uint16_t  hours;
            uint16_t  milliseconds;
            uint16_t  seconds;
            uint16_t  minutes;
            uint8_t   leapSeconds;

            le_dls_Link_t   next;
        }
        taf_pos_Sample_t;

        typedef struct
        {
            taf_posCtrl_ActivationRef_t  posCtrlActivationRef;
            le_msg_SessionRef_t          sessionRef;
            le_dls_Link_t                next;
        }
        ClientRequest_t;

        typedef struct
        {
            taf_pos_SampleRef_t   positionSampleRef;
            taf_pos_Sample_t*     posSampleNodePtr;
            le_msg_SessionRef_t   sessionRef;
            le_dls_Link_t         next;
        }
        PosSampleRequest_t;

        typedef struct taf_pos_SampleHandler
        {
            taf_pos_MovementHandlerFunc_t handlerFuncPtr;
            void*                        handlerContextPtr;
            uint32_t                     acquisitionRate;
            uint32_t                     verticalMagnitude;
            uint32_t                     horizontalMagnitude;
            int32_t                      lastLong;
            int32_t                      lastLat;
            int32_t                      lastAlt;
            le_msg_SessionRef_t          sessionRef;
            le_dls_Link_t                next;
        }
        taf_pos_SampleHandler_t;

        typedef enum
        {
            ALTITUDE,
            H_ACCURACY,
            V_ACCURACY
        }
        taf_pos_DistanceValueType_t;

        class taf_Pos: public ITafSvc
        {
            public:
                taf_Pos() {};
                ~taf_Pos() {};
                void Init();
                static taf_Pos &GetInstance();
                le_result_t SetAcquisitionRate(uint32_t acquisitionRate);
                uint32_t GetAcquisitionRate();
                le_result_t GetFixState(taf_gnss_FixState_t* statePtr);
                taf_pos_MovementHandlerRef_t AddMovementHandler(uint32_t hMagnitude,uint32_t vMagnitude,
                        taf_pos_MovementHandlerFunc_t handlerPtr, void* contextPtr);
                void RemoveMovementHandler(taf_pos_MovementHandlerRef_t handlerRef);
                uint32_t ComputeCommonSmallestRate(uint32_t rate);
                uint32_t CalculateAcquisitionRate(uint32_t averageSpeed,uint32_t horizontalMagnitude,uint32_t verticalMagnitude);
                static void PositionHandler(taf_gnss_SampleRef_t positionSampleRef, void* contextPtr);
                static void PosCloseSessionEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
                static void PosCtrlCloseSessionEventHandler(le_msg_SessionRef_t sessionRef, void* contextPtr);
                static void PosHandlerDestructor(void* obj);
                static void PosSampleDestructor(void* obj);
                static le_result_t CalculateMove(taf_pos_SampleHandler_t *posSampleHandlerNodePtr, const PositionParam_t  *posParamPtr,
                        bool *hflagPtr, bool *vflagPtr);
                static int32_t TransformDistance( int32_t value, taf_pos_DistanceValueType_t type);
                static bool IsBeyondMagnitude( uint32_t magnitude, uint32_t move, uint32_t accuracy);
                static uint32_t CalculateDistance(uint32_t latitude1, uint32_t longitude1, uint32_t latitude2, uint32_t longitude2);
                void Release(taf_pos_SampleRef_t positionSampleRef);
                void posCtrl_Release( taf_posCtrl_ActivationRef_t ref);
                taf_posCtrl_ActivationRef_t posCtrl_Request( void);
                le_result_t GetTime( uint16_t* hoursPtr, uint16_t* minutesPtr, uint16_t* secondsPtr, uint16_t* millisecondsPtr);
                le_result_t Get2DLocation(int32_t* latitudePtr, int32_t* longitudePtr, int32_t* hAccuracyPtr);
                le_result_t GetDirection( uint32_t* directionPtr, uint32_t* directionAccuracyPtr);
                le_result_t Get3DLocation( int32_t* latitudePtr, int32_t* longitudePtr, int32_t* hAccuracyPtr, int32_t* altitudePtr,
                        int32_t* vAccuracyPtr);
                le_result_t sample_Get2DLocation(taf_pos_SampleRef_t positionSampleRef, int32_t* latitudePtr, int32_t* longitudePtr,
                        int32_t* horizontalAccuracyPtr);
                le_result_t sample_GetAltitude(taf_pos_SampleRef_t positionSampleRef, int32_t* altitudePtr, int32_t* altitudeAccuracyPtr);
                le_result_t sample_GetTime( taf_pos_SampleRef_t  positionSampleRef,
                        uint16_t* hoursPtr, uint16_t* minutesPtr, uint16_t* secondsPtr, uint16_t* millisecondsPtr);
                le_result_t sample_GetHorizontalSpeed(taf_pos_SampleRef_t positionSampleRef, uint32_t* hSpeedPtr, uint32_t* hSpeedAccuracyPtr);
                le_result_t sample_GetDirection(taf_pos_SampleRef_t  positionSampleRef, uint32_t* directionPtr, uint32_t* directionAccuracyPtr);
                le_result_t sample_GetVerticalSpeed( taf_pos_SampleRef_t  positionSampleRef, int32_t* vSpeedPtr, int32_t* vSpeedAccuracyPtr);
                le_result_t SetDistanceResolution(taf_pos_Resolution_t resolution);
                le_result_t sample_GetFixState(taf_pos_SampleRef_t  positionSampleRef,taf_gnss_FixState_t*  statePtr);

            private:
                le_mem_PoolRef_t  PosPoolRef;
                le_mem_PoolRef_t  PosRequestPoolRef;
                le_mem_PoolRef_t   PosHandlerPoolRef;
                le_mem_PoolRef_t PosCtrlHandlerPoolRef;
                le_ref_MapRef_t PosSampleMap;
                le_ref_MapRef_t ActivationRequestRefMap;
                le_msg_ServiceRef_t posMsgService;
                le_msg_ServiceRef_t posCtrlMsgService;
                le_dls_List_t PosHandlerList;
                le_dls_List_t PosSampleList;
                taf_pos_Resolution_t DistanceResolution;
                taf_gnss_PositionHandlerRef_t GnssHandlerRef;
                uint32_t AcqRate;
                int32_t NumOfHandlers;
                int CurrentActivationsCount;
        };
    }
}
