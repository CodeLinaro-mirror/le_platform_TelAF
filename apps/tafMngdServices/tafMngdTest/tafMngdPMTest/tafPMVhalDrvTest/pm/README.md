# PM VHAL Sample
PM VHAL is a module which can be implemented by different customers based on the different use cases and the interfaces defined by Qualcomm. It's integrated with TelAF Managed Power Management service as a component to fulfill the TCU level power state management.

This sample is a reference implementation of the PM VHAL interfaces.

## How to build
With the pre-set TelAF app development environment, follow the commands below to build the .so file.
```bash
cd pmDriver
./build.sh
Enter target (sa515m or sa525m):
sa525m # Supported targets: sa515m, sa525m.
```
The output pmVHalDrv.so is generated at pmDriver/sa525m/.
## How to deploy
Push the pmVHalDrv.so to the device via ADB tool.
```bash
adb push pmVHalDrv.so /app/
```
## Install the module
Login to the device console, ex. via ADB tool.
Run the following command to install the module.
```bash
tafmodule install /app/pmVHalDrv.so
```
After the successful module loading, restart Managed Power Management service by the following command to make the module take effect.
```bash
app restart tafMngdPMSvc
```
## How to simulate BUB status
The sample relies on pre-configured file to simulate different BUB status. Please make sure there's a file named bubevent existing in /data/le_fs/ folder on the device.

For the first time loading, please create the file manually.
```bash
touch /data/le_fs/bubevent
```
Fill in different BUB status in the file to simulate the wanted status.

### Candidate BUB Status
-1: BUB_STATUS_UNKNOWN

0: BUB_STATUS_NOT_IN_USE

1: BUB_STATUS_IN_USE


## How to simulate state change ack status
The sample relies on pre-configured file to simulate different state change ack status. Please make sure there's a file named ack existing in /data/le_fs/ folder on the device.

For the first time loading, please create the file manually.
```bash
touch /data/le_fs/ack
```
Fill in different state change ack status in the file to simulate the wanted status.

### Candidate state change ack Status
0: HAL_PM_RSP_READY
1: HAL_PM_RSP_NOT_READY
2: HAL_PM_RSP_TIMEOUT
3: HAL_PM_RSP_INVALID_REQUEST

## How to simulate Node state change status
The sample relies on pre-configured file to simulate different state change ack status. Please make sure there's a file named event existing in /data/le_fs/ folder on the device.

For the first time loading, please create the file manually.
```bash
touch /data/le_fs/event
```
Fill in different Node state change status in the file to simulate the wanted status.

### Candidate Node state change Status
0: HAL_PM_NODE_STATUS_READY
1: HAL_PM_NODE_STATUS_NOT_READY

## How to simulate wakeup vehichle response status
The sample relies on pre-configured file to simulate different wakeup vehichle response status. Please make sure there's a file named wakeupvehichleresp existing in /data/le_fs/ folder on the device.

For the first time loading, please create the file manually.
```bash
touch /data/le_fs/wakeupvehichleresp
```
Fill in different wakeup vehichle response status in the file to simulate the wanted status.

### Candidate wakeup vehichle response Status
0: HAL_PM_VEHICHLE_WAKEUP_STATUS_AWAKE
1: HAL_PM_VEHICHLE_WAKEUP_STATUS_INVALID_REQ
2: HAL_PM_VEHICHLE_WAKEUP_STATUS_UNKNOWN
