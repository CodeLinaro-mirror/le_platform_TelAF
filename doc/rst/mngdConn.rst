
.. comment::

   Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   SPDX-License-Identifier: BSD-3-Clause-Clear


Automatic Data Connection Establishment
---------------------------------------

.. mermaid::

   sequenceDiagram
       participant App as tafMngdConnSvc app
       participant Svc as tafMngdConnSvc service
       participant DCS as tafDataCallSvc

       Note over Svc: COMPONENT INIT

       Note over Svc: s.1 Validate mngdConnectivity.json

       App->>Svc: a.1 taf_mngdConn_GetData()
       Svc-->>App: LE_OK

       App->>Svc: a.2 taf_mngdConn_AddDataStateHandler()
       Svc-->>App: LE_OK

       App->>Svc: a.3 taf_mngdConn_GetDataConnectionState()
       Svc-->>App: DATA_DISCONNECTED

       Note over Svc: s.2 Check SIM presence and Registration
       Note over Svc: s.3 Register handlers

       Svc->>DCS: s.4 taf_dcs_StartSession()
       DCS-->>Svc: LE_OK

       Svc->>App: a.4 DATA_CONNECTED

       App->>Svc: a.5 taf_mngdConn_DataGetConnectionIPAddresses()
       Svc-->>App: IP Addresses


.. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - Background
     - Data→AutoStart set to Yes in mngdConnectivity.json
   * - a.1
     - Get data reference
   * - a.2
     - Register state handler
   * - a.3
     - Query connection state
   * - a.4
     - Receive DATA_CONNECTED
   * - a.5
     - Get IP address
   * - s.1
     - Validate configuration
   * - s.2
     - Check SIM
   * - s.3
     - Register handlers
   * - s.4
     - Start data call
   * - s.5
     - Process notification


.. note::

   The application steps from a.1 to a.5 happen in parallel with TelAF managed connectivity operation steps from s.1 to s.5. There is no strict sequence dependency from application operations to the service operations.


Customer Application Triggered Data Connection Establishment
------------------------------------------------------------

.. mermaid::

   sequenceDiagram
       participant App as tafMngdConnSvc app
       participant Svc as tafMngdConnSvc service
       participant DCS as tafDataCallSvc

       Note over Svc: COMPONENT INIT

       Note over Svc: s.1 Validate configuration
       Note over Svc: s.2 Register handlers

       App->>Svc: a.3 taf_mngdConn_GetData()
       Svc-->>App: LE_OK

       App->>Svc: a.4 taf_mngdConn_AddDataStateHandler()
       Svc-->>App: LE_OK

       App->>Svc: a.5 taf_mngdConn_GetDataConnectionState()
       Svc-->>App: DATA_DISCONNECTED

       App->>Svc: a.6 taf_mngdConn_StartData()

       Note over Svc: s.7 Check SIM

       Svc->>DCS: s.8 taf_dcs_StartSessionAsync()
       DCS-->>Svc: LE_OK

       Svc->>App: a.10 DATA_CONNECTED

       App->>Svc: a.11 taf_mngdConn_DataGetConnectionIPAddresses()
       Svc-->>App: IP Addresses


.. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - Background
     - AutoStart disabled
   * - s.1
     - Validate configuration
   * - s.2
     - Register handlers
   * - a.3
     - Get data reference
   * - a.4
     - Register handler
   * - a.5
     - Query state
   * - a.6
     - Trigger data connection
   * - s.7
     - Check SIM
   * - s.8
     - Start data call
   * - s.9
     - Process notification
   * - a.10
     - Receive DATA_CONNECTED
   * - a.11
     - Get IP address

