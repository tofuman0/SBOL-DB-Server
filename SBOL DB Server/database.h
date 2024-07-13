#pragma once
#define my_socket_defined
typedef int my_socket;

#include <string>
#include <map>
#include <vector>
#include <SQLite\sqlite3.h>
#include <mysql.h>
#include <iostream>
#include <exception>
#include "globals.h"

struct db_blob {
	uint32_t size;
	std::vector<uint8_t> data;
};

class Database
{
public:
	Database();
	Database(uint32_t dbType, const char* dbUsername, const char* dbPassword, const char* dbHost, const char* dbName, uint16_t dbPort);
	~Database();
	std::vector<std::map<std::string, std::string>> results;
	char* dbErr;
	std::string GetValue(std::vector<std::map<std::string, std::string>>* data, int row, int column);
	std::string GetValue(std::vector<std::map<std::string, std::string>>* data, int row, char* column);
	int Open(const char* filename);
	int Open(const char* host, uint16_t port, const char* username, const char* password, const char* database);
	int Open();
	int Exec(const char* sql);
	db_blob GetBlob(const char* sql, uint32_t col = 0);
	int Close();
	char* Error();
private:
	std::string tempStr;
	sqlite3* sqliteDb;
	MYSQL* mysql;
	uint32_t type;
	std::string username;
	std::string password;
	std::string host;
	std::string name;
	uint16_t port;
	char* ErrorSqlite();
	char* ErrorMysql();
	int ExecSqlite(const char* sql);
	int ExecMysql(const char* sql);
	int OpenSqlite();
	int OpenMysql();
	db_blob GetBlobSqlite(const char* sql, uint32_t col = 0);
	db_blob GetBlobMysql(const char* sql, uint32_t col = 0);
	int CloseSqlite();
	int CloseMysql();
};

