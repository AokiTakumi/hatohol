describe('HatoholServerEditDialog', function() {
  var dialog;

  beforeEach(function() {
    dialog = undefined;
  });

  afterEach(function() {
    if (dialog)
      dialog.closeDialog();
  });

  it('new with empty params', function() {
    var expectedId = "#server-edit-dialog";
    dialog = new HatoholServerEditDialog({});
    expect(dialog).not.to.be(undefined);
    expect($(expectedId)).to.have.length(1);
    var buttons = $(expectedId).dialog("option", "buttons");
    expect(buttons).to.have.length(2);
    expect(buttons[0].text).to.be(gettext("ADD"));

    expect(parseInt($("#selectServerType").val())).to.be(0);
    expect($("#inputNickName").val()).to.be.empty();
    expect($("#inputHostName").val()).to.be.empty();
    expect($("#inputIpAddress").val()).to.be.empty();
    expect(parseInt($("#inputPort").val())).to.be(80);
  });

  it('new with a nagios server', function() {
    var expectedId = "#server-edit-dialog";
    var server = {
      id: 1,
      type: hatohol.MONITORING_SYSTEM_NAGIOS,
      hostName: "localhost",
      ipAddress: "127.0.0.1",
      nickname: "MySelf",
      port: 3306
    };
    dialog = new HatoholServerEditDialog({
      targetServer: server
    });
    expect(dialog).not.to.be(undefined);
    expect($(expectedId)).to.have.length(1);
    var buttons = $(expectedId).dialog("option", "buttons");
    expect(buttons).to.have.length(2);
    expect(buttons[0].text).to.be(gettext("APPLY"));
    expect(parseInt($("#selectServerType").val())).to.be(server.type);
    expect($("#inputNickName").val()).to.be(server.nickname);
    expect($("#inputHostName").val()).to.be(server.hostName);
    expect($("#inputIpAddress").val()).to.be(server.ipAddress);
    expect(parseInt($("#inputPort").val())).to.be(server.port);
  });

  it('DB name visibility', function() {
    dialog = new HatoholServerEditDialog({});
    expect($("#dbNameArea").css("display")).to.be("none");
    $("#selectServerType").val(1).change();
    expect($("#dbNameArea").css("display")).not.to.be("none");
  });
});
