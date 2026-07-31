################################################################################################
#
#  TC8 client app build steps
#
###############################################################################################
Compile and runtime environment dependencies:
  Linux operating system
  A C++17 enabled compiler is needed.
  ets client app uses CMake as buildsystem.
  ets client app uses Boost >= 1.66.0

###############################################################################################
## Specify installation path
export WORKSPACE=~/workdir/TC8
export CMAKE_PREFIX_PATH=${WORKSPACE}/install-soa/
export GTEST_ROOT=~/workdir/vsomeip/googletest-main

1) vsomeip3 : Git clone (3.4.10) and build.
$ git clone https://github.com/COVESA/vsomeip.git -b 3.4.10
$ cd vsomeip
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=${WORKSPACE}/install-soa/ ..
$ make
$ make install

NOTE: make sure you have already installed the dependent software package(e.g. boost, dlt, gtest etc).
You can refer to https://github.com/COVESA/vsomeip for details.

2) CommonAPI : Git clone (3.2.0.1) and build steps:
$ git clone https://git.codelinaro.org/clo/la/platform/external/capicxx-core-runtime.git
$ cd capicxx-core-runtime
$ git reset --hard 89720d3c63bbd22cbccc80cdc92c2f2dd20193ba
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=${WORKSPACE}/install-soa/ ..
$ make
$ make install

3) CommonAPI-SomeIP : Git clone (3.2.0.1) and build steps:
$ git clone https://git.codelinaro.org/clo/la/platform/external/capicxx-someip-runtime.git
$ cd capicxx-someip-runtime
$ git reset --hard 0ad2bdc1807fc0f078b9f9368a47ff2f3366ed13
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=${WORKSPACE}/install-soa/ ..
$ make
$ make install

4) Required CommonAPI tool to be installed on the development host environment
a) capicxx-core-gen:
$ wget https://github.com/GENIVI/capicxx-core-tools/releases/download/3.2.0.1/commonapi_core_generator.zip
$ unzip commonapi_core_generator.zip -d commonapi_core_generator/

b) capicxx-someip-gen:
$ wget https://github.com/GENIVI/capicxx-someip-tools/releases/download/3.2.0.1/commonapi_someip_generator.zip
$ unzip commonapi_someip_generator.zip -d commonapi_someip_generator/

5) Use the installed CommonAPI tools to generate code for Fidl and Fdepl files.
By default, code will be generated in the src-gen folder of the current path
$ ./commonapi_core_generator/commonapi-core-generator-linux-x86_64 -sk etsApp/fidl/ets.fidl -d etsApp/src-gen/
$ ./commonapi_core_generator/commonapi-core-generator-linux-x86_64 -sk etsApp/fidl/ets.fdepl -d etsApp/src-gen/
$ ./commonapi_someip_generator/commonapi-someip-generator-linux-x86_64 etsApp/fidl/ets.fdepl -d etsApp/src-gen/

6) Compile ETS client app.
## Before compilation, modify the ip and port information in etsCliMain.cpp as per client device and server device

$ cd ets_app
$ mkdir build
$ cd build
$ export CMAKE_PREFIX_PATH=${WORKSPACE}/install-soa/
$ cmake -DCMAKE_INSTALL_PREFIX=${WORKSPACE}/install-soa/ ..
$ make
$ make install

8) Compile ETS service
## Add the ETS configuration in components/tafSomeipGWSvc/tafSomeipGWSvc.json
    "applications" :
    [
        ...
        {
            "name" : "ets_default_service",
            "id" : "0x5444"
        },
        {
            "name" : "ets_default_client",
            "id" : "0x5445"
        },
        {
            "name" : "ets_secondary_client",
            "id" : "0x5446"
        }
    ],

    "services" :
    [
        ...
        {
            "service": "257",
            "instance": "1",
            "unreliable": "30515",
            "unicast": "192.168.225.1",
            "someip-tp": {
                "service-to-client": ["77", "112", "113", "114", "32780", "79"]
            }
        },
        {
            "service": "258",
            "instance": "244",
            "unreliable": "30516",
            "unicast": "192.168.225.24",
            "multicast" :
            {
                "address" : "224.0.0.1",
                "port" : "32344"
            }
        }
    ],

## Add $TELAF_ROOT/apps/sample/etsSvc/tafEtsSvc/tafEtsSvc in modules/sa525m.sdef
## Build telaf image

9) Run ETS service
## connect to the DUT(SA525m) and run the ets
$ app start tafEtsSvc

10) Run ETS Client app
## Before running, configure the client json file as per client device (eg - unicast: 192.168.225.24, multicast: 224.0.0.1)

$ sudo route add 224.0.0.1 dev eth0
$ export COMMONAPI_CONFIG=${WORKSPACE}/ets_app/config/commonapiRef.ini
$ export VSOMEIP_CONFIGURATION=${WORKSPACE}/ets_app/config/ets_someip_client.json
$ export LD_LIBRARY_PATH=${WORKSPACE}/vsomeip/build:/home/lianyun/workdir/TC8/install-soa/lib:${PATH}
$ cd ${WORKSPACE}/ets_app/build
$ ./someipETSClient ets_default_client
