#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "WinDivert\windivert.h"

#define DIVERT_QUEUE_LEN_MAX 8192
#define DIVERT_QUEUE_TIME_MAX 2048
#define DIVERT_PACKET_SIZE_MAX 0xFFFF

#define BYTESWAP16(x) ((((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00))
#define BYTESWAP32(x) ((((x) >> 24) & 0x000000FF) | (((x) >> 8) & 0x0000FF00) | (((x) << 8) & 0x00FF0000) | (((x) << 24) & 0xFF000000))

struct Host
{
	char Name[256];

	unsigned int Address;

	unsigned short int Port;
};

struct Protocol
{
	char Name[256];

	size_t Length;

	size_t Occ[UCHAR_MAX + 1];
};

const int ProtocolCount = 2;

Protocol Protocols[ProtocolCount + 1];

char MainWallet[256] = {0}, NoDevFeeWallet[256] = {0}, NoDevFeeWorker[256] = {0};

int NoDevFeeProtocol = 0, LogLevel = 2, ShowConsole = 1, OutputDelay = 10;

Host MainPools[256], DevFeePools[256], NoDevFeePools[256];

int MainPoolCount = 0, DevFeePoolCount = 0, NoDevFeePoolCount = 0;

Host MainHosts[256], DevFeeHosts[256], NoDevFeeHosts[256];

int MainHostCount = 0, DevFeeHostCount = 0, NoDevFeeHostCount = 0;

FILE *Log = 0;

static void CreateOccTable(const unsigned char *needle, size_t needleLength, size_t *occ)
{
	for (size_t i = 0; i < UCHAR_MAX + 1; ++i)
		occ[i] = needleLength;

	const size_t needleLengthMinus1 = needleLength - 1;

	for (size_t i = 0; i < needleLengthMinus1; ++i)
		occ[needle[i]] = needleLengthMinus1 - i;
}

inline const unsigned char* SearchBMH(const unsigned char *haystack, size_t haystackLength, const unsigned char *needle, size_t needleLength, const size_t *occ)
{
	if (needleLength > haystackLength)
		return 0;

	const size_t needleLengthMinus1 = needleLength - 1;

	const unsigned char lastNeedleChar = needle[needleLengthMinus1];

	size_t haystackPosition = 0;

	while (haystackPosition <= haystackLength - needleLength)
	{
		const unsigned char occChar = haystack[haystackPosition + needleLengthMinus1];

		if ((lastNeedleChar == occChar) && (memcmp(needle, haystack + haystackPosition, needleLengthMinus1) == 0))
			return haystack + haystackPosition;

		haystackPosition += occ[occChar];
	}

	return 0;
}

inline void Print(FILE *stream1, FILE *stream2, const char *format, ...)
{
	va_list list;

	va_start(list, format);

	if (stream1)
		vfprintf(stream1, format, list);

	if (stream2)
		vfprintf(stream2, format, list);

	va_end(list);
}

inline void PrintTime(FILE *stream1, FILE *stream2)
{
	time_t t = time(0);
	tm *local = localtime(&t);

	Print(stream1, stream2, "%02d.%02d.%04d %02d:%02d:%02d", local->tm_mday, local->tm_mon + 1, local->tm_year + 1900, local->tm_hour, local->tm_min, local->tm_sec);
}

