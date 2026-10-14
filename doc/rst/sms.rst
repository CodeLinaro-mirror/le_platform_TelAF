
.. comment::

   Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   SPDX-License-Identifier: BSD-3-Clause-Clear


Message Sending
---------------


    .. mermaid::

      sequenceDiagram
          participant App as SMS app
          participant Svc as SMS service

          Note over Svc: COMPONENT INIT

          Note over App,Svc: Create message reference
          App->>Svc: 0. taf_sms_Create()
          Svc-->>App: return the message reference

          Note over App,Svc: Set message content
          App->>Svc: 1. taf_sms_SetText(msgRef, msgContent)
          Svc-->>App: LE_OK

          Note over App,Svc: Set destination
          App->>Svc: 2. taf_sms_SetDestination(msgRef, destPhoneNum)
          Svc-->>App: LE_OK

          Note over App,Svc: Set callback
          App->>Svc: 3. taf_sms_SetCallback(msgRef, txCallback, context)
          Svc-->>App: LE_OK

          Note over App,Svc: Send message
          App->>Svc: 4. taf_sms_Send(msgRef)
          Svc-->>App: LE_OK

          Note over App,Svc: Delete message
          App->>Svc: 5. taf_sms_Delete(msgRef)
          Svc-->>App: LE_OK

    .. list-table::
     :header-rows: 1
     :class: longtable table-wrap
     :widths: 10 90

     * - Step
       - Description
     * - 0
       - Create the message, the following manipulation will use the returned message reference from service.
     * - 1
       - Set the message content.
     * - 2
       - Set the receiver's phone number.
     * - 3
       - Set the message callback to get the sending result.
     * - 4
       - Send the message.
     * - 5
       - Delete the message object if it is not used anymore.



Message Receiving
-----------------

    .. mermaid::

      sequenceDiagram
          participant App as SMS app
          participant Svc as SMS service

          Note over App,Svc: Set preferred storage
          App->>Svc: 0. taf_sms_SetPreferredStorage(storage)
          Svc-->>App: LE_OK

          Note over App,Svc: Register Rx message handler
          App->>Svc: 1. taf_sms_AddRxMsgHandler(handler, ctx)
          Svc-->>App: RxMsgHandlerRef

          Note over App,Svc: Receive message notification
          Svc->>App: 2. RxMsgHandlerFunction(msgRef, ctx)

          Note over App,Svc: Get PDU data
          App->>Svc: 3. taf_sms_GetPDU(msgRef, pdu)
          Svc-->>App: LE_OK

    .. list-table::
     :header-rows: 1
     :class: longtable table-wrap
     :widths: 10 90

     * - Step
       - Description
     * - 0
       - Set preferred storage to one of the storage types.
     * - 1
       - Set the message receiving handler to be able to receive any messages sent to the application.
     * - 2
       - Get the new message notification with the message reference.
     * - 3
       - Get the PDU data with the specified message reference.



Message Retrieving from Storage
-------------------------------

    .. mermaid::

      sequenceDiagram
          participant App as SMS app
          participant Svc as SMS service

          Note over App,Svc: Create Rx message list
          App->>Svc: 0. taf_sms_CreateRxMsgList()
          Svc-->>App: MsgListRef

          Note over App,Svc: Get first message
          App->>Svc: 1. taf_sms_GetFirst(msgListRef)
          Svc-->>App: MsgRef

          Note over App,Svc: Get next message
          App->>Svc: 2. taf_sms_GetNext(msgListRef)
          Svc-->>App: MsgRef

          Note over App,Svc: Delete message
          App->>Svc: 3. taf_sms_DeleteFromStorage(msgRef)
          Svc-->>App: LE_OK

          Note over App,Svc: Delete Rx message list
          App->>Svc: 4. taf_sms_DeleteList(msgListRef)

    .. list-table::
     :header-rows: 1
     :class: longtable table-wrap
     :widths: 10 90

     * - Step
       - Description
     * - 0
       - Create the Rx message list for listing the messages stored in SIM and HLOS.
     * - 1
       - Get message reference of the first message from Rx message list.
     * - 2
       - Get the message reference of the next message from the Rx message list.
     * - 3
       - Delete the message with the specified message reference.
     * - 4
       - Delete the Rx message list after all manipulations are complete.
