//
// SQFuncs: These are the tools to register custom functions within the Squirrel VM.
//
//	Written for Liberty Unleashed by the Liberty Unleashed Team.
//

#pragma once
#ifndef _SQFUNCS_H
#define _SQFUNCS_H

#include "SQMain.h"
#include "squirrel.h"

#define _SQUIRRELDEF(x) SQInteger x(HSQUIRRELVM v)

#ifdef __cplusplus
extern "C" {
#endif
	SQInteger				RegisterSquirrelFunc				(HSQUIRRELVM v, SQFUNCTION f, const SQChar* fname, unsigned char uiParams, const SQChar* szParams);
	void					RegisterFuncs						(HSQUIRRELVM v);
	int FirstMessageLength(unsigned char* message);
	int countMessage(unsigned char* rmsg, int len);
	void SendPacket(int actorid, unsigned char* packet, int plen);
	void BuyActor(int aid);
	bool IsActorAvailable(int actorid);
	void FreeActor(int aid);
	void CreateActorForPlay(int skinId, char* fname,int playerid);
	bool DisconnectActor(char* name, int len);
	bool IsActorDisconnected(int aid);
#ifdef __cplusplus
}
#endif

#endif
