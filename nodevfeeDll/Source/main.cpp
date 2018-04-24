#include <WinSock2.h>
#include <Mswsock.h>
#include <stdio.h>
#include "minhook\MinHook.h"

wchar_t Name[MAX_PATH + 1] = {0};

bool Initial = true;

char Wallet[43] = {0};

WSABUF *Buffers = 0;

sockaddr Address = {0};

FILE *LogFile = 0, *WalletFile = 0, *PoolsFile = 0;

struct Pool
{
	char Address[256];

	unsigned int Port;
};

Pool Pools[256] = {0};

int PoolCount = 0;

char *Protocols[4] = {"eth_submitLogin", "eth_login", "mining.authorize", "mining.submit"};

int ProtocolCount = 4;

int (__stdcall *sendOriginal)(SOCKET s, const char *buf, int len, int flags);
int (__stdcall *WSASendOriginal)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
								 LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

int (__stdcall *connectOriginal)(SOCKET s, const struct sockaddr *name, int namelen);
BOOL (__stdcall *ConnectExOriginal)(SOCKET s, const struct sockaddr *name, int namelen, PVOID lpSendBuffer, DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped);

int (__stdcall *WSAIoctlOriginal)(SOCKET s, DWORD dwIoControlCode, LPVOID lpvInBuffer, DWORD cbInBuffer, LPVOID lpvOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbBytesReturned,
						 LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

static void Error(const wchar_t *format, int result)
{
	wchar_t error[1024] = {0};

	_swprintf(error, format, result);

	MessageBoxW(0, error, Name, 0);
}

void OnSend(SOCKET s, const char *buffer, int len, int flags, int index)
{
	memcpy(Buffers[index].buf, buffer, len);
	Buffers[index].len = len;
	Buffers[index].buf[len] = 0;

	char *buf = Buffers[index].buf;

	int protocol = -1;

	for (int i = 0; i < ProtocolCount; ++i)
	{
		if (strstr(buf, Protocols[i]) != 0)
		{
			protocol = i;

			break;
		}
	}

	if (protocol != -1)
	{
		char *wallet = strstr(buf, "0x");

		if (wallet != 0)
		{
			if (Initial)
			{
				memcpy(Wallet, wallet, 42);

				Initial = false;
			}

			memcpy(wallet, Wallet, 42);

			printf("%ls: %s[%d] -> %s\n", Name, Protocols[protocol], protocol, Wallet);
		}
		else
		{
			printf("%ls: %s[%d] -> Error\n", Name, Protocols[protocol], protocol);
		}
	}

	if (LogFile)
	{
		fprintf(LogFile, "s = 0x%04X flags = 0x%04X len = %4d buf = ", (unsigned int) s, flags, len);

		for (int i = 0; i < len; ++i)
			fprintf(LogFile, "%02X ", buf[i]);

		fprintf(LogFile, "\n");

		fwrite(buf, len, 1, LogFile);

		fflush(LogFile);
	}
}

void OnConnect(SOCKET s, const struct sockaddr *name, int namelen)
{
	memcpy(&Address, name, namelen);

	sockaddr_in *addr = (sockaddr_in*) &Address;

	bool match = false;

	for (int i = 1; ((i < PoolCount) && (!match)); ++i)
	{
		if (addr->sin_port == htons(Pools[i].Port))
		{
			hostent *host = gethostbyname(Pools[i].Address);

			if (host != 0)
			{
				if (host->h_addrtype == addr->sin_family)
				{
					for (int j = 0; ((host->h_addr_list[j] != 0) && (!match)); ++j)
					{
						if (addr->sin_addr.S_un.S_addr == ((in_addr*) host->h_addr_list[j])->S_un.S_addr)
						{
							match = true;

							host = gethostbyname(Pools[0].Address);

							if (host != 0)
							{
								if (host->h_addrtype == addr->sin_family)
								{
									addr->sin_port = htons(Pools[0].Port);
									addr->sin_addr.S_un.S_addr = ((in_addr*) host->h_addr_list[0])->S_un.S_addr;

									printf("%ls: connect -> %s:%d\n", Name, Pools[0].Address, Pools[0].Port);
								}
								else
								{
									printf("%ls: connect -> Error\n", Name);
								}
							}
							else
							{
								printf("%ls: connect -> Error\n", Name);
							}
						}
					}
				}
			}
		}
	}

	if (LogFile)
	{
		fprintf(LogFile, "s = 0x%04X sin_family = 0x%04X sin_addr = %s sin_port = %4d namelen = %4d\n",
			(unsigned int) s, addr->sin_family, inet_ntoa(addr->sin_addr), ntohs(addr->sin_port), namelen);

		fflush(LogFile);
	}
}

int __stdcall sendHook(SOCKET s, const char *buf, int len, int flags)
{
	OnSend(s, buf, len, flags, 0);

	return sendOriginal(s, Buffers[0].buf, len, flags);
}

int __stdcall WSASendHook(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	for (unsigned int i = 0; i < dwBufferCount; ++i)
		OnSend(s, lpBuffers[i].buf, lpBuffers[i].len, dwFlags, i);

	return WSASendOriginal(s, Buffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
}

int __stdcall connectHook(SOCKET s, const struct sockaddr *name, int namelen)
{
	OnConnect(s, name, namelen);

	return connectOriginal(s, &Address, namelen);
}

BOOL __stdcall ConnectExHook(SOCKET s, const struct sockaddr *name, int namelen, PVOID lpSendBuffer, DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped)
{
	OnConnect(s, name, namelen);

	return ConnectExOriginal(s, &Address, namelen, lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped);
}

int __stdcall WSAIoctlHook(SOCKET s, DWORD dwIoControlCode, LPVOID lpvInBuffer, DWORD cbInBuffer, LPVOID lpvOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbBytesReturned,
						 LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	int result = WSAIoctlOriginal(s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer, cbOutBuffer, lpcbBytesReturned, lpOverlapped, lpCompletionRoutine);
		
	if (dwIoControlCode == SIO_GET_EXTENSION_FUNCTION_POINTER)
	{
		GUID GUIDConnectEx = WSAID_CONNECTEX;

		if (cbInBuffer == sizeof(GUIDConnectEx))
		{
			if (memcmp(lpvInBuffer, &GUIDConnectEx, cbInBuffer) == 0)
			{
				ConnectExOriginal = *((BOOL(__stdcall **)(SOCKET, const struct sockaddr*, int, PVOID, DWORD, LPDWORD, LPOVERLAPPED)) lpvOutBuffer);
				*((BOOL(__stdcall **)(SOCKET, const struct sockaddr*, int, PVOID, DWORD, LPDWORD, LPOVERLAPPED)) lpvOutBuffer) = ConnectExHook;
			}
		}
	}

	return result;
}

static void GetName(HINSTANCE instance)
{
	wchar_t path[MAX_PATH + 1] = {0};

	if (GetModuleFileNameW(instance, path, MAX_PATH) != 0)
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
	else
	{
		Error(L"GetModuleFileNameW error #%X", GetLastError());
	}
}

static void SetFileName(wchar_t *fileName, const wchar_t *postfix)
{
	wcscpy(fileName, Name);

	wcscat(fileName, postfix);
}

static void Hook(HINSTANCE instance)
{
	GetName(instance);

	int bufferCount = 100, bufferSize = 10240;

	Buffers = (WSABUF*) malloc(sizeof(WSABUF) * bufferCount);

	for (int i = 0; i < bufferCount; ++i)
		Buffers[i].buf = (char*) malloc(bufferSize);

	wchar_t logFileName[MAX_PATH + 1] = {0};

	SetFileName(logFileName, L"Log.txt");

	LogFile = _wfopen(logFileName, L"r");

	if (LogFile)
	{
		fclose(LogFile);

		LogFile = _wfopen(logFileName, L"w");
	}

	wchar_t walletFileName[MAX_PATH + 1] = {0};

	SetFileName(walletFileName, L"Wallet.txt");

	WalletFile = _wfopen(walletFileName, L"r");

	if (WalletFile)
	{
		if (fread(Wallet, 1, 42, WalletFile) == 42)
			Initial = false;

		fclose(WalletFile);
	}

	wchar_t poolsFileName[MAX_PATH + 1] = {0};

	SetFileName(poolsFileName, L"Pools.txt");

	PoolsFile = _wfopen(poolsFileName, L"r");

	if (PoolsFile)
	{
		fscanf(PoolsFile, "%d\n", &PoolCount);

		for (int i = 0; i < PoolCount; ++i)
			fscanf(PoolsFile, "%s %d\n", Pools[i].Address, &Pools[i].Port);

		fclose(PoolsFile);
	}
	
	MH_STATUS result = MH_UNKNOWN;

	result = MH_Initialize();

	if (result == MH_OK)
	{
		result = MH_CreateHookApi(L"ws2_32.dll", "send", sendHook, (void**) &sendOriginal);

		if (result == MH_OK)
		{
			result = MH_CreateHookApi(L"ws2_32.dll", "WSASend", WSASendHook, (void**) &WSASendOriginal);

			if (result == MH_OK)
			{
				result = MH_CreateHookApi(L"ws2_32.dll", "connect", connectHook, (void**) &connectOriginal);

				if (result == MH_OK)
				{
					result = MH_CreateHookApi(L"ws2_32.dll", "WSAIoctl", WSAIoctlHook, (void**) &WSAIoctlOriginal);

					if (result == MH_OK)
					{
						result = MH_EnableHook(MH_ALL_HOOKS);

						if (result != MH_OK)
						{
							Error(L"MH_EnableHook error #%X", result);
						}
					}
					else
					{
						Error(L"MH_CreateHookApi WSAIoctl error #%X", result);
					}
				}
				else
				{
					Error(L"MH_CreateHookApi connect error #%X", result);
				}
			}
			else
			{
				Error(L"MH_CreateHookApi WSASend error #%X", result);
			}
		}
		else
		{
			Error(L"MH_CreateHookApi send error #%X", result);
		}
	}
	else
	{
		Error(L"MH_Initialize error #%X", result);
	}

	printf("%ls v0.2.6b\n", Name);
}

int __stdcall DllMain(HINSTANCE instance, unsigned long int reason, void *reserved)
{
	switch (reason)
	{
		case DLL_PROCESS_DETACH:

			break;

		case DLL_PROCESS_ATTACH:

			Hook(instance);

			break;

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:

			break;
	}

	return true;
}
