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

#include "legato.h"
#include "interfaces.h"

#define NODE_ID 0
#define VEHICHLE_WAKEUP_REASON_DEFAULT 0

const char* wakeuptype = "";
static le_sem_Ref_t semRef = NULL, queueSemRef = NULL;
static le_thread_Ref_t threadRef = NULL;
taf_pm_StateChangeHandlerRef_t handlerRef;
taf_pm_StateChangeExHandlerRef_t handlerExRef;
taf_mngdPm_StateChangeHandlerRef_t mpmsHanlerRef;
le_clk_Time_t Timeout = { 3 , 0 };
int status = EXIT_SUCCESS;
const char* vHalTag = "vehichle_on";

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

static char * StateToString(taf_mngdPm_State_t *state)
{
    switch (*state)
    {
        case TAF_MNGDPM_STATE_RESUME:
            return "RESUME";
        case TAF_MNGDPM_STATE_SUSPEND:
            return "SUSPEND";
        case TAF_MNGDPM_STATE_SHUTDOWN:
            return "SHUTDOWN";
        default:
            LE_ERROR("unknown state");
            return "UNKNOWN";
    }
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

// function called on MPMS power state change
void TestMPMSStateChangeHandler(taf_mngdPm_StateInd_t* indication, void* contextPtr)
{
    taf_mngdPm_State_t state;
    switch(indication->state)
    {
        case TAF_MNGDPM_STATE_RESUME:
            state = TAF_MNGDPM_STATE_RESUME;
            LE_TEST_INFO("MPMS state change to %s\n", StateToString(&state));
            printf("\nMPMS state change to %s\n", "TAF_MNGDPM_STATE_RESUME");
            break;

        case TAF_MNGDPM_STATE_SUSPEND:
            LE_TEST_INFO("MPMS state change to %s\n", "TAF_MNGDPM_STATE_SUSPEND");
            printf("\nMPMS state change to %s\n", "TAF_MNGDPM_STATE_SUSPEND");
            break;

        case TAF_MNGDPM_STATE_SHUTDOWN:
            LE_TEST_INFO("MPMS state change to %s\n", "TAF_MNGDPM_STATE_SHUTDOWN");
            printf("\nMPMS state change to %s\n", "TAF_MNGDPM_STATE_SHUTDOWN");
            break;

        case TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE:
            LE_TEST_INFO("MPMS state change to %s\n", "TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE");
            printf("\nMPMS state change to %s\n", "TAF_MNGDPM_STATE_RELEASING_WAKE_SOURCE");
            break;

        case TAF_MNGDPM_STATE_SUSPENDING:
            LE_TEST_INFO("MPMS state change to %s\n", "TAF_MNGDPM_STATE_SUSPENDING");
            printf("\nMPMS state change to %s\n", "TAF_MNGDPM_STATE_SUSPENDING");
            break;

        case TAF_MNGDPM_STATE_SHUTTING_DOWN:
            LE_TEST_INFO("MPMS state change to %s\n", "TAF_MNGDPM_STATE_SHUTTING_DOWN");
            printf("\nMPMS state change to %s\n", "TAF_MNGDPM_STATE_SHUTTING_DOWN");
            break;

        case TAF_MNGDPM_STATE_WAKING_UP:
            LE_TEST_INFO("MPMS state change to %s\n", "TAF_MNGDPM_STATE_WAKING_UP");
            printf("\nMPMS state change to %s\n", "TAF_MNGDPM_STATE_WAKING_UP");
            break;

        default:
            break;
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

    LE_TEST_INFO("Testing taf_mngdPm_AddStateChangeHandler on valid handler reference");
    mpmsHanlerRef = taf_mngdPm_AddStateChangeHandler(
                    (taf_mngdPm_StateChangeHandlerFunc_t)TestMPMSStateChangeHandler,
                    NULL);
    LE_TEST_OK(handlerExRef != NULL,"Register MPMS state change handler is successfull");

    le_sem_Post(semRef);
    le_event_RunLoop();
}

le_result_t SetModemWakeupSource(const char* wakeupSource)
{

    // Convert string to uint32_t
    uint32_t uintResult = (uint32_t)strtoul(wakeupSource, NULL, 10);
    // Check for conversion errors
    if (uintResult > UINT32_MAX) {
        fprintf(stderr, "Value out of range.\n");
        exit(EXIT_FAILURE);
    }
    // Print the result
    printf("String: %s\nConverted to uint32_t: %u\n", wakeupSource, uintResult);

    le_result_t res = LE_FAULT;
    res = taf_mngdPm_SetModemWakeupSource(uintResult);
    if(res == LE_OK) {
        return res;
    }
    else
    {
        LE_ERROR("SetModemWakeupSource request failed");
        return res;
    }
}

le_result_t SuspendSystem(const char* wakeuptype)
{
    le_result_t res = LE_FAULT;
    taf_mngdPm_wsRef_t wsRef = NULL;
    uint8_t pmNodeId = 0;
    if(strcmp(wakeuptype, "1") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
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
        wsRef = taf_mngdPm_NewNodeWakeupSource(pmNodeId, TAF_MNGDPM_VOICE_CALL, vHalTag);
        if(wsRef != NULL) {
            LE_INFO("NewNodeWakeupSource ref is created for VOICE_CALL");
            res = taf_mngdPm_RelaxNode(wsRef);
            if(res == LE_OK)
                LE_INFO("suspended sysytem with wakeuptype VOICE_CALL");
        }
    }
    else if(strcmp(wakeuptype, "3") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
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
    taf_mngdPm_wsRef_t wsRef = NULL;
    if(strcmp(wakeuptype, "1") == 0) {
        LE_INFO("NewNodeWakeupSource wakeuptype is SMS");
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

static le_result_t TestSMSWakeupType()
{
    le_result_t res = LE_FAULT;
    LE_INFO("TestSMSWakeupType");
    LE_TEST_INFO("To test SMS as newnode wakeuptype!");
    res = SetModemWakeupSource("1");
    LE_TEST_OK(res==LE_OK, "Test SMS as newnode wakeuptype! - Pass");
    if(res != LE_OK)
        return res;

    LE_TEST_INFO("To test ResumeSystem when SMS as wakeuptype!");
    res = ResumeSystem("1");
    LE_TEST_OK(res==LE_OK, "Test ResumeSystem when SMS as wakeuptype! - Pass");
    if(res != LE_OK)
        return res;
    le_sem_WaitWithTimeOut(semRef, Timeout);

    LE_TEST_INFO("To test SuspendSystem when SMS as wakeuptype!");
    res = SuspendSystem("1");
    LE_TEST_OK(res==LE_OK, "Test SuspendSystem when SMS as wakeuptype! - Pass");
    return res;
}

static le_result_t TestVoiceCallWakeupType()
{
    le_result_t res = LE_FAULT;
    LE_INFO("TestVoiceCallWakeupType");
    LE_TEST_INFO("To test VOICE_CALL as newnode wakeuptype!");
    res = SetModemWakeupSource("2");
    LE_TEST_OK(res==LE_OK, "Test VOICE_CALL as newnode wakeuptype! - Pass");
    if(res != LE_OK)
        return res;

    LE_TEST_INFO("To test ResumeSystem when VOICE_CALL as wakeuptype!");
    res = ResumeSystem("2");
    LE_TEST_OK(res==LE_OK, "Test ResumeSystem when VOICE_CALL as wakeuptype! - Pass");
    if(res != LE_OK)
        return res;
    le_sem_WaitWithTimeOut(semRef, Timeout);

    LE_TEST_INFO("To test SuspendSystem when VOICE_CALL as wakeuptype!");
    res = SuspendSystem("2");
    LE_TEST_OK(res==LE_OK, "Test SuspendSystem when VOICE_CALL as wakeuptype! - Pass");
    return res;
}

static le_result_t TestMcuVhalWakeupType()
{
    le_result_t res = LE_FAULT;
    LE_INFO("TestMcuVhalWakeupType");
    LE_TEST_INFO("To test MCU_VHAL as newnode wakeuptype!");
    res = SetModemWakeupSource("4");
    LE_TEST_OK(res==LE_OK, "Test MCU_VHAL as newnode wakeuptype! - Pass");
    if(res != LE_OK)
        return res;

    LE_TEST_INFO("To test ResumeSystem when MCU_VHAL as wakeuptype!");
    res = ResumeSystem("3");
    LE_TEST_OK(res==LE_OK, "Test ResumeSystem when MCU_VHAL as wakeuptype! - Pass");
    if(res != LE_OK)
        return res;
    le_sem_WaitWithTimeOut(semRef, Timeout);

    LE_TEST_INFO("To test SuspendSystem when MCU_VHAL as wakeuptype!");
    res = SuspendSystem("3");
    LE_TEST_OK(res==LE_OK, "Test SuspendSystem when MCU_VHAL as wakeuptype! - Pass");
    return res;
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
        void* contextPtr)
{
    LE_INFO("RestartCallback response mode is %d", rspmode);
    exit(status);
}

static int RestartSystem()
{
    LE_TEST_INFO("To test RestartSystem!" );

    le_result_t res = taf_mngdPm_RestartReqAsync(TAF_MNGDPM_RESTART_SYSTEM_OFF_ON,
            RestartCallback, NULL);

    if(res == LE_OK)
    {
        LE_ERROR("RestartSystem request failed");
        return EXIT_FAILURE;
    }
    LE_TEST_OK(res==LE_OK, "Test RestartSystem! - Pass");
    return EXIT_SUCCESS;
}

void ForcedSystemShutdownCallBack(taf_mngdPm_ShutdownMode_t mode,
    taf_mngdPm_ResponseMode_t ResponseMode, void* contextPtr)
{
    LE_INFO("ForcedSystemShutdownCallBack response mode is %d", ResponseMode);
    exit(status);
}

static int ForcedSystemShutdown()
{
    LE_TEST_INFO("To test ForcedSystemShutdown!");
    le_result_t result;

    result = taf_mngdPm_ShutdownReqAsync(TAF_MNGDPM_SHUTDOWN_MODE_NORMAL,
            ForcedSystemShutdownCallBack, NULL);
    if(result != LE_OK)
    {
        LE_ERROR("ForcedSystemShutdown request failed");
        return EXIT_FAILURE;
    }
    LE_TEST_OK(result==LE_OK, "Test ForcedSystemShutdown! - Pass");
    return EXIT_SUCCESS;
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
        void* contextPtr)
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
    le_result_t res;
    if(threadRef == NULL)
    {
        threadRef = le_thread_Create("state_trigger_thread",
                                    test_stateChangeHandler, NULL);
        le_thread_Start(threadRef);
        le_sem_Wait(semRef);
    }
    res = TestSMSWakeupType();
    if(res != LE_OK)
    {
        LE_ERROR("TestSMSWakeupType failed");
        return EXIT_FAILURE;
    }
    le_sem_WaitWithTimeOut(semRef, Timeout);
    res = TestVoiceCallWakeupType();
    if(res != LE_OK)
    {
        LE_ERROR("TestVoiceCallWakeupType failed");
        return EXIT_FAILURE;
    }
    le_sem_WaitWithTimeOut(semRef, Timeout);
    res = TestMcuVhalWakeupType();
    if(res != LE_OK)
    {
        LE_ERROR("TestMcuVhalWakeupType failed");
        return EXIT_FAILURE;
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
        status = ForcedSystemShutdown();
        exit(status);
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