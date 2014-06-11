/*
 * Copyright (C) 2014 Project Hatohol
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

#include <cstring>
#include <MutexLock.h>
#include <SmartBuffer.h>
#include <SmartTime.h>
#include <Reaper.h>
#include <qpid/messaging/Address.h>
#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Session.h>
#include "HatoholArmPluginInterface.h"
#include "HatoholException.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

const char *HatoholArmPluginInterface::DEFAULT_BROKER_URL = "localhost:5672";
const uint32_t HatoholArmPluginInterface::SEQ_ID_UNKNOWN = UINT32_MAX;
const uint32_t HatoholArmPluginInterface::SEQ_ID_MAX     = UINT32_MAX - 1;

struct HatoholArmPluginInterface::PrivateContext {
	HatoholArmPluginInterface *hapi;
	bool       workInServer;
	string     queueAddr;
	Connection connection;
	Session    session;
	Sender     sender;
	Receiver   receiver;
	bool       initiated;
	uint64_t   initiationKey;
	Message   *currMessage;
	CommandHandlerMap receiveHandlerMap;
	string     receiverAddr;
	uint32_t   sequenceId;
	uint32_t   sequenceIdOfCurrCmd;

	PrivateContext(HatoholArmPluginInterface *_hapi,
	               const string &_queueAddr,
	               const bool &_workInServer)
	: hapi(_hapi),
	  workInServer(_workInServer),
	  queueAddr(_queueAddr),
	  initiated(false),
	  initiationKey(0),
	  currMessage(NULL),
	  sequenceId(0),
	  sequenceIdOfCurrCmd(SEQ_ID_UNKNOWN),
	  connected(false)
	{
	}

	virtual ~PrivateContext()
	{
		disconnect();
	}

	void setQueueAddress(const string &_queueAddr)
	{
		queueAddr = _queueAddr;
	}

	void connect(void)
	{
		const string brokerUrl = DEFAULT_BROKER_URL;
		const string connectionOptions;
		connectionLock.lock();
		Reaper<MutexLock> unlocker(&connectionLock, MutexLock::unlock);
		connection = Connection(brokerUrl, connectionOptions);
		connection.open();
		session = connection.createSession();

		string queueAddrS = queueAddr + "-S"; // Plugin -> Hatohol
		string queueAddrT = queueAddr + "-T"; // Plugin <- Hatohol
		if (workInServer) {
			queueAddrS += "; {create: always}";
			queueAddrT += "; {create: always}";
			sender = session.createSender(queueAddrT);
			receiver = session.createReceiver(queueAddrS);
			receiverAddr = queueAddrS;
		} else {
			sender = session.createSender(queueAddrS);
			receiver = session.createReceiver(queueAddrT);
			receiverAddr = queueAddrT;
		}
		connected = true;
		hapi->onConnected(connection);
		hapi->onSessionChanged(&session);
	}

	void disconnect(void)
	{
		connectionLock.lock();
		Reaper<MutexLock> unlocker(&connectionLock, MutexLock::unlock);
		if (!connected)
			return;
		try {
			session.sync();
			session.close();
			connection.close();
		} catch (...) {
		}
		connected = false;
		hapi->onSessionChanged(NULL);
	}

	void acknowledge(void)
	{
		Reaper<MutexLock> unlocker(&connectionLock, MutexLock::unlock);
		connectionLock.lock();
		if (!connected)
			return;
		session.acknowledge();
	}

	void completeInitiation(void)
	{
		initiated = true;
		hapi->onInitiated();
	}

private:
	bool       connected;
	MutexLock  connectionLock;
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginInterface::HatoholArmPluginInterface(
  const string &queueAddr, const bool &workInServer)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(this, queueAddr, workInServer);
}

HatoholArmPluginInterface::~HatoholArmPluginInterface()
{
	exitSync();
	if (m_ctx)
		delete m_ctx;
}

void HatoholArmPluginInterface::setQueueAddress(const string &queueAddr)
{
	m_ctx->setQueueAddress(queueAddr);
}

void HatoholArmPluginInterface::send(const string &message)
{
	Message request;
	request.setReplyTo(m_ctx->receiverAddr);
	request.setContent(message);
	m_ctx->sender.send(request);
}

void HatoholArmPluginInterface::send(const SmartBuffer &smbuf)
{
	Message request;
	request.setReplyTo(m_ctx->receiverAddr);
	request.setContent(smbuf.getPointer<char>(0), smbuf.size());
	m_ctx->sender.send(request);
}

void HatoholArmPluginInterface::reply(const mlpl::SmartBuffer &replyBuf)
{
	HATOHOL_ASSERT(m_ctx->currMessage,
	               "This object doesn't have a current message.\n");
	const Address& address = m_ctx->currMessage->getReplyTo();
	if (!address) {
		MLPL_ERR("No return address.\n");
		m_ctx->session.reject(*m_ctx->currMessage);
		return;
	}
	Message reply;
	reply.setContent(replyBuf.getPointer<char>(0), replyBuf.size());
	Sender sender = m_ctx->session.createSender(address);
	sender.send(reply);
}

void HatoholArmPluginInterface::replyError(const HapiResponseCode &code)
{
	SmartBuffer resBuf;
	setupResponseBuffer<void>(resBuf, 0, code);
	reply(resBuf);
}

void HatoholArmPluginInterface::exitSync(void)
{
	requestExit();
	m_ctx->disconnect();
	HatoholThreadBase::exitSync();
}

void HatoholArmPluginInterface::registerCommandHandler(
  const HapiCommandCode &code, CommandHandler handler)
{
	m_ctx->receiveHandlerMap[code] = handler;
}

const string &HatoholArmPluginInterface::getQueueAddress(void) const
{
	return m_ctx->queueAddr;
}

const char *HatoholArmPluginInterface::getString(
  const SmartBuffer &repBuf, const void *head,
  const size_t &offset, const size_t &length)
{
	// check if the buffer length is enough
	const char *bufTopAddr = repBuf.getPointer<char>(0);
	const char *bufTailAddr = bufTopAddr;
	bufTailAddr += (repBuf.size() - 1);

	const char *stringAddr = static_cast<const char *>(head);
	stringAddr += offset;

	const size_t sizeOfNullTerm = 1;
	const char *stringTailAddr = stringAddr;
	stringTailAddr += (length + sizeOfNullTerm) - 1;

	if (stringAddr < bufTopAddr || stringTailAddr > bufTailAddr) {
		MLPL_ERR("Invalid paramter: bufTopAddr: %p, head: %p, "
		         "offset: %zd, length: %zd\n",
		         bufTopAddr, head, offset, length);
		return NULL;
	}

	if (*stringTailAddr != '\0') {
		MLPL_ERR("Not end with NULL: %02x\n", *stringTailAddr);
		return NULL;
	}

	return stringAddr;
}

char *HatoholArmPluginInterface::putString(
  void *buf, const void *refAddr, const string &src,
  uint16_t *offsetField, uint16_t *lengthField)
{
	const size_t length = src.size();
	*lengthField = length;
	*offsetField =
	  static_cast<uint16_t>((long)buf - (long)refAddr);
	memcpy(buf, src.c_str(), length + 1); // +1: Null terminator.

	char *nextAddr = reinterpret_cast<char *>(buf);
	nextAddr += (length + 1);
	return nextAddr;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer HatoholArmPluginInterface::mainThread(HatoholThreadArg *arg)
{
	m_ctx->connect();
	if (m_ctx->workInServer)
		sendInitiationPacket();
	while (!isExitRequested()) {
		Message message;
		m_ctx->receiver.fetch(message);
		if (isExitRequested())
			break;
		SmartBuffer sbuf;
		load(sbuf, message);
		sbuf.resetIndex();
		m_ctx->currMessage = &message;
		onReceived(sbuf);
		m_ctx->currMessage = NULL;
		m_ctx->sequenceIdOfCurrCmd = SEQ_ID_UNKNOWN,
		m_ctx->acknowledge();
	};
	m_ctx->disconnect();
	return NULL;
}

void HatoholArmPluginInterface::onConnected(Connection &conn)
{
}

void HatoholArmPluginInterface::onInitiated(void)
{
}

void HatoholArmPluginInterface::onSessionChanged(Session *session)
{
}

void HatoholArmPluginInterface::onReceived(mlpl::SmartBuffer &smbuf)
{
	if (!m_ctx->initiated) {
		initiation(smbuf);
		return;
	}

	if (smbuf.size() < sizeof(HapiCommandHeader)) {
		MLPL_ERR("Got a too small packet: %zd.\n", smbuf.size());
		replyError(HAPI_RES_INVALID_HEADER);
		return;
	}

	HapiCommandHeader *header;
	const uint16_t type = smbuf.getValue<uint16_t>();
	switch (type) {
	case HAPI_MSG_INITIATION:
		initiation(smbuf);
		break;
	case HAPI_MSG_COMMAND:
		header = smbuf.getPointer<HapiCommandHeader>();
		m_ctx->sequenceIdOfCurrCmd = header->sequenceId;
		parseCommand(header, smbuf);
		break;
	case HAPI_MSG_RESPONSE:
		parseResponse(smbuf.getPointer<HapiResponseHeader>(), smbuf);
		break;
	default:
		MLPL_ERR("Got an invalid message type: %d.\n", type);
		replyError(HAPI_RES_INVALID_HEADER);
	}
}

void HatoholArmPluginInterface::parseCommand(
  const HapiCommandHeader *header, mlpl::SmartBuffer &cmdBuf)
{
	CommandHandlerMapConstIterator it =
	  m_ctx->receiveHandlerMap.find(header->code);
	if (it == m_ctx->receiveHandlerMap.end()) {
		replyError(HAPI_RES_UNKNOWN_CODE);
		return;
	}
	CommandHandler handler = it->second;
	(this->*handler)(header);
}

void HatoholArmPluginInterface::parseResponse(
  const HapiResponseHeader *header, mlpl::SmartBuffer &resBuf)
{
	if (header->sequenceId != m_ctx->sequenceId) {
		MLPL_WARN("Got unexpected response: "
		          "expect: %08" PRIx32 ", actual: %08" PRIx32 "\n", 
		          m_ctx->sequenceId, header->sequenceId);
		// TODO: Consider if we should call onGotError().
		return;
	}
	onGotResponse(header, resBuf);
}

void HatoholArmPluginInterface::onGotError(
  const HatoholArmPluginError &hapError)
{
	THROW_HATOHOL_EXCEPTION("Got error: %d\n", hapError.code);
}

void HatoholArmPluginInterface::onGotResponse(
  const HapiResponseHeader *header, mlpl::SmartBuffer &resBuf)
{
}

void HatoholArmPluginInterface::sendInitiationPacket(void)
{
	SmartBuffer pktBuf(sizeof(HapiInitiationPacket));
	HapiInitiationPacket *initPkt =
	  pktBuf.getPointer<HapiInitiationPacket>(0);
	initPkt->type = HAPI_MSG_INITIATION;

	// TODO: improve the quality of random
	SmartTime stime(SmartTime::INIT_CURR_TIME);
	srandom(stime.getAsTimespec().tv_nsec);
	m_ctx->initiationKey = ((double)random() / RAND_MAX) * UINT64_MAX;
	initPkt->key = m_ctx->initiationKey;
	send(pktBuf);
}

void HatoholArmPluginInterface::initiation(const mlpl::SmartBuffer &sbuf)
{
	const size_t bufferSize = sbuf.size();
	if (bufferSize != sizeof(HapiInitiationPacket)) {
		MLPL_INFO("[Init] Drop a message: size: %zd.\n",
		          bufferSize);
		return;
	}
	HapiInitiationPacket *initPkt =
	  sbuf.getPointer<HapiInitiationPacket>(0);
	if (m_ctx->workInServer)
		waitInitiationResponse(initPkt);
	else
		replyInitiationPacket(initPkt);
}

void HatoholArmPluginInterface::waitInitiationResponse(
  const HapiInitiationPacket *initPkt)
{
	if (initPkt->type != HAPI_MSG_INITIATION_RESPONSE) {
		MLPL_INFO("[Init] Drop a message: type: %d.\n", initPkt->type);
		return;
	}
	if (initPkt->key != m_ctx->initiationKey) {
		MLPL_INFO("[Init] Ignore unexpected key: %" PRIx64 ", "
		          "actual: %" PRIx64 ". Ignored.\n",
		          initPkt->key, m_ctx->initiationKey);
		return;
	}
	m_ctx->completeInitiation();
}

void HatoholArmPluginInterface::replyInitiationPacket(
  const HapiInitiationPacket *initPkt)
{
	if (initPkt->type != HAPI_MSG_INITIATION) {
		MLPL_INFO("[Init] Drop a message: type: %d.\n", initPkt->type);
		return;
	}

	SmartBuffer resBuf(sizeof(HapiInitiationPacket));
	HapiInitiationPacket *initRes =
	  resBuf.getPointer<HapiInitiationPacket>(0);
	initRes->type = HAPI_MSG_INITIATION_RESPONSE;
	initRes->key = initPkt->key;
	reply(resBuf);
	m_ctx->completeInitiation();
}

void HatoholArmPluginInterface::load(SmartBuffer &sbuf, const Message &message)
{
	sbuf.addEx(message.getContentPtr(), message.getContentSize());
}

const HapiResponseHeader *HatoholArmPluginInterface::getResponseHeader(
  SmartBuffer &resBuf) throw(HatoholException)
{
	if (resBuf.size() < sizeof(HapiResponseHeader)) {
		THROW_HATOHOL_EXCEPTION("Invalid message size: %zd\n",
		                        sizeof(HapiResponseHeader));
	}
	const HapiResponseHeader *header =
	  resBuf.getPointer<HapiResponseHeader>(0);
	if (header->code != HAPI_RES_OK) {
		THROW_HATOHOL_EXCEPTION("Bad response code: %d\n",
		                        header->code);
	}
	return header;
}

uint32_t HatoholArmPluginInterface::getIncrementedSequenceId(void)
{
	m_ctx->sequenceId++;
	if (m_ctx->sequenceId > SEQ_ID_MAX)
		m_ctx->sequenceId = 0;
	return m_ctx->sequenceId;
}

void HatoholArmPluginInterface::setSequenceId(const uint32_t &sequenceId)
{
	m_ctx->sequenceId = sequenceId;
}

uint32_t HatoholArmPluginInterface::getSequenceIdInProgress(void)
{
	return m_ctx->sequenceIdOfCurrCmd;
}

void HatoholArmPluginInterface::dumpBuffer(
  const SmartBuffer &sbuf, const string &label)
{
	size_t size = sbuf.size();
	printf("===============================================\n");
	printf("[%s] obj: %p, size: %zd\n", label.c_str(), this, size);
	uint8_t *p = sbuf.getPointer<uint8_t>(0);
	size_t i = 0;
	while (true) {
		printf("%02x ", p[i]);
		i++;
		if (i >= size) {
			printf("\n");
			break;
		}
		if (i % 16 == 0)
			printf("\n");
	}
	printf("-----------------------------------------------\n");
}
