
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafSnapshotSvc.hpp"

using namespace telux::tafsvc;
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

    cfg::Node & ff = dtc.get_child("identification.freeze_frames");
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
        size_t didValLen = cfg::get_did_value_size(did);
        if (pos + 2 + didValLen > dataSize)
        {
            LE_FATAL("Responsed DID raw data mismatch their configurations.");
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
        #define DID_OF_SUPPLIER_FC 0xEF01
        taf_DataAccess_DidNode_t * node =
                   (taf_DataAccess_DidNode_t*)le_mem_ForceAlloc(DidNodePool);
        node->did = (uint16_t) DID_OF_SUPPLIER_FC;
        size_t sfcSize = cfg::get_did_value_size((uint16_t) DID_OF_SUPPLIER_FC);
        if ((*it)->supplierFaultCodeSize != sfcSize)
        {
            LE_WARN("Supplier fault code DID size mismatch its configuration. "
                     "(cfg: %d != app: %d)",
                     (int) sfcSize, (int) (*it)->supplierFaultCodeSize);
        }
        node->val = (*it)->supplierFaultCodePtr; // Like other nodes, release in ReleaseList
        node->len = (*it)->supplierFaultCodeSize;
        node->link = LE_DLS_LINK_INIT;
        le_dls_Queue(list, &node->link);
    }

    int occurrence = 1;
    le_result_t ret = taf_DataAccess_SetSnapshotData(dtcCode, list, &occurrence, ReleaseList);

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
    string didType = dtc.get<string>("identification.snapshot_record_content");
    cfg::Node & root = cfg::get_root_node();
    cfg::Node & dids = root.get_child("data_indentifier_set").get_child(didType);
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
    if (! NeedToBeTriggered(dtcCode, typeOfTrigger))
    {
        LE_INFO("No matching type to trigger snapshot for dtc [0x%03X].", dtcCode);
        return;
    }

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
        LE_FATAL("Trigger snapshot fatal: %s", e.what());
    }
}
