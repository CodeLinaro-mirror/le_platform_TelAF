/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "interfaces.h"

#define NODE_ID 0
#define VEHICHLE_WAKEUP_REASON_DEFAULT 0

const char* wakeuptype = "";
static le_sem_Ref_t semRef = NULL, queueSemRef = NULL;
static le_thread_Ref_t threadRef = NULL;
taf_pm_StateChangeHandlerRef_t handlerRef;
taf_pm_StateChangeExHandlerRef_t handlerExRef;
le_clk_Time_t Timeout = { 3 , 0 };
int status = EXIT_SUCCESS;
const char* vHalTag = "vehichle_on";
taf_mngdPm_wsRef_t wsRef = NULL;
taf_mngdPm_NodePowerStateChangeBitMask_t stateMask = 0;

static void PrintUsage
(
    void
)
{
    puts(
        "\n"
        "NAME:\n"
        "    tafMngdPMUnitTest - Used to perform Manged PM Service testcases.\n"
        "\n"
        "PREREQUISITES:\n"
        "    tafMngdPMSvc is running.\n"
        "    tafMngdPMSvc can be started using \"app start tafMngdPMSvc.\"\n"
        "\n"
        "NOTE:\n"
        " Need to run tafMngdPMUnitTest manually to perform Modem filter testcases.\n"
        " app start tafMngdPMUnitTest - Will perform Modem filter testcases.\n"
        "\n"
        "DESCRIPTION:\n"
        "    To run the GracefulSystemShutdown :\n"
        "    app runProc tafMngdPMUnitTest tafMngdPMUnitTest -- GracefulSystemShutdown \n"
        "\n"
        "    To run the ForcedSystemShutdown :\n"
        "    app runProc tafMngdPMUnitTest tafMngdPMUnitTest -- ForcedSystemShutdown \n"
        "\n"
        "    To run the RestartSystem :\n"
        "    app runProc tafMngdPMUnitTest tafMngdPMUnitTest -- RestartSystem \n"
        "\n"
        "    To run the ShutdownNode :\n"
        "    app runProc tafMngdPMUnitTest tafMngdPMUnitTest -- ShutdownNode <NODE_ID>\n"
        "\n"
        "    To run the RestartNode :\n"
        "    app runProc tafMngdPMUnitTest tafMngdPMUnitTest -- RestartNode <NODE_ID>\n"
        "\n"
        "    For usage:\n"
        "    app runProc tafMngdPMUnitTest tafMngdPMUnitTest -- help\n"
        );

    exit(EXIT_SUCCESS);
}

//Function to convert taf state to string
char* tafStateToString(taf_pm_State_t tafState)
{
    char* state;
    switch(tafState) {
        case TAF_PM_STATE_RESUME:
            state = "Resume";
            break;
        case TAF_PM_STATE_SUSPEND:
            state = "Suspend";
            break;
        case  TAF_PM_STATE_SHUTDOWN:
            state = "Shutdown";
            break;
        default :
            state = "Unknown";
            break;
    }
    return state;
}

void TestStateChangeHandler(taf_pm_State_t state, void* contextPtr)
{
    LE_INFO("State change triggered for %s\n", tafStateToString(state));
    printf("\nState change triggered for %s\n", tafStateToString(state));
}

//Function called on power state change
void TestStateChangeExHandler(taf_pm_PowerStateRef_t powerStateRef,
        taf_pm_NadVm_t vm_id, taf_pm_State_t state, void* contextPtr)
{
    LE_TEST_INFO("State change triggered for %s\n", tafStateToString(state));
    printf("\nState change triggered for %s\n", tafStateToString(state));
    taf_pm_SendStateChangeAck(powerStateRef, state, TAF_PM_PVM, TAF_PM_READY);
    LE_INFO("Sent state change acknowledge for %s\n", tafStateToString(state));
    printf("\n Sent state change acknowledge for %s\n", tafStateToString(state));
    if(state == TAF_PM_STATE_SHUTDOWN)
    {
        LE_INFO("Sent state change NACK for %s\n", tafStateToString(state));
        printf("\n Sent state change NACK for %s\n", tafStateToString(state));
    taf_pm_SendStateChangeAck(powerStateRef, state, TAF_PM_PVM, TAF_PM_NOT_READY);
    }
}

