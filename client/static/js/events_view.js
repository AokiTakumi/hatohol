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

var EventsView = function(userProfile, baseElem) {
  var self = this;
  self.baseElem = baseElem;
  self.currentPage = 0;
  self.limiOfUnifiedId = 0;
  self.rawData = {};
  self.durations = {};

  var status_choices = [gettext('OK'), gettext('Problem'), gettext('Unknown')];
  var severity_choices = [
    gettext('Not classified'), gettext('Information'), gettext('Warning'),
    gettext('Average'), gettext('High'), gettext('Disaster')];

  var connParam =  {
    replyCallback: function(reply, parser) {
      self.updateScreen(reply, updateCore);
    },
    parseErrorCallback: function(reply, parser) {
      hatoholErrorMsgBoxForParser(reply, parser);

      self.setStatus({
        "class" : "danger",
        "label" : gettext("ERROR"),
        "lines" : [ msg ],
      });
    }
  };

  // call the constructor of the super class
  HatoholMonitoringView.apply(userProfile);

  self.userConfig = new HatoholUserConfig(); 
  start();

  //
  // Private functions 
  //
  function start() {
    var DEFAULT_NUM_EVENTS_PER_PAGE = 50;
    var DEFAULT_SORT_TYPE = "time";
    var DEFAULT_SORT_ORDER = hatohol.DATA_QUERY_OPTION_SORT_DESCENDING;
    self.userConfig.get({
      itemNames:['num-events-per-page', 'event-sort-order'],
      successCallback: function(conf) {
        self.numEventsPerPage =
          self.userConfig.findOrDefault(conf, 'num-events-per-page',
                                        DEFAULT_NUM_EVENTS_PER_PAGE);
        self.sortType = 
          self.userConfig.findOrDefault(conf, 'event-sort-type',
                                        DEFAULT_SORT_TYPE);
        self.sortOrder = 
          self.userConfig.findOrDefault(conf, 'event-sort-order',
                                        DEFAULT_SORT_ORDER);
        createPage();
      },
      connectErrorCallback: function(XMLHttpRequest, textStatus, errorThrown) {
        // TODO: implement
      },
    });
  }

  function getEventsURL(loadNextPage) {
    if (loadNextPage) {
      self.currentPage += 1;
      if (!self.limitOfUnifiedId)
        self.limitOfUnifiedId = self.rawData.lastUnifiedEventId;
    } else {
      self.currentPage = 0;
      self.limitOfUnifiedId = 0;
    }

    var serverId = self.getTargetServerId();
    var hostId = self.getTargetHostId();
    var query = {
      minimumSeverity: $("#select-severity").val(),
      status:          $("#select-status").val(),
      maximumNumber:   self.numEventsPerPage,
      offset:          self.numEventsPerPage * self.currentPage,
      sortType:        self.sortType,
      sortOrder:       self.sortOrder
    };
    if (serverId)
      query.targetServerId = serverId;
    if (hostId)
      query.targetHostId = hostId;

    return '/events?' + $.param(query);
  };

  function createPage() {
    setupEvents();
    load();
  }

  function load(loadNextPage) {
    connParam.url = getEventsURL(loadNextPage);
    if (self.connector)
      self.connector.start(connParam);
    else
      self.connector = new HatoholConnector(connParam);
    $(self.baseElem).scrollTop(0);
    setLoading(true);
  }

  function setupEvents() {
    $("#table").stupidtable();
    $("#table").bind('aftertablesort', function(event, data) {
      var th = $(this).find("th");
      th.find("i.sort").remove();
      var icon = data.direction === "asc" ? "up" : "down";
      th.eq(data.column).append("<i class='sort glyphicon glyphicon-arrow-" + icon +"'></i>");
    });

    $("#select-severity").change(function() {
      load();
    });
    $("#select-server").change(function() {
      var serverId = $("#select-server").val();
      self.setHostFilterCandidates(self.rawData["servers"], serverId);
      load();
    });
    $("#select-host").change(function() {
      load();
    });
    $("#select-status").change(function() {
      load();
    });

    $('#num-events-per-page').val(self.numEventsPerPage);
    $('#num-events-per-page').change(function() {
      var val = parseInt($('#num-events-per-page').val());
      if (!isFinite(val))
        val = self.numEventsPerPage;
      $('#num-events-per-page').val(val);
      self.numEventsPerPage = val;

      var params = {
        items: {'num-events-per-page': val},
        successCallback: function(){ /* we just ignore it */ },
        connectErrorCallback: function() {
          // TODO: show an error message
        },
      };
      self.userConfig.store(params);
    });

    $('#next-events-button').click(function() {
      var loadNextPage = true;
      load(loadNextPage);
    });

    $('#latest-events-button').click(function() {
      load();
    });
  }

  function setLoading(loading) {
    if (loading) {
      $("#select-severity").attr("disabled", "disabled");
      $("#select-status").attr("disabled", "disabled");
      $("#select-server").attr("disabled", "disabled");
      $("#select-host").attr("disabled", "disabled");
      $("#num-events-per-page").attr("disabled", "disabled");
      $("#latest-events-button").attr("disabled", "disabled");
      $("#next-events-button").attr("disabled", "disabled");
    } else {
      $("#select-severity").removeAttr("disabled");
      $("#select-status").removeAttr("disabled");
      $("#select-server").removeAttr("disabled");
      if ($("#select-host option").length > 1)
        $("#select-host").removeAttr("disabled");
      $("#num-events-per-page").removeAttr("disabled");
      $("#latest-events-button").removeAttr("disabled");
      $("#next-events-button").removeAttr("disabled");
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

  function drawTableBody() {
    var serverName, hostName, clock, status, severity, duration;
    var server, event, html = "";
    var x;
    var targetServerId = self.getTargetServerId();
    var targetHostId = self.getTargetHostId();
    var minimumSeverity = $("#select-severity").val();
    var targetStatus = $("#select-status").val();

    for (x = 0; x < self.rawData["events"].length; ++x) {
      event = self.rawData["events"][x];
      if (event["severity"] < minimumSeverity)
        continue;
      if (targetStatus >= 0 && event["type"] != targetStatus)
        continue;

      var serverId = event["serverId"];
      var hostId = event["hostId"];
      server     = self.rawData["servers"][serverId];
      serverName = getServerName(server, serverId);
      hostName   = getHostName(server, hostId);
      clock      = event["time"];
      status     = event["type"];
      severity   = event["severity"];
      duration   = self.durations[serverId][event["triggerId"]][clock];

      if (targetServerId && serverId != targetServerId)
        continue;
      if (targetHostId && hostId != targetHostId)
        continue;

      html += "<tr><td>" + escapeHTML(serverName) + "</td>";
      html += "<td data-sort-value='" + escapeHTML(clock) + "'>" + formatDate(clock) + "</td>";
      html += "<td>" + escapeHTML(hostName) + "</td>";
      html += "<td>" + escapeHTML(event["brief"]) + "</td>";
      html += "<td class='status" + escapeHTML(status) + "' data-sort-value='" + escapeHTML(status) + "'>" + status_choices[Number(status)] + "</td>";
      html += "<td class='severity" + escapeHTML(severity) + "' data-sort-value='" + escapeHTML(severity) + "'>" + severity_choices[Number(severity)] + "</td>";
      html += "<td data-sort-value='" + duration + "'>" + formatSecond(duration) + "</td>";
      /*
      html += "<td>" + "unsupported" + "</td>";
      html += "<td>" + "unsupported" + "</td>";
      */
      html += "</tr>";
    }

    return html;
  }

  function drawTableContents() {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody());
  }

  function updateCore(reply) {
    self.rawData = reply;
    self.durations = parseData(self.rawData);

    self.setServerFilterCandidates(self.rawData["servers"]);
    self.setHostFilterCandidates(self.rawData["servers"],
                                 self.getTargetServerId());
    drawTableContents();
    setLoading(false);
  }
};

EventsView.prototype = Object.create(HatoholMonitoringView.prototype);
EventsView.prototype.constructor = EventsView;
