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
// HatoholDeleter
// ---------------------------------------------------------------------------
var HatoholDeleter = function(deleteParameters) {
  //
  // deleteParameters has following parameters.
  //
  // * id
  // * type
  // TODO: Add the description.
  //
  var count = 0;
  var total = 0;
  var errors = 0;
  for (var i = 0; i < deleteParameters.id.length; i++) {
    count++;
    total++;
    new HatoholConnector({
      url: '/' + deleteParameters.type + '/' + deleteParameters.id[i],
        request: "DELETE",
        context: deleteParameters.id[i],
        replyCallback: function(data, parser, context) {
          parseDeleteResult(data);
        },
        connectErrorCallback: function(XMLHttpRequest,
                                textStatus, errorThrown) {
          var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
          XMLHttpRequest.statusText;
          hatoholErrorMsgBox(errorMsg);
          deleteParameters.deleteIdArray.errors++;
        },
        completionCallback: function(context) {
          compleOneDel();
        }
    });
  }

  function checkParseResult(data) {
    var msg;
    var malformed =false;
    if (data.result == undefined)
      malformed = true;
    if (!malformed && !data.result && data.message == undefined)
      malformed = true;
    if (malformed) {
      msg = "The returned content is malformed: " +
        "Not found 'result' or 'message'.\n" +
        JSON.stringify(data);
      hatoholErrorMsgBox(msg);
      return false;
    }
    if (!data.result) {
      msg = "Failed:\n" + data.message;
      hatoholErrorMsgBox(msg);
      return false;
    }
    if (data.id == undefined) {
      msg = "The returned content is malformed: " +
        "'result' is true, however, 'id' missing.\n" +
        JSON.stringfy(data);
      hatoholErrorMsgBox(msg);
      return false;
    }
    return true;
  }

  function parseDeleteResult(data) {
    if (!checkParseResult(data))
      return;
    if (!(data.id in id)) {
      alert("Fatal Error: You should reload page.\nID: " + data.id +
          " is not in deleteIdArray: " + id);
      errors++;
      return;
    }
    delete id[data.id];
  }

  function compleOneDel() {
    count--;
    var completed = total - count;
    hatoholErrorMsgBox(gettext("Deleting...") + " " + completed +
                       " / " + total);
    if (count > 0)
      return;

    // close dialogs
    hatoholInfoMsgBox(gettext("Completed. (Number of errors: ") +
                      errors + ")");

    // update the main view
    startConnection(deleteParameters.type, updateCore);
  }
}
