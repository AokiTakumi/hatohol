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

var EventsView = function() {
  $("#table").stupidtable();
  $("#table").bind('aftertablesort', function(event, data) {
    var th = $(this).find("th");
    th.find("i.sort").remove();
    var icon = data.direction === "asc" ? "up" : "down";
    th.eq(data.column).append("<i class='sort icon-arrow-" + icon +"'></i>");
  });

  var status_choices = [gettext('OK'), gettext('Problem'), gettext('Unknown')];
  var severity_choices = [
    gettext('Not classified'), gettext('Information'), gettext('Warning'),
    gettext('Average'), gettext('High'), gettext('Disaster')];

  $("#select-server").change(function() {
    chooseRow();
  });

  startConnection('event', updateCore);

  //
  // Private functions 
  //
  function parseData(rd) {
    var pd = new Object();
    var o, server, triggerId;
    var x;
    var ts, ss;
    var list, map;

    pd.diffs = new Object();

    ts = new Object();
    for (x = 0; x < rd["events"].length; ++x) {
      o = rd["events"][x];
      server    = rd["servers"][o["serverId"]]["name"];
      triggerId = o["triggerId"];
      if ( !(server in ts) ) {
        ts[server] = new Object();
      }
      if ( !(triggerId in ts[server]) ) {
        ts[server][triggerId] = [];
      }
      ts[server][triggerId].push(o["time"]);
    }

    ss = [];
    for ( server in ts ) {
      ss.push(server);
      for ( triggerId in ts[server] ) {
        list = ts[server][triggerId].uniq().sort();
        map = new Object();
        for (x = 0 ; x < list.length; ++x) {
          if ( x + 1 < list.length ) {
            map[list[x]] = Number(list[x + 1]) - Number(list[x]);
          }
          else {
            map[list[x]] = (new Date()).getTime() / 1000 - Number(list[x]);
          }
        }
        ts[server][triggerId] = map;
      }
      pd.diffs[server] = ts[server];
    }
    pd.servers = ss.sort();

    return pd;
  }

  function drawTableBody(rd, pd) {
    var s = "";
    var o;
    var x;
    var klass, server, host, clock, status, severity, diff;

    for (x = 0; x < rd["events"].length; ++x) {
      o = rd["events"][x];
      server   = rd["servers"][o["serverId"]]["name"];
      host     = rd["servers"][o["serverId"]]["hosts"][o["hostId"]]["name"];
      clock    = o["time"];
      status   = o["type"];
      severity = o["severity"];
      diff     = pd.diffs[server][o["triggerId"]][clock];
      klass    = server;
      s += "<tr class='" + klass + "'>";
      s += "<td>" + server + "</td>";
      s += "<td data-sort-value='" + clock + "'>" + formatDate(clock) + "</td>";
      s += "<td>" + host + "</td>";
      s += "<td>" + o["brief"] + "</td>";
      s += "<td class='status" + status + "' data-sort-value='" + status + "'>" + status_choices[Number(status)] + "</td>";
      s += "<td class='severity" + severity + "' data-sort-value='" + severity + "'>" + severity_choices[Number(severity)] + "</td>";
      s += "<td data-sort-value='" + diff + "'>" + formatSecond(diff) + "</td>";
      s += "<td>" + "unsupported" + "</td>";
      s += "<td>" + "unsupported" + "</td>";
      s += "</tr>";
    }

    return s;
  }

  function updateCore(reply) {
    var rawData = reply;
    var parsedData = parseData(rawData);

    setCandidate($("#select-server"), parsedData.servers);

    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(rawData, parsedData));
  }
};