static void* test_stateChangeHandler(void* ctxPtr)
{
    taf_pm_ConnectService();
    taf_mngdPm_ConnectService();

    LE_TEST_INFO("Testing taf_pm_AddStateChangeHandler on valid handler reference");
    handlerRef = taf_pm_AddStateChangeHandler(TestStateChangeHandler, NULL);
    LE_TEST_OK(handlerRef != NULL,"Register state change handler is successfull");

    LE_TEST_INFO("Testing taf_pm_AddStateChangeExHandler on valid handler reference");
    handlerExRef = taf_pm_AddStateChangeExHandler(TestStateChangeExHandler, NULL);
    LE_TEST_OK(handlerExRef != NULL,"Register state change handler is successfull");

    le_sem_Post(semRef);
    le_event_RunLoop();
}

le_result_t SuspendSystem(const char* wakeuptype)
{
    le_result_t res = LE_FAULT;
    uint8_t pmNodeId = 0;
    if(strcmp(wakeuptype, "1") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
        if(wsRef == NULL)
        wsRef = taf_mngdPm_NewNodeWakeupSource(pmNodeId, TAF_MNGDPM_SMS, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for SMS");
            res = taf_mngdPm_RelaxNode(wsRef);
            if(res == LE_OK)
                LE_INFO("suspended system with wakeuptype SMS");
        }
    }
    else if(strcmp(wakeuptype, "2") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is VOICE_CALL");
        if(wsRef == NULL)
        wsRef = taf_mngdPm_NewNodeWakeupSource(pmNodeId, TAF_MNGDPM_VOICE_CALL, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for VOICE_CALL");
            res = taf_mngdPm_RelaxNode(wsRef);
            if(res == LE_OK)
                LE_INFO("suspended sysytem with wakeuptype VOICE_CALL");
        }
    }
    else if(strcmp(wakeuptype, "3") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is MCU_VHAL");
        if(wsRef == NULL)
        wsRef = taf_mngdPm_NewNodeWakeupSource(pmNodeId, TAF_MNGDPM_MCU_VHAL, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for MCU_VHAL");
            res = taf_mngdPm_RelaxNode(wsRef);
            if(res == LE_OK)
                LE_INFO("suspended sysytem with wakeuptype MCU_VHAL");
        }
    }
    else {
        LE_ERROR("SuspendSystem failed due to unsupoorted wakeupSource type");
        return res;
    }
    return res;
}

le_result_t ResumeSystem(const char* wakeuptype)
{
    le_result_t res = LE_FAULT;
    if(strcmp(wakeuptype, "1") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
        if(wsRef == NULL)
        wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_SMS, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for SMS");
            res = taf_mngdPm_StayAwakeNode(wsRef);
            if(res == LE_OK) {
                LE_INFO("Resumed sysytem with wakeuptype SMS");
             }
        }
    }
    else if(strcmp(wakeuptype, "2") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is VOICE_CALL");
        if(wsRef == NULL)
        wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_VOICE_CALL, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for VOICE_CALL");
            res = taf_mngdPm_StayAwakeNode(wsRef);
            if(res == LE_OK) {
                LE_INFO("Resumed sysytem with wakeuptype VOICE_CALL");
             }
        }
    }
    else if(strcmp(wakeuptype, "3") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
        if(wsRef == NULL)
        wsRef = taf_mngdPm_NewNodeWakeupSource(NODE_ID, TAF_MNGDPM_MCU_VHAL, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for MCU_VHAL");
            res = taf_mngdPm_StayAwakeNode(wsRef);
            if(res == LE_OK) {
                LE_INFO("Resumed sysytem with wakeuptype MCU_VHAL");
             }
        }
    }
    else {
        LE_ERROR("ResumeSystem failed");
        return res;
   }
   return res;
}

