#include "includes.h"
#include "configmanager.h"

#include <fstream>
#include <sstream>

ConfigManager::~ConfigManager()
{
	if (m_json) {
		delete m_json;
	}
}

bool jsonCompareKey(const Json* baseJson, const Json* compareJson)
{
	for (auto it = baseJson->begin(); it != baseJson->end(); ++it) {
		bool foundKey = false;
		for (auto cit = compareJson->begin(); cit != compareJson->end(); ++cit) {
			if (cit.key() == it.key()) {
				if (it.value().type_name() != cit.value().type_name()) {
					std::cout << "Different types of values in the key " << it.key() << std::endl;
					return false;
				}

				if (it.value().is_object() && !jsonCompareKey(&it.value(), &cit.value())) {
					return false;
				}

				foundKey = true;
				break;
			}
		}

		if (!foundKey) {
			std::cout << "No default value found for " << it.key() << std::endl;
			return false;
		}
	}
	return true;
}

bool ConfigManager::load()
{
	std::ifstream file{ "config.json" };
	
	if (!file.is_open()) {
		std::cout << "Missing file config.json" << std::endl;
		return false;
	}

	m_json = new Json();
	file >> *m_json;

	if (!m_json->is_object()) {
		std::cout << "Malformed file config.json" << std::endl;
		delete m_json;
		m_json = nullptr;
		return false;
	}

	if (!jsonCompareKey(m_json, &DefaultJsonConfig)) {
		std::cout << "Malformed file config.json" << std::endl;
	}

	return true;
}

void ConfigManager::reset()
{
	if (!m_json) {
		return;
	}

	m_json->clear();
}

std::string ConfigManager::getString(const std::string& key) const
{
	if (!m_json) {
		std::cout << "ConfigManager has not been loaded" << std::endl;
		return DefaultJsonConfig[key];
	}

	auto it = m_json->find(key);
	if (it == m_json->end()) {
		return DefaultJsonConfig[key];
	}

	if (!it->is_string()) {
		return DefaultJsonConfig[key];
	}

	return it->get<std::string>();
}

bool ConfigManager::getBoolean(const std::string& key) const
{
	if (!m_json) {
		std::cout << "ConfigManager has not been loaded" << std::endl;
		return DefaultJsonConfig[key];
	}

	auto it = m_json->find(key);
	if (it == m_json->end()) {
		return DefaultJsonConfig[key];
	}

	if (!it->is_string()) {
		return DefaultJsonConfig[key];
	}

	return it->get<bool>();
}
