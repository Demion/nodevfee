#include <Windows.h>
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
				if (WriteProcessMemory(process, remoteString, dllName, length - sizeof(wchar_t), 0))
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

	return EXIT_SUCCESS;
}