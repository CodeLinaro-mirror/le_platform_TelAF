
..
   Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   SPDX-License-Identifier: BSD-3-Clause-Clear


Forced System Shutdown
----------------------

  .. mermaid::

    sequenceDiagram
        box TCU
            participant App as OEM Application
            participant Svc as taf_mngdPm
            participant VHAL as VHAL
        end

        box MCU
            participant MCUSW as MCUSW
        end

        Note over App,Svc: Request forceful TCU shutdown
        App->>Svc: 1. taf_mngdPM_ShutdownReqAsync(...)
        Svc->>VHAL: nodeStateChangePrepareAsync(...)

        Note over Svc,VHAL: VHAL confirms shutdown
        VHAL-->>Svc: 2. Accept / Reject shutdown

        Note over App,Svc: MPMS responds to client
        Svc-->>App: 3. async callback

        Note over Svc,VHAL: MPMS notifies VHAL to prepare
        Svc->>VHAL: 4. nodeStateChangeNotification(...)


        Note over App,Svc: Notify OEM client
        Svc->>App: 5. SHUTDOWN_PREPARE notification

        Note over App,Svc: OEM acknowledges
        App->>Svc: 6. taf_mngdPM_SendNodePowerStateChangeAck(...)

        Note over Svc,VHAL: Notify final shutdown
        Svc->>VHAL: 7. nodeStateChangeReqAsync(...)

        Note over VHAL,MCUSW: UART communication

        VHAL-->>Svc: nodeStateChangeReqAsyncCallback

        Note over Svc: Execute NAD shutdown
        Svc->>Svc: 8. Execute Linux shutdown

  .. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - 1
     - Client requests forceful TCU shutdown. MPMS requests VHAL to confirm whether the shutdown
       can be accepted or not.
   * - 2
     - VHAL will respond if it is ready or not, or if the request is invalid. In a successful
       forceful shutdown scenario, VHAL will respond in the callback with READY.
   * - 3
     - MPMS responds to the client on the confirmation result via the asynchronous callback.
   * - 4
     - MPMS notifies PM VHAL to prepare for shutdown which does not require acknowledgement.
   * - 5
     - MPMS notifies the OEM client application to prepare for shutdown which requires
       acknowledgement.
   * - 6
     - OEM client application acknowledges that it is prepared to shut down.
   * - 7
     - MPMS notifies VHAL for NAD shutdown execution after detecting all notifications are
       ACKed and VHAL will respond via a callback.
   * - 8
     - MPMS executes the NAD Linux shutdown.
