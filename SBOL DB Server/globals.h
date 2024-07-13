#pragma once
#ifdef _DEBUG
#define PACKET_OUTPUT
#endif
// TODO: FIX COMPRESSION
#define DISABLE_COMPRESSION
#define DISABLE_ENCRYPTION

#define DBTYPE_SQLITE			0
#define DBTYPE_MYSQL			1

#define VERSION_STRING			L"0.1"
#define MAX_MESG_LEN			4096
//#define CLIENT_BUFFER_SIZE		64000
#define CLIENT_BUFFER_SIZE		(1024 * 8)
#define TCP_BUFFER_SIZE			65536
#define BLOCK_SIZE				16
#define KEY_SIZE				32

#define HEARTBEAT_TIME			30

#define DB_OK					0
#define DB_TABLE_MISSING		1
#define DB_ERR					255

#define SWAP_SHORT(l)						   \
            ( ( ((l) >>  8) & 0x00FF ) |       \
              ( ((l) <<  8) & 0xFF00 ) )

#define SWAP_LONG(l)                                \
            ( ( ((l) >> 24) & 0x000000FFL ) |       \
              ( ((l) >>  8) & 0x0000FF00L ) |       \
              ( ((l) <<  8) & 0x00FF0000L ) |       \
              ( ((l) << 24) & 0xFF000000L ) )

#define SWAP_LONGLONG(l)                                     \
            ( ( ((l) >> 56) & 0x00000000000000FFLL ) |       \
              ( ((l) >> 40) & 0x000000000000FF00LL ) |       \
              ( ((l) >> 24) & 0x0000000000FF0000LL ) |       \
              ( ((l) >>  8) & 0x00000000FF000000LL ) |       \
              ( ((l) <<  8) & 0x000000FF00000000LL ) |       \
              ( ((l) << 24) & 0x0000FF0000000000LL ) |       \
              ( ((l) << 40) & 0x00FF000000000000LL ) |       \
              ( ((l) << 56) & 0xFF00000000000000LL ) )

enum LOGTYPE {
	LOGTYPE_NONE,
	LOGTYPE_PACKET,
	LOGTYPE_DATABASE,
	LOGTYPE_ERROR,
	LOGTYPE_DEBUG,
	LOGTYPE_COMM,
	LOGTYPE_SERVER
};
enum PACKETTYPE {
	PACKETTYPE_AUTH,
	PACKETTYPE_CLIENTAUTH,
	PACKETTYPE_CLIENTREQ,
	PACKETTYPE_TBC,
	PACKETTYPE_CLIENTOPS,
	PACKETTYPE_SERVEROPS
};
extern const wchar_t* LOGFILES[];
extern const char* SQLITE_STATEMENT_ACCOUNT_DATA;
extern const char* SQLITE_STATEMENT_RIVAL_DATA;
extern const char* SQLITE_STATEMENT_GARAGE_DATA;
extern const char* SQLITE_STATEMENT_TEAMGARAGE_DATA;
extern const char* SQLITE_STATEMENT_ITEM_DATA;
extern const char* SQLITE_STATEMENT_SIGN_DATA;
extern const char* SQLITE_STATEMENT_TEAM_DATA;
extern const char* SQLITE_STATEMENT_SERVER_KEYS;
extern const char* MYSQL_STATEMENT_ACCOUNT_DATA;
extern const char* MYSQL_STATEMENT_RIVAL_DATA;
extern const char* MYSQL_STATEMENT_GARAGE_DATA;
extern const char* MYSQL_STATEMENT_TEAMGARAGE_DATA;
extern const char* MYSQL_STATEMENT_ITEM_DATA;
extern const char* MYSQL_STATEMENT_SIGN_DATA;
extern const char* MYSQL_STATEMENT_TEAM_DATA;
extern const char* MYSQL_STATEMENT_SERVER_KEYS;
extern const char* HASH_SECRET;
