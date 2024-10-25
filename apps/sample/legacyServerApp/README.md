# A sample application converted from a legacy application and acting as a server of its client application
The application of cppserver_dataAdaptor is an application calling taf_dcs APIs and offering APIs to the client application
of cclient_dataAdaptor.

In the server application, it dumps the available data profiles and makes a data call with the default profile.
The .cdef wraps the cmake application to a TelAF component named dataAdaptorComp and provides the API of DumpProfile to its client.
The .adef creates a standard TelAF application depending on the dataAdaptorComp component.

In the client application, it calls the DumpProfile API from the server application to dump all the availabe data profiles.
## Get Started
Before building the applications, the toolchain needs to be installed properly and TelAF framework needs to be built successfully.

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

### Step 3 Build and install the server application
#### Step 3.1 Build the server application
```bash
cd ~/cppserver_dataAdaptor
mkapp -t sa525m -i $TELAF_INTERFACES dataAdaptorSvc.adef
```

The output dataAdaptorSvc.sa515m.update package locates in the current directory.

#### Step 3.2 Run the application on the target
Push dataAdaptorSvc.sa515m.update package to /data/ on device via ADB.
On device, install the package by the following command.

Lastly run the executable file.
```bash
cd /data/
update dataAdaptorSvc.sa525m.update
```
Now the server application is installed and brought up on the device.

### Step 4 Build and install the client application
#### Step 4.1 Build the client application
```bash
cd ~/cclient_dataAdaptor
mkapp -t sa525m -i $TELAF_INTERFACES dataAdaptorClient.adef
```

The output dataAdaptorClient.sa515m.update package locates in the current directory.

#### Step 4.2 Run the application on the target
Push dataAdaptorClient.sa515m.update package to /data/ on device via ADB.
On device, install the package by the following command.

Lastly run the executable file.
```bash
cd /data/
update dataAdaptorClient.sa525m.update
```
Now the client application is installed and brought up on the device.