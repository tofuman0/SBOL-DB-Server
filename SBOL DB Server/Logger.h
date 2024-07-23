#pragma once
#include "globals.h"
#include <fstream>
#include <iostream>
#include <string>

#include <stdio.h>

class Logger
{
public:
	Logger();
	Logger(char* path);
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
	void CheckLogPath(std::string path);
	std::string logpath;
};
