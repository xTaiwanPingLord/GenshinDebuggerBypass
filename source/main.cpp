﻿#pragma once
#include "DebuggerBypass.h"
#include "CloseMhyprot3.h"

#include <stdio.h>
#include <format>
#define LOG(fmtstr, ...) printf("%s\n", std::format(fmtstr, ##__VA_ARGS__).c_str());

void RunMain(HMODULE* phModule)
{
	// New Console
	AllocConsole();
	freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
	SetConsoleOutputCP(CP_UTF8);

	DebuggerBypassPre();

	while (GetModuleHandle(L"UserAssembly.dll") == nullptr) {
		LOG("[INFO] UserAssembly.dll isn't initialized. Waiting for 2 second.");
		Sleep(2000);
	}

	DebuggerBypassPost();

	LOG("[INFO] Debug bypass complete.");

	//这里不加大括号编译过不了 🖕Fuck C++🖕 🖕Fuck Microsoft🖕
	if (CloseHandleByName(L"\\Device\\mhyprot2")) {
		LOG("[INFO] Mhyprot3 was closed successfully.");
	}
	else {
		LOG("[ERROR] Failed to close mhyprot3 handle.");
	}

	return;
}