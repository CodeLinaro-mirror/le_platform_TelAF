# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

APP=backtraceTest
PASS=0
FAIL=0
WARN=0
TIMEOUT=20      # seconds to wait for BACKTRACE output per case
TIMEOUT_CASE4=10 # Case 4: CrashWaitAtExit holds _exit() for up to 2s; add flush margin
WARMUP_SECS=3   # must match C source WARMUP_SECS
LOG_FILE=/tmp/backtrace_test_$$.log

# Colour — use printf so it works with /bin/sh (no echo -e)
RED='\033[0;31m'
GRN='\033[0;32m'
YLW='\033[1;33m'
BLU='\033[0;34m'
NC='\033[0m'

pass() { printf "${GRN}[PASS]${NC} %s\n" "$1"; PASS=$((PASS+1)); }
fail() { printf "${RED}[FAIL]${NC} %s\n" "$1"; FAIL=$((FAIL+1)); }
warn() { printf "${YLW}[WARN]${NC} %s\n" "$1"; WARN=$((WARN+1)); }
info() { printf "${BLU}[INFO]${NC} %s\n" "$1"; }

# Clean up log file on exit
trap 'rm -f $LOG_FILE' EXIT

# =============================================================================
# capture_backtrace <timeout_secs>
#   Reads logread -f output into $LOG_FILE, stopping when DONE is seen
#   or timeout expires.
#
#   logread on Legato prefixes every line with a syslog header, e.g.:
#     Nov 28 10:23:45 swi-mdm9x28 user.crit Legato:  BACKTRACE
#     Nov 28 10:23:45 swi-mdm9x28 user.crit Legato:  DONE
#
#   We strip the prefix (everything up to and including the last ': ')
#   so that $BTRACE contains only the bare message text, making all
#   pattern matches simple and prefix-independent.
# =============================================================================
# =============================================================================
# get_app_pid
#   Returns the PID of the running backtraceTest process.
#   Tries 'app status' output first, falls back to pgrep.
# =============================================================================
get_app_pid()
{
    local pid
    # 'app status backtraceTest' prints lines like:
    #   backtraceTest[1234] (running)
    pid=$(app status $APP 2>/dev/null | grep -o "$APP\[[0-9]*\]" | grep -o '[0-9]*')
    if [ -z "$pid" ]; then
        pid=$(pgrep -x $APP 2>/dev/null | head -1)
    fi
    printf '%s' "$pid"
}

# =============================================================================
# capture_backtrace <app_pid> <timeout_secs>
#   Waits for BACKTRACE..DONE in logread output, filtering to lines from
#   <app_pid> only. This prevents stale output from a previous test case
#   (still in the ring buffer) from being mistaken for the current one.
#
#   Strategy:
#     1. Start logread -f in background immediately (catches fast crashes).
#     2. Also scan the existing ring buffer (logread) for the PID — catches
#        output that arrived before logread -f started.
#     3. Filter both to lines containing [<pid>] before extracting the block.
# =============================================================================
BTRACE=""
capture_backtrace()
{
    local app_pid=$1
    local timeout_secs=$2
    local elapsed=0
    local pid_filter
    rm -f "$LOG_FILE"

    # Build the PID filter pattern: "backtraceTest[<pid>]"
    # If PID is unknown (app crashed before we could read it), accept any PID.
    if [ -n "$app_pid" ]; then
        pid_filter="${APP}\[${app_pid}\]"
    else
        pid_filter="${APP}\["
        info "(PID unknown — accepting any $APP log line)"
    fi

    # Start logread -f in background to catch output still in flight
    logread -f > "$LOG_FILE" 2>/dev/null &
    local logpid=$!

    # Also append the existing ring buffer immediately — catches output
    # that arrived between 'app start' and logread -f opening the socket.
    # Append (>>) so logread -f output is not overwritten.
    logread >> "$LOG_FILE" 2>/dev/null

    while [ $elapsed -lt $timeout_secs ]; do
        sleep 1
        elapsed=$((elapsed + 1))
        # Stop when DONE is seen, or for Case 4 when the app is no longer running
        if grep "$pid_filter" "$LOG_FILE" 2>/dev/null | grep -q 'DONE'; then
            break
        fi
        if [ "$3" = "nowait" ] && ! app status $APP 2>/dev/null | grep -q 'running'; then
            sleep 2  # let logger flush remaining pipe output before we read
            break
        fi
    done

    kill $logpid 2>/dev/null
    wait $logpid 2>/dev/null

    # Extract: keep only lines for this PID, strip syslog prefix, find block.
    # Log line format:
    #   Apr 18 16:12:41 sa525m user.err TelAF: =ERR= | backtraceTest[4930] | BACKTRACE
    # grep filters to this PID, sed strips everything up to last '| '.
    BTRACE=$(grep "$pid_filter" "$LOG_FILE" | \
             sed 's/.*| //' | \
             awk '/^BACKTRACE$/{found=1} found{print} /^DONE$/{exit}')
}

