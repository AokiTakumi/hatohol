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


var HatoholHostgroupPrivilegeEditDialog = function(userId, serverId, applyCallback) {
  var self = this;
  self.mainTableId = "HostgroupPrivilegeEditDialogMainTable";
  self.userId = userId;
  self.serverId = serverId;
  self.applyCallback = applyCallback;
  self.hostgroupData = null;
  self.loadError = false;

  var dialogButtons = [{
    text: gettext("APPLY"),
    click: function() { self.applyButtonClicked(); }
  }, {
    text: gettext("CANCEL"),
    click: function() { self.cancelButtonClicked(); }
  }];

  // call the constructor of the super class
  var dialogAttrs = { width: "600" };
  HatoholDialog.apply(
    this, ["hostgroup-privilege-edit-dialog", gettext("Edit Hostgroup privilege"),
           dialogButtons, dialogAttrs]);
  self.start();
};

HatoholHostgroupPrivilegeEditDialog.prototype =
  Object.create(HatoholDialog.prototype);
HatoholHostgroupPrivilegeEditDialog.prototype.constructor = HatoholHostgroupPrivilegeEditDialog;

HatoholHostgroupPrivilegeEditDialog.prototype.createMainElement = function() {
  var ptag = $("<p/>");
  ptag.attr("id", "hostgroupPrivilegeEditDialogMsgArea");
  ptag.text(gettext("Now getting information..."));
  return ptag;
};

HatoholHostgroupPrivilegeEditDialog.prototype.applyButtonClicked = function() {
  this.applyPrivileges();
  if (this.applyCallback)
    this.applyCallback();
};

HatoholHostgroupPrivilegeEditDialog.prototype.cancelButtonClicked = function() {
  this.closeDialog();
};

HatoholHostgroupPrivilegeEditDialog.prototype.setMessage = function(msg) {
  $("#hostgroupPrivilegeEditDialogMsgArea").text(msg);
};

HatoholHostgroupPrivilegeEditDialog.prototype.start = function() {
  var self = this;
  self.loadError = false;

  loadHostgroups();

  function makeQueryData() {
    var queryData = {};
    queryData.serverId = self.serverId;
    return queryData;
  }

  function loadHostgroups() {
    new HatoholConnector({
      url: "/hostgroup",
      request: "GET",
      data: makeQueryData(),
      replyCallback: function(hostgroupData, parser) {
        if (self.loadError)
          return;
        if (!hostgroupData.numberOfHostgroups) {
          self.setMessage(gettext("No data."));
          return;
        }
        self.hostgroupData = hostgroupData;
        self.updateHostgroupTable();
        loadAccessInfo();
      },
      parseErrorCallback: function(reply, parser) {
        if (self.loadError)
          return;
        self.setMessage(parser.getMessage());
        self.loadError = true;
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                       XMLHttpRequest.statusText;
        self.setMessage(errorMsg);
        self.erorr = true;
      }
    });
  }

  function loadAccessInfo() {
    new HatoholConnector({
      url: "/user/" + self.userId + "/access-info",
      request: "GET",
      data: {},
      replyCallback: function(reply, parser) {
        if (self.loadError)
          return;
        self.allowedServers = reply.allowedServers;
        self.updateAllowCheckboxes();
      },
      parseErrorCallback: function(reply, parser) {
        self.setMessage(parser.getMessage());
        self.loadError = true;
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
                       XMLHttpRequest.statusText;
        self.setMessage(errorMsg);
        self.erorr = true;
      }
    });
  }
};

HatoholHostgroupPrivilegeEditDialog.prototype.updateHostgroupTable = function() {
  if (this.loadError)
        return;
  if (!this.hostgroupData)
    return;

  var table = this.generateMainTable();
  var rows = this.generateTableRows(this.serversData);
  this.replaceMainElement(table);
  $("#" + this.mainTableId + " tbody").append(rows);
  this.updateAllowCheckboxes();
};

HatoholHostgroupPrivilegeEditDialog.prototype.generateMainTable = function() {
  var html =
  '<table class="table table-condensed table-striped table-hover" id=' +
  this.mainTableId + '>' +
  '  <thead>' +
  '    <tr>' +
  '      <th>' + gettext("Allow") + '</th>' +
  '      <th>' + gettext("Hostgroup ID") + '</th>' +
  '      <th>' + gettext("Hostgroup Name") + '</th>' +
  '    </tr>' +
  '  </thead>' +
  '  <tbody></tbody>' +
  '</table>';
  return html;
};

