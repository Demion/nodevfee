#include <Windows.h>
#include <TlHelp32.h>
#include <stdio.h>

wchar_t Name[MAX_PATH + 1] = {0};

static void Error(const wchar_t *format, int result)
{
	wchar_t error[1024] = {0};

	_swprintf(error, format, result);

	MessageBoxW(0, error, Name, 0);
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
									Error(L"LoadLibraryW error #%X", GetLastError());
								}
							}
							else
							{
								Error(L"GetExitCodeThread error #%X", GetLastError());
							}
						}
						else
						{
							Error(L"WaitForSingleObject error #%X", GetLastError());
						}

						VirtualFreeEx(process, remoteString, 0, MEM_RELEASE);

						CloseHandle(thread);
					}
					else
					{
						Error(L"CreateRemoteThread error #%X", GetLastError());
					}
				}
				else
				{
					Error(L"WriteProcessMemory error #%X", GetLastError());
				}
			}
			else
			{
				Error(L"VirtualAllocEx error #%X", GetLastError());
			}
		}
		else
		{
			Error(L"GetProcAddress error #%X", GetLastError());
		}
	}
	else
	{
		Error(L"GetModuleHandleW error #%X", GetLastError());
	}
}

static void GetName(const wchar_t *path)
{
	int length = (int) wcslen(path);

	for (int i = length - 1; i >= 0; --i)
	{
		if ((path[i] == '/') || (path[i] == '\\'))
		{
			wcscpy(Name, path + i + 1);

			break;
		}
	}

	if (wcslen(Name) == 0)
		wcscpy(Name, path);

	Name[wcslen(Name) - 4] = 0;
}

static void SetFileName(wchar_t *fileName, const wchar_t *postfix)
{
	wcscpy(fileName, Name);

	wcscat(fileName, postfix);
}

int wmain(int argc, wchar_t *argv[])
{
	GetName(argv[0]);

	wchar_t dllFileName[MAX_PATH + 1] = {0};

	SetFileName(dllFileName, L".dll");

	if (argc > 1)
	{
		STARTUPINFO si = {0};

		si.cb = sizeof(si);

		PROCESS_INFORMATION pi = {0};

		int flags = CREATE_SUSPENDED;

		//flags |= REALTIME_PRIORITY_CLASS;

		if (CreateProcessW(0, wcsstr(GetCommandLineW(), argv[1]), 0, 0, false, flags, 0, 0, &si, &pi) != 0)
		{
			InjectDll(pi.hProcess, dllFileName);

			if (ResumeThread(pi.hThread) == -1)
			{
				Error(L"ResumeThread error #%X", GetLastError());
			}

			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		}
		else
		{
			Error(L"CreateProcessW error #%X", GetLastError());
		}
	}
	else
	{
		wchar_t injectFileName[MAX_PATH + 1] = {0};

		SetFileName(injectFileName, L"Inject.txt");

		FILE *injectFile = _wfopen(injectFileName, L"r");

		if (injectFile)
		{
			//ShowWindow(GetConsoleWindow(), SW_HIDE);

			wchar_t processName[MAX_PATH + 1] = {0};

			fscanf(injectFile, "%ls", processName);

			fclose(injectFile);

			if (wcslen(processName) > 0)
			{
				while (true)
				{
					HANDLE processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

					if (processSnapshot != INVALID_HANDLE_VALUE)
					{
						PROCESSENTRY32W processEntry = {0};

						processEntry.dwSize = sizeof(processEntry);

						if (Process32FirstW(processSnapshot, &processEntry))
						{
							do
							{
								if (wcscmp(processEntry.szExeFile, processName) == 0)
								{
									HANDLE moduleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processEntry.th32ProcessID);

									if (moduleSnapshot != INVALID_HANDLE_VALUE)
									{
										bool injected = false;

										MODULEENTRY32W moduleEntry = {0};

										moduleEntry.dwSize = sizeof(moduleEntry);

										if (Module32FirstW(moduleSnapshot, &moduleEntry))
										{
											do
											{
												if (wcscmp(moduleEntry.szModule, dllFileName) == 0)
												{
													injected = true;

													break;
												}
											}
											while (Module32NextW(moduleSnapshot, &moduleEntry));
										}
										else
										{
											Error(L"Module32FirstW error #%X", GetLastError());
										}

										if (!injected)
										{
											HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION,
																		 false, processEntry.th32ProcessID);

											if (process != 0)
											{
												InjectDll(process, dllFileName);

												printf("\"%ls\" [%d] -> \"%ls\"\n", processName, processEntry.th32ProcessID, dllFileName);

												CloseHandle(process);
											}
											else
											{
												Error(L"OpenProcess error #%X", GetLastError());
											}
										}

										CloseHandle(moduleSnapshot);
									}
									else
									{
										Error(L"CreateToolhelp32Snapshot error #%X", GetLastError());
									}
								}
							}
							while (Process32NextW(processSnapshot, &processEntry));
						}
						else
						{
							Error(L"Process32FirstW error #%X", GetLastError());
						}

						CloseHandle(processSnapshot);
					}
					else
					{
						Error(L"CreateToolhelp32Snapshot error #%X", GetLastError());
					}

					Sleep(1000);
				}
			}
		}
	}

	return EXIT_SUCCESS;
}