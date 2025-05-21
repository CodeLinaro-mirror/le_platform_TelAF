/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

#include <iostream>

#include "taf_gptpTime.h"

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <errno.h>

#define TAF_TIME_CLOCKFD                   3
#define TAF_TIME_FD_TO_CLOCKID(fd)        ((~(clockid_t) (fd) << 3) | TAF_TIME_CLOCKFD)
#define TAF_TIME_LOCAL_PTP_NODE           "/dev/ptp0"
#define TAF_TIME_ETH0_NODE                "eth0"

#define GPTP_DEVICE_STR_BUF_MAX    60
#define GPTP_DEVICE_NUM_MAX        2

#define BUFSIZE 8192

le_mem_PoolRef_t GptpTimeMemPoolRef;
le_ref_MapRef_t GptpTimeRefMap;

int ifindex = 0;
bool isPTPDevDown = false;
int sockfd = -1;
le_fdMonitor_Ref_t monitorRef = NULL;
int clientRefCount = 0;

le_result_t SetupNetlinkSocket(void);
void TeardownNetlinkSocket(void);

typedef struct
{
    clockid_t clkid;
    int fd;
    char deviceName[GPTP_DEVICE_STR_BUF_MAX];
    taf_gptpTime_Ref_t safeRef;
    le_timer_Ref_t deviceOpenTimer = NULL;
} taf_GptpTime_t;

taf_GptpTime_t* isGptpRefExist(const char* deviceName)
{
    if (!deviceName) return NULL;
    le_ref_IterRef_t iterRef = le_ref_GetIterator(GptpTimeRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_GptpTime_t* gptpTime  = (taf_GptpTime_t*)le_ref_GetValue(iterRef);
        if (gptpTime &&
            strncmp(gptpTime->deviceName, deviceName, GPTP_DEVICE_STR_BUF_MAX) == 0
            )
        {
            return gptpTime;
        }
    }

    return NULL;
}

LE_SHARED taf_gptpTime_Ref_t taf_gptpTime_CreateRef
(
    const char* deviceName
)
{
    if (!deviceName || strlen(deviceName) >= GPTP_DEVICE_STR_BUF_MAX)
    {
        LE_ERROR("Device name is not correct");
        return NULL;
    }

    if (monitorRef == NULL)
    {
        if (LE_OK != SetupNetlinkSocket())
        {
            LE_ERROR("Create netlink socket failed");
            return NULL;
        }
    }

    taf_GptpTime_t* gptpTime = isGptpRefExist(deviceName);
    if(gptpTime != NULL)
    {
        return gptpTime->safeRef;
    }
    gptpTime = (taf_GptpTime_t*)le_mem_ForceAlloc(GptpTimeMemPoolRef);
    int fd = open(deviceName, O_RDONLY);
    if (-1 == fd)
    {
        switch (errno)
        {
            case EPERM:
                LE_ERROR("No permission to open the device\n");
                break;

            case ENOENT:
                LE_ERROR("Not found the device\n");
                break;

            case ETXTBSY:
                LE_ERROR("The device is busy\n");
                break;

            default:
                LE_ERROR("Open %s failed: %d, %s", deviceName,
                                                    errno, strerror(errno));
                break;
        }
        le_mem_Release(gptpTime);
        return NULL;
    }
    gptpTime->clkid = TAF_TIME_FD_TO_CLOCKID(fd);
    le_utf8_Copy(gptpTime->deviceName, deviceName, sizeof(gptpTime->deviceName), NULL);
    gptpTime->fd = fd;
    gptpTime->safeRef = (taf_gptpTime_Ref_t)le_ref_CreateRef(GptpTimeRefMap, gptpTime);
    clientRefCount++;
    return gptpTime->safeRef;
}

LE_SHARED le_result_t taf_gptpTime_GetTimeValue
(
    taf_gptpTime_Ref_t gptpTimeRef,
    struct timespec* gptpTimeValuePtr
)
{
    taf_GptpTime_t* gptpPtr = (taf_GptpTime_t*)le_ref_Lookup(GptpTimeRefMap, gptpTimeRef);
    if(gptpPtr == NULL)
    {
        LE_ERROR("GPTP Reference is not found!");
        return LE_BAD_PARAMETER;
    }

    if(gptpPtr->fd == -1)
    {
        LE_ERROR("PTP Device is down, please try after some time");
        return LE_UNAVAILABLE;
    }

    struct timespec ts;
    if (-1 == clock_gettime(gptpPtr->clkid, &ts))
    {
        switch (errno)
        {
            case EPERM:
                LE_ERROR("No permission to read time\n");
                return LE_NOT_PERMITTED;

            case EINVAL:
                LE_ERROR("Invalid parameter\n");
                return LE_BAD_PARAMETER;

            default:
                LE_ERROR("Get time failed (errno = %d)\n", errno);
                return LE_FAULT;
        }
    }

    gptpTimeValuePtr->tv_sec = ts.tv_sec;
    gptpTimeValuePtr->tv_nsec = ts.tv_nsec;
    return LE_OK;
}
 
