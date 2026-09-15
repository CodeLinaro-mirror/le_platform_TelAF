#!/bin/sh
#
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#
# Helper for the keyFileReliabilityTest app. Invoked by
# keyFileReliabilityTest.c (TakeActionToKeyFile()) as:
#   sh keyFileReliabilityTest.sh <action> <keyFileName> <backupKeyFileName>
#
# <action> is one of:
#   del_act_restart_restore      del_bak_restart_restore
#   corrupt_act_restart_restore  corrupt_bak_restart_restore
#   del_act_runtime_restore      del_bak_runtime_restore
#   corrupt_act_runtime_restore  corrupt_bak_runtime_restore
#   check_files_restored         (post-condition for the runtime cases)
#
# <keyFileName> and <backupKeyFileName> are passed in by the caller rather than
# hardcoded here, so the expected key files stay defined in one place (KeyFile
# and KeyBackupFile in keyFileReliabilityTest.c) alongside the key ID they are
# derived from. The two names do not match: the service names the backup copy
# with its own scheme, so it cannot be derived from the active one.

action="$1"
keyFileName="$2"
backupKeyFileName="$3"

if [ -z "$action" ] || [ -z "$keyFileName" ] || [ -z "$backupKeyFileName" ]; then
    echo "keyFileReliabilityTest.sh: usage: $0 <action> <keyFileName>" \
         "<backupKeyFileName>" >&2
    exit 1
fi

# mkTools gives the app only "<app bin>:/usr/local/bin:/usr/bin:/bin", but the
# two tools used below live outside that: "logread" is a busybox applet
# symlinked into /sbin, and Legato's "app" tool is under the framework bin dir.
PATH="$PATH:/sbin:/usr/sbin:/legato/systems/current/bin"

activeKeyFileDir="/persist/tafKeyStoreSvc/keyMaster_pa"
backupKeyFileDir="/data/persist/tafKeyStoreSvc/AppKey"
activeKeyFile="${activeKeyFileDir}/${keyFileName}"
backupKeyFile="${backupKeyFileDir}/${backupKeyFileName}"

# Covers the restart and the wait for re-initialization together, since both run
# under a single "timeout" below.
RESTART_WAIT_TIMEOUT_SEC=30
# Logged by tafKeyStoreSvc.c COMPONENT_INIT once taf_pa_ks_Init() returns OK,
# which is after the PA has loaded (and restored) the app key files, and just
# before the service is advertised.
RESTART_INIT_MARKER="TelAF keyStore PA initialized."

# A missing key file means the test's precondition never held, so treat it as
# a failure rather than silently "succeeding" at deleting/corrupting nothing.
RequireKeyFile()
{
    if [ ! -f "$1" ]; then
        echo "keyFileReliabilityTest.sh: key file '$1' does not exist." >&2
        return 1
    fi
    return 0
}

# Deletes $1. Succeeds only if the file is actually gone afterward.
DeleteKeyFile()
{
    RequireKeyFile "$1" || return 1
    rm -f "$1"
    [ ! -e "$1" ]
}

# Overwrites $1 in place with zero bytes, keeping its size unchanged, so it
# still exists and looks like a key file but its content is garbage.
CorruptKeyFile()
{
    RequireKeyFile "$1" || return 1
    fileSize=$(wc -c < "$1" 2>/dev/null)
    if [ -z "$fileSize" ] || [ "$fileSize" -eq 0 ]; then
        echo "keyFileReliabilityTest.sh: key file '$1' is empty or unreadable." >&2
        return 1
    fi
    dd if=/dev/zero of="$1" bs="$fileSize" count=1 conv=notrunc >/dev/null 2>&1
    [ "$(wc -c < "$1" 2>/dev/null)" = "$fileSize" ]
}