inline void PrintPacket(FILE *stream1, FILE *stream2, const WINDIVERT_ADDRESS *address, const WINDIVERT_IPHDR *ipHeader, const WINDIVERT_TCPHDR *tcpHeader, const char *data, unsigned int dataLength)
{
	Print(stream1, stream2, "Packet [Direction=%u IfIdx=%u SubIfIdx=%u]\n", address->Direction, address->IfIdx, address->SubIfIdx);

	if (ipHeader)
	{
		unsigned char *srcAddr = (unsigned char*) &ipHeader->SrcAddr;
		unsigned char *dstAddr = (unsigned char*) &ipHeader->DstAddr;

		Print(stream1, stream2, 
			  "IPv4 [Version=%u HdrLength=%u TOS=%u Length=%u Id=0x%.4X "
			  "Reserved=%u DF=%u MF=%u FragOff=%u TTL=%u Protocol=%u "
			  "Checksum=0x%.4X SrcAddr=%u.%u.%u.%u DstAddr=%u.%u.%u.%u]\n",
			  ipHeader->Version, ipHeader->HdrLength,
			  ipHeader->TOS, BYTESWAP16(ipHeader->Length),
			  BYTESWAP16(ipHeader->Id), WINDIVERT_IPHDR_GET_RESERVED(ipHeader),
			  WINDIVERT_IPHDR_GET_DF(ipHeader), WINDIVERT_IPHDR_GET_MF(ipHeader),
			  BYTESWAP16(WINDIVERT_IPHDR_GET_FRAGOFF(ipHeader)), ipHeader->TTL,
			  ipHeader->Protocol, BYTESWAP16(ipHeader->Checksum),
			  srcAddr[0], srcAddr[1], srcAddr[2], srcAddr[3],
			  dstAddr[0], dstAddr[1], dstAddr[2], dstAddr[3]);
	}

	if (tcpHeader)
	{
		Print(stream1, stream2, 
			  "TCP [SrcPort=%u DstPort=%u SeqNum=%u AckNum=%u "
			  "HdrLength=%u Reserved1=%u Reserved2=%u Urg=%u Ack=%u "
			  "Psh=%u Rst=%u Syn=%u Fin=%u Window=%u Checksum=0x%.4X "
			  "UrgPtr=%u]\n",
			  BYTESWAP16(tcpHeader->SrcPort), BYTESWAP16(tcpHeader->DstPort),
			  BYTESWAP32(tcpHeader->SeqNum), BYTESWAP32(tcpHeader->AckNum),
			  tcpHeader->HdrLength, tcpHeader->Reserved1,
			  tcpHeader->Reserved2, tcpHeader->Urg, tcpHeader->Ack,
			  tcpHeader->Psh, tcpHeader->Rst, tcpHeader->Syn,
			  tcpHeader->Fin, BYTESWAP16(tcpHeader->Window),
			  BYTESWAP16(tcpHeader->Checksum), BYTESWAP16(tcpHeader->UrgPtr));
	}

	Print(stream1, stream2, "Data [Length=%u]\n", dataLength);

	if ((data) && (dataLength > 0))
	{
		if (stream1)
			fwrite(data, dataLength, 1, stream1);

		if (stream2)
			fwrite(data, dataLength, 1, stream2);
	}

	Print(stream1, stream2, (data[dataLength - 1] == '\n') ? "\n" : "\n\n");
}

static int SetHosts(const Host *pools, int poolCount, Host *hosts, int hostCount)
{
	for (int i = 0; i < poolCount; ++i)
	{
		hostent *host = gethostbyname(pools[i].Name);

		if (host != 0)
		{
			if (host->h_addrtype == AF_INET)
			{
				for (int j = 0; (host->h_addr_list[j] != 0); ++j)
				{
					strcpy(hosts[hostCount].Name, pools[i].Name);

					hosts[hostCount].Address = ((in_addr*) host->h_addr_list[j])->S_un.S_addr;;
					hosts[hostCount].Port = BYTESWAP16(pools[i].Port);

					++hostCount;
				}
			}
		}
	}

	return hostCount;
}

static void ReadConfig(const char *fileName)
{
	FILE *config = fopen(fileName, "r");

	if (config)
	{
		fscanf(config, "%s\n%s\n%s\n%d\n", MainWallet, NoDevFeeWallet, NoDevFeeWorker, &NoDevFeeProtocol);

		fscanf(config, "%d\n", &MainPoolCount);

		for (int i = 0; i < MainPoolCount; ++i)
			fscanf(config, "%s %hu\n", MainPools[i].Name, &MainPools[i].Port);

		fscanf(config, "%d\n", &DevFeePoolCount);

		for (int i = 0; i < DevFeePoolCount; ++i)
			fscanf(config, "%s %hu\n", DevFeePools[i].Name, &DevFeePools[i].Port);

		fscanf(config, "%d\n", &NoDevFeePoolCount);

		for (int i = 0; i < NoDevFeePoolCount; ++i)
			fscanf(config, "%s %hu\n", NoDevFeePools[i].Name, &NoDevFeePools[i].Port);

		fscanf(config, "%d %d %d\n", &LogLevel, &ShowConsole, &OutputDelay);

		fclose(config);
	}
}

