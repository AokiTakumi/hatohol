/*
 * Copyright (C) 2015 Project Hatohol
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

describe('TriggersView', function() {
  var TEST_FIXTURE_ID = 'triggersViewFixture';
  var triggersViewHTML;

  var defaultTriggers = [
    {
      "id": "18446744073709550616",
      "status": 0,
      "severity": 5,
      "lastChangeTime": 1422584694,
      "serverId": 1,
      "hostId": "18446744073709551612",
      "brief": "Failed in connecting to Zabbix."
    },
    {
      "id": "14441",
      "status": 0,
      "severity": 1,
      "lastChangeTime": 0,
      "serverId": 1,
      "hostId": "10106",
      "brief": "Host name of zabbix_agentd was changed on {HOST.NAME}",
      "expandedDescription": "Host name of zabbix_agentd was changed on TestHost0"
    }
  ];
  var defaultServers = {
    "1": {
      "nickname": "Zabbix",
      "type": 0,
      "ipAddress": "192.168.1.100",
      "hosts": {
        "10101": {
          "name": "Host1",
        },
      },
    },
  };

  function triggersJson(triggers, servers) {
    return JSON.stringify({
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK,
      triggers: triggers ? triggers : [],
      servers: servers ? servers : {}
    });
  }

  function fakeAjax() {
    var requests = this.requests = [];
    this.xhr = sinon.useFakeXMLHttpRequest();
    this.xhr.onCreate = function(xhr) {
      requests.push(xhr);
    };
  }

  function respond(configJson, triggersJson) {
    var header = { "Content-Type": "application/json" };
    this.requests[0].respond(200, header, configJson);
    this.requests[1].respond(200, header, triggersJson);
  }

  function restoreAjax() {
    this.xhr.restore();
  }

  function loadFixture(pathFromTop, onLoad) {
    var iframe = $("<iframe>", {
      id: "loaderFrame",
      src: "../../" + pathFromTop + "?start=false",
      load: function() {
        var html = $("#main", this.contentDocument).html();
        onLoad(html);
        $('#loaderFrame').remove();
      }
    });
    $('body').append(iframe);
  }

  beforeEach(function(done) {
    $('body').append($('<div>', { id: TEST_FIXTURE_ID }));
    var setupFixture = function(html) {
      triggersViewHTML = html;
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: "main" }));
      $("#main").html(triggersViewHTML);
      fakeAjax();
      done();
    };

    if (triggersViewHTML)
      setupFixture(triggersViewHTML);
    else
      loadFixture("ajax_triggers", setupFixture);
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  // -------------------------------------------------------------------------
  // Test cases
  // -------------------------------------------------------------------------

  it('new with empty data', function() {
    var view = new TriggersView($('#' + TEST_FIXTURE_ID).get(0));
    var heads = $("div#" + TEST_FIXTURE_ID + " h2");
    respond('{}', triggersJson());

    expect(heads).to.have.length(1);
    expect($("#table")).to.have.length(1);
    expect($("tr")).to.have.length(1);
  });

  it('Base elements', function() {
    var view = new TriggersView($('#' + TEST_FIXTURE_ID).get(0));
    var heads = $("div#" + TEST_FIXTURE_ID + " h2");
    respond('{}', triggersJson(defaultTriggers));

    expect(heads).to.have.length(1);
    expect($("#table")).to.have.length(1);
    expect($("tr")).to.have.length(defaultTriggers.length + 1);
  });
});
