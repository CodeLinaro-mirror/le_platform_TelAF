# A sample application of TelAF plugin by using lagency libs
Important:
   This aplication is trying to call the config tree API which is NOT suggested, but only an example for very special case. Do Not Call service API in plugin.

## Get Started
Before building the application, the toolchain needs to be installed properly and TelAF framework needs to be built successfully.

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
cd telaf/apps/sample/plugin/didStore
mkdir build
cd build
cmake ..
make
```

The output  locates in telaf/apps/sample/plugin/didStore/build/libTafPiDiagDID_shared_lib.so.

### Step 4 Run the application on the target
Push the lib "libTafPiDiagDID_shared_lib.so" to $target device and rename to "TafPiDiagDID.so".
$ adb push libTafPiDiagDID_shared_lib.so  /tmp/TafPiDiagDID.so

On device, run the following commands to install this lib.
```bash
setenforce 0
tafmodule install /tmp/TafPiDiagDID.so
```

