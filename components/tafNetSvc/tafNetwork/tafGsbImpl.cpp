/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include "tafGsbImpl.hpp"
#include "tafSvcIF.hpp"

#define CONFIG_GSB_TIMEOUT 10

using namespace tafsvc;

le_sem_Ref_t tafGsbCallback::semaphore = nullptr;



/*======================================================================

 FUNCTION        taf_Gsb::Init

 DESCRIPTION     Initialization of the taf Gsb component

 DEPENDENCIES    The initialization of telaf.

 PARAMETERS      None

 RETURN VALUE    None

======================================================================*/
void taf_Gsb::Init(void)
{

    // Initiate the semaphore
    tafGsbCallback::semaphore = le_sem_Create("taf_GsbRespCbSem", 0);

    return;
}

/*======================================================================

 FUNCTION        taf_Gsb::GetInstance

 DESCRIPTION     Get the instance of gsb.

 DEPENDENCIES    The initialization of gsb.

 PARAMETERS      None

 RETURN VALUE    taf_Gsb &

======================================================================*/
taf_Gsb &taf_Gsb::GetInstance()
{
    static taf_Gsb instance;
    return instance;
}

/*======================================================================

 FUNCTION        taf_Gsb::AddGsb

 DESCRIPTION     Add generic software bridge configuration for an interface.

 DEPENDENCIES    The initialization of gsb.

 PARAMETERS      [IN] char* ifName : The interface name.
                 [IN] taf_net_GsbIfType_t ifType : The interface type.
                 [IN] uint32_t bandwidth : The bandwidth(in Mbps).

 RETURN VALUE    le_result_t
                     LE_OK:            Succeeded to add gsb
                     LE_BAD_PARAMETER: Invalid parameter.
                     LE_FAULT:         Failed to add gsb.

======================================================================*/
le_result_t taf_Gsb::AddGsb(const char* ifName, taf_net_GsbIfType_t ifType, uint32_t bandwidth)
{
    return LE_UNSUPPORTED;
}

/*======================================================================

 FUNCTION        taf_Gsb::RemoveGsb

 DESCRIPTION     Remove generic software bridge configuration for an interface.

 DEPENDENCIES    The initialization of gsb.

 PARAMETERS      [IN] char* ifName : The interface name.

 RETURN VALUE    le_result_t
                     LE_OK:            Succeeded to add gsb
                     LE_BAD_PARAMETER: Invalid parameter.
                     LE_FAULT:         Failed to add gsb.

======================================================================*/
le_result_t taf_Gsb::RemoveGsb(const char* ifName)
{
    return LE_UNSUPPORTED;
}

/*======================================================================

 FUNCTION        taf_Gsb::EnableGsb

 DESCRIPTION     Enable the generic software bridge.

 DEPENDENCIES    The initialization of gsb.

 PARAMETERS      [IN] bool enable : True for enable or false for disable gsb.

 RETURN VALUE    le_result_t
                     LE_OK:            Succeeded to enable gsb
                     LE_FAULT:         Failed to enable gsb.

======================================================================*/
le_result_t taf_Gsb::EnableGsb(bool enable)
{
    return LE_UNSUPPORTED;
}

/*======================================================================

 FUNCTION        taf_Gsb::GetGsbList

 DESCRIPTION     Get the reference of the generic software bridge list.

 DEPENDENCIES    The initialization of gsb.

 PARAMETERS      None.

 RETURN VALUE    taf_net_GsbListRef_t
                     nullptr:     Failure
                     non-nullptr: Success

======================================================================*/
taf_net_GsbListRef_t taf_Gsb::GetGsbList()
{
    return NULL;
}

/*======================================================================

 FUNCTION        taf_Gsb::GetFirstGsb

 DESCRIPTION     Get the reference of the first generic software bridge with a list reference.

 DEPENDENCIES    Initialization of a gsb list

 PARAMETERS      [IN] taf_net_GsbListRef_t gsbListRef: The gsb list reference.

 RETURN VALUE    taf_net_GsbRef_t
                     nullptr:     Failure
                     non-nullptr: Success

======================================================================*/
taf_net_GsbRef_t taf_Gsb::GetFirstGsb( taf_net_GsbListRef_t gsbListRef )
{
    return NULL;
}

/*======================================================================

 FUNCTION        taf_Gsb::GetNextGsb

 DESCRIPTION     Get the reference of the next gsb from a list.

 DEPENDENCIES    Initialization of a gsb list

 PARAMETERS      [IN] taf_net_GsbListRef_t gsbListRef: The gsb list reference.

 RETURN VALUE    taf_net_GsbRef_t
                     nullptr:     Failure
                     non-nullptr: Success

======================================================================*/
taf_net_GsbRef_t taf_Gsb::GetNextGsb( taf_net_GsbListRef_t gsbListRef )
{
    return NULL;
}

/*======================================================================

 FUNCTION        taf_Gsb::DeleteGsbList

 DESCRIPTION     Delete a reference of a gsb list.

 DEPENDENCIES    Initialization of a gsb list

 PARAMETERS      [IN] taf_net_GsbListRef_t gsbListRef: The gsb list reference.

 RETURN VALUE    le_result_t
                     LE_BAD_PARAMETER: Invalid parameters.
                     LE_NOT_FOUND:     Failure.
                     LE_OK:            Success.

======================================================================*/
le_result_t taf_Gsb::DeleteGsbList( taf_net_GsbListRef_t gsbListRef )
{
    return LE_UNSUPPORTED;
}


/*======================================================================

 FUNCTION        taf_Gsb::GetGsbInterfaceName

 DESCRIPTION     Get the interface name of a generic software bridge.

 DEPENDENCIES    Initialization of a gsb list and get a safe reference of a gsb.

 PARAMETERS      [IN] taf_net_GsbRef_t gsbRef: The gsb reference.
                 [OUT] char* ifNamePtr: The interface name.
                 [IN] size_t ifNamePtrSize: The interface name size.

 RETURN VALUE    le_result_t
                     LE_OK  Successful
                     LE_NOT_FOUND  Not found the interface name
                     LE_BAD_PARAMETER  Invalid parameter
                     LE_FAULT  Failed

======================================================================*/
le_result_t taf_Gsb::GetGsbInterfaceName
(
    taf_net_GsbRef_t gsbRef,
    char* ifNamePtr,
    size_t ifNamePtrSize
)
{
    return LE_UNSUPPORTED;
}

/*======================================================================

 FUNCTION        taf_Gsb::GetGsbInterfaceType

 DESCRIPTION     Get the interface type of a generic software bridge.

 DEPENDENCIES    Initialization of a gsb list and get a safe reference of a gsb.

 PARAMETERS      [IN] taf_net_GsbRef_t gsbRef: The gsb reference.

 RETURN VALUE    taf_net_GsbIfType_t
                               The interface type

======================================================================*/
taf_net_GsbIfType_t taf_Gsb::GetGsbInterfaceType
(
    taf_net_GsbRef_t gsbRef
)
{
    return TAF_NET_GSB_UNKNOWN;
}

/*======================================================================

 FUNCTION        taf_Gsb::GetGsbBandWidth

 DESCRIPTION     Get the band width of a generic software bridge.

 DEPENDENCIES    Initialization of a gsb list and get a safe reference of a gsb.

 PARAMETERS      [IN] taf_net_GsbRef_t gsbRef: The gsb reference.

 RETURN VALUE    int32_t
                     -1    ERROR
                     others bandWitdh

======================================================================*/
int32_t taf_Gsb::GetGsbBandWidth
(
    taf_net_GsbRef_t gsbRef
)
{
    return -1;
}