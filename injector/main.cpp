/*
	Program responsible for injecting the aimbot dll into the bzflag process.

	Steps to do this are as follows:
		1. Get the full path of our dll.
		2. Get the process id of the bzflag process.
		3. Create a process handle with all permissions.
		4. Allocate memory inside bzflag to store the full path of the dll.
		5. Write the dll name into the allocated memory.
		6. Create a remote thread inside bzflag that calls the LoadLibrary function. 
			Pass the dll path as the argument to LoadLibrary.
		
	Requirements:
		1. bzflag and injector must be run as an adminstrator-level user.
		2. The bzflag window must be named "bzflag".
		3. The injector and dll must be in the same directory.
*/

#include <windows.h>
#include <iostream>
#include <stdlib.h>

int main(int argc, char** argv) {
	char dllPath[_MAX_PATH] = { 0 };
	DWORD processId = 0;
	HANDLE process = NULL;
	void* lpBaseAddress;
	DWORD exitCode;

	// Get the full path of this executable, remove the executable name from the end, and append the dll name
	GetModuleFileName(NULL, dllPath, sizeof(dllPath));
	for (int i = strlen(dllPath); dllPath[i] != '\\'; i--) {
		dllPath[i] = 0;
	}
	strcat_s(dllPath, sizeof(dllPath), "aimbot.dll");

	HWND hwnd = FindWindow(NULL, "bzflag");
	GetWindowThreadProcessId(hwnd, &processId);
	process = OpenProcess(PROCESS_ALL_ACCESS, true, processId);
	lpBaseAddress = VirtualAllocEx(process, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (!lpBaseAddress) {
		std::cout << "Failed to allocate memory" << std::endl;
		return EXIT_FAILURE;
	}
	WriteProcessMemory(process, lpBaseAddress, dllPath, strlen(dllPath) + 1, NULL);

	HANDLE thread = CreateRemoteThread(
			process, 
			NULL,
			0,
			(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"),
			lpBaseAddress,
			0,
			NULL
		);
	if (!thread) {
		std::cout << "Failed to create thread" << std::endl;
		return EXIT_FAILURE;
	}
	WaitForSingleObject(thread, INFINITE);
	GetExitCodeThread(thread, &exitCode);

	if (!exitCode) {
		std::cout << "Injection failed" << std::endl;
		return EXIT_FAILURE;
	} 
	else {
		std::cout << "Injected successfully" << std::endl;
	}

	VirtualFreeEx(process, lpBaseAddress, 0, MEM_RELEASE);
	CloseHandle(thread);
	CloseHandle(process);
	
	return EXIT_SUCCESS;
}
