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
  self.durations = {};

  var rawData;

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
        self.limitOfUnifiedId = rawData.lastUnifiedEventId;
    } else {
      self.currentPage = 0;
      self.limitOfUnifiedId = 0;
    }

    var query = {
      maximumNumber: self.numEventsPerPage,
      offset:        self.numEventsPerPage * self.currentPage,
      sortType:      self.sortType,
      sortOrder:     self.sortOrder
    };
    return '/events?' + $.param(query);
  };

  function createPage() {
    createUI(self.baseElem);
    setupEvents();
    connParam.url = getEventsURL();
    self.connector = new HatoholConnector(connParam);
  }

  function createUI(elem) {
    var s = '';
    s += '<h2>' + gettext('Events') + '</h2>';

    s += '<form class="form-inline hatohol-filter-toolbar">';
    s += '  <label>' + gettext('Minimum Severity:') + '</label>';
    s += '  <select id="select-severity" class="form-control">';
    s += '    <option>0</option>';
    s += '    <option>1</option>';
    s += '    <option>2</option>';
    s += '    <option>3</option>';
    s += '    <option>4</option>';
    s += '  </select>';
    s += '  <label>' + gettext('Status:') + '</label>';
    s += '  <select id="select-status" class="form-control">';
    s += '    <option value="-1">---------</option>';
    s += '    <option value="0">' + gettext('OK') + '</option>';
    s += '    <option value="1">' + gettext('Problem') + '</option>';
    s += '    <option value="2">' + gettext('Unknown') + '</option>';
    s += '  </select>';
    s += '  <label>' + gettext('Server:') + '</label>';
    s += '  <select id="select-server" class="form-control">';
    s += '    <option>---------</option>';
    s += '  </select>';
    s += '  <label>' + gettext('Host:') + '</label>';
    s += '  <select id="select-host" class="form-control">';
    s += '    <option>---------</option>';
    s += '  </select>';
    s += '  <label for="num-events-per-page">' + gettext("# of events per page") + '</label>';
    s += '  <input type="text" id="num-events-per-page" class="form-control" style="width:4em;">';
    s += '</form>';

    s += '<table class="table table-condensed table-hover" id="table">';
    s += '  <thead>';
    s += '    <tr>';
    s += '      <th data-sort="string">' + gettext('Server') + '</th>';
    s += '      <th data-sort="int">' + gettext('Time') + '</th>';
    s += '      <th data-sort="string">' + gettext('Host') + '</th>';
    s += '      <th data-sort="string">' + gettext('Brief') + '</th>';
    s += '      <th data-sort="int">' + gettext('Status') + '</th>';
    s += '      <th data-sort="int">' + gettext('Severity') + '</th>';
    s += '      <th data-sort="int">' + gettext('Duration') + '</th>';
    /* Not supported yet
    s += '      <th data-sort="int">' + gettext('Comment') + '</th>';
    s += '      <th data-sort="int">' + gettext('Action') + '</th>';
    */
    s += '    </tr>';
    s += '  </thead>';
    s += '  <tbody>';
    s += '  </tbody>';
    s += '</table>';

    s += '<center>';
    s += '<form class="form-inline">';
    s += '  <input id="latest-events-button" type="button" class="btn btn-info" value="' + gettext('Latest events') + '" />';
    s += '  <input id="next-events-button" type="button" class="btn btn-primary" value="' + gettext('To next') + '" />';
    s += '</form>';
    s += '</center>';
    s += '<br>';

    $(elem).append(s);
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
      self.updateScreen(rawData, updateCore);
    });
    $("#select-server").change(function() {
      var serverName = $("#select-server").val();
      setHostFilterCandidates();
      drawTableContents(rawData);
    });
    $("#select-host").change(function() {
      drawTableContents(rawData);
    });
    $("#select-status").change(function() {
      drawTableContents(rawData);
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
      connParam.url = getEventsURL(loadNextPage);
      self.connector.start(connParam);
      $(self.baseElem).scrollTop(0);
    });

    $('#latest-events-button').click(function() {
      connParam.url = getEventsURL();
      self.connector.start(connParam);
      $(self.baseElem).scrollTop(0);
    });
  }

  function parseData(replyData) {
    var durations = {}, durationsForTrigger;
    var triggerId;
    var x, event, server;
    var allTimes, serverName, hostName;
    var times, now;

    // extract times from raw data
    allTimes = {};
    for (x = 0; x < replyData["events"].length; ++x) {
      event = replyData["events"][x];
      var serverId = event["serverId"];
      server = replyData["servers"][serverId];
      serverName = getServerName(server, serverId);
      triggerId = event["triggerId"];

      if (!allTimes[serverName])
        allTimes[serverName] = {};
      if (!allTimes[serverName][triggerId])
        allTimes[serverName][triggerId] = [];
      
      allTimes[serverName][triggerId].push(event["time"]);
    }

    // create durations map
    for (serverName in allTimes) {
      for (triggerId in allTimes[serverName]) {
        times = allTimes[serverName][triggerId].uniq().sort();
        durationsForTrigger = {};
        for (x = 0; x < times.length; ++x) {
          if (x == times.length - 1) {
            now = parseInt((new Date()).getTime() / 1000);
            durationsForTrigger[times[x]] = now - Number(times[x]);
          } else {
            durationsForTrigger[times[x]] = Number(times[x + 1]) - Number(times[x]);
          }
        }
        allTimes[serverName][triggerId] = durationsForTrigger;
      }
      durations[serverName] = allTimes[serverName];
    }

    return durations;
  }

  function getTargetServerId() {
    var id = $("#select-server").val();
    if (id == "---------")
      id = null;
    return id;
  }

  function getTargetHostId() {
    var id = $("#select-host").val();
    if (id == "---------")
      id = null;
    return id;
  }

  function compareFilterLabel(a, b) {
    if (a.label < b.label)
      return -1;
    if (a.label > b.label)
      return 1;
    if (a.id < b.id)
      return -1;
    if (a.id > b.id)
      return 1;
    return 0;
  }

  function setServerFilterCandidates(selector) {
    var servers = rawData["servers"], serverLabels = [];
    if (!selector)
      selector = $('#select-server');
    for (id in servers) {
      serverLabels.push({
        label: getServerName(servers[id], id),
        value: id
      });
    }
    serverLabels.sort(compareFilterLabel);
    self.setFilterCandidates(selector, serverLabels);
  }

  function setHostFilterCandidates(serverId, selector) {
    var servers, server, hosts, hostLabels = [];

    if (!serverId)
      serverId = getTargetServerId();
    if (!selector)
      selector = $('#select-host');

    self.setFilterCandidates(selector);

    servers = rawData["servers"];
    if (!servers || !servers[serverId])
      return;

    server = servers[serverId];
    hosts = server.hosts;
    for (id in hosts) {
      hostLabels.push({
        label: getHostName(server, id),
        value: id
      });
    }
    hostLabels.sort(compareFilterLabel);
    self.setFilterCandidates(selector, hostLabels);
  }

  function drawTableBody(rd) {
    var serverName, hostName, clock, status, severity, duration;
    var server, event, html = "";
    var x;
    var targetServerId = getTargetServerId();
    var targetHostId = getTargetHostId();
    var minimumSeverity = $("#select-severity").val();
    var targetStatus = $("#select-status").val();

    for (x = 0; x < rd["events"].length; ++x) {
      event = rd["events"][x];
      if (event["severity"] < minimumSeverity)
        continue;
      if (targetStatus >= 0 && event["type"] != targetStatus)
        continue;

      var serverId = event["serverId"];
      var hostId = event["hostId"];
      server     = rd["servers"][serverId];
      serverName = getServerName(server, serverId);
      hostName   = getHostName(server, hostId);
      clock      = event["time"];
      status     = event["type"];
      severity   = event["severity"];
      duration   = self.durations[serverName][event["triggerId"]][clock];

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

  function drawTableContents(rawData) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(rawData));
  }

  function updateCore(reply) {
    rawData = reply;
    self.durations = parseData(rawData);

    setServerFilterCandidates();
    setHostFilterCandidates();
    drawTableContents(rawData);
  }
};

EventsView.prototype = Object.create(HatoholMonitoringView.prototype);
EventsView.prototype.constructor = EventsView;
