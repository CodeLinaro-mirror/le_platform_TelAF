
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafSnapshotSvc.hpp"

using namespace tafsvc;
using namespace std;

taf_SnapshotSvr::taf_SnapshotSvr()
{
    SnapshotItemPool = le_mem_CreatePool("SnapshotItemPool", sizeof(SnapshotItem_t));
    DidNodePool = le_mem_CreatePool("DidNodePool", sizeof(taf_DataAccess_DidNode_t));
    DidListPool = le_mem_CreatePool("DidListPool", sizeof(le_dls_List_t));

    #define DID_VAL_CONTAINER_SIZE 1024
    #define DID_VAL_TYPICAL_BYTES 32
    DidValPool = le_mem_CreatePool("DidValPoolOrig", DID_VAL_CONTAINER_SIZE);
    DidValPool = le_mem_CreateReducedPool(DidValPool, "DidValPool",
                                          0, DID_VAL_TYPICAL_BYTES);
}

taf_SnapshotSvr::~taf_SnapshotSvr() {}

void taf_SnapshotSvr::Init(void)
{
    LE_INFO("%s", __FUNCTION__);
}

taf_SnapshotSvr& taf_SnapshotSvr::GetInstance(void)
{
    static taf_SnapshotSvr svc;
    return svc;
}

bool taf_SnapshotSvr::NeedToBeTriggered
(
    uint32_t dtcCode,
    cfg::freeze_frame_trigger_type_t typeOfTrigger
)
{
    cfg::Node & dtc = cfg::get_dtc_node(dtcCode);

    cfg::Node & ff = dtc.get_child("snapshots.freeze_frames");
    for (auto & f : ff)
    {
        string item = f.second.get_value<string>("");

        cfg::Node & ffNode = cfg::top_freeze_frames<string>("short_name", item);
        string triggerType = ffNode.get<string>("trigger");

        if (typeOfTrigger == cfg::s_to_freeze_frame_trigger_type(triggerType))
        {
            return true;
        }
    }

    return false;
}

void taf_SnapshotSvr::ReleaseList(le_dls_List_t *list)
{
    le_dls_Link_t* linkPtr = le_dls_Pop(list);

    while (linkPtr != NULL)
    {
        taf_DataAccess_DidNode_t* didNode = CONTAINER_OF(linkPtr,
                                                         taf_DataAccess_DidNode_t,
                                                         link);
        le_mem_Release(didNode->val);
        le_mem_Release(didNode);

        linkPtr = le_dls_Pop(list);
    }

    le_mem_Release(list);
}

