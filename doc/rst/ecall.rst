
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
       Svc-->>App: return the eCall reference

       Note over App,Svc: Register state change handler
       App->>Svc: 2. taf_ecall_AddStateChangeHandler(handler, context)
       Svc-->>App: return the handler reference

       Note over App,Svc: Construct MSD message
       App->>Svc: 3. taf_ecall_SetMsdXxx()
       Svc-->>App: LE_OK

       Note over App,Svc: Start eCall
       App->>Svc: 4. taf_ecall_StartAutomatic(eCallRef)
       Svc-->>App: LE_OK
       Svc->>PSAP: PSAP received an eCall
       Svc->>App: StateChangeHandler(DIALING, ALERTING, ACTIVE)
       PSAP-->>Svc: eCall connected

       Note over App,Svc: Send MSD
       App->>Svc: 5. taf_ecall_SendMsd(eCallRef)
       Svc-->>App: LE_OK
       Svc->>PSAP: MSD sent to PSAP
       PSAP-->>Svc: All MSD received by PSAP
       Svc->>App: StateChangeHandler(MSD_TRANSMISSION_STARTED, LL_ACK_RECEIVED, SUCCESS)

       Note over App,PSAP: Call active

       Note over App,Svc: End eCall
       App->>Svc: 6. taf_ecall_End(eCallRef)
       Svc-->>App: LE_OK
       Svc->>PSAP: End the eCall
       PSAP-->>Svc: eCall Ended
       Svc->>App: StateChangeHandler(ENDED)


.. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - 1
     - Create an eCall session and get the eCall reference which will be used to make an eCall
       with PSAP.
   * - 2
     - Register the state change handler to get notifications from the TelAF eCall service when
       the eCall state gets changed.
   * - 3
     - Construct MSD message, including position information from the GNSS service, vehicle
       information, passenger count, etc.
   * - 4
     - Start an eCall via the eCall Service. The eCall service will initiate a corresponding
       eCall to PSAP. After this, the state change handler will be notified that the eCall state
       is DIALING and then ALERTING which means that the eCall successfully reached the PSAP.
       After the eCall is answered by the PSAP, the eCall state changes to ACTIVE and in MSD
       Pull mode, the eCall Service gets a state indication TAF_STATE_MSD_UPDATE_REQ from the
       PSAP to receive MSD. Without this state indication, as of now, the client application
       should update MSD to the NAD (i.e., modem) by calling the taf_ecall_SendMsd() API. Later,
       when the NAD gets an indication from the PSAP in pull mode, NAD will use the updated MSD
       and send it to the PSAP.
   * - 5
     - Start sending encoded MSD via the eCall Service to the PSAP. The eCall state becomes
       TAF_STATE_MSD_TRANSMISSION_STARTED and the TAF_STATE_LL_ACK_RECEIVED. After PSAP
       successfully receives the MSD, the eCall state changes to
       TAF_STATE_MSD_TRANSMISSION_SUCCESS and the state change handler will be notified.
       Call active: The eCall app might need to stay active to reserve a voice call channel
       to enable the passenger to talk with the PSAP.
   * - 6
     - When the conversation is finished, the eCall app can end the call using the eCall Service
       API. This will send the BYE signal to the PSAP. After the eCall is successfully ended, the
       eCall state will change to ENDED and the state change handler will be notified of the same.

