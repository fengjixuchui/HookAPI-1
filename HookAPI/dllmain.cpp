﻿// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "windows.h"
#include "tchar.h"

#define STATUS_SUCCESS						(0x00000001) 

typedef LONG NTSTATUS;

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation = 0,
	SystemPerformanceInformation = 2,
	SystemTimeOfDayInformation = 3,
	SystemProcessInformation = 5,
	SystemProcessorPerformanceInformation = 8,
	SystemInterruptInformation = 23,
	SystemExceptionInformation = 33,
	SystemRegistryQuotaInformation = 37,
	SystemLookasideInformation = 45
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG NextEntryOffset;
	ULONG NumberOfThreads;
	BYTE Reserved1[48];
	PVOID Reserved2[3];
	HANDLE UniqueProcessId;
	PVOID Reserved3;
	ULONG HandleCount;
	BYTE Reserved4[4];
	PVOID Reserved5[11];
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;
	LARGE_INTEGER Reserved6[6];
} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

typedef NTSTATUS(WINAPI* PFZWQUERYSYSTEMINFORMATION)
(SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength);

#define DEF_NTDLL                       ("kernel32.dll")
#define DEF_ZWQUERYSYSTEMINFORMATION    ("DeleteFileA")


// global variable (in sharing memory)
#pragma comment(linker, "/SECTION:.SHARE,RWS")
#pragma data_seg(".SHARE")
TCHAR g_szProcName[MAX_PATH] = { 0, };
#pragma data_seg()

// global variable
BYTE g_pOrgBytes[5] = { 0, };


BOOL hook_by_code(LPCSTR szDllName, LPCSTR szFuncName, PROC pfnNew, PBYTE pOrgBytes)
{
	FARPROC pfnOrg;
	DWORD dwOldProtect, dwAddress;
	BYTE pBuf[5] = { 0xE9, 0, };
	PBYTE pByte;

	// 获取挂钩的API地址
	pfnOrg = (FARPROC)GetProcAddress(GetModuleHandleA(szDllName), szFuncName);
	pByte = (PBYTE)pfnOrg;

	if (pByte[0] == 0xE9)
		return FALSE;

	VirtualProtect((LPVOID)pfnOrg, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	memcpy(pOrgBytes, pfnOrg, 5);

	// JMP 计算地址 (E9 XXXX)
	// => XXXX = pfnNew - pfnOrg - 5
	dwAddress = (DWORD)pfnNew - (DWORD)pfnOrg - 5;
	memcpy(&pBuf[1], &dwAddress, 4);

	// Hook - 5 byte  (JMP XXXX)
	memcpy(pfnOrg, pBuf, 5);

	VirtualProtect((LPVOID)pfnOrg, 5, dwOldProtect, &dwOldProtect);

	return TRUE;
}


BOOL unhook_by_code(LPCSTR szDllName, LPCSTR szFuncName, PBYTE pOrgBytes)
{
	FARPROC pFunc;
	DWORD dwOldProtect;
	PBYTE pByte;


	pFunc = GetProcAddress(GetModuleHandleA(szDllName), szFuncName);
	pByte = (PBYTE)pFunc;


	if (pByte[0] != 0xE9)
		return FALSE;

	VirtualProtect((LPVOID)pFunc, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	// Unhook
	memcpy(pFunc, pOrgBytes, 5);

	VirtualProtect((LPVOID)pFunc, 5, dwOldProtect, &dwOldProtect);

	return TRUE;
}


NTSTATUS WINAPI NewZwQuerySystemInformation(
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength)
{
	NTSTATUS status;
	FARPROC pFunc;
	PSYSTEM_PROCESS_INFORMATION pCur, pPrev;
	char szProcName[MAX_PATH] = { 0, };

	//	//  unhook
	//	unhook_by_code(DEF_NTDLL, DEF_ZWQUERYSYSTEMINFORMATION, g_pOrgBytes);
	//
	//	// original API 
	//	pFunc = GetProcAddress(GetModuleHandleA(DEF_NTDLL),
	//		DEF_ZWQUERYSYSTEMINFORMATION);
	//	status = ((PFZWQUERYSYSTEMINFORMATION)pFunc)
	//		(SystemInformationClass, SystemInformation,
	//			SystemInformationLength, ReturnLength);
	//
	//	if (status != STATUS_SUCCESS)
	//		goto __NTQUERYSYSTEMINFORMATION_END;
	//
	//	// SystemProcessInformation 仅在以下情况下有效
	//	if (SystemInformationClass == SystemProcessInformation)
	//	{
	//		// SYSTEM_PROCESS_INFORMATION 
	//		// pCur 是 single linked list 的 head
	//		pCur = (PSYSTEM_PROCESS_INFORMATION)SystemInformation;
	//
	//		while (TRUE)
	//		{
	//			// 比较进程名称
	//			// g_szProcName = 要隐藏的进程名称
	//			// (=> SetProcName() 开始)
	//			if (pCur->Reserved2[1] != NULL)
	//			{
	//				if (!_tcsicmp((PWSTR)pCur->Reserved2[1], g_szProcName))
	//				{
	//					// 从链接列表中删除隐身进程
	//					if (pCur->NextEntryOffset == 0)
	//						pPrev->NextEntryOffset = 0;
	//					else
	//						pPrev->NextEntryOffset += pCur->NextEntryOffset;
	//				}
	//				else
	//					pPrev = pCur;
	//			}
	//
	//			if (pCur->NextEntryOffset == 0)
	//				break;
	//
	//			// 链接列表中的下一项
	//			pCur = (PSYSTEM_PROCESS_INFORMATION)
	//				((ULONG)pCur + pCur->NextEntryOffset);
	//		}
	//	}
	//
	//__NTQUERYSYSTEMINFORMATION_END:
	//
	//	// 再次在函数结束之前 API Hooking
	//	hook_by_code(DEF_NTDLL, DEF_ZWQUERYSYSTEMINFORMATION,
	//		(PROC)NewZwQuerySystemInformation, g_pOrgBytes);

	MessageBox(NULL, TEXT("i am DeletefileA hook api"), TEXT("hi"), MB_ICONINFORMATION | MB_YESNO);

	return STATUS_SUCCESS;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	char            szCurProc[MAX_PATH] = { 0, };
	char* p = NULL;

	// #1. 异常处理
	// 如果当前进程是HookProc.exe，退出而不进行钩挂
	GetModuleFileNameA(NULL, szCurProc, MAX_PATH);
	p = strrchr(szCurProc, '\\');
	if ((p != NULL) && !_stricmp(p + 1, "HideProc.exe"))
		return TRUE;

	switch (fdwReason)
	{
		// #2. API Hooking
	case DLL_PROCESS_ATTACH:
		hook_by_code(DEF_NTDLL, DEF_ZWQUERYSYSTEMINFORMATION,
			(PROC)NewZwQuerySystemInformation, g_pOrgBytes);
		break;

		// #3. API Unhooking 
	case DLL_PROCESS_DETACH:
		unhook_by_code(DEF_NTDLL, DEF_ZWQUERYSYSTEMINFORMATION,
			g_pOrgBytes);
		break;
	}

	return TRUE;
}


#ifdef __cplusplus
extern "C" {
#endif
	__declspec(dllexport) void SetProcName(LPCTSTR szProcName)
	{
		_tcscpy_s(g_szProcName, szProcName);
	}
#ifdef __cplusplus
}
#endif