static void PrintConfig()
{
	Print(stdout, Log, "Main Wallet: %s\nNoDevFee Wallet: %s\nNoDevFee Worker: %s\nNoDevFee Protocol: %d\n", MainWallet, NoDevFeeWallet, NoDevFeeWorker, NoDevFeeProtocol);

	Print(stdout, Log, "Main Pool Count: %d (%d)\n", MainPoolCount, MainHostCount);

	for (int i = 0; i < MainHostCount; ++i)
	{
		unsigned char *addr = (unsigned char*) &MainHosts[i].Address;

		Print(stdout, Log, "[%d] %s %u.%u.%u.%u:%u\n", i, MainHosts[i].Name, addr[0], addr[1], addr[2], addr[3], BYTESWAP16(MainHosts[i].Port));
	}

	Print(stdout, Log, "DevFee Pool Count: %d (%d)\n", DevFeePoolCount, DevFeeHostCount);

	for (int i = 0; i < DevFeeHostCount; ++i)
	{
		unsigned char *addr = (unsigned char*) &DevFeeHosts[i].Address;

		Print(stdout, Log, "[%d] %s %u.%u.%u.%u:%u\n", i, DevFeeHosts[i].Name, addr[0], addr[1], addr[2], addr[3], BYTESWAP16(DevFeeHosts[i].Port));
	}

	Print(stdout, Log, "NoDevFee Pool Count: %d (%d)\n", NoDevFeePoolCount, NoDevFeeHostCount);

	for (int i = 0; i < NoDevFeeHostCount; ++i)
	{
		unsigned char *addr = (unsigned char*) &NoDevFeeHosts[i].Address;

		Print(stdout, Log, "[%d] %s %u.%u.%u.%u:%u\n", i, NoDevFeeHosts[i].Name, addr[0], addr[1], addr[2], addr[3], BYTESWAP16(NoDevFeeHosts[i].Port));
	}

	Print(stdout, Log, "Log Level: %d\nShow Console : %d\nOutput Delay : %d\n\n", LogLevel, ShowConsole, OutputDelay);
}

static void PrintVersion(const char *version)
{
	Print(stdout, Log, "-------------------------------------------------------------------------------\n");
	Print(stdout, Log, "                                  %s\n", version);
	Print(stdout, Log, "-------------------------------------------------------------------------------\n");

	PrintTime(stdout, Log);
	Print(stdout, Log, "\n");
}

static int SetPorts(const Host *hosts, int hostCount, unsigned short int *ports, int portCount)
{
	for (int i = 0; i < hostCount; ++i)
	{
		bool match = false;

		for (int j = 0; j < portCount; ++j)
		{
			if (ports[j] == hosts[i].Port)
			{
				match = true;

				break;
			}
		}

		if (!match)
			ports[portCount++] = hosts[i].Port;
	}

	return portCount;
}

static void SetFilter(char *filter)
{
	unsigned short int ports[256];

	int portCount = 0;

	portCount = SetPorts(MainHosts, MainHostCount, ports, portCount);
	portCount = SetPorts(DevFeeHosts, DevFeeHostCount, ports, portCount);
	portCount = SetPorts(NoDevFeeHosts, NoDevFeeHostCount, ports, portCount);

	if (LogLevel >= 3)
	{
		Print(0, Log, "Port Count: %d\n", portCount);

		for (int i = 0; i < portCount; ++i)
			Print(0, Log, "%u\n", BYTESWAP16(ports[i]));
	}

	strcpy(filter, "ip && tcp");

	if (portCount > 0)
	{
		strcat(filter, " && (inbound ? (");

		for (int i = 0; i < portCount; ++i)
			sprintf(filter + strlen(filter), "tcp.SrcPort == %u%s", BYTESWAP16(ports[i]), (i == portCount - 1) ? ") : (" : " || ");

		for (int i = 0; i < portCount; ++i)
			sprintf(filter + strlen(filter), "tcp.DstPort == %u%s", BYTESWAP16(ports[i]), (i == portCount - 1) ? "))" : " || ");
	}

	if (LogLevel >= 3)
		Print(0, Log, "\n%s\n\n", filter);
}

static void SetProtocol(int index, const char *name)
{
	strcpy(Protocols[index].Name, name);

	Protocols[index].Length = strlen(name);

	CreateOccTable((unsigned char*) Protocols[index].Name, Protocols[index].Length, Protocols[index].Occ);
}

void CloseLog()
{
	if (Log)
	{
		fflush(Log);

		fclose(Log);

		Log = 0;
	}
}

int __stdcall ConsoleHandler(unsigned long int type)
{
	CloseLog();

	return 1;
}

