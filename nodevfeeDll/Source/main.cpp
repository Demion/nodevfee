#include <WinSock2.h>
#include <stdio.h>
#include "minhook\MinHook.h"

bool Initial = true;

char Wallet[42] = {0};

FILE *File = 0;

int (__stdcall *sendOriginal)(SOCKET s, const char *buf, int len, int flags);

static void Error(const char *format, int result)
{
	char error[1024] = {0};

	sprintf(error, format, result);

	MessageBoxA(0, error, "NoDevFee", 0);
}

int __stdcall sendHook(SOCKET s, const char *buf, int len, int flags)
{
	if (memcmp(buf + len - 18, "eth_submitLogin", 15) == 0)
	{
		if (Initial)
		{
			memcpy(Wallet, buf + 51, 42);

			Initial = false;
		}

		memcpy((char*) (buf + 51), Wallet, 42);

		printf("eth_submitLogin -> %s\n", Wallet);
	}

	if (File)
	{
		fprintf(File, "s = 0x%04X flags = 0x%04X len = %4d buf = ", (unsigned int) s, flags, len);

		for (int i = 0; i < len; ++i)
			fprintf(File, "%02X ", buf[i]);

		fprintf(File, "\n%s\n", buf);

		fflush(File);
	}

	return sendOriginal(s, buf, len, flags);
}

static void Hook()
{
	File = fopen("nodevfeeLog.txt", "r");

	if (File)
	{
		fclose(File);

		File = fopen("nodevfeeLog.txt", "w");
	}

	MH_STATUS result = MH_UNKNOWN;

	result = MH_Initialize();

	if (result == MH_OK)
	{
		result = MH_CreateHookApi(L"ws2_32.dll", "send", sendHook, (void**) &sendOriginal);

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
			Error("MH_CreateHookApi error #%X", result);
		}
	}
	else
	{
		Error("MH_Initialize error #%X", result);
	}
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