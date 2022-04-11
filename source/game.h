#ifndef FS_GAME_H
#define FS_GAME_H

enum GameState_t {
	GAME_STATE_STARTUP,
	GAME_STATE_INIT,
	GAME_STATE_NORMAL,
	GAME_STATE_CLOSED,
	GAME_STATE_SHUTDOWN,
	GAME_STATE_CLOSING,
	GAME_STATE_MAINTAIN,
};

class ServiceManager;

class Game {
public:
	Game() = default;
	~Game() {}

	// non-copyable
	Game(const Game&) = delete;
	Game& operator=(const Game&) = delete;

	GameState_t getGameState() const;
	void setGameState(GameState_t newState);

	void start(ServiceManager* serviceManager);
	void shutdown();

private:
	GameState_t m_gameState;
	ServiceManager* m_serviceManager = nullptr;
};

#endif // !FS_GAME_H
