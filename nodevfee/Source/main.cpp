#include <Windows.h>
#include <TlHelp32.h>
#include <stdio.h>

static void Error(const char *format, int result)
{
	char error[1024] = {0};

	sprintf(error, format, result);

	MessageBoxA(0, error, "NoDevFee", 0);
}

static void InjectDll(HANDLE process, const wchar_t *dllName)
{
	HMODULE kernel = GetModuleHandleW(L"kernel32.dll");

	if (kernel != 0)
	{
		void *loadLibrary = GetProcAddress(kernel, "LoadLibraryW");

		if (loadLibrary != 0)
		{
			size_t length = (wcslen(dllName) + 1) * sizeof(wchar_t);

			void *remoteString = VirtualAllocEx(process, 0, length, MEM_COMMIT, PAGE_READWRITE);

			if (remoteString != 0)
			{
				if (WriteProcessMemory(process, remoteString, dllName, length, 0))
				{
					HANDLE thread = CreateRemoteThread(process, 0, 0, (LPTHREAD_START_ROUTINE) loadLibrary, remoteString, 0, 0);

					if (thread != 0)
					{
						if (WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0)
						{
							unsigned long int exitCode = 0;

							if (GetExitCodeThread(thread, &exitCode))
							{
								if (exitCode == 0)
								{
									Error("LoadLibraryW error #%X", GetLastError());
								}
							}
							else
							{
								Error("GetExitCodeThread error #%X", GetLastError());
							}
						}
						else
						{
							Error("WaitForSingleObject error #%X", GetLastError());
						}

						VirtualFreeEx(process, remoteString, 0, MEM_RELEASE);

						CloseHandle(thread);
					}
					else
					{
						Error("CreateRemoteThread error #%X", GetLastError());
					}
				}
				else
				{
					Error("WriteProcessMemory error #%X", GetLastError());
				}
			}
			else
			{
				Error("VirtualAllocEx error #%X", GetLastError());
			}
		}
		else
		{
			Error("GetProcAddress error #%X", GetLastError());
		}
	}
	else
	{
		Error("GetModuleHandleW error #%X", GetLastError());
	}
}

int wmain(int argc, wchar_t *argv[])
{
	if (argc > 1)
	{
		STARTUPINFO si = {0};

		si.cb = sizeof(si);

		PROCESS_INFORMATION pi = {0};

		if (CreateProcessW(0, wcsstr(GetCommandLineW(), argv[1]), 0, 0, false, CREATE_SUSPENDED | REALTIME_PRIORITY_CLASS, 0, 0, &si, &pi) != 0)
		{
			InjectDll(pi.hProcess, L"nodevfeeDll.dll");

			if (ResumeThread(pi.hThread) == -1)
			{
				Error("ResumeThread error #%X", GetLastError());
			}

			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		}
		else
		{
			Error("CreateProcessW error #%X", GetLastError());
		}
	}
	else
	{
		FILE *injectFile = fopen("nodevfeeInject.txt", "r");

		if (injectFile)
		{
			wchar_t processName[256] = {0};

			fscanf(injectFile, "%ls", processName);

			fclose(injectFile);

			if (wcslen(processName) > 0)
			{
				unsigned int processId = 0;

				while (true)
				{
					HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

					if (snapshot != INVALID_HANDLE_VALUE)
					{
						PROCESSENTRY32W entry = {0};

						entry.dwSize = sizeof(entry);

						if (Process32FirstW(snapshot, &entry))
						{
							do
							{
								if (wcscmp(entry.szExeFile, processName) == 0)
								{
									if (entry.th32ProcessID != processId)
									{
										processId = entry.th32ProcessID;

										HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, false, processId);

										if (process != 0)
										{
											InjectDll(process, L"nodevfeeDll.dll");

											CloseHandle(process);

											printf("\"%ls\" [%d] -> \"nodevfeeDll.dll\"\n", processName, processId);
										}
										else
										{
											Error("OpenProcess error #%X", GetLastError());
										}
									}
								}
							}
							while (Process32NextW(snapshot, &entry));
						}
						else
						{
							Error("Process32FirstW error #%X", GetLastError());
						}
					}
					else
					{
						Error("CreateToolhelp32Snapshot error #%X", GetLastError());
					}

					Sleep(1000);
				}
			}
		}
	}

	return EXIT_SUCCESS;
}