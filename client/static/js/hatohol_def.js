/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

var TRIGGER_STATUS_OK      = 0
var TRIGGER_STATUS_PROBLEM = 1

var EVENT_TYPE_GOOD = 0
var EVENT_TYPE_BAD  = 1

var TRIGGER_SEVERITY_UNKNOWN   = 0
var TRIGGER_SEVERITY_INFO      = 1
var TRIGGER_SEVERITY_WARNING   = 2
var TRIGGER_SEVERITY_ERROR     = 3
var TRIGGER_SEVERITY_CRITICAL  = 4
var TRIGGER_SEVERITY_EMERGENCY = 5

var ACTION_COMMAND  = 0
var ACTION_RESIDENT = 1

var CMP_INVALID = 0
var CMP_EQ      = 1
var CMP_EQ_GT   = 2

var MONITORING_SYSTEM_ZABBIX = 0
var MONITORING_SYSTEM_NAGIOS = 1

/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

var hatohol = {
  TRIGGER_STATUS_OK: 0,
  TRIGGER_STATUS_PROBLEM: 1,

  EVENT_TYPE_GOOD: 0,
  EVENT_TYPE_BAD: 1,

  TRIGGER_SEVERITY_UNKNOWN: 0,
  TRIGGER_SEVERITY_INFO: 1,
  TRIGGER_SEVERITY_WARNING: 2,
  TRIGGER_SEVERITY_ERROR: 3,
  TRIGGER_SEVERITY_CRITICAL: 4,
  TRIGGER_SEVERITY_EMERGENCY: 5,

  ACTION_USER_DEFINED: -2,
  ACTION_ALL: -1,
  ACTION_COMMAND: 0,
  ACTION_RESIDENT: 1,
  ACTION_INCIDENT_SENDER: 2,

  CMP_INVALID: 0,
  CMP_EQ: 1,
  CMP_EQ_GT: 2,

  MONITORING_SYSTEM_ZABBIX: 0,
  MONITORING_SYSTEM_NAGIOS: 1,
  MONITORING_SYSTEM_HAPI_ZABBIX: 2,
  MONITORING_SYSTEM_HAPI_NAGIOS: 3,
  MONITORING_SYSTEM_HAPI_JSON: 4,
  MONITORING_SYSTEM_HAPI_CEILOMETER: 6,
  MONITORING_SYSTEM_UNKNOWN: -2,

  INCIDENT_TRACKER_UNKNOWN: -2,
  INCIDENT_TRACKER_FAKE: -1,
  INCIDENT_TRACKER_REDMINE: 0,

  HTERR_OK: 0,
  HTERR_UNINITIALIZED: 1,
  HTERR_UNKNOWN_REASON: 2,
  HTERR_NOT_IMPLEMENTED: 3,
  HTERR_GOT_EXCEPTION: 4,
  HTERR_INTERNAL_ERROR: 5,
  HTERR_INVALID_USER: 6,
  HTERR_INVALID_URL: 7,
  HTERR_BAD_REST_RESPONSE: 8,
  HTERR_FAILED_TO_PARSE_JSON_DATA: 9,
  HTERR_NOT_FOUND_TARGET_RECORD: 10,
  HTERR_INVALID_MONITORING_SYSTEM_TYPE: 11,
  HTERR_INVALID_PORT_NUMBER: 12,
  HTERR_INVALID_IP_ADDRESS: 13,
  HTERR_INVALID_HOST_NAME: 14,
  HTERR_NO_IP_ADDRESS_AND_HOST_NAME: 19,
  HTERR_INVALID_INCIDENT_TRACKER_TYPE: 20,
  HTERR_NO_INCIDENT_TRACKER_LOCATION: 21,
  HTERR_EMPTY_USER_NAME: 22,
  HTERR_TOO_LONG_USER_NAME: 23,
  HTERR_INVALID_CHAR: 24,
  HTERR_EMPTY_PASSWORD: 25,
  HTERR_TOO_LONG_PASSWORD: 26,
  HTERR_USER_NAME_EXIST: 27,
  HTERR_NO_PRIVILEGE: 28,
  HTERR_INVALID_PRIVILEGE_FLAGS: 29,
  HTERR_EMPTY_USER_ROLE_NAME: 30,
  HTERR_TOO_LONG_USER_ROLE_NAME: 31,
  HTERR_USER_ROLE_NAME_OR_PRIVILEGE_FLAGS_EXIST: 32,
  HTERR_OFFSET_WITHOUT_LIMIT: 33,
  HTERR_NOT_FOUND_SORT_ORDER: 34,
  HTERR_DELETE_INCOMPLETE: 35,
  HTERR_UNSUPPORTED_FORMAT: 38,
  HTERR_NOT_FOUND_SESSION_ID: 39,
  HTERR_NOT_FOUND_ID_IN_URL: 40,
  HTERR_NOT_FOUND_PARAMETER: 41,
  HTERR_INVALID_PARAMETER: 42,
  HTERR_AUTH_FAILED: 43,
  HTERR_NOT_TEST_MODE: 44,
  HTERR_FAILED_TO_CREATE_DATA_STORE: 45,
  HTERR_FAILED_TO_REGIST_DATA_STORE: 46,
  HTERR_FAILED_TO_STOP_DATA_STORE: 47,
  HTERR_FAILED_TO_SEND_INCIDENT: 48,
  HTERR_FAILED_CONNECT_ZABBIX: 49,
  HTERR_FAILED_CONNECT_MYSQL: 50,
  HTERR_FAILED_CONNECT_BROKER: 51,
  HTERR_FAILED_CONNECT_HAP: 52,
  HTERR_HAP_INTERNAL_ERROR: 53,
  HTERR_ERROR_TEST: 54,
  HTERR_ERROR_TEST_WITHOUT_MESSAGE: 55,

  FACE_REST_API_VERSION: 3,
  FACE_REST_SESSION_ID_HEADER_NAME: 'X-Hatohol-Session',

  DATA_QUERY_OPTION_SORT_DONT_CARE: 0,
  DATA_QUERY_OPTION_SORT_ASCENDING: 1,
  DATA_QUERY_OPTION_SORT_DESCENDING: 2,

  ALL_PRIVILEGES: 8388607,
  NONE_PRIVILEGE: 0,
  OPPRVLG_CREATE_USER: 0,
  OPPRVLG_UPDATE_USER: 1,
  OPPRVLG_DELETE_USER: 2,
  OPPRVLG_GET_ALL_USER: 3,
  OPPRVLG_CREATE_SERVER: 4,
  OPPRVLG_UPDATE_SERVER: 5,
  OPPRVLG_UPDATE_ALL_SERVER: 6,
  OPPRVLG_DELETE_SERVER: 7,
  OPPRVLG_DELETE_ALL_SERVER: 8,
  OPPRVLG_GET_ALL_SERVER: 9,
  OPPRVLG_CREATE_ACTION: 10,
  OPPRVLG_UPDATE_ACTION: 11,
  OPPRVLG_UPDATE_ALL_ACTION: 12,
  OPPRVLG_DELETE_ACTION: 13,
  OPPRVLG_DELETE_ALL_ACTION: 14,
  OPPRVLG_GET_ALL_ACTION: 15,
  OPPRVLG_CREATE_USER_ROLE: 16,
  OPPRVLG_UPDATE_ALL_USER_ROLE: 17,
  OPPRVLG_DELETE_ALL_USER_ROLE: 18,
  OPPRVLG_CREATE_INCIDENT_SETTING: 19,
  OPPRVLG_UPDATE_INCIDENT_SETTING: 20,
  OPPRVLG_DELETE_INCIDENT_SETTING: 21,
  OPPRVLG_GET_ALL_INCIDENT_SETTINGS: 22,
  NUM_OPPRVLG: 23,

  SESSION_ID_LEN: 36,

  ENV_NAME_SESSION_ID: 'HATOHOL_SESSION_ID',

  ARM_WORK_STAT_INIT: 0,
  ARM_WORK_STAT_OK: 1,
  ARM_WORK_STAT_FAILURE: 2,

  errorMessages: {
    0: gettext('OK.'),
    1: gettext('Uninitialized (This is probably a bug).'),
    2: gettext('Unknown reason.'),
    3: gettext('Not implemented.'),
    4: gettext('Got exception.'),
    5: gettext('Internal error happend.'),
    6: gettext('Invalid user.'),
    7: gettext('Invalid URL.'),
    8: gettext('Invalid URL.'),
    9: gettext('Failed to parse JSON data.'),
    10: gettext('Not found target record.'),
    11: gettext('Invalid monitoring system type.'),
    12: gettext('Invalid port number.'),
    13: gettext('Invalid IP address.'),
    14: gettext('Invalid host name.'),
    19: gettext('No IP address and host name.'),
    20: gettext('Invalid incident tracker type.'),
    21: gettext('NO incident tracker location.'),
    22: gettext('Empty user name.'),
    23: gettext('Too long user name.'),
    24: gettext('Invalid character.'),
    25: gettext('Password is empty.'),
    26: gettext('Too long password.'),
    27: gettext('The same user name already exists.'),
    28: gettext('No privilege.'),
    29: gettext('Invalid privilege flags.'),
    30: gettext('Empty user role name.'),
    31: gettext('Too long User role name.'),
    32: gettext('The same user role name or a user role with the same privilege already exists.'),
    33: gettext('An offset value is specified but no limit value is specified.'),
    34: gettext('Not found sort order.'),
    35: gettext('The delete operation was incomplete.'),
    38: gettext('Unsupported format.'),
    39: gettext('Not found session ID.'),
    40: gettext('Not found ID in the URL.'),
    41: gettext('Not found parameter.'),
    42: gettext('Invalid parameter.'),
    43: gettext('Authentication failed.'),
    44: gettext('Not test mode.'),
    45: gettext('Failed to create a DataStore object.'),
    46: gettext('Failed to regist a DataStore object.'),
    47: gettext('Failed to stop a DataStore object.'),
    48: gettext('Failed to send an incident to an incident tracker.'),
    49: gettext('Failed in connecting to Zabbix.'),
    50: gettext('Failed in connecting to MySQL.'),
    51: gettext('Failed in connecting to Broker.'),
    52: gettext('Failed in connecting to ArmPlugin.'),
    53: gettext('Internal error happend in ArmPlugin.'),
    54: gettext('Error test.'),
  },
};

