#pragma once
#include "DebuggerBypass.h"

#include <stdio.h>
#include <format>
#include "CloseHandleByName.h"
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

	DebuggerBypassPost();

	LOG("[INFO] Debug bypass complete. You can inject other dll now. Happy debugging!");

	//这里不加括号编译过不了 傻逼编译器 加了看着难受啊啊啊啊啊啊啊
	if (CloseHandleByName(L"\\Device\\mhyprot2")) {
		LOG("[INFO] The Mhyprot2 handle was successfully closed. Happy hacking!");
	}
	else
		LOG("[ERROR] Failed to close mhyprot2 handle. Report this Issue and describe it.");

	return;
}