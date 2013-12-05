/*
 * Copyright (C) 2013 Project Hatohol
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

// ---------------------------------------------------------------------------
// HatohoReplyParser
// ---------------------------------------------------------------------------
var REPLY_STATUS = {
  OK: 0,
  NULL_OR_UNDEFINED:        1,
  NOT_FOUND_API_VERSION:    2,
  UNSUPPORTED_API_VERSION:  3,
  NOT_FOUND_ERROR_CODE:     4,
  ERROR_CODE_IS_NOT_OK:     5,
};

var HatoholReplyParser = function(reply) {

  this.stat = REPLY_STATUS.OK;
  this.optionMessage = "";

  if (!reply) {
    this.stat = REPLY_STATUS.NULL_OR_UNDEFINED;
    return;
  }
  
  // API version
  if (!("apiVersion" in reply)) {
    this.stat = REPLY_STATUS.NOT_FOUND_API_VERSION;
    return;
  }
  if (reply.apiVersion != hatohol.FACE_REST_API_VERSION) {
    this.stat = REPLY_STATUS.UNSUPPORTED_API_VERSION;
    return;
  }

  // error code
  if (!("errorCode" in reply)) {
    this.stat = REPLY_STATUS.NOT_FOUND_ERROR_CODE;
    return;
  }
  this.errorCode = reply.errorCode;
  if (reply.errorCode != hatohol.HTERR_OK) {
    this.stat = REPLY_STATUS.ERROR_CODE_IS_NOT_OK;
    return;
  }

  if ("optionMessage" in reply)
    this.optionMessage = reply.optionMessage;
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = HatoholReplyParser;
  module.exports.REPLY_STATUS = REPLY_STATUS;
  var hatohol = require("../../static/js/hatohol_def");
}

HatoholReplyParser.prototype.getStatus = function() {
  return this.stat;
}

HatoholReplyParser.prototype.getErrorCode = function() {
  return this.errorCode;
}

HatoholReplyParser.prototype.getStatusMessage = function() {
  switch (this.stat) {
  case REPLY_STATUS.OK:
    return gettext("OK.");
  case REPLY_STATUS.NULL_OR_UNDEFINED:
    return gettext("Null or undefined.");
  case REPLY_STATUS.NOT_FOUND_RESULT:
    return gettext("Not found: result.");
  case REPLY_STATUS.RESULT_IS_FALSE:
    return gettext("Result is false: ") + this.errorMessage;
  case REPLY_STATUS.RESULT_IS_FALSE_BUT_NOT_FOUND_MSG:
    return gettext("Result is false, but message is not found.");
  case REPLY_STATUS.NOT_FOUND_SESSION_ID:
    return gettext("Not found: sessionId.");
  }
  return gettext("Unknown status: ") + this.stat;
}

HatoholReplyParser.prototype.getErrorMessage = function() {
  switch (this.errorCode) {
  case hatohol.HTERR_AUTH_FAILED:
    return gettext("Invalid user name or password.");
  }
  return gettext("Unknown error: ") + this.errorCode;
}

HatoholReplyParser.prototype.getMessage = function() {
  if (this.stat == REPLY_STATUS.ERROR_CODE_IS_NOT_OK)
    return this.getErrorMessage();
  else
    return this.getStatusMessage();
}

// ---------------------------------------------------------------------------
// HatoholLoginReplyParser
// ---------------------------------------------------------------------------
var HatoholLoginReplyParser = function(reply) {
  HatoholReplyParser.apply(this, [reply]);
  if (this.getStatus() != REPLY_STATUS.OK)
    return;
  if (!("sessionId" in reply))
    this.stat = REPLY_STATUS.NOT_FOUND_SESSION_ID;
  this.sessionId = reply.sessionId
}

HatoholLoginReplyParser.prototype = Object.create(HatoholReplyParser.prototype);
HatoholLoginReplyParser.prototype.constructor = HatoholLoginReplyParser;

HatoholLoginReplyParser.prototype.getSessionId = function() {
  return this.sessionId;
}

