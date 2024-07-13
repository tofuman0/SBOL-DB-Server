#pragma once
//	Main Packet functions.
#include "server.h"

typedef void(PacketFunction)(CLIENT* client);

void ServerAuth(CLIENT* client);
void ClientAuth(CLIENT* client);
void ClientRequests(CLIENT* client);
void ClientOperations(CLIENT* client);
void ServerOperations(CLIENT* client);

void ClientPacketDoNothing(CLIENT* client) { }

PacketFunction* MainPacketFunctions[] =
{
	&ServerAuth,				// 0x00 - Handshake: Verify keys and encryption
	&ClientAuth,				// 0x01 - Verify client login: Check login. Including if banned and reason.
	&ClientRequests,			// 0x02 - Request client data: Retrieve car, garage and team information.
	&ClientPacketDoNothing,		// 0x03 - TBC
	&ClientOperations,			// 0x04 - Client operations: Kick, ban message clients
	&ServerOperations,			// 0x05 - Get Server Stats: Total player count, races in progress ....
};

