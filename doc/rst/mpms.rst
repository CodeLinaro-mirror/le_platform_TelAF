
.. comment::

   Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   SPDX-License-Identifier: BSD-3-Clause-Clear



Forced System Shutdown
----------------------

.. mermaid::

   sequenceDiagram
       participant App as OEM Application
       participant Svc as taf_mngdPm
       participant VHAL as VHAL
       participant MCUSW as MCUSW

       Note over App,Svc: Request forceful TCU shutdown
       App->>Svc: 1. taf_mngdPM_ShutdownReqAsync(...)
       Svc->>VHAL: nodeStateChangePrepareAsync(...)

       Note over Svc,VHAL: VHAL validates request
       VHAL-->>Svc: 2. Accept / Reject shutdown

       Note over App,Svc: Service responds to client
       Svc-->>App: 3. async callback

       Note over Svc,VHAL: Notify prepare phase
       Svc->>VHAL: 4. nodeStateChangeNotification(...)

       Note over App,Svc: Notify OEM client
       Svc->>App: 5. SHUTDOWN_PREPARE

       Note over App,Svc: OEM acknowledges
       App->>Svc: 6. SendNodePowerStateChangeAck(...)

       Note over Svc,VHAL: Trigger final shutdown
       Svc->>VHAL: 7. nodeStateChangeReqAsync(...)

       Note over VHAL,MCUSW: UART communication

       VHAL-->>Svc: nodeStateChangeReqAsyncCallback

       Note over Svc: Execute NAD shutdown
       Svc->>Svc: 8. Linux shutdown


.. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - 1
     - Client requests forceful shutdown; service asks VHAL for approval.
   * - 2
     - VHAL validates and returns readiness or rejection.
   * - 3
     - Service responds back to client asynchronously.
   * - 4
     - Service notifies VHAL to prepare for shutdown.
   * - 5
     - Service notifies OEM client to prepare for shutdown.
   * - 6
     - OEM client acknowledges readiness.
   * - 7
     - Service requests final shutdown execution through VHAL.
   * - 8
     - Service executes Linux shutdown.
