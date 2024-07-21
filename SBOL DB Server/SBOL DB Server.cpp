#pragma region Includes
#include "SBOL DB Server.h"
#include "server.h"
#include "globals.h"
#include "resource.h"
#pragma endregion

int wmain(int argc, wchar_t *argv[])
{
	_set_printf_count_output(1); // Enable %n as random crashing occurred while packet logging
	srand(static_cast<uint32_t>(time(NULL))); // Seed RNG
	if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
	{
		if (_wcsicmp(L"setpassword", argv[1] + 1) == 0)
		{
			if (argc != 4)
			{
				std::wcout << L"setpassword Parameters:" << std::endl;
				std::wcout << L" username          username to update the password of." << std::endl;
				std::wcout << L" password          password to set." << std::endl;
			}
			else
			{
				Server* server = new Server();
				if (server->SetPassword(argv[2], argv[3]))
				{
					server->logger->Log(LOGTYPE_ERROR, L"Unable to set password for %s.", argv[2]);
					server->CloseDB();
					delete server;
					return 1;
				}
				else
				{
					server->logger->Log(LOGTYPE_DATABASE, L"Set password for account for %s.", argv[2]);
					server->CloseDB();
				}
				delete server;
			}
		}
		else if (_wcsicmp(L"createaccount", argv[1] + 1) == 0)
		{
			if (argc != 6)
			{
				std::wcout << L"createaccount Parameters:" << std::endl;
				std::wcout << L" username          username to create." << std::endl;
				std::wcout << L" password          password to set." << std::endl;
				std::wcout << L" email             email to set." << std::endl;
				std::wcout << L" privileges        account privileges to set (255 full admin)." << std::endl;
			}
			else
			{
				Server* server = new Server();
				if (server->CreateAccount(argv[2], argv[3], argv[4], argv[5]))
				{
					server->logger->Log(LOGTYPE_ERROR, L"Unable to create account for %s.", argv[2]);
					server->CloseDB();
					delete server;
					return 1;
				}
				else
				{
					server->logger->Log(LOGTYPE_DATABASE, L"Created account for %s.", argv[2]);
					server->CloseDB();
				}
				delete server;
			}
		}
		else if (_wcsicmp(L"createkey", argv[1] + 1) == 0)
		{
			Server* server = new Server();
			if (server->CreateKey())
			{
				server->logger->Log(LOGTYPE_ERROR, L"Unable to add key to database");
				delete server;
				return 1;
			}
			else
			{
				server->logger->Log(LOGTYPE_SERVER, L"Created new key and added to database");
			}
			delete server;
		}
		else if ((_wcsicmp(L"?", argv[1] + 1) == 0) || (_wcsicmp(L"help", argv[1] + 1) == 0))
		{
			DisplayHelp();
		}
		else
		{
			DisplayHelp();
		}
	}
	else
	{
		Server* server = new Server();
		server->logger->Log(LOGTYPE_NONE, L"Starting . . .");
		if (server->Start())
		{
			server->logger->Log(LOGTYPE_ERROR, L"Server failed to start.");
			server->CloseDB();
			delete server;
			return 1;
		}

		char titleBuf[0x100];
		snprintf(titleBuf, sizeof(titleBuf), "SBOL DB Server v%s. Clients: 0", server->logger->ToNarrow(VERSION_STRING).c_str());
		SetConsoleTitleA(titleBuf);

		HMODULE mainMod = GetModuleHandle(0);
		assert(mainMod);
		HICON mainIcon = ::LoadIcon(mainMod, MAKEINTRESOURCE(MAINICON));
		assert(mainIcon);
		ChangeIcon(mainIcon);

		while (server->IsRunning())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		server->CloseDB();
		server->logger->Log(LOGTYPE_SERVER, L"DB Server shutting down");
		delete server;
	}
    return 0;
}