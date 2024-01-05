# TelAF Connection Manager
TelAF Connection Manager is a TelAF Application that manages data connection(s) on the NAD.

TelAF-CM uses TelAF's Managed Connectivity Service APIs to start and manage data connection(s)
using Configuration and Policy JSON file. Please refer to the Managed Connectivity Service's
documentation for more information.

TelAF-CM packages sample configuration and policy JSON file. This file should be copied to the
following locations on the NAD:
| File         | Location |
|--------------|:-----:|
| mngdConnectivity.json | /data/ManagedServices/ |

# Running TelAF-CM
TelAF-CM will run only after the Managed Connectivity Service has successfully parsed the provided
configuration and policy files.
If data AutoStart is Yes, TelAF-CM will print connection information once connected.
If data AutoStart is No, TelAF-CM will start the data session and after data conntects, TelAF-CM
will print the connection information.