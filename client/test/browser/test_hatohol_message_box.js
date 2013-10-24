describe('HatoholMessageBox', function() {

  var msgbox;
  beforeEach(function(done) {
    msgbox = undefined;
    done();
  });

  afterEach(function(done) {
    HatoholDialogObserver.reset();
    if (msgbox)
      msgbox.destroy();
    done();
  });

  it('passes only the first (message) argument', function(done) {
    var msg = "Test message.";
    HatoholDialogObserver.registerCreatedCallback(function(id, obj) {
      if (!("getDefaultId" in obj))
        return;
      if (id != obj.getDefaultId())
        return;

      // message
      expect(obj.getMessage()).to.be(msg);

      // title bar
      expect(obj.isTitleBarVisible()).to.be(false);
      expect(obj.getTitleString()).to.be(obj.getDefaultTitleString());

      // button
      var buttons = obj.getButtons();
      expect(buttons).to.have.length(1);
      var button = buttons[0];
      expect(button.text).to.be(obj.getDefaultButtonLabel());

      done();
    });
    msgbox = new HatoholMessageBox(msg);
  });
});
