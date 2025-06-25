/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"

LE_SHARED void funC
(
    void* myPtr
)
{
    if (myPtr != NULL)
    {
        LE_INFO("Generate a segmentation fault crash.");
        *((int*)myPtr) = 1;
    }
}


LE_SHARED void funB
(
    void
)
{
    void* testPtr = (void*)1;

    funC(testPtr);
}

LE_SHARED void funA
(
   void
)
{
    funB();
}

COMPONENT_INIT
{
    funA();
}
