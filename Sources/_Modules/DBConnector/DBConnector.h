#pragma once
#include "mysql.h"
#include <string>

class DBConnector
{
public:
	DBConnector(
		const std::string&	connectIP,
		const int32_t		connectPort,
		const std::string&	userID,
		const std::string&	password,
		const std::string&	databaseName
	);
	virtual ~DBConnector();

	bool Connect();
	bool Disconnect();
	bool DoQuery(std::string& queryFormat, ...);
	bool DoQuerySave(std::string& queryFormat, ...);

};