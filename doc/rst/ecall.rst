
.. comment::

   Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   SPDX-License-Identifier: BSD-3-Clause-Clear


.. mermaid::

   sequenceDiagram
       participant App as ECall App
       participant Svc as ECall Service
       participant PSAP as PSAP

       Note over App,Svc: Create eCall session
       App->>Svc: 1. taf_ecall_Create()
       Svc-->>App: eCall reference

       Note over App,Svc: Register state change handler
       App->>Svc: 2. taf_ecall_AddStateChangeHandler(handler, context)
       Svc-->>App: handler reference

       Note over App,Svc: Construct MSD message
       App->>Svc: 3. taf_ecall_SetMsdXxx()
       Svc-->>App: LE_OK

       Note over App,Svc: Start eCall
       App->>Svc: 4. taf_ecall_StartAutomatic(eCallRef)
       Svc-->>App: LE_OK
       Svc->>PSAP: initiate eCall
       Svc->>App: DIALING / ALERTING / ACTIVE
       PSAP-->>Svc: call connected

       Note over App,Svc: Send MSD
       App->>Svc: 5. taf_ecall_SendMsd(eCallRef)
       Svc-->>App: LE_OK
       Svc->>PSAP: MSD transmission
       PSAP-->>Svc: MSD acknowledged
       Svc->>App: MSD states (STARTED → ACK → SUCCESS)

       Note over App,PSAP: Call active communication

       Note over App,Svc: End eCall
       App->>Svc: 6. taf_ecall_End(eCallRef)
       Svc-->>App: LE_OK
       Svc->>PSAP: BYE
       PSAP-->>Svc: call ended
       Svc->>App: ENDED


.. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - 1
     - Create an eCall session and obtain the eCall reference.
   * - 2
     - Register a state change handler for receiving eCall state updates.
   * - 3
     - Construct the MSD message including GNSS, vehicle, and passenger information.
   * - 4
     - Start the eCall; service initiates communication with PSAP.
   * - 5
     - Send MSD to PSAP and receive acknowledgment and transmission states.
   * - 6
     - End the eCall session and notify PSAP.
