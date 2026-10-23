
..
   Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   SPDX-License-Identifier: BSD-3-Clause-Clear


Forced System Shutdown
----------------------

  .. mermaid::

    sequenceDiagram
        box TCU
            participant App as OEM Application
            participant Svc as tafMngdPMSvc
            participant VHAL as VHAL
        end

        box MCU
            participant MCUSW as MCUSW
        end

        Note over App,Svc: Request forceful TCU shutdown
        App->>Svc: 1. taf_mngdPm_ShutdownReqAsync(...)
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
        App->>Svc: 6. taf_mngdPm_SendNodePowerStateChangeAck(...)

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


Graceful System Shutdown
-------------------------

  .. mermaid::

    sequenceDiagram
        box TCU
            participant App as OEM Application
            participant Svc as tafMngdPMSvc
            participant VHAL as VHAL
        end

        box MCU
            participant MCUSW as MCUSW
        end

        Note over App,Svc: Register power state change handler
        App->>Svc: 1. taf_mngdPm_AddNodePowerStateChangeHandler(handlerFn, pmNodeId, bitmask, ctx)
        Svc-->>App: nodePowerStateChangeHandlerRef

        Note over App,Svc: Request graceful TCU shutdown
        App->>Svc: 2. taf_mngdPm_SetNodeTargetedPowerMode(NODE_PRIMARY_NAD, SHUTDOWN)
        Svc-->>App: LE_OK

        Note over Svc: MPMS waits for all wake sources to be released, MPMS triggers power state notification

        Note over Svc,VHAL: MPMS notifies VHAL to prepare for shutdown (no ack required)
        Svc->>VHAL: 3. nodeStateChangeNotification(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SHUTDOWN, NULL)

        Note over App,Svc: MPMS notifies OEM client to prepare for shutdown
        Svc->>App: 4. NodePowerStateChangeHandler(pmNodeId, ref, NODE_STATE_SHUTDOWN_PREPARE)

        Note over App,Svc: OEM client acknowledges readiness
        App->>Svc: 5. taf_mngdPm_SendNodePowerStateChangeAck(NODE_PRIMARY_NAD, Ref, CLIENT_READY)

        Note over Svc,VHAL: MPMS sends final graceful shutdown notification to VHAL
        Svc->>VHAL: 6. nodeStateChangeReqAsync(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SHUTDOWN, HAL_PM_SHUTDOWN_MODE_GRACEFUL, NodeStateChangeReqRespCB)

        Note over VHAL,MCUSW: UART communication

        VHAL-->>Svc: NodeStateChangeReqRespCB()

        Note over Svc: Execute NAD shutdown
        Svc->>Svc: 7. Execute Linux shutdown

  .. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - 1
     - Register the node power state change handler to be notified of shutdown-prepare and other
       state-change events for this node.
   * - 2
     - Client requests graceful TCU shutdown by setting the targeted power mode to SHUTDOWN.
       Unlike a forceful shutdown, MPMS will wait for all wake sources to be released before
       proceeding.
   * - 3
     - MPMS notifies VHAL that the node is entering the shutdown state. This call does not
       require a VHAL acknowledgement.
   * - 4
     - MPMS notifies the OEM client application to prepare for shutdown, which requires
       acknowledgement.
   * - 5
     - OEM client application acknowledges that it is prepared to shut down.
   * - 6
     - MPMS notifies VHAL for NAD shutdown execution after detecting all notifications are
       ACKed and VHAL will respond via a callback.
   * - 7
     - MPMS executes the NAD Linux shutdown.


