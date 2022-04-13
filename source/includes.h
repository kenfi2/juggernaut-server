#pragma once

#include <iostream>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include <list>
#include <fstream>
#include <sstream>

class LuaInterface;
class LuaObject;
using LuaObject_ptr = std::shared_ptr<LuaObject>;
struct lua_State;

typedef int (*LuaCFunction) (lua_State* L);
typedef std::function<int(LuaInterface*)> LuaCppFunction;
typedef std::unique_ptr<LuaCppFunction> LuaCppFunction_ptr;

#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )

#ifdef _MSC_VER
#define __attribute__(A) /* do nothing */
#pragma comment(lib, "Dbghelp.lib")
#endif
