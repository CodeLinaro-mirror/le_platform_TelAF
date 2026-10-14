
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
        App->>Svc: 4. taf_locGnss_AddPositionHandler(PositionHandlerFunction, ctx)
        Svc-->>App: PositionHandlerRef

        Note over App,Svc: Position fix received
        Svc->>App: 5. PositionHandlerFunction(positionSampleRef, ctx)

        Note over App,Svc: Get location (latitude, longitude, hAccuracy)
        App->>Svc: 6. taf_locGnss_GetLocation(positionSampleRef, &lat, &lon, &hAcc)
        Svc-->>App: LE_OK

        Note over App,Svc: Get altitude
        App->>Svc: 7. taf_locGnss_GetAltitude(positionSampleRef, &alt, &vAcc)
        Svc-->>App: LE_OK

        Note over App,Svc: Get direction
        App->>Svc: 8. taf_locGnss_GetDirection(positionSampleRef, &dir, &dirAcc)
        Svc-->>App: LE_OK

        Note over App,Svc: Get horizontal speed
        App->>Svc: 9. taf_locGnss_GetHorizontalSpeed(positionSampleRef, &hSpd, &hSpdAcc)
        Svc-->>App: LE_OK

        Note over App,Svc: Get vertical speed
        App->>Svc: 10. taf_locGnss_GetVerticalSpeed(positionSampleRef, &vSpd, &vSpdAcc)
        Svc-->>App: LE_OK

        Note over App,Svc: Release sample
        App->>Svc: 11. taf_locGnss_ReleaseSampleRef(positionSampleRef)

        Note over App,Svc: Remove handler when done
        App->>Svc: 12. taf_locGnss_RemovePositionHandler(positionHandlerRef)

        Note over App,Svc: Stop GNSS session
        App->>Svc: 13. taf_locGnss_Stop()
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
     - Initialize the location service.
   * - 3
     - Start the GNSS session.
   * - 4
     - Add the position handler to be able to receive any GNSS fixes received from the location service.
   * - 5
     - Receive the position fix from the handler.
   * - 6
     - Get location information including longitude, latitude and horizontal accuracy.
   * - 7
     - Get the altitude information and its vertical accuracy.
   * - 8
     - Get the vehicle direction information and its accuracy.
   * - 9
     - Get the horizontal speed information and its accuracy.
   * - 10
     - Get the vertical speed information and its accuracy.
   * - 11
     - Release the sample reference.
   * - 12
     - When there's no need to receive any GNSS fixes from the application, the registered handler reference can be removed.
   * - 13
     - Stop the GNSS session with the service. This stops receiving fixes at the location service.

