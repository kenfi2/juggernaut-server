#ifndef FS_ENUMS_H
#define FS_ENUMS_H

enum ThreadState {
	THREAD_STATE_RUNNING,
	THREAD_STATE_CLOSING,
	THREAD_STATE_TERMINATED,
};

enum LoginOpcodes : uint8_t {
	InvalidAccountName = 0x01,
	InvalidPassword = 0x02,
	LoginSuccess = 0x03,
	DoLogin = 0x04,
	CreateAccount = 0x05,
	UsernameAlreadyExists = 0x06,
	EmailAlreadyRegistered = 0x07,
	AccountCannotBeCreated = 0x08,
	CreateAccountSuccess = 0x09,
};

#endif
