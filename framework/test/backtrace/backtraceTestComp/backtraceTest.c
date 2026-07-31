/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/* =========================================================================
 * Configuration
 * ========================================================================= */

/* Number of innocent background worker threads running during the crash.
 * Case 3 (stack overflow) runs with 0 workers — the overflow fires on the
 * thread stack and the signal handler runs on the altstack. Worker threads
 * calling malloc() concurrently would deadlock the handler because
 * SA_ONSTACK handlers cannot safely call any function that acquires the
 * malloc lock. Cases 1, 2, 4 use the full worker pool to stress-test
 * async-signal-safety under lock contention. */
#define N_WORKER_THREADS   8

/* Seconds to let workers run before triggering the crash. */
#define WARMUP_SECS        3

/* =========================================================================
 * Global state
 * ========================================================================= */

/* Prevents compiler from optimising away crash sites. */
static volatile uintptr_t      g_zero       = 0;
static volatile void          (*g_nullFn)(void) = NULL;

/*
 * Case 4 — simulates a service callback pointer that is valid during normal
 * operation but is NULL (cleared during teardown) when the crash thread
 * calls it, producing PC=0 exactly as seen in tafRadioSvc.
 */
static volatile void          (*g_crashCallback)(void) = NULL;

/* Shared mutex/condvar used by worker threads — held during crash to test
 * that the backtrace handler does not deadlock on a held mutex. */
static pthread_mutex_t  g_workMutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   g_workCond   = PTHREAD_COND_INITIALIZER;
static volatile int     g_workCount  = 0;

/* Set to 1 to ask worker threads to exit cleanly. */
static volatile int     g_shutdown   = 0;

/* =========================================================================
 * Worker thread — runs concurrently with the crash
 *
 * Does real work: acquires mutex, signals condvar, allocates/frees memory,
 * calls a moderately deep call chain. This ensures the crash handler is
 * tested while locks are held by other threads.
 * ========================================================================= */

static __attribute__((noinline)) int workerLevel3(int v)
{
    /* Allocate and free to exercise the heap allocator lock. */
    void *p = malloc(64);
    if (p) { memset(p, v & 0xFF, 64); free(p); }
    return v + 1;
}

static __attribute__((noinline)) int workerLevel2(int v)
{
    return workerLevel3(v + 1);
}

static __attribute__((noinline)) int workerLevel1(int v)
{
    return workerLevel2(v + 1);
}

static void *workerThread(void *arg)
{
    int id = (int)(intptr_t)arg;
    int iter = 0;

    while (!g_shutdown)
    {
        /* Hold the mutex briefly — tests that crash handler does not
         * deadlock when another thread holds this lock. */
        pthread_mutex_lock(&g_workMutex);
        g_workCount++;
        iter = workerLevel1(iter);
        pthread_cond_signal(&g_workCond);
        pthread_mutex_unlock(&g_workMutex);

        /* Sleep a random short interval so threads are not all in sync. */
        usleep((useconds_t)(10000 + (id * 3000) % 50000));
    }
    return NULL;
}

/* =========================================================================
 * Crash scenario functions
 * All are __attribute__((noinline)) so they appear as distinct frames in
 * the FP chain and stack scan output.
 * ========================================================================= */

/* --- Scenario 1: Null pointer read --- */
static __attribute__((noinline)) void crash1_inner(void)
{
    volatile int *p = (volatile int *)(uintptr_t)g_zero;
    (void)(*p);  /* SIGSEGV: read from address 0 */
}
static __attribute__((noinline)) void crash1_mid(void) { crash1_inner(); }
static __attribute__((noinline)) void crash1_outer(void) { crash1_mid(); }

static void *crashThread1(void *arg)
{
    (void)arg;
    LE_INFO("[crash1] null pointer read — thread TID=%ld",
            (long)syscall(SYS_gettid));
    sleep(WARMUP_SECS);
    crash1_outer();
    return NULL;
}

