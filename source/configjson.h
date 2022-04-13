#ifndef FS_ConfigJson_H
#define FS_ConfigJson_H

using Json = nlohmann::json;

class ConfigJson : public Json
{
public:
	using Json::Json;

	bool loadFile(const std::string& file);

	ConfigJson getTable(const std::string& key) const {
		ConfigJson json;
		json.swap(getConfig<Json>(key));
		return json;
	}

	template<typename T>
	T getValue(const uint32_t& key) const;

	template<typename T>
	T getConfig(const std::string& key) const;
private:
	Json m_defaultConfig;
};

template<typename T>
inline T ConfigJson::getConfig(const std::string& key) const
{
	auto it = find(key);
	if (it == end()) {
		auto cit = m_defaultConfig.find(key);
		if (cit == m_defaultConfig.end()) {
			std::cout << "[ConfigJson::getConfig] key " << key << " not configured" << std::endl;
			return T();
		}
		return cit->get<T>();
	}
	return it->get<T>();
}

template<typename T>
inline T ConfigJson::getValue(const uint32_t& key) const
{
	Json json = at(key);
	if (json.is_null()) {
		return T();
	}

	return json.get<T>();
}

#endif
