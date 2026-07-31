# tafLegacyCApp

A **legacy C application** that calls TelAF APIs **without being a Legato app**
(no `.adef`, not built by `mkapp`). It follows the official method in
[How to Use TelAF API in Legacy C App](https://qualcomm-confluence.atlassian.net/wiki/spaces/TAF2020/pages/278894990)
and the `apps/sample/legacyDcsApp` sample, but targets **tafTimeSvc**
(`taf_time`) instead of the data-call service.

It is intentionally **not** wired into the TelAF image build (not in
`modules/testApps.sinc`). You build it on demand with the toolchain + CMake and
push the binary to the target.

## How a legacy app uses TelAF (the real mechanism)

A legacy app is still linked against `liblegato` and uses the service's
**generated client-side IPC stubs** — it just integrates them manually (via
CMake `generate_client()` / `ifgen`) instead of through `mkapp`/`.adef`:

1. Generate the client stubs from the `.api`
   (`generate_client(${TELAF_ROOT}/interfaces/taf_time.api)`), producing
   `taf_time_interface.h`, `taf_time_client.c`, `taf_time_commonclient.c`.
2. `#include "legato.h"` and `#include "taf_time_interface.h"`.
3. On a thread that will use TelAF: `le_thread_InitLegatoThreadData(...)`, then
   `taf_time_ConnectService()` once, then call `taf_time_*()` APIs directly.
4. Link against `-llegato` (`pthread`, `rt`) with rpath to the on-target
   framework libs.

`main.c` here connects to `taf_time`, reads the system time via
`taf_time_GetTimeRef(TAF_TIME_SRC_NAME_SYSTEM)` + `taf_time_GetRefSystemTime()`,
prints it, and stays alive in the Legato event loop.

## Build

```bash
# 1. Source toolchain + TelAF env
cd ~/telaf
source set_af_env.sh sa525m
./bin/legs
export TARGET=$TARGET_GLOBAL

# 2. Build with CMake
cd apps/test/tafLegacyCApp
mkdir build && cd build
cmake ..
make
# -> build/tafLegacyCApp
```

## Run on target

```bash
# Push the binary (use a path your policy allows; /var/volatile is writable and
# is neither noexec nor nosuid on this platform).
adb push tafLegacyCApp /var/volatile/tafLegacyCApp
adb shell 'chmod +x /var/volatile/tafLegacyCApp; restorecon -v /var/volatile/tafLegacyCApp'

# Bind the legacy client's interface to the service (temporary, lost on restart).
# The bare process is identified by its user (root), hence the <root> form.
adb shell 'sdir bind "<root>.taf_time" "<telaf>.taf_time"'

# Run
adb shell '/var/volatile/tafLegacyCApp'
```

Expected: `LEGACY_APP: connected to taf_time service` followed by
`LEGACY_APP: TelAF system time = <sec>.<nsec>`.

## SELinux

Because the binary is not launched by the supervisor and does not live under
`/legato`, it needs its own policy module (in
`security/selinux/sepolicy/test/tafLegacyCApp/`) to:

- enter its own domain `telaf_tafLegacyCApp_t` (`init_telaf_app_domain`, which
  grants `telaf_use_service_directory`),
- get an explicit shell→app domain transition (it is run from a root shell, not
  the supervisor),
- reach the target service. Connecting a typed client to tafTimeSvc requires
  `telaf_ipc_bind(telaf_tafLegacyCApp_t, telaf_tafTimeSvc_t)`.

The `.fc` labels `/var/volatile/tafLegacyCApp` as `telaf_tafLegacyCApp_exec_t`.
The policy module must be built into the image (rebuild rootfs) for the type to
exist and for `restorecon` to apply it; `/var/volatile` is tmpfs so re-label
after each boot.

**Why /var/volatile:** on this platform the rootfs is read-only, and the other
writable mounts are either `noexec` (`/data`, `/persist`, `/systemrw`) or
`nosuid` (`/tmp`, `/run`). A SELinux domain transition is blocked on `nosuid`
mounts (`bounded_transition` denied), so `/var/volatile` — neither noexec nor
nosuid — is the only place the transition can occur for a side-loaded binary.
For production the binary should be integrated into the read-only image.
