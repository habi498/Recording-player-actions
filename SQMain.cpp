#include "SQMain.h"
#include "SQConsts.h"
#include "SQFuncs.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <map>
#include <fstream>
#include <ctime>
std::map<std::string, int> mapOfWords;
PluginFuncs* VCMP;
HSQUIRRELVM v;
HSQAPI sq;
extern int id;
bool recording = false;
int tempid;
int portToRecord = 0;
FILE* write_ptr;
#pragma comment(lib,"ws2_32.lib")
DWORD WINAPI replay(LPVOID lpParameter)//called by player
{
	int playerid = *(int*)lpParameter;
	FILE* ptr;
	unsigned char buffer[6];
	char name[30];
	VCMP->GetPlayerName(playerid, name, 30);
	ptr = fopen(name, "rb");  // r for read, b for binary
	if (ptr == NULL)
	{
		VCMP->SendClientMessage(playerid, 0xFFFFFF,
			"No recordings to play.");
		printf("no recordings");
		return 0;
	}
	unsigned long tick0 = NULL;
	int actorid = -1;
	for (int k = 0; k < id; k++)
	{
		if (IsActorAvailable(k))
		{
			BuyActor(k);
			actorid = k;
			break;
		}
	}
	if (actorid == -1)
	{
		int skinId = VCMP->GetPlayerSkin(playerid);
		CreateActorForPlay(skinId, name, playerid);
		//VCMP->SendClientMessage(playerid, 0x0,
		//"loading....");
		fclose(ptr);
		return 0;
	}
	VCMP->SendClientMessage(playerid, 0xFFFFFF, "Playing started..");
	while (true)
	{
		fread(buffer, sizeof(buffer), 1, ptr);
		unsigned long int tick =
			buffer[0] * 65536 * 256 +
			buffer[1] * 65536 +
			buffer[2] * 256 +
			buffer[3];
		if (tick0 == NULL)tick0 = tick;
		int plen = buffer[4] * 256 + buffer[5];
		if (plen == 0)break;
		unsigned char* packet;
		packet = new unsigned char[plen];
		// p len means packet len
		fread(packet, plen, 1, ptr);

		Sleep(tick - tick0);
		tick0 = tick;
		if (IsActorDisconnected(actorid))
		{
			printf("Interupted playing.\n");
			VCMP->SendClientMessage(playerid, 0xFFFFFF,
				"Playing interrupted.");
			delete[] packet;
			packet = NULL;
			return 0;
		}
		SendPacket(actorid, packet, plen);
		delete[] packet;
		packet = NULL;
	}
	FreeActor(actorid);
	VCMP->SendClientMessage(playerid, 0xFFFFFF,
		"Finished playing");
	fclose(ptr);
	return 1;
}

