# A sample C++ application that illustrates how a systemd based service/app can work with TelAF services through sync APIs/event callbacks/Async APIs.
This sample app also shows how 3rd-party application logics from "Main" and "AppTask" systemd based threads access TelAF services.
To emphasize the main scopes of the sample app outlined above, some aspects of the TelAF client side implementation, e.g., disconnect() from the
TelAF services and robustness of the client implementation are not shown. They will be addressed in the subsequent enhancements to the sample app.

libradio provides sample code to access to TelAF sync APIs and an event callback. Each C++ object instance is a client to the TelAF tafRadioSvc service.

libmpms provides sample code to access to TelAF async API. Each C++ object instance is a client to the TelAF tafMngdPMSvc service.

libdcs provides sample code to access to TelAF sync APIs and event callbacks. Each C++ object instance is a client to the TelAF tafDataCallSvc service.

The application has the following flow:
- Checks if radio state. If radio is off, it turns radio on.
- Waits for NAD registration
- Starts data with the default profile
- Prints the IPv4 address and host interface name
- Stops data
- Prints the call end reason
- Send a TCU wake up request asynchronously

The application can be stopped by using CTL+C

## Steps to build and execute sample application
Building the application requires TelAF toolchain installation and TelAF framework prebuilt.

### Step 1 Source the tool chain
```bash
cd ~/telaf
source set_af_env.sh sa525m
```

### Step 2 Setup TelAF building environment
```bash
cd ~/telaf
./bin/legs
export TARGET=$TARGET_GLOBAL
```

### Step 3 Build the application
```bash
cd apps/sample/legacyDcsApp/cmake_cpp_lib_telaf_api/
mkdir build
cd build
cmake ..
make
```

On successful build, libraries libmpmsAdaptor.so, libradioAdaptor.so and libdcsAdaptor.so will be
generated in build/libmpms/, build/libradio and build/libdcs respectively.
The sample app executable DataAppDemo will be generated in the build folder.

### Step 4 Run the application on the target

Ensure TelAF Managed Power Management Service is running. If the service is not running, start it.
```bash
app status tafMngdPMSvc
[stopped] tafMngdPMSvc
app start tafMngdPMSvc
app status tafMngdPMSvc
[running] tafMngdPMSvc
```

Push libmpmsAdaptor.so and libradioAdaptor.so to /tmp/lib on device, DataAppDemo to /tmp/ on device via ADB.
On device, run the following commands to set external library path and bind the APIs to taf_mngdPm and taf_radio service.

Note: /tmp is a volatile storage and its contents will be cleared after NAD reboot.

Lastly run the executable file.
```bash
export LD_LIBRARY_PATH=/tmp/lib/
sdir bind "<root>.taf_mngdPm" "<telaf>.taf_mngdPm"
sdir bind "<root>.taf_radio" "<telaf>.taf_radio"
sdir bind "<root>.taf_dcs" "<telaf>.taf_dcs"
./DataAppDemo
```