void NodePowerStateChangeHandlerCB(
     uint8_t pmNodeId,
     taf_mngdPm_nodePowerStateRef_t nodePowerStateRef,
     taf_mngdPm_NodePowerState_t state,
	 void *contextPtr)
{
    LE_INFO("NodePowerStateChangeHandlerFunc callback");
    le_result_t res = LE_FAULT;
    res = taf_mngdPm_SendNodePowerStateChangeAck(pmNodeId, nodePowerStateRef, TAF_MNGDPM_CLIENT_READY);
    if(res == LE_OK)
    {
        LE_INFO("SendNodePowerStateChangeAck is success");
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}

void AddNodePowerStateChangeHandler
(
    const char* NodePowerStateChangeBitMask,
    uint8_t pmNodeId
)
{
    LE_INFO("taf_mngdPm_AddNodePowerStateChangeHandler");
    if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE") == 0)
    {
        stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE");
        }
    }
    else if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE") == 0)
    {
        stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE");
        }
    }
    else if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE") == 0)
    {
        stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_SUSPEND_PREPARE");
        }
    }
    else if(strcmp(NodePowerStateChangeBitMask, "TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME") == 0)
    {
        stateMask = TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME;
        taf_mngdPm_NodePowerStateChangeHandlerRef_t ref = NULL;
        ref = taf_mngdPm_AddNodePowerStateChangeHandler(NodePowerStateChangeHandlerCB, NULL, pmNodeId, stateMask);
        if(ref)
        {
            LE_INFO("AddNodePowerStateChangeHandler is success for TAF_MNGDPM_NODE_STATE_BIT_MASK_RESUME");
        }
     }
}

static int GracefulSystemShutdown()
{
    LE_TEST_INFO("To test GracefulSystemShutdown!");
    le_result_t result =  LE_FAULT;

    result = taf_mngdPm_SetNodeTargetedPowerMode(NODE_ID, TAF_MNGDPM_SHUTDOWN);
    if(result != LE_OK)
    {
        LE_ERROR("GracefulSysShutdown request failed");
        return EXIT_FAILURE;
    }
    LE_TEST_OK(result==LE_OK, "Test GracefulSystemShutdown! - Pass");

    return EXIT_SUCCESS;
}

void RestartCallback(taf_mngdPm_RestartMode_t mode, taf_mngdPm_ResponseMode_t rspmode ,
        le_result_t result, void* contextPtr)
{
    LE_INFO("RestartCallback response mode is %d", rspmode);
    exit(status);
}

static int RestartSystem()
{
    LE_TEST_INFO("To test RestartSystem!" );
    uint8_t pmNodeId = 0;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_RESTART_PREPARE", pmNodeId);
    le_result_t res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON,
            RestartCallback, NULL, TAF_MNGDPM_RESTART_REASON_NORMAL);

    if(res == LE_OK)
    {
        LE_ERROR("RestartSystem request failed");
        return EXIT_FAILURE;
    }
    LE_TEST_OK(res==LE_OK, "Test RestartSystem! - Pass");
    return EXIT_SUCCESS;
}

void ForcedSystemShutdownCallBack(taf_mngdPm_ShutdownMode_t mode,
    taf_mngdPm_ResponseMode_t ResponseMode, le_result_t result, void* contextPtr)
{
    LE_INFO("ForcedSystemShutdownCallBack response mode is %d", ResponseMode);
    if(ResponseMode == 0)
    {
        LE_INFO("----ForcedSystemShutdown success----");
    }
    else{
        exit(EXIT_FAILURE);
    }
}

void ForcedSystemShutdown()
{
    LE_TEST_INFO("To test ForcedSystemShutdown!");
    le_result_t result;
    uint8_t pmNodeId = 0;
    AddNodePowerStateChangeHandler("TAF_MNGDPM_NODE_STATE_BIT_MASK_SHUTDOWN_PREPARE", pmNodeId);
    result = taf_mngdPm_ShutdownReqAsync(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
            ForcedSystemShutdownCallBack, NULL, TAF_MNGDPM_SHUTDOWN_REASON_NORMAL);
    if(result != LE_OK)
    {
        LE_ERROR("ForcedSystemShutdown request failed");
        exit(EXIT_FAILURE);
    }
    LE_TEST_OK(result==LE_OK, "Test ForcedSystemShutdown! - Pass");
}

