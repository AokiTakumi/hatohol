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

var IssueSendersView = function(userProfile) {
  //
  // Variables
  //
  var self = this;

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  //
  // main code
  //
  load();

  //
  // Main view
  //
  $("#table").stupidtable();
  $("#table").bind('aftertablesort', function(event, data) {
    var th = $(this).find("th");
    th.find("i.sort").remove();
    var icon = data.direction === "asc" ? "up" : "down";
    th.eq(data.column).append("<i class='sort glyphicon glyphicon-arrow-" + icon +"'></i>");
  });

  $("#add-issue-sender-button").click(function() {
    new HatoholAddActionDialog(addSucceededCb, true);
  });

  $("#delete-issue-sender-button").click(function() {
    var msg = gettext("Do you delete the selected items ?");
    hatoholNoYesMsgBox(msg, deleteActions);
  });

  $("#edit-issue-trackers-button").click(function() {
    new HatoholIssueTrackersEditor();
  });

  function addSucceededCb() {
    load();
  }

  //
  // Commonly used functions from a dialog.
  //
  function parseResult(data) {
    var msg;
    var malformed = false;
    if (data.result == undefined)
      malformed = true;
    if (!malformed && !data.result && data.message == undefined)
      malformed = true;
    if (malformed) {
      msg = "The returned content is malformed: " +
        "Not found 'result' or 'message'.\n" + JSON.stringify(data);
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
        "'result' is true, however, 'id' is missing.\n" +
        JSON.stringify(data);
      hatoholErrorMsgBox(msg);
      return false;
    }
    return true;
  }

  //
  // delete-action dialog
  //
  function deleteActions() {
    $(this).dialog("close");
    var checkbox = $(".selectcheckbox");
    var deletedIdArray = {count:0, total:0, errors:0};
    for (var i = 0; i < checkbox.length; i++) {
      if (!checkbox[i].checked)
        continue;
      var actionId = checkbox[i].getAttribute("actionId");
      deletedIdArray[actionId] = true;
      deletedIdArray.count++;
      deletedIdArray.total++;
      deleteOneAction(actionId, deletedIdArray);
    }
    hatoholInfoMsgBox(gettext("Deleting..."));
  }

  function deleteOneAction(id, deletedIdArray) {
    new HatoholConnector({
      url: '/action/' + id,
      request: "DELETE",
      context: deletedIdArray,
      replyCallback: function(data, parser, context) {
        parseDeleteActionResult(data, context);
      },
      connectErrorCallback: function(XMLHttpRequest,
                                     textStatus, errorThrown) {
        var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
          XMLHttpRequest.statusText;
        hatoholErrorMsgBox(errorMsg);
        deletedIdArray.errors++;
      },
      completionCallback: function(context) {
        completeOneDelAction(context);
      },
    });
  }

  function parseDeleteActionResult(data, deletedIdArray) {
    if (!parseResult(data))
      return;
    if (!(data.id in deletedIdArray)) {
      alert("Fatal Error: You should reload page.\nID: " + data.id +
            " is not in deletedIdArray: " + deletedIdArray);
      deletedIdArray.errors++;
      return;
    }
    delete deletedIdArray[data.id];
  }

  function completeOneDelAction(deletedIdArray) {
    deletedIdArray.count--;
    var completed = deletedIdArray.total - deletedIdArray.count;
    hatoholErrorMsgBox(gettext("Deleting...") + " " + completed +
                       " / " + deletedIdArray.total);
    if (deletedIdArray.count > 0)
      return;

    // close dialogs
    hatoholInfoMsgBox(gettext("Completed. (Number of errors: ") +
                      deletedIdArray.errors + ")");

    // update the main view
    load();
  }

  //
  // Functions for make the main view.
  //
  function makeNamelessServerLabel(serverId) {
    return "(ID:" + serverId + ")";
  }

  function makeNamelessHostgroupLabel(serverId, hostgroupId) {
    return "(S" + serverId + "-G" + hostgroupId + ")";
  }

  function makeNamelessTriggerLabel(triggerId) {
    return "(T" + triggerId + ")";
  }

  function makeSeverityCompTypeLabel(compType) {
    switch(compType) {
    case hatohol.CMP_EQ:
      return "=";
    case hatohol.CMP_EQ_GT:
      return ">=";
    default:
      return "INVALID: " + compType;
    }
  }

  //
  // parser of received json data
  //
  function getServerName(actionsPkt, actionDef) {
    var serverId = actionDef["serverId"];
    if (!serverId)
      return null;
    var server = actionsPkt["servers"][serverId];
    if (!server)
      return makeNamelessServerLabel(serverId);
    var serverName = actionsPkt["servers"][serverId]["name"];
    if (!serverName)
      return makeNamelessServerLabel(serverId);
    return serverName;
  }

  function getHostgroupName(actionsPkt, actionDef) {
    var hostgroupId = actionDef["hostgroupId"];
    if (!hostgroupId)
      return null;
    var serverId = actionDef["serverId"];
    if (!serverId)
      return makeNamelessHostgroupLabel(serverId, hostgroupId);
    var server = actionsPkt["servers"][serverId];
    if (!server)
      return null;
    var hostgroupArray = server["groups"];
    if (!hostgroupArray)
      return makeNamelessHostgroupLabel(serverId, hostgroupId);
    var hostgroup = hostgroupArray[hostgroupId];
    if (!hostgroup)
      return makeNamelessHostgroupLabel(serverId, hostgroupId);
    var hostgroupName = hostgroup["name"];
    if (!hostgroupName)
      return makeNamelessHostgroupLabel(serverId, hostgroupId);
    return hostgroupName;
  }

  function getTriggerBrief(actionsPkt, actionDef) {
    var triggerId = actionDef["triggerId"];
    if (!triggerId)
      return null;
    var serverId = actionDef["serverId"];
    if (!serverId)
      return makeNamelessTriggerLabel(triggerId);
    var server = actionsPkt["servers"][serverId];
    if (!server)
      return null;
    var triggerArray = server["triggers"];
    if (!triggerArray)
      return makeNamelessTriggerLabel(triggerId);
    var trigger = triggerArray[triggerId];
    if (!trigger)
      return makeNamelessTriggerLabel(triggerId);
    var triggerBrief = trigger["brief"];
    if (!triggerBrief)
      return makeNamelessTriggerLabel(triggerId);
    return triggerBrief;
  }

  //
  // callback function from the base template
  //
  function drawTableBody(actionsPkt) {
    var x;
    var klass, server, host;

    var s = "";
    for (x = 0; x < actionsPkt["actions"].length; ++x) {
      var actionDef = actionsPkt["actions"][x];
      s += "<tr>";
      s += "<td class='delete-selector'>";
      s += "<input type='checkbox' class='selectcheckbox' " +
        "actionId='" + escapeHTML(actionDef.actionId) + "'></td>";
      s += "<td>" + escapeHTML(actionDef.actionId) + "</td>";

      var serverName = getServerName(actionsPkt, actionDef);
      if (!serverName)
        serverName = "ANY";
      s += "<td>" + escapeHTML(serverName) + "</td>";

      var hostgroupName = getHostgroupName(actionsPkt, actionDef);
      if (!hostgroupName)
        hostgroupName = "ANY";
      s += "<td>" + escapeHTML(hostgroupName) + "</td>";

      var triggerBrief = getTriggerBrief(actionsPkt, actionDef);
      if (!triggerBrief)
        triggerBrief = "ANY";
      s += "<td>" + escapeHTML(triggerBrief) + "</td>";

      var triggerStatus = actionDef.triggerStatus;
      var triggerStatusLabel = "ANY";
      if (triggerStatus != undefined)
        triggerStatusLabel = makeTriggerStatusLabel(triggerStatus);
      s += "<td>" + triggerStatusLabel + "</td>";

      var triggerSeverity = actionDef.triggerSeverity;
      var severityLabel = "ANY";
      if (triggerSeverity != undefined)
        severityLabel = makeSeverityLabel(triggerSeverity);

      var severityCompType = actionDef.triggerSeverityComparatorType;
      var severityCompLabel = "";
      if (triggerSeverity != undefined)
        severityCompLabel = makeSeverityCompTypeLabel(severityCompType);

      s += "<td>" + severityCompLabel + " " + severityLabel + "</td>";

      var command = actionDef.command;
      s += "<td>" + escapeHTML(command) + "</td>";

      s += "</tr>";
    }

    return s;
  }

  function updateCore(rawData) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(rawData));
    self.setupCheckboxForDelete($("#delete-issue-sender-button"));
  }

  function getQuery() {
    var query = {
      type: hatohol.ACTION_ISSUE_SENDER,
    };
    return 'action?' + $.param(query);
  };

  function load() {
    self.startConnection(getQuery(), updateCore);
  }
};

IssueSendersView.prototype = Object.create(HatoholMonitoringView.prototype);
IssueSendersView.prototype.constructor = IssueSendersView;
