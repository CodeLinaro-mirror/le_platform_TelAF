# A sample C++ application that illustrates how a systemd based service/app can work with TelAF services through sync APIs/event callbacks/Async APIs.
This sample app also shows how 3rd-party application logics from "Main" and "AppTask" systemd based threads access TelAF services.
To emphasize the main scopes of the sample app outlined above, some aspects of the TelAF client side implementation, e.g., disconnect() from the
TelAF services and robustness of the client implementation are not shown. They will be addressed in the subsequent enhancements to the sample app.
libradio provides sample code to access to TelAF sync APIs and an event CB. Each C++ object instance is a client to the TelAF tafRadioSvc server.
libmpms provides sample code to access to TelAF async API. Each C++ object instance is a client to the TelAF tafMngdPMSvc server.
Since the examples of using TelAF Data Call Service (DCS) as an important part of the illustration will be added in the upcoming extension,
the sample application is placed under the "DcsApp" code structure.

## Get Started
Building the application requires TelAF toolchain installation and TelAF framework prebuild.

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

On successful build, libraries libmpmsAdaptor.so and libradioAdaptor.so will be generated in build/libmpms/ and build/libradio respectively.
The sample app executable DataAppDemo will be generated in the build folder.

### Step 4 Run the application on the target
Push libmpmsAdaptor.so and libradioAdaptor.so to /tmp/lib on device, DataAppDemo to /tmp/ on device via ADB.
On device, run the following commands to set external library path and bind the APIs to taf_mngdPm and taf_radio service.

Note: /tmp is a volatile storage and its contents will be cleared after NAD reboot.

Lastly run the executable file.
```bash
export LD_LIBRARY_PATH=/tmp/lib/
sdir bind "<root>.taf_mngdPm" "<telaf>.taf_mngdPm"
sdir bind "<root>.taf_radio" "<telaf>.taf_radio"
./DataAppDemo
```

