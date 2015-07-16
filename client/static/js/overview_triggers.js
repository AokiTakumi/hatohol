/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

var OverviewTriggers = function(userProfile) {
  var self = this;
  var rawData, parsedData;

  self.reloadIntervalSeconds = 60;
  self.baseQuery = {
    limit:           0,
    offset:          0,
    minimumSeverity: "0",
    status:          "-1",
    serverId:        "-1",
    hostgroupId:     "*",
    hostId:          "*",
  };
  $.extend(self.baseQuery, getTriggersQueryInURI());
  self.lastQuery = undefined;
  self.showToggleAutoRefreshButton();
  self.setupToggleAutoRefreshButtonHandler(load, self.reloadIntervalSeconds);

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  self.userConfig = new HatoholUserConfig();
  start();

  function start() {
    self.userConfig.get({
      itemNames:['overview-triggers-filter-minimum-severity',
                 'overview-triggers-filter-status',
                 'overview-triggers-filter-server',
                 'overview-triggers-filter-host-group',
                 'overview-triggers-filter-host'],
      successCallback: function(conf) {
        self.baseQuery.minimumSeverity =
          self.userConfig.findOrDefault(conf, 'overview-triggers-filter-minimum-severity',
                                        self.baseQuery.minimumSeverity);

        self.baseQuery.status =
          self.userConfig.findOrDefault(conf, 'overview-triggers-filter-status',
                                        self.baseQuery.status);

        self.baseQuery.serverId =
          self.userConfig.findOrDefault(conf, 'overview-triggers-filter-server',
                                        self.baseQuery.serverId);

        self.baseQuery.hostgroupId =
          self.userConfig.findOrDefault(conf, 'overview-triggers-filter-host-group',
                                        self.baseQuery.hostgroupId);

        self.baseQuery.hostId =
          self.userConfig.findOrDefault(conf, 'overview-triggers-filter-host',
                                        self.baseQuery.hostId);

        setupFilterValues();
        setupCallbacks();
        load();
      },
      connectErrorCallback: function(XMLHttpRequest) {
        showXHRError(XMLHttpRequest);
      },
    });
  }

  function showXHRError(XMLHttpRequest) {
    var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
      XMLHttpRequest.statusText;
    hatoholErrorMsgBox(errorMsg);
  }

  function saveConfig(items) {
    self.userConfig.store({
      items: items,
      successCallback: function() {
        // we just ignore it
      },
      connectErrorCallback: function(XMLHttpRequest) {
        showXHRError(XMLHttpRequest);
      },
    });
  }

  function setupCallbacks() {
    self.setupHostQuerySelectorCallback(
      load, '#select-server', '#select-host-group', '#select-host');

    $('#select-severity').change(function() {
      var val = $('#select-severity').val();
      if (self.baseQuery.minimumSeverity != val) {
        self.baseQuery.minimumSeverity = val;
        saveConfig({'overview-triggers-filter-minimum-severity': self.baseQuery.minimumSeverity});
      }
      load();
    });

    $('#select-status').change(function() {
      var val = $('#select-status').val();
      if (self.baseQuery.status != val) {
        self.baseQuery.status = val;
        saveConfig({'overview-triggers-filter-status': self.baseQuery.status});
      }
      load();
    });

    $("#select-server, #select-host-group, #select-host").change(function() {
      var val = "", items = {};

      val = self.getTargetServerId();
      if (!val) {
        val = "-1";
      }
      if (self.baseQuery.serverId != val) {
        self.baseQuery.serverId = val;
        $.extend(items, {'overview-triggers-filter-server': self.baseQuery.serverId});
      }

      val = self.getTargetHostgroupId();
      if (!val)
        val = "*";
      if (self.baseQuery.hostgroupId != val) {
        self.baseQuery.hostgroupId = val;
        $.extend(items, {'overview-triggers-filter-host-group': self.baseQuery.hostgroupId});
      }

      val = self.getTargetHostId();
      if (!val)
        val = "*";
      if (self.baseQuery.hostId != val) {
        self.baseQuery.hostId = val;
        $.extend(items, {'overview-triggers-filter-host': self.baseQuery.hostId});
      }

      saveConfig(items);
    });
  }

  function parseData(replyData, minimum) {
    var parsedData = {};
    var nickName, hostName, triggerName;
    var nickNames, triggerNames, hostNames;
    var server, trigger;
    var x;
    var targetSeverity = $("#select-severity").val();
    var targetStatus = $("#select-status").val();

    parsedData.hosts  = {};
    parsedData.values = {};

    nickNames = [];
    triggerNames = [];
    hostNames = {};

    for (x = 0; x < replyData["triggers"].length; ++x) {
      trigger = replyData["triggers"][x];
      if (trigger["severity"] < targetSeverity)
        continue;
      if (targetStatus >= 0 && trigger["status"] != targetStatus)
        continue;

      var serverId = trigger["serverId"];
      var hostId   = trigger["hostId"];
      server      = replyData["servers"][serverId];
      nickName    = getNickName(server, serverId);
      hostName    = getHostName(server, hostId);
      triggerName = trigger["brief"];

      nickNames.push(nickName);
      triggerNames.push(triggerName);
      if (!hostNames[nickName])
        hostNames[nickName] = [];
      hostNames[nickName].push(hostName);

      if (!parsedData.values[nickName])
        parsedData.values[nickName] = {};
      if (!parsedData.values[nickName][hostName])
        parsedData.values[nickName][hostName] = {};

      if (!parsedData.values[nickName][hostName][triggerName])
        parsedData.values[nickName][hostName][triggerName] = trigger;
   }

    parsedData.nicknames  = nickNames.uniq().sort();
    parsedData.triggers = triggerNames.uniq().sort();
    for (nickName in hostNames)
      parsedData.hosts[nickName] = hostNames[nickName].uniq().sort();

    return parsedData;
  }

  function setupFilterValues(servers, query) {
    if (!servers && rawData && rawData.servers)
      servers = rawData.servers;

    if (!query)
      query = self.lastQuery ? self.lastQuery : self.baseQuery;

    self.setupHostFilters(servers, query);

    if ("minimumSeverity" in query)
      $("#select-severity").val(query.minimumSeverity);
    if ("status" in query)
      $("#select-status").val(query.status);
  }

  function setLoading(loading) {
    if (loading) {
      $("#select-severity").attr("disabled", "disabled");
      $("#select-status").attr("disabled", "disabled");
      $("#select-server").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
    } else {
      $("#select-severity").removeAttr("disabled");
      $("#select-status").removeAttr("disabled");
      $("#select-server").removeAttr("disabled");
      if ($("#select-host option").length > 1)
        $("#select-host").removeAttr("disabled");
    }
  }

  function drawTableHeader(parsedData) {
    var serverName, hostNames, hostName;
    var x, serversRow, hostsRow;

    serversRow = "<tr><th></th>";
    hostsRow = "<tr><th></th>";
    for (nickName in parsedData.hosts) {
      hostNames = parsedData.hosts[nickName];
      serversRow += "<th style='text-align: center' colspan='" +
        hostNames.length + "'>" + escapeHTML(nickName) + "</th>";
      for (x = 0; x < hostNames.length; ++x) {
        hostName  = hostNames[x];
        hostsRow += "<th>" + escapeHTML(hostName) + "</th>";
      }
    }
    serversRow += "</tr>";
    hostsRow += "</tr>";

    return serversRow + hostsRow;
  }

  function drawTableBody(parsedData) {
    var triggerName, serverName, hostNames, hostName, trigger, html;
    var x, y;

    html = "";
    for (y = 0; y < parsedData.triggers.length; ++y) {
      triggerName = parsedData.triggers[y];
      html += "<tr>";
      html += "<th>" + escapeHTML(triggerName) + "</th>";
      for (serverName in parsedData.hosts) {
        hostNames = parsedData.hosts[serverName];
        for (x = 0; x < hostNames.length; ++x) {
          hostName  = hostNames[x];
          trigger = parsedData.values[serverName][hostName][triggerName];
          if (trigger) {
            switch (trigger["status"]) {
            case 1:
              html += "<td class='severity" +
                escapeHTML(trigger["severity"]) + "'>&nbsp;</td>";
              break;
            case 0:
              html += "<td class='healthy'>&nbsp;</td>";
              break;
            default:
              html += "<td class='unknown'>&nbsp;</td>";
              break;
            }
          } else {
            html += "<td>&nbsp;</td>";
          }
        }
      }
      html += "</tr>";
    }

    return html;
  }

  function drawTableContents(data) {
    $("#table thead").empty();
    $("#table thead").append(drawTableHeader(data));
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(data));
  }

  function updateCore(reply, param) {
    rawData = reply;
    parsedData = parseData(rawData, param);
    self.setServerFilterCandidates(rawData["servers"]);
    self.setHostgroupFilterCandidates(rawData["servers"]);
    self.setHostFilterCandidates(rawData["servers"]);
    drawTableContents(parsedData);
    setupFilterValues();
    setLoading(false);
    self.setAutoReload(load, self.reloadIntervalSeconds);
  }

  function getTriggersQueryInURI() {
    var knownKeys = [
      "serverId", "hostgroupId", "hostId",
      "limit", "offset",
      "minimumSeverity", "status",
    ];
    var i, allParams = deparam(), query = {};
    for (i = 0; i < knownKeys.length; i++) {
      if (knownKeys[i] in allParams)
        query[knownKeys[i]] = allParams[knownKeys[i]];
    }
    return query;
  }

  function getQuery() {
    var query = $.extend({}, self.baseQuery, {
      minimumSeverity: $("#select-severity").val(),
      status:          $("#select-status").val(),
      limit:           self.baseQuery.limit,
      offset:          self.baseQuery.offset,
    });
    if (self.lastQuery)
      $.extend(query, self.getHostFilterQuery());
    self.lastQuery = query;
    return 'trigger?' + $.param(query);
  };

  function load() {
    self.displayUpdateTime();
    self.startConnection(getQuery(), updateCore);
    setLoading(true);
  }
};

OverviewTriggers.prototype = Object.create(HatoholMonitoringView.prototype);
OverviewTriggers.prototype.constructor = OverviewTriggers;