void taf_SnapshotSvr::storeDidsAsSnapshot
(
    const uint8_t* didRawData,
    size_t dataSize,
    taf_ReadDIDRxMsg_t* msgPtr
)
{
    uint32_t dtcCode = 0;

    if (didRawData == NULL || dataSize == 0)
    {
        LE_ERROR("Invalid raw did data for snapshot.");
        return;
    }

    if (dataSize < 2)
    {
        LE_ERROR("None valid did code.");
        return;
    }

    if (SnapshotTriggeredList.size() == 0)
    {
        LE_ERROR("Why is the snapshot triggered? skip it.");
        return;
    }

    auto it = find_if(SnapshotTriggeredList.begin(), SnapshotTriggeredList.end(),
                      [&msgPtr](const SnapshotItem_t* item) {
                          return  item != NULL && item->msgPtr == msgPtr;
                      });

    if (it == SnapshotTriggeredList.end())
    {
        LE_ERROR("Snapshot triggered, but not found corresponding ref, ignore it.");
        return;
    }

    dtcCode = (*it)->dtcCode;

    le_dls_List_t * list = (le_dls_List_t *) le_mem_ForceAlloc(DidListPool);
    memset(list, 0, sizeof(le_dls_List_t));

    size_t pos = 0;
    while (pos < dataSize)
    {
        uint16_t did = (uint16_t) (didRawData[pos] << 8 | didRawData[pos+1]);
        bool isDidValid = false;

        for(int i=0; i < msgPtr->readDIDLen; i++)
        {
            if(did == msgPtr->readDID[i])
            {
                isDidValid = true;
                break;
            }
        }

        if(!isDidValid)
        {
            LE_ERROR("invalid DID: 0x%x, snapshot data is not correct.", did);
            ReleaseList(list);
            return;
        }

        size_t didValLen = cfg::get_did_value_size(did);
        if (pos + 2 + didValLen > dataSize)
        {
            LE_ERROR("Responsed DID raw data mismatch their configurations.");
            ReleaseList(list);
            return;
        }
        uint8_t *didVal = (uint8_t *) le_mem_ForceVarAlloc(DidValPool, didValLen);
        memcpy(didVal, didRawData + pos + 2, didValLen);

        taf_DataAccess_DidNode_t * node =
                   (taf_DataAccess_DidNode_t*)le_mem_ForceAlloc(DidNodePool);
        node->did = did;
        node->val = didVal;
        node->len = didValLen;
        node->link = LE_DLS_LINK_INIT;

        le_dls_Queue(list, &node->link);

        pos += 2 + didValLen;
    }

    if ( (*it)->supplierFaultCodePtr  != NULL &&
         (*it)->supplierFaultCodeSize != 0 )
    {
        size_t sfcSizeFromConf = cfg::get_did_value_size((uint16_t) DID_OF_SUPPLIER_FC);
        size_t sfcSizePassedIn = (*it)->supplierFaultCodeSize;

        if (sfcSizePassedIn > sfcSizeFromConf)
        {
            LE_ERROR("Bad Supplier Fault Code Size: (PASSIN: %" PRIuS ") > (YAML:%" PRIuS ")",
                     sfcSizePassedIn, sfcSizeFromConf);
            ReleaseList(list);
            return;
        }

        taf_DataAccess_DidNode_t * node =
                   (taf_DataAccess_DidNode_t*)le_mem_ForceAlloc(DidNodePool);

        if (sfcSizePassedIn < sfcSizeFromConf)
        {
            LE_WARN("Mismatched Supplier Fault Code Size: (PASSIN: %" PRIuS ") < (YAML:%" PRIuS ")",
                     sfcSizePassedIn, sfcSizeFromConf);

            {
                LE_DEBUG("Original SFC:");
                for (size_t i = 0; i < sfcSizePassedIn; i++)
                {
                    LE_DEBUG("[%d] %02X", (uint32_t)i, ((*it)->supplierFaultCodePtr)[i]);
                }
            }

            uint8_t *sfcNewVal = (uint8_t *) le_mem_ForceVarAlloc(DidValPool, sfcSizeFromConf);
            size_t diffSize = sfcSizeFromConf - sfcSizePassedIn;

            memset(sfcNewVal, 0x00, sfcSizeFromConf); // Set all bytes to ZERO

            memcpy(sfcNewVal + diffSize,
                   (*it)->supplierFaultCodePtr,
                   sfcSizePassedIn); // Put the bytes in the correct place

            le_mem_Release((*it)->supplierFaultCodePtr); // Release previous did-val pointer

            {
                LE_DEBUG("New padding SFC:");
                for (size_t i = 0; i < sfcSizeFromConf; i++)
                {
                    LE_DEBUG("[%d] %02X", (uint32_t)i, sfcNewVal[i]);
                }
            }

            node->val = sfcNewVal;
            node->len = sfcSizeFromConf;
        }
        else
        {
            node->val = (*it)->supplierFaultCodePtr; // Like other nodes, release in ReleaseList
            node->len = (*it)->supplierFaultCodeSize;
        }

        node->did = (uint16_t) DID_OF_SUPPLIER_FC;
        node->link = LE_DLS_LINK_INIT;
        le_dls_Queue(list, &node->link);
    }

#ifdef LE_CONFIG_DIAG_FEATURE_A
    // Deal with the Occurrence Counter storage
    {
        #define DID_OF_OCCURRENCE_COUNTER 0xF0D1
        uint8_t occurrenceCounter = taf_DataAccess_GetDTCOccurrenceCounter(dtcCode);
        size_t occCntSize = sizeof(occurrenceCounter);

        taf_DataAccess_DidNode_t * node =
                    (taf_DataAccess_DidNode_t*)le_mem_ForceAlloc(DidNodePool);

        uint8_t *occCntVal = (uint8_t *) le_mem_ForceVarAlloc(DidValPool, occCntSize);
        memcpy(occCntVal, &occurrenceCounter, occCntSize);

        node->did = (uint16_t) DID_OF_OCCURRENCE_COUNTER;
        node->val = occCntVal;
        node->len = occCntSize;
        node->link = LE_DLS_LINK_INIT;

        LE_INFO("Push [occurrence counter] to Freeze Frame...");
        le_dls_Queue(list, &node->link);
    }
#endif
    int occurrence = 1;
    le_result_t ret = taf_DataAccess_SetSnapshotData(dtcCode, list, &occurrence);
    ReleaseList(list);
    le_mem_Release(*it);
    SnapshotTriggeredList.erase(it);

    if (ret != LE_OK)
    {
        LE_ERROR("Store the snapshot to DB: FAILED.");
    }
}

