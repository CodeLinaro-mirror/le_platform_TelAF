# A sample application of TelAF Data Call Service in C
The application calls taf_dcs APIs.
It dumps the available data profiles and makes a data call with the default profile.
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
cd ~/cmake_app
mkdir build
cd build
cmake ..
make
```

The output executable DataAppDemo locates in build/.

### Step 4 Run the application on the target
Push DataAppDemo to /data/ on device via ADB.
On device, run the following commands to set external library path and bind the APIs to taf_dcs service.

Lastly run the executable file.
```bash
sdir bind "<root>.taf_dcs" "<root>.taf_dcs"
./DataAppDemo
```