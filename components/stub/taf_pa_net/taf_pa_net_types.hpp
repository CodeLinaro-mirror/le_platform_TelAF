/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 /*
 * @file  taf_pa_net_types.hpp
 * @brief TAF network types used in the net prop stub alone.
 */

#ifndef TAF_PA_NET_TYPES_HPP
#define TAF_PA_NET_TYPES_HPP

//--------------------------------------------------------------------------------------------------
/**
 * The device mode.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_NET_DEVICE_UNKNOWN = -1,
    ///< Unknown.
    TAF_NET_DEVICE_NONE = 0,
    ///< None.
    TAF_NET_DEVICE_L2L = 1,
    ///< Device LAN-to-LAN mode.
    TAF_NET_DEVICE_E2E = 2
    ///< Device end-to-end mode.
}
taf_net_DeviceMode_t;

//--------------------------------------------------------------------------------------------------
/**
 * The SOCKS authentication type.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_NET_SOCKS_UNKNOWN = -1,
        ///< Unknown.
    TAF_NET_SOCKS_NONE = 0,
        ///< No authentication.
    TAF_NET_SOCKS_USER_PASSWD = 1
        ///< Username and password.
}
taf_net_AuthMethod_t;

#endif /* TAF_PA_NET_TYPES_HPP */