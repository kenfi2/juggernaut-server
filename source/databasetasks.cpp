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

#include "databasetasks.h"
#include "tasks.h"

extern Dispatcher g_dispatcher;


void DatabaseTasks::start()
{
	m_db.connect();
	ThreadHolder::start();
}

void DatabaseTasks::threadMain()
{
	std::unique_lock<std::mutex> taskLockUnique(m_taskLock, std::defer_lock);
	while (getState() != THREAD_STATE_TERMINATED) {
		taskLockUnique.lock();
		if (m_tasks.empty()) {
			m_taskSignal.wait(taskLockUnique);
		}

		if (!m_tasks.empty()) {
			DatabaseTask task = std::move(m_tasks.front());
			m_tasks.pop_front();
			taskLockUnique.unlock();
			runTask(task);
		} else {
			taskLockUnique.unlock();
		}
	}
}

void DatabaseTasks::addTask(std::string query, std::function<void(DBResult_ptr, bool)> callback/* = nullptr*/, bool store/* = false*/)
{
	bool signal = false;
	m_taskLock.lock();
	if (getState() == THREAD_STATE_RUNNING) {
		signal = m_tasks.empty();
		m_tasks.emplace_back(std::move(query), std::move(callback), store);
	}
	m_taskLock.unlock();

	if (signal) {
		m_taskSignal.notify_one();
	}
}

void DatabaseTasks::runTask(const DatabaseTask& task)
{
	bool success;
	DBResult_ptr result;
	if (task.m_store) {
		result = m_db.storeQuery(task.m_query);
		success = true;
	} else {
		result = nullptr;
		success = m_db.executeQuery(task.m_query);
	}

	if (task.m_callback) {
		g_dispatcher.addTask(createTask(std::bind(task.m_callback, result, success)));
	}
}

void DatabaseTasks::flush()
{
	std::unique_lock<std::mutex> guard{ m_taskLock };
	while (!m_tasks.empty()) {
		auto task = std::move(m_tasks.front());
		m_tasks.pop_front();
		guard.unlock();
		runTask(task);
		guard.lock();
	}
}

void DatabaseTasks::shutdown()
{
	m_taskLock.lock();
	setState(THREAD_STATE_TERMINATED);
	m_taskLock.unlock();
	flush();
	m_taskSignal.notify_one();
}
