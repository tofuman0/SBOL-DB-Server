#include <Windows.h>
#include "..\server.h"

void ClientOperations(CLIENT* client)
{
	Server* server = (Server*)client->server;
	client->inbuf.setOffset(0x06);

	switch (client->inbuf.getSubType())
	{
	case 0x0000: // Offline Chat
	{
		uint8_t type = client->inbuf.get<uint8_t>();
		std::string fromHandle = client->inbuf.getStringA(0x10);
		std::string toHandle = client->inbuf.getStringA(0x10);
		std::string message = client->inbuf.getStringA(0x4E);
		client->outbuf.clearBuffer();
		client->outbuf.setSize(0x06);
		client->outbuf.setOffset(0x06);
		client->outbuf.setType(0x0004);
		client->outbuf.setSubType(0x0000);
		client->outbuf.append<uint8_t>(type);
		client->outbuf.appendString(fromHandle, 0x10);
		client->outbuf.appendString(toHandle, 0x10);
		client->outbuf.appendString(message, 0x4E);
		server->SendToAll(&client->outbuf);
	}
	return;
	case 0x0001: // Inform Client
	{
		std::string toHandle = client->inbuf.getStringA(0x10);
		std::string message = client->inbuf.getStringA(0x4E);
		int32_t colour = client->inbuf.get<uint32_t>();
		client->outbuf.clearBuffer();
		client->outbuf.setSize(0x06);
		client->outbuf.setOffset(0x06);
		client->outbuf.setType(0x0004);
		client->outbuf.setSubType(0x0001);
		client->outbuf.appendString(toHandle, 0x10);
		client->outbuf.appendString(message, 0x4E);
		client->outbuf.append<uint32_t>(colour);
		server->SendToAll(&client->outbuf);
	}
	return;
	case 0x0002: // Team Chat
	{
		std::string fromHandle = client->inbuf.getStringA(0x10);
		std::string message = client->inbuf.getStringA(0x4E);
		int32_t teamID = client->inbuf.get<uint32_t>();
		client->outbuf.clearBuffer();
		client->outbuf.setSize(0x06);
		client->outbuf.setOffset(0x06);
		client->outbuf.setType(0x0004);
		client->outbuf.setSubType(0x0002);
		client->outbuf.appendString(fromHandle, 0x10);
		client->outbuf.appendString(message, 0x4E);
		client->outbuf.append<uint32_t>(teamID);
		server->SendToAll(&client->outbuf);
	}
	return;
	}
}