LE_SHARED le_result_t taf_gptpTime_DeleteRef
(
    taf_gptpTime_Ref_t gptpTimeRef
)
{
    int result;
    taf_GptpTime_t* gptpPtr = (taf_GptpTime_t*)le_ref_Lookup(GptpTimeRefMap, gptpTimeRef);
    if(gptpPtr == NULL)
    {
        LE_ERROR("GPTP Reference is not found!");
        return LE_BAD_PARAMETER;
    }
    if (gptpPtr->fd != -1)
    {
        result = le_fd_Close(gptpPtr->fd);
        if (result != 0)
        {
            LE_ERROR("Failed to close file descriptor %d. Errno = %d.", gptpPtr->fd, errno);
            return LE_FAULT;
        }
        gptpPtr->fd = -1;
    }
    le_ref_DeleteRef(GptpTimeRefMap, gptpTimeRef);
    le_mem_Release(gptpPtr);
    clientRefCount--;
    if(clientRefCount == 0)
    {
        TeardownNetlinkSocket();
    }
    return LE_OK;
}

// Timer handler function to try opening the device
void DeviceOpenTimerHandler(le_timer_Ref_t timerRef)
{
    taf_GptpTime_t* gptpTime = (taf_GptpTime_t*)le_timer_GetContextPtr(timerRef);
    int fd = open(gptpTime->deviceName, O_RDONLY);
    if (fd != -1)
    {
        // Successfully opened the device
        gptpTime->clkid = TAF_TIME_FD_TO_CLOCKID(fd);
        gptpTime->fd = fd;
        LE_INFO("Successfully opened device %s\n", gptpTime->deviceName);

        // Stop the timer
        le_result_t res = le_timer_Stop(timerRef);
        if(res == LE_OK)
        {
            LE_INFO("Timer stopped for device: %s", gptpTime->deviceName);
        }
        else
        {
            LE_ERROR("Not able to stop timer for device: %s", gptpTime->deviceName);
        }
    }
    else
    {
        switch (errno)
        {
            case ETXTBSY:
                LE_ERROR("The device is busy\n");
                return;

            case ENOENT:
                LE_ERROR("No such file or directory\n");
                return;

            case ENODEV:
                LE_ERROR("No such device exist\n");
                return;

            case EBUSY:
                LE_ERROR("Device or resource busy\n");
                return;

            case ENXIO:
                LE_ERROR("No such device or address\n");
                return;

            case EACCES:
                LE_ERROR("Access to the device denied\n");
                break;

            case EIO:
                LE_ERROR("I/O error\n");
                break;

            case EPERM:
                LE_ERROR("No permission to open the device\n");
                break;

            default:
            LE_ERROR("Open %s failed: %d, %s",
                gptpTime->deviceName, errno, LE_ERRNO_TXT(errno));
                break;
        }
        // Stop the timer
        le_result_t res = le_timer_Stop(timerRef);
        if(res == LE_OK)
        {
            LE_INFO("Timer stopped for device: %s", gptpTime->deviceName);
        }
        else
        {
            LE_ERROR("Not able to stop timer for device: %s", gptpTime->deviceName);
        }
    }
}

void handleGptpReference(struct ifinfomsg *ifi, bool isPtpDown)
{
    char ifname[IFNAMSIZ];
    if_indextoname(ifi->ifi_index, ifname);

    le_ref_IterRef_t iterRef = le_ref_GetIterator(GptpTimeRefMap);
    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        taf_GptpTime_t* gptpTime = (taf_GptpTime_t*)le_ref_GetValue(iterRef);
        if (!gptpTime) continue;

        if (isPtpDown)
        {
            LE_INFO("Interface %s (index %d) is going down\n", ifname, ifi->ifi_index);

            if (gptpTime->fd != -1)
            {
                int result = le_fd_Close(gptpTime->fd);
                if (result != 0)
                {
                    LE_ERROR("Failed to close file descriptor %d. Errno = %d.",
                        gptpTime->fd, errno);
                }
                gptpTime->fd = -1;
            }

            if(gptpTime->deviceOpenTimer)
            {
                le_result_t res = le_timer_Stop(gptpTime->deviceOpenTimer);
                if(res == LE_OK)
                {
                    LE_INFO("Timer stopped for device: %s", gptpTime->deviceName);
                }
                else
                {
                    LE_ERROR("Not able to stop timer for device: %s", gptpTime->deviceName);
                }
            }

            LE_INFO("Successfully closed device %s\n", gptpTime->deviceName);
        }
        else
        {
            LE_INFO("Interface %s (index %d) is going up\n", ifname, ifi->ifi_index);
            // Create and start the timer to try opening the device every 1 second
            gptpTime->deviceOpenTimer = le_timer_Create("DeviceOpenTimer");
            le_timer_SetMsInterval(gptpTime->deviceOpenTimer, 1000); // 1 second interval
            le_timer_SetRepeat(gptpTime->deviceOpenTimer, 0); // Repeat indefinitely
            le_timer_SetHandler(gptpTime->deviceOpenTimer, DeviceOpenTimerHandler);
            le_timer_SetWakeup(gptpTime->deviceOpenTimer, false);
            le_timer_SetContextPtr(gptpTime->deviceOpenTimer, gptpTime);
            le_timer_Start(gptpTime->deviceOpenTimer);
        }
    }
}

