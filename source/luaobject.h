#ifndef FS_LUAOBJECT_H
#define FS_LUAOBJECT_H

class LuaObject : public std::enable_shared_from_this<LuaObject>
{
public:
	LuaObject() = default;
	~LuaObject() {}

	// non-copyable
	LuaObject(const LuaObject&) = delete;
	LuaObject& operator=(const LuaObject&) = delete;

	virtual std::string getClassName() const;

};

#endif
