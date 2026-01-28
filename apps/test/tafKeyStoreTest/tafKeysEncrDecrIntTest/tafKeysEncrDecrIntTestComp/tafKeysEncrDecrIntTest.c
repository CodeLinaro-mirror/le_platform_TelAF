/*Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Define message maximum buffer size.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_KS_MAX_BUFFER_SIZE 4096

//--------------------------------------------------------------------------------------------------
/**
 * Store encrypted message in /data/ partition, so that it can be used for encryption.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_KSENCRYPT_FOTA_MSG "/data/le_fs/encryptMsg" //Store encrypted message

//--------------------------------------------------------------------------------------------------
/**
 * Store encrypted message size in /data/ partition,so it can be used for encryption.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_KSENCRYPT_FOTA_MSG_SIZE "/data/le_fs/encryptMsgSize"

//--------------------------------------------------------------------------------------------------
/**
 * Store original message size in /data/ partition, so that it can be used for validation
 * of decrypted message.
 */
//--------------------------------------------------------------------------------------------------
#define TAF_KSENCRYPT_FOTA_ORG_MSG "/data/le_fs/originalMsg" //Store original message

//--------------------------------------------------------------------------------------------------
/**
 * Crypto session reference.
 */
//--------------------------------------------------------------------------------------------------
static taf_ks_CryptoSessionRef_t SessionRef;

//--------------------------------------------------------------------------------------------------
/**
 * Input message entered.
 */
//--------------------------------------------------------------------------------------------------
static char InputMessage[TAF_KS_MAX_BUFFER_SIZE]= { 0 };

//--------------------------------------------------------------------------------------------------
/**
 * Encrypted message data.
 */
//--------------------------------------------------------------------------------------------------
static uint8_t EncryptedData[TAF_KS_MAX_BUFFER_SIZE] = { 0 };

//--------------------------------------------------------------------------------------------------
/**
 * Read encrypted message data from /data/ partition.
 */
//--------------------------------------------------------------------------------------------------
static uint8_t EncryptedMsgRead[TAF_KS_MAX_BUFFER_SIZE] = { 0 };

//--------------------------------------------------------------------------------------------------
/**
 * Read original message data from /data/ partition.
 */
//--------------------------------------------------------------------------------------------------
static char OriginalMsgRead[TAF_KS_MAX_BUFFER_SIZE] = { 0 };

//--------------------------------------------------------------------------------------------------
/**
 * Key reference.
 */
//--------------------------------------------------------------------------------------------------
static taf_ks_KeyRef_t KeyRef;

//--------------------------------------------------------------------------------------------------
/**
 * Total size of an encrypted or decrypted message.
 */
//--------------------------------------------------------------------------------------------------
static size_t TotalSize;

//--------------------------------------------------------------------------------------------------
/**
 * Store an encrypted message data.
 */
//--------------------------------------------------------------------------------------------------
static size_t StoreEncSize;

//--------------------------------------------------------------------------------------------------
/**
 * Fetch an encrypted message size from /data/ partition.
 */
//--------------------------------------------------------------------------------------------------
static size_t FetchEncSize;

//--------------------------------------------------------------------------------------------------
/**
 * Define the unique key ID, which is used to create persistent key reference.
 */
//--------------------------------------------------------------------------------------------------
static const char KeyId[] = "KeyEncDecTest";

//--------------------------------------------------------------------------------------------------
/**
 * Nonce key to set the NONCE specifically for the AES CBC/CTR/GCM keys.
 */
//--------------------------------------------------------------------------------------------------
static const uint8_t NonceKey[TAF_KS_MAX_AES_NONCE_SIZE ] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
                      0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10};

//--------------------------------------------------------------------------------------------------
/**
 * FD monitor reference.
 */
//--------------------------------------------------------------------------------------------------
static le_fdMonitor_Ref_t MonitorRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Fetch original message data stored from /data/ partition.
 */