uint8_t onPlayerCommand(int32_t playerid, const char* message)
{
	//printf("Player with id %d, command %s", playerid,message);
	const char* cmd = "rec"; bool flag = false;

	for (int j = 0; j < 3; j++)
	{
		if (message[j] != cmd[j])flag = true;
	}
	if (strlen(message) != 3)flag = true;
	if (flag == false) //cmd ==record
	{
		//printf("Player with id %d has typed cmd /record", playerid);
		//caution: player name should not be changed in game
		char name[30];
		VCMP->GetPlayerName(playerid, name, 30);
		if (recording == true)
		{
			if (portToRecord == mapOfWords[name])//ofcourse name must exist in mapofwords
				VCMP->SendClientMessage(playerid, 0xFFFFFF,
					"You are already recording! Type /x");
			else
				VCMP->SendClientMessage(playerid, 0xFFFFFF,
					"You cannot record now");
		}
		else
		{
			recording = true;
			portToRecord = mapOfWords[name];
			VCMP->SendClientMessage(playerid, 0xFFFFFF,
				"Recording started. /x to stop");

			write_ptr = fopen(name, "wb");
		}
	}
	else
	{
		const char* cmd2 = "x"; bool flag2 = false;
		for (int j = 0; j < 1; j++)
		{
			if (cmd2[j] != message[j])flag2 = true;
		}
		if (strlen(message) != 1)flag2 = true;
		if (flag2 == false)//cmd==stoprecord
		{
			if (recording == false)
			{
				VCMP->SendClientMessage(playerid, 0xFFFFFF
					, "No recording in progress. Type /rec to start one");
			}
			else
			{
				char name[30];
				VCMP->GetPlayerName(playerid, name, 30);
				if (mapOfWords[name] != portToRecord)
				{
					VCMP->SendClientMessage(playerid, 0xFFFFFF,
						"The player who is recording is not you");
				}
				else
				{
					VCMP->SendClientMessage(playerid, 0xFFFFFF,
						"Recording saved. You can use /play");
					recording = false;
					portToRecord = 0;
					//printf("Get tick count is %lu\n", GetTickCount());
					unsigned long int tick = GetTickCount();
					unsigned char tick1 = tick % 256;
					unsigned char tick2 = (tick / 256) % 256;
					unsigned char tick3 = (tick / 65536) % 256;
					unsigned char tick4 = (tick / (65536 * 256)) % 256;
					unsigned char endbuf[6];
					endbuf[0] = tick4;
					endbuf[1] = tick3;
					endbuf[2] = tick2;
					endbuf[3] = tick1;
					endbuf[4] = 0;
					endbuf[5] = 0;
					fwrite(endbuf, 6, 1, write_ptr);
					fclose(write_ptr);
					printf("Recording Saved to file.\n");
				}
			}
		}
		else
		{
			const char* cmd3 = "play"; bool flag3 = false;
			for (int j = 0; j < 4; j++)
			{
				if (cmd3[j] != message[j])flag3 = true;
			}
			if (strlen(message) != 4)flag3 = true;
			if (flag3 == false)//cmd==play
			{
				DWORD myThreadID;
				tempid = playerid;
				HANDLE myHandle = CreateThread(0, 0, replay, &tempid, 0, &myThreadID);
			}
			else 
			{
				if (strlen(message) == 1 && message[0] == 'c')
				{
					printf("/c was called");
				}
			}
		}
	}
	return 1;
}
bool Detour64(char* src, char* dst, const intptr_t len)
{
	if (len < 12) return false;

	DWORD  curProtection;
	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &curProtection);

	intptr_t  absoluteAddress = (intptr_t)(dst);

	*src = (char)'\x48';
	*(src + 1) = (char)'\xb8';

	*(src + 2) = absoluteAddress % 256;
	*(src + 3) = (absoluteAddress / 256) % 256;
	*(src + 4) = (absoluteAddress / 65536) % 256;
	*(src + 5) = (absoluteAddress / (65536 * 256)) % 256;
	*(src + 6) = (absoluteAddress / ((long long)65536 * 65536)) % 256;
	*(src + 7) = (absoluteAddress / ((long long)65536 * 65536 * 256)) % 256;
	*(src + 8) = (absoluteAddress / ((long long)65536 * 65536 * 65536)) % 256;
	*(src + 9) = (absoluteAddress / ((long long)65536 * 65536 * 65536 * 256)) % 256;

	*(src + 10) = (char)'\xff';
	*(src + 11) = (char)'\xe0';
	for (int i = 12; i < len; i++)
		*(src + i) = (char)'\x90';

	VirtualProtect(src, len, curProtection, &curProtection);
	return true;
}

