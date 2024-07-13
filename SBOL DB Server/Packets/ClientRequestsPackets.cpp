#include <Windows.h>
#include "..\server.h"

typedef struct st_car {
	uint32_t carID;
	float KMs;
	uint32_t engineCondition;
	uint32_t bay;
	uint8_t cardata[96 + 91]; // mods and owned parts
} CAR;

typedef struct st_rivaldata {
	uint8_t rivalMember[8];
	uint32_t wins;
} RIVAL_STATUS;

void ClientRequests(CLIENT* client)
{
	Server* server = (Server*)client->server;
	client->inbuf.setOffset(0x06);

	switch (client->inbuf.getSubType())
	{
	case 0x0000: // Save Client Data
	{
		std::stringstream ss;
		std::vector<CAR> cars;
		std::vector<uint16_t> items;
		std::vector<uint8_t> garages;
		RIVAL_STATUS rivalStatus[100];
		uint32_t license = client->inbuf.get<uint32_t>();
		std::string handle = client->inbuf.getStringA(0x10);
		uint64_t CP = client->inbuf.get<int64_t>();
		uint32_t level = static_cast<uint32_t>(client->inbuf.get<uint8_t>());
		uint32_t points = client->inbuf.get<uint32_t>();
		uint32_t playerWin = client->inbuf.get<uint32_t>();
		uint32_t playerLose = client->inbuf.get<uint32_t>();
		uint32_t rivalWin = client->inbuf.get<uint32_t>();
		uint32_t rivalLose = client->inbuf.get<uint32_t>();
		client->inbuf.getArray((uint8_t*)&rivalStatus[0], sizeof(rivalStatus));
		uint32_t activeCar = static_cast<uint32_t>(client->inbuf.get<uint8_t>());
		uint32_t privileges = static_cast<uint32_t>(client->inbuf.get<uint8_t>());
		uint32_t notBeginner = static_cast<uint32_t>(client->inbuf.get<uint8_t>());
		int32_t teamId = client->inbuf.get<int32_t>();

		uint32_t garageCount = static_cast<uint32_t>(client->inbuf.get<uint8_t>());
		for (uint32_t i = 0; i < garageCount; i++) garages.push_back(client->inbuf.get<uint8_t>());

		uint32_t carsInGarage = static_cast<uint32_t>(client->inbuf.get<uint8_t>());
		for (uint32_t i = 0; i < carsInGarage; i++)
		{
			CAR currentCar = { 0 };
			currentCar.carID = client->inbuf.get<uint32_t>();
			currentCar.KMs = client->inbuf.get<float>();
			client->inbuf.getArray(&currentCar.cardata[0], sizeof(currentCar.cardata));
			currentCar.engineCondition = client->inbuf.get<uint32_t>();
			currentCar.bay = client->inbuf.get<uint32_t>();
			cars.push_back(currentCar);
		}

		uint32_t itemCount = client->inbuf.get<uint32_t>();
		for (uint32_t i = 0; i < itemCount; i++) items.push_back(client->inbuf.get<uint16_t>());

		uint8_t sign[54];
		client->inbuf.getArray(&sign[0], 54);

		int32_t res = server->db->Open();
		std::string teamIDStr;
		if (teamId < 0)
			teamIDStr = "NULL";
		else
			teamIDStr = std::to_string(teamId);
		if (res == DB_OK)
		{
			ss << "UPDATE account_data SET " \
				"handle = '" << handle <<
				"', cp = " << CP <<
				", level = " << level <<
				", points = " << points <<
				", playerwin = " << playerWin <<
				", playerlose = " << playerLose <<
				", rivalwin = " << rivalWin <<
				", rivallose = " << rivalLose <<
				", activecar = " << activeCar <<
				", privileges = " << privileges <<
				", teamid = " << teamIDStr <<
				", beginner = " << notBeginner <<
				", garagecount = " << garageCount <<
				" WHERE license = " << license << "";
			server->db->Exec(ss.str().c_str());
			
			std::string rivalStatusStr = server->HexString((uint8_t*)&rivalStatus[0], sizeof(rivalStatus));
			ss.clear();
			ss.str(std::string());

			ss << "SELECT * FROM rival_data WHERE license = " << license;
			res = server->db->Exec(ss.str().c_str());
			if (res)
			{
				ss.clear();
				ss.str(std::string());
				ss << "UPDATE rival_data SET " <<
					"rivalstatus = X'" << rivalStatusStr <<
					"' WHERE license = " << license;

				server->db->Exec(ss.str().c_str());
			}
			else
			{
				ss.clear();
				ss.str(std::string());
				ss << "INSERT INTO rival_data(license, rivalstatus) VALUES(" <<
					license << ", " <<
					"X'" << rivalStatusStr << "')";

				server->db->Exec(ss.str().c_str());
			}

			if (garageCount > 0)
			{
				std::string garageDataStr = server->HexString((uint8_t*)garages.data(), garages.size());
				ss.clear();
				ss.str(std::string());
				ss << "UPDATE account_data SET " <<
					"garagedata = X'" << garageDataStr << "' WHERE license = " << license;

				server->db->Exec(ss.str().c_str());
			}

			for (uint32_t i = 0; i < carsInGarage; i++)
			{
				std::string carDataStr = server->HexString((uint8_t*)&cars.at(i).cardata[0], sizeof(cars.at(i).cardata));
				ss.clear();
				ss.str(std::string());

				ss << "SELECT * FROM garage_data WHERE license = " << license << " AND bay = " << cars.at(i).bay << " AND impounded = 0";
				res = server->db->Exec(ss.str().c_str());
				if (res)
				{
					ss.clear();
					ss.str(std::string());
					ss << "UPDATE garage_data SET " <<
						"carID = " << cars.at(i).carID <<
						", KMs = " << cars.at(i).KMs <<
						", condition = " << cars.at(i).engineCondition <<
						", carData = X'" << carDataStr <<
						"' WHERE license = " << license <<
						" AND bay = " << cars.at(i).bay <<
						" AND impounded <> 1";

					server->db->Exec(ss.str().c_str());
				}
				else
				{
					ss.clear();
					ss.str(std::string());
					ss << "INSERT INTO garage_data(license, bay, carID, KMs, condition, carData) VALUES(" <<
						license << ", " <<
						cars.at(i).bay << ", " <<
						cars.at(i).carID << ", " <<
						cars.at(i).KMs << ", " <<
						cars.at(i).engineCondition << ", X'" <<
						carDataStr << "')";

					server->db->Exec(ss.str().c_str());
				}
			}

			ss.clear();
			ss.str(std::string());
			std::string itemDataStr = "0000";
			if (items.size() > 0) itemDataStr = server->HexString((uint8_t*)items.data(), items.size() * 2);

			ss << "SELECT * FROM item_data WHERE license = " << license << " AND blocked = 0";
			res = server->db->Exec(ss.str().c_str());
			if (res)
			{
				ss.clear();
				ss.str(std::string());
				ss << "UPDATE item_data SET " <<
					"itemcount = " << itemCount <<
					", itemdata = X'" << itemDataStr << "' WHERE license = " << license << " AND blocked = 0";

				server->db->Exec(ss.str().c_str());
			}
			else
			{
				ss.clear();
				ss.str(std::string());
				ss << "INSERT INTO item_data(license, itemcount, itemdata) VALUES(" <<
					license << ", " <<
					itemCount << ", X'" <<
					itemDataStr <<
					"')";

				server->db->Exec(ss.str().c_str());
			}

			ss.clear();
			ss.str(std::string());
			std::string signDataStr = server->HexString(&sign[0], 54);

			ss << "SELECT * FROM sign_data WHERE license = " << license;
			res = server->db->Exec(ss.str().c_str());
			if (res)
			{
				ss.clear();
				ss.str(std::string());
				ss << "UPDATE sign_data SET " <<
					"signdata = X'" << signDataStr << "' WHERE license = " << license;

				server->db->Exec(ss.str().c_str());
			}
			else
			{
				ss.clear();
				ss.str(std::string());
				ss << "INSERT INTO sign_data(license, signData) VALUES(" <<
					license << ", X'" <<
					signDataStr <<
					"')";

				server->db->Exec(ss.str().c_str());
			}
		}
		server->db->Close();
	}
	return;
	case 0x0001: // Swap Car in Garage
	{
		uint32_t license = client->inbuf.get<uint32_t>();
		uint8_t isSwapping = client->inbuf.get<uint8_t>();
		uint32_t fromBay = client->inbuf.get<uint32_t>();
		uint32_t toBay = client->inbuf.get<uint32_t>();

		int32_t res = server->db->Open();
		if (res == DB_OK)
		{
			std::stringstream ss;
			if (isSwapping)
			{
				ss << "UPDATE garage_data SET bay = CASE bay WHEN " << fromBay << " THEN " << toBay << " WHEN " << toBay << " THEN " << fromBay << " END " <<
					"WHERE (bay = " << fromBay << " AND license = " << license << ") OR (bay = " << toBay << " AND license = " << license << ")";
				server->db->Exec(ss.str().c_str());
			}
			else
			{
				ss << "UPDATE garage_data SET bay = " << toBay << " WHERE bay = " << fromBay << " AND license = " << license;
				server->db->Exec(ss.str().c_str());
			}
			server->db->Close();
		}
	}
	return;
	case 0x0002: // Remove Car from Garage
	{
		uint32_t license = client->inbuf.get<uint32_t>();
		uint32_t bay = client->inbuf.get<uint8_t>();

		int32_t res = server->db->Open();
		if (res == DB_OK)
		{
			std::stringstream ss;
			ss << "DELETE FROM garage_data WHERE bay = " << bay << " AND license = " << license;
			server->db->Exec(ss.str().c_str());
			server->db->Close();
		}
	}
	return;
	case 0x0003: // Check Create Team Name
	{
		uint32_t license = client->inbuf.get<uint32_t>();
		std::string teamName = client->inbuf.getStringA(0x10);
		std::string teamNameLower = teamName;
		std::transform(teamNameLower.begin(), teamNameLower.end(), teamNameLower.begin(), tolower);
		std::string teamNameHex = server->HexString((uint8_t*)teamNameLower.c_str(), teamNameLower.size());
		std::string teamid = "0";
		uint8_t result = 1;

		int32_t res = server->db->Open();
		if (res == DB_OK)
		{
			std::stringstream ss;
			ss << "SELECT teamid FROM account_data WHERE license = " << license << " AND (teamid IS NULL OR teamid == '')";
			res = server->db->Exec(ss.str().c_str());
			if (res)
			{	
				ss.clear();
				ss.str(std::string());
				ss << "SELECT * FROM team_data WHERE HEX(LOWER(teamname)) = '" << teamNameHex << "'";
				res = server->db->Exec(ss.str().c_str());
				if (!res)
				{
					teamNameHex = server->HexString((uint8_t*)teamName.c_str(), teamName.size());
					ss.clear();
					ss.str(std::string());
					ss << "INSERT INTO team_data(leader, teamname) VALUES(" <<
						license << ", X'" <<
						teamNameHex <<
						"')";
					server->db->Exec(ss.str().c_str());

					ss.clear();
					ss.str(std::string());
					ss << "SELECT teamid FROM team_data WHERE teamname = X'" << teamNameHex << "'";
					res = server->db->Exec(ss.str().c_str());

					if (res)
					{
						std::string teamid = server->db->GetValue(&server->db->results, 0, "teamid");
						ss.clear();
						ss.str(std::string());
						ss << "UPDATE account_data SET " \
							"teamid = " << teamid <<
							" WHERE license = " << license;
						server->db->Exec(ss.str().c_str());
						result = 0;
					}
				}
			}
			else
			{
				// Already in team set result to 2 incase I ever want to handle this
				result = 2;
			}
			server->db->Close();
		}

		client->outbuf.clearBuffer();
		client->outbuf.setSize(0x06);
		client->outbuf.setOffset(0x06);
		client->outbuf.setType(0x0002);
		client->outbuf.setSubType(0x0000);
		client->outbuf.append<uint32_t>(license);
		client->outbuf.append<uint32_t>(stoul(teamid));
		client->outbuf.append<uint8_t>(result);
		client->outbuf.appendString(teamName, 0x10);
	}
	break;
	case 0x0004: // Update Team Invite Type
	{
		uint32_t license = client->inbuf.get<uint32_t>();
		uint8_t inviteStatus = client->inbuf.get<uint8_t>() & 0x01;
		std::string teamid = "0";
		uint8_t result = 1;
		int32_t res = server->db->Open();
		if (res == DB_OK)
		{
			std::stringstream ss;
			ss << "SELECT teamid FROM account_data WHERE license = " << license << " AND teamid IS NOT NULL AND teamid <> ''";
			res = server->db->Exec(ss.str().c_str());
			if (res)
			{	
				teamid = server->db->GetValue(&server->db->results, 0, "teamid");
				ss.clear();
				ss.str(std::string());
				ss << "SELECT * FROM team_data WHERE teamid = " << teamid << " AND leader = " << license;
				res = server->db->Exec(ss.str().c_str());
				if (res)
				{
					uint32_t flag = stoul(server->db->GetValue(&server->db->results, 0, "flags"));
					flag &= ~0x01;
					flag |= inviteStatus;
					ss.clear();
					ss.str(std::string());
					ss << "UPDATE team_data SET " \
						"flags = " << flag <<
						" WHERE leader = " << license;
					server->db->Exec(ss.str().c_str());
					result = 0;
				}
				else
				{
					// Client not team leader so maybe hacking. Set result to 3 incase I ever want to handle this
					result = 3;
				}
			}
			else
			{
				// Client not in team set result to 2 incase I ever want to handle this
				result = 2;
			}
			server->db->Close();
		}
		client->outbuf.clearBuffer();
		client->outbuf.setSize(0x06);
		client->outbuf.setOffset(0x06);
		client->outbuf.setType(0x0002);
		client->outbuf.setSubType(0x0001);
		client->outbuf.append<uint32_t>(license);
		client->outbuf.append<uint32_t>(stoul(teamid));
		client->outbuf.append<uint8_t>(result);
		client->outbuf.append<uint8_t>(inviteStatus);
	}
	break;
	case 0x0005: // Update Team Comment
	{
		uint32_t license = client->inbuf.get<uint32_t>();
		std::string teamComment = client->inbuf.getStringA(0x28);
		std::string teamCommentHex = server->HexString((uint8_t*)teamComment.c_str(), teamComment.size());
		std::string teamid = "0";
		uint8_t result = 1;
		int32_t res = server->db->Open();
		if (res == DB_OK)
		{
			std::stringstream ss;
			ss << "SELECT teamid FROM account_data WHERE license = " << license << " AND teamid IS NOT NULL AND teamid <> ''";
			res = server->db->Exec(ss.str().c_str());
			if (res)
			{
				teamid = server->db->GetValue(&server->db->results, 0, "teamid");
				ss.clear();
				ss.str(std::string());
				ss << "SELECT * FROM team_data WHERE teamid = " << teamid << " AND leader = " << license;
				res = server->db->Exec(ss.str().c_str());
				if (res)
				{
					ss.clear();
					ss.str(std::string());
					ss << "UPDATE team_data SET " \
						"teamcomment = X'" << teamCommentHex <<
						"' WHERE leader = " << license;
					server->db->Exec(ss.str().c_str());
					result = 0;
				}
				else
				{
					// Client not team leader so maybe hacking. Set result to 3 incase I ever want to handle this
					result = 3;
				}
			}
			else
			{
				// Client not in team set result to 2 incase I ever want to handle this
				result = 2;
			}

			server->db->Close();
		}
		client->outbuf.clearBuffer();
		client->outbuf.setSize(0x06);
		client->outbuf.setOffset(0x06);
		client->outbuf.setType(0x0002);
		client->outbuf.setSubType(0x0002);
		client->outbuf.append<uint32_t>(license);
		client->outbuf.append<uint32_t>(stoul(teamid));
		client->outbuf.append<uint8_t>(result);
		client->outbuf.appendString(teamComment, 0x28);
	}
	break;
	case 0x0006: // Delete Team
	{
		uint32_t license = client->inbuf.get<uint32_t>();
		std::string teamid = "0";
		uint8_t result = 1;
		int32_t res = server->db->Open();
		if (res == DB_OK)
		{
			std::stringstream ss;
			ss << "SELECT teamid FROM account_data WHERE license = " << license << " AND teamid IS NOT NULL AND teamid <> ''";
			res = server->db->Exec(ss.str().c_str());
			if (res)
			{
				teamid = server->db->GetValue(&server->db->results, 0, "teamid");
				ss.clear();
				ss.str(std::string());
				ss << "SELECT * FROM team_data WHERE teamid = " << teamid << " AND leader = " << license;
				res = server->db->Exec(ss.str().c_str());
				if (res)
				{
					ss.clear();
					ss.str(std::string());
					ss << "DELETE FROM team_data WHERE teamid = " << teamid;
					server->db->Exec(ss.str().c_str());
					ss.clear();
					ss.str(std::string());
					ss << "UPDATE account_data SET " \
						"teamid = NULL AND teamarea = 0 WHERE teamid = " << teamid;
					server->db->Exec(ss.str().c_str());
					result = 0;
				}
				else
				{
					// Client not team leader so maybe hacking. Set result to 3 incase I ever want to handle this
					result = 3;
				}
			}
			else
			{
				// Client not in team set result to 2 incase I ever want to handle this
				result = 2;
			}
			server->db->Close();
		}
		client->outbuf.clearBuffer();
		client->outbuf.setSize(0x06);
		client->outbuf.setOffset(0x06);
		client->outbuf.setType(0x0002);
		client->outbuf.setSubType(0x0003);
		client->outbuf.append<uint32_t>(license);
		client->outbuf.append<uint32_t>(stoul(teamid));
		client->outbuf.append<uint8_t>(result);
	}
	break;
	case 0x0007: // Get Team Data
	{
		uint32_t license = client->inbuf.get<uint32_t>();
		int32_t res = server->db->Open();
		if (res == DB_OK)
		{
			std::stringstream ss;
			ss.clear();
			ss.str(std::string());
			ss << "SELECT * FROM account_data WHERE license = " << license;
			res = server->db->Exec(ss.str().c_str());
			if (res)
			{
				std::string teamid = server->db->GetValue(&server->db->results, 0, "teamid");
				std::string teamarea = server->db->GetValue(&server->db->results, 0, "teamarea");

				if (teamarea == "NULL" || teamarea == "")
					teamarea = "0";

				if (teamid == "NULL" || teamid == "")
				{
					client->outbuf.clearBuffer();
					client->outbuf.setSize(0x06);
					client->outbuf.setOffset(0x06);
					client->outbuf.setType(0x0002);
					client->outbuf.setSubType(0x0004);
					client->outbuf.append<uint32_t>(license);
					client->outbuf.append<uint32_t>(0xFFFFFFFF);
					client->outbuf.append<uint32_t>(0);
					client->outbuf.appendString(std::string(), 0x10);
					client->outbuf.appendString(std::string(), 0x10);
					client->outbuf.appendString(std::string(), 0x28);
					client->outbuf.append<uint32_t>(0);
					client->outbuf.append<uint32_t>(0);
					client->outbuf.append<uint32_t>(0);
					client->outbuf.append<uint32_t>(0);
					client->outbuf.append<uint8_t>(0);
					server->SendToAll(&client->outbuf);
					return;
				}
				else
				{
					ss.clear();
					ss.str(std::string());
					ss << "SELECT * FROM team_data WHERE teamid = " << teamid;
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
						ss << "SELECT * FROM account_data WHERE teamid = " << teamid;
						teamCount = server->db->Exec(ss.str().c_str());

						client->outbuf.clearBuffer();
						client->outbuf.setSize(0x06);
						client->outbuf.setOffset(0x06);
						client->outbuf.setType(0x0002);
						client->outbuf.setSubType(0x0004);
						client->outbuf.append<uint32_t>(license);
						client->outbuf.append<uint32_t>(stoul(teamid));
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
							client->outbuf.append<uint8_t>(static_cast<uint8_t>(std::stoul(memberArea)));
						}
						client->outbuf.append<uint8_t>(static_cast<uint8_t>(stoul(teamarea)));
						server->SendToAll(&client->outbuf);
						return;
					}
					else
					{
						server->db->Close();
						return;
					}
				}
			}
			else
			{
				server->db->Close();
				return;
			}
			server->db->Close();
		}
	}
	break;
	case 0x0008: // Adjust Team Area
	{
		uint32_t license = client->inbuf.get<uint32_t>();
		uint32_t memberID = client->inbuf.get<uint32_t>();
		uint32_t access = client->inbuf.get<uint8_t>();
		int32_t res = server->db->Open();
		if (res == DB_OK)
		{
			std::stringstream ss;
			ss.clear();
			ss.str(std::string());
			ss << "SELECT * FROM account_data WHERE license = " << license << " AND teamid IS NOT NULL AND teamid <> ''";
			res = server->db->Exec(ss.str().c_str());
			if (res)
			{
				std::string teamid = server->db->GetValue(&server->db->results, 0, "teamid");
				ss.clear();
				ss.str(std::string());
				ss << "SELECT * FROM team_data WHERE teamid = " << teamid << " AND leader = " << license;
				res = server->db->Exec(ss.str().c_str());
				if (res)
				{
					ss.clear();
					ss.str(std::string());
					ss << "SELECT * FROM account_data WHERE license = " << memberID << " AND teamid = " << teamid;
					res = server->db->Exec(ss.str().c_str());
					if (res)
					{
						ss.clear();
						ss.str(std::string());
						ss << "UPDATE account_data SET " \
							"teamarea = " << access << " WHERE license = " << memberID;
						server->db->Exec(ss.str().c_str());	
					}
				}
			}
			server->db->Close();
		}
	}
	return;
	case 0x0009: // Remove Team Member
	{
		uint32_t license = client->inbuf.get<uint32_t>();
		uint32_t memberID = client->inbuf.get<uint32_t>();
		uint8_t result = 1;
		int32_t res = server->db->Open();
		if (res == DB_OK && license != memberID)
		{
			std::stringstream ss;
			ss.clear();
			ss.str(std::string());
			ss << "SELECT * FROM account_data WHERE license = " << license << " AND teamid IS NOT NULL AND teamid <> ''";
			res = server->db->Exec(ss.str().c_str());
			if (res)
			{
				std::string teamid = server->db->GetValue(&server->db->results, 0, "teamid");
				ss.clear();
				ss.str(std::string());
				ss << "SELECT * FROM team_data WHERE teamid = " << teamid << " AND leader = " << license;
				res = server->db->Exec(ss.str().c_str());
				if (res)
				{
					ss.clear();
					ss.str(std::string());
					ss << "SELECT * FROM account_data WHERE license = " << memberID << " AND teamid = " << teamid;
					res = server->db->Exec(ss.str().c_str());
					if (res)
					{
						ss.clear();
						ss.str(std::string());
						ss << "UPDATE account_data SET " \
							"teamid = NULL AND teamarea = 0 WHERE license = " << memberID;
						res = server->db->Exec(ss.str().c_str());

						ss.clear();
						ss.str(std::string());
						ss << "SELECT * FROM account_data WHERE teamid = " << teamid;
						uint32_t teamcount = server->db->Exec(ss.str().c_str());
						if (teamcount)
						{
							if (teamcount > 1) teamcount--;
							ss.clear();
							ss.str(std::string());
							ss << "UPDATE team_data SET " \
								"teamcount = " << teamcount << " WHERE teamid = " << teamid;
							res = server->db->Exec(ss.str().c_str());
						}
						result = 0;
					}
				}
			}
			server->db->Close();
		}
		client->outbuf.clearBuffer();
		client->outbuf.setSize(0x06);
		client->outbuf.setOffset(0x06);
		client->outbuf.setType(0x0002);
		client->outbuf.setSubType(0x0005);
		client->outbuf.append<uint32_t>(license);
		client->outbuf.append<uint8_t>(result);
	}
	break;
	}
	client->Send();
}
