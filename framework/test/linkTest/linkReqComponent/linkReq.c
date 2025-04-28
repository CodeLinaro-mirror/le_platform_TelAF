/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include "interfaces.h"

int file_exists(const char *filename)
{
    return access(filename, F_OK) == 0;
}

int dir_exists(const char *directory)
{
    struct stat st;
    if (stat(directory, &st) != 0)
    {
        LE_ERROR("Failed to stat %s, %m", directory);
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

//--------------------------------------------------------------------------------------------------
/**
 * Before starting the test, please create below directories and files
 *
 * Directoires:
 *  - /data/TestDir
 *  - /data/TestSubDir
 *
 * Files:
 *  - /data/file1
 *  - /data/file2
 *  - /data/file3
 */
//--------------------------------------------------------------------------------------------------
void testLinks(void)
{
    const char *dir1 = "TestDir";
    const char *dir2 = "data/SubDir";
    const char *file1 = "file1";
    const char *file2 = "data/file2";
    const char *file3 = "data/file3";

    LE_TEST_ASSERT((dir_exists(dir1)),"Test dir: %s", dir1);
    LE_TEST_ASSERT((dir_exists(dir2)),"Test dir: %s", dir2);

    LE_TEST_ASSERT((file_exists(file1)),"Test file: %s", file1);
    LE_TEST_ASSERT((file_exists(file2)),"Test file: %s", file2);
    LE_TEST_ASSERT((file_exists(file3)),"Test file: %s", file3);
}

COMPONENT_INIT
{
    LE_INFO("---------- Started testing -------------------");

    testLinks();

    LE_INFO("---------- All Tests Completed ----------------");

    LE_TEST_EXIT;
}
