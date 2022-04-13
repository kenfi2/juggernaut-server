#include "includes.h"

#include "game.h"
#include "configjson.h"
#include "server.h"
#include "protocollogin.h"
#include "rsa.h"
#include "tasks.h"
#include "scheduler.h"
#include "tools.h"
#include "databasetasks.h"
#include "databasemanager.h"

std::mutex g_loaderLock;
std::condition_variable g_loaderSignal;
std::unique_lock<std::mutex> g_loaderUniqueLock(g_loaderLock);

Game g_game;
RSA g_RSA;
Dispatcher g_dispatcher;
Scheduler g_scheduler;
DatabaseTasks g_databaseTasks;

ConfigJson g_json
{
	{"ip", "127.0.0.1"},
	{"rsa", true},
	{"loginPort", 7171},
	{"gamePort", 7172},
	{"mysqlPort", 3306},
	{"mysqlHost", "127.0.0.1"},
	{"mysqlUser", "root"},
	{"mysqlPass", ""},
	{"mysqlDatabase", "database"},
	{"mysqlSock", ""},
	{"statusPort", 7171},
	{"bindOnlyGlobalAddress", true},
	{"startupDatabaseOptimization", true},
	{"maxPacketsPerSecond", 250}
};

void mainLoader(int, char* argv[], ServiceManager* services);

int main(int argc, char* argv[])
{
	ServiceManager serviceManager;

	g_dispatcher.start();
	g_scheduler.start();

	g_dispatcher.addTask(createTask(std::bind(mainLoader, argc, argv, &serviceManager)));

	g_loaderSignal.wait(g_loaderUniqueLock);

	if (serviceManager.is_running()) {
		std::cout << ">> Server Online!" << std::endl << std::endl;
		serviceManager.run();
	}
	else {
		std::cout << ">> No services running. The server is NOT online." << std::endl;
		g_scheduler.shutdown();
		g_databaseTasks.shutdown();
		g_dispatcher.shutdown();
	}

	g_scheduler.join();
	g_databaseTasks.join();
	g_dispatcher.join();
	return 0;
}

void startupErrorMessage(std::string msg)
{
	std::cout << msg << std::endl;
	g_loaderSignal.notify_all();
}

void mainLoader(int, char* argv[], ServiceManager* services)
{
	g_game.setGameState(GAME_STATE_STARTUP);

	srand(static_cast<unsigned int>(OTSYS_TIME()));

	std::cout << "Juggernaut " << std::endl;
	std::cout << "Compiled with " << BOOST_COMPILER << std::endl;
	std::cout << "Compiled on " << __DATE__ << ' ' << __TIME__ << " for platform ";

#if defined(__amd64__) || defined(_M_X64)
	std::cout << "x64" << std::endl;
#elif defined(__i386__) || defined(_M_IX86) || defined(_X86_)
	std::cout << "x86" << std::endl;
#elif defined(__arm__)
	std::cout << "ARM" << std::endl;
#else
	std::cout << "unknown" << std::endl;
#endif
	std::cout << std::endl;

	if (!g_json.loadFile("config.json")) {
		startupErrorMessage("Config not loaded!");
		return;
	}

	if (g_json.getConfig<bool>("rsa")) {
		//set RSA key
		try {
			g_RSA.loadPEM("key.pem");
		}
		catch (const std::exception& e) {
			std::cout << "> ERROR: " << e.what() << std::endl;
			startupErrorMessage("RSA not loaded!");
			return;
		}
	}

	std::cout << ">> Establishing database connection..." << std::flush;

	if (!Database::getInstance().connect()) {
		startupErrorMessage("Failed to connect to database.");
		return;
	}

	std::cout << " MySQL " << Database::getClientVersion() << std::endl;

	// run database manager
	std::cout << ">> Running database manager" << std::endl;

	if (!DatabaseManager::isDatabaseSetup()) {
		startupErrorMessage("The database you have specified in config.lua is empty, please import the schema.sql to your database.");
		return;
	}

	g_databaseTasks.start();

	if (g_json.getConfig<bool>("startupDatabaseOptimization") && !DatabaseManager::optimizeTables()) {
		std::cout << "> No tables were optimized." << std::endl;
	}

	std::cout << ">> Initializing gamestate" << std::endl;
	g_game.setGameState(GAME_STATE_INIT);

	// Game client protocols
	//services->add<ProtocolGame>(static_cast<uint16_t>(GAME_PORT));
	services->add<ProtocolLogin>(g_json.getConfig<uint16_t>("loginPort"));

	g_game.start(services);
	g_game.setGameState(GAME_STATE_NORMAL);
	g_loaderSignal.notify_all();
}
