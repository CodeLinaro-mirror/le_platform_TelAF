/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "diagPrivate.h"

#define UPDATE_FILE_PATH_LENGTH 1024

static FILE * mCompleteFileObject = NULL;
static char mCompleteFileAndPathName[UPDATE_FILE_PATH_LENGTH];

// ".incomplete" + '\0' size is [12]
#define INCOMPLETE_FILE_NAME_SIZE (UPDATE_FILE_PATH_LENGTH + 12)
#define INCOMPLETEFILE_SUFFIX ".incomplete"

static char mIncompleteFileAndPathName[INCOMPLETE_FILE_NAME_SIZE];
static FILE * mIncompleteFileObject = NULL;
static DIR * mCompleteDirObject = NULL;
static int activatedMoop = -1;
static uint64_t mRequestDirTotalSize = 0;

static le_sem_Ref_t semRef;

#define fileExist(fileName) (access(fileName, F_OK) == 0)
#define deleteFile(fileName) unlink(fileName)

#define _ERROR_IF_RET_NIL(condition, formatString, ...) \
    do { \
        if (condition) { \
            LE_ERROR(formatString, ##__VA_ARGS__); \
            return; \
        } \
    } while(0)

#define DIAG_38_RESPONSE(errCode) \
    do { \
          _ERROR_IF_RET_NIL( \
            (taf_diagUpdate_SendFileXferResp(rxMsgRef, errCode) != LE_OK), \
            "Failed to send response"); \
         if ((int)errCode != (int)TAF_DIAGUPDATE_FILE_XFER_NO_ERROR) \
            return; \
    } while(0)

#define DIAG_36_RESPONSE(errCode) \
    do { \
          _ERROR_IF_RET_NIL( \
            (taf_diagUpdate_SendXferDataResp(rxMsgRef, errCode, NULL, 0) != LE_OK), \
            "Failed to send response"); \
         if ((int)errCode != (int)TAF_DIAGUPDATE_FILE_XFER_NO_ERROR) \
            return; \
    } while(0)

#define DIAG_36_RESPONSE_DATA(errCode, data, size) \
    do { \
          _ERROR_IF_RET_NIL( \
            (taf_diagUpdate_SendXferDataResp(rxMsgRef, errCode, data, size) != LE_OK), \
            "Failed to send response"); \
         if ((int)errCode != (int)TAF_DIAGUPDATE_FILE_XFER_NO_ERROR) \
            return; \
    } while(0)

#define DIAG_37_RESPONSE(errCode) \
    do { \
          _ERROR_IF_RET_NIL( \
            (taf_diagUpdate_SendXferExitResp(rxMsgRef, errCode, NULL, 0) != LE_OK), \
            "Failed to send response"); \
         if ((int)errCode != (int)TAF_DIAGUPDATE_FILE_XFER_NO_ERROR) \
            return; \
    } while(0)

// Diag Update
static taf_diagUpdate_ServiceRef_t DiagUpdateSvcRef = NULL;
static taf_diagUpdate_RxFileXferMsgHandlerRef_t DiagFileXferMsgRef = NULL;
static taf_diagUpdate_RxXferDataMsgHandlerRef_t DiagXferDataMsgRef = NULL;
static taf_diagUpdate_RxXferExitMsgHandlerRef_t DiagXferExitMsgRef = NULL;

/*
 * Determines whether a given path is a directory.
 */
static int isDirectoryExists(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 1;
        }
    }
    return 0;
}

const char * diagRFT_GetCompleteFileName(void)
{
    return mCompleteFileAndPathName;
}

//Function to convert modeOfOperation type to string
static char* tafModeOfOperationToString(taf_diagUpdate_ModeOfOpsType_t moop)
{
    char* operation;
    switch(moop)
    {
        case TAF_DIAGUPDATE_ADD_FILE :
            operation = "Add file";
            break;
        case TAF_DIAGUPDATE_DELETE_FILE :
            operation = "Delete file";
            break;
        case TAF_DIAGUPDATE_REPLACE_FILE :
            operation = "Replace file";
            break;
        case TAF_DIAGUPDATE_READ_FILE :
            operation = "Read file";
            break;
        case TAF_DIAGUPDATE_READ_DIR:
            operation = "Read dir";
            break;
        case TAF_DIAGUPDATE_RESUME_FILE :
            operation = "Resume file";
            break;
        default :
            operation = "Not supported";
            break;
    }
    return operation;
}