/* --- Scenario 2: Null function pointer call (PC=0) --- */
static __attribute__((noinline)) void crash2_inner(void)
{
    g_nullFn();  /* PC=0 at crash */
}
static __attribute__((noinline)) void crash2_mid(void) { crash2_inner(); }
static __attribute__((noinline)) void crash2_outer(void) { crash2_mid(); }

static void *crashThread2(void *arg)
{
    (void)arg;
    LE_INFO("[crash2] null function pointer call (PC=0) — thread TID=%ld",
            (long)syscall(SYS_gettid));
    sleep(WARMUP_SECS);
    crash2_outer();
    return NULL;
}

/* --- Scenario 3: Stack overflow --- */
static __attribute__((noinline)) void crash3_recurse(int depth)
{
    volatile char buf[256];   /* consume stack quickly */
    buf[0] = (char)depth;
    if (depth > 0) { crash3_recurse(depth - 1); }
    (void)buf[0];
}

/* Per-thread alternate signal stack for crashThread3.
 * sigaltstack() is per-thread. The main thread's altstack (installed by
 * le_sig_InstallShowStackHandler) is not inherited by new threads.
 * crashThread3 must install its own so SA_ONSTACK delivery works when
 * the stack overflows. Statically allocated so it is always valid. */
static uint8_t crash3AltStack[65536U];

static void *crashThread3(void *arg)
{
    (void)arg;
    LE_INFO("[crash3] stack overflow — thread TID=%ld",
            (long)syscall(SYS_gettid));

    /* Install altstack on THIS thread before overflowing. */
    {
        stack_t altStack;
        altStack.ss_sp    = crash3AltStack;
        altStack.ss_size  = sizeof(crash3AltStack);
        altStack.ss_flags = 0;
        if (sigaltstack(&altStack, NULL) != 0)
        {
            LE_WARN("[crash3] sigaltstack failed: %m");
        }
    }

    sleep(WARMUP_SECS);
    crash3_recurse(100000);
    return NULL;
}

/* --- Scenario 4: Crash DURING Legato teardown (the real tafRadioSvc case) ---
 *
 * Sequence:
 *   1. App starts, worker threads run, crashThread4 sleeps WARMUP_SECS.
 *   2. Test script sends 'app stop' — Legato's TermSignalHandler calls
 *      exit(), which runs the framework's CrashWaitAtExit() atexit handler
 *      (registered early by le_sig_InstallShowStackHandler, runs last).
 *   3. CrashWaitAtExit sleeps 10ms then checks g_crashInProgress.
 *   4. Concurrently, crashThread4 wakes and immediately calls the NULL
 *      callback — SIGSEGV fires, ShowStackSignalHandler sets
 *      g_crashInProgress=1 and writes the full backtrace.
 *   5. CrashWaitAtExit sees the flag and sleeps 2s, holding _exit() at bay.
 *   6. Crash handler's tgkill re-raise terminates the process with a core dump.
 *
 * No app-level atexit handler is registered — this is the real user case.
 * The framework's CrashWaitAtExit() handles the wait transparently.
 */
static __attribute__((noinline)) void crash4_invokeCallback(void)
{
    /* g_crashCallback is always NULL — simulates a service callback cleared
     * during teardown, producing PC=0 as seen in tafRadioSvc. */
    if (g_crashCallback != NULL)
        g_crashCallback();
    else
        g_nullFn();  /* force PC=0 */
}
static __attribute__((noinline)) void crash4_doWork(void)
{
    crash4_invokeCallback();
}
static __attribute__((noinline)) void crash4_dispatch(void)
{
    crash4_doWork();
}

static void *crashThread4(void *arg)
{
    (void)arg;
    LE_INFO("[crash4] running (TID=%ld) — will crash after %ds",
            (long)syscall(SYS_gettid), WARMUP_SECS);
    LE_INFO("[crash4] send 'app stop backtraceTest' now");

    /* Sleep the same duration the test script waits before sending app stop.
     * The crash therefore races with exit() teardown — the real scenario. */
    sleep(WARMUP_SECS);

    LE_INFO("[crash4] crashing now (PC=0 expected)");
    crash4_dispatch();
    return NULL;
}

