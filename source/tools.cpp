#include "includes.h"

#include "tools.h"

#ifdef _MSC_VER

#include <windows.h>

#pragma warning (push)
#pragma warning (disable:4091) // warning C4091: 'typedef ': ignored on left of '' when no variable is declared
#include <dbghelp.h>
#pragma warning (pop)

#else

#include <cxxabi.h>
#include <cstring>
#include <cstdlib>

#endif

std::string convertIPToString(uint32_t ip)
{
	char buffer[17];

	int res = sprintf(buffer, "%u.%u.%u.%u", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24));
	if (res < 0) {
		return {};
	}

	return buffer;
}

uint32_t adlerChecksum(const uint8_t* data, size_t length)
{
	if (length > NETWORKMESSAGE_MAXSIZE) {
		return 0;
	}

	const uint16_t adler = 65521;

	uint32_t a = 1, b = 0;

	while (length > 0) {
		size_t tmp = length > 5552 ? 5552 : length;
		length -= tmp;

		do {
			a += *data++;
			b += a;
		} while (--tmp);

		a %= adler;
		b %= adler;
	}

	return (b << 16) | a;
}

int64_t OTSYS_TIME()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string demangle_name(const char* name)
{
#ifdef _MSC_VER
	static const unsigned BufferSize = 1024;
	static char Buffer[BufferSize] = {};

	int written = UnDecorateSymbolName(name, Buffer, BufferSize - 1, UNDNAME_COMPLETE);
	Buffer[written] = '\0';
#else
	size_t len;
	int status;
	char* demangled = abi::__cxa_demangle(name, nullptr, &len, &status);
	if (demangled) {
		std::string ret(demangle);
		free(demangled);
		return ret;
	}
#endif

	return Buffer;
}
