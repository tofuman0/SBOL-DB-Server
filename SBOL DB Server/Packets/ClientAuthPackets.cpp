#include <Windows.h>
#include "..\server.h"

HINSTANCE csp_crypt = LoadLibrary(TEXT("csp_crypt.dll")); // for AuthDataDecrypt and AuthDataDestroy
typedef long(__stdcall *CSP_AuthDataDestroy)(char * arg1);
typedef char*(__stdcall *CSP_AuthDataDecrypt)(char * arg1, char * arg2);
CSP_AuthDataDestroy AuthDataDestroy = (CSP_AuthDataDestroy)GetProcAddress(csp_crypt, "AuthDataDestroy");
CSP_AuthDataDecrypt AuthDataDecrypt = (CSP_AuthDataDecrypt)GetProcAddress(csp_crypt, "AuthDataDecrypt");

void ClientAuth(CLIENT* client)
{
	Server* server = (Server*)client->server;
	switch (client->inbuf.getSubType())
	{
	case 0x0000: // Authentication
	{
		client->inbuf.setOffset(0x06);
		int32_t clientSocket = client->inbuf.get<int32_t>();
		std::string username = client->inbuf.getStringA(0x14);
		std::transform(username.begin(), username.end(), username.begin(), tolower);
		std::string protectedpassword = client->inbuf.getStringA(client->inbuf.getSize() - 30);
		char *password = AuthDataDecrypt("authgenki", (char*)protectedpassword.data());
		int32_t res = server->db->Open();
		if (res == DB_OK)
		{
			std::string usernameStr = server->HexString((uint8_t*)username.data(), username.size());
			std::stringstream ss;

			std::string finalPassword = HASH_SECRET;
			finalPassword += '.';
			finalPassword += password;
			uint8_t cryptoPW[SHA256_DIGEST_LENGTH] = { 0 };
			char cryptoPWHEX[(SHA256_DIGEST_LENGTH * 2) + 1] = { 0 };

			server->GetHash((uint8_t*)finalPassword.c_str(), &cryptoPW[0], finalPassword.length());
			server->HexString(&cryptoPW[0], &cryptoPWHEX[0], SHA256_DIGEST_LENGTH);

			ss << "SELECT * FROM account_data WHERE HEX(LOWER(username)) = '" << usernameStr << "' AND password = '" << cryptoPWHEX << "'";
			res = server->db->Exec(ss.str().c_str());
			if (server->db->results.size() == 1)
			{
				std::string licenseNo = server->db->GetValue(&server->db->results, 0, "license");
				std::string handle = server->db->GetValue(&server->db->results, 0, "handle");
				std::string cp = server->db->GetValue(&server->db->results, 0, "cp");
				std::string level = server->db->GetValue(&server->db->results, 0, "level");
				std::string points = server->db->GetValue(&server->db->results, 0, "points");
				std::string playerwin = server->db->GetValue(&server->db->results, 0, "playerwin");
				std::string playerlose = server->db->GetValue(&server->db->results, 0, "playerlose");
				std::string rivalwin = server->db->GetValue(&server->db->results, 0, "rivalwin");
				std::string rivallose = server->db->GetValue(&server->db->results, 0, "rivallose");
				std::string activecar = server->db->GetValue(&server->db->results, 0, "activecar");
				std::string privileges = server->db->GetValue(&server->db->results, 0, "privileges");
				std::string teamid = server->db->GetValue(&server->db->results, 0, "teamid");
				std::string teamarea = server->db->GetValue(&server->db->results, 0, "teamarea");
				std::string state = server->db->GetValue(&server->db->results, 0, "state");
				std::string notbeginner = server->db->GetValue(&server->db->results, 0, "beginner");
				std::string garagecount = server->db->GetValue(&server->db->results, 0, "garagecount");

				ss.clear();
				ss.str(std::string());
				ss << "SELECT rivalstatus FROM rival_data WHERE license = " << licenseNo;
				db_blob rivalStatus = server->db->GetBlob(ss.str().c_str());

				if (handle == "NULL") handle = "";
				if (activecar == "NULL" || activecar == "") activecar = "-1";
				if (teamid == "NULL" || teamid == "") teamid = "-1";
				if (teamarea == "NULL" || teamid == "") teamarea = "0";
				if (licenseNo == "") licenseNo = "-1";
				if (cp == "") cp = "0";
				if (level == "") level = "0";
				if (points == "") points = "0";
				if (playerwin == "") playerwin = "0";
				if (playerlose == "") playerlose = "0";
				if (rivalwin == "") rivalwin = "0";
				if (rivallose == "") rivallose = "0";
				if (privileges == "") privileges = "0";
				if (state == "") state = "0";
				if (notbeginner == "") notbeginner = "0";
				if (garagecount == "NULL") garagecount = "0";
				if (garagecount == "") garagecount = "0";

				// TODO: Sent packet to servers to check if a player is logged in instead of this...
				// Check if player already logged in
				if (stoul(state) & 0x80000000)
				{
					client->outbuf.clearBuffer();
					client->outbuf.setSize(0x06);
					client->outbuf.setOffset(0x06);
					client->outbuf.setType(0x0001);
					client->outbuf.setSubType(0x0000);
					client->outbuf.append<int32_t>(clientSocket);
					client->outbuf.append<uint32_t>(stoul(licenseNo));
					client->outbuf.append<uint8_t>(1); // Logged in
				}

				else
				{
					//
					// General player details
					//
					client->outbuf.clearBuffer();
					client->outbuf.setSize(0x06);
					client->outbuf.setOffset(0x06);
					client->outbuf.setType(0x0001);
					client->outbuf.setSubType(0x0000);
					client->outbuf.append<int32_t>(clientSocket);
					client->outbuf.append<uint32_t>(stoul(licenseNo));
					client->outbuf.append<uint8_t>(0);
					client->outbuf.setArray((uint8_t*)handle.data(), handle.size(), client->outbuf.getOffset());
					client->outbuf.setOffset(client->outbuf.getOffset() + 16);
					client->outbuf.setSize(client->outbuf.getSize() + 16);
					client->outbuf.append<int64_t>(stoull(cp));
					client->outbuf.append<uint8_t>(static_cast<uint8_t>(stoul(level)));
					client->outbuf.append<uint32_t>(stoul(points));
					client->outbuf.append<uint32_t>(stoul(playerwin));
					client->outbuf.append<uint32_t>(stoul(playerlose));
					client->outbuf.append<uint32_t>(stoul(rivalwin));
					client->outbuf.append<uint32_t>(stoul(rivallose));

					//
					// Rival status
					//
					if (rivalStatus.size != 0x4B0)
					{
						client->outbuf.addSize(0x4B0);
						client->outbuf.addOffset(0x4B0);
					}
					else
						client->outbuf.appendArray((uint8_t*)rivalStatus.data.data(), rivalStatus.size);

					client->outbuf.append<uint8_t>(static_cast<uint8_t>(stoul(activecar)));
					client->outbuf.append<uint8_t>(static_cast<uint8_t>(stoul(privileges)));
					client->outbuf.append<uint8_t>(static_cast<uint8_t>(stoul(notbeginner)));
					client->outbuf.append<uint32_t>(stoul(teamid));
					client->outbuf.append<uint32_t>(stoul(state));

					//
					// Ranking
					//
					ss.clear();
					ss.str(std::string());
					ss << "SELECT license, playerwin - playerlose AS Score FROM account_data ORDER BY Score DESC";
					res = server->db->Exec(ss.str().c_str());
					if (res)
					{
						uint32_t ranking = 1;
						for (uint32_t i = 0; i < server->db->results.size(); i++)
						{
							if (licenseNo == server->db->GetValue(&server->db->results, i, "license"))
							{
								ranking = i + 1;
								break;
							}
						}
						client->outbuf.append<uint32_t>(ranking);
						ss.clear();
						ss.str(std::string());
						ss << "UPDATE account_data SET rank = " << ranking << " WHERE license = " << licenseNo;
						server->db->Exec(ss.str().c_str());
					}
					else
					{
						server->logger->Log(LOGTYPE_ERROR, L"Unable to retrieve ranking data with error: %u", res);
						client->outbuf.append<uint32_t>(1);
					}

					uint32_t _garagecount = stoul(garagecount);

					client->outbuf.append<uint8_t>(static_cast<uint8_t>(_garagecount));
					if (_garagecount > 0)
					{
						ss.clear();
						ss.str(std::string());
						ss << "SELECT garagedata FROM account_data WHERE license = " << licenseNo;
						db_blob garagedata = server->db->GetBlob(ss.str().c_str());
						if (garagedata.size == 0) goto failed;
						for (uint32_t i = 0; i < _garagecount; i++)
						{
							if (i >= garagedata.size) client->outbuf.append<uint8_t>(0);
							else client->outbuf.append<uint8_t>(garagedata.data[i]);
						}
					}

					if (handle != "")
					{
						//
						// Garage data
						//
						ss.clear();
						ss.str(std::string());
						ss << "SELECT * FROM garage_data WHERE license = " << licenseNo << " AND impounded = 0";
						uint32_t carCount = server->db->Exec(ss.str().c_str());
						client->outbuf.append<uint8_t>(static_cast<uint8_t>(carCount)); // Cars in Garage
						if (carCount > 0)
						{
							std::string bay;
							std::string carID;
							std::string KMs;
							std::string condition;

							for (uint32_t i = 0; i < carCount; i++)
							{
								ss.clear();
								ss.str(std::string());
								ss << "SELECT * FROM garage_data WHERE license = " << licenseNo << " AND impounded = 0";
								server->db->Exec(ss.str().c_str());
								bay = server->db->GetValue(&server->db->results, i, "bay");
								carID = server->db->GetValue(&server->db->results, i, "carID");
								KMs = server->db->GetValue(&server->db->results, i, "KMs");
								condition = server->db->GetValue(&server->db->results, i, "condition");

								ss.clear();
								ss.str(std::string());
								ss << "SELECT carData FROM garage_data WHERE license = " << licenseNo << " AND bay = " << bay << " AND impounded = 0";
								db_blob carData = server->db->GetBlob(ss.str().c_str());

								if (carData.size == 0) goto failed;
								uint8_t* _carData = (uint8_t*)calloc(1, 96 + 91);
								if (_carData == nullptr) goto failed;
								CopyMemory(_carData, carData.data.data(), min(carData.size, 96 + 91));

								client->outbuf.append<uint8_t>(static_cast<uint8_t>(stoul(bay)));
								client->outbuf.append<uint32_t>(stoul(carID));
								client->outbuf.append<float>(stof(KMs));
								client->outbuf.appendArray(_carData, carData.size);
								client->outbuf.append<uint32_t>(stoul(condition));

								free(_carData);
							}
						}
						else
						{
							client->outbuf.append<uint8_t>(0); // 0 Cars in Garage
						}

						//
						// Item Data
						//
						ss.clear();
						ss.str(std::string());
						ss << "SELECT itemcount FROM item_data WHERE license = " << licenseNo << " AND blocked = 0";
						res = server->db->Exec(ss.str().c_str());
						if (res)
						{
							std::string itemcount = server->db->GetValue(&server->db->results, 0, "itemcount");
							uint32_t itemCountVal = stoul(itemcount);
							ss.clear();
							ss.str(std::string());
							ss << "SELECT itemData FROM item_data WHERE license = " << licenseNo << " AND blocked = 0";
							db_blob itemdata = server->db->GetBlob(ss.str().c_str());

							client->outbuf.append<uint32_t>(itemCountVal);

							if (itemdata.size < (itemCountVal * 2))
							{
								client->outbuf.addSize(itemCountVal * 2);
								client->outbuf.addOffset((itemCountVal * 2));
							}
							else
							{
								client->outbuf.appendArray((uint8_t*)itemdata.data.data(), (itemCountVal * 2));
							}
						}
						else
						{
							client->outbuf.append<uint32_t>(0); // 0 Items
						}

						//
						// Sign Data
						//
						ss.clear();
						ss.str(std::string());
						ss << "SELECT * FROM sign_data WHERE license = " << licenseNo;
						res = server->db->Exec(ss.str().c_str());
						if (res)
						{
							ss.clear();
							ss.str(std::string());
							ss << "SELECT signData FROM sign_data WHERE license = " << licenseNo;
							db_blob signdata = server->db->GetBlob(ss.str().c_str());

							if (signdata.size < 54)
							{
								client->outbuf.addSize(54);
								client->outbuf.addOffset(54);
							}
							else
							{
								client->outbuf.appendArray((uint8_t*)signdata.data.data(), 54);
							}
						}
						else
						{
							// No sign data
							client->outbuf.addSize(54);
							client->outbuf.addOffset(54);
						}

						//
						// Team data
						//
						if (teamid != "-1")
						{
							uint32_t teamID = stoul(teamid);
							ss.clear();
							ss.str(std::string());
							ss << "SELECT * FROM team_data WHERE teamid = " << teamID;
							res = server->db->Exec(ss.str().c_str());
							if (res)
							{ 
								std::string leader = server->db->GetValue(&server->db->results, 0, "leader");
								std::string teamname = server->db->GetValue(&server->db->results, 0, "teamname");
								std::string comment = server->db->GetValue(&server->db->results, 0, "teamcomment");
								std::string survivalwin = server->db->GetValue(&server->db->results, 0, "survivalwin");
								std::string survivallose = server->db->GetValue(&server->db->results, 0, "survivallose");
								std::string flags = server->db->GetValue(&server->db->results, 0, "flags");
								std::string leaderName = "UNKNOWN";
								uint32_t teamCount = 0;
								
								ss.clear();
								ss.str(std::string());
								ss << "SELECT handle FROM account_data WHERE license = " << leader;
								
								res = server->db->Exec(ss.str().c_str());
								if (res)
								{
									leaderName = server->db->GetValue(&server->db->results, 0, "handle");
								}

								ss.clear();
								ss.str(std::string());
								ss << "SELECT * FROM account_data WHERE teamid = " << teamID;

								teamCount = server->db->Exec(ss.str().c_str());

								client->outbuf.append<uint8_t>(1); // Team found
								client->outbuf.append<uint32_t>(teamID);
								client->outbuf.append<uint32_t>(stoul(leader));
								client->outbuf.appendString(leaderName, 0x10);
								client->outbuf.appendString(teamname, 0x10);
								client->outbuf.appendString(comment, 0x28);
								client->outbuf.append<uint32_t>(stoul(survivalwin));
								client->outbuf.append<uint32_t>(stoul(survivallose));
								client->outbuf.append<uint32_t>(teamCount);
								client->outbuf.append<uint32_t>(stoul(flags));
								for (uint32_t i = 0; i < teamCount; i++)
								{
									std::string memberLicense = server->db->GetValue(&server->db->results, i, "license");
									if (memberLicense == "" || memberLicense == "NULL")
										memberLicense = "0";
									
									std::string memberName = server->db->GetValue(&server->db->results, i, "handle");
									
									std::string memberRank = server->db->GetValue(&server->db->results, i, "rank");
									if (memberRank == "" || memberRank == "NULL")
										memberRank = "0";
									
									uint8_t memberLeader = 0;
									if (leader == memberLicense)
										memberLeader = 1;

									std::string memberArea = server->db->GetValue(&server->db->results, i, "teamarea");
									if (memberArea == "" || memberArea == "NULL")
										memberArea = "0";
									
									client->outbuf.append<uint32_t>(stoul(memberLicense));
									client->outbuf.appendString(memberName, 0x10);
									client->outbuf.append<uint32_t>(stoul(memberRank));
									client->outbuf.append<uint8_t>(memberLeader);
									client->outbuf.append<uint8_t>(static_cast<uint8_t>(stoul(memberArea)));
								}

								ss.clear();
								ss.str(std::string());
								ss << "UPDATE team_data SET teamcount = " << teamCount << " WHERE teamid = " << teamID;
							}
							else
							{
								server->logger->Log(LOGTYPE_ERROR, L"Unable to retrieve team_data for %s", licenseNo);
								client->outbuf.append<uint8_t>(0); // Unable to find team
							}
						}
						else
							client->outbuf.append<uint8_t>(0); // Not in team
						client->outbuf.append<uint8_t>(static_cast<uint8_t>(stoul(teamarea)));
					}

					server->logger->Log(LOGTYPE_COMM, L"Client %s authenticated", server->logger->ToWide(&username[0]).c_str());
				}

			}
			else
			{
			failed:
				client->outbuf.clearBuffer();
				client->outbuf.setSize(0x06);
				client->outbuf.setOffset(0x06);
				client->outbuf.setType(0x0001);
				client->outbuf.setSubType(0x0000);
				client->outbuf.append<int32_t>(clientSocket);
				client->outbuf.append<uint32_t>(0xFFFFFFFF); // Login failed
			}
			server->db->Close();
		}

		AuthDataDestroy(password);
	}
	break;
	case 0x0001: // Handle check
	{
		int32_t clientSocket = client->inbuf.get<int32_t>(0x06);
		std::string handle = client->inbuf.getString(0x0A, 0x10);
		std::transform(handle.begin(), handle.end(), handle.begin(), tolower);
		handle = server->HexString((uint8_t*)handle.data(), handle.size());

		std::stringstream ss;

		client->outbuf.clearBuffer();
		client->outbuf.setSize(0x06);
		client->outbuf.setOffset(0x06);
		client->outbuf.setType(0x0001);
		client->outbuf.setSubType(0x0001);
		client->outbuf.append<int32_t>(clientSocket);

		int32_t res = server->db->Open();
		if (res == DB_OK)
		{
			ss << "SELECT handle FROM account_data WHERE hex(lower(handle)) = '" << handle << "'";
			res = server->db->Exec(ss.str().c_str());
			if (res)
			{
				client->outbuf.append<uint8_t>(1);
			}
			else
			{
				client->outbuf.append<uint8_t>(0);
			}
			server->db->Close();
		}
	}
	break;
	}
	client->Send();
}