# =============================================================================
# check <description> <grep_pattern>
#   Uses basic grep (BRE) — works on busybox grep on target.
# =============================================================================
check()
{
    local desc=$1
    local pattern=$2
    if echo "$BTRACE" | grep -q "$pattern"; then
        pass "$desc"
    else
        fail "$desc  (pattern not found: $pattern)"
        info "--- First 30 lines of captured backtrace ---"
        echo "$BTRACE" | head -30
        info "--------------------------------------------"
    fi
}

check_not()
{
    local desc=$1
    local pattern=$2
    if ! echo "$BTRACE" | grep -q "$pattern"; then
        pass "$desc"
    else
        fail "$desc  (pattern should NOT be present: $pattern)"
    fi
}

# =============================================================================
# check_raw_before_symbols <tag>
#   Key regression check: raw: line must appear before any (+0x symbol line.
# =============================================================================
check_raw_before_symbols()
{
    local tag=$1
    local raw_line sym_line
    raw_line=$(echo "$BTRACE" | grep -n 'raw:' | head -1 | cut -d: -f1)
    sym_line=$(echo "$BTRACE" | grep -n '(+0x' | head -1 | cut -d: -f1)

    if [ -n "$raw_line" ] && [ -n "$sym_line" ] && \
       [ "$raw_line" -lt "$sym_line" ] 2>/dev/null; then
        pass "[$tag] raw: line (L$raw_line) before symbol lines (L$sym_line)"
    elif [ -n "$raw_line" ] && [ -z "$sym_line" ]; then
        # No symbol lines at all — maps unavailable (teardown) or binary stripped.
        # raw: is still present so addresses are captured. This is acceptable.
        pass "[$tag] raw: present, no symbol lines (stripped binary or maps gone — OK)"
    elif [ -z "$raw_line" ]; then
        fail "[$tag] raw: line not found at all"
    else
        fail "[$tag] raw: must appear before symbol lines (raw=L$raw_line sym=L$sym_line)"
    fi
}

# =============================================================================
# check_common <tag>
#   Critical checks that apply to EVERY crash scenario.
#   These must always pass — they represent the minimum viable diagnostic.
# =============================================================================
check_common()
{
    local tag=$1
    check      "[$tag] BACKTRACE header"          '^BACKTRACE$'
    check      "[$tag] PC/SP/LR/FP registers"     'PC:.*SP:.*LR:.*FP:'
    check      "[$tag] FP CHAIN section"          'BACKTRACE: FP CHAIN'
    check      "[$tag] FP CHAIN END"              'FP CHAIN END'
    check      "[$tag] STACK SCAN section"        'BACKTRACE: STACK SCAN'
    check      "[$tag] raw: address line"         'raw:'
    check_not  "[$tag] no double-fault"           'Catching SEGV.*Catching SEGV'
    check_raw_before_symbols "$tag"
}

# =============================================================================
# check_common_full <tag>
#   Full checks for cases where the process lives long enough to complete.
#   Adds STACK SCAN END and DONE on top of check_common.
# =============================================================================
check_common_full()
{
    local tag=$1
    check_common "$tag"
    check      "[$tag] STACK SCAN END"            'STACK SCAN END'
    check      "[$tag] DONE marker"               '^DONE$'
}

