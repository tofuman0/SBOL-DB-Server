#include "database.h"

Database::Database()
{
	type = 0;
	username = "";
	password = "";
	host = "";
	name = "";
	port = 0;
}

Database::Database(uint32_t dbType, const char* dbUsername, const char* dbPassword, const char* dbHost, const char* dbName, uint16_t dbPort)
{
	type = dbType;
	username = dbUsername;
	password = dbPassword;
	host = dbHost;
	name = dbName;
	port = dbPort;
}

Database::~Database()
{
	results.clear();
	if (type == DBTYPE_SQLITE)
		sqlite3_close(sqliteDb);
	else
		mysql_close(mysql);
}

static int DatabaseCallback(void* data, int argc, char** argv, char** azColName)
{
	std::vector<std::map<std::string, std::string>>* result = (std::vector<std::map<std::string, std::string>>*)data;
	if (!result)
		return 1;

	std::map<std::string, std::string> row;

	for (int i = 0; i < argc; i++) {
		std::string col = azColName[i];
		std::string val = argv[i] ? std::string(argv[i]) : std::string("NULL");
		row.insert(std::pair<std::string, std::string>(col, val));
	}
	result->push_back(row);
	return DB_OK;
}

// SQLite
int Database::Open(const char* filename)
{
	return sqlite3_open(filename, &sqliteDb);
}
// MySQL
int Database::Open(const char * host, uint16_t port, const char * username, const char * password, const char * database)
{
	if ((mysql = mysql_init(NULL)))
	{
		if (!mysql_real_connect(mysql, host, username, password, database, port, NULL, 0))
			return DB_ERR;
		else
			return DB_OK;
	}
	else
		return DB_ERR;
}

int Database::Open()
{
	if (type == DBTYPE_SQLITE)
		return OpenSqlite();
	else
		return OpenMysql();
}

int Database::OpenSqlite()
{
	return sqlite3_open(name.c_str(), &sqliteDb);
}

int Database::OpenMysql()
{
	return Open(host.c_str(), port, username.c_str(), password.c_str(), name.c_str());
}

int Database::Exec(const char* sql)
{
	if (type == DBTYPE_SQLITE)
		return ExecSqlite(sql);
	else
		return ExecMysql(sql);
}

char * Database::ErrorSqlite()
{
	return dbErr;
}

char * Database::ErrorMysql()
{
	dbErr = (char*)mysql_error(mysql);
	return dbErr;
}

int Database::ExecSqlite(const char* sql)
{
	results.clear();
	sqlite3_exec(sqliteDb, sql, DatabaseCallback, (void*)&results, &dbErr);
	return results.size();
}

int Database::ExecMysql(const char* sql)
{
	results.clear();
	mysql_real_query(mysql, sql, strlen(sql));
	MYSQL_RES* result = mysql_store_result(mysql);
	int32_t num_fields = mysql_field_count(mysql);
	MYSQL_FIELD* field;
	unsigned long* lengths;
	MYSQL_ROW mysql_row;
	if (result)
	{
		
		std::vector<std::string> fieldnames;
		for (int32_t i = 0; i < num_fields; i++)
		{
			field = mysql_fetch_field(result);
			fieldnames.push_back(field->name);
		}
		while ((mysql_row = mysql_fetch_row(result)) != NULL)
		{
			std::map<std::string, std::string> row;
			lengths = mysql_fetch_lengths(result);
			for (int32_t i = 0; i < num_fields; i++)
			{
				char buf[30];
				std::string col = std::string(fieldnames[i].length() > 0 ? fieldnames[i].c_str() : _itoa(i, buf, 10));
				std::string val;
				if (!lengths[i])
					val = std::string("NULL");
				else
				{
					val.resize(lengths[i]);
					memcpy(&val[0], mysql_row[i], lengths[i]);
				}
				row.insert(std::pair<std::string, std::string>(col, val));
				
			}
			results.push_back(row);
		}
	}
	return results.size();
}

db_blob Database::GetBlob(const char* sql, uint32_t col)
{
	if (type == DBTYPE_SQLITE)
		return GetBlobSqlite(sql, col);
	else
		return GetBlobMysql(sql, col);
}

db_blob Database::GetBlobSqlite(const char* sql, uint32_t col)
{
	sqlite3_stmt *pStmt;
	db_blob blob = { 0 };
	int32_t res = sqlite3_prepare_v2(sqliteDb, sql, -1, &pStmt, 0);
	if (res != SQLITE_OK) return blob;
	res = sqlite3_step(pStmt);
	if (res != SQLITE_OK && res != SQLITE_ROW) return blob;
	const void* data = sqlite3_column_blob(pStmt, col);
	blob.size = sqlite3_column_bytes(pStmt, col);
	blob.data.resize(blob.size);
	memcpy(blob.data.data(), (uint8_t*)data, blob.size);
	sqlite3_finalize(pStmt);
	return blob;
}

db_blob Database::GetBlobMysql(const char* sql, uint32_t col)
{
	db_blob blob = { 0 };
	int32_t res = ExecMysql(sql);
	if (res)
	{
		std::string data = GetValue(&results, 0, col);
		blob.size = data.length();
		blob.data.assign(data.begin(), data.end());
	}
	return blob;
}

int Database::Close()
{
	if (type == DBTYPE_SQLITE)
		return CloseSqlite();
	else
		return CloseMysql();
}

char * Database::Error()
{
	if (type == DBTYPE_SQLITE)
		return ErrorSqlite();
	else
		return ErrorMysql();
}

int Database::CloseSqlite()
{
	return sqlite3_close(sqliteDb);
}

int Database::CloseMysql()
{
	if(mysql)
		mysql_close(mysql);
	return DB_OK;
}

std::string Database::GetValue(std::vector<std::map<std::string, std::string>>* data, int row, int column)
{
	uint32_t index = 0;
	auto ptr = next(data->begin(), row);
	std::map<std::string, std::string>::iterator rowend = ptr->end();
	for (std::map<std::string, std::string>::const_iterator it = ptr->begin(); it != rowend; ++it)
	{
		if (column == index)
			return it->second;
		index++;
	}
	return "";
}

std::string Database::GetValue(std::vector<std::map<std::string, std::string>>* data, int row, char* column)
{
	try {
		uint32_t index = 0;
		auto ptr = next(data->begin(), row);
		if (ptr->size() > 0)
			return ptr->at(column);
		else
			return "";
	}
	catch (std::exception ex)
	{
		return "";
	}
}