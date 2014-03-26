describe('HatoholMonitoringView', function() {

  var testDivId = "testDiv";
  afterEach(function(done) {
    $("#" + testDivId).remove();
    done();
  });

  function makeCheckBoxTestDiv() {
    var s = '';
    s += '<div id="' + testDivId + '">"';
    s += '  <div class="delete-selector" id="checkbox-div" style="display:none;">';
    s += '    <input type="checkbox" class="selectcheckbox" id="checkbox1">';
    s += '    <input type="checkbox" class="selectcheckbox" id="checkbox2">';
    s += '  </td>';
    s += '  <button id="delete-test-button" type="button" disabled style="display: none;">';
    s += '</div>';
    $("body").append(s);
  }

  function makeTestObject(flags) {
    var user = {flags: flags};
    var testObj = {userProfile: new HatoholUserProfile(user)};
    return testObj;
  }

  // ------------------------------------------------------------------------------------
  // Test cases
  // ------------------------------------------------------------------------------------
  it('set filter candidates', function() {
    var candidates = [
      { label: 'apple',  value: 1 },
      { label: 'orange', value: 2 },
      { label: 'lemon',  value: 3 }
    ];
    var target = $('<select>');
    var setCandidates = HatoholMonitoringView.prototype.setFilterCandidates;
    var expected = '<option>---------</option>';
    var i;
    for (i = 0; i < candidates.length; i++) {
      expected += '<option value="' + candidates[i].value + '">' +
                  candidates[i].label + "</option>";
    }
    setCandidates(target, candidates);
    expect(target.html()).to.be(expected);
  });

  it('set filter candidates with a string array', function() {
    var candidates = [ 'apple', 'orange', 'lemon' ];
    var target = $('<select>');
    var setCandidates = HatoholMonitoringView.prototype.setFilterCandidates;
    var expected = '<option>---------</option>';
    var i;
    for (i = 0; i < candidates.length; i++)
      expected += '<option>' + candidates[i] + "</option>";
    setCandidates(target, candidates);
    expect(target.html()).to.be(expected);
  });

  it('set empty filter', function() {
    var target = $('<select>');
    var setCandidates = HatoholMonitoringView.prototype.setFilterCandidates;
    setCandidates(target);
    var expected = '<option>---------</option>';
    expect(target.html()).to.be(expected);
  });

  it ('soon after calling setupCheckboxForDelete', function() {

    makeCheckBoxTestDiv();
    $('#delete-test-button').attr("disabled", "");
    $('#checkbox1').val(true);

    var user = {flags: 1 << hatohol.OPPRVLG_DELETE_SERVER};
    var testObj = {userProfile: new HatoholUserProfile(user)};
    HatoholMonitoringView.prototype.setupCheckboxForDelete.apply(
     testObj, [$('#delete-test-button')]);
    expect($('#delete-test-button').attr("disabled")).to.be("disabled");
    expect($('#checkbox1').prop('checked')).to.be(false);
    expect($('#checkbox2').prop('checked')).to.be(false);
    expect($('#checkbox-div').css('display')).to.be('block');
  });

  it ('not show check boxes if the user does not have the privilege', function() {

    makeCheckBoxTestDiv();
    $('#delete-test-button').attr("disabled", "");
    HatoholMonitoringView.prototype.setupCheckboxForDelete.apply(
     makeTestObject(0), [$('#delete-test-button')]);
    expect($('#checkbox-div').css('display')).to.be('none');
  });
});
