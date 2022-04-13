#include "includes.h"

#include "configjson.h"
#include "fileloader.h"

bool jsonCompareKey(const nlohmann::json* baseJson, const nlohmann::json* compareJson)
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

bool ConfigJson::loadFile(const std::string& file)
{
	std::ifstream ifile{ file };
	if (!ifile.is_open()) {
		std::cout << "Missing file " << file << std::endl;
		return false;
	}

	m_defaultConfig.clear();
	m_defaultConfig.swap(*this);

	clear();
	ifile >> *this;

	if (!is_object()) {
		std::cout << "Malformed file config.json" << std::endl;
		return false;
	}

	if (!m_defaultConfig.empty() && !jsonCompareKey(this, &m_defaultConfig)) {
		std::cout << "Malformed file config.json" << std::endl;
	}

	return true;
}
