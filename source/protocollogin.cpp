#include "includes.h"

#include "tools.h"

#include "protocollogin.h"
#include "configjson.h"

#include "outputmessage.h"
#include "tasks.h"

#include <iomanip>
#include "iologindata.h"

extern ConfigJson g_json;

void ProtocolLogin::verifyAccount(const std::string& email, const std::string& password)
{
	uint8_t opcodeMessage = IOLoginData::verifyAccount(email, password);

	auto output = OutputMessagePool::getOutputMessage();
	output->add(opcodeMessage);
	send(output);

	if (opcodeMessage != LoginSuccess) {
		disconnect();
	}
}

void ProtocolLogin::createAccount(const std::string& username, const std::string& email, const std::string& password)
{
	uint8_t opcodeMessage = IOLoginData::createAccount(username, email, password);

	auto output = OutputMessagePool::getOutputMessage();
	output->add(opcodeMessage);
	send(output);

	if (opcodeMessage != CreateAccountSuccess) {
		disconnect();
	}
}

void ProtocolLogin::onRecvFirstMessage(NetworkMessage& msg)
{
	if (g_json.getConfig<bool>("rsa") && !Protocol::RSA_decrypt(msg)) {
		disconnect();
		return;
	}

	uint8_t action = msg.getByte();

	xtea::key key;
	key[0] = msg.get<uint32_t>();
	key[1] = msg.get<uint32_t>();
	key[2] = msg.get<uint32_t>();
	key[3] = msg.get<uint32_t>();
	enableXTEAEncryption();
	setXTEAKey(std::move(key));

	auto thisPtr = std::static_pointer_cast<ProtocolLogin>(shared_from_this());
	if (action == LoginOpcodes::DoLogin) {
		std::string email = msg.getString();
		std::string password = msg.getString();

		g_dispatcher.addTask(createTask(std::bind(&ProtocolLogin::verifyAccount, thisPtr, email, password)));
	}
	else if (action == LoginOpcodes::CreateAccount) {
		std::string username = msg.getString();
		std::string email = msg.getString();
		std::string password = msg.getString();

		g_dispatcher.addTask(createTask(std::bind(&ProtocolLogin::createAccount, thisPtr, username, email, password)));
	}

}
