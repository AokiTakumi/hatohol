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

var LatestView = function(userProfile) {
  var self = this;
  var rawData, parsedData;

  self.reloadIntervalSeconds = 60;
  self.currentPage = 0;
  self.baseQuery = {
    limit:       50,
    offset:      0,
    serverId:    "-1",
    hostgroupId: "*",
    hostId:      "*",
    appName:     "",
  };
  $.extend(self.baseQuery, getItemsQueryInURI());
  self.lastQuery = undefined;
  self.showToggleAutoRefreshButton();
  self.setupToggleAutoRefreshButtonHandler(load, self.reloadIntervalSeconds);

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  self.pager = new HatoholPager();
  self.userConfig = new HatoholUserConfig();
  start();

  function start() {
    var numRecordsPerPage;
    self.userConfig.get({
      itemNames:['num-items-per-page', 'items-filter-offset',
                 'items-filter-server', 'items-filter-host-group',
                 'items-filter-host', 'items-filter-application'],
      successCallback: function(conf) {
        self.baseQuery.limit =
          self.userConfig.findOrDefault(conf, 'num-items-per-page',
                                        self.baseQuery.limit);

        self.baseQuery.offset =
          self.userConfig.findOrDefault(conf, 'items-filter-offset',
                                        self.baseQuery.offset);

        self.currentPage = self.baseQuery.offset / self.baseQuery.limit;

        self.baseQuery.serverId =
          self.userConfig.findOrDefault(conf, 'items-filter-server',
                                        self.baseQuery.serverId);

        self.baseQuery.hostgroupId =
          self.userConfig.findOrDefault(conf, 'items-filter-host-group',
                                        self.baseQuery.hostgroupId);

        self.baseQuery.hostId =
          self.userConfig.findOrDefault(conf, 'items-filter-host',
                                        self.baseQuery.hostId);

        self.baseQuery.appName =
          self.userConfig.findOrDefault(conf, 'items-filter-application',
                                        self.baseQuery.appName);

        updatePager();
        setupFilterValues();
        setupCallbacks();
        load(self.currentPage);
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

  function updatePager() {
    self.pager.update({
      numTotalRecords: rawData ? rawData["totalNumberOfItems"] : -1,
      numRecordsPerPage: self.baseQuery.limit,
      selectPageCallback: function(page) {
        load(page);
        var items = {}, isChanged = false;
        if (self.pager.numRecordsPerPage != self.baseQuery.limit) {
          self.baseQuery.limit = self.pager.numRecordsPerPage;
          $.extend(items, {'num-items-per-page': self.baseQuery.limit});
////          saveConfig({'num-items-per-page': self.baseQuery.limit});
          isChanged = true;
        }

        var val = self.currentPage * self.baseQuery.limit;
        if (self.baseQuery.offset != val) {
          self.baseQuery.offset = val;
//          saveConfig({'items-filter-offset': self.baseQuery.offset});
          $.extend(items, {'items-filter-offset': self.baseQuery.offset});
          isChanged = true;
        }

        if (isChanged)
           saveConfig(items);
      }
    });
  }

  function setupFilterValues(servers, query, withoutSelfMonitor) {
    if (!servers && rawData && rawData.servers)
      servers = rawData.servers;

    if (!query)
      query = self.lastQuery ? self.lastQuery : self.baseQuery;

    self.setupHostFilters(servers, query, withoutSelfMonitor);

    if ('limit' in query)
      $('#num-items-per-page').val(query.limit);
    if ("appName" in query)
      $('#select-application').val(self.baseQuery.appName);
  }

  function setupCallbacks() {
    $("#table").stupidtable();
    $("#table").bind('aftertablesort', function(event, data) {
      var th = $(this).find("th");
      th.find("i.sort").remove();
      var icon = data.direction === "asc" ? "up" : "down";
      th.eq(data.column).append("<i class='sort glyphicon glyphicon-arrow-" + icon +"'></i>");
    });

    self.setupHostQuerySelectorCallback(
      load, '#select-server', '#select-host-group', '#select-host', '#select-application');

    $("#select-server, #select-host-group, #select-host").change(function() {
      var val = "", items = {};

      val = self.getTargetServerId();
      if (!val) {
        val = "-1";
      }
      if (self.baseQuery.serverId != val) {
        self.baseQuery.serverId = val;
        $.extend(items, {'items-filter-server': self.baseQuery.serverId});
      }

      val = self.getTargetHostgroupId();
      if (!val)
        val = "*";
      if (self.baseQuery.hostgroupId != val) {
        self.baseQuery.hostgroupId = val;
        $.extend(items, {'items-filter-host-group': self.baseQuery.hostgroupId});
      }

      val = self.getTargetHostId();
      if (!val)
        val = "*";
      if (self.baseQuery.hostId != val) {
        self.baseQuery.hostId = val;
        $.extend(items, {'items-filter-host': self.baseQuery.hostId});
      }

      if (self.baseQuery.offset != 0) {
        self.baseQuery.offset = 0;
        $.extend(items, {'items-filter-offset': self.baseQuery.offset});
      }

      saveConfig(items);
    });

    $('#select-application').change(function() {
      var val = self.getTargetAppName();
      if (self.baseQuery.appName != val) {
        self.baseQuery.appName = val;
        saveConfig({'items-filter-application': self.baseQuery.appName});
      }
    });
  }

  function parseData(replyData) {
    var parsedData = {};
    var appNames = [];
    var x, item;

    for (x = 0; x < replyData["applications"].length; ++x) {
      item = replyData["applications"][x];

      if (item["name"].length == 0)
        item["name"] = "_non_";
      else
        appNames.push(item["name"]);
    }
    parsedData.applications = appNames.uniq().sort();

    return parsedData;
  }

  function setLoading(loading) {
    if (loading) {
      $("#select-server").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
      $("#select-application").attr("disabled", "disabled");
    } else {
      $("#select-server").removeAttr("disabled");
      if ($("#select-host option").length > 1)
        $("#select-host").removeAttr("disabled");
    }
    $("#select-application").removeAttr("disabled");
  }

  function getGraphURL(item) {
    var query = {
      serverId: item["serverId"],
      hostId:   item["hostId"],
      itemId:   item["id"]
    };
    return "ajax_history?" + $.param(query);
  }

  function getGraphLink(item) {
    if (!item || !item["lastValue"] || isNaN(item["lastValue"]))
      return "";
    var link = "<a href='" + getGraphURL(item) + "'>";
    link += gettext("Graph");
    link += "</a>";
    return link;
  }

  function drawTableBody(replyData) {
    var nickName, hostName, clock, appName;
    var html = "", url, server, item, x;
    var targetAppName = self.getTargetAppName();

    html = "";
    for (x = 0; x < replyData["items"].length; ++x) {
      item       = replyData["items"][x];
      server     = replyData["servers"][item["serverId"]];
      url        = getItemGraphLocation(server, item["id"]);
      nickName = getNickName(server, item["serverId"]);
      hostName   = getHostName(server, item["hostId"]);
      clock      = item["lastValueTime"];
      appName    = item["itemGroupName"];

      if (targetAppName && appName != targetAppName)
        continue;

      html += "<tr><td>" + escapeHTML(nickName) + "</td>";
      html += "<td>" + escapeHTML(hostName) + "</td>";
      html += "<td>" + escapeHTML(appName) + "</td>";
      if (url)
        html += "<td><a href='" + url + "' target='_blank'>"
                + escapeHTML(item["brief"])  + "</a></td>";
      else
        html += "<td>" + escapeHTML(item["brief"])  + "</td>";
      html += "<td data-sort-value='" + escapeHTML(clock) + "'>" + formatDate(clock) + "</td>";
      html += "<td>" + formatItemLastValue(item) + "</td>";
      html += "<td>" + getGraphLink(item) + "</td>";
      html += "</tr>";
    }

    return html;
  }

  function drawTableContents(rawData) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(rawData));
  }

  function updateCore(reply) {
    rawData = reply;
    parsedData = parseData(rawData);

    self.setServerFilterCandidates(rawData["servers"]);
    self.setHostgroupFilterCandidates(rawData["servers"]);
    self.setHostFilterCandidates(rawData["servers"]);
    self.setApplicationFilterCandidates(parsedData.applications);

    drawTableContents(rawData);
    self.pager.update({ numTotalRecords: rawData["totalNumberOfItems"] });
    setupFilterValues(rawData.servers,
                      self.lastQuery ? self.lastQuery : self.baseQuery,
                      true);
    setLoading(false);
    self.setAutoReload(load, self.reloadIntervalSeconds);
  }

  function getItemsQueryInURI() {
    var knownKeys = [
      "serverId", "hostgroupId", "hostId",
      "limit", "offset", "appName",
    ];
    var i, allParams = deparam(), query = {};
    for (i = 0; i < knownKeys.length; i++) {
      if (knownKeys[i] in allParams)
        query[knownKeys[i]] = allParams[knownKeys[i]];
    }
    return query;
  }

  function getQuery(page) {
    if (isNaN(page))
      page = 0;
    if (self.currentPage != page)
      self.currentPage = page;
    var val = self.baseQuery.limit * self.currentPage;
    if (self.baseQuery.offset != val) {
      self.baseQuery.offset = val;
      saveConfig({'items-filter-offset': self.baseQuery.offset});
    }

    var query = $.extend({}, self.baseQuery, {
      limit:  self.baseQuery.limit,
    });
    if (self.lastQuery) {
      $.extend(query, self.getHostFilterQuery());
      $.extend(query, query, { appName: self.getTargetAppName() });
    }
    self.lastQuery = query;
    return 'item?' + $.param(query);
  };

  function load(page) {
    self.displayUpdateTime();
    setLoading(true);
    if (isNaN(page))
      page = 0;
    self.currentPage = page;
    var val = self.currentPage * self.baseQuery.limit;
    if (self.baseQuery.offset != val) {
      self.baseQuery.offset = val;
      saveConfig({'items-filter-offset': self.baseQuery.offset});
    }
    self.startConnection(getQuery(self.currentPage), updateCore);
    self.pager.update({ currentPage: self.currentPage });
    $(document.body).scrollTop(0);
  }
};

LatestView.prototype = Object.create(HatoholMonitoringView.prototype);
LatestView.prototype.constructor = LatestView;
