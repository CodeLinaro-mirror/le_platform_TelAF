/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAFHALIF_HPP
#define TAFHALIF_HPP

#include <stdint.h>
#include <unistd.h>

//--------------------------------------------------------------------------------------------------
/**
 * VHAL macros used for information management
 */
//--------------------------------------------------------------------------------------------------
#define TAF_HAL_NAME_MAX_LEN        50  // 50 bytes including null terminated
#define TAF_HAL_VERSION_MAX_LEN     10  // Format [xx].[yy]
#define TAF_HAL_VENDOR_NAME_MAX_LEN 50  // 50 bytes including null terminated_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------------------------
/**
 * VHAL module type definition
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TAF_MODULETYPE_HAL,
    TAF_MODULETYPE_PLUG_IN
} taf_hal_ModuleType;

//--------------------------------------------------------------------------------------------------
/**
 * Type definition for management interfaces
 */
//--------------------------------------------------------------------------------------------------
typedef int (*TAF_HAL_HWINIT)(void);
typedef void (*TAF_HAL_POWERON)(void);
typedef void (*TAF_HAL_POWEROFF)(void);

typedef void (*TAF_HAL_SLEEP)(void);
typedef void (*TAF_HAL_WAKEUP)(void);
typedef int32_t (*TAF_HAL_SELFTEST)(void);

typedef void* (*TAF_HAL_GETMODINF)(void);

//--------------------------------------------------------------------------------------------------
/**
 * VHAL tab for services/applications to get interfaces
 */
//--------------------------------------------------------------------------------------------------
#define TAF_HAL_INFO_TAB       tafHalInfoTab
#define TAF_HAL_INFO_TAB_STR   "tafHalInfoTab"

//--------------------------------------------------------------------------------------------------
/**
 * VHAL module interface
 *
 * The module handle will be closed by device manager.
 * Make sure after return from those API, it is clean.
 * No memory objects left in the system.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {

    // module info
    const char*         name;
    uint16_t            majorVer;
    uint16_t            minorVer;
    taf_hal_ModuleType  moduleType;
    const char*         vendor;

    uint8_t             serviceMax;

    // generic hal module interface
    TAF_HAL_HWINIT   hwInitInf;
    TAF_HAL_POWEROFF powerOffInf;
    TAF_HAL_POWERON  powerOnInf;

    // Self Test
    TAF_HAL_SELFTEST selfTest;

    //Get the specific interface
    TAF_HAL_GETMODINF getModInf;

    // Reserve
    uint8_t res[32];
}TAF_HAL_MGR_INF_t;

//--------------------------------------------------------------------------------------------------
/**
 * Definitions for safe call to handle singal error from calling VHAL functions
 */
//--------------------------------------------------------------------------------------------------
#define	DECLARE_SAFE_CALL()	\
        static jmp_buf env;	\
        static struct sigaction act;	\
        static struct sigaction oldAct1; \
        static struct sigaction oldAct2; \
        static int safeRun = 0;	\
        static void handler(int signum){ \
            if(safeRun == 1) \
            { \
                siglongjmp(env , 1); \
            } \
        } \

#define ENTER_SAFE_CALL(sec,ret,func,...)	\
    do {	\
        sigemptyset(&act.sa_mask); \
        act.sa_flags = SA_RESTART; \
        act.sa_handler = handler; \
        signal(SIGALRM, handler); \
        \
        if(sigaction(SIGSEGV,&act,&oldAct1)<0) \
        { \
             LE_ERROR("install signal error"); \
            exit(-1); \
        } \
           \
        if(sigaction(SIGALRM,&act,&oldAct2)<0) \
        { \
            LE_ERROR("install signal error"); \
            exit(-1); \
        } \
        \
        alarm(sec); \
           \
        if(sigsetjmp(env, 1)==0)	\
        {	\
            safeRun = 1; \
            func(__VA_ARGS__); \
            alarm(0); \
        } \
        else \
        { \
            alarm(0); \
            LE_INFO("now recovery\n"); \
            ret = -1; \
        \
        } \
    }while(0)

#define ENTER_SAFE_CALL_EX(sec,ret,funcRet,func,...) \
        do { \
            sigemptyset(&act.sa_mask); \
            act.sa_flags = SA_RESTART; \
            act.sa_handler = handler; \
            signal(SIGALRM, handler); \
            \
            if(sigaction(SIGSEGV,&act,&oldAct1)<0) \
            { \
                 LE_ERROR("install signal error"); \
                exit(-1); \
            } \
               \
            if(sigaction(SIGALRM,&act,&oldAct2)<0) \
            { \
                LE_ERROR("install signal error"); \
                exit(-1); \
            } \
            \
            alarm(sec); \
               \
            if(sigsetjmp(env, 1)==0)    \
            {   \
                safeRun = 1; \
                funcRet = func; \
                alarm(0); \
            } \
            else \
            { \
                alarm(0); \
                LE_INFO("now recovery\n"); \
                ret = -1; \
            \
            } \
        }while(0)

#define EXIT_SAFE_CALL()	\
    do {	\
        if(sigaction(SIGSEGV,&oldAct1,NULL)<0) \
        { \
             LE_ERROR("recover signal segment handler error"); \
            exit(-1); \
        } \
        if(sigaction(SIGALRM,&oldAct2,NULL)<0) \
        { \
            LE_ERROR("recover signal alarm handler error"); \
            exit(-1); \
        } \
        safeRun = 0; 	\
    }while(0)


#ifdef __cplusplus
}
#endif

#endif
