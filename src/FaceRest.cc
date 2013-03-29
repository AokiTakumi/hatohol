/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Logger.h>
using namespace mlpl;

#include "FaceRest.h"
#include "JsonBuilderAgent.h"
#include "AsuraException.h"
#include "ConfigManager.h"

typedef void (*RestHandler)
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, gpointer user_data);

static const guint DEFAULT_PORT = 33194;

const char *FaceRest::pathForGetServers = "/servers";
const char *FaceRest::pathForGetTriggers = "/triggers";

static const char *MIME_JSON = "application/json";

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
FaceRest::FaceRest(CommandLineArg &cmdArg)
: m_port(DEFAULT_PORT),
  m_soupServer(NULL)
{
	for (size_t i = 0; i < cmdArg.size(); i++) {
		string &cmd = cmdArg[i];
		if (cmd == "--face-rest-port")
			i = parseCmdArgPort(cmdArg, i);
	}
	MLPL_INFO("started face-rest, port: %d\n", m_port);
}

FaceRest::~FaceRest()
{
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer FaceRest::mainThread(AsuraThreadArg *arg)
{
	m_soupServer = soup_server_new(SOUP_SERVER_PORT, m_port, NULL);
	soup_server_add_handler(m_soupServer, NULL, handlerDefault, this, NULL);
	soup_server_add_handler(m_soupServer, pathForGetServers,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetServers, NULL);
	soup_server_add_handler(m_soupServer, pathForGetTriggers,
	                        launchHandlerInTryBlock,
	                        (gpointer)handlerGetTriggers, NULL);
	soup_server_run(m_soupServer);
	MLPL_INFO("exited face-rest\n");
	return NULL;
}

size_t FaceRest::parseCmdArgPort(CommandLineArg &cmdArg, size_t idx)
{
	if (idx == cmdArg.size() - 1) {
		MLPL_ERR("needs port number.");
		return idx;
	}

	idx++;
	string &port_str = cmdArg[idx];
	int port = atoi(port_str.c_str());
	if (port < 0 || port > 65536) {
		MLPL_ERR("invalid port: %s, %d\n", port_str.c_str(), port);
		return idx;
	}

	m_port = port;
	return idx;
}

void FaceRest::replyError(SoupMessage *msg, const string &errorMessage)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

// handlers
void FaceRest::handlerDefault(SoupServer *server, SoupMessage *msg,
                              const char *path, GHashTable *query,
                              SoupClientContext *client, gpointer user_data)
{
	MLPL_DBG("Default handler: path: %s, method: %s\n",
	         path, msg->method);
	soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
}

void FaceRest::launchHandlerInTryBlock
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, gpointer user_data)
{
	RestHandler handler = reinterpret_cast<RestHandler>(user_data);
	try {
		(*handler)(server, msg, path, query, client, user_data);
	} catch (const AsuraException &e) {
		MLPL_INFO("Got Exception: %s\n", e.getFancyMessage().c_str());
		replyError(msg, e.getFancyMessage());
	}
}

void FaceRest::handlerGetServers
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, gpointer user_data)
{
	soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
	ConfigManager *configManager = ConfigManager::getInstance();

	MonitoringServerInfoList monitoringServers;
	configManager->getTargetServers(monitoringServers);

	JsonBuilderAgent agent;
	agent.startObject();
	agent.addTrue("result");
	agent.add("numberOfServers", monitoringServers.size());
	agent.startArray("servers");
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	for (; it != monitoringServers.end(); ++it) {
		MonitoringServerInfo &serverInfo = *it;
		agent.startObject();
		agent.add("id", serverInfo.id);
		agent.add("type", serverInfo.type);
		agent.add("hostName", serverInfo.hostName);
		agent.add("ipAddress", serverInfo.ipAddress);
		agent.add("nickname", serverInfo.nickname);
		agent.endObject();
	}
	agent.endArray();
	agent.endObject();
	string response = agent.generate();
	soup_message_headers_set_content_type(msg->response_headers,
	                                      MIME_JSON, NULL);
	soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY,
	                         response.c_str(), response.size());
	soup_message_set_status(msg, SOUP_STATUS_OK);
}

void FaceRest::handlerGetTriggers
  (SoupServer *server, SoupMessage *msg, const char *path,
   GHashTable *query, SoupClientContext *client, gpointer user_data)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
}