# =============================================================================
# check_core <tag>
#   Verify a core dump was generated.
# =============================================================================
check_core()
{
    local tag=$1
    local core
    core=$(ls -t /tmp/core* /tmp/*.core /legato/core* 2>/dev/null | head -1)
    if [ -n "$core" ]; then
        pass "[$tag] Core dump generated: $core"
    else
        warn "[$tag] No core dump — ensure: ulimit -c unlimited && echo '/tmp/core.%p' > /proc/sys/kernel/core_pattern"
    fi
}

# =============================================================================
# run_case <case_num> <description>
#   Stops any running instance, sets TEST_CASE, starts app, captures output.
#   Case 4 sends app stop after warmup to trigger SIGTERM.
#   Returns 0 on success, 1 on timeout.
# =============================================================================
run_case()
{
    local tc=$1
    local desc=$2
    local timeout_secs=${3:-$TIMEOUT}   # optional override, default $TIMEOUT
    local nowait=${4:-""}               # "nowait" = stop capture when app exits

    printf "\n"
    info "======================================================"
    info "Case $tc: $desc"
    info "======================================================"

    # Remove old core dumps so check_core is unambiguous
    rm -f /tmp/core* /tmp/*.core 2>/dev/null

    # Stop any running instance and reconfigure
    app stop $APP 2>/dev/null
    sleep 1
    config set apps/$APP/procs/$APP/envVars/TEST_CASE $tc
    app start $APP

    # Capture the PID immediately after start, before the crash fires.
    # Small sleep to let the process register with the supervisor.
    sleep 1
    local app_pid
    app_pid=$(get_app_pid)
    if [ -n "$app_pid" ]; then
        info "Case $tc: app PID=$app_pid"
    else
        info "Case $tc: PID not found yet (fast crash) — will accept any $APP log line"
    fi

    if [ "$tc" -eq 4 ]; then
        # crashThread4 crashes after WARMUP_SECS from app start.
        # We already spent 1s waiting for the PID, so wait WARMUP_SECS-1
        # more seconds so app stop arrives just as the crash fires.
        local wait4=$((WARMUP_SECS - 1))
        [ $wait4 -lt 0 ] && wait4=0
        info "Waiting ${wait4}s then sending 'app stop' to race with crash thread..."
        sleep $wait4
        app stop $APP &
    fi

    info "Waiting up to ${timeout_secs}s for BACKTRACE output..."
    capture_backtrace "$app_pid" $timeout_secs "$nowait"

    if [ -z "$BTRACE" ]; then
        fail "Case $tc: No BACKTRACE output within ${timeout_secs}s — possible deadlock"
        info "--- Last 10 lines of raw log ($LOG_FILE) ---"
        tail -10 "$LOG_FILE"
        info "--------------------------------------------"
        return 1
    fi

    local lines
    lines=$(echo "$BTRACE" | wc -l)
    info "Case $tc: Captured $lines lines (after prefix strip)"
    info "--- First 5 lines of stripped BTRACE ---"
    echo "$BTRACE" | head -5
    info "----------------------------------------"
    return 0
}

# =============================================================================
# Individual test cases
# =============================================================================

run_case1()
{
    if run_case 1 "Null pointer read"; then
        check_common_full 1
        check      "[1] frame count summary"       'frames:.*linked'
        check_not  "[1] PC not zero (read fault)"   'PC: 0x0000000000000000'
        # Binary is stripped — no symbol names, only offsets like (+0x1234).
        # Verify the FP chain produced at least 3 resolved frames.
        local fp_frames
        fp_frames=$(echo "$BTRACE" | grep -c '(+0x')
        if [ "$fp_frames" -ge 3 ]; then
            pass "[1] FP chain has $fp_frames resolved frames (>=3 expected)"
        else
            fail "[1] FP chain has only $fp_frames resolved frames (need >=3)"
        fi
        check_core 1
    fi
}

run_case2()
{
    if run_case 2 "Null function pointer (PC=0)"; then
        check_common_full 2
        check      "[2] frame count summary"       'frames:.*linked'
        check      "[2] PC is zero"                  'PC: 0x0000000000000000'
        # PC=0 is printed by SigWriteHexPtr(NULL) which zero-pads to full width.
    # Match the compact form 0x0 OR the zero-padded form 0x0000000000000000.
    check      "[2] raw line has 0x0"            'raw:.*0x0'
        # FP chain must have recovered frames despite PC=0
        local fp_frames
        fp_frames=$(echo "$BTRACE" | grep -c '(+0x')
        if [ "$fp_frames" -ge 3 ]; then
            pass "[2] FP chain recovered $fp_frames frames from valid FP (>=3 expected)"
        else
            fail "[2] FP chain has only $fp_frames frames (need >=3 — FP walk may have failed)"
        fi
        check_core 2
    fi
}

run_case3()
{
    if run_case 3 "Stack overflow"; then
        # check_common (not check_common_full): STACK SCAN END and DONE are
        # best-effort for stack overflow.  The normal stack is trashed; the
        # sigsetjmp guard (SA_ONSTACK) catches the fault and siglongjmp fires,
        # which may skip STACK SCAN END and DONE before tgkill terminates.
        # The critical minimum (BACKTRACE header through raw:) must always pass.
        check_common 3
        check_not  "[3] no double-fault"             'Catching SEGV while dumping'
        # STACK SCAN END and DONE: pass if present, warn if absent (not a hard fail).
        if echo "$BTRACE" | grep -q 'STACK SCAN END'; then
            pass "[3] STACK SCAN END present (handler survived scan)"
        else
            warn "[3] STACK SCAN END absent — sigsetjmp fired mid-scan (stack trashed, expected)"
        fi
        if echo "$BTRACE" | grep -q '^DONE$'; then
            pass "[3] DONE marker present"
        else
            warn "[3] DONE absent — process terminated before handler completed (expected for stack overflow)"
        fi
        # Stack overflow trashes the normal stack so the scan anchor may not
        # be found — accept either the summary line OR the anchor-not-found message.
        if echo "$BTRACE" | grep -q 'frames:.*linked'; then
            pass "[3] frame count summary present"
        elif echo "$BTRACE" | grep -q 'start_thread anchor not found'; then
            pass "[3] scan attempted (anchor not found — stack trashed, expected)"
        else
            warn "[3] neither frame count summary nor anchor-not-found message found (scan may have been interrupted)"
        fi
        check_core 3
    fi
}

run_case4()
{
    # Case 4: app stop is sent at WARMUP_SECS (same moment the crash fires).
    # CrashWaitAtExit's 50ms sleep overlaps the crash, sees the flag, and
    # holds _exit() at bay. Full output including STACK SCAN END + DONE expected.
    if run_case 4 "Crash during app stop (SIGTERM + null callback)" "$TIMEOUT_CASE4" "nowait"; then
        check_common_full 4
        check      "[4] PC is zero"               'PC: 0x0000000000000000'
        check      "[4] raw line has 0x0"         'raw:.*0x0'
        check      "[4] frame count summary"      'frames:.*linked'
        local fp_frames
        fp_frames=$(echo "$BTRACE" | grep -c '(+0x')
        if [ "$fp_frames" -ge 3 ]; then
            pass "[4] FP chain recovered $fp_frames frames during teardown (>=3 expected)"
        else
            info "[4] Only $fp_frames symbol lines (maps may be gone during teardown — OK if raw: present)"
        fi
        check_core 4
    fi
}

# =============================================================================
# Main
# =============================================================================

# Must run as root
if [ "$(id -u)" != "0" ]; then
    echo "ERROR: must run as root"
    exit 1
fi

# Enable core dumps
ulimit -c unlimited 2>/dev/null
echo '/tmp/core.%p' > /proc/sys/kernel/core_pattern 2>/dev/null
info "Core pattern set to /tmp/core.%p"

RUN_CASE=${1:-all}

case "$RUN_CASE" in
    1) run_case1 ;;
    2) run_case2 ;;
    3) run_case3 ;;
    4) run_case4 ;;
    *)
        run_case1
        run_case2
        run_case3
        run_case4
        ;;
esac

# =============================================================================
# Summary
# =============================================================================
printf "\n"
printf "============================================================\n"
printf " Results: %d passed, %d failed, %d warnings\n" $PASS $FAIL $WARN
printf "============================================================\n"

if [ $FAIL -eq 0 ]; then
    printf "${GRN}ALL TESTS PASSED${NC}\n"
    exit 0
else
    printf "${RED}%d TEST(S) FAILED${NC}\n" $FAIL
    exit 1
fi