HatoholHostgroupPrivilegeEditDialog.prototype.generateTableRows = function() {
  var s = '';
  var hostgroup = this.hostgroupData.hostgroups;
  for (var i = 0; i < hostgroup.length; i++) {
    hstgrp = hostgroup[i];
    s += '<tr>';
    s += '<td><input type="checkbox" class="hostgroupSelectCheckbox" ' +
               'hostgroupId="' + escapeHTML(hstgrp.groupId) + '"></td>';
    s += '<td>' + escapeHTML(hstgrp.groupId) + '</td>';
    s += '<td>' + escapeHTML(hstgrp.groupName)  + '</td>';
    s += '</tr>';
  }
  return s;
};

HatoholHostgroupPrivilegeEditDialog.prototype.updateAllowCheckboxes = function() {
  if (!this.hostgroupData || !this.allowedServers)
    return;

  var i, checkboxes = $(".hostgroupSelectCheckbox");
  var allowedHostgroup = this.allowedServers[1]["allowedHostGroups"];
  var isAllowedHostgroup = function (accessInfo, hostgroupId) {
    for (var i = 0; i < allowedHostgroup.numberOfAllowedHostgroups; i++) {
      if (allowedHostgroup[i] == hostgroupId)
        return true;
    }
    return false;
  }

  for (i = 0; i < checkboxes.length; i++) {
    hostgroupId = checkboxes[i].getAttribute("hostgroupId");
    if (isAllowedHostgroup(allowedHostgroup, hostgroupId))
      checkboxes[i].checked = true;
  }
};

// TODO: Should this dialog post access-info ?
HatoholHostgroupPrivilegeEditDialog.prototype.addAccessInfo = function(accessInfo) {
  var self = this;
  var userId = this.userId;
  new HatoholConnector({
    url: "/user/" + userId + "/access-info",
    request: "POST",
    data: accessInfo,
    replyCallback: function(reply, parser) {
      self.applyResult.numSucceeded += 1;
      self.checkApplyResult();
    },
    parseErrorCallback: function(reply, parser) {
      self.applyResult.numFailed += 1;
      self.checkApplyResult();
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      self.applyResult.numFailed += 1;
      self.checkApplyResult();
    }
  });
};

HatoholHostgroupPrivilegeEditDialog.prototype.deleteAccessInfo = function(accessInfoId) {
  var self = this;
  var userId = this.userId;
  new HatoholConnector({
    url: "/user/" + userId + "/access-info/" + accessInfoId,
    request: "DELETE",
    replyCallback: function(reply, parser) {
      self.applyResult.numSucceeded += 1;
      self.checkApplyResult();
    },
    parseErrorCallback: function(reply, parser) {
      self.applyResult.numFailed += 1;
      self.checkApplyResult();
    },
    connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
      self.applyResult.numFailed += 1;
      self.checkApplyResult();
    }
  });
};

HatoholHostgroupPrivilegeEditDialog.prototype.checkApplyResult = function(accessInfo) {
  var result = this.applyResult;
  var numCompleted = result.numSucceeded + result.numFailed;

  hatoholInfoMsgBox(gettext("Aplpying...") + " " +
                    numCompleted + " / " + result.numServers);

  if (numCompleted < result.numServers)
    return;

  // completed
  if (result.numFailed > 0) {
    hatoholErrorMsgBox(gettext("Failed to apply."));
  } else {
    hatoholInfoMsgBox(gettext("Succeeded to apply."));
    this.closeDialog();
  }
};

HatoholHostgroupPrivilegeEditDialog.prototype.applyPrivileges = function() {
  var self = this;
  var i, serverId, accessInfoId;
  var checkboxes = $(".hostgroupSelectCheckbox");
  var getAccessInfoId = function(serverId) {
    var id, allowedHostGroups, allowedHostGroup;
    var ALL_HOST_GROUPS = -1;
    if (self.allowedServers && self.allowedServers[serverId])
      allowedHostGroups = self.allowedServers[serverId]["allowedHostGroups"];
    if (allowedHostGroups)
      allowedHostGroup = allowedHostGroups[ALL_HOST_GROUPS];
    if (allowedHostGroup)
      id = allowedHostGroup["accessInfoId"];
    return id;
  };

  self.applyResult = {
    numServers:   checkboxes.length,
    numSucceeded: 0,
    numFailed:    0
  };
  for (i = 0; i < checkboxes.length; i++) {
    hostgroupId = checkboxes[i].getAttribute("hostgroupId");
    accessInfoId = getAccessInfoId(serverId);

    if (checkboxes[i].checked) {
      if (!accessInfoId)
        this.addAccessInfo({ serverId: this.serverId, hostGroupId: hostgroupId });
      else
        self.applyResult.numSucceeded += 1;
    } else {
      if (accessInfoId)
        this.deleteAccessInfo(accessInfoId);
      else
        self.applyResult.numSucceeded += 1;
    }
  }
  self.checkApplyResult();
};
