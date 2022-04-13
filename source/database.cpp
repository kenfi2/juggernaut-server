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

#include "configjson.h"
#include "database.h"

#include <mysql/errmsg.h>

extern ConfigJson g_json;

Database::~Database()
{
	if (m_handle != nullptr) {
		mysql_close(m_handle);
	}
}

bool Database::connect()
{
	// connection handle initialization
	m_handle = mysql_init(nullptr);
	if (!m_handle) {
		std::cout << std::endl << "Failed to initialize MySQL connection handle." << std::endl;
		return false;
	}

	// automatic reconnect
	bool reconnect = true;
	mysql_options(m_handle, MYSQL_OPT_RECONNECT, &reconnect);

	// connects to database
	if (!mysql_real_connect(m_handle, g_json.getConfig<std::string>("mysqlHost").c_str(), g_json.getConfig<std::string>("mysqlUser").c_str(), g_json.getConfig<std::string>("mysqlPass").c_str(), g_json.getConfig<std::string>("mysqlDatabase").c_str(), g_json.getConfig<uint32_t>("mysqlPort"), g_json.getConfig<std::string>("mysqlSock").c_str(), 0)) {
		std::cout << std::endl << "MySQL Error Message: " << mysql_error(m_handle) << std::endl;
		return false;
	}

	DBResult_ptr result = storeQuery("SHOW VARIABLES LIKE 'max_allowed_packet'");
	if (result) {
		m_maxPacketSize = result->getNumber<uint64_t>("Value");
	}
	return true;
}

bool Database::beginTransaction()
{
	if (!executeQuery("BEGIN")) {
		return false;
	}

	m_databaseLock.lock();
	return true;
}

bool Database::rollback()
{
	if (mysql_rollback(m_handle) != 0) {
		std::cout << "[Error - mysql_rollback] Message: " << mysql_error(m_handle) << std::endl;
		m_databaseLock.unlock();
		return false;
	}

	m_databaseLock.unlock();
	return true;
}

bool Database::commit()
{
	if (mysql_commit(m_handle) != 0) {
		std::cout << "[Error - mysql_commit] Message: " << mysql_error(m_handle) << std::endl;
		m_databaseLock.unlock();
		return false;
	}

	m_databaseLock.unlock();
	return true;
}

