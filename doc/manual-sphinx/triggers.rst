=========================
Trigger
=========================

Request
=======

Path
----
.. list-table::
   :header-rows: 1

   * - Format
     - URL
   * - JSON
     - /triggers.json
   * - JSONP
     - /triggers.jsonp

Parameters
----------
.. list-table::
   :header-rows: 1

   * - Parameter
     - Brief
     - JSON
     - JSONP
   * - callback
     - A callback function name for a JSONP format.
     - N/A
     - Mandatory

Response
========

Repsponse structure
-------------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
     - Condition
   * - apiVersion
     - Number
     - An API version of this URL.
       This document is written for version **1**.
     - Always
   * - result
     - Boolean
     - True on success. Otherwise False and the reason is shown in the
       element: message.
     - Always
   * - message
     - String
     - Error message. This key is reply only when result is False.
     - False
   * - numberOfTriggers
     - Number
     - The number of triggers.
     - True
   * - triggers
     - Array
     - The array of `Trigger object`_.
     - True

.. note:: [Condition] Always: always, True: only when result is True, False: only when result is False.

Trigger object
-------------
.. list-table::
   :header-rows: 1

   * - Key
     - Value type
     - Brief
   * - status
     - Number
     - A `Trigger status`_.
   * - severity
     - Number
     - A `Trigger severity`_.
   * - lastChangeTime
     - Number
     - A last change time of the trigger.
   * - serverId
     - Number
     - A server ID.
   * - hostId
     - Number
     - A host ID.
   * - hostName
     - String
     - A host name the trigger is concerned with.
   * - brief
     - String
     - A brief of the trigger.

Trigger status
--------------
.. list-table::

   * - 0
     - TRIGGER_STATUS_OK
   * - 1
     - TRIGGER_STATUS_PROBLEM

Trigger severity
----------------
.. list-table::

   * - 0
     - TRIGGER_SEVERITY_INFO
   * - 1
     - TRIGGER_SEVERITY_WARN
