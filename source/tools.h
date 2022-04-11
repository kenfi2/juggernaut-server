#ifndef FS_TOOLS_H_5F9A9742DA194628830AA1C64909AE43
#define FS_TOOLS_H_5F9A9742DA194628830AA1C64909AE43

#include <random>

#include "const.h"
#include "enums.h"

std::string convertIPToString(uint32_t ip);

uint32_t adlerChecksum(const uint8_t* data, size_t length);

int64_t OTSYS_TIME();

#endif
