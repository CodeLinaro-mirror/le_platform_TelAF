/*
* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef TAFIVSSSVCCOMMON_HPP_
#define TAFIVSSSVCCOMMON_HPP_

#include "legato.h"
#include "interfaces.h"
#include <CommonAPI/CommonAPI.hpp>

#define TAF_ERROR_IF_COND_POST_SEM(condition, semRef, formatString, ...) \
    do { \
        if (condition) { \
            le_sem_Post(semRef); \
            LE_ERROR(formatString, ##__VA_ARGS__); \
            return; \
        } \
    } while(0);

#endif // TAFIVSSSVCCOMMON_HPP_
