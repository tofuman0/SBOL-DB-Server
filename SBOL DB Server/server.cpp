#include <WinSock2.h>
#include <WS2tcpip.h>
#include "server.h"

Server::Server(bool type)
{
	databasetype = DBTYPE_SQLITE;
	databaseusername = "";
	databasepassword = "";
	databasehost = "";
	databasename = "";
	databaseport = 3306;
	webserver = true;
	serverport = 7946;
	webserverport = 8080;
	running = false;
	paused = false;
	isService = type;
	serverNumConnections = 0;
	ServerSocket = -1;
	TempSocket = -1;
	serverClientLimit = 10;
	logger = new Logger(type);
}
Server::~Server()
{
	for (uint32_t i = 0; i < connections.size(); i++)
	{
		delete connections[i];
	}
}
void Server::Initialize()
{
	connections.resize(serverClientLimit);
	serverConnections.resize(serverClientLimit);
	for (uint32_t i = 0; i < connections.size(); i++)
	{
		connections[i] = new CLIENT();
		connections[i]->logger = logger;
		connections[i]->server = this;
		serverConnections[i] = 0;
	}
}
char* Server::HexString(uint8_t* in, char* out, uint32_t length = 0)
{
	uint32_t offset = 0;
	if (!in) return out;
	uint8_t* getLength = in;
	while ((*getLength && (getLength - in) < MAX_MESG_LEN) || ((length > 0) && (offset / 2 < length)))
	{
		sprintf(&out[offset], "%02X", getLength[0]);
		offset += 2;
		getLength++;
		if (length && (offset / 2 >= length)) break;
	}
	out[offset] = 0;
	return out;
}
char* Server::HexString(uint8_t* in, uint32_t length = 0)
{
	char* out = &hexStr[0];
	uint32_t offset = 0;
	if (!in) return out;
	uint8_t* getLength = in;
	while ((*getLength && (getLength - in) < MAX_MESG_LEN) || (length && ((offset / 2) < length)))
	{
		sprintf(&out[offset], "%02X", getLength[0]);
		offset += 2;
		getLength++;
		if (length && (offset / 2 >= length)) break;
	}
	out[offset] = 0;
	return out;
}
int32_t Server::Start()
{
	if (VerifyConfig() || checkDB())
		return 1;

	Initialize();
	running = true;
	std::thread sThread(ServerThread, this);
	sThread.detach();
	logger->Log(LOGTYPE_NONE, L"Server Started.");
	return 0;
}
bool Server::IsRunning()
{
	return running;
}
int32_t Server::Stop()
{
	if (running == false)
		return 1;
	else
		running = false;
	return 0;
}
int32_t Server::Pause()
{
	if (paused == true)
		return 1;
	else
		paused = true;
	return 0;
}
int32_t Server::Unpause()
{
	if (paused == false)
		return 1;
	else
		paused = false;
	return 0;
}
int32_t Server::CreateAccount(wchar_t* username, wchar_t* password, wchar_t* email, wchar_t* privileges)
{
	if (VerifyConfig() || checkDB())
		return 1;

	int32_t res = db->Open();

	if (res == DB_OK)
	{
		std::stringstream ss;
		std::string un = HexString((uint8_t*)ToNarrow(username).c_str());
		std::string em = HexString((uint8_t*)ToNarrow(email).c_str());

		ss << "SELECT username FROM account_data WHERE username = X\'" << un << "\'";
		res = db->Exec(ss.str().c_str());

		if (db->results.size() == 0)
		{
			std::string finalPassword = HASH_SECRET;
			finalPassword += '.';
			finalPassword += ToNarrow(std::wstring(password));
			uint8_t cryptoPW[SHA256_DIGEST_LENGTH] = { 0 };
			char cryptoPWHEX[(SHA256_DIGEST_LENGTH * 2) + 1] = { 0 };

			GetHash((uint8_t*)finalPassword.c_str(), &cryptoPW[0], finalPassword.length());
			HexString(&cryptoPW[0], &cryptoPWHEX[0], SHA256_DIGEST_LENGTH);

			ss.clear();
			ss.str(std::string());
			uint8_t priv = min(_wtoi(privileges), 255);
			
			ss << "INSERT INTO account_data (username, password, email, privileges) VALUES (X\'" << un << "\', \'" << cryptoPWHEX << "\', X\'" << em << "\', " << (int)priv << ");";
			const char* temp = un.c_str();
			res = db->Exec(ss.str().c_str());
			res = db->Close();
			if (res != DB_OK) return 1;
			else return 0;
		}
		else
		{
			logger->Log(LOGTYPE_NONE, L"Account already exists for: %s.", username);
		}

		return 1;
	}

	else
	{
		logger->Log(LOGTYPE_ERROR, L"Error Connecting to database: %s.", ToWide(databasename.c_str()));
		return 1;
	}


	return 0;
}
int32_t Server::SetPassword(wchar_t* username, wchar_t* password)
{
	if (VerifyConfig() || checkDB())
		return 1;

	int32_t res = db->Open();

	if (res == DB_OK)
	{
		std::stringstream ss;
		ss << "SELECT username FROM account_data WHERE username = \'" << ToNarrow(username) << "\'";
		res = db->Exec(ss.str().c_str());

		if (db->results.size() > 0)
		{
			std::string finalPassword = HASH_SECRET;
			finalPassword += '.';
			finalPassword += ToNarrow(password);
			uint8_t cryptoPW[SHA256_DIGEST_LENGTH] = { 0 };
			char cryptoPWHEX[(SHA256_DIGEST_LENGTH * 2) + 1] = { 0 };

			GetHash((uint8_t*)finalPassword.c_str(), &cryptoPW[0], finalPassword.length());
			HexString(&cryptoPW[0], &cryptoPWHEX[0], SHA256_DIGEST_LENGTH);
			ss.clear();
			ss.str(std::string());
			ss << "UPDATE account_data SET password = \'" << cryptoPWHEX << "\' WHERE username = \'" << ToNarrow(username) << "\'";
			res = db->Exec(ss.str().c_str());
			res = db->Close();
			if (res != DB_OK) return 1;
			else return 0;
		}

		return 1;
	}

	else
	{
		logger->Log(LOGTYPE_ERROR, L"Error Connecting to database: %s.", ToWide(databasename.c_str()));
		return 1;
	}

	return 0;
}
int32_t Server::CreateKey()
{
	if (VerifyConfig() || checkDB())
		return 1;

	uint8_t key[KEY_SIZE] = { 0 };
	uint8_t iv[BLOCK_SIZE] = { 0 };

	RAND_bytes(key, KEY_SIZE);
	RAND_bytes(iv, BLOCK_SIZE);

	uint8_t tempKey[48];

	CopyMemory(&tempKey[0], &key[0], KEY_SIZE);
	CopyMemory(&tempKey[32], &iv[0], BLOCK_SIZE);

	std::ofstream output(".\\serverkey.bin", std::ios::binary);
	if (output.is_open())
	{
		try {
			output.write((char*)&tempKey[0], KEY_SIZE + BLOCK_SIZE);
			output.close();
		}
		catch (std::exception ex) {
			logger->Log(LOGTYPE_ERROR, L"Unable to create key file: %s", ex.what());
			return 1;
		}
	}
	else
	{
		logger->Log(LOGTYPE_ERROR, L"Unable to create key file (file locked?)");
		return 1;
	}

	int32_t res = db->Open();

	if (res == DB_OK)
	{
		std::string k = HexString(&key[0], KEY_SIZE);
		std::string v = HexString(&iv[0], BLOCK_SIZE);
		std::stringstream ss;
		ss << "INSERT INTO server_keys (key, iv) VALUES (X'" << k << "', X'" << v << "')";
		res = db->Exec(ss.str().c_str());

		if (res != DB_OK)
		{
			db->Close();
			return 1;
		}
		db->Close();
	}

	else
	{
		logger->Log(LOGTYPE_ERROR, L"Error Connecting to database: %s.", ToWide(databasename.c_str()));
		return 1;
	}

	return 0;
}
bool Server::GetHash(uint8_t *src, uint8_t* dst, uint32_t length)
{
	SHA256_CTX ctx;
	if (!SHA256_Init(&ctx)) return false;
	if (!SHA256_Update(&ctx, src, length)) return false;
	if (!SHA256_Final(dst, &ctx)) return false;
	return true;
}
int32_t Server::LoadConfig(const wchar_t* filename)
{
	bool malformed = false;
	std::wstring error = L"";
	std::ifstream inFile(filename);
	if (inFile.fail())
	{
		logger->Log(LOGTYPE_ERROR, L"Configuration file missing: %s. Default config is being created.", filename);
		std::ofstream outfile(filename);
		if (outfile.fail())
		{
			logger->Log(LOGTYPE_ERROR, L"Failure to create default configuration file: %s.", filename);
			return 1;
		}
		outfile << DEFAULT_JSON_CONFIG;
		outfile.close();
		inFile.open(filename);
	}
	if (!inFile.is_open())
	{
		logger->Log(LOGTYPE_ERROR, L"Error Loading configuration file: %s.", filename);
		return 1;
	}

	json in;

	try {
		inFile >> in;
	}
	catch (json::parse_error) {
		logger->Log(LOGTYPE_ERROR, L"JSON Parse Error\n");
		inFile.close();
		return 1;
	}

	inFile.close();

	if (in.empty())
	{
		error = L"file is empty";
		malformed = true;
	}
	else
	{
		if (in["databasetype"].is_string())
		{
			try {
				std::string dbType = in["databasetype"].get<std::string>();
				if (dbType == "mysql")
					databasetype = DBTYPE_MYSQL;
				else
					databasetype = DBTYPE_SQLITE;
				logger->Log(LOGTYPE_DEBUG, L"CONFIG: Database type is %s.", databasetype ? L"MySQL" : L"SQLite");
			}
			catch (json::parse_error) {
				logger->Log(LOGTYPE_ERROR, L"JSON Parse Error\n");
				return 1;
			}

		}
		else
		{
			error = L"databasetype value is not valid mysql or sqlite";
			malformed = true;
		}

		if (in["databaseusername"].is_string())
		{
			if (databasetype == DBTYPE_MYSQL)
			{
				try {
					databaseusername = in["databaseusername"].get<std::string>();
					logger->Log(LOGTYPE_DEBUG, L"CONFIG: Database username is %s.", databaseusername.length() ? ToWide(databaseusername).c_str() : L"EMPTY");
				}
				catch (json::parse_error) {
					logger->Log(LOGTYPE_ERROR, L"JSON Parse Error\n");
					return 1;
				}
			}
		}
		else
		{
			if (databasetype == DBTYPE_MYSQL)
			{
				error = L"databaseusername value is not string";
				malformed = true;
			}
		}

		if (in["databasepassword"].is_string())
		{
			if (databasetype == DBTYPE_MYSQL)
			{
				try {
					databasepassword = in["databasepassword"].get<std::string>();
					logger->Log(LOGTYPE_DEBUG, L"CONFIG: Database password is %s.", databasepassword.length() ? L"********" : L"EMPTY");
				}
				catch (json::parse_error) {
					logger->Log(LOGTYPE_ERROR, L"JSON Parse Error\n");
					return 1;
				}
			}
		}
		else
		{
			if (databasetype == DBTYPE_MYSQL)
			{
				error = L"databasepassword value is not string";
				malformed = true;
			}
		}

		if (in["databasehost"].is_string())
		{
			if (databasetype == DBTYPE_MYSQL)
			{
				try {
					databasehost = in["databasehost"].get<std::string>();
					logger->Log(LOGTYPE_DEBUG, L"CONFIG: Database host is %s.", databasehost.length() ? ToWide(databasehost).c_str() : L"EMPTY");
				}
				catch (json::parse_error) {
					logger->Log(LOGTYPE_ERROR, L"JSON Parse Error\n");
					return 1;
				}
			}
		}
		else
		{
			if (databasetype == DBTYPE_MYSQL)
			{
				error = L"databasehost value is not string";
				malformed = true;
			}
		}

		if (in["databasename"].is_string())
		{
			try {
				databasename = in["databasename"].get<std::string>();
				logger->Log(LOGTYPE_DEBUG, L"CONFIG: Database name is %s.", databasename.length() ? ToWide(databasename).c_str() : L"EMPTY");
			}
			catch (json::parse_error) {
				logger->Log(LOGTYPE_ERROR, L"JSON Parse Error\n");
				return 1;
			}
		}
		else
		{
			error = L"database value is not string";
			malformed = true;
		}

		if (in["databaseport"].is_number_integer())
		{
			if (databasetype == DBTYPE_MYSQL)
			{
				try {
					serverport = in["databaseport"].get<unsigned short>();
					logger->Log(LOGTYPE_DEBUG, L"CONFIG: Server port is %u.", databaseport);
				}
				catch (json::parse_error) {
					logger->Log(LOGTYPE_ERROR, L"JSON Parse Error\n");
					return 1;
				}
			}
		}
		else
		{
			if (databasetype == DBTYPE_MYSQL)
			{
				error = L"databaseport value is not integer";
				malformed = true;
			}
		}

		if (in["serverport"].is_number_integer())
		{
			try {
				serverport = in["serverport"].get<unsigned short>();
				logger->Log(LOGTYPE_DEBUG, L"CONFIG: Server port is %u.", serverport);
			}
			catch (json::parse_error) {
				logger->Log(LOGTYPE_ERROR, L"JSON Parse Error\n");
				return 1;
			}
		}
		else
		{
			error = L"serverport value is not integer";
			malformed = true;
		}

		if (in["webserver"].is_boolean())
		{
			try {
				webserver = in["webserver"].get<bool>();
				logger->Log(LOGTYPE_DEBUG, L"CONFIG: Webserver is %s.", webserver ? L"Enabled" : L"Disabled");
			}
			catch (json::parse_error) {
				logger->Log(LOGTYPE_ERROR, L"JSON Parse Error\n");
				return 1;
			}
		}
		else
		{
			error = L"webserver value is not integer";
			malformed = true;
		}

		if (in["webserverport"].is_number_integer())
		{
			try {
				webserverport = in["webserverport"].get<unsigned short>();
				logger->Log(LOGTYPE_DEBUG, L"CONFIG: Webserver port is %u.", webserverport);
			}
			catch (json::parse_error) {
				logger->Log(LOGTYPE_ERROR, L"JSON Parse Error\n");
				return 1;
			}
		}
		else
		{
			error = L"webserverport value is not integer";
			malformed = true;
		}

		if (in["logpath"].is_string())
		{
			try {
				logger->LogPath(in["logpath"].get<std::string>());
				logger->Log(LOGTYPE_DEBUG, L"CONFIG: Log path is %s.", logger->IsLogPathSet() ? ToWide(logger->LogPath()).c_str() : L"EMPTY");
			}
			catch (json::parse_error) {
				logger->Log(LOGTYPE_ERROR, L"JSON Parse Error\n");
				return 1;
			}
		}
		else
		{
			error = L"logpath value is not string";
			malformed = true;
		}
	}

	if (malformed)
	{
		logger->Log(LOGTYPE_ERROR, L"Configuration file: %s is malformed (%s).", filename, error.c_str());
		return 1;
	}
	else
		return 0;
}
std::wstring Server::ToWide(std::string in)
{
	std::wstring temp(in.length(), L' ');
	copy(in.begin(), in.end(), temp.begin());
	return temp;
}
std::string Server::ToNarrow(std::wstring in)
{
	std::string temp(in.length(), ' ');
	copy(in.begin(), in.end(), temp.begin());
	return temp;
}
int32_t Server::VerifyConfig()
{
	if (LoadConfig(CONFIG_FILENAME))
		return 1;

	switch (databasetype)
	{
	case DBTYPE_SQLITE:
		if (databasename == "" || (webserverport == 0 && webserver == true))
			return 1;
		break;
	case DBTYPE_MYSQL:
		if (databasename == "" || databaseusername == "" || databasepassword == "" || databasehost == "" || databaseport == 0 || serverport == 0 || (webserverport == 0 && webserver == true))
			return 1;
		break;
	}

	db = new Database(databasetype, databaseusername.c_str(), databasepassword.c_str(), databasehost.c_str(), databasename.c_str(), databaseport);
	return 0;
}
void Server::CloseDB()
{
	if (db)
		db->Close();
}
int32_t Server::checkDB()
{
	if (databasetype == DBTYPE_SQLITE)
	{
		int32_t res = db->Open();

		if (res == SQLITE_OK)
		{
			// Check for required tables and create if needed.
			res = db->Exec("PRAGMA table_info(account_data);");
			if (!db->results.size())
			{
				logger->Log(LOGTYPE_ERROR, L"account_data table is missing. Recreating");
				res = db->Exec(SQLITE_STATEMENT_ACCOUNT_DATA);
				if (res != SQLITE_OK) { logger->Log(LOGTYPE_ERROR, L"unable to create account_data table with error: %u", res); return 1; }
				// Insert default admin account and set password to be reset
				logger->Log(LOGTYPE_DATABASE, L"Created default account_data table");
				res = db->Exec("INSERT INTO account_data (license, username, password, email, handle, privileges, state) VALUES (100000, 'sboladmin', 'changeme', 'changeme', 'sbgajp', 255, 128);");
			}
			res = db->Exec("SELECT sql FROM sqlite_master WHERE name = 'account_data'; ");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), SQLITE_STATEMENT_ACCOUNT_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: account_data table doesn\'t match default structure.");

			res = db->Exec("PRAGMA table_info(rival_data);");
			if (!db->results.size())
			{
				logger->Log(LOGTYPE_ERROR, L"rival_data table is missing. Recreating");
				res = db->Exec(SQLITE_STATEMENT_RIVAL_DATA);
				if (res != SQLITE_OK) { logger->Log(LOGTYPE_ERROR, L"unable to create rival_data table with error: %u", res); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default rival_data table");
			}
			res = db->Exec("SELECT sql FROM sqlite_master WHERE name = 'rival_data'; ");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), SQLITE_STATEMENT_RIVAL_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: rival_data table doesn\'t match default structure.");

			res = db->Exec("PRAGMA table_info(garage_data);");
			if (!db->results.size())
			{
				logger->Log(LOGTYPE_ERROR, L"garage_data table is missing. Recreating");
				res = db->Exec(SQLITE_STATEMENT_GARAGE_DATA);
				if (res != SQLITE_OK) { logger->Log(LOGTYPE_ERROR, L"unable to create garage_data table with error: %u", res); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default garage_data table");
			}
			res = db->Exec("SELECT sql FROM sqlite_master WHERE name = 'garage_data'; ");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), SQLITE_STATEMENT_GARAGE_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: garage_data table doesn\'t match default structure.");

			res = db->Exec("PRAGMA table_info(teamgarage_data);");
			if (!db->results.size())
			{
				logger->Log(LOGTYPE_ERROR, L"teamgarage_data table is missing. Recreating");
				res = db->Exec(SQLITE_STATEMENT_TEAMGARAGE_DATA);
				if (res != SQLITE_OK) { logger->Log(LOGTYPE_ERROR, L"unable to create teamgarage_data table with error: %u", res); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default teamgarage_data table");
			}
			res = db->Exec("SELECT sql FROM sqlite_master WHERE name = 'teamgarage_data'; ");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), SQLITE_STATEMENT_TEAMGARAGE_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: teamgarage_data table doesn\'t match default structure.");

			res = db->Exec("PRAGMA table_info(item_data);");
			if (!db->results.size())
			{
				logger->Log(LOGTYPE_ERROR, L"item_data table is missing. Recreating");
				res = db->Exec(SQLITE_STATEMENT_ITEM_DATA);
				if (res != SQLITE_OK) { logger->Log(LOGTYPE_ERROR, L"unable to create item_data table with error: %u", res); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default item_data table");
			}
			res = db->Exec("SELECT sql FROM sqlite_master WHERE name = 'item_data'; ");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), SQLITE_STATEMENT_ITEM_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: item_data table doesn\'t match default structure.");

			res = db->Exec("PRAGMA table_info(sign_data);");
			if (!db->results.size())
			{
				logger->Log(LOGTYPE_ERROR, L"sign_data table is missing. Recreating");
				res = db->Exec(SQLITE_STATEMENT_SIGN_DATA);
				if (res != SQLITE_OK) { logger->Log(LOGTYPE_ERROR, L"unable to create sign_data table with error: %u", res); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default sign_data table");
			}
			res = db->Exec("SELECT sql FROM sqlite_master WHERE name = 'sign_data'; ");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), SQLITE_STATEMENT_SIGN_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: sign_data table doesn\'t match default structure.");

			res = db->Exec("PRAGMA table_info(team_data);");
			if (!db->results.size())
			{
				logger->Log(LOGTYPE_ERROR, L"team_data table is missing. Recreating");
				res = db->Exec(SQLITE_STATEMENT_TEAM_DATA);
				if (res != SQLITE_OK) { logger->Log(LOGTYPE_ERROR, L"unable to create team_data table with error: %u", res); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default team_data table");
			}
			res = db->Exec("SELECT sql FROM sqlite_master WHERE name = 'team_data'; ");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), SQLITE_STATEMENT_TEAM_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: team_data table doesn\'t match default structure.");

			res = db->Exec("PRAGMA table_info(server_keys);");
			if (!db->results.size())
			{
				logger->Log(LOGTYPE_ERROR, L"server_keys table is missing. Recreating");
				res = db->Exec(SQLITE_STATEMENT_SERVER_KEYS);
				if (res != SQLITE_OK) { logger->Log(LOGTYPE_ERROR, L"unable to create server_keys table with error: %u", res); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default server_keys table");
			}
			res = db->Exec("SELECT sql FROM sqlite_master WHERE name = 'server_keys'; ");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), SQLITE_STATEMENT_SERVER_KEYS) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: server_keys table doesn\'t match default structure.");

			res = db->Close();
			return 0;
		}
		else
		{
			logger->Log(LOGTYPE_ERROR, L"Error Connecting to database: %s.", ToWide(databasename.c_str()));
			return 1;
		}
	}
	else
	{ // MySQL Assumed for any other value
		int32_t res = db->Open();

		if (res == DB_OK)
		{
			// Check for required tables and create if needed.
			res = db->Exec("SHOW CREATE TABLE `account_data`;");
			if (!res)
			{
				logger->Log(LOGTYPE_ERROR, L"account_data table is missing. Recreating");
				db->Exec(MYSQL_STATEMENT_ACCOUNT_DATA);
				res = db->Exec("SHOW CREATE TABLE `account_data`;");
				if (!res) { logger->Log(LOGTYPE_ERROR, L"unable to create account_data table with error: %s", db->Error()); return 1; }
				// Insert default admin account and set password to be reset
				logger->Log(LOGTYPE_DATABASE, L"Created default account_data table");
				res = db->Exec("INSERT INTO account_data (license, username, password, email, handle, privileges, state) VALUES (100000, 'sboladmin', 'changeme', 'changeme', 'sbgajp', 255, 128);");
			}
			res = db->Exec("SHOW CREATE TABLE `account_data`;");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), MYSQL_STATEMENT_ACCOUNT_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: account_data table doesn\'t match default structure.");

			res = db->Exec("SHOW CREATE TABLE `rival_data`;");
			if (!res)
			{
				logger->Log(LOGTYPE_ERROR, L"rival_data table is missing. Recreating");
				db->Exec(MYSQL_STATEMENT_RIVAL_DATA);
				res = db->Exec("SHOW CREATE TABLE `rival_data`;");
				if (!res) { logger->Log(LOGTYPE_ERROR, L"unable to create rival_data table with error: %s", db->Error()); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default rival_data table");
			}
			res = db->Exec("SHOW CREATE TABLE `rival_data`;");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), MYSQL_STATEMENT_RIVAL_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: rival_data table doesn\'t match default structure.");

			res = db->Exec("SHOW CREATE TABLE `garage_data`;");
			if (!res)
			{
				logger->Log(LOGTYPE_ERROR, L"garage_data table is missing. Recreating");
				db->Exec(MYSQL_STATEMENT_GARAGE_DATA);
				res = db->Exec("SHOW CREATE TABLE `garage_data`;");
				if (!res) { logger->Log(LOGTYPE_ERROR, L"unable to create garage_data table with error: %s", db->Error()); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default garage_data table");
			}
			res = db->Exec("SHOW CREATE TABLE `garage_data`;");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), MYSQL_STATEMENT_GARAGE_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: garage_data table doesn\'t match default structure.");

			res = db->Exec("SHOW CREATE TABLE `teamgarage_data`;");
			if(!res)
			{
				logger->Log(LOGTYPE_ERROR, L"teamgarage_data table is missing. Recreating");
				db->Exec(MYSQL_STATEMENT_TEAMGARAGE_DATA);
				res = db->Exec("SHOW CREATE TABLE `teamgarage_data`;");
				if (!res) { logger->Log(LOGTYPE_ERROR, L"unable to create teamgarage_data table with error: %s", db->Error()); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default teamgarage_data table");
			}
			res = db->Exec("SHOW CREATE TABLE `teamgarage_data`;");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), MYSQL_STATEMENT_TEAMGARAGE_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: teamgarage_data table doesn\'t match default structure.");

			res = db->Exec("SHOW CREATE TABLE `item_data`;");
			if (!res)
			{
				logger->Log(LOGTYPE_ERROR, L"item_data table is missing. Recreating");
				db->Exec(MYSQL_STATEMENT_ITEM_DATA);
				res = db->Exec("SHOW CREATE TABLE `item_data`;");
				if (!res) { logger->Log(LOGTYPE_ERROR, L"unable to create item_data table with error: %s", db->Error()); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default item_data table");
			}
			res = db->Exec("SHOW CREATE TABLE `item_data`;");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), MYSQL_STATEMENT_ITEM_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: item_data table doesn\'t match default structure.");

			res = db->Exec("SHOW CREATE TABLE `sign_data`;");
			if (!res)
			{
				logger->Log(LOGTYPE_ERROR, L"sign_data table is missing. Recreating");
				db->Exec(MYSQL_STATEMENT_SIGN_DATA);
				res = db->Exec("SHOW CREATE TABLE `sign_data`;");
				if (!res) { logger->Log(LOGTYPE_ERROR, L"unable to create sign_data table with error: %s", db->Error()); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default sign_data table");
			}
			res = db->Exec("SHOW CREATE TABLE `sign_data`;");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), MYSQL_STATEMENT_SIGN_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: sign_data table doesn\'t match default structure.");

			res = db->Exec("SHOW CREATE TABLE `team_data`;");
			if (!res)
			{
				logger->Log(LOGTYPE_ERROR, L"team_data table is missing. Recreating");
				db->Exec(MYSQL_STATEMENT_TEAM_DATA);
				res = db->Exec("SHOW CREATE TABLE `team_data`;");
				if (!res) { logger->Log(LOGTYPE_ERROR, L"unable to create team_data table with error: %s", db->Error()); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default team_data table");
			}
			res = db->Exec("SHOW CREATE TABLE `team_data`;");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), MYSQL_STATEMENT_TEAM_DATA) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: team_data table doesn\'t match default structure.");

			res = db->Exec("SHOW CREATE TABLE `server_keys`;");
			if (!res)
			{
				logger->Log(LOGTYPE_ERROR, L"server_keys table is missing. Recreating");
				db->Exec(MYSQL_STATEMENT_SERVER_KEYS);
				res = db->Exec("SHOW CREATE TABLE `server_keys`;");
				if (!res) { logger->Log(LOGTYPE_ERROR, L"unable to create server_keys table with error: %s", db->Error()); return 1; }
				logger->Log(LOGTYPE_DATABASE, L"Created default server_keys table");
			}
			res = db->Exec("SHOW CREATE TABLE `server_keys`;");
			if (strcmp(db->GetValue(&db->results, 0, 0).c_str(), MYSQL_STATEMENT_SERVER_KEYS) != 0) logger->Log(LOGTYPE_DATABASE, L"WARNING: server_keys table doesn\'t match default structure.");

			res = db->Close();

			return 0;
		}
	}
	return 1;
}
void Server::CheckClientPackets(CLIENT* client, uint32_t rcvcopy, uint8_t * tmprcv)
{
	if (rcvcopy >= 6)
	{
		client->inbuf.setArray(tmprcv, rcvcopy, 0x00);
		uint16_t size = client->inbuf.getSizeFromBuffer();
		client->inbuf.setSize(size);
		if (client->inbuf.getSize() > CLIENT_BUFFER_SIZE)
		{
			// Packet too big, disconnect the client.
			logger->Log(LOGTYPE_COMM, L"Client %s sent a invalid packet.", ToWide((char*)client->IP_Address).c_str());
			client->Disconnect();
		}
		else
		{
			client->ProcessPacket();
		}
	}
}
void Server::InitializeConnection(CLIENT* connect)
{

	if (connect->ClientSocket >= 0)
	{
		logger->Log(LOGTYPE_COMM, L"Client %s has disconnected.", ToWide((char*)connect->IP_Address).c_str());

		int j = 0;
		for (uint32_t i = 0; i < serverNumConnections; i++)
		{
			if (serverConnections[i] != connect->connection_index)
				serverConnections[j++] = serverConnections[i];
		}
		serverNumConnections = j;
		closesocket(connect->ClientSocket);
	}
	connect->Initialize();
	char titleBuf[0x100];
	snprintf(titleBuf, sizeof(titleBuf), "SBOL DB Server v%s. Clients: %u", logger->ToNarrow(VERSION_STRING).c_str(), GetClientCount());
	SetConsoleTitleA(titleBuf);
}
int32_t Server::TcpSockOpen(struct in_addr ip, uint16_t port)
{
	int32_t fd, bufsize, turn_on_option_flag = 1, rcSockopt;
	struct sockaddr_in sa;

	ZeroMemory((void *)&sa, sizeof(sa));

	fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if (fd < 0) {
		return -1;
	}

	sa.sin_family = AF_INET;
	CopyMemory((void *)&sa.sin_addr, (void *)&ip, sizeof(struct in_addr));
	sa.sin_port = htons(port);

	/* Reuse port */

	rcSockopt = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&turn_on_option_flag, sizeof(turn_on_option_flag));

	/* Increase the TCP/IP buffer size */
	bufsize = TCP_BUFFER_SIZE;
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, sizeof(bufsize));
	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize, sizeof(bufsize));

	/* bind() the socket to the interface */
	if (::bind(fd, (struct sockaddr *)&sa, sizeof(struct sockaddr)) < 0) {
		return -2;
	}
	return fd;
}
int32_t Server::TcpSetNonblocking(int32_t fd)
{
	u_long flags = 1;
	if (ioctlsocket(fd, FIONBIO, &flags))
		return 1;
	return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&flags, sizeof(flags));
}
int32_t Server::TcpListen(int32_t sockfd)
{
	if (listen(sockfd, 10) < 0)
	{
		return 1;
	}
	TcpSetNonblocking(sockfd);
	return 0;
}
int32_t Server::TcpAccept(int32_t sockfd, struct sockaddr *client_addr, int32_t *addr_len)
{
	int fd, bufsize;

	if ((fd = accept(sockfd, client_addr, addr_len)) != INVALID_SOCKET)
	{
		bufsize = TCP_BUFFER_SIZE;
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&bufsize, sizeof(bufsize));
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&bufsize, sizeof(bufsize));
		TcpSetNonblocking(fd);
		return fd;
	}
	else
	{
		bufsize = WSAGetLastError();
		if (bufsize != WSAEWOULDBLOCK)
			logger->Log(LOGTYPE_COMM, L"Could not accept connection.");
		return -1;
	}
}
bool Server::AcceptConnection()
{
	int32_t listen_length;
	struct sockaddr_in listen_in;
	CLIENT* workConnect;

	listen_length = sizeof(listen_in);

	while ((TempSocket = TcpAccept(ServerSocket, (struct sockaddr*) &listen_in, &listen_length)) != -1)
	{
		int32_t freeConnection = FreeConnection();
		if (freeConnection != -1)
		{
			workConnect = connections[freeConnection];
			if (!workConnect) break;
			workConnect->connection_index = freeConnection;
			serverConnections[serverNumConnections++] = freeConnection;
			workConnect->ClientSocket = TempSocket;
			InetNtopA(AF_INET, &listen_in.sin_addr, (char*)&workConnect->IP_Address[0], 16);
			*(uint32_t *)&workConnect->ipaddr = *(uint32_t *)&listen_in.sin_addr;
			logger->Log(LOGTYPE_COMM, L"Accepted SERVER connection from %s:%u", ToWide((char*)workConnect->IP_Address).c_str(), listen_in.sin_port);

			char titleBuf[0x100];
			snprintf(titleBuf, sizeof(titleBuf), "SBOL DB Server v%s. Clients: %u", logger->ToNarrow(VERSION_STRING).c_str(), GetClientCount());
			SetConsoleTitleA(titleBuf);

			return true;
		}
	}
	if (TempSocket != -1)
		closesocket(TempSocket);
	return false;
}
bool Server::Send(CLIENT* client)
{
	int32_t sent_len, wserror, max_send;
	uint8_t sendbuf[CLIENT_BUFFER_SIZE];
	SEND_QUEUE entry;

	if (client->snddata > 0 || client->MessagesInSendQueue())
	{
		if (client->snddata <= 0)
		{
			entry = client->GetFromSendQueue();
			client->snddata = *(int16_t*)&entry.sndbuf[0] + 2;
			CopyMemory(client->sndbuf, entry.sndbuf, CLIENT_BUFFER_SIZE);
		}

		if (client->snddata > CLIENT_BUFFER_SIZE)
			max_send = CLIENT_BUFFER_SIZE;
		else
			max_send = client->snddata;

		if((sent_len = send(client->ClientSocket, (char *)&client->sndbuf[0], max_send, 0)) == SOCKET_ERROR)
		{
			wserror = WSAGetLastError();
			if (wserror != WSAEWOULDBLOCK)
			{
				logger->Log(LOGTYPE_COMM, L"Could not write data to client %s. Socket Error %X08", ToWide((char*)client->IP_Address).c_str(), wserror);
				client->Disconnect();
				return false;
			}
		}
		else
		{
			client->snddata -= sent_len;

			if (client->snddata > 0)
			{
				CopyMemory(&sendbuf[0], &client->sndbuf[sent_len], client->snddata);
				CopyMemory(&client->sndbuf[0], &sendbuf[0], client->snddata);
			}
		}
	}
	return true;
}
bool Server::Recv(CLIENT* client)
{
	int32_t  wserror, recv_len, max_read;
	
	max_read = CLIENT_BUFFER_SIZE - client->rcvread;

	if ((recv_len = recv(client->ClientSocket, (char *)&client->rcvbuf[client->rcvread], max_read, 0)) == SOCKET_ERROR)
	{
		wserror = WSAGetLastError();
		if (wserror != WSAEWOULDBLOCK)
		{
			logger->Log(LOGTYPE_COMM, L"Could not read data from client %s. Socket Error %X08", ToWide((char*)client->IP_Address).c_str(), wserror);
			client->Disconnect();
			return false;
		}
	}
	else
	{
	_processpacket:
		if (recv_len == 0)
		{
			wserror = WSAGetLastError();
			if (wserror == WSAECONNRESET)
				return false;
		}
		ProcessRecv(client, recv_len);
		return true;
	}
	if (client->rcvread && recv_len == -1)
	{
		recv_len = 0;
		goto _processpacket;
	}
	return true;
}
bool Server::ProcessRecv(CLIENT* client, int32_t len)
{
	uint8_t recvbuf[CLIENT_BUFFER_SIZE];
	if(!client || len < 0)
		return false;

	client->rcvread += len;
	if ((client->hassize == false || client->packetsize == 0) && client->rcvread >= 2)
	{
		client->packetsize = *(uint16_t*)&client->rcvbuf[0] + 2;
		client->hassize = true;
	}
	if (client->hassize == true && client->rcvread >= client->packetsize && client->packetsize >= 6)
	{
		CopyMemory(&recvbuf[0], &client->rcvbuf[0], client->packetsize);
		CheckClientPackets(client, client->packetsize, recvbuf);
		CopyMemory(&recvbuf[0], &client->rcvbuf[client->packetsize], CLIENT_BUFFER_SIZE - client->packetsize);
		CopyMemory(&client->rcvbuf[0], &recvbuf[0], CLIENT_BUFFER_SIZE - client->packetsize);
		client->hassize = false;
		client->rcvread -= client->packetsize;
		client->packetsize = 0;
		return true;
	}
	return false;
}
bool Server::RecvToProcess(CLIENT* client)
{
	if (!client || client->todc == true)
		return false;
	return client->rcvread ? true : false;
}
int32_t Server::FreeConnection()
{
	CLIENT* wc;

	for (uint32_t fc = 0; fc < connections.size(); fc++)
	{
		wc = connections[fc];
		if (wc->ClientSocket<0)
			return fc;
	}
	return -1;
}
void Server::ServerThread(void* parg)
{
	Server* server = (Server*)parg;
	int32_t selectcount;
	struct in_addr server_in;
	WSADATA winsock_data;
	CLIENT* workConnect;
	fd_set ReadFDs, WriteFDs;
	WSAStartup(MAKEWORD(2, 2), &winsock_data);

	struct timeval select_timeout = {
		0,
		1
	};

	if (!server)
		return;

	for (auto& connection : server->connections)
	{
		server->InitializeConnection(connection);
	}

	server_in.s_addr = INADDR_ANY;
	server->ServerSocket = server->TcpSockOpen(server_in, server->serverport);

	if (server->ServerSocket < 0)
	{
		switch (server->ServerSocket)
		{
		case -1:
			server->logger->Log(LOGTYPE_ERROR, L"Could not create socket.");
			break;
		case -2:
			server->logger->Log(LOGTYPE_ERROR, L"Could not bind to port %u.", server->serverport);
			break;
		default:
			break;
		}
		server->Stop();
		return;
	}

	if (server->TcpListen(server->ServerSocket))
		server->logger->Log(LOGTYPE_ERROR, L"Could not listen to port %u.", server->serverport);
	else
		server->logger->Log(LOGTYPE_COMM, L"Listening on port %u.", server->serverport);
	
	server->running = true;

	while (server->running)
	{
		while(server->paused) 
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		FD_ZERO(&ReadFDs);
		FD_ZERO(&WriteFDs);

		// Read/write data from clients.
		for(auto& workConnect : server->connections)
		{
			if ((workConnect) && (workConnect->ClientSocket >= 0))
			{
				if (!workConnect->todc)
				{
					FD_SET(workConnect->ClientSocket, &ReadFDs);
				}
				if (workConnect->snddata > 0 || workConnect->MessagesInSendQueue())
					FD_SET(workConnect->ClientSocket, &WriteFDs);
			}
		}

		// Set Read on server socket
		FD_SET(server->ServerSocket, &ReadFDs);
		
		selectcount = select(0, &ReadFDs, &WriteFDs, NULL, &select_timeout);

		// Accept incomming connection
		if (FD_ISSET(server->ServerSocket, &ReadFDs))
		{
			FD_CLR(server->ServerSocket, &ReadFDs);
			server->AcceptConnection();
		}

		for (uint32_t i = 0; i < server->connections.size(); i++)
		{
			workConnect = server->connections[i];

			if ((workConnect) && (workConnect->ClientSocket >= 0))
			{
				// Send
				if (FD_ISSET(workConnect->ClientSocket, &WriteFDs))
				{
					FD_CLR(workConnect->ClientSocket, &WriteFDs);
					if (server->Send(workConnect) == false)
						workConnect->Disconnect();
				}
				// Receive
				//if (FD_ISSET(workConnect->ClientSocket, &ReadFDs))
				{
					//FD_CLR(workConnect->ClientSocket, &ReadFDs);
					if (server->Recv(workConnect) == false)
						workConnect->Disconnect();
				}
				if (workConnect->todc)
					server->InitializeConnection(workConnect);

				if (workConnect->lastHeartBeat < time(NULL))
				{
					workConnect->lastHeartBeat = time(NULL) + HEARTBEAT_TIME;
					workConnect->SendHeartBeat();
				}
			}
		}
		if (selectcount == 0)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
void Server::SendToAll(SERVERPACKET* src)
{
	for (uint32_t i = 0; i < connections.size(); i++)
	{
		if (connections[i]->ClientSocket >= 0) connections[i]->Send(src);
	}
}

uint32_t Server::GetClientCount()
{
	uint32_t count = 0;
	for (uint32_t i = 0; i < connections.size(); i++)
	{
		if (connections[i]->ClientSocket >= 0) count++;
	}
	return count;
}
