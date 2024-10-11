# A sample application of TelAF Data Call Service in C
The application calls taf_dcs APIs.
It dumps the available data profiles and makes a data call with the default profile.
The .cdef wraps the cmake application to a TelAF component named DataAppDemoComp.
The .adef creates a standard TelAF application depending on the DataAppDemoComp component.
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
```

### Step 3 Build the application
```bash
cd ~/cmake_app_bundle
mkapp -t sa525m -i $TELAF_INTERFACES DataAppDemo.adef
```

The output DataAppDemo.sa515m.update package locates in the current directory.

### Step 4 Run the application on the target
Push DataAppDemo.sa515m.update package to /data/ on device via ADB.
On device, install the package by the following command.

Lastly run the executable file.
```bash
cd /data/
update DataAppDemo.sa525m.update
```
Now the application is installed and brought up on the device.