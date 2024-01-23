The multiple routing manager test apps are running on 2 devices (Device A and Device B).Test app A is running on Device A, test app B is runing onDevice B. The system configurations are as below, you can also find corresponding JSON configuration files in config/ directory.


-----------                                             -----------
|testApp A|     <================================>      |testApp B|
|Device A |                                             |Device B |
-----------                                             -----------

--------------------------------------------------------------------
|           default routing interface: "bridge0"                   |
|           default routing multicast: "224.0.0.1"                 |
|           default routing VLAN ID : 0                            |
--------------------------------------------------------------------
|unicast: "192.168.225.1"                   unicast:"192.168.225.2"|
|clientID: "0x0101"                         clientID: "0x0102"     |
--------------------------------------------------------------------

--------------------------------------------------------------------
|           routing 1 interface: "eth0.1"                          |
|           routing 1 multicast: "225.0.0.1"                       |
|           routing 1 VLAN ID : 1                                  |
--------------------------------------------------------------------
|unicast: "10.10.1.1"                       unicast:"10.10.1.2"    |
|clientID: "0x0201"                         clientID: "0x0202"     |
--------------------------------------------------------------------

--------------------------------------------------------------------
|           routing 2 interface: "eth0.2"                          |
|           routing 2 multicast: "226.0.0.1"                       |
|           routing 2 VLAN ID : 2                                  |
--------------------------------------------------------------------
|unicast: "10.10.2.1"                       unicast:"10.10.2.2"    |
|clientID: "0x0301"                         clientID: "0x0302"     |
--------------------------------------------------------------------


