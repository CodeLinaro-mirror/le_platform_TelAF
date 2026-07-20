
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

         Note over App,Svc: Get data reference
         App->>Svc: a.1. taf_mngdConn_GetData() OR taf_mngdConn_GetDataByName()
         Svc-->>App: LE_OK

         Note over App,Svc: Register data state handler
         App->>Svc: a.2. taf_mngdConn_AddDataStateHandler()
         Svc-->>App: LE_OK

         Note over App,Svc: Query data connection state
         App->>Svc: a.3. taf_mngdConn_GetDataConnectionState
         Svc-->>App: DATA_DISCONNECTED

         Note over Svc: s.2 Check SIM presence and Registration
         Note over Svc: s.3 Register Handlers (SIM, Radio and DCS)

         Note over Svc,DCS: Start data call
         Svc->>DCS: s.4. taf_dcs_StartSession()
         DCS-->>Svc: LE_OK

         Note over Svc: s.5 Call DataStateHandler

         Note over App,Svc: Data connected notification
         Svc->>App: a.4. DATA_CONNECTED

         Note over App,Svc: Query IP address
         App->>Svc: a.5. taf_mngdConn_DataGetConnectionIPAddresses()
         Svc-->>App: IP Addresses

.. list-table::
  :header-rows: 1
  :class: longtable table-wrap
  :widths: 10 90

  * - Step
    - Description
  * - Background setup
    - Set the Data→AutoStart key as Yes in the mngdConnectivity.json file.
  * - a.1
    - Get the data reference for a given data ID.
  * - a.2
    - Register the data state change handler to be able to receive the data state notification.
  * - a.3
    - Query the data connection state with the data reference.
  * - a.4
    - Receive the data connected notification from TelAF managed connectivity service.
  * - a.5
    - Query the IP address of the active data connection.
  * - s.1
    - Parse mngdConnectivity.json and validate Configuration and Policy parameters. If the .json
      file is inaccessible or if the validation fails, tafMngdConnSvc will use default json.
  * - s.2
    - Check SIM presence and registration.
  * - s.3
    - Register notification handlers.
  * - s.4
    - Start the data call.
  * - s.5
    - Receive the data call notification and process it.


**NOTE:** The application steps from a.1 to a.5 happen in parallel with TelAF managed connectivity
operation steps from s.1 to s.5. There is no strict sequence dependency from application operations
to the service operations.

Customer Application Triggered Data Connection Establishment
------------------------------------------------------------

.. mermaid::

     sequenceDiagram
         participant App as tafMngdConnSvc app
         participant Svc as tafMngdConnSvc service
         participant DCS as tafDataCallSvc

         Note over Svc: COMPONENT INIT

         Note over Svc: s.1 Validate mngdConnectivity.json
         Note over Svc: s.2 Register Handlers (SIM, Radio and DCS)

         Note over App,Svc: Get data reference
         App->>Svc: a.3. taf_mngdConn_GetData() OR taf_mngdConn_GetDataByName()
         Svc-->>App: LE_OK

         Note over App,Svc: Register data state handler
         App->>Svc: a.4. taf_mngdConn_AddDataStateHandler()
         Svc-->>App: LE_OK

         Note over App,Svc: Query data connection state
         App->>Svc: a.5. taf_mngdConn_GetDataConnectionState
         Svc-->>App: DATA_DISCONNECTED

         Note over App,Svc: Trigger data connection
         App->>Svc: a.6. taf_mngdConn_StartData()

         Note over Svc: s.7 Check SIM presence and Registration

         Note over Svc,DCS: Start data call
         Svc->>DCS: s.8. taf_dcs_StartSessionAsync()
         DCS-->>Svc: LE_OK

         Note over Svc: s.9 Call DataStateHandler

         Note over App,Svc: Data connected notification
         Svc->>App: a.10. DATA_CONNECTED

         Note over App,Svc: Query IP address
         App->>Svc: a.11. taf_mngdConn_DataGetConnectionIPAddresses()
         Svc-->>App: IP Addresses

.. list-table::
  :header-rows: 1
  :class: longtable table-wrap
  :widths: 10 90

  * - Step
    - Description
  * - Background setup
    - Set Data→AutoStart key as No in the mngdConnectivity.json file.
  * - s.1
    - Parse mngdConnectivity.json and validate Configuration and Policy parameters.
      If the .json file is inaccessible or if the validation fails, tafMngdConnSvc will use default
      json.
  * - s.2
    - Register notification handlers.
  * - a.3
    - Get the data reference for a given data ID.
  * - a.4
    - Register the data state change handler to be able to receive the data state notification.
  * - a.5
    - Query the data connection state with the data reference.
  * - a.6
    - Trigger the data connection establishment startup.
  * - s.7
    - Check SIM presence and registration.
  * - s.8
    - Start the data call.
  * - s.9
    - Receive the data call state notification and process it.
  * - a.10
    - Receive the data connected state notification.
  * - a.11
    - Query the IP address of the active data connection.
