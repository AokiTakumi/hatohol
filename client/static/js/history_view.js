/*
 * Copyright (C) 2014 Project Hatohol
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

var HistoryView = function(userProfile, options) {
  var self = this;
  var itemQuery, historyQuery;

  self.reloadIntervalSeconds = 60;
  self.replyItem = null;
  self.replyHistoryArray = [];

  if (!options)
    options = {};

  itemQuery = self.parseItemQuery(options.query);
  historyQuery = self.parseHistoryQuery(options.query);

  appendGraphArea();
  loadItemAndHistory();

  function appendGraphArea() {
    $("#main").append($("<div>", {
      id: "item-graph",
      height: "300px",
    }));
  };

  function formatHistoryData(item, historyReplies) {
    var i, j, idx = 0, history;
    var data = [ { label: item.brief, data:[] } ];
    if (item.unit)
      data[0].label += " [" + item.unit + "]";
    for (j = 0; j < historyReplies.length; j++) {
      history = historyReplies[j].history;
      for (i = 0; i < history.length; i++) {
	data[0].data[idx++] = [
          // Xaxis: UNIX time in msec
          history[i].clock * 1000 + Math.floor(history[i].ns / 1000000),
          // Yaxis: value
          history[i].value
	];
      }
    }
    return data;
  };

  function drawGraph(item, history) {
    var options = {
      xaxis: {
	mode: "time",
	timezone: "browser",
        tickFormatter: function(val, axis) {
          return formatDate(Math.round(val / 1000));
        }
      },
      yaxis: {
        tickFormatter: function(val, axis) {
          return formatItemValue("" + val, item.unit);
        }
      },
      legend: {
	show: true,
      },
    };
    if (item.valueType == hatohol.ITEM_INFO_VALUE_TYPE_INTEGER)
      options.yaxis.minTickSize = 1;
    $.plot($("#item-graph"), history, options);
  }

  function updateView(reply) {
    var item = self.replyItem.items[0];
    var history = formatHistoryData(item, self.replyHistoryArray);
    self.displayUpdateTime();
    drawGraph(item, history);
    self.setAutoReload(loadHistory, self.reloadIntervalSeconds);
  }

  function getItemQuery() {
    return 'item?' + $.param(itemQuery);
  };

  function getHistoryQuery() {
    var query = $.extend({}, historyQuery);
    var secondsInDay = 60 * 60 * 24;
    var lastReply, lastData, now;

    // omit loading existing data
    if (self.replyHistoryArray.length > 0) {
      lastReply = self.replyHistoryArray[self.replyHistoryArray.length - 1];
      if (lastReply.history.length > 0) {
	lastData = lastReply.history[lastReply.history.length - 1]
	query.beginTime = lastData.clock + 1;
      }
    }

    // set missing time range
    if (!query.endTime) {
      now = new Date();
      query.endTime = Math.floor(now.getTime() / 1000);
    }
    if (!query.beginTime)
      query.beginTime = query.endTime - secondsInDay;

    return 'history?' + $.param(query);
  };

  function buildHostName(itemReply) {
    var item = itemReply.items[0];
    var server = itemReply.servers[item.serverId];
    var serverName = getServerName(server, item["serverId"]);
    var hostName   = getHostName(server, item["hostId"]);
    return serverName + ": " + hostName;
  }

  function setItemDescription(itemReply) {
    var item = itemReply.items[0];
    var hostName = buildHostName(itemReply);
    var title = "";
    title += item.brief;
    if (item.unit)
      title += " [" + item.unit + "]";
    title += " (" + hostName + ")";
    $("title").text(title);
    $("h2").text(title);
  }

  function onLoadItem(reply) {
    var items = reply["items"];
    var messageDetail;

    self.replyItem = reply;

    if (items && items.length == 1) {
      setItemDescription(reply);
      loadHistory();
    } else {
      messageDetail =
        "Monitoring Server ID: " + query.serverId + ", " +
        "Host ID: " + query.hostId + ", " +
        "Item ID: " + query.itemId;
      if (!items || items.length == 0)
        self.showError(gettext("No such item: ") + messageDetail);
      else if (items.length > 1)
        self.showError(gettext("Too many items are found for ") +
                       messageDetail);
    }
  }

  function onLoadHistory(reply) {
    var maxRecordsPerRequest = 1000;
    self.replyHistoryArray.push(reply);
    if (reply.history.length >= maxRecordsPerRequest)
      loadHistory();
    else
      updateView(reply);
  }

  function loadHistory() {
    self.startConnection(getHistoryQuery(), onLoadHistory);
  }

  function loadItemAndHistory() {
    self.startConnection(getItemQuery(), onLoadItem);
  }
};

HistoryView.prototype = Object.create(HatoholMonitoringView.prototype);
HistoryView.prototype.constructor = HistoryView;

HistoryView.prototype.parseQuery = function(query, knownKeys) {
  var knownKeys = ["serverId", "hostId", "itemId"];
  var i, allParams = deparam(query), queryTable = {};
  for (i = 0; i < knownKeys.length; i++) {
    if (knownKeys[i] in allParams)
      queryTable[knownKeys[i]] = allParams[knownKeys[i]];
  }
  return queryTable;
};

HistoryView.prototype.parseItemQuery = function(query) {
  var knownKeys = ["serverId", "hostId", "itemId"];
  return this.parseQuery(query, knownKeys);
};

HistoryView.prototype.parseHistoryQuery = function(query) {
  var knownKeys = ["serverId", "hostId", "itemId", "beginTime", "endTime"];
  var startTime = new Date();
  queryTable = this.parseQuery(query, knownKeys);
  return queryTable;
};

HistoryView.prototype.showError = function(message) {
  hatoholErrorMsgBox(message);
};
