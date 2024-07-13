#pragma once
#include "serverpacket.h"
#include "Logger.h"
#include <vector>
#include <queue>
#include <mutex>

typedef struct st_sendqueue {
	uint8_t sndbuf[CLIENT_BUFFER_SIZE];
} SEND_QUEUE;

class CLIENT
{
	std::mutex _muClient;
public:
	CLIENT();
	~CLIENT();
	Logger* logger;
	void* server;
	int32_t ClientSocket;
	bool todc;
	bool isAuth;
	time_t lastHeartBeat = 0;
	int32_t snddata;
	uint32_t connection_index;
	uint32_t rcvread;
	uint16_t packetsize;
	uint16_t packetoffset;
	bool hassize;
	bool packetready;
	uint8_t IP_Address[16]; // Text version
	uint8_t ipaddr[4]; // Binary version
	uint8_t rcvbuf[CLIENT_BUFFER_SIZE];
	uint8_t sndbuf[CLIENT_BUFFER_SIZE];
	uint8_t key[32];
	uint8_t iv[16];
	SERVERPACKET inbuf;
	SERVERPACKET outbuf;
	void SetAuth() { isAuth = TRUE; }
	void ClearAuth() { isAuth = FALSE; }
	void Initialize();
	void Send(SERVERPACKET* src = nullptr);
	void Disconnect();
	void ProcessPacket();
	int32_t Encrypt(uint8_t* src, uint8_t* dst, int32_t len);
	int32_t Decrypt(uint8_t* src, uint8_t* dst, int32_t len);
	int32_t Compress(uint8_t* src, uint8_t* dst, int32_t len);
	int32_t Decompress(uint8_t* src, uint8_t* dst, int32_t len);
	void AddToSendQueue(SERVERPACKET* src);
	SEND_QUEUE GetFromSendQueue();
	uint32_t MessagesInSendQueue();
	void ClearSendQueue();
#pragma region Client Packets
	void sendAuth();
	void SendHeartBeat();
#pragma endregion
private:
	std::queue<SEND_QUEUE> sendQueue;
};

