/*
* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef TAFETSCOMMON_HPP_
#define TAFETSCOMMON_HPP_

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

#endif // TAFETSCOMMON_HPP_
