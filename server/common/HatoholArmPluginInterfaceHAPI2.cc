/*
 * Copyright (C) 2015 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "AMQPMessageHandler.h"
#include "AMQPConnectionInfo.h"
#include "AMQPConsumer.h"
#include "AMQPPublisher.h"
#include "HAPI2Procedure.h"
#include "HatoholArmPluginInterfaceHAPI2.h"
#include "JSONBuilder.h"

using namespace std;
using namespace mlpl;

// Invalid JSON was received by the server.
// An error occurred on the server while parsing the JSON text.
const int JSON_RPC_PARSE_ERROR = -32700;

// The JSON sent is not a valid Request object.
const int JSON_RPC_INVALID_REQUEST  = -32600;

// The method does not exist / is not available.
const int JSON_RPC_METHOD_NOT_FOUND = -32601;

//  Invalid method parameter(s).
const int JSON_RPC_INVALID_PARAMS = -32602;

// Internal JSON-RPC error.
const int JSON_RPC_INTERNAL_ERROR = -32603;

// Reserved for implementation-defined server-errors.
const int JSON_RPC_SERVER_ERROR_BEGIN = -32000;
const int JSON_RPC_SERVER_ERROR_END = -32099;

class HatoholArmPluginInterfaceHAPI2::AMQPHAPI2MessageHandler
  : public AMQPMessageHandler
{
public:
	AMQPHAPI2MessageHandler(HatoholArmPluginInterfaceHAPI2 &hapi2)
	: m_hapi2(hapi2)
	{
	}

	bool handle(AMQPConnection &connection, const AMQPMessage &message)
	{
		MLPL_DBG("message: <%s>/<%s>\n",
			 message.contentType.c_str(),
			 message.body.c_str());

		JsonParser *parser = json_parser_new();
		GError *error = NULL;
		if (json_parser_load_from_data(parser,
					       message.body.c_str(),
					       message.body.size(),
					       &error)) {
			process(connection, json_parser_get_root(parser), message.body);
		} else {
			g_error_free(error);
		}
		g_object_unref(parser);
		return true;
	}

private:
	void process(AMQPConnection &connection, JsonNode *root, const string &body)
	{
		HAPI2Procedure procedure(root);
		StringList errors;
		if (!procedure.validate(errors)) {
			for (auto errorMessage : errors) {
				MLPL_ERR("%s\n", errorMessage.c_str());
			}
			return;
		}
		HAPI2ProcedureType type = procedure.getType();
		AMQPJSONMessage message;
		message.body = m_hapi2.interpretHandler(type, body);
		bool succeeded = connection.publish(message);
		if (!succeeded) {
			// TODO: retry?
		}
		m_hapi2.onHandledCommand(type);
	}

private:
	HatoholArmPluginInterfaceHAPI2 &m_hapi2;
};

struct HatoholArmPluginInterfaceHAPI2::Impl
{
	ArmPluginInfo m_pluginInfo;
	HatoholArmPluginInterfaceHAPI2 &hapi2;
	ProcedureHandlerMap procedureHandlerMap;
	AMQPConnectionInfo m_connectionInfo;
	AMQPConsumer *m_consumer;
	AMQPHAPI2MessageHandler *m_handler;

	Impl(HatoholArmPluginInterfaceHAPI2 &_hapi2)
	: hapi2(_hapi2),
	  m_consumer(NULL),
	  m_handler(NULL)
	{
	}

	~Impl()
	{
		if (m_consumer) {
			m_consumer->exitSync();
			delete m_consumer;
		}
		delete m_handler;
	}

	void setArmPluginInfo(const ArmPluginInfo &armPluginInfo)
	{
		m_pluginInfo = armPluginInfo;
	}

	void setupAMQPConnectionInfo(void)
	{
		if (!m_pluginInfo.brokerUrl.empty())
			m_connectionInfo.setURL(m_pluginInfo.brokerUrl);
		string queueName;
		if (m_pluginInfo.staticQueueAddress.empty())
			queueName = generateQueueName(m_pluginInfo);
		else
			queueName = m_pluginInfo.staticQueueAddress;
		m_connectionInfo.setQueueName(queueName);

		m_connectionInfo.setTLSCertificatePath(
			m_pluginInfo.tlsCertificatePath);
		m_connectionInfo.setTLSKeyPath(
			m_pluginInfo.tlsKeyPath);
		m_connectionInfo.setTLSCACertificatePath(
			m_pluginInfo.tlsCACertificatePath);
		m_connectionInfo.setTLSVerifyEnabled(
			m_pluginInfo.isTLSVerifyEnabled());
	}

	void setupAMQPConnection(void)
	{
		setupAMQPConnectionInfo();

		m_handler = new AMQPHAPI2MessageHandler(hapi2);
		m_consumer = new AMQPConsumer(m_connectionInfo, m_handler);
	}

	string generateQueueName(const ArmPluginInfo &pluginInfo)
	{
		return StringUtils::sprintf("hapi2.%" FMT_SERVER_ID,
					    pluginInfo.serverId);
	}

	void start(void)
	{
		if (!m_consumer)
			return;
		setupAMQPConnection();
		m_consumer->start();
	}

	string buildErrorReply(const int errorCode,
			       const string errorMessage)
	{
		JSONBuilder agent;
		agent.startObject();
		agent.add("jsonrpc", "2.0");
		agent.add("id", 1); // TODO: Use random number
		agent.startObject("error");
		agent.add("code", errorCode);
		agent.add("message", errorMessage);
		agent.endObject(); // error
		return agent.generate();
	}
};

HatoholArmPluginInterfaceHAPI2::HatoholArmPluginInterfaceHAPI2()
: m_impl(new Impl(*this))
{
}

HatoholArmPluginInterfaceHAPI2::~HatoholArmPluginInterfaceHAPI2()
{
}

void HatoholArmPluginInterfaceHAPI2::setArmPluginInfo(
  const ArmPluginInfo &pluginInfo)
{
	m_impl->setArmPluginInfo(pluginInfo);
}

void HatoholArmPluginInterfaceHAPI2::registerProcedureHandler(
  const HAPI2ProcedureType &type, ProcedureHandler handler)
{
	m_impl->procedureHandlerMap[type] = handler;
}

string HatoholArmPluginInterfaceHAPI2::interpretHandler(
  const HAPI2ProcedureType &type, const string json)
{
	ProcedureHandlerMapConstIterator it =
	  m_impl->procedureHandlerMap.find(type);
	if (it == m_impl->procedureHandlerMap.end()) {
		// TODO: Add a supplied method name
		string message = StringUtils::sprintf("Method not found");
		return m_impl->buildErrorReply(JSON_RPC_METHOD_NOT_FOUND,
					       message);
	}
	ProcedureHandler handler = it->second;
	return (this->*handler)(type, json);
}

void HatoholArmPluginInterfaceHAPI2::onHandledCommand(const HAPI2ProcedureType &type)
{
}

void HatoholArmPluginInterfaceHAPI2::start(void)
{
	m_impl->start();
}

void HatoholArmPluginInterfaceHAPI2::send(const std::string &message)
{
	// TODO: Should use only one conection per one thread
	AMQPPublisher publisher(m_impl->m_connectionInfo);
	AMQPJSONMessage amqpMessage;
	amqpMessage.body = message;
	publisher.setMessage(amqpMessage);
	publisher.publish();
}

mt19937 HatoholArmPluginInterfaceHAPI2::getRandomEngine(void)
{
	std::random_device rd;
	std::mt19937 engine(rd());
	return engine;
}
