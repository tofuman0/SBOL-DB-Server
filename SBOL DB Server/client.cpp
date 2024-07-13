#include <Windows.h>
#include "globals.h"
#include "client.h"
#include "packets.h"
#include <openssl/sha.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

CLIENT::CLIENT()
{
	Initialize();
}
CLIENT::~CLIENT()
{
}
void CLIENT::Initialize()
{
	ClientSocket = -1;
	todc = false;
	isAuth = false;
	lastHeartBeat = 0;
	snddata = 0;
	connection_index = 0;
	rcvread = 0;
	packetsize = 0;
	packetoffset = 0;
	ZeroMemory(&IP_Address[0], sizeof(IP_Address));
	ZeroMemory(&ipaddr[0], sizeof(ipaddr));
	inbuf.clearBuffer();
	outbuf.clearBuffer();
	ZeroMemory(&rcvbuf[0], sizeof(rcvbuf));
	ZeroMemory(&sndbuf[0], sizeof(sndbuf));
	ZeroMemory(&key[0], sizeof(key));
	ZeroMemory(&iv[0], sizeof(iv));
	ClearSendQueue();
}
void CLIENT::Send(SERVERPACKET* src)
{
	if (src == nullptr)
		src = &outbuf;
	if (ClientSocket >= 0)
	{
		if (CLIENT_BUFFER_SIZE < ((int32_t)src->getSize() + 15))
			Disconnect();
		else
		{
			uint16_t size = src->getSize();
			if ((size - 4) < 0 || size > CLIENT_BUFFER_SIZE)
			{
				logger->Log(LOGTYPE_ERROR, L"Unabled to send packet to client %s as packet will be %ubytes", logger->ToWide((char*)IP_Address).c_str(), size);
				return;
			}
			src->setSize(size - 2);
			uint16_t newsize = src->getSize();
			// Compress Packet
#ifndef DISABLE_COMPRESSION
			string compressed;
			compressed.resize(newsize);
			CopyMemory((char*)compressed.data(), &src->buffer[0x02], newsize);
			compressed = compress(compressed);
			newsize = compressed.size();
			if (newsize > CLIENT_BUFFER_SIZE)
			{
				logger->Log(LOGTYPE_ERROR, L"Unabled to send packet to client %s as packet will be %ubytes", logger->ToWide((char*)IP_Address).c_str(), newsize);
				return;
			}
			if (newsize)
			{
				inbuf.clearBuffer();
				inbuf.setArray((uint8_t*)compressed.data(), newsize, 0x02);
			}
			src->setSize(newsize);
#endif
			while (src->getSize() % 16 || src->getSize() <= 16)
				src->append<uint8_t>(0);
			newsize = src->getSize();
			if (isAuth == true)
			{
				if (newsize + BLOCK_SIZE > CLIENT_BUFFER_SIZE)
				{
					logger->Log(LOGTYPE_ERROR, L"Unabled to send packet to client %s as packet will be %ubytes", logger->ToWide((char*)IP_Address).c_str(), newsize + BLOCK_SIZE);
					return;
				}
				newsize = Encrypt(&src->buffer[0x02], &src->buffer[0x02], newsize);
				if (newsize < BLOCK_SIZE) return;
			}
			AddToSendQueue(src);
		}
	}
}
void CLIENT::Disconnect()
{
	todc = true;
}
void CLIENT::ProcessPacket()
{
	if (!todc)
	{
		try {
			int16_t size = inbuf.getSize();
			if (isAuth == true)
			{
				size = Decrypt(&inbuf.buffer[0x02], &inbuf.buffer[0x02], size);
				if (size < 16) return;
				inbuf.setSize(size);
			}

			// Decompress Packet
#ifndef DISABLE_COMPRESSION
			string decompressed;
			decompressed.resize(size+10);
			CopyMemory((char*)decompressed.data(), &inbuf.buffer[0x02], size);
			decompressed = decompress(decompressed);
			size = decompressed.size();
			if (size)
			{
				inbuf.clearBuffer();
				inbuf.setArray((uint8_t*)decompressed.data(), size, 0x02);
			}
			else
				return;
			inbuf.setSize(size);
#endif
			
		}
		catch (std::exception ex)
		{
			logger->Log(LOGTYPE_COMM, L"Invalid Packet Message: %04X from client %s", inbuf.getType(), logger->ToWide((char*)IP_Address).c_str());
			Disconnect();
			return;
		}

		if (inbuf.getType() > sizeof(MainPacketFunctions) / 4 || (isAuth == false && inbuf.getType() != PACKETTYPE_AUTH))
		{
			logger->Log(LOGTYPE_COMM, L"Invalid Packet Message: %04X from client %s", inbuf.getType(), logger->ToWide((char*)IP_Address).c_str());
			Disconnect();
			return;
		}
#ifdef PACKET_OUTPUT
		if (inbuf.getType() != 0x0000 && inbuf.getSubType() != 0x0001)
		{
			logger->Log(LOGTYPE_PACKET, L"Packet: Client -> Server");
			logger->Log(LOGTYPE_PACKET, logger->PacketToText(&inbuf.buffer[0], inbuf.getSize()).c_str());
		}
#endif
		MainPacketFunctions[inbuf.getType()](this);
	}
}
int32_t CLIENT::Encrypt(uint8_t* src, uint8_t* dst, int32_t len)
{
#ifndef DISABLE_ENCRYPTION
	try {
		int32_t _len;
		int32_t _ciphertext_len;
		EVP_CIPHER_CTX *ctx = nullptr;
		ctx = EVP_CIPHER_CTX_new();

		if (ctx != nullptr)
		{
			if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, &key[0], &iv[0]) != 1) goto crypterror;
			EVP_CIPHER_CTX_set_padding(ctx, 0);
			if (EVP_EncryptUpdate(ctx, dst, &_len, src, len) != 1) goto crypterror;
			_ciphertext_len = _len;
			if (EVP_EncryptFinal_ex(ctx, dst + len, &_len) != 1) goto crypterror;
			_ciphertext_len += _len;
			EVP_CIPHER_CTX_free(ctx);

			return _ciphertext_len;
		}
		else
		{
		crypterror:
			ERR_print_errors_fp(stderr);
			logger->Log(LOGTYPE_ERROR, L"Unable to encrypt data.");
			return -1;
		}
	}
	catch (exception ex) {
		ERR_print_errors_fp(stderr);
		logger->Log(LOGTYPE_ERROR, L"Unable to encrypt data: %s", ex.what());
		return -1;
	}