char* TrampHook64(char* src, char* dst, const intptr_t len)
{
	// Make sure the length is greater than 5
	if (len < 12) return 0;

	// Create the ga+teway (len + 5 for the overwritten bytes + the jmp)
	void* gateway = VirtualAlloc(0, len + 12, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	//Write the stolen bytes into the gateway
	memcpy(gateway, src, len);

	// Get the gateway to destination addy
	intptr_t  gatewayAbsoluteAddr = (intptr_t)src + len;

	// Add the jmp opcode to the end of the gateway
	*((char*)gateway + len) = 0x48;

	*((char*)gateway + len + 1) = 0xb8;

	// Add the address to the jmp
	*((char*)gateway + len + 2) = gatewayAbsoluteAddr % 256;

	*((char*)gateway + len + 3) = (gatewayAbsoluteAddr / 256) % 256;

	*((char*)gateway + len + 4) = (gatewayAbsoluteAddr / 65536) % 256;

	*((char*)gateway + len + 5) = (gatewayAbsoluteAddr / (65536 * 256)) % 256;

	*((char*)gateway + len + 6) = (gatewayAbsoluteAddr / ((long long)65536 * 65536)) % 256;

	*((char*)gateway + len + 7) = (gatewayAbsoluteAddr / ((long long)65536 * 65536 * 256)) % 256;

	*((char*)gateway + len + 8) = (gatewayAbsoluteAddr / ((long long)65536 * 65536 * 65536)) % 256;
	*((char*)gateway + len + 9) = (gatewayAbsoluteAddr / ((long long)65536 * 65536 * 65536 * 256)) % 256;


	*((char*)gateway + len + 10) = 0xff;
	*((char*)gateway + len + 11) = 0xe0;

	// Perform the detour
	Detour64(src, dst, len);
	printf("hooked\n");
	return (char*)gateway;
}
typedef int(__stdcall* trecvfrom)(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen);
//typedef int(__stdcall* tMessageBoxA)(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);
trecvfrom orecvfrom = nullptr;

int __stdcall hkrecvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
{
	int iResult = orecvfrom(s, buf, len, flags, from, fromlen);

	if (iResult > 0)
	{
		int sport =

			(unsigned char)(from->sa_data[0]) * 256 +

			(unsigned char)(from->sa_data[1]);

		unsigned char* packet = (unsigned char*)buf;

		if (packet[0] == 132)
		{

			unsigned char* message = packet + 4;

			char t = countMessage(message, iResult - 4);

			for (int i = 0; i < t; i++)
			{
				if (message[0] == 64 &&

					message[6] == 152)//0x98 joining packet
				{

					char name[30];

					unsigned char namelen = message[15];

					for (int j = 0; j < namelen; j++)
					{
						name[j] = message[16 + j];
					}

					name[namelen] = 0;

					if (mapOfWords.count(name) == 0)
					{

						mapOfWords[name] = sport;

						//printf("Player %s connected, port is %d\n", name
						//	, mapOfWords[name]);

					}
				}

				message = message + FirstMessageLength(message);

			}
		}
		if (recording == true && portToRecord == sport)
		{
			unsigned char* buffer;
			buffer = new unsigned char[(unsigned long long)4 + 2 + iResult];
			int tick = GetTickCount();
			unsigned char tick1 = tick % 256;
			unsigned char tick2 = (tick / 256) % 256;
			unsigned char tick3 = (tick / 65536) % 256;
			unsigned char tick4 = (tick / (65536 * 256)) % 256;
			buffer[0] = tick4;
			buffer[1] = tick3;
			buffer[2] = tick2;  buffer[3] = tick1;
			unsigned char lenOne = iResult / 256;
			unsigned char lenTwo = iResult % 256;
			buffer[4] = lenOne;
			buffer[5] = lenTwo;
			for (int k = 0; k < iResult; k++)
				buffer[6 + k] = packet[k];
			fwrite(buffer, (unsigned long long)4 + 2 + iResult, 1, write_ptr);
			char test = 1;
			delete[] buffer;
			buffer = NULL;
		}
	}
	return iResult;
}
void onPlayerDisconnect(int32_t playerid, vcmpDisconnectReason reason)
{
	char name[30];

	VCMP->GetPlayerName(playerid, name, 30); 

	mapOfWords.erase(name);
	
	if (DisconnectActor(name,strlen(name)))
	{
		printf("One actor disconnected.\n");
	}
}


// Attaching plugin's load to the squirrel VM
void OnSquirrelScriptLoad() {
	// See if we have any imports from Squirrel
	size_t size;
	int32_t sqId      = VCMP->FindPlugin(const_cast<char*>("SQHost2"));
	const void ** sqExports = VCMP->GetPluginExports(sqId, &size);

	// We do!
	if (sqExports != NULL && size > 0) {
		// Cast to a SquirrelImports structure
		SquirrelImports ** sqDerefFuncs = (SquirrelImports **)sqExports;
		
		// Now let's change that to a SquirrelImports pointer
		SquirrelImports * sqFuncs       = (SquirrelImports *)(*sqDerefFuncs);
		
		// Now we get the virtual machine
		if (sqFuncs) {
			// Get a pointer to the VM and API
			v = *(sqFuncs->GetSquirrelVM());
			sq = *(sqFuncs->GetSquirrelAPI());

			// Register functions
			RegisterFuncs(v);
			
			// Register constants
			RegisterConsts(v);
		}
	}
	else
		OutputError("Failed to attach to SQHost2.");
}

// Called when server is loading the plugin
uint8_t OnServerInitialise() {
	OutputMessage("Loaded actorplus module");
	return 1;
}

// Called when the server is unloading the plugin
void OnServerShutdown() {
   
}

// Called when the server is loading the Squirrel plugin
uint8_t OnPluginCommand(uint32_t type, const char* text) {
	switch (type) {
		case 0x7D6E22D8:
			OnSquirrelScriptLoad();
			break;
		default:
			break;
	}

	return 1;
}

extern "C" unsigned int VcmpPluginInit(PluginFuncs* pluginFuncs, PluginCallbacks* pluginCalls, PluginInfo* pluginInfo) {
	VCMP = pluginFuncs;

	// Plugin information
	pluginInfo->pluginVersion = 0x110;
	pluginInfo->apiMajorVersion = PLUGIN_API_MAJOR;
	pluginInfo->apiMinorVersion = PLUGIN_API_MINOR;
	strcpy(pluginInfo->name, "actorplus");

	pluginCalls->OnServerInitialise = OnServerInitialise;
	pluginCalls->OnServerShutdown = OnServerShutdown;
	pluginCalls->OnPluginCommand = OnPluginCommand;
	pluginCalls->OnPlayerCommand = onPlayerCommand;
	pluginCalls->OnPlayerDisconnect = onPlayerDisconnect;
	// Perform hooking

	HINSTANCE libr = LoadLibrary("ws2_32.dll");

	orecvfrom = (trecvfrom)GetProcAddress(libr, "recvfrom");

	orecvfrom = (trecvfrom)TrampHook64((char*)orecvfrom, (char*)hkrecvfrom, 15);


return 1;
}

