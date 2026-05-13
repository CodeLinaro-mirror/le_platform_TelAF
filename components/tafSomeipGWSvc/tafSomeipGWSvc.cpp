/*
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "legato.h"
#include "interfaces.h"
#include "tafSomeipGWSvc.hpp"
#include "tafSomeipSvr.hpp"
#include "tafSomeipClnt.hpp"
#include "tafSvcIF.hpp"
#include "jansson.h"

#define VSOMEIP_APP_MAX_CNT 8
#define VSOMEIP_APP_NAME "tafSomeipGWSvc"
#define ROUTING_INTF_NAME_SIZE 32
#define ROUTING_IP_ADDR_SIZE 48
#define MAX_ADD_ROUTE_RETRIES 5

using namespace tafsvc;

class taf_vsomeipApp
{
    public:
        taf_vsomeipApp(const uint8_t idx, const std::string appName, const std::string devName,
                            const std::string uniAddr, const std::string multiAddr):
            app(vsomeip::runtime::get()->create_application(appName)),
            routingId(idx),
            routingName(appName),
            deviceName(devName),
            unicastAddr(uniAddr),
            multicastAddr(multiAddr),
            routeAdded(false),
            addRouteRetryCount(0)
        {
        };
        ~taf_vsomeipApp()
        {
        };
        bool init()
        {
            if (!app->init())
            {
                LE_ERROR("Couldn't initialize routing manager '%s'.", routingName.c_str());
                return false;
            }
            app->register_state_handler(std::bind(&taf_vsomeipApp::onState,
                                        this, std::placeholders::_1));
            app->register_message_handler(vsomeip::ANY_SERVICE,
                                          vsomeip::ANY_INSTANCE,
                                          vsomeip::ANY_METHOD,
                                          std::bind(&taf_vsomeipApp::onMessage,
                                          this, std::placeholders::_1));

            vsClientId = app->get_client();

            LE_INFO("vsomeip routing manager '%s'" \
                    "(idx=%d, clientID=0x%x, unicast='%s', multicast='%s', intf='%s') initialized.",
                    routingName.c_str(), routingId, vsClientId, unicastAddr.c_str(),
                    multicastAddr.c_str(), deviceName.c_str());

            return true;
        }
        void start()
        {
            app->start();
        }
        void stop()
        {
            app->clear_all_handler();
            app->stop();
        }
        std::shared_ptr<vsomeip::application>& getApp()
        {
            return app;
        }
        const std::string& getIntfName() const
        {
            return deviceName;
        }
        const std::string& getMulticastAddr() const
        {
            return multicastAddr;
        }
        const std::string& getRoutingName() const
        {
            return routingName;
        }
        uint8_t getRoutingId() const
        {
            return routingId;
        }
        uint16_t getClientId() const
        {
            return vsClientId;
        }
        bool isRouteAdded() const
        {
            return routeAdded;
        }
        void setRouteAdded(bool added)
        {
            routeAdded = added;
        }
        int32_t getAddRouteRetryCount() const
        {
            return addRouteRetryCount;
        }
        void incrementAddRouteRetryCount()
        {
            addRouteRetryCount++;
        }
        void resetAddRouteRetryCount()
        {
            addRouteRetryCount = 0;
        }
        void onState(vsomeip::state_type_e state)
        {
            if (state == vsomeip::state_type_e::ST_REGISTERED)
            {
                LE_INFO("Routing manager '%s' is registered.", routingName.c_str());
            }
            else if (state == vsomeip::state_type_e::ST_DEREGISTERED)
            {
                LE_INFO("Routing manager '%s' is de-registered.", routingName.c_str());
            }
        }
        void onMessage(const std::shared_ptr<vsomeip::message> &msg)
        {
            vsomeip::message_type_e msgType = msg->get_message_type();
            vsomeip::length_t msgLen = msg->get_payload()->get_length();
            vsomeip::method_t methodId = msg->get_method();
#if 0
            LE_DEBUG("VSOMEIP message(len=%" PRIu32 ")with Client/Session/Type[0x%x/0x%x/0x%x].",
                     msgLen, msg->get_client(), msg->get_session(), (uint32_t)msgType);
#endif
            // Sanity check for the payload size.
            if (msgLen > TAF_SOMEIPDEF_MAX_PAYLOAD_SIZE)
            {
                LE_WARN("VSOMEIP message size is too long, dropped it.");
                return;
            }
            // Sanity check for request message for server instance.
            if (((vsomeip::message_type_e::MT_REQUEST == msgType) ||
                (vsomeip::message_type_e::MT_REQUEST_NO_RETURN == msgType)) &&
                !(methodId & TAF_SOMEIPDEF_EVENT_MASK))
            {
                taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
                mySomeipSvr.VSOMEIPHandler(routingId, msg);
                return;
            }
            // Sanity check for a response for client instance.
            else if (((vsomeip::message_type_e::MT_RESPONSE == msgType) ||
                (vsomeip::message_type_e::MT_ERROR == msgType)) &&
                !(methodId & TAF_SOMEIPDEF_EVENT_MASK))
            {
                taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
                mySomeipClient.VSOMEIPRespHandler(routingId, msg);
                return;
            }
            // Sanity check for an event for client instance.
            else if ((vsomeip::message_type_e::MT_NOTIFICATION == msgType) &&
                     (methodId & TAF_SOMEIPDEF_EVENT_MASK))
            {
                taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
                mySomeipClient.VSOMEIPEventHandler(routingId, msg);
                return;
            }
        }

        le_thread_Ref_t someipThreadRef;

    private:
        std::shared_ptr<vsomeip::application> app;
        uint16_t vsClientId;
        uint8_t routingId;
        std::string routingName;
        std::string deviceName;
        std::string unicastAddr;
        std::string multicastAddr;
        bool routeAdded;
        int32_t addRouteRetryCount;
};

//--------------------------------------------------------------------------------------------------
/**
 * Routing Configuration entry data struct.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char intfName[ROUTING_INTF_NAME_SIZE];
    char unicastAddr[ROUTING_IP_ADDR_SIZE];
    char multicastAddr[ROUTING_IP_ADDR_SIZE];
}RoutingConfig_t;

//--------------------------------------------------------------------------------------------------
/**
 * Routing Manager Tables
 */