#else
	CopyMemory(dst, src, len);
	return len;
#endif
}
int32_t CLIENT::Decrypt(uint8_t* src, uint8_t* dst, int32_t len)
{
#ifndef DISABLE_ENCRYPTION
	try {
		int32_t _len;
		int32_t _ciphertext_len;
		EVP_CIPHER_CTX *ctx = nullptr;
		ctx = EVP_CIPHER_CTX_new();

		if (ctx != nullptr)
		{
			if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, &key[0], &iv[0]) != 1) goto crypterror;
			EVP_CIPHER_CTX_set_padding(ctx, 0);
			if (EVP_DecryptUpdate(ctx, dst, &_len, src, len) != 1) goto crypterror;
			_ciphertext_len = _len;
			if (EVP_DecryptFinal_ex(ctx, dst + _len, &_len) != 1) goto crypterror;
			_ciphertext_len += _len;
			EVP_CIPHER_CTX_free(ctx);

			return _ciphertext_len;
		}
		else
		{
		crypterror:
			ERR_print_errors_fp(stderr);
			logger->Log(LOGTYPE_ERROR, L"Unable to decrypt data.");
			return -1;
		}

	}
	catch (exception ex) {
		ERR_print_errors_fp(stderr);
		logger->Log(LOGTYPE_ERROR, L"Unable to decrypt data: %s", ex.what());
		return -1;
	}
#else
	CopyMemory(dst, src, len);
	return len;
#endif
}
int32_t CLIENT::Compress(uint8_t* src, uint8_t* dst, int32_t len)
{	// TODO: implement compression
	int32_t result = 0;
	return result;
}
int32_t CLIENT::Decompress(uint8_t* src, uint8_t* dst, int32_t len)
{	// TODO: implement decompression
	int32_t result = 0;
	return result;
}
void CLIENT::AddToSendQueue(SERVERPACKET* src)
{
	*(uint16_t*)&src->buffer[0x00] = src->getSize();
	SEND_QUEUE entry = { 0 };
	CopyMemory(&entry.sndbuf[0], &src->buffer[0x00], min(src->getSize() + 2, sizeof(sndbuf) - 2));
	std::lock_guard<std::mutex> locker(_muClient);
	sendQueue.push(entry);
}
SEND_QUEUE CLIENT::GetFromSendQueue()
{
	SEND_QUEUE current = { 0 };
	std::lock_guard<std::mutex> locker(_muClient);
	if (sendQueue.size())
	{
		current = sendQueue.front();
		sendQueue.pop();
		return current;
	}
	else
	{
		return current;
	}
}
uint32_t CLIENT::MessagesInSendQueue()
{
	std::lock_guard<std::mutex> locker(_muClient);
	if (sendQueue.size() > 10)
	{
		logger->Log(LOGTYPE_COMM, L"Send Queue exceeds 10 for Battle Server %s. %u in queue",
			logger->ToWide((char*)IP_Address).c_str(),
			sendQueue.size()
		);
	}
	else if (sendQueue.size() > 100)
	{
		logger->Log(LOGTYPE_COMM, L"Send Queue exceeds 100 for Battle Server %s so was disconnected",
			logger->ToWide((char*)IP_Address).c_str()
		);
		Disconnect();
		return 0;
	}
	return sendQueue.size();
}
void CLIENT::ClearSendQueue()
{
	std::lock_guard<std::mutex> locker(_muClient);
	std::queue<SEND_QUEUE> q;
	sendQueue.swap(q);
}
#pragma region Client Packets
void CLIENT::sendAuth()
{
	outbuf.clearBuffer();
	outbuf.setType(0x0000);
	outbuf.setSubType(0x0000);
	outbuf.setSize(0x06);
	outbuf.setOffset(0x06);
	uint32_t rnd = rand();
	outbuf.append<uint32_t>(rnd);
	outbuf.append<uint32_t>(rnd / 8);
	outbuf.appendString(std::string("Tofuman"), 0x10);
	Send();
}
void CLIENT::SendHeartBeat()
{
	outbuf.clearBuffer();
	outbuf.setType(0x0000);
	outbuf.setSubType(0x0001);
	outbuf.setSize(0x06);
	outbuf.setOffset(0x06);
	uint32_t currTime = timeGetTime();
	outbuf.append<uint32_t>(currTime);
	Send();
}
#pragma endregion