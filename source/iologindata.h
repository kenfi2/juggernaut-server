#ifndef FS_IOLOGINDATA_H
#define FS_IOLOGINDATA_H

class IOLoginData {
public:
	static LoginOpcodes verifyAccount(const std::string& email, const std::string& password);
	static LoginOpcodes createAccount(const std::string& uername, const std::string& email, const std::string& password);
};

#endif
