#pragma once
#include "globals.h"
#include <iostream>
#include <fstream>
#include <string>

#include <stdio.h>

class Logger
{
public:
	Logger(bool type = false);
	Logger(char* path, bool type = false);
	~Logger();
	void Log(LOGTYPE type, const wchar_t* in, ...);
	std::string LogPath();
	void LogPath(char* in);
	void LogPath(std::string& in);
	bool IsLogPathSet();
	std::wstring PacketToText(uint8_t* buf, uint32_t len);
	std::wstring ToWide(std::string in);
	std::string ToNarrow(std::wstring in);
private:
	bool isService;
	std::string logpath;
};
