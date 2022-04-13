#include "includes.h"

#include "tools.h"

#include "iologindata.h"
#include "databasemanager.h"

LoginOpcodes IOLoginData::verifyAccount(const std::string& accountName, const std::string& password)
{
	Database& db = Database::getInstance();

	std::ostringstream query;
	query << "SELECT `password` FROM `accounts` WHERE name = " << db.escapeString(accountName) << " OR e_mail = " << db.escapeString(accountName);

	DBResult_ptr result = db.storeQuery(query.str());
	if (!result) {
		return InvalidAccountName;
	}

	if (transformToSHA1(password) != result->getString("password"))
		return InvalidPassword;

	return LoginSuccess;
}

LoginOpcodes IOLoginData::createAccount(const std::string& username, const std::string& email, const std::string& password)
{
	Database& db = Database::getInstance();

	DBResult_ptr result = db.storeQuery("SELECT `name` FROM `accounts` WHERE `name` = " + db.escapeString(username));
	if (result)
		return UsernameAlreadyExists;

	result = db.storeQuery("SELECT `e_mail` FROM `accounts` WHERE `e_mail` = " + db.escapeString(email));
	if (result)
		return EmailAlreadyRegistered;

	std::ostringstream query;
	query << "INSERT INTO `accounts`(`name`, `password`, `e_mail`) VALUES";
	query << "(" << db.escapeString(username) << ", " << db.escapeString(transformToSHA1(password)) << ", " << db.escapeString(email) << ")";

	bool res = Database::getInstance().executeQuery(query.str());
	if (!res) {
		return AccountCannotBeCreated;
	}
	return CreateAccountSuccess;
}
