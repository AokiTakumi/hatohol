describe('ActionsView', function() {
  var TEST_FIXTURE_ID = 'actionsViewFixture';
  var actionsViewHTML;
  var defaultActions = [
    {
      "actionId": 1,
      "enableBits": 0,
      "serverId": null,
      "hostId": null,
      "hostgroupId": null,
      "triggerId": null,
      "triggerStatus": null,
      "triggerSeverity": null,
      "triggerSeverityComparatorType": 0,
      "type": 0,
      "workingDirectory": "",
      "command": "hatohol-actor-mail --to-address hoge@example.com",
      "timeout": 100,
      "ownerUserId": 1
    },
  ];

  function actionsJson(actions) {
    return JSON.stringify({
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK,
      actions: actions ? actions : [],
    });
  }

  function fakeAjax() {
    var requests = this.requests = [];
    this.xhr = sinon.useFakeXMLHttpRequest();
    this.xhr.onCreate = function(xhr) {
      requests.push(xhr);
    };
  }

  function restoreAjax() {
    this.xhr.restore();
  }

  function respond(actionsJson) {
    var request = this.requests[0];
    request.respond(200, { "Content-Type": "application/json" },
                    actionsJson);
  }

  function loadFixture(pathFromTop, onLoad) {
    var iframe = $("<iframe>", {
      id: "loaderFrame",
      src: "../../" + pathFromTop + "?start=false",
      load: function() {
        var html = $("#main", this.contentDocument).html();
        onLoad(html);
        $('#loaderFrame').remove()
      }
    })
    $('body').append(iframe);
  }

  beforeEach(function(done) {
    $('body').append($('<div>', { id: TEST_FIXTURE_ID }));
    var setupFixture = function(html) {
      actionsViewHTML = html;
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: "main" }))
      $("#main").html(actionsViewHTML);
      fakeAjax();
      done();
    };

    if (actionsViewHTML)
      setupFixture(actionsViewHTML);
    else
      loadFixture("ajax_actions", setupFixture)
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it('Base elements', function() {
    var userProfile = new HatoholUserProfile(defaultActions[0]);
    var view = new ActionsView(userProfile);
    respond(actionsJson(defaultActions));
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');

    expect(heads).to.have.length(1);
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(defaultActions.length + 1);
  });

  it('with delete privilege', function() {
    var operator = {
      "userId": 1,
      "name": "admin",
      "flags": (1 << hatohol.OPPRVLG_GET_ALL_ACTION |
                1 << hatohol.OPPRVLG_DELETE_ACTION)
    };
    var userProfile = new HatoholUserProfile(operator);
    var view = new ActionsView(userProfile);
    respond(actionsJson(defaultActions));

    var deleteButton = $('#delete-action-button');
    var checkboxes = $('.delete-selector .selectcheckbox');
    expect(deleteButton).to.have.length(1);
    expect(checkboxes).to.have.length(1);
    expect(deleteButton.is(":visible")).to.be(true);
    expect(checkboxes.is(":visible")).to.be(true);
  });

  it('with no delete privilege', function() {
    var operator = {
      "userId": 2,
      "name": "guest",
      "flags": 0
    };
    var userProfile = new HatoholUserProfile(operator);
    var view = new ActionsView(userProfile);
    respond(actionsJson(defaultActions));

    var deleteButton = $('#delete-action-button');
    var checkboxes = $('.delete-selector .selectcheckbox');
    expect(deleteButton).to.have.length(1);
    expect(checkboxes).to.have.length(1);
    expect(deleteButton.is(":visible")).to.be(false);
    expect(checkboxes.is(":visible")).to.be(false);
  });
});