System Suspend
-------------------------

  .. mermaid::

    sequenceDiagram
        box TCU
            participant App as OEM Application
            participant Svc as tafMngdPMSvc
            participant VHAL as VHAL
        end

        box MCU
            participant MCUSW as MCUSW
        end

        Note over App,Svc: Register power state change handler
        App->>Svc: 1. taf_mngdPm_AddNodePowerStateChangeHandler(handlerFn, pmNodeId, bitmask, ctx)
        Svc-->>App: nodePowerStateChangeHandlerRef

        Note over App,Svc: Request TCU suspend
        alt Trigger 1
            App->>Svc: 2.1. taf_mngdPm_SetNodeTargetedPowerMode(NODE_PRIMARY_NAD, SUSPEND)
            Svc-->>App: LE_OK
        else Trigger 2
            App->>Svc: 2.2. taf_mngdPm_Relax(wsRef)
        end

        Note over Svc: MPMS waits for all wake sources to be released, MPMS triggers power state notification

        Note over Svc,VHAL: MPMS notifies VHAL to prepare for suspend (no ack required)
        Svc->>VHAL: 3. nodeStateChangeNotification(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SUSPEND, NULL)

        Note over App,Svc: MPMS notifies OEM client to prepare for suspend
        Svc->>App: 4. NodePowerStateChangeHandler(pmNodeId, ref, NODE_STATE_SUSPEND_PREPARE)

        Note over App,Svc: OEM client acknowledges readiness
        App->>Svc: 5. taf_mngdPm_SendNodePowerStateChangeAck(NODE_PRIMARY_NAD, Ref, CLIENT_READY)

        Note over Svc,VHAL: MPMS sends final suspend notification to VHAL
        Svc->>VHAL: 6. nodeStateChangeReqAsync(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_SUSPEND, HAL_PM_SUSPEND_MODE_FULL, NodeStateChangeReqRespCB)

        Note over VHAL,MCUSW: UART communication

        VHAL-->>Svc: NodeStateChangeReqRespCB [nodeStateChangeReqAsyncCallback]

        Note over Svc: Execute NAD suspend
        Svc->>Svc: 7. Execute Linux suspend


  .. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - 1
     - Register the node power state change handler to be notified of suspend-prepare and other
       state-change events for this node.
   * - 2.1
     - Client requests TCU suspend by setting the targeted power mode to SUSPEND. MPMS returns
       ``LE_OK``, waits for all wake sources to be released, and triggers the power state
       notification.
   * - 2.2
     - Client releases the wake source using ``taf_mngdPm_Relax(wsRef)``. MPMS waits for all wake sources to be released, and triggers the power state notification.
   * - 3
     - MPMS notifies VHAL that the node is entering the suspend state. This call does not
       require a VHAL acknowledgement.
   * - 4
     - MPMS notifies the OEM client application to prepare for suspend, which requires
       acknowledgement.
   * - 5
     - OEM client application acknowledges that it is prepared to suspend.
   * - 6
     - MPMS sends the final suspend notification to VHAL after detecting all clients have
       acknowledged. VHAL responds via a callback.
   * - 7
     - MPMS executes the NAD Linux suspend.



TCU Resume
----------

  .. mermaid::

    sequenceDiagram
        box TCU
            participant App as OEM Application
            participant Svc as tafMngdPMSvc
            participant VHAL as VHAL
        end

        box MCU
            participant MCUSW as MCUSW
        end

        rect rgb(235, 235, 235)
            Note over App,VHAL: NAD Suspend
        end

        Note over App,VHAL: NAD awakes (e.g. incoming SMS)

        Note over App,Svc: Create a wakeup source reference
        App->>Svc: 1. taf_mngdPm_CreateWakeupSource(reason, option, wsTag)
        Svc-->>App: wsRef

        Note over App,Svc: Acquire the wake source (TCU resume trigger)
        App->>Svc: 2. taf_mngdPm_StayAwake(wsRef)

        Note over Svc,VHAL: MPMS notifies VHAL that the wake source was acquired
        Svc->>VHAL: 3. nodeInfoNotification(NODE_ID, HAL_PM_NODE_INFO_LOCK_ACQUIRED, reason)

        Svc-->>App: LE_OK

        Note over Svc: MPMS transitions NAD to RESUME

        Note over Svc: MPMS triggers power state notification

        Note over Svc,VHAL: MPMS notifies VHAL of the resume state (no ack required)
        Svc->>VHAL: 4. nodeStateChangeNotification(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_RESUME, NULL)

        Note over App,Svc: MPMS notifies OEM client of resume
        Svc->>App: 5. NodePowerStateChangeHandler(pmNodeId, ref, NODE_STATE_RESUME)

        Note over App,Svc: OEM client acknowledges readiness
        App->>Svc: 6. taf_mngdPm_SendNodePowerStateChangeAck(NODE_PRIMARY_NAD, Ref, CLIENT_READY)

  .. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - 1
     - Create a wakeup source reference that will be used to acquire the wake source that
       triggers TCU resume.
   * - 2
     - Client acquires the wake source via the created wakeup source reference.
   * - 3
     - MPMS notifies VHAL that a wake source has been acquired. This call does not require a
       VHAL acknowledgement.
   * - 4
     - Once MPMS transitions to RESUME (triggered by the wake source count moving off zero),
       MPMS notifies VHAL of the resume state. This call does not require a VHAL
       acknowledgement.
   * - 5
     - MPMS notifies the OEM client application that the node has resumed.
   * - 6
     - OEM client application acknowledges the resume notification.


