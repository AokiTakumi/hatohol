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

/**
 * Show a message box.
 *
 * @param msg
 * A message string.
 * 
 * @param param
 * A parameter object. Available elements are the following.
 *
 * title:
 * A title string. If this is undefined, the title is not shown.
 *
 * buttons:
 * A button definition array passed to jQuery dialog. If this is undefined,
 * a default button labeled 'CLOSE' (or the translated word) is shown.
 *
 * defaultButtonLabel:
 * A label on the default button. If the above 'buttons' is specified,
 * this parameter is not used.
 *
 * id:
 * An ID of the dialog. If the dialog with 'id' exists, it is reused.
 * For example, a displayed message is replaced.
 *
 */
var HatoholMessageBox = function(msg, param) {

  var self = this;

  var id = getId();
  self.msgDivId = "#" + id
  var msgDiv = $(self.msgDivId)[0];
  if (!msgDiv) {
    var div = "<div id='" + id + "'>" + msg + "</div>";
    $("body").append(div);
  } else  {
    $(msgDiv).text(msg);
  }

  var title = getTitle();
  var buttons = getButtons();
  $(self.msgDivId).dialog({
    autoOpen: false,
    title: title,
    closeOnEscape: false,
    modal: true,
    buttons: buttons,
    open: function(event, ui){
      $(".ui-dialog-titlebar-close").hide();
      if (!param || !param.title)
        $(".ui-dialog-titlebar").hide();
    }
  });

  $(self.msgDivId).dialog("open");
  HatoholDialogObserver.notifyCreated(id, this);

  function getId() {
    if (!param || !param.id)
      return self.getDefaultId();
    return param.id;
  }

  function getTitle() {
    var DEFAULT_TITLE_BAR_STRING = gettext("MessageBox");
    if (!param || !param.title)
      return DEFAULT_TITLE_BAR_STRING;
    return param.title;
  }

  function getButtons() {
    if (!param || !param.buttons)
      return getDefualtButtons();
    return param.buttons;
  }

  function getDefaultButtonLabel() {
    var DEFAULT_LABEL = gettext("CLOSE");
    if (!param || !param.defaultButtonLabel)
      return DEFAULT_LABEL;
    return param.defaultButtonLabel;
  }

  function getDefualtButtons(label) {
    if (!label)
      label = getDefaultButtonLabel();
    return [{
      text: label,
      click: function() {
        $(this).dialog("destroy");
        $(self.msgDivId).remove();
      }
    }];
  }
};

HatoholMessageBox.prototype.getDefaultId = function () {
  var DEFAULT_ID = "hatohol-message-box";
  return DEFAULT_ID;
}

HatoholMessageBox.prototype.getMessage = function () {
  return $(this.msgDivId).text();
}

function hatoholInfoMsgBox(msg) {
  var param = {title: gettext("Information")};
  new HatoholMessageBox(msg, param);
};

function hatoholWarnMsgBox(msg) {
  var param = {title: gettext("Warning")};
  new HatoholMessageBox(msg, param);
};

function hatoholErrorMsgBox(msg) {
  var param = {title: gettext("Error")};
  new HatoholMessageBox(msg, param);
};

function hatoholMsgBoxForParser(reply, parser, title) {
  var msg = gettext("Failed to parse the result. status code: ");
  msg += parser.getStatus();
  hatoholErrorMsgBox(msg);
};

function hatoholErrorMsgBoxForParser(reply, parser) {
  hatoholMsgBoxForParser(reply, parser, gettext("Error"));
};
