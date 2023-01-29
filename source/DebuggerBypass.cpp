#include "DebuggerBypass.h"
#include "Hook.h"

#include <format>
#define LOG(fmtstr, ...) printf("%s\n", std::format(fmtstr, ##__VA_ARGS__).c_str());

NtQueryInformationThread_t fnNtQueryInformationThread = nullptr;
NtSetInformationThread_t fnNtSetInformationThread = nullptr;
DbgUiRemoteBreakin_t fnDbgUiRemoteBreakin = nullptr;

std::string GetLastErrorAsString(DWORD errorId = 0)
{
	//Get the error message ID, if any.
	DWORD errorMessageID = errorId == 0 ? ::GetLastError() : errorId;
	if (errorMessageID == 0)
	{
		return std::string(); //No error message has been recorded
	}

	LPSTR messageBuffer = nullptr;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
								 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	//Copy the error message into a std::string.
	std::string message(messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}

void DebuggerBypassPre()
{
	if (!Patch_NtSetInformationThread())
		LOG("Failed to patch NtSetInformationThread. Main thread will be hidden from debugger.");
}

void DebuggerBypassPost()
{
	if (!Patch_DbgUiRemoteBreakin())
		LOG("Failed to patch DbgUiRemoteBreakin. The game will crash when you try to attach debbuger.");

	RunVEH();
	DisableVMP();
}

static void RunVEH()
{
	AddVectoredExceptionHandler(1, DebugHandler);
}

static bool Patch_NtSetInformationThread()
{
	if (fnNtSetInformationThread == nullptr && (FindAPI(), fnNtSetInformationThread == nullptr))
		return false;

	Hook::install(fnNtSetInformationThread, NtSetInformationThread_Hook);
	return true;
}

static bool Patch_DbgUiRemoteBreakin()
{
	if (fnDbgUiRemoteBreakin == nullptr && (FindAPI(), fnDbgUiRemoteBreakin == nullptr))
		return false;

	Hook::install(fnDbgUiRemoteBreakin, DbgUiRemoteBreakin_Hook);
	return true;
}

static void FindAPI()
{
	HMODULE hNTDLL = GetModuleHandle(L"ntdll.dll");
	if (hNTDLL == NULL)
	{
		LOG("[ERROR] Failed to get the \"ntdll.dll\" handle");
		return;
	}

	if (fnDbgUiRemoteBreakin == nullptr)
	{
		fnDbgUiRemoteBreakin = reinterpret_cast<DbgUiRemoteBreakin_t>(GetProcAddress(hNTDLL, "DbgUiRemoteBreakin"));
		if (fnDbgUiRemoteBreakin == nullptr)
			LOG("[ERROR] GetProcAddress(ntdll::DbgUiRemoteBreakin) failed");
	}

	if (fnNtQueryInformationThread == nullptr)
	{
		fnNtQueryInformationThread = reinterpret_cast<NtQueryInformationThread_t>(GetProcAddress(hNTDLL, "NtQueryInformationThread"));
		if (fnNtQueryInformationThread == nullptr)
			LOG("[ERROR] GetProcAddress(ntdll::NtQueryInformationThread) failed");
	}

	if (fnNtSetInformationThread == nullptr)
	{
		fnNtSetInformationThread = reinterpret_cast<NtSetInformationThread_t>(GetProcAddress(hNTDLL, "NtSetInformationThread"));
		if (fnNtSetInformationThread == nullptr)
			LOG("[ERROR] GetProcAddress(ntdll::NtSetInformationThread) failed");
	}
}

// Taken from https://github.com/yubie-re/vmp-virtualprotect-bypass/blob/main/src/vp-patch.hpp
static void DisableVMP()
{
	DWORD oldProtect;
	auto ntdll = GetModuleHandleA("ntdll.dll");
	bool isWine = (GetProcAddress(ntdll, "wine_get_version") != NULL);
	BYTE callCode = ((BYTE *)GetProcAddress(ntdll, isWine ? "NtPulseEvent" : "NtQuerySection"))[4] - 1;
	BYTE restore[] = { 0x4C, 0x8B, 0xD1, 0xB8, callCode };
	auto nt_vp = (BYTE *)GetProcAddress(ntdll, "NtProtectVirtualMemory");
	VirtualProtect(nt_vp, sizeof(restore), PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(nt_vp, restore, sizeof(restore));
	VirtualProtect(nt_vp, sizeof(restore), oldProtect, &oldProtect);
}

// Modified version of https://guidedhacking.com/threads/how-to-find-hidden-threads-threadhidefromdebugger-antidebug-trick.14281/
static bool IsThreadHidden(DWORD threadID)
{
	if (fnNtQueryInformationThread == nullptr &&
		(FindAPI(), fnNtQueryInformationThread == nullptr)) // Yeah, seems like a shit ^)
		return false;

	HANDLE hThread = OpenThread(
		THREAD_QUERY_INFORMATION,
		false,
		threadID);

	if (hThread == NULL)
	{
		LOG("\033[%41mError:\033[m {}", GetLastErrorAsString());
		return false;
	}

	unsigned char lHideThread = 0;
	ULONG lRet = 0;
	NTSTATUS errorCode = fnNtQueryInformationThread(hThread, (THREADINFOCLASS)0x11, &lHideThread, sizeof(lHideThread), &lRet);
	WaitForSingleObject(hThread, INFINITE);
	return static_cast<bool>(lHideThread);
}

static long WINAPI DebugHandler(PEXCEPTION_POINTERS exception)
{
	PEXCEPTION_RECORD record = exception->ExceptionRecord;
	PCONTEXT context = exception->ContextRecord;

	if (record->ExceptionCode == EXCEPTION_SINGLE_STEP)
	{
		SuspendThread(GetCurrentThread());
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	else if (record->ExceptionCode == EXCEPTION_BREAKPOINT)
	{
		CONTEXT ctx = {};
		ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

		DWORD threadID = GetThreadId(GetCurrentThread());
		LOG("Breakpoint exception! Thread: {}; Hidden: {}", threadID, IsThreadHidden(threadID) ? "true" : "false");
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	else if (context->Rip == 0x8887777)
	{
		SuspendThread(GetCurrentThread());
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

static void WINAPI DbgUiRemoteBreakin_Hook()
{
	return;
}

static NTSTATUS WINAPI NtSetInformationThread_Hook(HANDLE handle, THREADINFOCLASS infoClass, PVOID pValue, ULONG pSize)
{
	if (infoClass == ThreadHideFromDebugger)
	{
		return 1;
	}
	return CALL_ORIGIN(NtSetInformationThread_Hook, handle, infoClass, pValue, pSize);
}