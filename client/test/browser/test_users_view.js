describe('UsersView', function() {
  var TEST_FIXTURE_ID = 'usersViewFixture';
  var usersViewHTML;
  var defaultUsers = [
    {
      "userId": 1,
      "name": "admin",
      "flags": hatohol.ALL_PRIVILEGES
    },
    {
      "userId": 2,
      "name": "guest",
      "flags": 0
    },
    {
      "userId": 2,
      "name": "users manager",
      "flags": (1 << hatohol.OPPRVLG_GET_ALL_USER |
                1 << hatohol.OPPRVLG_DELETE_USER)
    }
  ];

  function usersJson(users) {
    return JSON.stringify({
      apiVersion: 3,
      errorCode: hatohol.HTERR_OK,
      users: users ? users : [],
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

  function respond(usersJson) {
    var request = this.requests[0];
    request.respond(200, { "Content-Type": "application/json" },
                    usersJson);
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
      usersViewHTML = html;
      $("#" + TEST_FIXTURE_ID).append($("<div>", { id: "main" }))
      $("#main").html(usersViewHTML);
      fakeAjax();
      done();
    };

    if (usersViewHTML)
      setupFixture(usersViewHTML);
    else
      loadFixture("ajax_users", setupFixture)
  });

  afterEach(function() {
    restoreAjax();
    $("#" + TEST_FIXTURE_ID).remove();
  });

  it('Base elements', function() {
    var userProfile = new HatoholUserProfile(defaultUsers[0]);
    var view = new UsersView(userProfile);
    respond(usersJson(defaultUsers));
    var heads = $('div#' + TEST_FIXTURE_ID + ' h2');

    expect(heads).to.have.length(1);
    expect($('#table')).to.have.length(1);
    expect($('tr')).to.have.length(defaultUsers.length + 1);
  });

  it('with delete privilege', function() {
    var userProfile = new HatoholUserProfile(defaultUsers[2]);
    var view = new UsersView(userProfile);
    respond(usersJson(defaultUsers));

    expect($('#delete-user-button').is(":visible")).to.be(true);
    expect($('.delete-selector .selectcheckbox').is(":visible")).to.be(true);
  });

  it('with no delete privilege', function() {
    var userProfile = new HatoholUserProfile(defaultUsers[1]);
    var view = new UsersView(userProfile);
    respond(usersJson(defaultUsers));

    expect($('#delete-user-button').is(":visible")).to.be(false);
    expect($('.delete-selector .selectcheckbox').is(":visible")).to.be(false);
  });
});
