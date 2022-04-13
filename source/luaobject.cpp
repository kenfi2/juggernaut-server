#include "includes.h"

#include "tools.h"

#include "luaobject.h"

std::string LuaObject::getClassName() const
{
	return demangle_name(typeid(*this).name());
}
