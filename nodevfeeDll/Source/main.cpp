#include <WinSock2.h>
#include <Mswsock.h>
#include <stdio.h>
#include "minhook\MinHook.h"

bool Initial = true;

char Wallet[43] = {0};

FILE *LogFile = 0, *WalletFile = 0, *PoolsFile = 0;

struct Pool
{
	char Address[256];

	unsigned int Port;
};

Pool Pools[256] = {0};

int PoolCount = 0;

char *Protocols[2] = {"eth_submitLogin", "eth_login"};

int ProtocolCount = 2;

int (__stdcall *sendOriginal)(SOCKET s, const char *buf, int len, int flags);
int (__stdcall *WSASendOriginal)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
								 LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

int (__stdcall *connectOriginal)(SOCKET s, const struct sockaddr *name, int namelen);
BOOL (__stdcall *ConnectExOriginal)(SOCKET s, const struct sockaddr *name, int namelen, PVOID lpSendBuffer, DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped);

int (__stdcall *WSAIoctlOriginal)(SOCKET s, DWORD dwIoControlCode, LPVOID lpvInBuffer, DWORD cbInBuffer, LPVOID lpvOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbBytesReturned,
						 LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

static void Error(const char *format, int result)
{
	char error[1024] = {0};

	sprintf(error, format, result);

	MessageBoxA(0, error, "NoDevFeeDll", 0);
}

void OnSend(SOCKET s, char *buf, int len, int flags)
{
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

			printf("NoDevFee: %s[%d] -> %s\n", Protocols[protocol], protocol, Wallet);
		}
		else
		{
			printf("NoDevFee: %s[%d] -> Error\n", Protocols[protocol], protocol);
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

void OnConnect(SOCKET s, struct sockaddr *name, int namelen)
{
	sockaddr_in *addr = (sockaddr_in*) name;

	if (PoolCount > 1)
	{
		for (int i = 1; i < PoolCount; ++i)
		{
			hostent *host = gethostbyname(Pools[i].Address);

			if (host != 0)
			{
				if ((host->h_addrtype == addr->sin_family) && (addr->sin_port == htons(Pools[i].Port)) && (addr->sin_addr.S_un.S_addr == ((in_addr*) host->h_addr_list[0])->S_un.S_addr))
				{
					host = gethostbyname(Pools[0].Address);

					if (host != 0)
					{
						addr->sin_port = htons(Pools[0].Port);
						addr->sin_addr.S_un.S_addr = ((in_addr*) host->h_addr_list[0])->S_un.S_addr;

						printf("NoDevFee: connect -> %s:%d\n", Pools[0].Address, Pools[0].Port);

						break;
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
	OnSend(s, (char*) buf, len, flags);

	return sendOriginal(s, buf, len, flags);
}

int __stdcall WSASendHook(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	for (unsigned int i = 0; i < dwBufferCount; ++i)
		OnSend(s, lpBuffers[i].buf, lpBuffers[i].len, dwFlags);

	return WSASendOriginal(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
}

int __stdcall connectHook(SOCKET s, const struct sockaddr *name, int namelen)
{
	OnConnect(s, (sockaddr*) name, namelen);

	return connectOriginal(s, name, namelen);
}

BOOL __stdcall ConnectExHook(SOCKET s, const struct sockaddr *name, int namelen, PVOID lpSendBuffer, DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped)
{
	OnConnect(s, (sockaddr*) name, namelen);

	return ConnectExOriginal(s, name, namelen, lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped);
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

static void Hook()
{
	LogFile = fopen("nodevfeeLog.txt", "r");

	if (LogFile)
	{
		fclose(LogFile);

		LogFile = fopen("nodevfeeLog.txt", "w");
	}

	WalletFile = fopen("nodevfeeWallet.txt", "r");

	if (WalletFile)
	{
		if (fread(Wallet, 1, 42, WalletFile) == 42)
			Initial = false;

		fclose(WalletFile);
	}

	PoolsFile = fopen("nodevfeePools.txt", "r");

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
							Error("MH_EnableHook error #%X", result);
						}
					}
					else
					{
						Error("MH_CreateHookApi WSAIoctl error #%X", result);
					}
				}
				else
				{
					Error("MH_CreateHookApi connect error #%X", result);
				}
			}
			else
			{
				Error("MH_CreateHookApi WSASend error #%X", result);
			}
		}
		else
		{
			Error("MH_CreateHookApi send error #%X", result);
		}
	}
	else
	{
		Error("MH_Initialize error #%X", result);
	}

	printf("NoDevFee v0.2.4b\n");
}

int __stdcall DllMain(HINSTANCE instance, unsigned long int reason, void *reserved)
{
	switch (reason)
	{
		case DLL_PROCESS_DETACH:

			break;

		case DLL_PROCESS_ATTACH:

			Hook();

			break;

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:

			break;
	}

	return true;
}