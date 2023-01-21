#pragma once
#include "DebuggerBypass.h"

#include <stdio.h>
#include <format>
#define LOG(fmt, ...) printf("%s \n", std::format(fmt, __VA_ARGS__).c_str());

void RunMain(HMODULE *phModule)
{
	// New Console
	AllocConsole();
	freopen_s((FILE **)stdin, "CONIN$", "r", stdin);
	freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
	freopen_s((FILE **)stderr, "CONOUT$", "w", stderr);
	SetConsoleOutputCP(CP_UTF8);

	DebuggerBypassPre();

	while (GetModuleHandle(L"UserAssembly.dll") == nullptr)
	{
		LOG("UserAssembly.dll isn't initialized, waiting for 2 sec.");
		Sleep(2000);
	}

	LOG("Waiting 10sec for loading game library.");
	Sleep(15000);

	DebuggerBypassPost();

	LOG("Debug bypass complete");
}