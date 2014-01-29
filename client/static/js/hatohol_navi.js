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

var HatoholNavi = function(user, currentPage) {
  var i, title, klass;
  var menuItems = [
    {
      title: gettext("Dashboard"),
      href:  "ajax_dashboard"
    },
    {
      title: gettext("Overview : Triggers"),
      href:  "ajax_overview_triggers"
    },
    {
      title: gettext("Overview : Items"),
      href:  "ajax_overview_items"
    },
    {
      title: gettext("Latest data"),
      href:  "ajax_latest"
    },
    {
      title: gettext("Triggers"),
      href:  "ajax_triggers" },
    {
      title: gettext("Events"),
      href:  "ajax_events"
    },
    {
      title: gettext("Servers"),
      href:  "ajax_servers"
    },
    {
      title: gettext("Actions"),
      href:  "ajax_actions"
    },
    {
      title: gettext("Users"),
      href:  "ajax_users",
      flags: (1 << hatohol.OPPRVLG_CREATE_USER) |
             (1 << hatohol.OPPRVLG_UPDATE_USER) |
             (1 << hatohol.OPPRVLG_DELETE_USER) |
             (1 << hatohol.OPPRVLG_GET_ALL_USERS)
    },
  ];
  var matchResults;

  if (currentPage) {
    this.currentPage = currentPage;
  } else if (location.pathname.match(".*/$")) {
    this.currentPage = menuItems[0].href;
  } else {
    matchResults = location.pathname.match(".*/(.+)$");
    if (matchResults && matchResults.length > 1)
      this.currentPage = matchResults[1];
    else
      this.currentPage = location.pathname;
  }

  for (i = 0; i < menuItems.length; ++i) {
    if (menuItems[i].flags != undefined &&
        !hasFlags(user, menuItems[i].flags))
    {
      continue;
    }
    if (menuItems[i].href == this.currentPage) {
      title = '<a>' + menuItems[i].title + '</a>';
      klass = "active";
    } else {
      title = '<a href=' + menuItems[i].href + '>' + menuItems[i].title + '</a>';
      klass = undefined;
    }
    $("<li/>", {
      html: title,
      class: klass,
    }).appendTo("ul.nav:first");
  };
};
