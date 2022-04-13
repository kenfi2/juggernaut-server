#include "includes.h"

#include "fileloader.h"

PropStream& PropStream::operator<<(PropWriteStream& writer)
{
	size_t size;
	const char* buffer = writer.getStream(size);

	init(buffer, size);
	return *this;
}