/* =========================================================================
 * Thread launch helper
 * ========================================================================= */
static pthread_t launchThread(void *(*fn)(void *), void *arg, const char *name)
{
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int rc = pthread_create(&tid, &attr, fn, arg);
    if (rc != 0)
        LE_ERROR("Failed to create thread %s: %d", name, rc);
    pthread_attr_destroy(&attr);
    return tid;
}

/* =========================================================================
 * COMPONENT_INIT
 * ========================================================================= */
COMPONENT_INIT
{
    int i;
    const char *tcStr = getenv("TEST_CASE");
    int tc = tcStr ? atoi(tcStr) : 0;

    /* Seed random for default random-case selection. */
    srand((unsigned int)time(NULL));

    /* Resolve tc==0 FIRST so the worker-launch decision below is correct. */
    if (tc == 0)
    {
        tc = 1 + (rand() % 3);
        LE_INFO("Default: randomly selected crash type %d", tc);
    }

    LE_INFO("==================================================================");
    LE_INFO("Backtrace multi-thread stress test — TEST_CASE=%d", tc);
    LE_INFO("  1 = null pointer read");
    LE_INFO("  2 = null function pointer (PC=0)");
    LE_INFO("  3 = stack overflow");
    LE_INFO("  4 = crash during app stop [KEY SCENARIO — send app stop]");
    LE_INFO("==================================================================");

    /* --- Start background worker threads (not for Case 3) ---
     * Case 3 (stack overflow) must run without any worker threads.
     * See N_WORKER_THREADS comment above for the full explanation. */
    if (tc != 3)
    {
        LE_INFO("Starting %d background worker threads...", N_WORKER_THREADS);
        for (i = 0; i < N_WORKER_THREADS; i++)
        {
            char name[32];
            snprintf(name, sizeof(name), "worker%d", i);
            launchThread(workerThread, (void *)(intptr_t)i, name);
        }
        LE_INFO("All worker threads started. Warming up for %d seconds...",
                WARMUP_SECS);
    }
    else
    {
        LE_INFO("Case 3: skipping worker threads (stack overflow — no concurrent malloc)");
    }

    /* --- Launch crash thread --- */
    switch (tc)
    {
        case 1:
            LE_INFO("--- Crash scenario 1: null pointer read ---");
            LE_INFO("Expected: FP chain crash1_inner<-crash1_mid<-crash1_outer");
            launchThread(crashThread1, NULL, "crashThread1");
            break;

        case 2:
            LE_INFO("--- Crash scenario 2: null function pointer (PC=0) ---");
            LE_INFO("Expected: PC=0, raw: has 0x0, FP chain crash2_inner<-mid<-outer");
            launchThread(crashThread2, NULL, "crashThread2");
            break;

        case 3:
            LE_INFO("--- Crash scenario 3: stack overflow ---");
            LE_INFO("Expected: FP chain broken, handler on altstack, no double-fault");
            launchThread(crashThread3, NULL, "crashThread3");
            break;

        case 4:
        default:
            LE_INFO("--- Crash scenario 4: crash DURING Legato teardown ---");
            LE_INFO("Sequence:");
            LE_INFO("  1. App runs with %d worker threads", N_WORKER_THREADS);
            LE_INFO("  2. Send 'app stop' — TermSignalHandler calls exit()");
            LE_INFO("  3. Concurrently crash4 thread wakes and crashes (PC=0)");
            LE_INFO("  4. Framework CrashWaitAtExit() holds _exit() for 2s");
            LE_INFO("  5. Crash handler finishes, tgkill terminates process");
            LE_INFO("Expected:");
            LE_INFO("  PC: 0x0000000000000000");
            LE_INFO("  raw: line in log BEFORE any (+0x) symbol lines");
            LE_INFO("  FP chain: crash4_invokeCallback<-crash4_doWork<-crash4_dispatch");
            LE_INFO("  Core dump generated");
            /* No app-level atexit handler — framework handles it. */
            launchThread(crashThread4, NULL, "crashThread4");
            break;
    }
}
