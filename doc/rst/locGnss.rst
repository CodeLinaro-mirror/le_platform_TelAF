
.. comment::

   Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   SPDX-License-Identifier: BSD-3-Clause-Clear



GNSS Session and Position Retrieval
----------------------------------

.. mermaid::

   sequenceDiagram
       participant App as Location Application
       participant Svc as tafLocationSvc

       Note over App,Svc: Session setup
       App->>Svc: 1. taf_locGnss_ConnectService()
       App->>Svc: 2. taf_locGnss_Enable()
       Svc-->>App: LE_OK

       Note over App,Svc: Start GNSS session
       App->>Svc: 3. taf_locGnss_Start()
       Svc-->>App: LE_OK

       Note over App,Svc: Register position handler
       App->>Svc: 4. taf_locGnss_AddPositionHandler(handlerFunc, ctx)
       Svc-->>App: PositionHandlerRef

       Note over App,Svc: Position fix received
       Svc->>App: 5. PositionHandlerFunction(positionSampleRef, ctx)

       Note over App,Svc: Check fix state
       App->>Svc: 6. taf_locGnss_GetPositionState(positionSampleRef)
       Svc-->>App: LE_OK (Fix 2D or 3D)

       Note over App,Svc: Get location
       App->>Svc: 7. taf_locGnss_GetLocation(...)
       Svc-->>App: LE_OK

       Note over App,Svc: Get altitude (3D only)
       App->>Svc: 8. taf_locGnss_GetAltitude(...)
       Svc-->>App: LE_OK

       Note over App,Svc: Get direction
       App->>Svc: 9. taf_locGnss_GetDirection(...)
       Svc-->>App: LE_OK

       Note over App,Svc: Get horizontal speed
       App->>Svc: 10. taf_locGnss_GetHorizontalSpeed(...)
       Svc-->>App: LE_OK

       Note over App,Svc: Get vertical speed
       App->>Svc: 11. taf_locGnss_GetVerticalSpeed(...)
       Svc-->>App: LE_OK

       Note over App,Svc: Release sample
       App->>Svc: 12. taf_locGnss_ReleaseSampleRef()

       Note over App,Svc: Remove handler
       App->>Svc: 13. taf_locGnss_RemovePositionHandler()

       Note over App,Svc: Stop GNSS
       App->>Svc: 14. taf_locGnss_Stop()
       Svc-->>App: LE_OK


.. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - 1
     - Connect to the location service.
   * - 2
     - Enable the GNSS service.
   * - 3
     - Start the GNSS session.
   * - 4
     - Register a position handler.
   * - 5
     - Receive position fix from handler.
   * - 6
     - Check fix state (2D or 3D).
   * - 7
     - Get latitude, longitude, and accuracy.
   * - 8
     - Get altitude (for 3D fix).
   * - 9
     - Get direction.
   * - 10
     - Get horizontal speed.
   * - 11
     - Get vertical speed.
   * - 12
     - Release sample reference.
   * - 13
     - Remove the handler.
   * - 14
     - Stop GNSS session.
