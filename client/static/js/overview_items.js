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

var OverviewItems = function(userProfile) {
  var self = this;
  var rawData, parsedData;

  self.reloadIntervalSeconds = 60;
  self.baseQuery = {
    limit:       0,
    offset:      0,
    serverId:    "-1",
    hostgroupId: "*",
    hostId:      "*",
  };
  $.extend(self.baseQuery, getItemsQueryInURI());
  self.lastQuery = undefined;
  self.showToggleAutoRefreshButton();
  self.setupToggleAutoRefreshButtonHandler(load, self.reloadIntervalSeconds);

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  self.userConfig = new HatoholUserConfig();
  start();

  function start() {
    self.userConfig.get({
      itemNames:['overview-items-filter-server',
                 'overview-items-filter-host-group',
                 'overview-items-filter-host'],
      successCallback: function(conf) {
        self.baseQuery.serverId =
          self.userConfig.findOrDefault(conf, 'overview-items-filter-server',
                                        self.baseQuery.serverId);

        self.baseQuery.hostgroupId =
          self.userConfig.findOrDefault(conf, 'overview-items-filter-host-group',
                                        self.baseQuery.hostgroupId);

        self.baseQuery.hostId =
          self.userConfig.findOrDefault(conf, 'overview-items-filter-host',
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

    $("#select-severity").change(function() {
      // not implemented yet
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
        $.extend(items, {'overview-items-filter-server': self.baseQuery.serverId});
      }

      val = self.getTargetHostgroupId();
      if (!val)
        val = "*";
      if (self.baseQuery.hostgroupId != val) {
        self.baseQuery.hostgroupId = val;
        $.extend(items, {'overview-items-filter-host-group': self.baseQuery.hostgroupId});
      }

      val = self.getTargetHostId();
      if (!val)
        val = "*";
      if (self.baseQuery.hostId != val) {
        self.baseQuery.hostId = val;
        $.extend(items, {'overview-items-filter-host': self.baseQuery.hostId});
      }

      saveConfig(items);
    });
  }

  function parseData(replyData) {
    var parsedData = {};
    var nickName, hostName, itemName;
    var nickNames, itemNames, hostNames;
    var server, item;
    var x;

    parsedData.hosts  = {};
    parsedData.values = {};

    nickNames = [];
    itemNames = [];
    hostNames = {};

    for (x = 0; x < replyData["items"].length; ++x) {
      item = replyData["items"][x];
      serverId = item["serverId"];
      server = replyData["servers"][serverId];
      nickName   = getNickName(server, serverId);
      hostName   = getHostName(server, item["hostId"]);
      itemName   = item["brief"];

      nickNames.push(nickName);
      itemNames.push(itemName);
      if (!hostNames[nickName])
        hostNames[nickName] = [];
      hostNames[nickName].push(hostName);

      if (!parsedData.values[nickName])
        parsedData.values[nickName] = {};
      if (!parsedData.values[nickName][hostName])
        parsedData.values[nickName][hostName] = {};

      if (!parsedData.values[nickName][hostName][itemName])
        parsedData.values[nickName][hostName][itemName] = item;
    }

    parsedData.items   = itemNames.uniq().sort();
    for (nickName in hostNames)
      parsedData.hosts[nickName] = hostNames[nickName].uniq().sort();

    return parsedData;
  }

  function setupFilterValues(servers, query, withoutSelfMonitor) {
    if (!servers && rawData && rawData.servers)
      servers = rawData.servers;

    if (!query)
      query = self.lastQuery ? self.lastQuery : self.baseQuery;

    self.setupHostFilters(servers, query, withoutSelfMonitor);
  }

  function setLoading(loading) {
    if (loading) {
      $("#select-server").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
    } else {
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
    for (serverName in parsedData.hosts) {
      hostNames = parsedData.hosts[serverName];
      serversRow += "<th style='text-align: center' colspan='" +
        hostNames.length + "'>" + escapeHTML(serverName) + "</th>";
      for (x = 0; x < hostNames.length; ++x) {
        hostName = hostNames[x];
        hostsRow += "<th>" + escapeHTML(hostName) + "</th>";
      }
    }
    hostsRow += "</tr>";
    hostsRow += "</tr>";

    return serversRow + hostsRow;
  }

  function drawTableBody(parsedData) {
    var serverName, hostNames, hostName, itemName, item, html = "";
    var x, y;

    for (y = 0; y < parsedData.items.length; ++y) {
      itemName = parsedData.items[y];
      html += "<tr>";
      html += "<th>" + escapeHTML(itemName) + "</th>";
      for (serverName in parsedData.hosts) {
        hostNames = parsedData.hosts[serverName];
        for (x = 0; x < hostNames.length; ++x) {
          hostName = hostNames[x];
          item = parsedData.values[serverName][hostName][itemName];
          if (item && item["lastValue"] != undefined) {
            html += "<td>" + formatItemLastValue(item) + "</td>";
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

  function updateCore(reply) {
    rawData = reply;
    parsedData = parseData(reply);
    self.setServerFilterCandidates(rawData["servers"]);
    self.setHostgroupFilterCandidates(rawData["servers"]);
    self.setHostFilterCandidates(rawData["servers"]);
    drawTableContents(parsedData);
    setupFilterValues(rawData.servers,
                      self.lastQuery ? self.lastQuery : self.baseQuery,
                      true);
    setLoading(false);
    self.setAutoReload(load, self.reloadIntervalSeconds);
  }

  function getItemsQueryInURI() {
    var knownKeys = [
      "serverId", "hostgroupId", "hostId",
      "limit", "offset",
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
      limit:  self.baseQuery.limit,
      offset: self.baseQuery.offset,
    });
    if (self.lastQuery)
      $.extend(query, self.getHostFilterQuery());
    self.lastQuery = query;
    return 'item?' + $.param(query);
  };

  function load() {
    self.displayUpdateTime();
    self.startConnection(getQuery(), updateCore);
    setLoading(true);
  }
};

OverviewItems.prototype = Object.create(HatoholMonitoringView.prototype);
OverviewItems.prototype.constructor = OverviewItems;