Forced System Restart (NAD Reboot)
-----------------------------------

  .. mermaid::

    sequenceDiagram
        box TCU
            participant App as OEM Application
            participant Svc as tafMngdPMSvc
            participant VHAL as VHAL
        end

        box MCU
            participant MCUSW as MCUSW
        end

        Note over App,Svc: Client requests forceful system restart for NAD reboot
        App->>Svc: 1. taf_mngdPm_RestartReqAsync(RESTART_MODE_NAD_REBOOT, handlerRef, reason)

        Note over Svc,VHAL: MPMS asks VHAL to prepare for restart
        Svc->>VHAL: 2. nodeStateChangePrepareAsync(NODE_ID, HAL_PM_NODE_STATE_RESTART, HAL_PM_RESTART_MODE_NAD_REBOOT, reason, RestartPrepareRespCB)

        Note over App,Svc: MPMS responds to client that restart is accepted
        Svc-->>App: 3. AsyncRestartReqHandler(RESTART_MODE_NAD_REBOOT, READY, LE_OK, contextPtr)

        Note over Svc,VHAL: MPMS notifies VHAL of the restart-prepare state (no ack required)
        Svc->>VHAL: 4. nodeStateChangeNotification(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_RESTART, NULL)

        Note over App,Svc: MPMS notifies OEM client to prepare for restart
        Svc->>App: 5. NodePowerStateChangeHandler(pmNodeId, ref, NODE_STATE_RESTART_PREPARE)

        Note over App,Svc: OEM client acknowledges readiness
        App->>Svc: 6. taf_mngdPm_SendNodePowerStateChangeAck(NODE_PRIMARY_NAD, Ref, CLIENT_READY)

        Note over Svc,VHAL: MPMS sends the final NAD reboot notification to VHAL
        Svc->>VHAL: 7. nodeStateChangeReqAsync(NODE_PRIMARY_NAD, HAL_PM_NODE_STATE_RESTART, HAL_PM_RESTART_MODE_NAD_REBOOT, NodeStateChangeReqRespCB)

        Note over VHAL,MCUSW: UART communication with MCU
        VHAL-->>Svc: 8. NodeStateChangeReqRespCB(pmNodeId, HAL_PM_NODE_STATE_RESTART, HAL_PM_RESTART_MODE_NAD_REBOOT)

        Note over Svc: Execute NAD reboot
        Svc->>Svc: 9. Execute Linux reboot

  .. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - 1
     - Client requests forceful restart for NAD reboot.
   * - 2
     - MPMS asks VHAL to prepare for restart, passing the NAD reboot power mode.
   * - 3
     - MPMS responds to the client via the asynchronous callback that the restart is
       accepted.
   * - 4
     - MPMS notifies VHAL of the restart-prepare state. This call does not
       require a VHAL acknowledgement.
   * - 5
     - MPMS notifies the OEM client application to prepare for restart, which requires
       acknowledgement.
   * - 6
     - OEM client application acknowledges that it is prepared to restart.
   * - 7
     - MPMS sends the final NAD reboot notification to VHAL after detecting all clients have
       acknowledged.
   * - 8
     - VHAL confirms the NAD reboot request via its response callback.
   * - 9
     - The NAD Linux reboot is executed.

