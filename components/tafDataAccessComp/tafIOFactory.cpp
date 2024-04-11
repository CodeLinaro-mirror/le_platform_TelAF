/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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