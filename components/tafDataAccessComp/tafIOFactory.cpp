/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include "tafDataAccessCompImpl.hpp"

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
    //Create the directory for DEM database
    struct stat sb;

    if(stat(DEM_DATABASE_DIR, &sb) == -1)
    {
        LE_INFO("The DEM database dir(%s) does not exist, create it.", DEM_DATABASE_DIR);

        if(mkdir(DEM_DATABASE_DIR, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", DEM_DATABASE_DIR);
            LE_ERROR("Reason: %s", strerror(errno));
            return;
        }

#ifdef LE_CONFIG_ENABLE_SELINUX
        // Set selinux context to created folder
        if(setfilecon(DEM_DATABASE_DIR, DEM_DATABASE_CONTEXT) != 0)
        {
            LE_ERROR("Failed to change SELinux context");
        }
#endif
    }
    else if((sb.st_mode & S_IFMT) == S_IFDIR)
    {
        // assume the sub dir was created as well. do nothing
        LE_INFO("The DEM database dir was found!");
    }
    else
    {
        // some other file objects. delete first
        LE_ERROR("Delete the file object, then create the dir");

        // try to delete it
        unlink(DEM_DATABASE_DIR);

        // Create the directory
        if(mkdir(DEM_DATABASE_DIR, 0755) != 0)
        {
            LE_ERROR("Failed to create the dir %s", DEM_DATABASE_DIR);
            return;
        }
    }
}