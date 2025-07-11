/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_SNAPSHOT_SVR_HPP
#define TAF_SNAPSHOT_SVR_HPP

#include "legato.h"
#include "interfaces.h"
#include "tafSvcIF.hpp"
#include "configuration.hpp"
#include "tafDataIDSvr.hpp"
#include "tafDataAccessComp.h"

#define DID_OF_SUPPLIER_FC 0xEF01

namespace tafsvc {

typedef struct SnapshotItem {
    uint32_t dtcCode;
    taf_ReadDIDRxMsg_t* msgPtr;
    uint8_t * supplierFaultCodePtr;
    size_t supplierFaultCodeSize;
} SnapshotItem_t;

class taf_SnapshotSvr: public ITafSvc
{
public:
    taf_SnapshotSvr();
    ~taf_SnapshotSvr();
    void Init();
    static taf_SnapshotSvr& GetInstance();

    void triggerSnapshot
    (
        uint32_t dtcCode,
        cfg::freeze_frame_trigger_type_t typeOfTrigger,
        const uint8_t* supplierFaultCodePtr,
        size_t supplierFaultCodeSize
    );

    void storeDidsAsSnapshot
    (
        const uint8_t* didRawData,
        size_t dataSize,
        taf_ReadDIDRxMsg_t* msgPtr
    );

private:
    std::vector<SnapshotItem_t *> SnapshotTriggeredList;
    le_mem_PoolRef_t SnapshotItemPool;
    le_mem_PoolRef_t DidNodePool;
    le_mem_PoolRef_t DidListPool;
    le_mem_PoolRef_t DidValPool;

    static void ReleaseList(le_dls_List_t* list);

    bool NeedToBeTriggered
    (
        uint32_t dtcCode,
        cfg::freeze_frame_trigger_type_t typeOfTrigger
    );

    void SendRequestToCollectDids
    (
        uint32_t dtcCode,
        const uint8_t* supplierFaultCodePtr,
        size_t supplierFaultCodeSize
    );

    void IndicateToDataIdSvc
    (
        uint32_t dtcCode,
        std::vector<uint16_t> &list,
        const uint8_t* supplierFaultCodePtr,
        size_t supplierFaultCodeSize
    );

    void _triggerSnapshot
    (
        uint32_t dtcCode,
        cfg::freeze_frame_trigger_type_t typeOfTrigger,
        const uint8_t* supplierFaultCodePtr,
        size_t supplierFaultCodeSize
    );

};

}

#endif /* TAF_SNAPSHOT_SVR_HPP */
