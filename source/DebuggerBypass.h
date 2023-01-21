#pragma once
//#include <Ntdef.h>
#include <windows.h>
#include <stdio.h>
#include <string>

enum THREADINFOCLASS
{
	ThreadHideFromDebugger = 0x11
};

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
//typedef NTSTATUS *PNTSTATUS;

typedef NTSTATUS(WINAPI *NtQueryInformationThread_t)(HANDLE, THREADINFOCLASS, PVOID, ULONG, LPVOID);
typedef NTSTATUS(WINAPI *NtSetInformationThread_t)(HANDLE, THREADINFOCLASS, PVOID, ULONG);
typedef void(WINAPI *DbgUiRemoteBreakin_t)();

std::string GetLastErrorAsString(DWORD errorId);

void DebuggerBypassPre();
void DebuggerBypassPost();

static void RunVEH();
static void FindAPI();
static void DisableVMP();
static bool Patch_NtSetInformationThread();
static bool Patch_DbgUiRemoteBreakin();

static long WINAPI DebugHandler(PEXCEPTION_POINTERS exception);
static void WINAPI DbgUiRemoteBreakin_Hook();
static NTSTATUS WINAPI NtSetInformationThread_Hook(HANDLE handle, THREADINFOCLASS infoClass, PVOID pValue, ULONG pSize);