unsigned long int __stdcall FlushThread(void *parameter)
{
	while (true)
	{
		fflush(Log);

		Sleep(OutputDelay * 1000);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	atexit(CloseLog);

	ReadConfig("config.txt");
		
	if (!ShowConsole)
		ShowWindow(GetConsoleWindow(), SW_HIDE);

	if (LogLevel >= 2)
	{
		Log = fopen("log.txt", "a");

		CreateThread(0, 0, FlushThread, 0, 0, 0);
	}

	if (LogLevel >= 1)
		PrintVersion("divert 3.6");

	if (!SetConsoleCtrlHandler(ConsoleHandler, 1))
		if (LogLevel >= 1)
			Print(stdout, Log, "Error SetConsoleCtrlHandler %d\n", GetLastError());

	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		Print(stdout, Log, "Error WSAStartup %d\n", WSAGetLastError());

		system("pause");

		return EXIT_FAILURE;
	}

	MainHostCount = SetHosts(MainPools, MainPoolCount, MainHosts, 0);
	DevFeeHostCount = SetHosts(DevFeePools, DevFeePoolCount, DevFeeHosts, 0);
	NoDevFeeHostCount = SetHosts(NoDevFeePools, NoDevFeePoolCount, NoDevFeeHosts, 0);

	WSACleanup();

	if (LogLevel >= 1)
		PrintConfig();

	char filter[50000];

	SetFilter(filter);

	SetProtocol(0, "eth_submitLogin");
	SetProtocol(1, "eth_Login");
	SetProtocol(ProtocolCount, "0x");

	HANDLE handle = WinDivertOpen(filter, WINDIVERT_LAYER_NETWORK, -1000, 0);

	if (handle == INVALID_HANDLE_VALUE)
	{
		Print(stdout, Log, "Error WinDivertOpen %d\n", GetLastError());

		system("pause");

		return EXIT_FAILURE;
	}

	if (!WinDivertSetParam(handle, WINDIVERT_PARAM_QUEUE_LEN, DIVERT_QUEUE_LEN_MAX))
		if (LogLevel >= 1)
			Print(stdout, Log, "Error WinDivertSetParam WINDIVERT_PARAM_QUEUE_LEN %d\n", GetLastError());

	if (!WinDivertSetParam(handle, WINDIVERT_PARAM_QUEUE_TIME, DIVERT_QUEUE_TIME_MAX))
		if (LogLevel >= 1)
			Print(stdout, Log, "Error WinDivertSetParam WINDIVERT_PARAM_QUEUE_TIME %d\n", GetLastError());

	WINDIVERT_ADDRESS address;

	unsigned char packet[DIVERT_PACKET_SIZE_MAX];

	unsigned int packetLength;

	WINDIVERT_IPHDR *ipHeader;
	WINDIVERT_TCPHDR *tcpHeader;

	char *data;

	unsigned int dataLength;

	unsigned long int srcAddress = 0, dstAddress = 0;
	unsigned short int srcPort = 0, dstPort = 0;

	char devFeeWallet[256] = {0};

	int index = 0;

	Host NoDevFeeHost;

	memcpy(&NoDevFeeHost, (NoDevFeeHostCount > 0) ? &NoDevFeeHosts[0] : &MainHosts[0], sizeof(NoDevFeeHost));

	while (true)
	{
		if (!WinDivertRecv(handle, packet, sizeof(packet), &address, &packetLength))
		{
			if (LogLevel >= 1)
			{
				PrintTime(stdout, Log);
				Print(stdout, Log, " Error WinDivertRecv %d\n", GetLastError());
			}
		}

		WinDivertHelperParsePacket(packet, packetLength, &ipHeader, 0, 0, 0, &tcpHeader, 0, (void**) &data, &dataLength);

		bool modified = false;

		if ((ipHeader) && (tcpHeader))
		{
			if (address.Direction == WINDIVERT_DIRECTION_OUTBOUND)
			{
				bool main = false;

				if (NoDevFeeHostCount == 0)
				{
					for (int i = 0; i < MainHostCount; ++i)
					{
						if ((ipHeader->DstAddr == MainHosts[i].Address) && (tcpHeader->DstPort == MainHosts[i].Port))
						{
							if ((NoDevFeeHost.Address != MainHosts[i].Address) || (NoDevFeeHost.Port != MainHosts[i].Port))
							{
								memcpy(&NoDevFeeHost, &MainHosts[i], sizeof(NoDevFeeHost));

								index = i;
							}

							main = true;

							break;
						}
					}
				}

				if (!main)
				{
					for (int i = 0; i < DevFeeHostCount; ++i)
					{
						if ((ipHeader->DstAddr == DevFeeHosts[i].Address) && (tcpHeader->DstPort == DevFeeHosts[i].Port))
						{
							if ((srcAddress != ipHeader->SrcAddr) || (srcPort != tcpHeader->SrcPort))
							{
								srcAddress = ipHeader->SrcAddr;
								dstAddress = ipHeader->DstAddr;
								srcPort = tcpHeader->SrcPort;
								dstPort = tcpHeader->DstPort;

								unsigned char *devFeeAddr = (unsigned char*) &DevFeeHosts[i].Address;
								unsigned char *noDevFeeAddr = (unsigned char*) &NoDevFeeHost.Address;

								if (LogLevel >= 1)
								{
									PrintTime(stdout, Log);
									Print(stdout, Log, " connect\n[%d] %s %u.%u.%u.%u:%u\n[%d] %s %u.%u.%u.%u:%u\n\n",
										  i, DevFeeHosts[i].Name, devFeeAddr[0], devFeeAddr[1], devFeeAddr[2], devFeeAddr[3], BYTESWAP16(DevFeeHosts[i].Port),
										  index, NoDevFeeHost.Name, noDevFeeAddr[0], noDevFeeAddr[1], noDevFeeAddr[2], noDevFeeAddr[3], BYTESWAP16(NoDevFeeHost.Port));
								}
							}

							ipHeader->DstAddr = NoDevFeeHost.Address;
							tcpHeader->DstPort = NoDevFeeHost.Port;

							modified = true;

							break;
						}
					}
				}

				if ((data) && (dataLength > 0))
				{
					int protocol = -1;

					for (int i = 0; i < ProtocolCount; ++i)
					{
						if (SearchBMH((unsigned char*) data, dataLength, (unsigned char*) Protocols[i].Name, Protocols[i].Length, Protocols[i].Occ) != 0)
						{
							protocol = i;

							break;
						}
					}

					if (protocol != -1)
					{
						char *wallet = (char*) SearchBMH((unsigned char*) data, dataLength, (unsigned char*) Protocols[ProtocolCount].Name, Protocols[ProtocolCount].Length, Protocols[ProtocolCount].Occ);
					
						if (wallet != 0)
						{
							if ((strlen(MainWallet) < 42) || (strlen(NoDevFeeWallet) < 42))
							{
								memcpy(MainWallet, wallet, 42);
								memcpy(NoDevFeeWallet, wallet, 42);

								if (LogLevel >= 1)
									Print(stdout, Log, "Main Wallet: %s\nNoDevFee Wallet: %s\n\n", MainWallet, NoDevFeeWallet);
							}

							if (memcmp(wallet, MainWallet, 42) != 0)
							{
								memcpy(devFeeWallet, wallet, 42);
								memcpy(wallet, NoDevFeeWallet, 42);

								if (LogLevel >= 1)
								{
									PrintTime(stdout, Log);
									Print(stdout, Log, " %s\n%s\n%s\n\n", Protocols[protocol].Name, devFeeWallet, NoDevFeeWallet);
								}

								modified = true;
							}

							if (LogLevel >= 3)
								PrintPacket(0, Log, &address, ipHeader, tcpHeader, data, dataLength);
						}
					}
					else
					{
						if (LogLevel >= 4)
							PrintPacket(0, Log, &address, ipHeader, tcpHeader, data, dataLength);
					}
				}
			}
			else if (address.Direction == WINDIVERT_DIRECTION_INBOUND)
			{
				if ((ipHeader->SrcAddr == NoDevFeeHost.Address) && (ipHeader->DstAddr == srcAddress) && (tcpHeader->SrcPort == NoDevFeeHost.Port) && (tcpHeader->DstPort == srcPort))
				{
					ipHeader->SrcAddr = dstAddress;
					tcpHeader->SrcPort = dstPort;

					modified = true;
				}

				if ((data) && (dataLength > 0))
					if (LogLevel >= 4)
						PrintPacket(0, Log, &address, ipHeader, tcpHeader, data, dataLength);
			}
		}

		WinDivertHelperCalcChecksums(packet, packetLength, modified ? 0 : WINDIVERT_HELPER_NO_REPLACE);

		if (!WinDivertSend(handle, packet, packetLength, &address, 0))
		{
			if (LogLevel >= 1)
			{
				PrintTime(stdout, Log);
				Print(stdout, Log, " Error WinDivertSend %d\n", GetLastError());
			}
		}
	}

	return EXIT_SUCCESS;
}