void taf_SnapshotSvr::IndicateToDataIdSvc
(
    uint32_t dtcCode,
    std::vector<uint16_t> &list,
    const uint8_t* supplierFaultCodePtr,
    size_t supplierFaultCodeSize
)
{
    /* Event report is used by SnapshotTriggerTheCollectionOfDIDs
     * It can be used to handle multi requests */
    taf_ReadDIDRxMsg_t* msgPtr = NULL;
    le_result_t ret = taf_DataIDSvr::GetInstance().
                        SnapshotTriggerTheCollectionOfDIDs(list.data(),
                                                  (size_t) list.size(),
                                                           &msgPtr);
    if (ret == LE_OK)
    {
        SnapshotItem_t * ssi = (SnapshotItem_t *)le_mem_ForceAlloc(SnapshotItemPool);
        memset(ssi, 0, sizeof(*ssi));
        ssi->dtcCode = dtcCode;
        ssi->msgPtr = msgPtr;
        if (supplierFaultCodePtr != NULL && supplierFaultCodeSize != 0)
        {
            ssi->supplierFaultCodePtr =
              (uint8_t *) le_mem_ForceVarAlloc(DidValPool,supplierFaultCodeSize);
            memcpy(ssi->supplierFaultCodePtr,
                   supplierFaultCodePtr,
                   supplierFaultCodeSize);
            ssi->supplierFaultCodeSize = supplierFaultCodeSize;
        }

        SnapshotTriggeredList.push_back(ssi);
    }
    else
    {
        LE_INFO("No registered apps to collect the DIDs for Snapshot.");
    }
}

void taf_SnapshotSvr::SendRequestToCollectDids
(
    uint32_t dtcCode,
    const uint8_t* supplierFaultCodePtr,
    size_t supplierFaultCodeSize
)
{
    cfg::Node & dtc = cfg::get_dtc_node(dtcCode);
    string didType = dtc.get<string>("snapshots.snapshot_record_content");
    cfg::Node & root = cfg::get_root_node();
    cfg::Node & dids = root.get_child("data_identifier_set").get_child(didType);
    std::vector<uint16_t> didRequestList;

    for (auto & did : dids)
    {
        uint16_t didCode = (uint16_t) did.second.get_value<int>();
        didRequestList.push_back(didCode);
    }

    LE_DEBUG("DID size: %d", (int) didRequestList.size());
    for (auto didCode : didRequestList)
    {
        LE_DEBUG("DID: 0x%02X", didCode);
    }

    IndicateToDataIdSvc(dtcCode,
                        didRequestList,
                        supplierFaultCodePtr,
                        supplierFaultCodeSize);
}

void taf_SnapshotSvr::_triggerSnapshot
(
    uint32_t dtcCode,
    cfg::freeze_frame_trigger_type_t typeOfTrigger,
    const uint8_t* supplierFaultCodePtr,
    size_t supplierFaultCodeSize
)
{
#ifndef LE_CONFIG_DIAG_FEATURE_A
    if (! NeedToBeTriggered(dtcCode, typeOfTrigger))
    {
        LE_INFO("No matching type to trigger snapshot for dtc [0x%03X].", dtcCode);
        return;
    }
#endif
    SendRequestToCollectDids(dtcCode,
                             supplierFaultCodePtr,
                             supplierFaultCodeSize);
}

void taf_SnapshotSvr::triggerSnapshot
(
    uint32_t dtcCode,
    cfg::freeze_frame_trigger_type_t typeOfTrigger,
    const uint8_t* supplierFaultCodePtr,
    size_t supplierFaultCodeSize
)
{
    try
    {
        _triggerSnapshot(dtcCode,
                         typeOfTrigger,
                         supplierFaultCodePtr,
                         supplierFaultCodeSize);
    }
    catch (const std::exception& e)
    {
        LE_ERROR("Trigger snapshot fatal: %s <--", e.what());
    }
}
