/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
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

//--------------------------------------------------------------------------------------------------
/**
 * Key owner app name.
 */
//--------------------------------------------------------------------------------------------------
#define OWNER_APP "keySharingOwnerTest"

//--------------------------------------------------------------------------------------------------
/**
 * Key shared app name.
 */
//--------------------------------------------------------------------------------------------------
#define SHARED_APP "keySharingClientTest"

//--------------------------------------------------------------------------------------------------
/**
 * Non-existed key.
 */
//--------------------------------------------------------------------------------------------------
#define NONEXIST_KEY "nonexistKey"

//--------------------------------------------------------------------------------------------------
/**
 * Unshared key. (Never shared to other apps).
 */
//--------------------------------------------------------------------------------------------------
#define UNSHARED_KEY "unsharedKey"

//--------------------------------------------------------------------------------------------------
/**
 * Shared key that only allows to use for 10 times, then cancels the sharing.
 */
//--------------------------------------------------------------------------------------------------
#define SHORT_SHARED_KEY "shortSharedKey"

//--------------------------------------------------------------------------------------------------
/**
 * Shared key that allows to use permanently, never cancels the sharing.
 */
//--------------------------------------------------------------------------------------------------
#define PERM_SHARED_KEY "permanentSharedKey"

//--------------------------------------------------------------------------------------------------
/**
 * Shared key that allows to be deleted by shared app.
 */
//--------------------------------------------------------------------------------------------------
#define DELE_SHARED_KEY "deletableSharedKey"

//--------------------------------------------------------------------------------------------------
/**
 * Shared key that allows to be exported by shared app.
 */
//--------------------------------------------------------------------------------------------------
#define EXPO_SHARED_KEY "exportableSharedKey"

//--------------------------------------------------------------------------------------------------
/**
 * Shared key that allows to get shared app list by shared app.
 */
//--------------------------------------------------------------------------------------------------
#define LIST_SHARED_KEY "listableSharedKey"

//--------------------------------------------------------------------------------------------------
/**
 * Plain and Cipher data max size.
 */
//--------------------------------------------------------------------------------------------------
#define TEXT_MAX_SIZE 256

//--------------------------------------------------------------------------------------------------
/**
 * AEAD used by AES ECM key.
 */
//--------------------------------------------------------------------------------------------------
#define AES_AEAD "KEY_SHARING_TESTING_AEAD"

//--------------------------------------------------------------------------------------------------
/**
 * NONCE used by AES ECM key (fixed 12 bytes)
 */
//--------------------------------------------------------------------------------------------------
#define AES_NONCE "MY_KEY_NONCE"

//--------------------------------------------------------------------------------------------------
/**
 * Data chunk size.
 */
//--------------------------------------------------------------------------------------------------
#define CHUNK_SIZE TEXT_MAX_SIZE

//--------------------------------------------------------------------------------------------------
/**
 * Message to indicate the client test is done.
 */
//--------------------------------------------------------------------------------------------------
#define KEY_SHARING_TEST_DONE "KeySharingTestIsDone"
