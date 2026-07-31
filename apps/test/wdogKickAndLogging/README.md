# wdogKickAndLogging

Two standalone TelAF test applications designed to simulate sustained high-load conditions for power-management testing. `tafTestWdogKick` continuously feeds watchdog kicks to `watchdogDaemon`, while `tafTestLoggingStorm` continuously feeds log messages to `logCtrlDaemon`. When either daemon holds the `CAP_BLOCK_SUSPEND` capability, the system cannot enter suspend mode while that daemon is actively processing — running both apps together keeps the system awake indefinitely under those conditions. Both apps can be run independently or simultaneously.

## Directory Layout

```
wdogKickAndLogging/
├── wdogKick/                    # Watchdog-kick component
│   ├── Component.cdef
│   └── wdogKick.c
├── loggingStorm/                # Logging-storm component
│   ├── Component.cdef
│   └── loggingStorm.c
├── tafTestWdogKick.adef         # App definition for wdogKick
└── tafTestLoggingStorm.adef     # App definition for loggingStorm
```

---

## Background — CAP_BLOCK_SUSPEND and Suspend Prevention

On Linux-based embedded systems, a process that holds the `CAP_BLOCK_SUSPEND` capability can acquire a wake lock that prevents the kernel from entering suspend mode. Two TelAF daemons are involved in these tests:

| Daemon | Triggered by | Holds `CAP_BLOCK_SUSPEND` while… |
|--------|-------------|----------------------------------|
| `watchdogDaemon` | Watchdog kick IPC calls | Processing each `le_wdog` kick request |
| `logCtrlDaemon` | Log message IPC calls | Routing each `LE_INFO` / log message |

The two test apps provide a controllable stimulus:

- **`tafTestWdogKick`** → keeps `watchdogDaemon` continuously busy with periodic IPC kicks.
- **`tafTestLoggingStorm`** → keeps `logCtrlDaemon` continuously busy with burst log messages.

If either daemon holds `CAP_BLOCK_SUSPEND` and its corresponding app is running, the system will repeatedly resume from (or never reach) suspend. Running both apps together is the worst-case scenario: both daemons hold wake locks concurrently, and the system stays fully awake regardless of the configured suspend policy.

---

## tafTestWdogKick

### Design

`tafTestWdogKick` uses the TelAF `watchdogChain` library to monitor the main event loop and automatically kick a single watchdog at a user-supplied interval.

- **Watchdog chain**: one watchdog (index 0) is initialized via `le_wdogChain_Init(1)` and bound to the event loop via `le_wdogChain_MonitorEventLoop(0, kickInterval)`. The chain library kicks the watchdog on behalf of the process whenever the loop completes a cycle within the interval.
- **Watchdog timeout**: 60 min (`watchdogTimeout: 3600000 ms`); maximum allowed is 100 min. Action on timeout is `stop`.
- **User requirement**: must run as `telaf`. The watchdog daemon crashes if the process runs as `root`.
- **Sandbox**: `sandboxed: false` — required for watchdog access.
- **Binding**: `tafTestWdogKick.watchdogChain.le_wdog` → `<tafcore>.le_wdog`.

### Parameters

| Flag | Long form | Required | Valid range | Description |
|------|-----------|----------|-------------|-------------|
| `-s` | `--second-kick=<sec>` | Yes | 0 < sec < 3600 | Watchdog kick interval (seconds) |
| `-h` | `--help` | No | — | Print usage and exit |

### Usage

```sh
# Print help
app runProc tafTestWdogKick --exe=tafTestWdogKick -- --help

# Kick every 2 s
app runProc tafTestWdogKick --exe=tafTestWdogKick -- -s 2 &

# Kick every 3 s
app runProc tafTestWdogKick --exe=tafTestWdogKick -- -s 3 &

# Kick every 5 s
app runProc tafTestWdogKick --exe=tafTestWdogKick -- -s 5 &
```

> **Note:** The following invocations do **not** work — the app has no `processes:` section:
> ```sh
> app start tafTestWdogKick                                              # does not work
> /legato/systems/current/appsWriteable/tafTestWdogKick/bin/tafTestWdogKick -s 5  # does not work
> ```
> Always use `app runProc … --exe=…`.

---

## tafTestLoggingStorm

### Design

`tafTestLoggingStorm` generates a controlled stream of log output by alternating between a **silence period** and a **logging burst**.

```
(start)
  │
  ├─ [--logging-first?] → LE_INFO × n ──┐
  │                                      │
  └─────────── wait <ms> ────────────────┘
                   │
                   └─ LE_INFO × n → wait <ms> → LE_INFO × n → …
```

- **Silence period**: a TelAF timer (`le_timer_SetRepeat(ref, 0)` for infinite repeat) fires after `<ms>` milliseconds.
- **Logging burst**: the timer handler calls `LE_INFO("Logging Storm ~ [%d]", i)` in a loop `<n>` times.
- **`--logging-first`**: if set, one burst is fired synchronously at startup before the timer is armed, so the sequence begins with logging instead of silence.
- No watchdog involvement.
- Must run as `telaf`, non-sandboxed, manual start.

### Parameters

| Flag | Long form | Required | Valid range | Description |
|------|-----------|----------|-------------|-------------|
| `-ms` | — | Yes | 0 < ms < 60000 | Silence interval between bursts (ms) |
| `-n` | — | Yes | n > 0 | Number of `LE_INFO` messages per burst |
| — | `--logging-first` | No | — | Fire one burst immediately before the first sleep |
| `-h` | `--help` | No | — | Print usage and exit |

### Usage

```sh
# Sleep 1 s, then log 10 messages, repeat
app runProc tafTestLoggingStorm --exe=tafTestLoggingStorm -- -ms 1000 -n 10 &

# Log 10 messages first, then sleep 1 s, repeat
app runProc tafTestLoggingStorm --exe=tafTestLoggingStorm -- -ms 1000 -n 10 --logging-first &

# Sleep 2 s, then log 20 messages, repeat
app runProc tafTestLoggingStorm --exe=tafTestLoggingStorm -- -ms 2000 -n 20 &

# Log 30 messages first, then sleep 3 s, repeat
app runProc tafTestLoggingStorm --exe=tafTestLoggingStorm -- -ms 3000 -n 30 --logging-first &
```

---

## Combined Testing

### Suspend-prevention test

Run both apps simultaneously to reproduce the worst-case condition where `watchdogDaemon` and `logCtrlDaemon` are both continuously active:

```sh
# Keep logCtrlDaemon busy: 500 ms silence, 50 messages per burst
app runProc tafTestLoggingStorm --exe=tafTestLoggingStorm -- -ms 500 -n 50 --logging-first &

# Keep watchdogDaemon busy: kick every 5 s
app runProc tafTestWdogKick --exe=tafTestWdogKick -- -s 5 &
```

**Expected outcome with `CAP_BLOCK_SUSPEND` present**: the system keeps resuming and cannot reach suspend mode.

**Expected outcome after removing `CAP_BLOCK_SUSPEND`**: the system enters suspend normally between kick/logging cycles once no daemon holds a wake lock.
