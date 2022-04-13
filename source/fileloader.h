#ifndef FS_FILELOADER_H
#define FS_FILELOADER_H

class PropWriteStream;

class PropStream
{
	public:
		void init(const char* a, size_t size) {
			p = a;
			end = a + size;
		}

		size_t size() const {
			return end - p;
		}

		template <typename T>
		bool read(T& ret) {
			if (size() < sizeof(T)) {
				return false;
			}

			memcpy(&ret, p, sizeof(T));
			p += sizeof(T);
			return true;
		}

		bool readString(std::string& ret) {
			uint16_t strLen;
			if (!read<uint16_t>(strLen)) {
				return false;
			}

			if (size() < strLen) {
				return false;
			}

			char* str = new char[strLen + 1];
			memcpy(str, p, strLen);
			str[strLen] = 0;
			ret.assign(str, strLen);
			delete[] str;
			p += strLen;
			return true;
		}

		bool skip(size_t n) {
			if (size() < n) {
				return false;
			}

			p += n;
			return true;
		}

		PropStream& operator<<(PropWriteStream& writer);

		PropStream& operator<<(std::ifstream file)
		{
			if (!file.is_open())
				return *this;

			std::streampos fileSize;
			file.seekg(0, file.end);
			fileSize = file.tellg();
			file.seekg(0, file.beg);

			std::vector<char> data(fileSize);
			file.read(&data[0], fileSize);

			init(&data[0], fileSize);
			file.close();
			return *this;
		}

	private:
		const char* p = nullptr;
		const char* end = nullptr;
};

class PropWriteStream
{
	using String = std::string;
	struct _String : public std::string { using String::String; };

	public:
		PropWriteStream() = default;

		// non-copyable
		PropWriteStream(const PropWriteStream&) = delete;
		PropWriteStream& operator=(const PropWriteStream&) = delete;

		const char* getStream(size_t& size) const {
			size = buffer.size();
			return buffer.data();
		}

		void clear() {
			buffer.clear();
		}
		/// <summary>
		/// Convert to include only the string in the buffer
		/// </summary>
		static _String String(const std::string& str) {
			_String _str;
			_str.append(str.begin(), str.end());
			return _str;
		}

		PropWriteStream& operator>>(const _String& str) {
			std::copy(str.begin(), str.end(), std::back_inserter(buffer));
			return *this;
		}

		PropWriteStream& operator>>(const std::string& str)
		{
			size_t strLength = str.size();
			if (strLength > std::numeric_limits<uint16_t>::max()) {
				return *this >> (uint16_t)0x0;
			}

			*this >> static_cast<uint16_t>(strLength);
			std::copy(str.begin(), str.end(), std::back_inserter(buffer));
			return *this;
		}

		PropWriteStream& operator>>(bool add) { write(add); return *this; }
		PropWriteStream& operator>>(uint8_t add) { write(add); return *this; }
		PropWriteStream& operator>>(uint16_t add) { write(add); return *this; }
		PropWriteStream& operator>>(uint32_t add) { write(add); return *this; }
		PropWriteStream& operator>>(float add) { write(add); return *this; }
		PropWriteStream& operator>>(uint64_t add) { write(add); return *this; }
		PropWriteStream& operator>>(double add) { write(add); return *this; }
		PropWriteStream& operator>>(std::ofstream& file) {
			if (!file.is_open())
				return *this;

			file.write(buffer.data(), buffer.size());
			file.close();
			return *this;
		}
		
	private:
		std::vector<char> buffer;

		template<typename T>
		void write(T add) {
			char* addr = reinterpret_cast<char*>(&add);
			std::copy(addr, addr + sizeof(T), std::back_inserter(buffer));
		}
};

#endif
