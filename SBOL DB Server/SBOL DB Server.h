#pragma once
#include <windows.h>
#include <iostream>
#include <cassert>

#pragma region Variables
const wchar_t* LOGFILES[]{
	nullptr,
	L"packet",
	L"database",
	L"error",
	L"debug",
	L"comm",
	L"server"
};
const char* SQLITE_STATEMENT_ACCOUNT_DATA = "CREATE TABLE \"account_data\" (\n	`license`	INTEGER NOT NULL,\n	`username`	TEXT NOT NULL UNIQUE,\n	`password`	TEXT NOT NULL,\n	`email`	TEXT NOT NULL,\n	`joindate`	TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,\n	`handle`	TEXT,\n	`cp`	INTEGER DEFAULT 0,\n	`level`	INTEGER DEFAULT 1,\n	`points`	INTEGER NOT NULL DEFAULT 0,\n	`playerwin`	INTEGER NOT NULL DEFAULT 0,\n	`playerlose`	INTEGER NOT NULL DEFAULT 0,\n	`rivalwin`	INTEGER NOT NULL DEFAULT 0,\n	`rivallose`	INTEGER NOT NULL DEFAULT 0,\n	`activecar`	INTEGER,\n	`rank`	INTEGER DEFAULT 0,\n	`privileges`	INTEGER NOT NULL DEFAULT 0,\n	`teamid`	INTEGER,\n	`teamarea`	INTEGER DEFAULT 0,\n	`state`	INTEGER NOT NULL DEFAULT 0,\n	`beginner`	INTEGER DEFAULT 0,\n	`garagecount`	INTEGER DEFAULT 0,\n	`garagedata`	INTEGER,\n	PRIMARY KEY(`license`)\n)";
const char* SQLITE_STATEMENT_RIVAL_DATA = "CREATE TABLE \"rival_data\" (\n	`license`	INTEGER NOT NULL,\n	`rivalstatus`	BLOB\n)";
const char* SQLITE_STATEMENT_GARAGE_DATA = "CREATE TABLE \"garage_data\" (\n	`license`	INTEGER NOT NULL,\n	`bay`	INTEGER,\n	`carID`	INTEGER NOT NULL,\n	`KMs`	REAL NOT NULL DEFAULT 0.0,\n	`carData`	BLOB,\n	`condition`	INTEGER NOT NULL DEFAULT 0,\n	`impounded`	INTEGER NOT NULL DEFAULT 0\n)";
const char* SQLITE_STATEMENT_TEAMGARAGE_DATA = "CREATE TABLE \"teamgarage_data\" (\n	`teamid`	INTEGER NOT NULL,\n	`bay`	INTEGER,\n	`carID`	INTEGER NOT NULL,\n	`KMs`	REAL NOT NULL DEFAULT 0,\n	`carData`	BLOB,\n	`condition`	INTEGER NOT NULL DEFAULT 0,\n	`impounded`	INTEGER NOT NULL DEFAULT 0\n)";
const char* SQLITE_STATEMENT_ITEM_DATA = "CREATE TABLE \"item_data\" (\n	`license`	INTEGER NOT NULL,\n	`itemcount`	INTEGER NOT NULL DEFAULT 0,\n	`itemData`	BLOB,\n	`blocked`	INTEGER NOT NULL DEFAULT 0\n)";
const char* SQLITE_STATEMENT_SIGN_DATA = "CREATE TABLE \"sign_data\" (\n	`license`	INTEGER NOT NULL,\n	`signData`	BLOB\n)";
const char* SQLITE_STATEMENT_TEAM_DATA = "CREATE TABLE \"team_data\" (\n	`teamid`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,\n	`leader`	INTEGER NOT NULL,\n	`teamname`	TEXT,\n	`teamcomment`	TEXT,\n	`survivalwin`	INTEGER NOT NULL DEFAULT 0,\n	`survivallose`	INTEGER NOT NULL DEFAULT 0,\n	`teamcount`	INTEGER NOT NULL DEFAULT 1,\n	`flags`	INTEGER NOT NULL DEFAULT 0\n)";
const char* SQLITE_STATEMENT_SERVER_KEYS = "CREATE TABLE \"server_keys\" (\n	`key`	BLOB,\n	`iv`	BLOB\n)";