//--------------------------------------------------------------------------------------------------
static void FetchOriginalMessage
(
   void
)
{
    LE_TEST_INFO("FetchOriginalMessage");

    //Fetch original message from /data/ partition
    if (access(TAF_KSENCRYPT_FOTA_ORG_MSG, F_OK) == 0)
    {
        FILE* fp_orgMsg = fopen(TAF_KSENCRYPT_FOTA_ORG_MSG, "r");
        if (fp_orgMsg == NULL)
        {
            LE_TEST_INFO("Can not open original message file %s.", TAF_KSENCRYPT_FOTA_ORG_MSG);
            return;
        }
        else
        {
            memset(OriginalMsgRead, 0, sizeof(OriginalMsgRead));
            if (fread(OriginalMsgRead, sizeof(OriginalMsgRead), 1, fp_orgMsg) <= 0)
            {
                LE_TEST_INFO("Read %s failed", TAF_KSENCRYPT_FOTA_ORG_MSG);
            }
            else
            {
                LE_TEST_INFO("Original message read is succesful");
            }
            LE_TEST_INFO("FetchOriginalMessage OriginalMsgRead: %s", OriginalMsgRead);
            LE_TEST_INFO("FetchOriginalMessage OriginalMsgRead size: %lu", sizeof(OriginalMsgRead));
            fclose(fp_orgMsg);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Store original message data into /data/ partition.
 */
//--------------------------------------------------------------------------------------------------
static void StoreOriginalMessage
(
   void
)
{
    LE_TEST_INFO("StoreOriginalMessage");

    FILE* fp_orgMsg = fopen(TAF_KSENCRYPT_FOTA_ORG_MSG, "w");
    if (fp_orgMsg == NULL)
    {
       LE_TEST_INFO("Can not open original message file %s.", TAF_KSENCRYPT_FOTA_ORG_MSG);
       return;
    }
    else
    {
        LE_TEST_INFO("Writing Original message into a file : %s",InputMessage);
        fwrite(InputMessage,sizeof(InputMessage),1,fp_orgMsg);
        fflush(fp_orgMsg);
        fclose(fp_orgMsg);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Fetch an encrypted message stored from /data/ partition.
 */
//--------------------------------------------------------------------------------------------------
static void FetchEncryptMessage
(
    void
)
{
    LE_TEST_INFO("FetchEncryptMessage");

    //Fetch Encrypted message size /data/ partition
    if (access(TAF_KSENCRYPT_FOTA_MSG_SIZE, F_OK) == 0)
    {
        FILE* fp_size = fopen(TAF_KSENCRYPT_FOTA_MSG_SIZE, "r");
        if (fp_size == NULL)
        {
            LE_TEST_INFO("Can not open encrypted message size file %s.", TAF_KSENCRYPT_FOTA_MSG_SIZE);
            return;
        }
        else
        {
            FetchEncSize = 0;
            if (fread(&FetchEncSize, sizeof(FetchEncSize), 1, fp_size) <= 0)
            {
                LE_TEST_INFO("Read %s failed", TAF_KSENCRYPT_FOTA_MSG);
            }
            else
            {
                LE_TEST_INFO("Encrypted message size read is succesful");
            }
            fclose(fp_size);
        }
    }

    //Fetch Encrypted message from /data/ partition
    if (access(TAF_KSENCRYPT_FOTA_MSG, F_OK) == 0)
    {
        FILE* fp = fopen(TAF_KSENCRYPT_FOTA_MSG, "r");
        if (fp == NULL)
        {
            LE_TEST_INFO("Can not open encrypted message file %s.", TAF_KSENCRYPT_FOTA_MSG);
            return;
        }
        else
        {
            memset(EncryptedMsgRead, 0, sizeof(EncryptedMsgRead));
            if (fread(EncryptedMsgRead, FetchEncSize, 1, fp) <= 0)
            {
                LE_TEST_INFO("Read %s failed", TAF_KSENCRYPT_FOTA_MSG);
            }
            else
            {
                LE_TEST_INFO("Encrypted message read is succesful");
                LE_TEST_INFO("FetchEncryptMessage FetchEncSize :%lu", FetchEncSize);
            }
            LE_TEST_INFO("FetchEncryptMessage : %s", EncryptedMsgRead);
            fclose(fp);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Store an encrypted message stored from /data/ partition.
 */
//--------------------------------------------------------------------------------------------------
static void StoreEncryptMessage
(
    const uint8_t EncryptedMsg[TAF_KS_MAX_BUFFER_SIZE] ///< [IN] Encrypted Msg.
)
{
    LE_TEST_INFO("StoreEncryptMessage :%s", EncryptedMsg);
    LE_TEST_INFO("StoreEncryptMessage size = %"PRIuS".", StoreEncSize);

    //Store the encrypted message size n /data/ partition
    FILE* fp_size = fopen(TAF_KSENCRYPT_FOTA_MSG_SIZE, "w");
    if (fp_size == NULL)
    {
       LE_TEST_INFO("Can not open encrypted message size file %s.", TAF_KSENCRYPT_FOTA_MSG_SIZE);
       return;
    }
    else
    {
        LE_TEST_INFO("Writing Encrypted message size into a file");
        fwrite(&StoreEncSize,sizeof(StoreEncSize),1,fp_size);
        fflush(fp_size);
        fclose(fp_size);
    }

    //Store the encrypted message in /data/ partition
    FILE* fp = fopen(TAF_KSENCRYPT_FOTA_MSG, "w");
    if (fp == NULL)
    {
       LE_TEST_INFO("Can not open encrypted message file %s.", TAF_KSENCRYPT_FOTA_MSG);
       return;
    }
    else
    {
        LE_TEST_INFO("Writing Encrypted message into a file");
        fwrite(EncryptedMsg,StoreEncSize,1,fp);
        fflush(fp);
        fclose(fp);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Decrypt the encrypted message.
 */
//--------------------------------------------------------------------------------------------------
static void TafMessageDecryption
(
    void
)
{
    le_result_t result;
    uint8_t decryptedData[TAF_KS_MAX_BUFFER_SIZE] = { 0 };
    size_t decryptedDataSize = sizeof(decryptedData);
    size_t decSize;

    LE_TEST_INFO("tafMessageDecryption");

    //Start Decryption: key-> AES_ENCRYPT_DECRYPT crypto key-> CRYPTO_DECRYPT
    LE_TEST_ASSERT(LE_OK == taf_ks_GetKey(KeyId, &KeyRef), "Test Geting the key.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(KeyRef, &SessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetAesNonce(SessionRef, NonceKey, sizeof(NonceKey)),
                   "Test seting nonce.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(SessionRef, TAF_KS_CRYPTO_DECRYPT),
                   "Test start session for data decryption.");
    FetchEncryptMessage();//Fetch the encrypted message stored in /data/ partition
    memset(decryptedData,0,sizeof(decryptedData));//Reset the decrypted message before fetching it.
    LE_TEST_INFO("Before Decrypted message:%s",decryptedData);
    result = taf_ks_CryptoSessionProcess(SessionRef,
                                         EncryptedMsgRead,//Stored encrypted message from /data/ partition
                                         FetchEncSize,    //Stored encrypted message size
                                         decryptedData,
                                         &decryptedDataSize);
    decSize = decryptedDataSize;
    LE_TEST_ASSERT(LE_OK == result, "Test decrypted size = %"PRIuS"", decSize);
    decryptedDataSize = sizeof(decryptedData) - decSize;
    result = taf_ks_CryptoSessionEnd(SessionRef,
                                     NULL, 0,
                                     decryptedData + decSize,
                                     &decryptedDataSize);
    TotalSize = decSize + decryptedDataSize;
    printf("======================================================\n");
    printf("Decrypted message: %s\n",decryptedData);
    LE_TEST_INFO("After Decrypted message:%s",decryptedData);
    printf("======================================================\n");
    LE_TEST_INFO("Test message decrypted total size = %"PRIuS".", TotalSize);
    FetchOriginalMessage();//Fetch the original message stored in /data/ partition
    if(strlen(OriginalMsgRead)>0) //if input string is provided
    {
        LE_TEST_ASSERT(TotalSize == sizeof(OriginalMsgRead), "Test message data size verification.");
        LE_TEST_ASSERT(0 == memcmp(OriginalMsgRead, decryptedData, TotalSize),
                   "Test default message decrypted data verification.");
        printf("Message decryption verification :PASS!\n\n");
    }
    else //when no input message is provided
    {
        LE_TEST_INFO("No input message is provided!");
        printf("No input message is provided! ->FAIL!\n\n");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Encrypt the input message entered.
 */
//--------------------------------------------------------------------------------------------------
static void TafMessageEncryption
(
    void
)
{
    le_result_t result = LE_NOT_PERMITTED;
    size_t encryptedDataSize = sizeof(EncryptedData);
    size_t encSize;

    LE_TEST_INFO("tafMessageEncryption");

    //Start encryption: key->AES_ENCRYPT_DECRYPT  crypto key-> CRYPTO_ENCRYPT
    LE_TEST_ASSERT(LE_OK == taf_ks_GetKey(KeyId, &KeyRef), "Test Geting the key.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionCreate(KeyRef, &SessionRef),
                   "Test session creation.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionSetAesNonce(SessionRef, NonceKey, sizeof(NonceKey)),
                   "Test seting nonce.");
    LE_TEST_ASSERT(LE_OK == taf_ks_CryptoSessionStart(SessionRef, TAF_KS_CRYPTO_ENCRYPT),
                   "Test start session for data encryption.");
    FetchOriginalMessage();//Fetch if any input message was already been stored in /data/ partition
    if(strlen(InputMessage)>0) //if any input string is provided
    {
        LE_TEST_INFO("length of input message :%lu",strlen(InputMessage));
        LE_TEST_INFO("==================================================");
        LE_TEST_INFO(" Message:<<<<<<<<<<<< %s >>>>>>>>>>>>>>>>>>>>>>>>>>", InputMessage);
        LE_TEST_INFO("==================================================");
        result = taf_ks_CryptoSessionProcess(SessionRef,
                                             (uint8_t*)InputMessage,
                                             sizeof(InputMessage),
                                             EncryptedData,
                                             &encryptedDataSize);
    }
    else if(strlen(OriginalMsgRead)>0) //if original message was already stored in /data/
    {
        LE_TEST_INFO("length of input message :%lu",strlen(OriginalMsgRead));
        LE_TEST_INFO("==================================================");
        LE_TEST_INFO(" Original Message:<<<<<<<<<<<< %s >>>>>>>>>>>>>>>>>>>>>>>>>>", OriginalMsgRead);
        LE_TEST_INFO("==================================================");
        result = taf_ks_CryptoSessionProcess(SessionRef,
                                             (uint8_t*)OriginalMsgRead,
                                             sizeof(OriginalMsgRead),
                                             EncryptedData,
                                             &encryptedDataSize);
    }
    else //When neither input message is provided nor original stored in /data/ partition
    {
        printf("No input message is provided! ->FAIL!\n\n");
    }
    encSize = encryptedDataSize;
    LE_TEST_INFO("encryptedData: %s",EncryptedData);
    LE_TEST_ASSERT(LE_OK == result, "Test encrypted data size = %"PRIuS".", encSize);
    encryptedDataSize = sizeof(EncryptedData) - encSize;
    result = taf_ks_CryptoSessionEnd(SessionRef,
                                     NULL, 0,
                                     EncryptedData + encSize,
                                     &encryptedDataSize);
    TotalSize = encryptedDataSize + encSize;
    LE_TEST_ASSERT(LE_OK == result,
                   "Test message encrypted data total size = %"PRIuS".", TotalSize);
    StoreEncSize = TotalSize;
    StoreEncryptMessage(EncryptedData);//store the encrypted message in /data/ partition
    printf("\nThe input message is succefully encrypted!\n\n");
}

//--------------------------------------------------------------------------------------------------
/**
 * Input message function.
 */
//--------------------------------------------------------------------------------------------------
static void TafMessageInput
(
    void
)
{
    printf("Enter an input message (up to 4095 characters):");
    if (fgets(InputMessage, sizeof(InputMessage), stdin) != NULL)
    {
        size_t len = strlen(InputMessage);
        if (len > 0 && InputMessage[len-1] == '\n')
        {
            InputMessage[len-1] = '\0';
        }
        else if (len == TAF_KS_MAX_BUFFER_SIZE - 1)
        {
            printf("Warning: Input may have been truncated as it reached the maximum buffer size.\n");
        }
        printf("You entered:%s\n\n",InputMessage);
        StoreOriginalMessage();//Store the entered message in /data/ partition
    }
    else
    {
        printf("Error reading input\n");
        InputMessage[0] = '\0'; // Ensure empty string on error
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete the key created using key reference.
 */
//--------------------------------------------------------------------------------------------------
static void TafMessageKeysDeletion
(
    void
)
{
    le_result_t result;
    LE_TEST_INFO("tafMessageKeysDeletion");

    //Key deletion
    taf_ks_GetKey(KeyId, &KeyRef);
    result = taf_ks_DeleteKey(KeyRef);
    LE_TEST_INFO("result :%d",(int) result);
    if (LE_NOT_FOUND == result)
    {
        LE_TEST_INFO("Key doesnt exist");
        printf("Key doesnt exist, so deletion is FAIL !\n");
    }
    else
    {
        LE_TEST_ASSERT(LE_OK == result, "Delete the created key.");
        LE_TEST_INFO("Key reference is deleted");
        printf("Key deletion is PASS\n");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Create the key using unique key id.
 */
//--------------------------------------------------------------------------------------------------
static void TafMessageKeysCreation
(
    void
)
{
    le_result_t result;
    LE_TEST_INFO("tafMessageKeysCreation");
    //Key creation-> TAF_KS_AES_ENCRYPT_DECRYPT
    if (LE_NOT_FOUND == taf_ks_GetKey(KeyId, &KeyRef))
    {
        LE_TEST_ASSERT(LE_OK == taf_ks_CreateKey(KeyId, TAF_KS_AES_ENCRYPT_DECRYPT, &KeyRef),
                                "Test AES key encryption key creation");
        result = taf_ks_ProvisionAesKeyValue(KeyRef,
                                             TAF_KS_AES_SIZE_128,
                                             TAF_KS_AES_MODE_CBC_PAD_NONE,
                                             NULL, 0);
        LE_TEST_ASSERT(LE_OK == result, "Test AES encryption key value provision.");
    }
    else
    {
        LE_TEST_INFO("Key has already been created KeyRef: %p",KeyRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Options menu print function.
 */
//--------------------------------------------------------------------------------------------------
void PrintUsages()
{

    printf("************************************************\n");;
    printf("Usages:\n");
    printf("Enter 'h' of 'help' or '?' -> to display help menu\n");
    printf("To run the test app -> app runProc tafKeysEncrDecrIntTest tafKeysEncrDecrIntTest\n");
    printf("Enter 'KeyDel' -> for TAF_KS_AES_ENCRYPT_DECRYPT Key deletion\n");
    printf("Enter 'EncryptInputMsg' to generate the key, to input the message and to be encrypted\n");
    printf("Enter 'Decrypt'-> for TAF_KS_CRYPTO_DECRYPT Decryption\n");
    printf("Enter 'q'  -> to quit the application\n");
    printf("************************************************\n\n");
}

//--------------------------------------------------------------------------------------------------
/**
 * FD monitor event handler function to accept the input commands.
 */
//--------------------------------------------------------------------------------------------------
void KeyStoreEventHandler(int fd, short events)
{
    if (events & POLLIN)    // Data available to read
    {
        char inputStr[TAF_KS_MAX_BUFFER_SIZE]={0};
        ssize_t bytesRead = read(fd, inputStr, sizeof(inputStr));
        if (bytesRead > 0)
        {
            // Process the data read from the console
            printf("Input string read from FDMonitor(stdin): %.*s\n", (int)bytesRead, inputStr);
            LE_INFO("Input string: %s num of bytes: %ld",inputStr,bytesRead);
            if (strncmp(inputStr, "h", 1) == 0 || strncmp(inputStr, "?", 1) == 0 ||
                strncmp(inputStr, "help", 4) == 0)
            {
                PrintUsages();
            }
            else if(strncmp(inputStr, "EncryptInputMsg", 15) == 0)
            {
                LE_TEST_INFO("---------- Message Keys creation started:---");
                TafMessageKeysCreation();
                LE_TEST_INFO("---------- Message Keys creation completed:---");
                LE_TEST_INFO("---------- Input Message started:---");
                TafMessageInput();
                LE_TEST_INFO("---------- Input Message completed:---");
                LE_TEST_INFO("---------- Message Encryption started:---");
                TafMessageEncryption();
                LE_TEST_INFO("---------- Message Encryption completed:---");
            }
            else if(strncmp(inputStr, "KeyDel", 6) == 0)
            {
                LE_TEST_INFO("---------- Message Keys deletion started:---");
                TafMessageKeysDeletion();
                LE_TEST_INFO("---------- Message Keys deletion completed:---");
            }
            else if(strncmp(inputStr, "Decrypt", 7) == 0)
            {
                LE_TEST_INFO("---------- Message Decryption started:---");
                TafMessageDecryption();
                LE_TEST_INFO("---------- Message Decryption completed:---");
            }
            else if (strncmp(inputStr, "q", 1) == 0)
            {
                le_fdMonitor_Delete(MonitorRef);
                LE_TEST_INFO("Exiting the input monitoring...");
                LE_TEST_EXIT;
            }
            else
            {
                LE_TEST_INFO("Invalid input!");
                printf("Invalid input!\n");
            }
        }
    }
    if ((events & POLLERR) || (events & POLLHUP) || (events & POLLRDHUP))// Error or hang-up?
    {
        // Handle error or hang-up
        printf("Error or hang-up detected\n");
        LE_ERROR("FD monitor error or hang-up detected, cleaning up resources");

        // Delete the monitor and exit
        le_fdMonitor_Delete(MonitorRef);
        MonitorRef = NULL;
        LE_TEST_EXIT;
    }
}

COMPONENT_INIT
{
    int fd = STDIN_FILENO;
    PrintUsages();
    MonitorRef = le_fdMonitor_Create("KeysEncDecTestApp",
                                      fd, KeyStoreEventHandler, POLLIN);

    if(MonitorRef != NULL)
    {
        LE_TEST_INFO("FdMonitor creation is succesful");
    }
    else
    {
        LE_TEST_INFO("FdMonitor creation is failed monitorRef:%p",MonitorRef);
        LE_TEST_FATAL("Failed to create FD monitor. Application cannot continue.");
    }
}