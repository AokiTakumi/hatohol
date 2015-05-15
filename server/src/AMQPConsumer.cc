/*
 * Copyright (C) 2014 Project Hatohol
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

#include "AMQPConsumer.h"
#include "AMQPConnection.h"
#include "AMQPConnectionInfo.h"
#include "AMQPMessageHandler.h"
#include <unistd.h>
#include <Logger.h>
#include <Reaper.h>
#include <StringUtils.h>
#include <amqp_tcp_socket.h>
#include <amqp_ssl_socket.h>

using namespace std;
using namespace mlpl;

class AMQPConsumerConnection : public AMQPConnection {
public:
	AMQPConsumerConnection(const AMQPConnectionInfo &info)
	: AMQPConnection(info)
	{
	}

	~AMQPConsumerConnection()
	{
	}

	bool consume(amqp_envelope_t &envelope)
	{
		amqp_maybe_release_buffers(getConnection());

		struct timeval timeout = {
			getTimeout(),
			0
		};
		const int flags = 0;
		amqp_rpc_reply_t reply = amqp_consume_message(getConnection(),
							      &envelope,
							      &timeout,
							      flags);
		switch (reply.reply_type) {
		case AMQP_RESPONSE_NORMAL:
			break;
		case AMQP_RESPONSE_LIBRARY_EXCEPTION:
			if (reply.library_error != AMQP_STATUS_TIMEOUT) {
				logErrorResponse("consume message", reply);
				disposeConnection();
			}
			return false;
		default:
			logErrorResponse("consume message", reply);
			disposeConnection();
			return false;
		}

		return true;
	}

private:
	bool initializeConnection() override
	{
		if (!AMQPConnection::initializeConnection())
			return false;
		if (!startConsuming())
			return false;
		return true;
	}

	bool startConsuming()
	{
		const amqp_bytes_t queue =
			amqp_cstring_bytes(getQueueName().c_str());
		const amqp_bytes_t consumer_tag = amqp_empty_bytes;
		const amqp_boolean_t no_local = false;
		const amqp_boolean_t no_ack = true;
		const amqp_boolean_t exclusive = false;
		const amqp_table_t arguments = amqp_empty_table;
		const amqp_basic_consume_ok_t *response;
		response = amqp_basic_consume(getConnection(),
					      getChannel(),
					      queue,
					      consumer_tag,
					      no_local,
					      no_ack,
					      exclusive,
					      arguments);
		if (!response) {
			const amqp_rpc_reply_t reply =
				amqp_get_rpc_reply(getConnection());
			if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
				logErrorResponse("start consuming", reply);
				return false;
			}
		}
		return true;
	}
};

AMQPConsumer::AMQPConsumer(const AMQPConnectionInfo &connectionInfo,
			   AMQPMessageHandler *handler)
: m_connectionInfo(connectionInfo),
  m_handler(handler)
{
}

AMQPConsumer::~AMQPConsumer()
{
}

gpointer AMQPConsumer::mainThread(HatoholThreadArg *arg)
{
	AMQPConsumerConnection connection(m_connectionInfo);
	while (!isExitRequested()) {
		if (!connection.isConnected()) {
			connection.connect();
		}

		if (!connection.isConnected()) {
			sleep(1); // TODO: Make retry interval customizable
			continue;
		}

		amqp_envelope_t envelope;
		Reaper<amqp_envelope_t> envelopeReaper(&envelope,
						       amqp_destroy_envelope);
		const bool consumed = connection.consume(envelope);
		if (!consumed)
			continue;

		m_handler->handle(&envelope);
	}
	return NULL;
}