// Function to create a file
static FILE* createFile(const char * pathNamePtr)
{
    le_result_t result;
    mode_t f_attrib;
    f_attrib = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    FILE* file = le_flock_TryCreateStream(pathNamePtr, LE_FLOCK_READ_AND_APPEND,
            LE_FLOCK_FAIL_IF_EXIST , f_attrib, &result);
    if(result != LE_OK)
    {
        LE_ERROR("ERROR TO CREATE FILE: (%m)");
        return NULL;
    }

    return file;

}

// Function to close a file
static void closeFile()
{
    le_flock_CloseStream(mIncompleteFileObject);
    mIncompleteFileObject = NULL;
}

// Function to write data into file
static le_result_t writeFile(const uint8_t *data, const uint16_t len)
{
    int bytes;
    if(mIncompleteFileObject == NULL)
    {
        LE_ERROR("write data error");
        return LE_FAULT;
    }
    if ((bytes = write(fileno(mIncompleteFileObject), data, len) < 0))
    {
        LE_ERROR("write data error");
        return LE_FAULT;
    }
    return LE_OK;
}

// When the security session changing from programming session to anther.
void diagRFT_DeactivateProgramming(void)
{
    if (mCompleteFileObject)
    {
        if (fclose(mCompleteFileObject) == 0)
        {
            mCompleteFileObject = NULL;
        }
    }

    if (mIncompleteFileObject)
    {
        if (fclose(mIncompleteFileObject) == 0)
        {
            mIncompleteFileObject = NULL;
        }
    }

    if (mCompleteDirObject)
    {
        if (closedir(mCompleteDirObject) == 0)
        {
            mCompleteDirObject = NULL;
        }
    }

    memset(mCompleteFileAndPathName, 0, sizeof(mCompleteFileAndPathName));
    memset(mIncompleteFileAndPathName, 0, sizeof(mIncompleteFileAndPathName));

    activatedMoop = -1;
    mRequestDirTotalSize = 0;
}

