/*
**  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef LE_CONFIG_ENABLE_SELINUX
#include <selinux/selinux.h>
#endif

#include "tafBaseDAO.hpp"
#include "tafIOFactory.hpp"

using namespace taf::dataAccess;

IOFactory &IOFactory::GetInstance()
{
    static IOFactory instance;

    return instance;
}

void IOFactory::Init
(
)
{

}