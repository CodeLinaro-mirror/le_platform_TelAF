# A sample application of TelAF Data Call Service in C++
The application calls taf_dcs APIs and taf_radio APIs via a dynamic library called DataAdaptor and RadioAdaptor.
It dumps the available data profiles and makes a data call with the default profile when the radio power status is on.
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
export TARGET=sa525m
```

### Step 3 Build the application
```bash
cd ~/cmake_app_with_share_library_cpp
mkdir build
cd build
cmake ..
make
```

The output libdataAdaptor.so locates in build/datalib/, libreadioAdaptor.so locates in build/radiolib,
the executable DataAppDemo locates in build/.

### Step 4 Run the application on the target
Push libdataAdaptor.so and libreadioAdaptor.so to /data/lib on device, DataAppDemo to /data/ on device via ADB.
On device, run the following commands to set external library path and bind the APIs to taf_dcs and taf_radio service.

Lastly run the executable file.
```bash
export LD_LIBRARY_PATH=/data/lib/
sdir bind "<root>.taf_dcs" "<root>.taf_dcs"
sdir bind "<root>.taf_radio" "<root>.taf_radio"
./DataAppDemo
```

