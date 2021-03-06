/**
 * The Forgotten Server - a free and open-source MMORPG server emulator
 * Copyright (C) 2019 Mark Samman <mark.samman@gmail.com>
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
#include <csignal>

#include "signals.h"
#include "tasks.h"
#include "scheduler.h"


extern Scheduler g_scheduler;
extern Dispatcher g_dispatcher;

using ErrorCode = boost::system::error_code;

Signals::Signals(boost::asio::io_service& service) :
	set(service)
{
	set.add(SIGINT);
	set.add(SIGTERM);
#ifndef _WIN32
	set.add(SIGUSR1);
	set.add(SIGHUP);
#else
	// This must be a blocking call as Windows calls it in a new thread and terminates
	// the process when the handler returns (or after 5 seconds, whichever is earlier).
	// On Windows it is called in a new thread.
	signal(SIGBREAK, dispatchSignalHandler);
#endif

	asyncWait();
}

void Signals::asyncWait()
{
	set.async_wait([this] (ErrorCode err, int signal) {
		if (err) {
			std::cerr << "Signal handling error: "  << err.message() << std::endl;
			return;
		}
		dispatchSignalHandler(signal);
		asyncWait();
	});
}

// On Windows this function does not need to be signal-safe,
// as it is called in a new thread.
// https://github.com/otland/forgottenserver/pull/2473
void Signals::dispatchSignalHandler(int signal)
{
	switch(signal) {
		case SIGINT: //Shuts the server down
			g_dispatcher.addTask(createTask(sigintHandler));
			break;
		case SIGTERM: //Shuts the server down
			g_dispatcher.addTask(createTask(sigtermHandler));
			break;
#ifndef _WIN32
		case SIGHUP: //Reload config/data
			g_dispatcher.addTask(createTask(sighupHandler));
			break;
		case SIGUSR1: //Saves game state
			g_dispatcher.addTask(createTask(sigusr1Handler));
			break;
#else
		case SIGBREAK: //Shuts the server down
			g_dispatcher.addTask(createTask(sigbreakHandler));
			// hold the thread until other threads end
			g_scheduler.join();
			g_dispatcher.join();
			break;
#endif
		default:
			break;
	}
}

void Signals::sigbreakHandler()
{
	//Dispatcher thread
	std::cout << "SIGBREAK received, shutting game server down..." << std::endl;
	//GAME_STATE_SHUTDOWN;
}

void Signals::sigtermHandler()
{
	//Dispatcher thread
	std::cout << "SIGTERM received, shutting game server down..." << std::endl;
	//GAME_STATE_SHUTDOWN;
}

void Signals::sigusr1Handler()
{
	//Dispatcher thread
	std::cout << "SIGUSR1 received, saving the game state..." << std::endl;
	// SAVE;
}

void Signals::sighupHandler()
{
	//Dispatcher thread
	std::cout << "SIGHUP received, reloading config files..." << std::endl;

	//lua_gc(g_luaEnvironment.getLuaState(), LUA_GCCOLLECT, 0);
}

void Signals::sigintHandler()
{
	//Dispatcher thread
	std::cout << "SIGINT received, shutting game server down..." << std::endl;
	//SHUTDOWN;
}