bool Database::executeQuery(const std::string& query)
{
	bool success = true;

	// executes the query
	m_databaseLock.lock();

	while (mysql_real_query(m_handle, query.c_str(), query.length()) != 0) {
		std::cout << "[Error - mysql_real_query] Query: " << query.substr(0, 256) << std::endl << "Message: " << mysql_error(m_handle) << std::endl;
		auto error = mysql_errno(m_handle);
		if (error != CR_SERVER_LOST && error != CR_SERVER_GONE_ERROR && error != CR_CONN_HOST_ERROR && error != 1053/*ER_SERVER_SHUTDOWN*/ && error != CR_CONNECTION_ERROR) {
			success = false;
			break;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	MYSQL_RES* m_res = mysql_store_result(m_handle);
	m_databaseLock.unlock();

	if (m_res) {
		mysql_free_result(m_res);
	}

	return success;
}

DBResult_ptr Database::storeQuery(const std::string& query)
{
	m_databaseLock.lock();

	retry:
	while (mysql_real_query(m_handle, query.c_str(), query.length()) != 0) {
		std::cout << "[Error - mysql_real_query] Query: " << query << std::endl << "Message: " << mysql_error(m_handle) << std::endl;
		auto error = mysql_errno(m_handle);
		if (error != CR_SERVER_LOST && error != CR_SERVER_GONE_ERROR && error != CR_CONN_HOST_ERROR && error != 1053/*ER_SERVER_SHUTDOWN*/ && error != CR_CONNECTION_ERROR) {
			break;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	// we should call that every time as someone would call executeQuery('SELECT...')
	// as it is described in MySQL manual: "it doesn't hurt" :P
	MYSQL_RES* res = mysql_store_result(m_handle);
	if (res == nullptr) {
		std::cout << "[Error - mysql_store_result] Query: " << query << std::endl << "Message: " << mysql_error(m_handle) << std::endl;
		auto error = mysql_errno(m_handle);
		if (error != CR_SERVER_LOST && error != CR_SERVER_GONE_ERROR && error != CR_CONN_HOST_ERROR && error != 1053/*ER_SERVER_SHUTDOWN*/ && error != CR_CONNECTION_ERROR) {
			m_databaseLock.unlock();
			return nullptr;
		}
		goto retry;
	}
	m_databaseLock.unlock();

	// retrieving results of query
	DBResult_ptr result = std::make_shared<DBResult>(res);
	if (!result->hasNext()) {
		return nullptr;
	}
	return result;
}

std::string Database::escapeString(const std::string& s) const
{
	return escapeBlob(s.c_str(), s.length());
}

std::string Database::escapeBlob(const char* s, uint32_t length) const
{
	// the worst case is 2n + 1
	size_t maxLength = (length * 2) + 1;

	std::string escaped;
	escaped.reserve(maxLength + 2);
	escaped.push_back('\'');

	if (length != 0) {
		char* output = new char[maxLength];
		mysql_real_escape_string(m_handle, output, s, length);
		escaped.append(output);
		delete[] output;
	}

	escaped.push_back('\'');
	return escaped;
}

DBResult::DBResult(MYSQL_RES* res)
{
	m_handle = res;

	size_t i = 0;

	MYSQL_FIELD* field = mysql_fetch_field(m_handle);
	while (field) {
		m_listNames[field->name] = i++;
		field = mysql_fetch_field(m_handle);
	}

	m_row = mysql_fetch_row(m_handle);
}

DBResult::~DBResult()
{
	mysql_free_result(m_handle);
}

std::string DBResult::getString(const std::string& s) const
{
	auto it = m_listNames.find(s);
	if (it == m_listNames.end()) {
		std::cout << "[Error - DBResult::getString] Column '" << s << "' does not exist in result set." << std::endl;
		return std::string();
	}

	if (m_row[it->second] == nullptr) {
		return std::string();
	}

	return std::string(m_row[it->second]);
}

const char* DBResult::getStream(const std::string& s, unsigned long& size) const
{
	auto it = m_listNames.find(s);
	if (it == m_listNames.end()) {
		std::cout << "[Error - DBResult::getStream] Column '" << s << "' doesn't exist in the result set" << std::endl;
		size = 0;
		return nullptr;
	}

	if (m_row[it->second] == nullptr) {
		size = 0;
		return nullptr;
	}

	size = mysql_fetch_lengths(m_handle)[it->second];
	return m_row[it->second];
}

bool DBResult::hasNext() const
{
	return m_row != nullptr;
}

bool DBResult::next()
{
	m_row = mysql_fetch_row(m_handle);
	return m_row != nullptr;
}

DBInsert::DBInsert(std::string query) : m_query(std::move(query))
{
	m_length = m_query.length();
}

bool DBInsert::addRow(const std::string& row)
{
	// adds new row to buffer
	const size_t rowLength = row.length();
	m_length += rowLength;
	if (m_length > Database::getInstance().getMaxPacketSize() && !execute()) {
		return false;
	}

	if (m_values.empty()) {
		m_values.reserve(rowLength + 2);
		m_values.push_back('(');
		m_values.append(row);
		m_values.push_back(')');
	} else {
		m_values.reserve(m_values.length() + rowLength + 3);
		m_values.push_back(',');
		m_values.push_back('(');
		m_values.append(row);
		m_values.push_back(')');
	}
	return true;
}

bool DBInsert::addRow(std::ostringstream& row)
{
	bool ret = addRow(row.str());
	row.str(std::string());
	return ret;
}

bool DBInsert::execute()
{
	if (m_values.empty()) {
		return true;
	}

	// executes buffer
	bool res = Database::getInstance().executeQuery(m_query + m_values);
	m_values.clear();
	m_length = m_query.length();
	return res;
}
