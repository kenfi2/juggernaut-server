#include "includes.h"

#include "game.h"
#include "server.h"
#include "scheduler.h"

GameState_t Game::getGameState() const
{
	return m_gameState;
}

void Game::setGameState(GameState_t newState)
{
	if (m_gameState == GAME_STATE_SHUTDOWN) {
		return; //this cannot be stopped
	}

	if (m_gameState == newState) {
		return;
	}

	m_gameState = newState;
	switch (newState) {
	case GAME_STATE_INIT: {

		break;
	}

	case GAME_STATE_SHUTDOWN: {
		g_dispatcher.addTask(
			createTask(std::bind(&Game::shutdown, this)));

		g_scheduler.stop();
		g_dispatcher.stop();
		break;
	}

	case GAME_STATE_CLOSED: {
		break;
	}

	default:
		break;
	}
}

void Game::start(ServiceManager* serviceManager)
{
	m_serviceManager = serviceManager;
}

void Game::shutdown()
{
	std::cout << "Shutting down..." << std::flush;

	g_scheduler.shutdown();
	g_dispatcher.shutdown();

	if (m_serviceManager) {
		m_serviceManager->stop();
	}

	ConnectionManager::getInstance().closeAll();

	std::cout << " done!" << std::endl;
}
