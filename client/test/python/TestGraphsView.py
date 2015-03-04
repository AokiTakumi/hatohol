#!/usr/bin/env python
"""
  Copyright (C) 2015 Project Hatohol

  This file is part of Hatohol.

  Hatohol is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License, version 3
  as published by the Free Software Foundation.

  Hatohol is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Hatohol. If not, see
  <http://www.gnu.org/licenses/>.
"""

import unittest
from django.http import HttpRequest
import json
import httplib
import urllib
from hatohol.models import Graph
from hatohol.views import graphs
from hatohol_server_emulator import HatoholServerEmulator
from hatohol import hatoholserver


class TestGraphsView(unittest.TestCase):

    def _setup_emulator(self):
        self._emulator = HatoholServerEmulator()
        self._emulator.start_and_wait_setup_done()

    def _request(self, method, id=None, body=None, POST=None):
        request = HttpRequest()
        request.method = method
        self._setSessionId(request)
        if body:
            request.META['CONTENT_TYPE'] = "application/x-www-form-urlencoded"
            request._body = urllib.urlencode(body)
        if POST:
            request.POST = POST
        return graphs(request, id)

    def _get(self, id=None):
        return self._request('GET', id=id)

    def _post(self, body=None):
        return self._request('POST', POST=body)

    def _put(self, id=None, body=None):
        return self._request('PUT', id=id, body=body)

    def _delete(self, id=None):
        return self._request('DELETE', id=id)

    def setUp(self):
        Graph.objects.all().delete()
        self._setup_emulator()

    def tearDown(self):
        if self._emulator is not None:
            self._emulator.shutdown()
            self._emulator.join()
            del self._emulator


class TestGraphsViewAuthorized(TestGraphsView):

    def _setSessionId(self, request):
        # The following session ID is just fake, because the request is
        # recieved in the above HatoholServerEmulatorHandler that
        # acutually doesn't verify it.
        request.META[hatoholserver.SESSION_NAME_META] = \
            'c579a3da-65db-44b4-a0da-ebf27548f4fd'

    def test_get(self):
        graph = Graph(
            user_id=5,
            settings_json='{"server_id":1,"host_id":2,"item_id":3}')
        graph.save()
        response = self._get(None)
        self.assertEquals(response.status_code, httplib.OK)
        record = {
            'id': graph.id,
            'user_id': 5,
            'server_id': 1,
            'host_id': 2,
            'item_id': 3,
        }
        self.assertEquals(json.loads(response.content),
                          [record])

    def test_get_without_own_record(self):
        graph = Graph(
            user_id=4,
            settings_json='{"server_id":1,"host_id":2,"item_id":3}')
        graph.save()
        response = self._get(None)
        self.assertEquals(response.status_code, httplib.OK)
        self.assertEquals(json.loads(response.content),
                          [])


class TestGraphsViewUnauthorized(TestGraphsView):

    def _setSessionId(self, request):
        pass  # Don't set session ID

    def test_get(self):
        response = self._get(None)
        self.assertEquals(response.status_code, httplib.FORBIDDEN)
