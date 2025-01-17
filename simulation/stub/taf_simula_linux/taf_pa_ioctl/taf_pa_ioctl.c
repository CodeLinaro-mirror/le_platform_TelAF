/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <unistd.h>
#include <sys/ioctl.h>

typedef int (*orig_ioctl_t)(int, unsigned long, ...);

int ioctl(int fd, unsigned long request, ...) {

    orig_ioctl_t orig_ioctl = (orig_ioctl_t)dlsym(RTLD_NEXT, "ioctl");

    if (request == FS_IOC_ADD_ENCRYPTION_KEY)
    {
        LE_INFO("[simulation] ioctl request -> FS_IOC_ADD_ENCRYPTION_KEY\n");
    }

    return 0;
}
