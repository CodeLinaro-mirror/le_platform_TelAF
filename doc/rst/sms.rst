
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

       App->>Svc: 0. taf_sms_Create()
       Svc-->>App: message reference

       App->>Svc: 1. taf_sms_SetText(msgRef, msgContent)
       Svc-->>App: LE_OK

       App->>Svc: 2. taf_sms_SetDestination(msgRef, destPhoneNum)
       Svc-->>App: LE_OK

       App->>Svc: 3. taf_sms_SetCallback(...)
       Svc-->>App: LE_OK

       App->>Svc: 4. taf_sms_Send(msgRef)
       Svc-->>App: LE_OK

       App->>Svc: 5. taf_sms_Delete(msgRef)
       Svc-->>App: LE_OK


.. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - 0
     - Create the message object.
   * - 1
     - Set the message content.
   * - 2
     - Set the destination.
   * - 3
     - Register callback.
   * - 4
     - Send message.
   * - 5
     - Delete message.


Message Receiving
-----------------

.. mermaid::

   sequenceDiagram
       participant App as SMS app
       participant Svc as SMS service

       App->>Svc: 0. taf_sms_SetPreferredStorage()
       Svc-->>App: LE_OK

       App->>Svc: 1. taf_sms_AddRxMsgHandler()
       Svc-->>App: handlerRef

       Svc->>App: 2. receive message callback

       App->>Svc: 3. taf_sms_GetPDU()
       Svc-->>App: LE_OK


.. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - 0
     - Set preferred storage.
   * - 1
     - Register receive handler.
   * - 2
     - Receive message notification.
   * - 3
     - Read message content.


Message Retrieving from Storage
-------------------------------

.. mermaid::

   sequenceDiagram
       participant App as SMS app
       participant Svc as SMS service

       App->>Svc: 0. taf_sms_CreateRxMsgList()
       Svc-->>App: MsgListRef

       App->>Svc: 1. taf_sms_GetFirst()
       Svc-->>App: MsgRef

       App->>Svc: 2. taf_sms_GetNext()
       Svc-->>App: MsgRef

       App->>Svc: 3. taf_sms_DeleteFromStorage()
       Svc-->>App: LE_OK

       App->>Svc: 4. taf_sms_DeleteList()


.. list-table::
   :header-rows: 1
   :class: longtable table-wrap
   :widths: 10 90

   * - Step
     - Description
   * - 0
     - Create RX list.
   * - 1
     - Get first message.
   * - 2
     - Iterate messages.
   * - 3
     - Delete message.
   * - 4
     - Delete list.