static int ShutdownNode()
{
    LE_TEST_INFO("To test ShutdownNode!");
    le_result_t result;
    result = taf_mngdPm_ShutdownNode(NODE_ID);
    if(result != LE_OK)
    {

        LE_ERROR("ShutdownNode request failed");
        return EXIT_FAILURE;
    }
    LE_TEST_OK(result==LE_OK, "Test ShutdownNode! - Pass");
    return EXIT_SUCCESS;
}

static int RestartNode()
{
    LE_TEST_INFO("To test RestartNode!");
    le_result_t result;

    result = taf_mngdPm_RestartNode(NODE_ID);
    if(result != LE_OK)
    {
        LE_ERROR("RestartNode request failed");
        return EXIT_FAILURE;
    }
    LE_TEST_OK(result==LE_OK, "Test RestartNode! - Pass");
    return EXIT_SUCCESS;
}

void WakeupVehicleback(int32_t reason, int32_t rspmode ,
        le_result_t result, void* contextPtr)
{
    LE_INFO("WakeupVehicleback response is %d", rspmode);
    exit(status);
}

static int WakeupVehicle()
{
    LE_INFO("WakeupVehicle");
    le_result_t res = taf_mngdPm_WakeupVehicleReqAsync(VEHICHLE_WAKEUP_REASON_DEFAULT,
            WakeupVehicleback, NULL);

    if(res == LE_OK)
    {
        LE_INFO("----WakeupVehicle success----");
        status = EXIT_SUCCESS;
    }
    else
    {
        LE_ERROR("WakeupVehicle request failed");
        status = EXIT_FAILURE;
    }
    return status;
}
static int TestMngdPMUnitTest()
{
    LE_INFO("TestMngdPMUnitTest start");
    if(threadRef == NULL)
    {
        threadRef = le_thread_Create("state_trigger_thread",
                                    test_stateChangeHandler, NULL);
        le_thread_Start(threadRef);
        le_sem_Wait(semRef);
    }

    le_sem_WaitWithTimeOut(semRef, Timeout);

    LE_INFO("====All tests are passed=====");
    return EXIT_SUCCESS;
}

COMPONENT_INIT
{
    int NumberOfArgs = le_arg_NumArgs();
    LE_INFO("NumberOfArgs is %d", NumberOfArgs);
    const char* arg;
    arg = le_arg_GetArg(0);
    semRef = le_sem_Create("MngdPMSem", 0);
    queueSemRef = le_sem_Create("MngdPMQueueSem", 0);
    if(le_arg_NumArgs() == 0)
    {
        status = TestMngdPMUnitTest();
        exit(status);
    }
    else if (arg!= NULL && strncmp(arg, "help", 4) == 0)
    {
        PrintUsage();
    }
    else if (arg!= NULL && strncmp(arg, "GracefulSystemShutdown", 22) == 0)
    {
        status = GracefulSystemShutdown();
        exit(status);
    }
    else if (arg!= NULL && strncmp(arg, "ForcedSystemShutdown", 20) == 0)
    {
        ForcedSystemShutdown();
    }
    else if (arg!= NULL && strncmp(arg, "RestartSystem", 13) == 0)
    {
        status = RestartSystem();
        exit(status);
    }
    else if (arg!= NULL && strncmp(arg, "ShutdownNode", 12) == 0)
    {
        status = ShutdownNode();
        exit(status);
    }
    else if (arg!= NULL && strncmp(arg, "RestartNode", 11) == 0)
    {
        status = RestartNode();
        exit(status);
    }
    else if (arg!= NULL && strncmp(arg, "WakeupVehicle", 13) == 0)
    {
        status = WakeupVehicle();
        exit(status);
    }
}