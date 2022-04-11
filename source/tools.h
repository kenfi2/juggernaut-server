#ifndef FS_TOOLS_H_5F9A9742DA194628830AA1C64909AE43
#define FS_TOOLS_H_5F9A9742DA194628830AA1C64909AE43

#include <random>

#include "const.h"
#include "enums.h"

std::string convertIPToString(uint32_t ip);

uint32_t adlerChecksum(const uint8_t* data, size_t length);

int64_t OTSYS_TIME();

// Demangle names for GNU g++ compiler
std::string demangle_name(const char* name);

// Returns the name of a type
template<typename T>
std::string demangle_type() { return demangle_name(typeid(T).name()); }

#endif
