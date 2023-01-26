#pragma once
#include "DebuggerBypass.h"

#include <stdio.h>
#include <format>
#define LOG(fmtstr, ...) printf("%s\n", std::format(fmtstr, ##__VA_ARGS__).c_str());

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
		LOG("[INFO] UserAssembly.dll isn't initialized. Waiting for 2 seconds.");
		Sleep(2000);
	}

	/* UNCOMMENT THIS and set larger value if your game crashs.*/
	//LOG("Waiting 10sec for loading game library.");
	//Sleep(10000);

	DebuggerBypassPost();

	LOG("[INFO] Debug bypass complete. You can inject other dll now. Happy debugging :)");
}