// Callback function for file transfer request message
void fileXferMsgHandler
(
    taf_diagUpdate_RxFileXferMsgRef_t rxMsgRef,
    taf_diagUpdate_ModeOfOpsType_t moop,
    void* contextPtr
)
{
    le_result_t result;
    char filePathAndName[UPDATE_FILE_PATH_LENGTH];
    size_t fileLen = UPDATE_FILE_PATH_LENGTH;
    long incompleteFileSize = -1;
    long completeFileSize = -1;

    LE_INFO("Received file transfer req msg %s", tafModeOfOperationToString(moop));

    result = taf_diagUpdate_GetFilePathAndName(rxMsgRef,
                                                (uint8_t *)filePathAndName,
                                                &fileLen );
    if(result != LE_OK)
    {
        LE_ERROR("Fail to get file name");
        DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
    }

    LE_INFO("Filename = %s, MOOP: [0x%02X]", filePathAndName, moop);

    le_utf8_Copy(mCompleteFileAndPathName, filePathAndName, UPDATE_FILE_PATH_LENGTH, NULL);

    switch(moop)
    {
        case TAF_DIAGUPDATE_ADD_FILE:
        {
            if (fileExist(mCompleteFileAndPathName))
            {
                LE_ERROR("File [%s] already exist.", mCompleteFileAndPathName);
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }
        }
        // No 'break' for reusing code block
        case TAF_DIAGUPDATE_REPLACE_FILE:
        {
            le_utf8_Copy(mIncompleteFileAndPathName, mCompleteFileAndPathName, UPDATE_FILE_PATH_LENGTH, NULL);
            result = le_utf8_Append(mIncompleteFileAndPathName, INCOMPLETEFILE_SUFFIX,
                                    INCOMPLETE_FILE_NAME_SIZE, NULL);
            if (result == LE_OVERFLOW)
            {
                LE_ERROR("File name path string is too long.(REPLACE)");
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            LE_INFO("Incomplete-File: |%s|", mIncompleteFileAndPathName);

            if (fileExist(mIncompleteFileAndPathName))
            {
                LE_INFO("Remove incomplete file: |%s|", mIncompleteFileAndPathName);

                if (deleteFile(mIncompleteFileAndPathName) != 0)
                {
                    LE_ERROR("Fail to remove file : |%s| (%m)", mIncompleteFileAndPathName);
                    DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
                }
            }

            mIncompleteFileObject = createFile(mIncompleteFileAndPathName);
            if(mIncompleteFileObject == NULL)
            {
                LE_ERROR("Fail to create file |%s|", mIncompleteFileAndPathName);
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            LE_INFO("File: |%s| is created", mIncompleteFileAndPathName);
        }
        break;

        case TAF_DIAGUPDATE_DELETE_FILE:
        {
            if (deleteFile(filePathAndName) != 0)
            {
                LE_ERROR("Fail to delete |%s| (%m)", filePathAndName);
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }
        }
        break;

        case TAF_DIAGUPDATE_RESUME_FILE:
        {
            le_utf8_Copy(mIncompleteFileAndPathName, mCompleteFileAndPathName, UPDATE_FILE_PATH_LENGTH, NULL);
            result = le_utf8_Append(mIncompleteFileAndPathName, INCOMPLETEFILE_SUFFIX,
                                    INCOMPLETE_FILE_NAME_SIZE, NULL);
            if (result == LE_OVERFLOW)
            {
                LE_ERROR("File name path string is too long.(RESUME)");
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            if (! fileExist(mIncompleteFileAndPathName))
            {
                LE_ERROR("Does not exist file : |%s|, can't resume", mIncompleteFileAndPathName);
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            if (fileExist(mCompleteFileAndPathName))
            {
                LE_INFO("Target file exists (from REPLACE).");
            }
            else
            {
                LE_INFO("Target file doesn't exist (from ADD).");
            }

            mIncompleteFileObject = le_flock_OpenStream(mIncompleteFileAndPathName, LE_FLOCK_APPEND, NULL);
            if (mIncompleteFileObject == NULL)
            {
                LE_ERROR("Fail to open: |%s| (%m)", mIncompleteFileAndPathName);
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            if (fseek(mIncompleteFileObject, 0, SEEK_END) != 0)
            {
                LE_ERROR("Fail to seek file: |%s| (%m)", mIncompleteFileAndPathName);
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            incompleteFileSize = ftell(mIncompleteFileObject);
            if (incompleteFileSize == -1)
            {
                LE_ERROR("Fail to tell file size: |%s| (%m)", mIncompleteFileAndPathName);
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            LE_INFO("incompleteFileSize = %ld(0x%08X)", incompleteFileSize,
                                                        (unsigned int) incompleteFileSize);

            result = taf_diagUpdate_SetFilePosition(rxMsgRef, incompleteFileSize);
            if (result != LE_OK)
            {
                LE_ERROR("Fail to set file position as response: %d", result);
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }
        }
        break;

        case TAF_DIAGUPDATE_READ_FILE:
        {
            if (! fileExist(mCompleteFileAndPathName))
            {
                LE_ERROR("Target file does NOT exists.");
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            mCompleteFileObject = le_flock_OpenStream(mCompleteFileAndPathName, LE_FLOCK_READ, NULL);
            if (mCompleteFileObject == NULL)
            {
                LE_ERROR("Fail to open: |%s| (%m)", mCompleteFileAndPathName);
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            LE_INFO("Open file %s for reading", mCompleteFileAndPathName);

            if (fseek(mCompleteFileObject, 0, SEEK_END) != 0)
            {
                LE_ERROR("Fail to seek file to end: |%s| (%m)", mCompleteFileAndPathName);
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            completeFileSize = ftell(mCompleteFileObject);
            if (completeFileSize == -1)
            {
                LE_ERROR("Fail to tell file size: |%s| (%m)", mCompleteFileAndPathName);
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            if (fseek(mCompleteFileObject, 0, SEEK_SET) != 0)
            {
                LE_ERROR("Fail to seek file to begin: |%s| (%m)", mCompleteFileAndPathName);
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            result = taf_diagUpdate_SetFileSizeOrDirInfoLength(rxMsgRef,
                                                               (uint64_t)completeFileSize,
                                                               (uint64_t)completeFileSize);
            if (result != LE_OK)
            {
                LE_ERROR("Fail to set file size or dir info len as response: %d", result);
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }
        }
        break;

        case TAF_DIAGUPDATE_READ_DIR:
        {
            if (! isDirectoryExists(mCompleteFileAndPathName))
            {
                LE_ERROR("NamedPath is NOT a directory");
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            mCompleteDirObject = opendir(mCompleteFileAndPathName);
            if (mCompleteDirObject == NULL)
            {
                LE_ERROR("Can NOT open dir (%m)");
                DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
            }

            LE_INFO("Open dir |%s| for reading", mCompleteFileAndPathName);

            struct dirent * entry;
            LE_DEBUG("In dir: %s", mCompleteFileAndPathName);
            while ((entry = readdir(mCompleteDirObject)) != NULL) {
                LE_DEBUG("-> %s", entry->d_name);
                mRequestDirTotalSize += strlen(entry->d_name) + 1; // + '\0'
            }

            taf_diagUpdate_SetFileSizeOrDirInfoLength(rxMsgRef,
                                                      (uint64_t)mRequestDirTotalSize,
                                                      0);
            seekdir(mCompleteDirObject, 0);
        }
        break;
    }

    DIAG_38_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_NO_ERROR);

    // Mark latest activated MOOP
    activatedMoop = moop;
}

// Callback function for data transfer request message
// Write data into file
void xferDataMsgHandler
(
    taf_diagUpdate_RxXferDataMsgRef_t rxMsgRef,
    void* contextPtr
)
{
    le_result_t result;
    uint8_t xferData[TAF_DIAGUPDATE_MAX_XFER_PARAM_REC_SIZE];
    size_t xferDataLen = 0;
    size_t nbytes = 0;

    LE_DEBUG("Received transfer data req msg in %s", __FUNCTION__);

    if (activatedMoop == TAF_DIAGUPDATE_DELETE_FILE)
    {
        LE_ERROR("Getting file len");
        DIAG_36_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
    }
    // For input data actions
    else if (activatedMoop == TAF_DIAGUPDATE_ADD_FILE
         ||  activatedMoop == TAF_DIAGUPDATE_RESUME_FILE
         ||  activatedMoop == TAF_DIAGUPDATE_REPLACE_FILE)
    {
        result = taf_diagUpdate_GetXferDataParamRecLen( rxMsgRef, (uint16_t *)&xferDataLen);

        if(result != LE_OK || xferDataLen == 0 || xferDataLen > TAF_DIAGUPDATE_MAX_XFER_PARAM_REC_SIZE)
        {
            LE_ERROR("Getting file len");
            DIAG_36_RESPONSE(TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE);
        }

        result = taf_diagUpdate_GetXferDataParamRec( rxMsgRef, xferData, &xferDataLen);

        if(result != LE_OK)
        {
            LE_ERROR("Getting file name");
            DIAG_36_RESPONSE(TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE);
        }

        if(xferDataLen <=0)
        {
            LE_ERROR("Getting data");
            DIAG_36_RESPONSE(TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE);
        }

        if(mIncompleteFileObject != NULL)
        {
            //write data into file
            result = writeFile(xferData, xferDataLen);
            if(result != LE_OK)
            {
                LE_ERROR("Failed to write data");
                DIAG_36_RESPONSE(TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE);
            }
            else
            {
                DIAG_36_RESPONSE(TAF_DIAGUPDATE_XFER_DATA_NO_ERROR);
            }
        }
        else
        {
            LE_ERROR("mIncompleteFileObject is NULL");
            DIAG_36_RESPONSE(TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE);
        }
    }
    // For output data actions
    else if (activatedMoop == TAF_DIAGUPDATE_READ_FILE
         ||  activatedMoop == TAF_DIAGUPDATE_READ_DIR)
    {
        if (activatedMoop == TAF_DIAGUPDATE_READ_FILE)
        {
            if (mCompleteFileObject == NULL)
            {
                LE_ERROR("No opened file for reading");
                DIAG_36_RESPONSE(TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE);
            }

            nbytes = fread(xferData, 1, TAF_DIAGUPDATE_MAX_XFER_PARAM_REC_SIZE, mCompleteFileObject);
            if (nbytes > 0)
            {
                DIAG_36_RESPONSE_DATA(TAF_DIAGUPDATE_XFER_DATA_NO_ERROR, xferData, nbytes);
            }
            else /* == 0, then check EOF or ERROR */
            {
                if (ferror(mCompleteFileObject))
                {
                    LE_ERROR("Fail to read file: %m");
                    DIAG_36_RESPONSE(TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE);
                }
                else
                {
                    LE_INFO("Done for reading file");
                }
            }
        }
        else /* == TAF_DIAGUPDATE_READ_DIR */
        {
            if (mCompleteDirObject == NULL)
            {
                LE_ERROR("No opened dir for reading");
                DIAG_36_RESPONSE(TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE);
            }

            struct dirent *entry = readdir(mCompleteDirObject);
            if (entry != NULL)
            {
                // For each file name including '\0' end of string
                size_t fileItemLength = strlen(entry->d_name) + 1;

                if (fileItemLength > TAF_DIAGUPDATE_MAX_XFER_PARAM_REC_SIZE)
                {
                    LE_ERROR("Too long file name for reading");
                    DIAG_36_RESPONSE(TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE);
                }

                memcpy(xferData, entry->d_name, fileItemLength);
                DIAG_36_RESPONSE_DATA(TAF_DIAGUPDATE_XFER_DATA_NO_ERROR, xferData, fileItemLength);
            }
            else
            {
                LE_INFO("Done for reading directory");
            }
        }
    }
    else
    {
        LE_ERROR("Bad activated MOOP: 0x%02X", activatedMoop);
        DIAG_36_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
    }
}

// Callback function for transfer exit request message
// Close the file
void xferExitMsgHandler
(
    taf_diagUpdate_RxXferExitMsgRef_t rxMsgRef,
    void* contextPtr
)
{
    LE_INFO("Received transfer exit req msg");

    if (activatedMoop == TAF_DIAGUPDATE_DELETE_FILE)
    {
        LE_ERROR("Bad condition detected");
        DIAG_37_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
    }
    else if (activatedMoop == TAF_DIAGUPDATE_ADD_FILE
         ||  activatedMoop == TAF_DIAGUPDATE_RESUME_FILE
         ||  activatedMoop == TAF_DIAGUPDATE_REPLACE_FILE)
    {
        if(mIncompleteFileObject != NULL)
        {
            closeFile();

            LE_INFO("Rename |%s| -> |%s|", mIncompleteFileAndPathName, mCompleteFileAndPathName);

            if (rename(mIncompleteFileAndPathName, mCompleteFileAndPathName) != 0)
            {
                LE_ERROR("fail to rename: (%m)");
                DIAG_37_RESPONSE(TAF_DIAGUPDATE_XFER_DATA_GENERAL_PROGRAMMING_FAILURE);
            }
        }
    }
    else if(activatedMoop == TAF_DIAGUPDATE_READ_FILE
        ||  activatedMoop == TAF_DIAGUPDATE_READ_DIR)
    {
        if (activatedMoop == TAF_DIAGUPDATE_READ_FILE)
        {
            if (mCompleteFileObject != NULL)
            {
                if (fclose(mCompleteFileObject) != 0)
                {
                    LE_ERROR("fail to fclose: (%m)");
                    DIAG_37_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
                }
                mCompleteFileObject = NULL;
            }
        }
        else /* TAF_DIAGUPDATE_READ_DIR */
        {
            if (mCompleteDirObject != NULL)
            {
                if (closedir(mCompleteDirObject) != 0)
                {
                    LE_ERROR("fail to closedir: (%m)");
                    DIAG_37_RESPONSE(TAF_DIAGUPDATE_FILE_XFER_CONDITIONS_NOT_CORRECT);
                }
                mCompleteDirObject = NULL;
            }
        }
    }

    DIAG_37_RESPONSE(TAF_DIAGUPDATE_XFER_EXIT_NO_ERROR);
}

static void* diagUpdateMsgThread(void* ctxPtr)
{
    taf_diagUpdate_ConnectService();

    DiagFileXferMsgRef = taf_diagUpdate_AddRxFileXferMsgHandler(
                            DiagUpdateSvcRef,
                            fileXferMsgHandler, NULL);

    DiagXferDataMsgRef = taf_diagUpdate_AddRxXferDataMsgHandler(
                            DiagUpdateSvcRef,
                            xferDataMsgHandler, NULL);

    DiagXferExitMsgRef = taf_diagUpdate_AddRxXferExitMsgHandler(
                            DiagUpdateSvcRef,
                            xferExitMsgHandler, NULL);

    if (DiagFileXferMsgRef == NULL
    ||  DiagXferDataMsgRef == NULL
    ||  DiagXferExitMsgRef == NULL)
    {
        LE_ERROR("Fail to register handler for diagUpdateSvc !");
        le_sem_Post(semRef);
        return NULL;
    }

    le_sem_Post(semRef);
    le_event_RunLoop();
    return NULL;
}

le_result_t diagRequestFileTransfer_Init(void)
{
    semRef = le_sem_Create("SemRef", 0);

    //get diag update reference
    DiagUpdateSvcRef = taf_diagUpdate_GetService();
    if(DiagUpdateSvcRef == NULL)
    {
        LE_ERROR("Get diagUpdate service");
        return LE_FAULT;
    }

    // Create diag upate message handle thread to handle filetransfer(0x38), transferdata(0x36)
    // and transfer exit(0x37)
    le_thread_Ref_t diagUpdateThreadRef = le_thread_Create("diagUpdateTd",
            diagUpdateMsgThread, NULL);

    le_thread_Start(diagUpdateThreadRef);
    le_sem_Wait(semRef);

    return LE_OK;
}
