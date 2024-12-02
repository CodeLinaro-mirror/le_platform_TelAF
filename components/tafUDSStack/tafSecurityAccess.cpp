/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafSecurityAccess.hpp"
#include "configuration.hpp"

using namespace telux::tafsvc;

namespace taf {
namespace uds {

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MEvent_s {
    uint8_t sig;
} MEvent_t;

typedef uint8_t MState_t;

typedef MState_t (* MStateHandler_t) (void *self, MEvent_t const *ev);

typedef struct MFsm_s {
    MStateHandler_t state_fn;
} MFsm_t;

#define MFsm_Construct(self, init_fn) ((self)->state_fn = (init_fn))

void MFsm_init(MFsm_t *self, MEvent_t const *ev);
void MFsm_dispatch(MFsm_t *self, MEvent_t const *ev);

#define MFsm_ctor(self_, initial_) do { \
    (self_)->state_fn = (initial_); \
} while (0)

#define M_RETURN_HANDLED ((MState_t)0U)
#define M_RETURN_IGNORED ((MState_t)1U)
#define M_RETURN_TRANSME ((MState_t)2U)

#define M_Handled()   (M_RETURN_HANDLED)
#define M_Ignored()   (M_RETURN_IGNORED)
#define M_Translate(target_fn) \
    (((MFsm_t *)self)->state_fn = (MStateHandler_t)(target_fn),\
     M_RETURN_TRANSME)

enum MPrivateSignal_s {
    M_ENTRY_SIG = 0,
    M_EXIT_SIG,
    M_INIT_SIG,