# Restarts tafKeyStoreSvc and waits for it to finish re-initializing.
#
# The log is streamed rather than sampled after the fact. "logread -f" reports
# only what is appended once it has attached, and the restart floods the syslog
# ring buffer, so a reader that inspects the buffer afterwards can find the
# marker already evicted. "-F" is not a substitute either: it replays the buffer,
# so it matches markers left by earlier cases or by boot and returns before the
# service has actually restarted.
#
# Ordering assumption: "app restart" returns once the Supervisor has spawned the
# new process, while the marker is logged later, from its COMPONENT_INIT, after
# the PA has initialized -- around a second on this target. "logread -f" is
# exec'd in between and attaches in milliseconds, so it is listening well before
# the marker appears. The margin is large but it is a margin, not a guarantee: a
# service that reached COMPONENT_INIT faster than logread attaches would be
# missed, and the case would report a timeout.
#
# Everything runs in the foreground as one pipeline. Backgrounding the reader was
# tried and abandoned: it needs explicit cleanup, and signalling it by process
# group is easy to get wrong in a script with no controlling terminal -- an
# earlier revision killed the script along with the reader that way. Here
# "grep -q" exits at the first match, the closed pipe makes "logread -f" exit on
# SIGPIPE, and one "timeout" bounds the restart and the wait together, so a
# restart that never returns fails the case instead of hanging it.
RestartAndWaitForReinit()
{
    if ! timeout "$RESTART_WAIT_TIMEOUT_SEC" \
            sh -c 'app restart tafKeyStoreSvc && logread -f' \
            | grep -q "$RESTART_INIT_MARKER"; then
        echo "keyFileReliabilityTest.sh: restart did not reach '$RESTART_INIT_MARKER'" \
             "within ${RESTART_WAIT_TIMEOUT_SEC}s (restart failure and timeout are" \
             "indistinguishable here). Last syslog lines follow." >&2
        logread 2>/dev/null | tail -n 5 >&2
        return 1
    fi

    return 0
}

# Active and backup key files must end up byte-identical once the service has
# restored the missing/corrupted one from the other. Byte equality is the right
# assertion: the backup is a plain copy (BackUpFileAndSELinuxContext() in
# tafFilePa.c), and the service itself decides a backup is current by comparing
# its CRC32C against the primary's (RefreshBackupIfNeeded()). Only the file names
# differ. Weakening this to an existence or non-empty test would make the
# "corrupt" cases unfailable, since zero-filling preserves the file size.
FilesMatchAfterRestore()
{
    [ -f "$activeKeyFile" ] && [ -f "$backupKeyFile" ] && cmp -s "$activeKeyFile" "$backupKeyFile"
}

# Post-condition for the 4 "*_runtime_restore" cases, run by the caller once it
# has exercised the key through the service. Asserting this separately from the
# signature check is what tells a real restore apart from an operation served
# out of the service's in-memory key cache, which would leave the damaged file
# on disk untouched and still report a passing signature.
CheckFilesRestored()
{
    if ! FilesMatchAfterRestore; then
        echo "keyFileReliabilityTest.sh: active/backup key file still missing or mismatched" \
             "after runtime restore." >&2
        return 1
    fi
    return 0
}

# Common tail for the 4 "*_restart_restore" cases: restart tafKeyStoreSvc,
# wait for it to finish re-initializing, then check the restore result.
RunRestartRestoreCase()
{
    if ! RestartAndWaitForReinit; then
        return 1
    fi

    if ! FilesMatchAfterRestore; then
        echo "keyFileReliabilityTest.sh: active/backup key file mismatch after restart restore." >&2
        return 1
    fi

    return 0
}

case "$action" in
    del_act_restart_restore)
        DeleteKeyFile "$activeKeyFile" && RunRestartRestoreCase
        ;;
    del_bak_restart_restore)
        DeleteKeyFile "$backupKeyFile" && RunRestartRestoreCase
        ;;
    corrupt_act_restart_restore)
        CorruptKeyFile "$activeKeyFile" && RunRestartRestoreCase
        ;;
    corrupt_bak_restart_restore)
        CorruptKeyFile "$backupKeyFile" && RunRestartRestoreCase
        ;;
    del_act_runtime_restore)
        DeleteKeyFile "$activeKeyFile"
        ;;
    del_bak_runtime_restore)
        DeleteKeyFile "$backupKeyFile"
        ;;
    corrupt_act_runtime_restore)
        CorruptKeyFile "$activeKeyFile"
        ;;
    corrupt_bak_runtime_restore)
        CorruptKeyFile "$backupKeyFile"
        ;;
    check_files_restored)
        CheckFilesRestored
        ;;
    *)
        echo "keyFileReliabilityTest.sh: unknown action '$action'." >&2
        exit 1
        ;;
esac

exit $?
