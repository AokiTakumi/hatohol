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
// HatohoConnector
// ---------------------------------------------------------------------------
var HatoholConnector = function(connectParams) {
  // connectParams has the following paramters.
  //   url: mandatory (e.g. /tunnel/server)
  //   request: GET, POST, PUT, and DELETE (Default: GET)
  //   data: data to send
  //   replyCallback: function(reply, parser)
  //   connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown)
  //     If undefined, a message box is shown.
  //   parseErrorCallback: function(reply, parser)
  //     If undefined, replyCallback is called.
  //   replyParser  : (Default: HatoholReplyParser)
  var self = this;
  if (connectParams.request)
    self.request = connectParams.request;
  else
    self.request = "GET";

  var sessionId = HatoholSessionManager.get();
  if (!sessionId) {
    login();
    return;
  }
  request();

  function login() {
    self.dialog = new HatoholLoginDialog(loginReadyCallback);
  }

  function loginReadyCallback(user, password) {
    $.ajax({
      url: "/tunnel/login?user=" + encodeURI(user)
           + "&password=" + encodeURI(password),
      type: "GET",
      success: function(data) {
        parseLoginResult(data);
      },
      error: connectError,
    });
  }

  function parseLoginResult(data) {
    var parser = new HatoholLoginReplyParser(data);
    if (parser.getStatus() != REPLY_STATUS.OK) {
      var msg = gettext("Failed to login.") + parser.getStatusMessage();
      hatoholErrorMsgBox(msg);
      return;
    }
    var sessionId = parser.getSessionId();
    HatoholSessionManager.set(sessionId);
    self.dialog.closeDialog();
    request();
  }

  function request() {
    var url = "/tunnel" + connectParams.url;
    var hdrs = {};
    hdrs[hatohol.FACE_REST_SESSION_ID_HEADER_NAME] =
       HatoholSessionManager.get();
    $.ajax({
      url: url,
      headers: hdrs,
      type: self.request,
      data: connectParams.data,
      success: function(data) {
        var parser;
        if (connectParams.replyParser)
          parser = new connectParams.replyParser(reply)
        else
          parser = new HatoholReplyParser(data);
        if (parser.getStatus() == REPLY_STATUS.ERROR_CODE_IS_NOT_OK) {
          if (data.errorCode == hatohol.HTERR_NOT_FOUND_SESSION_ID) {
            login();
            return;
          }
          if (connectParams.parseErrorCallback) {
            connectParams.parseErrorCallback(data, parser)
            return;
          }
        }
        connectParams.replyCallback(data, parser);
      },
      error: connectError,
    });
  }

  function connectError(XMLHttpRequest, textStatus, errorThrown) {
    if (connectParams.connectErrorCallback) {
      connectParams.connectErrorCallback(XMLHttpReuest, textStatus, errorThrown);
      return;
    }
    var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                   XMLHttpRequest.statusText;
    showErrorMessageBox(errorMsg);
  }
};