//--------------------------------------------------------------------------------------------------
static taf_vsomeipApp* RoutingManagerTable[VSOMEIP_APP_MAX_CNT] = { NULL };

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore to indicate all vsomeip apps have stopped
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t VsomeipStopSem = NULL;
static int32_t ActiveVsomeipAppCount = 0;
static le_mutex_Ref_t VsomeipCountMutex = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Auto add route flag based on AUTO_ADD_ROUTE environment variable
 */
//--------------------------------------------------------------------------------------------------
static bool AutoAddRouteFlag = false;

//--------------------------------------------------------------------------------------------------
/**
 * Get the vsomeip routing manager instance by routing ID.
 */
//--------------------------------------------------------------------------------------------------
std::shared_ptr<vsomeip::application>& someip_GetRoutingManager
(
    uint8_t id
)
{
    // Sanity check for the ID range.
    LE_ASSERT((id < VSOMEIP_APP_MAX_CNT) && (RoutingManagerTable[id] != NULL));

    return RoutingManagerTable[id]->getApp();
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the vsomeip client ID by routing ID.
 */
//--------------------------------------------------------------------------------------------------
uint16_t someip_GetClientId
(
    uint8_t id
)
{
    // Sanity check for the ID range.
    LE_ASSERT((id < VSOMEIP_APP_MAX_CNT) && (RoutingManagerTable[id] != NULL));

    return RoutingManagerTable[id]->getClientId();
}

//--------------------------------------------------------------------------------------------------
/**
 * Get routing ID by interface name.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetRoutingIdByIntfName
(
    const char* intfName,
    uint8_t* idPtr
)
{
    if ((intfName == NULL) || (idPtr == NULL))
    {
        return LE_BAD_PARAMETER;
    }

    for (uint8_t id = 0; id < VSOMEIP_APP_MAX_CNT; id++)
    {
        if ((RoutingManagerTable[id] != NULL) &&
            (strcmp(RoutingManagerTable[id]->getIntfName().c_str(), intfName) == 0))
        {
            *idPtr = RoutingManagerTable[id]->getRoutingId();
            return LE_OK;
        }
    }

    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 * Validate routing info in JSON file.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ValidateRoutingInfo
(
    const json_t* root,
    const char* appNamePtr
)
{
    json_t *js_applications = NULL;
    json_t *js_application = NULL;
    json_t *js_appId = NULL;
    json_t *js_appName = NULL;
    json_t *js_routing = NULL;
    json_t *js_network = NULL;

    size_t index = 0;
    bool isRoutingFound = false;

    if ((root == NULL) || (appNamePtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    // Load the 'applications' array.
    js_applications = json_object_get(root, "applications");
    if ((!json_is_array(js_applications)) || (json_array_size(js_applications) == 0))
    {
        LE_ERROR("applications array is not set in JSON file of routing manager %s.", appNamePtr);
        return LE_FAULT;
    }

    // Get each application entry in the array.
    json_array_foreach(js_applications, index, js_application)
    {
        // Get "id", "name" for each application entry.
        js_appId = json_object_get(js_application, "id");
        js_appName = json_object_get(js_application, "name");
        if (json_is_string(js_appId) && json_is_string(js_appName))
        {
            // Validate the app name.
            if (strcmp(json_string_value(js_appName), appNamePtr) == 0)
            {
                isRoutingFound = true;
                break;
            }
        }
    }

    if (!isRoutingFound)
    {
        LE_ERROR("application name is not set in JSON file of routing manager %s.", appNamePtr);
        return LE_FAULT;
    }

    // Load 'network' and 'routing'.
    js_network = json_object_get(root, "network");
    js_routing = json_object_get(root, "routing");
    if (json_is_string(js_network) && json_is_string(js_routing))
    {
        // Validate the network and routing.
        if ((strcmp(json_string_value(js_network), appNamePtr) == 0)&&
            (strcmp(json_string_value(js_routing), appNamePtr) == 0))
        {
            return LE_OK;
        }
    }

    LE_ERROR("network or routing is not set in JSON file of routing manager %s.", appNamePtr);
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Load JSON file and Parse routing information.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadRoutingConfigFile
(
    const char* appNamePtr,
    RoutingConfig_t* configInfoPtr
)
{
    json_t *root;
    json_error_t error;
    char fileName[64] = "";

    if ((appNamePtr == NULL) || (configInfoPtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    // Validate file name size.
    if (snprintf(fileName, sizeof(fileName), "%s.json", appNamePtr) >= (int)sizeof(fileName))
    {
        LE_ERROR("routing manager name '%s' is too long.", appNamePtr);
        return LE_BAD_PARAMETER;
    }

    // Check if file exists.
    struct stat fileStatus;
    if ((stat(fileName, &fileStatus) != 0) || (!S_ISREG(fileStatus.st_mode)))
    {
        return LE_NOT_FOUND;
    }

    // Load entire JSON file.
    root = json_load_file(fileName, 0, &error);
    if (root == NULL)
    {
        LE_ERROR("JSON file error: line: %d, column: %d, position: %d, source: '%s', error: %s",
                 error.line, error.column, error.position, error.source, error.text);
        return LE_NOT_FOUND;
    }

    // Check if "root" is an object.
    if (!json_is_object(root))
    {
        LE_ERROR("root is not an object.");
        json_decref(root);
        return LE_FAULT;
    }

    // Validate Routing info.
    if (LE_OK != ValidateRoutingInfo(root, appNamePtr))
    {
        json_decref(root);
        return LE_FAULT;
    }

    // Get the 'unicast' 'multicast' and 'device'.
    memset(configInfoPtr, 0, sizeof(RoutingConfig_t));
    json_t* js_unicast;
    json_t* js_device;
    json_t* js_multicast;

    js_unicast = json_object_get(root, "unicast");
    js_device = json_object_get(root, "device");
    js_multicast = json_object_get(json_object_get(root, "service-discovery"), "multicast");
    if ((!json_is_string(js_unicast)) || (!json_is_string(js_device)) ||
        (!json_is_string(js_multicast)))
    {
        LE_ERROR("unicast/device/multicast is not set in JSON file of routing manager %s",
                 appNamePtr);
        json_decref(root);
        return LE_FAULT;
    }

    le_utf8_Copy(configInfoPtr->unicastAddr, json_string_value(js_unicast),
                 ROUTING_IP_ADDR_SIZE, NULL);
    le_utf8_Copy(configInfoPtr->intfName, json_string_value(js_device),
                 ROUTING_INTF_NAME_SIZE, NULL);
    le_utf8_Copy(configInfoPtr->multicastAddr, json_string_value(js_multicast),
                 ROUTING_IP_ADDR_SIZE, NULL);

    json_decref(root);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * VSOMEIP dedicated entry thread.
 */
//--------------------------------------------------------------------------------------------------
static void* VSOMEIPThread
(
    void* contextPtr
)
{
    taf_vsomeipApp* myRoutingMgrPtr = (taf_vsomeipApp*)contextPtr;
    LE_ASSERT(myRoutingMgrPtr != NULL);

    LE_INFO("vsomeip routing manager '%s' started.", myRoutingMgrPtr->getRoutingName().c_str());

    le_mutex_Lock(VsomeipCountMutex);
    ActiveVsomeipAppCount++;
    le_mutex_Unlock(VsomeipCountMutex);

    myRoutingMgrPtr->start();

    LE_WARN("vsomeip routing manager '%s' exited.", myRoutingMgrPtr->getRoutingName().c_str());

    le_mutex_Lock(VsomeipCountMutex);
    ActiveVsomeipAppCount--;
    if (ActiveVsomeipAppCount == 0)
    {
        le_sem_Post(VsomeipStopSem);
    }
    le_mutex_Unlock(VsomeipCountMutex);

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create and start default routing manager.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartDefaultRoutingManager
(
    void
)
{
    taf_vsomeipApp* routePtr = NULL;
    RoutingConfig_t routingInfo;

    // Load default routing manager config file.
    memset(&routingInfo, 0, sizeof(RoutingConfig_t));
    if (LE_OK == LoadRoutingConfigFile(VSOMEIP_APP_NAME, &routingInfo))
    {
        // Create and start default routing manager.
        routePtr = new taf_vsomeipApp(0, VSOMEIP_APP_NAME, routingInfo.intfName,
                                      routingInfo.unicastAddr, routingInfo.multicastAddr);
        if ((routePtr != NULL) && routePtr->init())
        {
            RoutingManagerTable[0] = routePtr;
            routePtr->someipThreadRef =
                le_thread_Create(VSOMEIP_APP_NAME, VSOMEIPThread, (void*)routePtr);
            le_thread_Start(routePtr->someipThreadRef);

            return LE_OK;
        }
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create and start additional routing managers.
 */
//--------------------------------------------------------------------------------------------------
static void StartAdditionalRoutingManagers
(
    void
)
{
    uint8_t idx;
    taf_vsomeipApp* routePtr = NULL;
    RoutingConfig_t routingInfo;
    char appName[32] = "";

    for (idx = 1; idx < VSOMEIP_APP_MAX_CNT; idx++)
    {
        // Load additional routing manager config file.
        memset(&routingInfo, 0, sizeof(RoutingConfig_t));
        snprintf(appName, sizeof(appName), "%s_%u", VSOMEIP_APP_NAME, idx);
        if (LE_OK != LoadRoutingConfigFile(appName, &routingInfo))
        {
            continue;
        }

        // Create and start additional routing manager.
        routePtr = new taf_vsomeipApp(idx, appName, routingInfo.intfName,
                                  routingInfo.unicastAddr, routingInfo.multicastAddr);
        if ((routePtr != NULL) && routePtr->init())
        {
            RoutingManagerTable[idx] = routePtr;
            routePtr->someipThreadRef =
                le_thread_Create(appName, VSOMEIPThread, (void*)routePtr);
            le_thread_Start(routePtr->someipThreadRef);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set IPv4 multicast route with ioctl.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetIpv4MulticastRouteWithIoctl
(
    const char *destAddrPtr,
    const char *intfPtr,
    bool isAdd
)
{
    int32_t sockfd;
    struct rtentry rt;
    struct in_addr destAddr;

    if ((destAddrPtr == NULL) || (intfPtr == NULL))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    // Check if destAddrPtr is a valid IPv4 address and convert it to in_addr structure
    if (inet_aton(destAddrPtr, &destAddr) == 0)
    {
        LE_ERROR("destAddrPtr inet_aton error");
        return LE_BAD_PARAMETER;
    }

    // Check if the address is within the multicast address range
    if (destAddr.s_addr < inet_addr("224.0.0.0") ||
        destAddr.s_addr > inet_addr("239.255.255.255"))
    {
        LE_ERROR("destAddrPtr is not a valid multicast address.");
        return LE_BAD_PARAMETER;
    }

    memset(&rt, 0, sizeof(struct rtentry));

    ((struct sockaddr_in *)&rt.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *)&rt.rt_dst)->sin_addr = destAddr;
    ((struct sockaddr_in *)&rt.rt_gateway)->sin_family = AF_INET;
    ((struct sockaddr_in *)&rt.rt_gateway)->sin_addr.s_addr = inet_addr("0.0.0.0");
    ((struct sockaddr_in *)&rt.rt_genmask)->sin_family = AF_INET;
    ((struct sockaddr_in *)&rt.rt_genmask)->sin_addr.s_addr = inet_addr("255.255.255.255");

    rt.rt_dev = (char*)intfPtr;
    rt.rt_flags = RTF_UP;
    rt.rt_metric = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        LE_ERROR("socket error");
        return LE_FAULT;
    }

    if (isAdd)
    {
        if (ioctl(sockfd, SIOCADDRT, &rt) < 0)
        {
            if (errno == EEXIST)
            {
                // Route was already created by another process; return LE_DUPLICATE.
                LE_INFO("Route already exists for destination, created by another process.");
                close(sockfd);
                return LE_DUPLICATE;
            }
            LE_ERROR("Failed to add route, error:%s", strerror(errno));
            close(sockfd);
            return LE_FAULT;
        }
    }
    else
    {
        if (ioctl(sockfd, SIOCDELRT, &rt) < 0)
        {
            LE_ERROR("Failed to delete route, error:%s", strerror(errno));
            close(sockfd);
            return LE_FAULT;
        }
    }

    close(sockfd);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler for retrying the addition of routing for multicast address.
 */
//--------------------------------------------------------------------------------------------------
void TimerHandler
(
    le_timer_Ref_t timerRef
)
{
    taf_vsomeipApp* app = static_cast<taf_vsomeipApp*>(le_timer_GetContextPtr(timerRef));
    const char* multicastAddr = app->getMulticastAddr().c_str();
    const char* intfName = app->getIntfName().c_str();

    app->incrementAddRouteRetryCount();

    le_result_t result = SetIpv4MulticastRouteWithIoctl(multicastAddr, intfName, true);
    if (result == LE_OK)
    {
        LE_INFO("Retry %d: Completed adding route for %s on %s.", app->getAddRouteRetryCount(),
            multicastAddr, intfName);
        app->setRouteAdded(true);
        app->resetAddRouteRetryCount();
        le_timer_Delete(timerRef);
    }
    else if (result == LE_DUPLICATE)
    {
        // Route was already created by another process; stop retrying.
        LE_INFO("Retry %d: Route for %s on %s already exists, created by another process.",
            app->getAddRouteRetryCount(), multicastAddr, intfName);
        app->resetAddRouteRetryCount();
        le_timer_Delete(timerRef);
    }
    else
    {
        LE_WARN("Retry %d: Failed to add route for %s on %s.", app->getAddRouteRetryCount(),
            multicastAddr, intfName);

        if (app->getAddRouteRetryCount() >= MAX_ADD_ROUTE_RETRIES)
        {
            LE_ERROR("Failed to add route for %s on %s after maximum retries of %d.",
                multicastAddr, intfName, app->getAddRouteRetryCount());
            app->resetAddRouteRetryCount();
            le_timer_Delete(timerRef);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Add routing for multicast addresses.
 */
//--------------------------------------------------------------------------------------------------
static void AddRoutingForMulticastAddr
(
    void
)
{
    for (uint8_t id = 0; id < VSOMEIP_APP_MAX_CNT; id++)
    {
        if (RoutingManagerTable[id] != NULL)
        {
            const char* multicastAddr = RoutingManagerTable[id]->getMulticastAddr().c_str();
            const char* intfName = RoutingManagerTable[id]->getIntfName().c_str();
            char timerName[64];
            snprintf(timerName, sizeof(timerName), "AddRouteRetryTimer_%d", id);

            le_result_t result = SetIpv4MulticastRouteWithIoctl(multicastAddr, intfName, true);
            if (result == LE_OK)
            {
                LE_INFO("Completed adding route for %s on %s.", multicastAddr, intfName);
                RoutingManagerTable[id]->setRouteAdded(true);
            }
            else if (result == LE_DUPLICATE)
            {
                // Route was already created by another process.
                LE_INFO("Route for %s on %s already exists, created by another process.",
                    multicastAddr, intfName);
            }
            else
            {
                LE_WARN("Failed to add route for %s on %s initially.", multicastAddr, intfName);

                le_timer_Ref_t timerRef = le_timer_Create(timerName);
                le_timer_SetMsInterval(timerRef, 1000);
                le_timer_SetHandler(timerRef, TimerHandler);
                le_timer_SetRepeat(timerRef, MAX_ADD_ROUTE_RETRIES);
                le_timer_SetContextPtr(timerRef, RoutingManagerTable[id]);
                le_timer_Start(timerRef);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete routing for multicast addresses.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteRoutingForMulticastAddr
(
    void
)
{
    for (uint8_t id = 0; id < VSOMEIP_APP_MAX_CNT; id++)
    {
        if ((RoutingManagerTable[id] != NULL) && (RoutingManagerTable[id]->isRouteAdded()))
        {
            const char* multicastAddr = RoutingManagerTable[id]->getMulticastAddr().c_str();
            const char* intfName = RoutingManagerTable[id]->getIntfName().c_str();

            if (LE_OK == SetIpv4MulticastRouteWithIoctl(multicastAddr, intfName, false))
            {
                LE_INFO("Completed deleting route for %s on %s.", multicastAddr, intfName);
                RoutingManagerTable[id]->setRouteAdded(false);
            }
            else
            {
                LE_ERROR("Failed to delete route for %s on %s.", multicastAddr, intfName);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop all vsomeip applications
 */
//--------------------------------------------------------------------------------------------------
static void StopVsomeipApplication
(
    void
)
{
    for (uint8_t id = 0; id < VSOMEIP_APP_MAX_CNT; id++)
    {
        if (RoutingManagerTable[id] != NULL)
        {
            LE_INFO("Starting to stop vsomeip application id: %u", id);
            RoutingManagerTable[id]->stop();
        }
    }

    le_clk_Time_t timeToWait = {1, 0};
    if (le_sem_WaitWithTimeOut(VsomeipStopSem, timeToWait) == LE_OK)
    {
        LE_INFO("Obtained vsomeip app stop semaphore.");
    }
    else
    {
        LE_ERROR("Timeout occurred while waiting for vsomeip app stop semaphore");
    }
    le_sem_Delete(VsomeipStopSem);

}

//--------------------------------------------------------------------------------------------------
/**
 * Handle safe unloading of the application.
 */
//--------------------------------------------------------------------------------------------------
static void SafeUnloadHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    StopVsomeipApplication();
    if (AutoAddRouteFlag)
    {
        DeleteRoutingForMulticastAddr();
    }

    LE_INFO("Completed unloading.");
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Signal handler for SIGTERM
 */
//--------------------------------------------------------------------------------------------------
static void TafSigTermEventHandler
(
    int sigNum
)
{
    LE_INFO("TafSigTermEventHandler :%d", sigNum);
    le_event_QueueFunction(SafeUnloadHandler, NULL, NULL);
}

static void SetBootKpiMarker(const char* markerPtr){
    const char *kpi_file = "/sys/kernel/boot_kpi/kpi_values";
    FILE *file = fopen(kpi_file, "w");
    if (file == NULL)
    {
        LE_WARN("%s not able to open due to %s", kpi_file, strerror(errno));
        return;
    }
    if (fwrite(markerPtr, sizeof(char), strlen(markerPtr), file) != strlen(markerPtr))
    {
        LE_ERROR("failed to write %s to %s", markerPtr, kpi_file);
    }
    fclose(file);
}

//--------------------------------------------------------------------------------------------------
/**
 * The initialization of TelAF SOME/IP GW service component.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Create and start TelAF SOME/IP client and server proxies.
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    mySomeipSvr.Init();
    mySomeipClient.Init();

    // Create vsomeip app stop semaphore.
    VsomeipStopSem = le_sem_Create("VsomeipStopSem", 0);
    VsomeipCountMutex = le_mutex_CreateNonRecursive("VsomeipCountMutex");

    // Set auto add route flag.
    const char* autoAddRoute = getenv("AUTO_ADD_ROUTE");
    if (autoAddRoute != NULL && (strcmp(autoAddRoute, "YES") == 0 || strcmp(autoAddRoute, "ON") == 0
        || strcmp(autoAddRoute, "1") == 0))
    {
        AutoAddRouteFlag = true;
    }

    // Create routing managers.
    memset(RoutingManagerTable, 0, sizeof(RoutingManagerTable));
    if (LE_OK != StartDefaultRoutingManager())
    {
        LE_FATAL("Failed to start default routing manager.");
    }
    StartAdditionalRoutingManagers();

    // Add routing for multicast address.
    if (AutoAddRouteFlag)
    {
        AddRoutingForMulticastAddr();
    }

    le_sig_SetEventHandler(SIGTERM, TafSigTermEventHandler);

    LE_INFO("TelAF SOME/IP GateWay Service initialized.");
    // Add boot KPI marker
    SetBootKpiMarker("L - TelAF SomeipGW service is ready");
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference to a server-service-instance on default network interface.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to represent the service.
 */
//--------------------------------------------------------------------------------------------------
taf_someipSvr_ServiceRef_t taf_someipSvr_GetService
(
    uint16_t serviceId,
        ///< [IN] Service ID
    uint16_t instanceId
        ///< [IN] Instance ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetServiceRef(0, serviceId, instanceId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference to a server-service-instance on a dedicated network interface. The ifName must
 * be the device specified in one of the JSON files.
 *
 * @return
 *     - Reference to the service instance.
 *     - NULL if not allowed to represent the service.
 */
//--------------------------------------------------------------------------------------------------
taf_someipSvr_ServiceRef_t taf_someipSvr_GetServiceEx
(
    uint16_t serviceId,
        ///< [IN] Service ID.
    uint16_t instanceId,
        ///< [IN] Instance ID.
    const char* LE_NONNULL ifName
        ///< [IN] Network interface name.
)
{
    uint8_t routingId;

    if (GetRoutingIdByIntfName(ifName, &routingId) != LE_OK)
    {
        return NULL;
    }

    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetServiceRef(routingId, serviceId, instanceId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the major and minor version for the server service instance. The default major version = 0x00
 * minor version = 0x00000000.
 *
 * NOTE: This API must be called before OfferService() if needed.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_SetServiceVersion
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint8_t majVer,
        ///< [IN] Major Version
    uint32_t minVer
        ///< [IN] Minor Version
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.SetServiceVersion(serviceRef, majVer, minVer);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set up an UDP server and/or a TCP server with given port(s) for the server service instance.
 *
 * NOTE: This API must be called before OfferService() if needed.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_SetServicePort
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t udpPort,
        ///< [IN] Set up an UDP server with the given UDP port for the
    uint16_t tcpPort,
        ///< [IN] Set up an TCP server with the given TCP port for the
        ///< service. 0 means no TCP server is set up.
    bool enableMagicCookies
        ///< [IN] If TCP magic cookie is enabled when TCP server is set
        ///< up.
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.SetServicePort(serviceRef, udpPort, tcpPort, enableMagicCookies);
}

//--------------------------------------------------------------------------------------------------
/**
 * Offer a server service instance. The TCP port or/and UDP port must be set if the service need to
 * be offered remotely, otherwise it's offered locally.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_OfferService
(
    taf_someipSvr_ServiceRef_t serviceRef
        ///< [IN] Service Reference
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.OfferService(serviceRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop offering a server service instance. It also stops offering all events of the service.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_StopOfferService
(
    taf_someipSvr_ServiceRef_t serviceRef
        ///< [IN] Service Reference
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.StopOfferService(serviceRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the event type for an event of a specified service.
 *
 * NOTE: The default type is ET_EVENT and the API must be called before OfferEvent() if needed.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_SetEventType
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId,
        ///< [IN] Event ID
    taf_someipDef_EventType_t eventType
        ///< [IN] Event Type
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.SetEventType(serviceRef, eventId, eventType);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the cycle time for an periodic event notification(ET_EVENT) of a specified service. By
 * default the cycle time is 0 which means periodic event notification is disabled.
 *
 * NOTE: This API has no effect for field notification(ET_FIELD) and must be called before
 * OfferEvent() if needed.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_SetEventCycleTime
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId,
        ///< [IN] Event ID
    uint32_t cycleTime
        ///< [IN] Cycle Time in milliseconds
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.SetEventCycleTime(serviceRef, eventId, cycleTime);
}

//--------------------------------------------------------------------------------------------------
/**
 * Enable an event and add the event to an event group.
 *
 * NOTE: This API must be called before offerEvent() and can be called for multiple times if the
 * event need to be added into more than one event group.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_EnableEvent
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId,
        ///< [IN] Event ID
    uint16_t eventgroupId
        ///< [IN] Event Group ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.EnableEvent(serviceRef, eventId, eventgroupId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable an event and remove the event from all event groups.
 *
 * NOTE: This API must be called before OfferEvent() if needed.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_DisableEvent
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId
        ///< [IN] Event ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.DisableEvent(serviceRef, eventId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Offer an event which is already enabled for a server service instance.
 *
 * NOTE: This API must be called after OfferService() if needed.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_OfferEvent
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId
        ///< [IN] Event ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.OfferEvent(serviceRef, eventId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop offering an event for a server service instance.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_StopOfferEvent
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId
        ///< [IN] Event ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.StopOfferEvent(serviceRef, eventId);
}

//--------------------------------------------------------------------------------------------------
/**
 * Fire an event or field notification. The specified event is updated with the specified payload
 * Data. Dependent on the type of the event, the payload is distributed to all notified clients
 * (always for events, only if the payload has changed for fields).
 *
 * NOTE: This API is only available after offerService() and OfferEvent().
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_Notify
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    uint16_t eventId,
        ///< [IN] Event ID
    const uint8_t* dataPtr,
        ///< [IN] Payload Data
    size_t dataSize
        ///< [IN]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.Notify(serviceRef, eventId, dataPtr, dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_someipSvr_RxMsg'
 *
 * This event provides information on Rx message.
 */
//--------------------------------------------------------------------------------------------------
taf_someipSvr_RxMsgHandlerRef_t taf_someipSvr_AddRxMsgHandler
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference
    taf_someipSvr_RxMsgHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.AddRxMsgHandler(serviceRef, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_someipSvr_RxMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_someipSvr_RemoveRxMsgHandler
(
    taf_someipSvr_RxMsgHandlerRef_t handlerRef
        ///< [IN]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.RemoveRxMsgHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_someipSvr_Subscription'
 *
 * This event provides information on event group subscription.
 */
//--------------------------------------------------------------------------------------------------
taf_someipSvr_SubscriptionHandlerRef_t taf_someipSvr_AddSubscriptionHandler
(
    taf_someipSvr_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t eventGroupId,
        ///< [IN] Event Group ID.
    taf_someipSvr_SubscriptionHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.AddSubscriptionHandler(serviceRef, eventGroupId, handlerPtr, contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_someipSvr_Subscription'
 */
//--------------------------------------------------------------------------------------------------
void taf_someipSvr_RemoveSubscriptionHandler
(
    taf_someipSvr_SubscriptionHandlerRef_t handlerRef
        ///< [IN]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.RemoveSubscriptionHandler(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the service ID and Instance ID of the Rx Message.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_GetServiceId
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message Reference
    uint16_t* serviceIdPtr,
        ///< [OUT] Service ID
    uint16_t* instanceIdPtr
        ///< [OUT] Instance ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetServiceId(msgRef, serviceIdPtr, instanceIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the method ID of the Rx message.
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_GetMethodId
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message reference
    uint16_t* methodIdPtr
        ///< [OUT] Method ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetMethodId(msgRef, methodIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client ID of the Rx message.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_GetClientId
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message reference
    uint16_t* clientIdPtr
        ///< [OUT] Client ID
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetClientId(msgRef, clientIdPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the message type of the Rx message. The possible message type can be MT_REQUEST(0x00) or
 * MT_REQUEST_NO_RETURN(0x01).
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_GetMsgType
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message reference
    uint8_t* msgTypePtr
        ///< [OUT] Message Type
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetMsgType(msgRef, msgTypePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the payload size of the Rx message.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_GetPayloadSize
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message reference
    uint32_t* payloadSizePtr
        ///< [OUT] Payload Size in bytes
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetPayloadSize(msgRef, payloadSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the payload data of the Rx message.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_GetPayloadData
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message reference
    uint8_t* dataPtr,
        ///< [OUT] Payload Data
    size_t* dataSizePtr
        ///< [INOUT]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.GetPayloadData(msgRef, dataPtr, dataSizePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a response message with message type MT_RESPONSE(0x80) for a Rx message.
 *
 * NOTE: Only the Rx message with message type MT_REQUEST(0x00) is allowed to send a response.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_SendResponse
(
    taf_someipSvr_RxMsgRef_t msgRef,
        ///< [IN] Rx Message reference
    bool isErrRsp,
        ///< [IN] If true set message type to MT_ERROR(0x81)
    uint8_t returnCode,
        ///< [IN] Return Code
    const uint8_t* dataPtr,
        ///< [IN] Payload Data
    size_t dataSize
        ///< [IN]
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.SendResponse(msgRef, isErrRsp, returnCode, dataPtr, dataSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Release a Rx message.
 *
 * @return
 *     - LE_OK if successful.
 *     - LE_FAULT if any error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipSvr_ReleaseRxMsg
(
    taf_someipSvr_RxMsgRef_t msgRef
        ///< [IN] Rx Message reference
)
{
    taf_SomeipSvr& mySomeipSvr = taf_SomeipSvr::GetInstance();
    return mySomeipSvr.ReleaseRxMsg(msgRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the SOME/IP client ID of client-service-instance on default network interface.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_someipClnt_GetClientId
(
    void
)
{
    // Get the vsomeip client ID of default routing manager.
    return someip_GetClientId(0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the SOME/IP client ID of client-service-instance on a dedicated network interface.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameters.
 *     - LE_NOT_FOUND -- Client ID is not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_GetClientIdEx
(
    const char* LE_NONNULL ifName,
        ///< [IN] Network interface name.
    uint16_t* clientIdPtr
        ///< [OUT] SOME/IP Client ID.
)
{
    uint8_t routingId;

    if (clientIdPtr == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    if (GetRoutingIdByIntfName(ifName, &routingId) != LE_OK)
    {
        return LE_NOT_FOUND;
    }

    *clientIdPtr = someip_GetClientId(routingId);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Requests a client-service-instance on default network interface and returns the reference to the
 * client-service-instance.
 *
 * @return
 *     - Reference to the client-service-instance.
 *     - NULL if invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
taf_someipClnt_ServiceRef_t taf_someipClnt_RequestService
(
    uint16_t serviceId,
        ///< [IN] Service ID.
    uint16_t instanceId
        ///< [IN] Instance ID.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.RequestService(0, serviceId, instanceId,
                                         TAF_SOMEIPDEF_ANY_MAJOR, TAF_SOMEIPDEF_ANY_MINOR);
}

//--------------------------------------------------------------------------------------------------
/**
 * Requests a client-service-instance on a dedicated network interface and returns the reference to
 * the client-service-instance. The ifName must be the device specified in one of the JSON files.
 *
 * @return
 *     - Reference to the client-service-instance.
 *     - NULL if invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
taf_someipClnt_ServiceRef_t taf_someipClnt_RequestServiceEx
(
    uint16_t serviceId,
        ///< [IN] Service ID.
    uint16_t instanceId,
        ///< [IN] Instance ID.
    const char* LE_NONNULL ifName
        ///< [IN] Network interface name.
)
{
    uint8_t routingId;

    if (GetRoutingIdByIntfName(ifName, &routingId) != LE_OK)
    {
        return NULL;
    }

    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.RequestService(routingId, serviceId, instanceId,
                                         TAF_SOMEIPDEF_ANY_MAJOR, TAF_SOMEIPDEF_ANY_MINOR);
}

//--------------------------------------------------------------------------------------------------
/**
 * Requests a client-service-instance with specified major/minor version on a dedicated network
 * interface and returns the reference to the client-service-instance. The ifName shall match the
 * device name specified in one of the JSON files.
 *
 * NOTE: The possible majorVersion range is 0 - 0xFF, while minorVersion range is 0 - 0xFFFFFFFF.
 * TAF_SOMEIPDEF_ANY_MAJOR(0xFF) and TAF_SOMEIPDEF_ANY_MAJOR(0XFFFFFFFF) can be used to specify
 * any major/minor version if needed.
 *
 * @return
 *     - Reference to the client-service-instance.
 *     - NULL if invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
taf_someipClnt_ServiceRef_t taf_someipClnt_RequestServiceWithVersion
(
    uint16_t serviceId,
        ///< [IN] Service ID.
    uint16_t instanceId,
        ///< [IN] Instance ID.
    uint8_t  majVer,
        ///< [OUT] Major Version.
    uint32_t minVer,
        ///< [OUT] Minor Version.
    const char* LE_NONNULL ifName
        ///< [IN] Network interface name.
)
{
    uint8_t routingId;

    if (GetRoutingIdByIntfName(ifName, &routingId) != LE_OK)
    {
        return NULL;
    }

    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.RequestService(routingId, serviceId, instanceId, majVer, minVer);
}

//--------------------------------------------------------------------------------------------------
/**
 * Releases a client-service-instance to disconnect the service. This also clears all pending
 * messages, unsubscribe event groups and remove all registered handlers for the client application.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_ReleaseService
(
    taf_someipClnt_ServiceRef_t serviceRef
        ///< [IN] Service Reference.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.ReleaseService(serviceRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the service state.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_GetState
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    taf_someipClnt_State_t* statePtr
        ///< [OUT] Service State if return LE_OK.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.GetState(serviceRef, statePtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the service version.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid parameters.
 *     - LE_UNAVAILABLE -- Service is unavailable.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_GetVersion
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint8_t* majVerPtr,
        ///< [OUT] Major Version of the service if return LE_OK.
    uint32_t* minVerPtr
        ///< [OUT] Minor Version of the service if return LE_OK.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.GetVersion(serviceRef, majVerPtr, minVerPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_someipClnt_StateChange'
 *
 * This event provides information on service state change.
 */
//--------------------------------------------------------------------------------------------------
taf_someipClnt_StateChangeHandlerRef_t taf_someipClnt_AddStateChangeHandler
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    taf_someipClnt_StateChangeHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.AddStateChangeHandler(serviceRef, handlerPtr, contextPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_someipClnt_StateChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_someipClnt_RemoveStateChangeHandler
(
    taf_someipClnt_StateChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.RemoveStateChangeHandler(handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Creates a request message and set the destination.
 *
 * @return
 *     - Reference to the request message.
 *     - NULL if invalid parameters.
 */
//--------------------------------------------------------------------------------------------------
taf_someipClnt_TxMsgRef_t taf_someipClnt_CreateMsg
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t methodId
        ///< [IN] Method ID.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.CreateMsg(serviceRef, methodId);
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the request to a non-return-request(MT_REQUEST_NO_RETURN). By default it's MT_REQUEST.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_SetNonRet
(
    taf_someipClnt_TxMsgRef_t msgRef
        ///< [IN] Tx message reference.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.SetNonRet(msgRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Uses TCP to send the request. By default is using UDP.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_SetReliable
(
    taf_someipClnt_TxMsgRef_t msgRef
        ///< [IN] Tx message reference.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.SetReliable(msgRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets timeout milliseconds waiting for the response. By default the timeout is 0(forever).
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_SetTimeout
(
    taf_someipClnt_TxMsgRef_t msgRef,
        ///< [IN] Tx message reference.
    uint32_t timeOut
        ///< [IN] Timeout in milliseconds.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.SetTimeout(msgRef, timeOut);
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets payload data of the request. By default the payload is empty.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_SetPayload
(
    taf_someipClnt_TxMsgRef_t msgRef,
        ///< [IN] Tx message reference.
    const uint8_t* dataPtr,
        ///< [IN] Payload Data.
    size_t dataSize
        ///< [IN]
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.SetPayload(msgRef, dataPtr, dataSize);
}
//--------------------------------------------------------------------------------------------------
/**
 * Delete a request message.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_DeleteMsg
(
    taf_someipClnt_TxMsgRef_t msgRef
        ///< [IN] Tx message reference.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.DeleteMsg(msgRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Send an asynchronous request message. The response handler will be called once the response is
 * received or any errors occur.
 *
 * NOTE: The request message will be automatically deleted after calling this API.
 */
//--------------------------------------------------------------------------------------------------
void taf_someipClnt_RequestResponse
(
    taf_someipClnt_TxMsgRef_t msgRef,
        ///< [IN] Tx message reference.
    taf_someipClnt_RespMsgHandlerFunc_t handlerPtr,
        ///< [IN] Response message handler.
    void* contextPtr
        ///< [IN]
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.RequestResponse(msgRef, handlerPtr, contextPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Enable an event group by adding an event into the group.
 *
 * NOTE: This API can be called for multiple times if there are more than one events adding into the
 * group. Currently one event can be only added into one group, and one event group can be enabled
 * only by one client.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 *     - LE_NOT_PERMITTED -- The event group is subscribed or already enabled by another client,
 *       or the event is already added into another group.
 *     - LE_DUPLICATE -- The event is already added into this group.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_EnableEventGroup
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t eventGroupId,
        ///< [IN] Event Group ID.
    uint16_t eventId,
        ///< [IN] Event ID.
    taf_someipDef_EventType_t eventType
        ///< [IN] Event Type.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.EnableEventGroup(serviceRef, eventGroupId, eventId, eventType);
}
//--------------------------------------------------------------------------------------------------
/**
 * Disable an event group by removing all of the events from the group.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 *     - LE_NOT_PERMITTED -- The event group is subscribed or already enabled by another client.
 *     - LE_DUPLICATE -- The event group is not enabled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_DisableEventGroup
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t eventGroupId
        ///< [IN] Event Group ID.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.DisableEventGroup(serviceRef, eventGroupId);
}
//--------------------------------------------------------------------------------------------------
/**
 * Subscribe an event group of a service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 *     - LE_NOT_PERMITTED -- The event group is not enabled or already enabled by another client.
 *     - LE_DUPLICATE -- The event group is already subscribed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_SubscribeEventGroup
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t eventGroupId
        ///< [IN] Event Group ID.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.SubscribeEventGroup(serviceRef, eventGroupId);
}
//--------------------------------------------------------------------------------------------------
/**
 * Unsubscribe an event group of a service.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Invalid input parameters.
 *     - LE_NOT_PERMITTED -- The event group is not enabled or already enabled by another client.
 *     - LE_DUPLICATE -- the event group is not subscribed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_someipClnt_UnsubscribeEventGroup
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t eventGroupId
        ///< [IN] Event Group ID.
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.UnsubscribeEventGroup(serviceRef, eventGroupId);
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_someipClnt_EventMsg'
 *
 * This event provides information on event message.
 */
//--------------------------------------------------------------------------------------------------
taf_someipClnt_EventMsgHandlerRef_t taf_someipClnt_AddEventMsgHandler
(
    taf_someipClnt_ServiceRef_t serviceRef,
        ///< [IN] Service Reference.
    uint16_t eventGroupId,
        ///< [IN] Event group ID.
    taf_someipClnt_EventMsgHandlerFunc_t handlerPtr,
        ///< [IN] Event Handler.
    void* contextPtr
        ///< [IN]
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.AddEventMsgHandler(serviceRef, eventGroupId, handlerPtr, contextPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_someipClnt_EventMsg'
 */
//--------------------------------------------------------------------------------------------------
void taf_someipClnt_RemoveEventMsgHandler
(
    taf_someipClnt_EventMsgHandlerRef_t handlerRef
        ///< [IN]
)
{
    taf_SomeipClient& mySomeipClient = taf_SomeipClient::GetInstance();
    return mySomeipClient.RemoveEventMsgHandler(handlerRef);
}