const char* MYSQL_STATEMENT_ACCOUNT_DATA = "CREATE TABLE `account_data` (\n  `license` int(11) NOT NULL AUTO_INCREMENT,\n  `username` text NOT NULL,\n  `password` text NOT NULL,\n  `email` text NOT NULL,\n  `joindate` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,\n  `handle` text,\n  `cp` bigint(20) DEFAULT '0',\n  `level` int(11) DEFAULT '1',\n  `points` int(11) NOT NULL DEFAULT '0',\n  `playerwin` int(11) NOT NULL DEFAULT '0',\n  `playerlose` int(11) NOT NULL DEFAULT '0',\n  `rivalwin` int(11) NOT NULL DEFAULT '0',\n  `rivallose` int(11) NOT NULL DEFAULT '0',\n  `activecar` int(11) DEFAULT NULL,\n  `rank` int(11) NOT NULL DEFAULT '0',\n  `privileges` int(11) NOT NULL DEFAULT '0',\n  `teamid` int(11) DEFAULT NULL,\n  `teamarea` int(11) NOT NULL DEFAULT '0',\n  `state` int(11) NOT NULL DEFAULT '0',\n  `beginner` int(11) DEFAULT '0',\n  `garagecount` int(11) DEFAULT '0',\n  `garagedata` blob,\n  PRIMARY KEY (`license`)\n) ENGINE=InnoDB DEFAULT CHARSET=latin1";
const char* MYSQL_STATEMENT_RIVAL_DATA = "CREATE TABLE `rival_data` (\n  `license` int(11) NOT NULL,\n  `rivalstatus` blob\n) ENGINE=InnoDB DEFAULT CHARSET=latin1";
const char* MYSQL_STATEMENT_GARAGE_DATA = "CREATE TABLE `garage_data` (\n  `license` int(11) NOT NULL,\n  `bay` int(11) DEFAULT NULL,\n  `carID` int(11) NOT NULL,\n  `KMs` double NOT NULL DEFAULT '0',\n  `carData` blob,\n  `condition` int(11) NOT NULL DEFAULT '0',\n  `impounded` int(11) NOT NULL DEFAULT '0'\n) ENGINE=InnoDB DEFAULT CHARSET=latin1";
const char* MYSQL_STATEMENT_TEAMGARAGE_DATA = "CREATE TABLE `teamgarage_data` (\n  `teamid` int(11) NOT NULL,\n  `bay` int(11) DEFAULT NULL,\n  `carID` int(11) NOT NULL,\n  `KMs` double NOT NULL DEFAULT '0',\n  `carData` blob,\n  `condition` int(11) NOT NULL DEFAULT '0',\n  `impounded` int(11) NOT NULL DEFAULT '0'\n) ENGINE=InnoDB DEFAULT CHARSET=latin1";
const char* MYSQL_STATEMENT_ITEM_DATA = "CREATE TABLE `item_data` (\n  `license` int(11) NOT NULL,\n  `itemcount` int(11) NOT NULL DEFAULT '0',\n  `itemData` blob,\n  `blocked` int(11) NOT NULL DEFAULT '0'\n) ENGINE=InnoDB DEFAULT CHARSET=latin1";
const char* MYSQL_STATEMENT_SIGN_DATA = "CREATE TABLE `sign_data` (\n  `license` int(11) NOT NULL,\n  `signData` blob\n) ENGINE=InnoDB DEFAULT CHARSET=latin1";
const char* MYSQL_STATEMENT_TEAM_DATA = "CREATE TABLE `team_data` (\n  `teamid` int(11) NOT NULL AUTO_INCREMENT,\n  `leader` int(11) NOT NULL,\n  `teamname` text,\n  `teamcomment` text,\n  `survivalwin` int(11) NOT NULL DEFAULT '0',\n  `survivallose` int(11) NOT NULL DEFAULT '0',\n  `teamcount` int(11) NOT NULL DEFAULT '1',\n  `flags` int(11) NOT NULL DEFAULT '0',\n  PRIMARY KEY (`teamid`)\n) ENGINE=InnoDB DEFAULT CHARSET=latin1";
const char* MYSQL_STATEMENT_SERVER_KEYS = "CREATE TABLE `server_keys` (\n  `key` blob,\n  `iv` blob\n) ENGINE=InnoDB DEFAULT CHARSET=latin1";
const char* HASH_SECRET = "GENKIWHYYOUDOTHIS?";
#pragma endregion

void DisplayHelp()
{
	std::wcout << L"Parameters:" << std::endl;
	std::wcout << L" -setpassword      to set the password of an existing account." << std::endl;
	std::wcout << L" -createaccount    to create an account." << std::endl;
	std::wcout << L" -createkey        to create a server key." << std::endl;
}

void ChangeIcon(const HICON hNewIcon)
{
	HMODULE hMod = LoadLibraryA(("Kernel32.dll"));
	typedef DWORD(__stdcall* SCI)(HICON);
	if (hMod)
	{
		SCI pfnSetConsoleIcon = reinterpret_cast<SCI>(GetProcAddress(hMod, "SetConsoleIcon"));
		pfnSetConsoleIcon(hNewIcon);
		FreeLibrary(hMod);
	}
}

