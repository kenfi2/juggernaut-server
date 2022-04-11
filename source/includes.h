#pragma once

#include <iostream>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )

#ifdef _MSC_VER
#pragma comment(lib, "Dbghelp.lib")
#endif

