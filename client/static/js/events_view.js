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

var EventsView = function(userProfile, baseElem) {
  var self = this;
  self.baseElem = baseElem;
  self.reloadIntervalSeconds = 60;
  self.currentPage = 0;
  self.limitOfUnifiedId = 0;
  self.rawData = {};
  self.durations = {};
  self.baseQuery = {
    limit:            50,
    offset:           0,
    limitOfUnifiedId: 0,
    sortType:         "time",
    sortOrder:        hatohol.DATA_QUERY_OPTION_SORT_DESCENDING,
    minimumSeverity:  "0",
    status:           "-1",
    serverId:         "-1",
    hostgroupId:      "*",
    hostId:           "*",
  };
  $.extend(self.baseQuery, getEventsQueryInURI());
  self.lastQuery = undefined;
  self.showToggleAutoRefreshButton();
  self.setupToggleAutoRefreshButtonHandler(load, self.reloadIntervalSeconds);

  var status_choices = [gettext('OK'), gettext('Problem'), gettext('Unknown'),
                        gettext('Notification')];
  var severity_choices = [
    gettext('Not classified'), gettext('Information'), gettext('Warning'),
    gettext('Average'), gettext('High'), gettext('Disaster')];

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  self.pager = new HatoholEventPager();
  self.userConfig = new HatoholUserConfig();
  start();

  //
  // Private functions
  //
  function start() {
    self.userConfig.get({
      itemNames:['num-records-per-page', 'event-sort-order',
                 'event-filter-offset', 'event-filter-minimum-severity',
                 'event-filter-status', 'event-filter-server',
                 'event-filter-host-group', 'event-filter-host'],
      successCallback: function(conf) {
        self.baseQuery.limit =
          self.userConfig.findOrDefault(conf, 'num-records-per-page',
                                        self.baseQuery.limit);
        self.baseQuery.sortType =
          self.userConfig.findOrDefault(conf, 'event-sort-type',
                                        self.baseQuery.sortType);
        self.baseQuery.sortOrder =
          self.userConfig.findOrDefault(conf, 'event-sort-order',
                                        self.baseQuery.sortOrder);

        self.baseQuery.offset =
          self.userConfig.findOrDefault(conf, 'event-filter-offset',
                                        self.baseQuery.offset);

        self.currentPage = self.baseQuery.offset / self.baseQuery.limit;

        self.baseQuery.minimumSeverity =
          self.userConfig.findOrDefault(conf, 'event-filter-minimum-severity',
                                        self.baseQuery.minimumSeverity);

        self.baseQuery.status =
          self.userConfig.findOrDefault(conf, 'event-filter-status',
                                        self.baseQuery.status);

        self.baseQuery.serverId =
          self.userConfig.findOrDefault(conf, 'event-filter-server',
                                        self.baseQuery.serverId);

        self.baseQuery.hostgroupId =
          self.userConfig.findOrDefault(conf, 'event-filter-host-group',
                                        self.baseQuery.hostgroupId);

        self.baseQuery.hostId =
          self.userConfig.findOrDefault(conf, 'event-filter-host',
                                        self.baseQuery.hostId);

        updatePager();
        setupFilterValues();
        setupCallbacks();

        var direction =
          (self.baseQuery.sortOrder == hatohol.DATA_QUERY_OPTION_SORT_ASCENDING) ? "asc" : "desc";
        var thTime = $("#table").find("thead th").eq(1);
        thTime.stupidsort(direction);
        load(self.currentPage);
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        showXHRError(XMLHttpRequest);
      },
    });
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
      currentPage: self.currentPage,
      numRecords: self.rawData ? self.rawData["numberOfEvents"] : -1,
      numRecordsPerPage: self.baseQuery.limit,
      selectPageCallback: function(page) {
        load(page);

        var items = {}, isChanged = false;
        if (self.pager.numRecordsPerPage != self.baseQuery.limit) {
          self.baseQuery.limit = self.pager.numRecordsPerPage;
          items = {'num-records-per-page': self.baseQuery.limit};
          isChanged = true;
        }
        var val = self.currentPage * self.baseQuery.limit;
        if (self.baseQuery.offset != val) {
          self.baseQuery.offset = val;
          $.extend(items, {'event-filter-offset': self.baseQuery.offset});
          isChanged = true;
        }
        if (isChanged)
          saveConfig(items);
      }
    });
  }

  function showXHRError(XMLHttpRequest) {
    var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
      XMLHttpRequest.statusText;
    hatoholErrorMsgBox(errorMsg);
  }

  function getEventsQueryInURI() {
    var knownKeys = [
      "serverId", "hostgroupId", "hostId",
      "limit", "offset", "limitOfUnifiedId",
      "sortType", "sortOrder",
      "minimumSeverity", "status", "triggerId",
    ];
    var i, allParams = deparam(), query = {};
    for (i = 0; i < knownKeys.length; i++) {
      if (knownKeys[i] in allParams)
        query[knownKeys[i]] = allParams[knownKeys[i]];
    }
    return query;
  }

  function getQuery(page) {
    if (!page) {
      self.limitOfUnifiedId = 0;
    } else {
      if (!self.limitOfUnifiedId)
        self.limitOfUnifiedId = self.rawData.lastUnifiedEventId;
    }

    var query = $.extend({}, self.baseQuery, {
      minimumSeverity:  $("#select-severity").val(),
      status:           $("#select-status").val(),
      offset:           self.baseQuery.limit * self.currentPage,
      limitOfUnifiedId: self.limitOfUnifiedId,
    });
    if (self.lastQuery)
      $.extend(query, self.getHostFilterQuery());
    self.lastQuery = query;

    return 'events?' + $.param(query);
  };

  function load(page) {
    self.displayUpdateTime();
    setLoading(true);
    if (!isNaN(page)) {
      self.currentPage = page;
      self.disableAutoRefresh();
    } else {
      self.currentPage = 0;
    }
    self.startConnection(getQuery(self.currentPage), updateCore);
    $(document.body).scrollTop(0);
  }

  function setupFilterValues(servers, query) {
    if (!servers && self.rawData && self.rawData.servers)
      servers = self.rawData.servers;

    if (!query)
      query = self.lastQuery ? self.lastQuery : self.baseQuery;

    self.setupHostFilters(servers, query);

    if ('limit' in query)
      $('#num-records-per-page').val(query.limit);
    if ("minimumSeverity" in query)
      $("#select-severity").val(query.minimumSeverity);
    if ("status" in query)
      $("#select-status").val(query.status);
  }

  function setupCallbacks() {
    $("#table").stupidtable();
    $("#table").bind('aftertablesort', function(event, data) {
      var icon;
      if (data.column == 1) { // "Time" column
        if (data.direction === "asc") {
          icon = "up";
          if (self.baseQuery.sortOrder != hatohol.DATA_QUERY_OPTION_SORT_ASCENDING) {
            self.baseQuery.sortOrder = hatohol.DATA_QUERY_OPTION_SORT_ASCENDING;
            saveConfig({'event-sort-order': self.baseQuery.sortOrder});
            self.startConnection(getQuery(self.currentPage), updateCore);
          }
        } else {
          icon = "down";
          if (self.baseQuery.sortOrder != hatohol.DATA_QUERY_OPTION_SORT_DESCENDING) {
            self.baseQuery.sortOrder = hatohol.DATA_QUERY_OPTION_SORT_DESCENDING;
            saveConfig({'event-sort-order': self.baseQuery.sortOrder});
            self.startConnection(getQuery(self.currentPage), updateCore);
          }
        }
      }
      var th = $(this).find("th");
      th.find("i.sort").remove();
      if (icon)
        th.eq(data.column).append("<i class='sort glyphicon glyphicon-arrow-" + icon +"'></i>");
    });

    $('#select-severity').change(function() {
      var val = $('#select-severity').val();
      if (self.baseQuery.minimumSeverity != val) {
        self.baseQuery.minimumSeverity = val;
        saveConfig({'event-filter-minimum-severity': self.baseQuery.minimumSeverity});
      }
      load();
    });

    $('#select-status').change(function() {
      var val = $('#select-status').val();
      if (self.baseQuery.status != val) {
        self.baseQuery.status = val;
        saveConfig({'event-filter-status': self.baseQuery.status});
      }
      load();
    });

    self.setupHostQuerySelectorCallback(
      load, '#select-server', '#select-host-group', '#select-host');

    $("#select-server, #select-host-group, #select-host").change(function() {
      var val = "", items = {};

      val = self.getTargetServerId();
      if (!val) {
        val = "-1";
      }
      if (self.baseQuery.serverId != val) {
        self.baseQuery.serverId = val;
        $.extend(items, {'event-filter-server': self.baseQuery.serverId});
      }

      val = self.getTargetHostgroupId();
      if (!val)
        val = "*";
      if (self.baseQuery.hostgroupId != val) {
        self.baseQuery.hostgroupId = val;
        $.extend(items, {'event-filter-host-group': self.baseQuery.hostgroupId});
      }

      var val = self.getTargetHostId();
      if (!val)
        val = "*";
      if (self.baseQuery.hostId != val) {
        self.baseQuery.hostId = val;
        $.extend(items, {'event-filter-host': self.baseQuery.hostId});
      }

      saveConfig(items);
    });

    $('#num-records-per-page').change(function() {
      var val = parseInt($('#num-records-per-page').val());
      if (!isFinite(val))
        val = self.baseQuery.limit;
      $('#num-records-per-page').val(val);
      self.baseQuery.limit = val;

      var params = {
        items: {'num-records-per-page': val},
        successCallback: function(){ /* we just ignore it */ },
        connectErrorCallback: function(XMLHttpRequest, textStatus,
                                       errorThrown) {
          showXHRError(XMLHttpRequest);
        },
      };
      self.userConfig.store(params);
    });

    $('button.latest-button').click(function() {
      load();
    });
  }

  function setLoading(loading) {
    if (loading) {
      $("#select-severity").attr("disabled", "disabled");
      $("#select-status").attr("disabled", "disabled");
      $("#select-server").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
      $("#num-records-per-page").attr("disabled", "disabled");
      $("#latest-events-button1").attr("disabled", "disabled");
      $("#latest-events-button2").attr("disabled", "disabled");
    } else {
      $("#select-severity").removeAttr("disabled");
      $("#select-status").removeAttr("disabled");
      $("#select-server").removeAttr("disabled");
      if ($("#select-host option").length > 1)
        $("#select-host").removeAttr("disabled");
      $("#num-records-per-page").removeAttr("disabled");
      $("#latest-events-button1").removeAttr("disabled");
      $("#latest-events-button2").removeAttr("disabled");
    }
  }

  function parseData(replyData) {
    // The structur of durations:
    // {
    //   serverId1: {
    //     triggerId1: {
    //       clock1: duration1,
    //       clock2: duration2,
    //       ...
    //     },
    //     triggerId2: ...
    //   },
    //   serverId2: ...
    // }

    var durations = {};
    var serverId, triggerId;
    var x, event, now, times, durationsForTrigger;

    // extract times from raw data
    for (x = 0; x < replyData["events"].length; ++x) {
      event = replyData["events"][x];
      serverId = event["serverId"];
      triggerId = event["triggerId"];

      if (!durations[serverId])
        durations[serverId] = {};
      if (!durations[serverId][triggerId])
        durations[serverId][triggerId] = [];

      durations[serverId][triggerId].push(event["time"]);
    }

    // create durations maps and replace times arrays with them
    for (serverId in durations) {
      for (triggerId in durations[serverId]) {
        times = durations[serverId][triggerId].uniq().sort();
        durationsForTrigger = {};
        for (x = 0; x < times.length; ++x) {
          if (x == times.length - 1) {
            now = parseInt((new Date()).getTime() / 1000);
            durationsForTrigger[times[x]] = now - Number(times[x]);
          } else {
            durationsForTrigger[times[x]] = Number(times[x + 1]) - Number(times[x]);
          }
        }
        durations[serverId][triggerId] = durationsForTrigger;
      }
    }

    return durations;
  }

  function isSelfMonitoringHost(hostId) {
    return  hostId == "__SELF_MONITOR";
  }

  function generateTimeColumn(serverURL, hostId, triggerId, eventId, clock) {
    var html = "";
    if (serverURL.indexOf("zabbix") >= 1 && !isSelfMonitoringHost(hostId)) {
      html += "<td><a href='" + serverURL + "tr_events.php?&triggerid="
              + triggerId + "&eventid=" + eventId
              + "' target='_blank'>" + escapeHTML(formatDate(clock))
              + "</a></td>";
    } else {
      html += "<td data-sort-value='" + escapeHTML(clock) + "'>" +
              formatDate(clock) + "</td>";
    }
    return html;
  }

  function generateHostColumn(serverURL, hostId, hostName) {
    var html = "";
    if (serverURL.indexOf("zabbix") >= 0 && !isSelfMonitoringHost(hostId)) {
      html += "<td><a href='" + serverURL + "latest.php?&hostid="
              + hostId + "' target='_blank'>" + escapeHTML(hostName)
              + "</a></td>";
    } else if (serverURL.indexOf("nagios") >= 0 && !isSelfMonitoringHost(hostId)) {
      html += "<td><a href='" + serverURL + "cgi-bin/status.cgi?host="
        + hostName + "' target='_blank'>" + escapeHTML(hostName) + "</a></td>";
    } else {
      html += "<td>" + escapeHTML(hostName) + "</td>";
    }
    return html;
  }

  function generateIncidentColumns(haveIncident, incident) {
    var html = "";
    if (haveIncident) {
      html += "<td class='incident'>";
      html += "<a href='" + escapeHTML(incident.location)
              + "' target='_blank'>";
      html += escapeHTML(incident.status) + "</a>";
      html += "</td>";
      html += "<td class='incident'>";
      html += escapeHTML(incident.priority);
      html += "</td>";
      html += "<td class='incident'>";
      html += escapeHTML(incident.assignee);
      html += "</td>";
      html += "<td class='incident'>";
      if (incident.status)
        html += escapeHTML(incident.doneRatio) + "%";
      html += "</td>";
    }
    return html;
  }

  function getEventDescription(event) {
    var extendedInfo, name;

    try {
      extendedInfo = JSON.parse(event["extendedInfo"]);
      name = extendedInfo["expandedDescription"];
    } catch(e) {
    }
    return name ? name : event["brief"];
  }

  function drawTableBody() {
    var serverName, nickName, hostName, clock, status, severity, incident, duration, description;
    var server, event, eventId, serverId, serverURL, hostId, triggerId, html = "";
    var x, severityClass;

    for (x = 0; x < self.rawData["events"].length; ++x) {
      event      = self.rawData["events"][x];
      serverId   = event["serverId"];
      hostId     = event["hostId"];
      triggerId  = event["triggerId"];
      eventId    = event["eventId"];
      server     = self.rawData["servers"][serverId];
      nickName   = getNickName(server, serverId);
      serverURL  = getServerLocation(server);
      hostName   = getHostName(server, hostId);
      clock      = event["time"];
      status     = event["type"];
      severity   = event["severity"];
      duration   = self.durations[serverId][event["triggerId"]][clock];
      incident   = event["incident"];
      severityClass = "severity";
      if (status == hatohol.EVENT_TYPE_BAD)
        severityClass += escapeHTML(severity);
      description = getEventDescription(event);
      if (serverURL) {
        html += "<tr><td><a href='" + serverURL + "' target='_blank'>" + escapeHTML(nickName)
                + "</a></td>";
        html += generateTimeColumn(serverURL, hostId, triggerId, eventId, clock);
        html += generateHostColumn(serverURL, hostId, hostName);
      } else {
        html += "<tr><td>" + escapeHTML(nickName)+ "</td>";
        html += "<td data-sort-value='" + escapeHTML(clock) + "'>" +
                formatDate(clock) + "</td>";
        html += "<td>" + escapeHTML(hostName) + "</td>";
      }
      html += "<td>" + escapeHTML(description) + "</td>";
      html += "<td class='status" + escapeHTML(status) +
        "' data-sort-value='" + escapeHTML(status) + "'>" +
        status_choices[Number(status)] + "</td>";
      html += "<td class='" + severityClass +
        "' data-sort-value='" + escapeHTML(severity) + "'>" +
        severity_choices[Number(severity)] + "</td>";
      html += "<td data-sort-value='" + duration + "'>" +
        formatSecond(duration) + "</td>";
      html += generateIncidentColumns(self.rawData["haveIncident"], incident);
      html += "</tr>";
    }

    return html;
  }

  function drawTableContents() {
    if (self.rawData["haveIncident"]) {
      $(".incident").show();
    }
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody());
  }

  function updateCore(reply) {
    self.rawData = reply;
    self.durations = parseData(self.rawData);
    if (!self.rawData["events"].length) {
      if (self.baseQuery.offset != 0) {
        self.baseQuery.offset = 0;
        saveConfig({'event-filter-offset': self.baseQuery.offset});
      }
      if (self.currentPage != 0)
        self.currentPage = 0;
    }

    setupFilterValues();
    drawTableContents();
    updatePager();
    setLoading(false);
    if (self.currentPage == 0)
      self.enableAutoRefresh(load, self.reloadIntervalSeconds);
  }
};

EventsView.prototype = Object.create(HatoholMonitoringView.prototype);
EventsView.prototype.constructor = EventsView;