void handleNetlinkMessage(struct nlmsghdr *nlh)
{
    struct ifinfomsg *ifi = (struct ifinfomsg *)NLMSG_DATA(nlh);

    if (ifi->ifi_index != ifindex) {
        LE_DEBUG("Ignoring message for interface index %d\n", ifi->ifi_index);
        return; // Ignore messages for other interfaces
    }

    if (!(ifi->ifi_flags & IFF_UP) && !isPTPDevDown)
    {
        isPTPDevDown = true;
        handleGptpReference(ifi, isPTPDevDown);
    }

    else if((ifi->ifi_flags & IFF_UP) && isPTPDevDown)
    {
        isPTPDevDown = false;
        handleGptpReference(ifi, isPTPDevDown);
    }
}

static void NetlinkEventHandler(int fd, short events)
{
    struct sockaddr_nl sa;
    char buf[BUFSIZE];
    struct iovec iov = { buf, sizeof(buf) };
    struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };

    ssize_t len = recvmsg(fd, &msg, MSG_DONTWAIT);
    if (len < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LE_ERROR("recvmsg failed\n");
            return;
        }
    } else {
        struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
        for (; NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len)) {
            if (nlh->nlmsg_type == RTM_NEWLINK) {
                handleNetlinkMessage(nlh);
            }
        }
    }
}
le_result_t SetupNetlinkSocket(void)
{
    struct sockaddr_nl sa;

    ifindex = if_nametoindex(TAF_TIME_ETH0_NODE);
    if (ifindex == 0) {
        LE_ERROR("Failed to get index for %s.  %m.", TAF_TIME_ETH0_NODE);
        return LE_FAULT;
    }

    sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sockfd < 0) {
        LE_ERROR("Failed to create netlink socket.  %m.");
        return LE_FAULT;
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1 || fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
         LE_ERROR("Failed to set socket to non-blocking mode.  %m.");
        goto NetlinkErr;
    }

    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTMGRP_LINK;

    if (bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        LE_ERROR("Failed to bind netlink socket.  %m.");
        goto NetlinkErr;
    }

    monitorRef = le_fdMonitor_Create("NetlinkMonitorForEth0",
                                        sockfd, NetlinkEventHandler, POLLIN);
    if (monitorRef == NULL) {
        LE_ERROR("Failed to create fd monitor\n");
        goto NetlinkErr;
    }

    LE_INFO("Netlink socket setup complete");
    return LE_OK;

NetlinkErr:
    close(sockfd);
    sockfd = -1;
    return LE_FAULT;

}

void TeardownNetlinkSocket(void)
{
    if (monitorRef) {
        le_fdMonitor_Delete(monitorRef);
        monitorRef = NULL;
    }

    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }

    LE_INFO("Netlink socket torn down");
}

COMPONENT_INIT {
    LE_INFO("Initializing GPTP component.");

    if(GptpTimeMemPoolRef == NULL)
    {
        GptpTimeMemPoolRef = le_mem_CreatePool("GptpTimePool", sizeof(taf_GptpTime_t));
    }
    if(GptpTimeRefMap == NULL)
    {
        GptpTimeRefMap = le_ref_CreateMap("GptpTime", GPTP_DEVICE_NUM_MAX);
    }
    if(GptpTimeMemPoolRef == NULL || GptpTimeRefMap == NULL)
    {
        LE_ERROR("Unable to allocate memory for gptp time. Please try again.");
        return;
    }

    if (LE_OK != SetupNetlinkSocket())
    {
        LE_WARN("Create netlink socket failed, needs to try later.");
    }
    LE_INFO("GPTP component initialization done.");
}

