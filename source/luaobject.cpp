#include "includes.h"

#include "luaobject.h"
#include "tools.h"

std::string LuaObject::getClassName() const
{
	return demangle_name(typeid(*this).name());
}
