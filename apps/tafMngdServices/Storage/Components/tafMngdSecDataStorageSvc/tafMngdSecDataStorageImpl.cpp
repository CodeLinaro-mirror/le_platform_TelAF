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

#include "tafMngdStorageSvc.hpp"

using namespace telux::tafsvc;


/**
 * Get MSS instance
 */
tafMngdStorageSvc &tafMngdStorageSvc::GetInstance()
{
   static tafMngdStorageSvc instance;
   return instance;
}

/**
 * tafMngdStorageSvc initialization
 */
void tafMngdStorageSvc::Init(void)
{
    InitStorage();
    taf_rfs_Init(true, nullptr);
    taf_rfs_SetBackupStorage(secDataRfsStorage);
}

size_t tafMngdStorageSvc::GetFileSize
(
    const char *filePath
)
{
    struct stat fileStat;

    // Get file statistics
    if (stat(filePath, &fileStat) == -1)
    {
        LE_ERROR("Failed to get file status for %s", filePath);
        return 0;
    }

    // Check if the entry is a regular file
    if (S_ISREG(fileStat.st_mode))
    {
        return (size_t)fileStat.st_size;
    }
    else
    {
        LE_ERROR("%s is not a regular file", filePath);
        return 0;
    }
}

le_result_t tafMngdStorageSvc::CreateDirectory(const char *path)
{
    return le_dir_MakePath(path, 0644);
}

size_t tafMngdStorageSvc::GetFilesSizeInDirectory
(
    const char *dirPath
)
{
    struct dirent *entry;
    struct stat fileStat;
    char filePath[LIMIT_MAX_PATH_BYTES];
    size_t totalSize = 0;

    DIR *dp = opendir(dirPath);
    if (dp == nullptr) {
        LE_ERROR("Failed to open directory");
        return 0;
    }

    while ((entry = readdir(dp)) != nullptr) {
        // Ignore "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);

        // Get file statistics
        if (stat(filePath, &fileStat) == -1) {
            LE_ERROR("Failed to get file status");
            continue;
        }

        // Check if the entry is a regular file
        if (S_ISREG(fileStat.st_mode)) {
            totalSize += (size_t)fileStat.st_size;
        }
    }

    closedir(dp);

    LE_INFO("File size in %s is %" PRIuS, dirPath, totalSize);

    return totalSize;
}


bool tafMngdStorageSvc::IsDirectoryEmpty(const char *path)
{
    int n = 0;
    struct dirent *d;
    DIR *dir = opendir(path);

    if (dir == nullptr)
    {
        return false; // Return false if the directory cannot be opened
    }

    while ((d = readdir(dir)) != nullptr) {
        if (++n > 2) {
            break;
        }
    }
    closedir(dir);
    return (n <= 2); // Return true if the directory has only '.' and '..'
}

bool tafMngdStorageSvc::IsDirectoryExisting(const char *path)
{
    struct stat info;

    // Get file status
    if (stat(path, &info) != 0)
    {
        // Path does not exist or error accessing path
        return false;
    }
    else if (info.st_mode & S_IFDIR)
    {
        // Path exists and is a directory
        return true;
    }
    else
    {
        // Path exists but is not a directory
        return false;
    }
}

bool tafMngdStorageSvc::IsFileExisting(const char *path)
{
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

le_result_t tafMngdStorageSvc::CheckValidPosixFileName(const char *fileName)
{
    // POSIX file name must not be empty
    if (fileName == nullptr || strlen(fileName) == 0)
    {
        return LE_BAD_PARAMETER;
    }

    // Iterate through each character in the fileName
    for (size_t i = 0; i < strlen(fileName); i++)
    {
        char ch = fileName[i];
        // Check if the character is alphanumeric or one of the allowed special characters
        if (!(isalnum((unsigned char)ch) || ch == '-' || ch == '_' || ch == '.'))
        {
            return LE_OUT_OF_RANGE;
        }
    }

    return LE_OK;
}

le_result_t tafMngdStorageSvc::GetAppNameBySessionRef
(
    le_msg_SessionRef_t clientSessionRef,           ///< [IN]  client session reference.
    char    *appNameStr,                            ///< [OUT] Application name buffer.
    size_t   appNameSize                            ///< [IN]  Buffer size.
)
{
    pid_t pid;
    const char* namePtr = nullptr;
    char procPath[LIMIT_MAX_PATH_BYTES] = {0};
    char appPath[LIMIT_MAX_PATH_BYTES] = {0};

    // Parameter check.
    if ((clientSessionRef == nullptr) || (appNameStr == nullptr) || (appNameSize == 0))
    {
        LE_ERROR("Bad parameters.");
        return LE_BAD_PARAMETER;
    }

    // Get pid from the sessionRef.
    if (le_msg_GetClientProcessId(clientSessionRef, &pid) != LE_OK)
    {
        LE_ERROR("Failed to get the pid from client session reference.");
        return LE_FAULT;
    }

    // Get the app name from the pid.
    if (le_appInfo_GetName(pid, appPath, sizeof(appPath)) != LE_OK)
    {
        // It's not a telaf app but should a legacy app.
        // Read the program name from the softlink of /proc/<pid>/exe .
        LE_ASSERT(snprintf(procPath, sizeof(procPath), "/proc/%d/exe", pid)
                  < static_cast<int>(sizeof(procPath)));

        memset(appPath, 0, sizeof(appPath));
        if (readlink(procPath, appPath, sizeof(appPath)) < 0)
        {
            LE_ERROR("readlink(%s) failed %s", procPath, LE_ERRNO_TXT(errno));
            return LE_FAULT;
        }

        // Get the program name from the executable Path.
        namePtr = le_path_GetBasenamePtr(appPath, "/");
    }
    else
    {
        // It's a telaf app.
        namePtr = appPath;
    }

    snprintf(appNameStr, appNameSize, "%s", namePtr);

    LE_INFO("Get appName: %s", appNameStr);

    return LE_OK;
}
