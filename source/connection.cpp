/**
 * The Forgotten Server - a free and open-source MMORPG server emulator
 * Copyright (C) 2019  Mark Samman <mark.samman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "includes.h"

#include "protocol.h"
#include "connection.h"
#include "outputmessage.h"
#include "server.h"
#include "tasks.h"
#include "configjson.h"

extern ConfigJson g_json;

Connection_ptr ConnectionManager::createConnection(boost::asio::io_service& io_service, ConstServicePort_ptr servicePort)
{
	std::lock_guard<std::mutex> lockClass(m_connectionManagerLock);

	auto connection = std::make_shared<Connection>(io_service, servicePort);
	m_connections.insert(connection);
	return connection;
}

void ConnectionManager::releaseConnection(const Connection_ptr& connection)
{
	std::lock_guard<std::mutex> lockClass(m_connectionManagerLock);

	m_connections.erase(connection);
}

void ConnectionManager::closeAll()
{
	std::lock_guard<std::mutex> lockClass(m_connectionManagerLock);

	for (const auto& connection : m_connections) {
		try {
			boost::system::error_code error;
			connection->m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
			connection->m_socket.close(error);
		} catch (boost::system::system_error&) {
		}
	}
	m_connections.clear();
}

// Connection

void Connection::close(bool force)
{
	//any thread
	ConnectionManager::getInstance().releaseConnection(getThis());

	std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);
	if (connectionState != CONNECTION_STATE_OPEN) {
		return;
	}
	connectionState = CONNECTION_STATE_CLOSED;

	if (m_protocol) {
		g_dispatcher.addTask(
			createTask(std::bind(&Protocol::release, m_protocol)));
	}

	if (m_messageQueue.empty() || force) {
		closeSocket();
	} else {
		//will be closed by the destructor or onWriteOperation
	}
}

void Connection::closeSocket()
{
	if (m_socket.is_open()) {
		try {
			m_readTimer.cancel();
			m_writeTimer.cancel();
			boost::system::error_code error;
			m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
			m_socket.close(error);
		} catch (boost::system::system_error& e) {
			std::cout << "[Network error - Connection::closeSocket] " << e.what() << std::endl;
		}
	}
}

Connection::~Connection()
{
	closeSocket();
}

void Connection::accept(Protocol_ptr protocol)
{
	m_protocol = protocol;
	g_dispatcher.addTask(createTask(std::bind(&Protocol::onConnect, protocol)));

	accept();
}

void Connection::accept()
{
	std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);
	try {
		m_readTimer.expires_from_now(boost::posix_time::seconds(CONNECTION_READ_TIMEOUT));
		m_readTimer.async_wait(std::bind(&Connection::handleTimeout, std::weak_ptr<Connection>(getThis()), std::placeholders::_1));

		// Read size of the first packet
		boost::asio::async_read(m_socket,
		                        boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::HEADER_LENGTH),
		                        std::bind(&Connection::parseHeader, getThis(), std::placeholders::_1));
	} catch (boost::system::system_error& e) {
		std::cout << "[Network error - Connection::accept] " << e.what() << std::endl;
		close(FORCE_CLOSE);
	}
}

void Connection::parseHeader(const boost::system::error_code& error)
{
	std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);
	m_readTimer.cancel();

	if (error) {
		close(FORCE_CLOSE);
		return;
	} else if (connectionState != CONNECTION_STATE_OPEN) {
		return;
	}

	uint32_t timePassed = std::max<uint32_t>(1, (time(nullptr) - m_timeConnected) + 1);
	if ((++m_packetsSent / timePassed) > g_json.getConfig<uint32_t>("maxPacketsPerSecond")) {
		std::cout << convertIPToString(getIP()) << " disconnected for exceeding packet per second limit." << std::endl;
		close();
		return;
	}

	if (timePassed > 2) {
		m_timeConnected = time(nullptr);
		m_packetsSent = 0;
	}

	uint16_t size = m_msg.getLengthHeader();
	if (size == 0 || size >= NETWORKMESSAGE_MAXSIZE - 16) {
		close(FORCE_CLOSE);
		return;
	}

	try {
		m_readTimer.expires_from_now(boost::posix_time::seconds(CONNECTION_READ_TIMEOUT));
		m_readTimer.async_wait(std::bind(&Connection::handleTimeout, std::weak_ptr<Connection>(getThis()),
		                                    std::placeholders::_1));

		// Read packet content
		m_msg.setLength(size + NetworkMessage::HEADER_LENGTH);
		boost::asio::async_read(m_socket, boost::asio::buffer(m_msg.getBodyBuffer(), size),
		                        std::bind(&Connection::parsePacket, getThis(), std::placeholders::_1));
	} catch (boost::system::system_error& e) {
		std::cout << "[Network error - Connection::parseHeader] " << e.what() << std::endl;
		close(FORCE_CLOSE);
	}
}

void Connection::parsePacket(const boost::system::error_code& error)
{
	std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);
	m_readTimer.cancel();

	if (error) {
		close(FORCE_CLOSE);
		return;
	} else if (connectionState != CONNECTION_STATE_OPEN) {
		return;
	}

	//Check packet checksum
	uint32_t checksum;
	int32_t len = m_msg.getLength() - m_msg.getBufferPosition() - NetworkMessage::CHECKSUM_LENGTH;
	if (len > 0) {
		checksum = adlerChecksum(m_msg.getBuffer() + m_msg.getBufferPosition() + NetworkMessage::CHECKSUM_LENGTH, len);
	} else {
		checksum = 0;
	}

	uint32_t recvChecksum = m_msg.get<uint32_t>();
	if (recvChecksum != checksum) {
		// it might not have been the checksum, step back
		m_msg.skipBytes(-NetworkMessage::CHECKSUM_LENGTH);
	}

	if (!receivedFirst) {
		// First message received
		receivedFirst = true;

		if (!m_protocol) {
			// Game protocol has already been created at this point
			m_protocol = m_service_port->make_protocol(recvChecksum == checksum, m_msg, getThis());
			if (!m_protocol) {
				close(FORCE_CLOSE);
				return;
			}
		} else {
			m_msg.skipBytes(1); // Skip protocol ID
		}
		
		m_protocol->onRecvFirstMessage(m_msg);
	} else {
		m_protocol->onRecvMessage(m_msg); // Send the packet to the current protocol
	}

	try {
		m_readTimer.expires_from_now(boost::posix_time::seconds(CONNECTION_READ_TIMEOUT));
		m_readTimer.async_wait(std::bind(&Connection::handleTimeout, std::weak_ptr<Connection>(getThis()),
		                                    std::placeholders::_1));

		// Wait to the next packet
		boost::asio::async_read(m_socket,
		                        boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::HEADER_LENGTH),
		                        std::bind(&Connection::parseHeader, getThis(), std::placeholders::_1));
	} catch (boost::system::system_error& e) {
		std::cout << "[Network error - Connection::parsePacket] " << e.what() << std::endl;
		close(FORCE_CLOSE);
	}
}

void Connection::send(const OutputMessage_ptr& msg)
{
	std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);
	if (connectionState != CONNECTION_STATE_OPEN) {
		return;
	}

	bool noPendingWrite = m_messageQueue.empty();
	m_messageQueue.emplace_back(msg);
	if (noPendingWrite) {
		internalSend(msg);
	}
}

void Connection::internalSend(const OutputMessage_ptr& msg)
{
	m_protocol->onSendMessage(msg);
	try {
		m_writeTimer.expires_from_now(boost::posix_time::seconds(CONNECTION_WRITE_TIMEOUT));
		m_writeTimer.async_wait(std::bind(&Connection::handleTimeout, std::weak_ptr<Connection>(getThis()),
		                                     std::placeholders::_1));

		boost::asio::async_write(m_socket,
		                         boost::asio::buffer(msg->getOutputBuffer(), msg->getLength()),
		                         std::bind(&Connection::onWriteOperation, getThis(), std::placeholders::_1));
	} catch (boost::system::system_error& e) {
		std::cout << "[Network error - Connection::internalSend] " << e.what() << std::endl;
		close(FORCE_CLOSE);
	}
}

uint32_t Connection::getIP()
{
	std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);

	// IP-address is expressed in network byte order
	boost::system::error_code error;
	const boost::asio::ip::tcp::endpoint endpoint = m_socket.remote_endpoint(error);
	if (error) {
		return 0;
	}

	return htonl(endpoint.address().to_v4().to_ulong());
}

void Connection::onWriteOperation(const boost::system::error_code& error)
{
	std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);
	m_writeTimer.cancel();
	m_messageQueue.pop_front();

	if (error) {
		m_messageQueue.clear();
		close(FORCE_CLOSE);
		return;
	}

	if (!m_messageQueue.empty()) {
		internalSend(m_messageQueue.front());
	} else if (connectionState == CONNECTION_STATE_CLOSED) {
		closeSocket();
	}
}

void Connection::handleTimeout(ConnectionWeak_ptr connectionWeak, const boost::system::error_code& error)
{
	if (error == boost::asio::error::operation_aborted) {
		//The timer has been manually cancelled
		return;
	}

	if (auto connection = connectionWeak.lock()) {
		connection->close(FORCE_CLOSE);
	}
}
