describe("HatoholReplyParser", function() {
  it("null", function() {
    var parser = new HatoholReplyParser(null);
    var stat = parser.getStatus();
    expect(stat).to.be(REPLY_STATUS.NULL_OR_UNDEFINED);
  });

  it("undefined", function() {
    var parser = new HatoholReplyParser(undefined);
    var stat = parser.getStatus();
    expect(stat).to.be(REPLY_STATUS.NULL_OR_UNDEFINED);
  });

  it("not found apiVersion", function() {
    var reply = {"errorCode":0};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    expect(stat).to.be(REPLY_STATUS.NOT_FOUND_API_VERSION);
  });

  it("not suport API version", function() {
    var reply = {"apiVersion":1};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    expect(stat).to.be(REPLY_STATUS.UNSUPPORTED_API_VERSION);
  });

  it("not found errorCode", function() {
    var reply = {"apiVersion":hatohol.FACE_REST_API_VERSION};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    var message = parser.getStatusMessage();
    var expectedMessage = gettext("Not found errorCode.");
    expect(stat).to.be(REPLY_STATUS.NOT_FOUND_ERROR_CODE);
    expect(message).to.be(expectedMessage);
  });

  it("errorCode is not OK", function() {
    var reply = {"apiVersion":hatohol.FACE_REST_API_VERSION,
                 errorCode:hatohol.HTERR_UNKOWN_REASON};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    expect(stat).to.be(REPLY_STATUS.ERROR_CODE_IS_NOT_OK);
  });

  it("get error code", function() {
    var reply = {"apiVersion":hatohol.FACE_REST_API_VERSION,
                 "errorCode":hatohol.HTERR_ERROR_TEST};
    var parser = new HatoholReplyParser(reply);
    var stat = parser.getStatus();
    var errorCode = parser.getErrorCode();
    expect(stat).to.be(REPLY_STATUS.ERROR_CODE_IS_NOT_OK);
    expect(errorCode).to.be(hatohol.HTERR_ERROR_TEST);
  });
});

describe("HatoholLoginReplyParser", function() {
  it("no sessionId", function() {
    var reply = {
      "apiVersion": hatohol.FACE_REST_API_VERSION,
      "errorCode": hatohol.HTERR_OK
    };
    var parser = new HatoholLoginReplyParser(reply);
    var stat = parser.getStatus();
    expect(stat).to.be(REPLY_STATUS.NOT_FOUND_SESSION_ID);
  });
});
