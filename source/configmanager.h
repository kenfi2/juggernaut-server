#ifndef FS_CONFIGMANAGER_H
#define FS_CONFIGMANAGER_H

static const Json DefaultJsonConfig{
	{"ip", "127.0.0.1"},
	{"rsa", true},
	{"mysqlPort", 3306},
	{"loginPort", 7171},
	{"gamePort", 7172},
	{"statusPort", 7171},
	{"bindOnlyGlobalAddress", true},
	{"maxPacketsPerSecond", 250}
};

class ConfigManager
{
public:
	ConfigManager() = default;
	~ConfigManager();

	// non-copyable
	ConfigManager(const ConfigManager&) = delete;
	ConfigManager& operator=(const ConfigManager&) = delete;

	bool load();
	void reset();

	template <typename T>
	T getNumber(const std::string& key) const {
		if (!m_json) {
			std::cout << "ConfigManager has not been loaded" << std::endl;
			return static_cast<T>(DefaultJsonConfig[key]);
		}

		auto it = m_json->find(key);
		if (it == m_json->end()) {
			return static_cast<T>(DefaultJsonConfig[key]);
		}
		
		if (!it->is_number()) {
			return static_cast<T>(DefaultJsonConfig[key]);
		}

		return it->get<T>();
	}

	std::string getString(const std::string& key) const;
	bool getBoolean(const std::string& key) const;

private:
	Json* m_json = nullptr;
};

#endif