    M_USER_SIG /* to be customized */
};

static MEvent_t const M_PrivateEvents[] = {
    { (uint8_t) M_ENTRY_SIG },
    { (uint8_t) M_EXIT_SIG  },
    { (uint8_t) M_INIT_SIG  }
};

void MFsm_init(MFsm_t *self, MEvent_t const *ev)
{
    (*self->state_fn)(self, ev); /* init_fn first */
    (void) (*self->state_fn)(self, &M_PrivateEvents[M_ENTRY_SIG]);
}

void MFsm_dispatch(MFsm_t *self, MEvent_t const *ev)
{
    MStateHandler_t hdlr = self->state_fn;
    MState_t st = (*hdlr)(self, ev);

    if (st == M_RETURN_TRANSME) {
        (void) (*hdlr)(self, &M_PrivateEvents[M_EXIT_SIG]);
        (void) (*self->state_fn)(self, &M_PrivateEvents[M_ENTRY_SIG]);
    }
}

/* ------------------------ [ Implementation ] ------------------------*/

typedef struct SecAccEventTag
{
    MEvent_t event;
    SecAccReport_t *report;
} SecAccEvent_t;

typedef struct SecurityLevel_s
{
    uint16_t Security_Level;
    uint32_t Att_Cnt; /* <-- variable */
    uint32_t Att_Cnt_Limit;
    uint16_t Static_Seed;
    uint16_t Delay_Timer;

    uint32_t key_size;
    uint32_t seed_size;
    uint32_t request_seed_id;
    le_sls_Link_t link;
} SecurityLevel_t;

typedef struct SecuritySession_s
{
    uint8_t session_id;
    le_sls_List_t level_list;   /* alias as : Subfunction */
    SecurityLevel_t * active_level;
    SecurityLevel_t * unlocked_level;
    le_sls_Link_t link;
} SecuritySession_t;

typedef struct AO_SecurityAccess_s
{
    MFsm_t super;
    le_sls_List_t session_list;
    SecuritySession_t * current_session;

    /* Each Active Object has their own dummy default configuration */
    SecuritySession_t * default_session;

    /* Record last pending session-id (uds-session-id) */
    uint8_t last_pending_session_id;
    /* Record last pending signal (control/timeout) */
    SecAccSignal_t last_pending_signal;

    /* Reflect to UDS manager */
    UdsCommunicationMgr * mMgr;

    /* Record the interface name for VLan */
    #define IF_NAME_MAX_LEN 64
    char ifname[IF_NAME_MAX_LEN];

    /* Reference for Delay_Timer */
    le_timer_Ref_t delay_timer_ref;

} AO_SecurityAccess_t;

/* Memory pool for some struct instances */
static le_mem_PoolRef_t SecurityActiveObjectPool;
static le_mem_PoolRef_t SecuritySessionPool;
static le_mem_PoolRef_t SecurityLevelPool;
static le_hashmap_Ref_t AO_timer_mapping_table;
#define MAX_AO_TIMER_TABLE_SIZE 16

le_event_Id_t SecAccEventIdRef = NULL;

/* default_session is not supported in Security-Access Field */
#define SEC_ACC_DEFAULT_SESSION_ID 0xFF

static SecuritySession_t * GetDefaultSession(void)
{
    SecuritySession_t * session = (SecuritySession_t*)le_mem_ForceAlloc(SecuritySessionPool);
    session->session_id = SEC_ACC_DEFAULT_SESSION_ID;
    session->level_list = LE_SLS_LIST_INIT;
    session->active_level = NULL; /* Must be NULL to be checked */
    session->unlocked_level = NULL; /* Must be NULL to be checked */
    session->link = LE_SLS_LINK_INIT;

    return session;
}

/* Standard States from ISO 14229:2020 Annex I section */
MState_t State_initial(AO_SecurityAccess_t * self, MEvent_t const *ev);
MState_t State_LockedNoActiveSeed(AO_SecurityAccess_t * self, MEvent_t const *ev);
MState_t State_LockedWaitingForKey(AO_SecurityAccess_t * self, MEvent_t const *ev);
MState_t State_UnlockedNoActiveSeed(AO_SecurityAccess_t * self, MEvent_t const *ev);
MState_t State_UnlockedWaitingForKey(AO_SecurityAccess_t * self, MEvent_t const *ev);

typedef MState_t (* State_Function) (AO_SecurityAccess_t * self, MEvent_t const *ev);

#define CFG_UDS_DIAG_SVC_NAME "tafDiagSvc:"
#define CFG_SECURITY_ACCESS_TREE CFG_UDS_DIAG_SVC_NAME "/security_access"
#define CFG_NODE_PATH_LEN 128
#define DELAY_TIMER_NAME_SIZE 64

#define SecAccType_yy(ev) ((((SecAccEvent_t *) (ev))->report->mgr->recvBuf[1]) & 0x7F)
#define SubFunction_xx(self) ((self)->current_session->active_level->Security_Level)
#define EVENT(ev) ((SecAccEvent_t *) ev)
#define CURRENT_SESSION_ID(ev) ((uint32_t)EVENT(ev)->report->mgr->SessionType)

#define SECACC_ASSERT_FATAL(condition) \
do { \
    if (! (condition) ) { \
        LE_FATAL("SecAcc-Fatal: %s", #condition); \
    } \
} while (0)

void TryToCreateStorageFromTree(AO_SecurityAccess_t *self)
{
    char nodePath[CFG_NODE_PATH_LEN] = {0};

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn(
                                         CFG_SECURITY_ACCESS_TREE);

    snprintf(nodePath, sizeof(nodePath), "%s", self->ifname);

    LE_DEBUG("Security config tree: %s", nodePath);

    if (le_cfg_NodeExists(iteratorRef, nodePath))
    {
        LE_INFO("Tree for security_access already exists.");

        /* FIXME: Now, if we want to update the configuration from YAML to configTree
         *        use the target-tool 'config' to delete the 'tafDiagSvc:' subTree,
         *        then restart our tafDiagSvc and their APPs!
         */

        le_cfg_CancelTxn(iteratorRef);
        return;
    }
    else /* no existing */
    {
        SecuritySession_t * sess;
        SecurityLevel_t * level;

        memset(nodePath, 0, sizeof(nodePath));

        LE_SLS_FOREACH(&self->session_list, sess, SecuritySession_t, link)
        {
            LE_SLS_FOREACH(&sess->level_list, level, SecurityLevel_t, link)
            {
                /* Example: if-name/session-id/level/Att_Cnt */
                snprintf(nodePath, sizeof(nodePath),
                         "%s/%02X/%02X/Att_Cnt", self->ifname,
                         sess->session_id, level->Security_Level);
                le_cfg_SetInt(iteratorRef, nodePath, level->Att_Cnt);
                LE_DEBUG("Tree node: [ %s ] created", nodePath);
            }
        }
        le_cfg_CommitTxn(iteratorRef);
        LE_INFO("Initialized the tree: " CFG_SECURITY_ACCESS_TREE);
    }
}

static void AO_SecurityAccess_ctor
(
    AO_SecurityAccess_t *self,
    UdsCommunicationMgr *mgr,
    char * ifname
)
{
    MFsm_ctor(&self->super, (MStateHandler_t)&State_initial);
    try
    {
        LE_INFO("Parsing security_binding ...");
        cfg::Node & sec_binding = cfg::get_root_node().get_child("security_binding");

        self->session_list = LE_SLS_LIST_INIT;
        self->current_session = NULL;
        self->default_session = NULL; /* Allocated dynamically */
        self->last_pending_session_id = SEC_ACC_DEFAULT_SESSION_ID;
        self->last_pending_signal = INVALID_SIG;
        self->mMgr = mgr; /* To the manager instance */
        self->delay_timer_ref = NULL; /* Will be filled soon */

        SECACC_ASSERT_FATAL(strlen(ifname) + 1 <= IF_NAME_MAX_LEN);
        le_utf8_Copy(self->ifname, ifname, IF_NAME_MAX_LEN, NULL);

        for (auto & binding: sec_binding) {
            std::string sname = binding.first;
            LE_INFO("- Pick sesion: %s", sname.c_str());
            SecuritySession_t * sess = (SecuritySession_t*)le_mem_ForceAlloc(SecuritySessionPool);

            cfg::Node & attr = binding.second;
            sess->session_id = attr.get<int>("session_id");
            sess->active_level = NULL;
            sess->unlocked_level = NULL;
            sess->level_list = LE_SLS_LIST_INIT;
            cfg::Node & sec_level = attr.get_child("security_level");

            for (auto & level_item: sec_level) {
                std::string level_name = level_item.second.get<std::string>("");
                cfg::Node & level_node = cfg::top_diagnostic_session_security_level<std::string>(
                        "short_name", level_name);

                SecurityLevel_t* level = (SecurityLevel_t*) le_mem_ForceAlloc(SecurityLevelPool);
                level->Att_Cnt_Limit = level_node.get<int>("num_failed_security_access");
                level->Delay_Timer = level_node.get<int>("security_delay_time");
                level->Static_Seed = level_node.get<bool>("static_seed");
                level->Security_Level = level_node.get<int>("level_id");
                level->seed_size = level_node.get<int>("seed_size");
                level->key_size = level_node.get<int>("key_size");
                level->request_seed_id = level_node.get<int>("request_seed_id");

                level->Att_Cnt = 0; /* Zero is required */
                level->link = LE_SLS_LINK_INIT;
                le_sls_Queue(&sess->level_list, &(level->link));
            }
            sess->link = LE_SLS_LINK_INIT;
            le_sls_Queue(&self->session_list, &(sess->link));
        }

        self->default_session = GetDefaultSession();
        SECACC_ASSERT_FATAL(self->default_session != NULL);

        /* Add dummy default session to session-list */
        le_sls_Queue(&self->session_list, &(self->default_session->link));
    }
    catch (const std::exception& e)
    {
        LE_FATAL("Bad configuration for security access service init: %s", e.what());
    }

    LE_INFO("[%s] Done", __FUNCTION__);
}

static void ResponseAllZeroSeed(AO_SecurityAccess_t * self, MEvent_t const * ev)
{
    SecAccEvent_t const * evp = (SecAccEvent_t *)ev;
    uint8_t * recv_buf = evp->report->mgr->recvBuf;
    uint8_t * send_buf = evp->report->mgr->sendBuf;

    send_buf[0] = recv_buf[0] + 0x40;
    send_buf[1] = recv_buf[1] & 0x7F;

    uint32_t seed_bit_size = self->current_session->active_level->seed_size;
    uint32_t seed_byte_size = (seed_bit_size % 8) ? (seed_bit_size / 8) + 1 : (seed_bit_size / 8);
    LE_INFO("Seed bit_size: %d, byte_size:%d", seed_bit_size, seed_byte_size);

    /* FIXME: Need to be checked size in init-stage */

    memset(send_buf + 2, 0x00, seed_byte_size);

    evp->report->mgr->sendDataLen = 2 + seed_byte_size; // 2 = (SID + SUBFUNC)

    evp->report->mgr->SendData(&evp->report->mgr->udsRespAddrInfo);
    *evp->report->is_internal = true;
    evp->report->mgr->remoteError = LE_OK;
    le_sem_Post(evp->report->sem);
}


static void ResponseNRC(AO_SecurityAccess_t * self, MEvent_t const * ev, uint8_t nrc)
{
    SecAccEvent_t const * evp = (SecAccEvent_t *)ev;

    if (nrc != 0x00)
    {
        *evp->report->is_internal = true;
        le_result_t result = evp->report->mgr->SendNRC(
                                evp->report->mgr->recvBuf[0],
                                nrc,
                                &evp->report->mgr->udsRespAddrInfo);
        evp->report->mgr->remoteError = result;
    }
    else /* nrc == 0 will be fine */
    {
        *evp->report->is_internal = false;
        evp->report->mgr->remoteError = LE_OK;
    }

    SECACC_ASSERT_FATAL(evp->report->sem != NULL);
    le_sem_Post(evp->report->sem);
}


static void SetResponseNRC(AO_SecurityAccess_t * self, MEvent_t const * ev, uint8_t nrc)
{
    SecAccEvent_t const * evp = (SecAccEvent_t *)ev;

    evp->report->mgr->nrcCode = nrc;

    SECACC_ASSERT_FATAL(evp->report->sem != NULL);
    le_sem_Post(evp->report->sem);
}

static void LoadAttCntAndDelayTimer(AO_SecurityAccess_t * self, MEvent_t const *ev)
{
    (void)ev;

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn(CFG_SECURITY_ACCESS_TREE);

    SecuritySession_t * sess;
    SecurityLevel_t * level;
    char nodePath[CFG_NODE_PATH_LEN] = {0};

    LE_SLS_FOREACH(&self->session_list, sess, SecuritySession_t, link)
    {
        LE_SLS_FOREACH(&sess->level_list, level, SecurityLevel_t, link)
        {
            snprintf(nodePath, sizeof(nodePath),
                     "%s/%02X/%02X/Att_Cnt", self->ifname,
                     sess->session_id, level->Security_Level);
            level->Att_Cnt = le_cfg_GetInt(iteratorRef, nodePath, 0);
        }
    }
    le_cfg_CancelTxn(iteratorRef);
    LE_INFO("Load Att_Cnt from tree");

    /* FIXME: Now, Delay_Timer duration time is fixed, to be reset every time */
}

static bool PreConditionIsNotFulfilled(AO_SecurityAccess_t * self, MEvent_t const *ev)
{
    uint8_t sub_function = EVENT(ev)->report->mgr->recvBuf[1] & 0x7F;
    uint32_t current_session_id = CURRENT_SESSION_ID(ev);

    if (current_session_id == DEFAULT_SESSION) {
        LE_INFO("[SecAcc] default_ession is not supported");
        return true;
    }

    SecuritySession_t * sess;
    SecurityLevel_t * level;

    bool found = false;
    LE_SLS_FOREACH(&self->session_list, sess, SecuritySession_t, link)
    {
        if (sess->session_id != current_session_id) {
            continue;
        }

        LE_SLS_FOREACH(&sess->level_list, level, SecurityLevel_t, link)
        {
            if (level->Security_Level == sub_function) {
                found = true;
                break;
            }
        }
        break;
    }

    if (found != true) {
        LE_INFO("[SecAcc] Requested security-access-type is out of range (cfg)");
        return true;
    }

    return false;
}

static bool MsgLengthIsNok(AO_SecurityAccess_t * self, MEvent_t const *ev, SecAccSignal_t sig)
{
    uint16_t recv_buf_len = ((SecAccEvent_t *) ev)->report->mgr->recvDataLen;

    if (sig == REQUEST_SEED_SIG)
    {
        if (recv_buf_len < UDS_SECURITY_ACCESS_REQ_MIN_LEN)
        {
            LE_INFO("[SecAcc] Request-Seed-Msg length is too short");
            return true;
        }
        else {

            /* Just check the configuration from YAML and pass-in from D-Tool */

            uint8_t current_session_id = CURRENT_SESSION_ID(ev);
            uint8_t sub_function = EVENT(ev)->report->mgr->recvBuf[1] & 0x7F;

            SecuritySession_t * sess;
            SecurityLevel_t * level;

            LE_SLS_FOREACH(&self->session_list, sess, SecuritySession_t, link)
            {
                if (sess->session_id != current_session_id) {
                    continue;
                }

                LE_SLS_FOREACH(&sess->level_list, level, SecurityLevel_t, link)
                {
                    if (level->Security_Level == sub_function) {

                        uint32_t seed_byte_size = (level->seed_size % 8)
                                                  ? (level->seed_size / 8) + 1
                                                  : (level->seed_size / 8);
                        if (recv_buf_len != seed_byte_size) {

                            /* Prompt & pass it to APP anyway */
                            LE_ERROR("[SecAcc] Seed-Size from D-Tool is different from YAML cfg");
                        }

                        break;
                    }
                }
                break;
            }
        }
    }
    else if (sig == SEND_KEY_SIG)
    {
        if (recv_buf_len < UDS_SECURITY_ACCESS_SEND_KEY_REQ_MIN_LEN)
        {
            LE_INFO("[SecAcc] Send-Key-Msg is too short");
            return true;
        }

        SECACC_ASSERT_FATAL(self->current_session->active_level != NULL);

        uint32_t key_bit_size = self->current_session->active_level->key_size;
        uint16_t key_byte_size = (key_bit_size % 8) ? (key_bit_size / 8) + 1 : (key_bit_size / 8);

        LE_INFO("Key bit_size: %d, byte_size:%d", key_bit_size, key_byte_size);

        if (recv_buf_len - 2 != key_byte_size) // 2 = sizeof( SID + SUBFUNCTION )
        {
            LE_ERROR("[SecAcc] Key size is not match YAML configuration");

            /* FIXME: for now, we pass the mismatched KEY to APP */
            // return true;
        }
    }

    return false;
}

static bool DelayTimerIsNotExpired(AO_SecurityAccess_t * self, MEvent_t const *ev)
{
    if (le_timer_IsRunning(self->delay_timer_ref))
    {
        uint32_t remaining = le_timer_GetMsTimeRemaining(self->delay_timer_ref);
        LE_DEBUG("Delay_Timer remains: %u(ms)", remaining);
        return true;
    }

    /* When the device reboot, the timer should be recovered based on Att_Cnt == Att_Cnt_Limit */

    SecuritySession_t * sess;
    SecurityLevel_t * level;

    LE_SLS_FOREACH(&self->session_list, sess, SecuritySession_t, link)
    {
        LE_SLS_FOREACH(&sess->level_list, level, SecurityLevel_t, link)
        {
            /* At the same time, only one takes effect for the Delay_Timer */

            if (level->Att_Cnt == level->Att_Cnt_Limit) {

                le_timer_SetMsInterval(self->delay_timer_ref, level->Delay_Timer * 1000);
                le_timer_Start(self->delay_timer_ref);

                /* Temporarily activated, to be used in DELAY_TIMER_EXPIRED_SIG */
                self->current_session = sess;
                self->current_session->active_level = level;

                LE_INFO("Timer isn't running ? but Att_Cnt == Att_Cnt_Limit(%u), session: %u, level: %u",
                        level->Att_Cnt_Limit,
                        sess->session_id,
                        level->Security_Level);
                return true; /* Not expired */
            }
        }
    }

    return false;
}

static void ActivateSubfunction(AO_SecurityAccess_t * self, MEvent_t const *ev)
{
    uint8_t sub_function = EVENT(ev)->report->mgr->recvBuf[1] & 0x7F;
    uint32_t current_session_id = CURRENT_SESSION_ID(ev);

    SecuritySession_t * sess;
    SecurityLevel_t * level;

    LE_SLS_FOREACH(&self->session_list, sess, SecuritySession_t, link)
    {
        if (sess->session_id != current_session_id) {
            continue;
        }

        LE_SLS_FOREACH(&sess->level_list, level, SecurityLevel_t, link)
        {
            if (level->Security_Level == sub_function) {
                self->current_session->active_level = level;
                break;
            }
        }
        self->current_session = sess;
        break;
    }

    SECACC_ASSERT_FATAL(self->current_session != NULL);
    SECACC_ASSERT_FATAL(self->current_session->active_level != NULL);
}

static void UnlockRequestedSecLevelAndLockOthers(AO_SecurityAccess_t * self)
{
    SECACC_ASSERT_FATAL(self->current_session->active_level != NULL);
    SECACC_ASSERT_FATAL(self->current_session->unlocked_level != NULL);

    if (self->current_session->active_level
    !=  self->current_session->unlocked_level) {

        self->current_session->unlocked_level = self->current_session->active_level;
        self->current_session->active_level = NULL;
    }
}

/* Note: to be called after set the Att_Cnt */
static void SaveAttCntToTree(AO_SecurityAccess_t * self)
{
    char nodePath[CFG_NODE_PATH_LEN] = {0};
    snprintf(nodePath, sizeof(nodePath),
             CFG_SECURITY_ACCESS_TREE "%s/%02X/%02X/Att_Cnt",
             self->ifname,
             self->current_session->session_id,
             self->current_session->active_level->Security_Level);
    le_cfg_QuickSetInt(nodePath, self->current_session->active_level->Att_Cnt);
}

static bool RequestedSubFunctionIsStaticSeed(AO_SecurityAccess_t * self, MEvent_t const *ev)
{
    SecAccEvent_t * evp = (SecAccEvent_t *) ev;

    uint8_t sub_function = 0x00;

    if (evp->report->type == REQUEST_SEED_RESPONSE_SIG) {
        sub_function = evp->report->mgr->recvBuf[1] & 0x7F;
    } else if (evp->report->type == SEND_KEY_RESPONSE_SIG) {
        /* When the SEND_KEY was received, the Security_Level should be (SecAccType - 1) */
        sub_function = (evp->report->mgr->recvBuf[1] & 0x7F) - 1;
    }

    SecurityLevel_t * level;
    LE_SLS_FOREACH(&self->current_session->level_list, level, SecurityLevel_t, link)
    {
        if (level->Security_Level == sub_function) {
            break;
        }
    }

    return level->Static_Seed;
}


static bool SecAccTypeIsNotActive(AO_SecurityAccess_t * self, MEvent_t const *ev)
{
    SecAccEvent_t * evp = (SecAccEvent_t *) ev;
    uint8_t sub_function = evp->report->mgr->recvBuf[1] & 0x7F;

    if (self->current_session->active_level->Security_Level == sub_function) {
        return false;
    }

    return true;
}

static bool KeyIsNok(AO_SecurityAccess_t * self, MEvent_t const *ev)
{
    SecAccEvent_t * evp = (SecAccEvent_t *) ev;
    uint8_t nrc = evp->report->mgr->nrcCode;

    if (nrc == INVALID_KEY) { // 0x35
        return true;
    }
    else if (nrc != POSITIVE_RESPONSE) {
        LE_ERROR("NRC != POSITIVE_RESPONSE from APP");

        /* For now, only INVALID_KEY should be true, others to be treated as false */
        return false;
    }
    else { /* nrc == POSITIVE_RESPONSE */
        return false;
    }
}

static void TriggerDelayTimer(AO_SecurityAccess_t * self)
{
    if (le_timer_IsRunning(self->delay_timer_ref)) {
        le_timer_Stop(self->delay_timer_ref);
    }
    uint32_t delay_s = self->current_session->active_level->Delay_Timer * 1000;
    le_timer_SetMsInterval(self->delay_timer_ref, delay_s);
    le_timer_Start(self->delay_timer_ref);
}

static void UnlockCurrentSecLevel(AO_SecurityAccess_t * self)
{
    self->current_session->unlocked_level =
                     self->current_session->active_level;
}

static void SwitchSessionBasedOnEvent(AO_SecurityAccess_t * self, MEvent_t const * ev)
{
    if (CURRENT_SESSION_ID(ev) == DEFAULT_SESSION) {
        self->current_session = self->default_session;
    }
    else {
        self->current_session = NULL;

        LE_INFO("current session id: %d", CURRENT_SESSION_ID(ev));

        SecuritySession_t * sess;
        LE_SLS_FOREACH(&self->session_list, sess, SecuritySession_t, link)
        {
            if (CURRENT_SESSION_ID(ev) == sess->session_id) {
                self->current_session = sess;
                break;
            }
        }

        SECACC_ASSERT_FATAL(self->current_session != NULL);
    }
}

static bool RequestedLevelIsUnlocked(AO_SecurityAccess_t * self, MEvent_t const *ev)
{
    SecAccEvent_t * evp = (SecAccEvent_t *) ev;

    uint8_t security_level_type = evp->report->mgr->recvBuf[1] & 0x7F;

    if (self->current_session->unlocked_level == NULL) {
        return false;
    }
    else { /* unlocked, check the SecAccType further */
        if (self->current_session->unlocked_level->Security_Level == security_level_type) {
            return true;
        } else {
            return false;
        }
    }
}

static void LockCurrentSession(AO_SecurityAccess_t * self)
{
    self->current_session->unlocked_level = NULL;
}

static void DeactivateAndLock(AO_SecurityAccess_t * self)
{
    self->current_session->active_level = NULL;
    LockCurrentSession(self);
}

static void SwitchSessionAfterDelayTimerTimeout(AO_SecurityAccess_t * self)
{
    if (self->last_pending_session_id == DEFAULT_SESSION) {
        self->current_session = self->default_session;
    }
    else {
        self->current_session = NULL;

        SecuritySession_t * sess;
        LE_SLS_FOREACH(&self->session_list, sess, SecuritySession_t, link)
        {
            if (self->last_pending_session_id == sess->session_id) {
                self->current_session = sess;
                break;
            }
        }
        LE_INFO("Pending session ID: %d", self->last_pending_session_id);
        SECACC_ASSERT_FATAL(self->current_session != NULL);
    }

    /* Reset all last-pending stuff */
    self->last_pending_signal = INVALID_SIG;
    self->last_pending_session_id = SEC_ACC_DEFAULT_SESSION_ID;
}

static void MarkLastPendingSession(AO_SecurityAccess_t * self, MEvent_t const *ev, SecAccSignal_t sig)
{
    self->last_pending_signal = sig;

    if (sig == SESSION_CONTROL_SIG) {
        self->last_pending_session_id = CURRENT_SESSION_ID(ev);
    }
    else if (sig == SESSION_TIMEOUT_SIG) {
        self->last_pending_session_id = SEC_ACC_DEFAULT_SESSION_ID;
    }
    else {
        LE_FATAL("%s should be called in [session control/timeout] event bank", __FUNCTION__);
    }
}

/* Dispatch the task to the Diag-Application */
#define GenerateSeed(self, ev) /* Nothing to do in stack */
#define StoreSeed(self, ev) /* Nothing to do in stack */
#define ReturnSavedSeed(self, ev) /* Nothing to do in stack */
#define ReturnNewSeed(self, ev) /* Nothing to do in stack */
#define ClearGeneratedSeed(self, ev) /* Nothing to do in stack */
#define SaveAttCntToTree_NOOP(self, ev) /* Nothing to do for Att_Cnt */

/* --- State Machine --- */

MState_t State_initial(AO_SecurityAccess_t * self, MEvent_t const *ev)
{
    LE_INFO("SECURITY_ACCESS: %s", __FUNCTION__);
    LoadAttCntAndDelayTimer(self, ev);
    self->current_session = self->default_session;
    return M_Translate(&State_LockedNoActiveSeed);
}

MState_t State_LockedNoActiveSeed(AO_SecurityAccess_t * self, MEvent_t const *ev)
{
    LE_DEBUG("S-Function [%s]", __FUNCTION__);
    switch(ev->sig) {
        case REQUEST_SEED_SIG: {
            if (MsgLengthIsNok(self, ev, REQUEST_SEED_SIG)) {
                ResponseNRC(self, ev, 0x13);
                return M_Handled();
            }
            else if (DelayTimerIsNotExpired(self, ev)) {
                ResponseNRC(self, ev, 0x37);
                return M_Handled();
            }
            else if (PreConditionIsNotFulfilled(self, ev)) {
                ResponseNRC(self, ev, 0x22);
                return M_Handled();
            }
            else {
                LE_DEBUG("TO -> APP");
                /* Pass REQ to APP, wait for REQUEST_SEED_RESPONSE_SIG from APP */
                ResponseNRC(self, ev, 0x00);
                return M_Handled();
            }
        }
        case REQUEST_SEED_RESPONSE_SIG: {
            uint8_t nrc = ((SecAccEvent_t *) ev)->report->mgr->nrcCode;
            if (nrc == 0x00) {
                GenerateSeed(self, ev);
                StoreSeed(self, ev);
                ActivateSubfunction(self, ev);
                ReturnNewSeed(self, ev);
                SetResponseNRC(self, ev, 0x00);
                return M_Translate(&State_LockedWaitingForKey);
            }
            else {
                /* Pass NRC from APP */
                SetResponseNRC(self, ev, nrc);

                /* Keep current state */
                return M_Handled();
            }
        }
        case SEND_KEY_SIG: {
            ResponseNRC(self, ev, 0x24);
            return M_Handled();
        }
        case DELAY_TIMER_EXPIRED_SIG: {
            self->current_session->active_level->Att_Cnt = 0;
            SaveAttCntToTree(self);
            if (self->last_pending_signal != INVALID_SIG) {
                DeactivateAndLock(self);
                SwitchSessionAfterDelayTimerTimeout(self);
            }
            LE_INFO("Delay_Timer signal was handled");

            /* Anyway, we're already on State_LockedNoActiveSeed */
            return M_Handled();
        }
        case SESSION_CONTROL_SIG: {
            if (DelayTimerIsNotExpired(self, ev)) {
                MarkLastPendingSession(self, ev, SESSION_CONTROL_SIG);
                return M_Handled();
            }
            else {
                DeactivateAndLock(self);
                SwitchSessionBasedOnEvent(self, ev);
                return M_Handled();
            }
            return M_Handled();
        }
        case SESSION_TIMEOUT_SIG: {
            if (DelayTimerIsNotExpired(self, ev)) {
                MarkLastPendingSession(self, ev, SESSION_TIMEOUT_SIG);
                return M_Handled();
            }
            else {
                DeactivateAndLock(self);
                self->current_session = self->default_session;
                return M_Handled();
            }
        }
    }

    return M_Ignored();
}

MState_t State_LockedWaitingForKey(AO_SecurityAccess_t * self, MEvent_t const *ev)
{
    LE_DEBUG("S-Function [%s]", __FUNCTION__);
    switch(ev->sig) {
        case REQUEST_SEED_SIG: {
            if (MsgLengthIsNok(self, ev, REQUEST_SEED_SIG)) {
                ResponseNRC(self, ev, 0x13);
                return M_Translate(&State_LockedNoActiveSeed);
            }
            else {
                LE_DEBUG("TO -> APP");
                /* Pass to APP, wait for REQUEST_SEED_RESPONSE_SIG */
                ResponseNRC(self, ev, 0x00);
                return M_Handled();
            }
        }
        case REQUEST_SEED_RESPONSE_SIG: {
            uint8_t nrc = ((SecAccEvent_t *) ev)->report->mgr->nrcCode;
            if (nrc == 0x00) {
                if (RequestedSubFunctionIsStaticSeed(self, ev)) {
                    if (SecAccTypeIsNotActive(self, ev)) {
                        GenerateSeed(self, ev);
                        StoreSeed(self, ev);
                        ActivateSubfunction(self, ev);
                        ReturnNewSeed(self, ev);
                        SetResponseNRC(self, ev, 0x00);
                        return M_Handled();
                    }
                    else { /* Security type is active */
                        ReturnSavedSeed(self, ev);
                        SetResponseNRC(self, ev, 0x00);
                        return M_Handled();
                    }
                }
                else { /* Static_Seed == false */
                    GenerateSeed(self, ev);
                    StoreSeed(self, ev);
                    ActivateSubfunction(self, ev);
                    ReturnNewSeed(self, ev);
                    SetResponseNRC(self, ev, 0x00);
                    return M_Handled();
                }
            }
            else {
                /* Pass NRC from APP */
                SetResponseNRC(self, ev, nrc);

                /* Change back */
                return M_Translate(&State_LockedNoActiveSeed);
            }
        }
        case SEND_KEY_SIG: {
            if (MsgLengthIsNok(self, ev, SEND_KEY_SIG)) {
                SaveAttCntToTree_NOOP(self, ev);
                ResponseNRC(self, ev, 0x13);
                return M_Translate(&State_LockedNoActiveSeed);
            }
            else if (SecAccType_yy(ev) != SubFunction_xx(self) + 1) {
                SaveAttCntToTree_NOOP(self, ev);
                ResponseNRC(self, ev, 0x24);
                return M_Translate(&State_LockedNoActiveSeed);
            }
            else { /* Pass to the APP and wait for SEND_KEY_RESPONSE_SIG */

                LE_DEBUG("TO -> APP");
                ResponseNRC(self, ev, 0x00);
                return M_Handled();
            }
        }
        case SEND_KEY_RESPONSE_SIG: {
            /* Note: for event -> :SEND_KEY_RESPONSE_SIG:
             * SetResponseNRC is need instead ResponseNRC */
            if (KeyIsNok(self, ev)) {
                /* Att_Cnt + 1 >= Att_Cnt_Limit */
                if (self->current_session->active_level->Att_Cnt + 1
                >= self->current_session->active_level->Att_Cnt_Limit) {
                    self->current_session->active_level->Att_Cnt =
                          self->current_session->active_level->Att_Cnt_Limit;
                    SaveAttCntToTree(self);
                    TriggerDelayTimer(self);
                    SetResponseNRC(self, ev, 0x36);
                    return M_Translate(&State_LockedNoActiveSeed);
                }
                else {
                    ++ self->current_session->active_level->Att_Cnt;
                    SaveAttCntToTree(self);
                    SetResponseNRC(self, ev, 0x35);
                    return M_Translate(&State_LockedNoActiveSeed);
                }
            }
            else {
                uint8_t nrc = EVENT(ev)->report->mgr->nrcCode;
                if (nrc != 0x00) { /* NRC from APP maybe not 0x00 */
                    SetResponseNRC(self, ev, nrc);

                    /* Change back */
                    return M_Translate(&State_LockedNoActiveSeed);
                } else { /* ALL PASS */
                    self->current_session->active_level->Att_Cnt = 0;
                    SaveAttCntToTree(self);
                    UnlockCurrentSecLevel(self);
                    if (! RequestedSubFunctionIsStaticSeed(self, ev)) {
                        ClearGeneratedSeed(self, ev);
                    }
                    SetResponseNRC(self, ev, 0x00);
                    return M_Translate(&State_UnlockedNoActiveSeed);
                }
            }
        }
        case SESSION_CONTROL_SIG: {
            DeactivateAndLock(self);
            SwitchSessionBasedOnEvent(self, ev);
            return M_Translate(&State_LockedNoActiveSeed);
        }
        case SESSION_TIMEOUT_SIG: {
            DeactivateAndLock(self);
            self->current_session = self->default_session;
            return M_Translate(&State_LockedNoActiveSeed);
        }
    }

    return M_Ignored();
}

MState_t State_UnlockedNoActiveSeed(AO_SecurityAccess_t * self, MEvent_t const *ev)
{
    LE_DEBUG("S-Function [%s]", __FUNCTION__);
    switch(ev->sig) {
        case REQUEST_SEED_SIG: {
            if (MsgLengthIsNok(self, ev, REQUEST_SEED_SIG)) {
                ResponseNRC(self, ev, 0x13);
                return M_Handled();
            }
            else if (PreConditionIsNotFulfilled(self, ev)) {
                ResponseNRC(self, ev, 0x22);
                return M_Handled();
            }
            else if (DelayTimerIsNotExpired(self, ev)) {
                ResponseNRC(self, ev, 0x37);
                return M_Handled();
            }
            else if (RequestedLevelIsUnlocked(self, ev)) {
                ResponseAllZeroSeed(self, ev);
                return M_Handled();
            }
            else { /* event > { 8 } < */
                GenerateSeed(self, ev);
                StoreSeed(self, ev);

                LE_DEBUG("To -> APP");
                /* Pass to APP, wait for REQUEST_SEED_RESPONSE_SIG */
                ResponseNRC(self, ev, 0x00);
                return M_Handled();
            }
        }
        case REQUEST_SEED_RESPONSE_SIG: {
            uint8_t nrc = ((SecAccEvent_t *) ev)->report->mgr->nrcCode;
            if (nrc == 0x00) {
                ActivateSubfunction(self, ev);
                SetResponseNRC(self, ev, 0x00); /* No Error -> 0x00 */

                /* To next state */
                return M_Translate(&State_UnlockedWaitingForKey);
            }
            else {
                /* Pass NRC from APP */
                SetResponseNRC(self, ev, nrc);

                /* Keep current state for this case */
                return M_Handled();
            }
        }
        case SEND_KEY_SIG: {
            ResponseNRC(self, ev, 0x24);
            return M_Handled();
        }
        case DELAY_TIMER_EXPIRED_SIG: {
            self->current_session->active_level->Att_Cnt = 0;
            SaveAttCntToTree(self);
            if (self->last_pending_signal != INVALID_SIG) {
                DeactivateAndLock(self);
                SwitchSessionAfterDelayTimerTimeout(self);

                /* Need to change state to State_LockedNoActiveSeed */
                return M_Translate(&State_LockedNoActiveSeed);
            }
            else {
                return M_Handled();
            }
        }
        case SESSION_CONTROL_SIG: {
            if (DelayTimerIsNotExpired(self, ev)) {
                MarkLastPendingSession(self, ev, SESSION_CONTROL_SIG);

                /* In unlocked state, lock current-session */
                LockCurrentSession(self);

                /* Keep current state, wait for: DELAY_TIMER_EXPIRED_SIG */
                return M_Handled();
            }
            else {
                DeactivateAndLock(self);
                SwitchSessionBasedOnEvent(self, ev);
                return M_Translate(&State_LockedNoActiveSeed);
            }
        }
        case SESSION_TIMEOUT_SIG: {
            if (DelayTimerIsNotExpired(self, ev)) {
                MarkLastPendingSession(self, ev, SESSION_TIMEOUT_SIG);

                /* In unlocked state, lock current-session */
                LockCurrentSession(self);

                /* Keep current state, wait for: DELAY_TIMER_EXPIRED_SIG */
                return M_Handled();
            }
            else {
                DeactivateAndLock(self);
                self->current_session = self->default_session;
                return M_Translate(&State_LockedNoActiveSeed);
            }
        }
    }

    return M_Ignored();
}

MState_t State_UnlockedWaitingForKey(AO_SecurityAccess_t * self, MEvent_t const *ev)
{
    LE_DEBUG("S-Function [%s]", __FUNCTION__);
    switch(ev->sig) {
        case REQUEST_SEED_SIG: {
            if (MsgLengthIsNok(self, ev, REQUEST_SEED_SIG)) {
                ResponseNRC(self, ev, 0x13);
                return M_Translate(&State_UnlockedNoActiveSeed);
            }
            else if (RequestedLevelIsUnlocked(self, ev)) {
                ResponseAllZeroSeed(self, ev);
                return M_Translate(&State_UnlockedNoActiveSeed);
            }
            else {
                LE_DEBUG("TO -> APP");
                /* Pass task to APP, wait for REQUEST_SEED_RESPONSE_SIG */
                ResponseNRC(self, ev, 0x00);
                return M_Handled();
            }
        }
        case REQUEST_SEED_RESPONSE_SIG: {
            uint8_t nrc = ((SecAccEvent_t *) ev)->report->mgr->nrcCode;
            if (nrc == 0x00) {
                if (RequestedSubFunctionIsStaticSeed(self, ev)) {
                    if (SecAccTypeIsNotActive(self, ev)) {
                        GenerateSeed(self, ev);
                        StoreSeed(self, ev);
                        ActivateSubfunction(self, ev);
                        ReturnNewSeed(self, ev);
                        SetResponseNRC(self, ev, 0x00);
                        return M_Handled();
                    }
                    else { /* Security type is active */
                        ReturnSavedSeed(self, ev);
                        SetResponseNRC(self, ev, 0x00);
                        return M_Handled();
                    }
                }
                else { /* Static_Seed == false */
                    GenerateSeed(self, ev);
                    StoreSeed(self, ev);
                    ActivateSubfunction(self, ev);
                    ReturnNewSeed(self, ev);
                    SetResponseNRC(self, ev, 0x00);
                    return M_Handled();
                }
            }
            else {
                /* Pass NRC from APP */
                SetResponseNRC(self, ev, nrc);

                /* Change back */
                return M_Translate(&State_UnlockedNoActiveSeed);
            }
        }
        case SEND_KEY_SIG: {
            if (MsgLengthIsNok(self, ev, SEND_KEY_SIG)) {
                ResponseNRC(self, ev, 0x13);
                return M_Translate(&State_UnlockedNoActiveSeed);
            }
            else if (SecAccType_yy(ev) != SubFunction_xx(self) + 1) {
                ResponseNRC(self, ev, 0x24);
                return M_Translate(&State_UnlockedNoActiveSeed);
            }
            else { /* Pass to the APP and wait for SEND_KEY_RESPONSE_SIG */
                LE_DEBUG("TO -> APP");
                ResponseNRC(self, ev, 0x00);
                return M_Handled();
            }
        }
        case SEND_KEY_RESPONSE_SIG: {
            /* Note: for event -> :SEND_KEY_RESPONSE_SIG:
             * SetResponseNRC is need instead ResponseNRC */
            if (KeyIsNok(self, ev)) {
                /* Att_Cnt + 1 >= Att_Cnt_Limit */
                if (self->current_session->active_level->Att_Cnt + 1
                >= self->current_session->active_level->Att_Cnt_Limit) {
                    self->current_session->active_level->Att_Cnt =
                          self->current_session->active_level->Att_Cnt_Limit;
                    SaveAttCntToTree(self);
                    TriggerDelayTimer(self);
                    SetResponseNRC(self, ev, 0x36);
                    return M_Translate(&State_UnlockedNoActiveSeed);
                }
                else {
                    ++ self->current_session->active_level->Att_Cnt;
                    SaveAttCntToTree(self);
                    SetResponseNRC(self, ev, 0x35);
                    return M_Translate(&State_UnlockedNoActiveSeed);
                }
            }
            else {
                uint8_t nrc = EVENT(ev)->report->mgr->nrcCode;
                if (nrc != 0x00) { /* NRC from APP maybe not 0x00 */
                    SetResponseNRC(self, ev, nrc);

                    /* Change back */
                    return M_Translate(&State_UnlockedNoActiveSeed);
                }
                else { /* ALL PASS */
                    self->current_session->active_level->Att_Cnt = 0;
                    SaveAttCntToTree(self);

                    /* Loack currently unlocked security level,
                    * Unlock security level for SubFunction xx */
                    UnlockRequestedSecLevelAndLockOthers(self);

                    if (! RequestedSubFunctionIsStaticSeed(self, ev)) {
                        ClearGeneratedSeed(self, ev);
                    }

                    SetResponseNRC(self, ev, 0x00);
                    return M_Translate(&State_UnlockedNoActiveSeed);
                }
            }
        }
        case SESSION_CONTROL_SIG: {
            DeactivateAndLock(self);
            SwitchSessionBasedOnEvent(self, ev);
            return M_Translate(&State_LockedNoActiveSeed);
        }
        case SESSION_TIMEOUT_SIG: {
            DeactivateAndLock(self);
            self->current_session = self->default_session;
            return M_Translate(&State_LockedNoActiveSeed);
        }
    }

    return M_Ignored();
}

bool SecurityAccess_IsUnlocked(UdsCommunicationMgr * mgr)
{
    return (mgr->mSecurityAccess->current_session->unlocked_level != NULL);
}

static void SecAcc_DelayTimerHandler(le_timer_Ref_t timerRef)
{
    LE_DEBUG("%s .. timerRef: %p", __FUNCTION__, timerRef);

    AO_SecurityAccess_t * object =
            (AO_SecurityAccess_t *) le_hashmap_Get(AO_timer_mapping_table, timerRef);
    LE_DEBUG("%s .. AO : %p", __FUNCTION__, object);

    LE_INFO("report -> DELAY_TIMER_EXPIRED_SIG");
    SecAccReport_t report = {
        .type = DELAY_TIMER_EXPIRED_SIG,
        .mgr = object->mMgr,
    };
    le_event_Report(SecAccEventIdRef, &report, sizeof(report));
}

static void SecurityAccessEventHandler(void * reportPayLoadPtr)
{
    SecAccReport_t * report = (SecAccReport_t *) reportPayLoadPtr;
    SecAccEvent_t ev = {
        .event = { report->type },
        .report = report,
    };
    MFsm_dispatch((MFsm_t *)report->mgr->mSecurityAccess, (MEvent_t *)&ev);
}

/* Thread for security access service */
static void * SecurityAccessWorker(void * ctx)
{
    /* Be used in Security Access Thread */
    le_cfg_ConnectService();

    SecAccEventIdRef = le_event_CreateId("Sec-Acc-Evt", sizeof(SecAccReport_t));
    le_event_AddHandler("Sec-Acc-Evt-Hdlr", SecAccEventIdRef, SecurityAccessEventHandler);

    /* post to be ready */
    le_sem_Post((le_sem_Ref_t) ctx);

    le_event_RunLoop();
    return NULL;
}

void SecurityAccess_Init(void * u, void * p)
{
    SecurityActiveObjectPool = le_mem_CreatePool("SecurityActiveObjectPool", sizeof(AO_SecurityAccess_t));
    SecuritySessionPool = le_mem_CreatePool("SecuritySessionPool", sizeof(SecuritySession_t));
    SecurityLevelPool = le_mem_CreatePool("SecurityLevelPool", sizeof(SecurityLevel_t));

    AO_timer_mapping_table = le_hashmap_Create(
                                "AO-Timer-Table",
                                MAX_AO_TIMER_TABLE_SIZE,
                                le_hashmap_HashVoidPointer,
                                le_hashmap_EqualsVoidPointer);
    LE_ASSERT(AO_timer_mapping_table != NULL);

    /* Be used in UDS Manager Thread */
    le_cfg_ConnectService();

    LE_INFO("[%s] Done", __FUNCTION__);
}

void SecurityAccess_CreateActiveObject(void * mgr_, void * ifname)
{
    UdsCommunicationMgr * mgr = (UdsCommunicationMgr *)mgr_;

    /* Create new Active Object and bind it to UDS manager */
    mgr->mSecurityAccess = (AO_SecurityAccess_t *) le_mem_ForceAlloc(SecurityActiveObjectPool);
    SECACC_ASSERT_FATAL(mgr->mSecurityAccess != NULL);

    LE_DEBUG("AO object address: %p (in)", mgr->mSecurityAccess);
    AO_SecurityAccess_ctor(mgr->mSecurityAccess, mgr, (char *)ifname);
    LE_DEBUG("AO object address: %p (out)", mgr->mSecurityAccess);

    TryToCreateStorageFromTree(mgr->mSecurityAccess);

    /* Create separated Delay Timer for each Active Object */
    char timerName[DELAY_TIMER_NAME_SIZE];
    snprintf(timerName, sizeof(timerName), "delay_timer_%s", (char *)ifname);
    mgr->mSecurityAccess->delay_timer_ref = le_timer_Create(timerName);
    SECACC_ASSERT_FATAL(mgr->mSecurityAccess->delay_timer_ref != NULL);

    LE_DEBUG("%s .. AO: %p (%s)", __FUNCTION__, mgr->mSecurityAccess, (char *)ifname);
    LE_DEBUG("%s .. delay_timer_ref: %p",
            __FUNCTION__,
            mgr->mSecurityAccess->delay_timer_ref);

    LE_ASSERT(le_hashmap_Size(AO_timer_mapping_table) <= MAX_AO_TIMER_TABLE_SIZE);
    le_hashmap_Put(AO_timer_mapping_table,
                   mgr->mSecurityAccess->delay_timer_ref,
                   mgr->mSecurityAccess);

    le_timer_SetRepeat(mgr->mSecurityAccess->delay_timer_ref, 1);
    le_timer_SetHandler(mgr->mSecurityAccess->delay_timer_ref, SecAcc_DelayTimerHandler);

    /* Trigger the initial stage */
    MFsm_init((MFsm_t *)mgr->mSecurityAccess, (MEvent_t*)0);

    LE_INFO("[%s] -> if-name: %s /AO created", __FUNCTION__, (char *)ifname);
}

void SecurityAccess_StartWorker(void * u, void *p)
{
    le_sem_Ref_t semRef = le_sem_Create("mReady", 0);

    le_thread_Ref_t udsSecurityAccessWorkerRef = le_thread_Create("udsSecAccTT",
                                                                  SecurityAccessWorker,
                                                                  (void*)semRef);
    le_thread_Start(udsSecurityAccessWorkerRef);

    le_sem_Wait(semRef); /* waiting for post-action */

    LE_INFO("[%s] Done", __FUNCTION__);
}

#ifdef __cplusplus
}
#endif

} /* uds */
} /* taf */
