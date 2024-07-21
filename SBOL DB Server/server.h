#pragma once
#include <nlohmann\json.hpp>
#include <openssl/sha.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <iostream>
#include <fstream>
#include <exception>
#include <thread>
#include <vector>
#include "globals.h"
#include "database.h"
#include "Logger.h"
#include "client.h"

using json = nlohmann::json;

class Server
{
public:
	Server(bool type = false);
	~Server();
	std::vector<CLIENT*> connections;
	Logger* logger;
	Database* db;
	int32_t ServerSocket;
	int32_t TempSocket;
	char* HexString(uint8_t* in, char* out, uint32_t length);
	char* HexString(uint8_t* in, uint32_t length);
	int32_t Start();
	int32_t Stop();
	int32_t Pause();
	int32_t Unpause();
	void Initialize();
	int32_t CreateAccount(wchar_t* username, wchar_t* password, wchar_t* email, wchar_t* privileges);
	int32_t SetPassword(wchar_t* username, wchar_t* password);
	int32_t CreateKey();
	bool GetHash(uint8_t *src, uint8_t* dst, uint32_t length);
	int32_t LoadConfig(const wchar_t* filename);
	void CloseDB();
	bool IsRunning();
	static void ServerThread(void* parg);
	void CheckClientPackets(CLIENT* client, uint32_t rcvcopy, uint8_t * tmprcv);
	void InitializeConnection(CLIENT* connect);
	int32_t TcpSockOpen(struct in_addr ip, uint16_t port);
	int32_t TcpSetNonblocking(int32_t fd);
	int32_t TcpListen(int32_t sockfd);
	int32_t TcpAccept(int32_t sockfd, struct sockaddr *client_addr, int32_t *addr_len);
	bool AcceptConnection();
	bool Send(CLIENT* client);
	bool Recv(CLIENT* client);
	bool ProcessRecv(CLIENT* client, int32_t len);
	bool RecvToProcess(CLIENT* client);
	int32_t FreeConnection();
	uint32_t GetDatabasetype() { return databasetype; }
	void SendToAll(SERVERPACKET* src);
	uint32_t GetClientCount();
private:
	std::vector<uint32_t> serverConnections;
	uint32_t serverNumConnections;
	bool running;
	bool paused;
	bool isService;
	char hexStr[(MAX_MESG_LEN + 1) * 2];
	const wchar_t* CONFIG_FILENAME = L"config.json";
	const char* DEFAULT_JSON_CONFIG = "{\n\t\"databasetype\":\"sqlite\",\n\t\"databasename\":\"sbol.db\",\n\t\"databasehost\":\"localhost\",\n\t\"databaseusername\":\"sbol\",\n\t\"databasepassword\":\"sbol\",\n\t\"databaseport\":3306,\n\t\"serverport\":7946,\n\t\"webserver\":true,\n\t\"webserverport\":8080,\n\t\"logpath\":\".\\\\log\"\n}";
	uint32_t databasetype;
	std::string databaseusername;
	std::string databasepassword;
	std::string databasehost;
	std::string databasename;
	uint16_t databaseport;
	bool webserver;
	uint16_t serverport;
	uint16_t webserverport;
	uint32_t serverClientLimit;
	std::wstring ToWide(std::string in);
	std::string ToNarrow(std::wstring in);
	int32_t VerifyConfig();
	int32_t checkDB();
};
