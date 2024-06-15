#pragma comment(lib, "Ws2_32.lib")
#include "SteamAPIWrapper.h"
#define STEAM_API_NODLL
#include "steam/steam_api.h"
#include "Aurie/shared.hpp"
#include "YYToolkit/Shared.hpp"
#include <Windows.h>
using namespace Aurie;
using namespace YYTK;

extern YYTKInterface* g_ModuleInterface;

typedef bool (*LPFNDLL_SteamAPI_Init)();
typedef void (*LPFNDLL_SteamAPI_RunCallbacks)();
typedef HSteamUser (*LPFNDLL_SteamAPI_GetHSteamUser)();
typedef void* (*LPFNDLL_SteamInternal_ContextInit)(void* pContextInitData);
typedef void* (*LPFNDLL_SteamInternal_FindOrCreateUserInterface)(HSteamUser hSteamUser, const char* pszVersion);
typedef void* (*LPFNDLL_SteamInternal_FindOrCreateGameServerInterface)(HSteamUser hSteamUser, const char* pszVersion);
typedef void (*LPFNDLL_SteamAPI_RegisterCallback)(class CCallbackBase* pCallback, int iCallback);
typedef void (*LPFNDLL_SteamAPI_UnregisterCallback)(class CCallbackBase* pCallback);
typedef void (*LPFNDLL_SteamAPI_RegisterCallResult)(class CCallbackBase* pCallback, SteamAPICall_t hAPICall);
typedef void (*LPFNDLL_SteamAPI_UnregisterCallResult)(class CCallbackBase* pCallback, SteamAPICall_t hAPICall);

HINSTANCE hinstSteamAPILib;
bool isSteamInitialized = false;
LPFNDLL_SteamAPI_Init lpfnDll_SteamAPI_Init = nullptr;
LPFNDLL_SteamAPI_RunCallbacks lpfnDll_SteamAPI_RunCallbacks = nullptr;
LPFNDLL_SteamAPI_GetHSteamUser lpfnDll_SteamAPI_GetHSteamUser = nullptr;
LPFNDLL_SteamInternal_ContextInit lpfnDll_SteamInternal_ContextInit = nullptr;
LPFNDLL_SteamInternal_FindOrCreateUserInterface lpfnDll_SteamInternal_FindOrCreateUserInterface = nullptr;
LPFNDLL_SteamInternal_FindOrCreateGameServerInterface lpfnDll_SteamInternal_FindOrCreateGameServerInterface = nullptr;
LPFNDLL_SteamAPI_RegisterCallback lpfnDll_SteamAPI_RegisterCallback = nullptr;
LPFNDLL_SteamAPI_UnregisterCallback lpfnDll_SteamAPI_UnregisterCallback = nullptr;
LPFNDLL_SteamAPI_RegisterCallResult lpfnDll_SteamAPI_RegisterCallResult = nullptr;
LPFNDLL_SteamAPI_UnregisterCallResult lpfnDll_SteamAPI_UnregisterCallResult = nullptr;

bool SteamAPI_Init()
{
	if (lpfnDll_SteamAPI_Init == nullptr)
	{
		g_ModuleInterface->Print(CM_RED, "lpfnDll_SteamAPI_Init is nullptr");
		return false;
	}
	return lpfnDll_SteamAPI_Init();
}

void SteamAPI_RunCallbacks()
{
	if (lpfnDll_SteamAPI_RunCallbacks == nullptr)
	{
		g_ModuleInterface->Print(CM_RED, "lpfnDll_SteamAPI_RunCallbacks is nullptr");
		return;
	}
	lpfnDll_SteamAPI_RunCallbacks();
}

HSteamUser SteamAPI_GetHSteamUser()
{
	if (lpfnDll_SteamAPI_GetHSteamUser == nullptr)
	{
		g_ModuleInterface->Print(CM_RED, "lpfnDll_SteamAPI_GetHSteamUser is nullptr");
		return 0;
	}
	return lpfnDll_SteamAPI_GetHSteamUser();
}

void* SteamInternal_ContextInit(void* pContextInitData)
{
	if (lpfnDll_SteamInternal_ContextInit == nullptr)
	{
		g_ModuleInterface->Print(CM_RED, "lpfnDll_SteamInternal_ContextInit is nullptr");
		return nullptr;
	}
	return lpfnDll_SteamInternal_ContextInit(pContextInitData);
}

void* SteamInternal_FindOrCreateUserInterface(HSteamUser hSteamUser, const char* pszVersion)
{
	if (lpfnDll_SteamInternal_FindOrCreateUserInterface == nullptr)
	{
		g_ModuleInterface->Print(CM_RED, "lpfnDll_SteamInternal_FindOrCreateUserInterface is nullptr");
		return nullptr;
	}
	return lpfnDll_SteamInternal_FindOrCreateUserInterface(hSteamUser, pszVersion);
}

void* SteamInternal_FindOrCreateGameServerInterface(HSteamUser hSteamUser, const char* pszVersion)
{
	if (lpfnDll_SteamInternal_FindOrCreateGameServerInterface == nullptr)
	{
		g_ModuleInterface->Print(CM_RED, "lpfnDll_SteamInternal_FindOrCreateGameServerInterface is nullptr");
		return nullptr;
	}
	return lpfnDll_SteamInternal_FindOrCreateGameServerInterface(hSteamUser, pszVersion);
}

void SteamAPI_RegisterCallback(class CCallbackBase* pCallback, int iCallback)
{
	if (lpfnDll_SteamAPI_RegisterCallback == nullptr)
	{
		g_ModuleInterface->Print(CM_RED, "lpfnDll_SteamAPI_RegisterCallback is nullptr");
		return;
	}
	lpfnDll_SteamAPI_RegisterCallback(pCallback, iCallback);
}

void SteamAPI_UnregisterCallback(class CCallbackBase* pCallback)
{
	if (lpfnDll_SteamAPI_UnregisterCallback == nullptr)
	{
		g_ModuleInterface->Print(CM_RED, "lpfnDll_SteamAPI_UnregisterCallback is nullptr");
		return;
	}
	lpfnDll_SteamAPI_UnregisterCallback(pCallback);
}

void SteamAPI_RegisterCallResult(class CCallbackBase* pCallback, SteamAPICall_t hAPICall)
{
	if (lpfnDll_SteamAPI_RegisterCallResult == nullptr)
	{
		g_ModuleInterface->Print(CM_RED, "lpfnDll_SteamAPI_RegisterCallResult is nullptr");
		return;
	}
	lpfnDll_SteamAPI_RegisterCallResult(pCallback, hAPICall);
}

void SteamAPI_UnregisterCallResult(class CCallbackBase* pCallback, SteamAPICall_t hAPICall)
{
	if (lpfnDll_SteamAPI_UnregisterCallResult == nullptr)
	{
		g_ModuleInterface->Print(CM_RED, "lpfnDll_SteamAPI_UnregisterCallResult is nullptr");
		return;
	}
	return lpfnDll_SteamAPI_UnregisterCallResult(pCallback, hAPICall);
}

void initSteamAPIWrapperFuncs()
{
	// TODO: Need to use LoadLibrary in order to dynamically load the dll to prevent it crashing for non steam installs
	hinstSteamAPILib = LoadLibrary(TEXT("steam_api64.dll"));
	if (isSteamInitialized = (hinstSteamAPILib != NULL))
	{
		lpfnDll_SteamAPI_Init = reinterpret_cast<LPFNDLL_SteamAPI_Init>(GetProcAddress(hinstSteamAPILib, "SteamAPI_Init"));
		if (lpfnDll_SteamAPI_Init == NULL)
		{
			g_ModuleInterface->Print(CM_RED, "Couldn't load SteamAPI_Init from steam_api64.dll");
			isSteamInitialized = false;
		}

		lpfnDll_SteamAPI_RunCallbacks = reinterpret_cast<LPFNDLL_SteamAPI_RunCallbacks>(GetProcAddress(hinstSteamAPILib, "SteamAPI_RunCallbacks"));
		if (lpfnDll_SteamAPI_RunCallbacks == NULL)
		{
			g_ModuleInterface->Print(CM_RED, "Couldn't load SteamAPI_RunCallbacks from steam_api64.dll");
			isSteamInitialized = false;
		}

		lpfnDll_SteamAPI_GetHSteamUser = reinterpret_cast<LPFNDLL_SteamAPI_GetHSteamUser>(GetProcAddress(hinstSteamAPILib, "SteamAPI_GetHSteamUser"));
		if (lpfnDll_SteamAPI_GetHSteamUser == NULL)
		{
			g_ModuleInterface->Print(CM_RED, "Couldn't load SteamAPI_GetHSteamUser from steam_api64.dll");
			isSteamInitialized = false;
		}

		lpfnDll_SteamInternal_ContextInit = reinterpret_cast<LPFNDLL_SteamInternal_ContextInit>(GetProcAddress(hinstSteamAPILib, "SteamInternal_ContextInit"));
		if (lpfnDll_SteamInternal_ContextInit == NULL)
		{
			g_ModuleInterface->Print(CM_RED, "Couldn't load SteamInternal_ContextInit from steam_api64.dll");
			isSteamInitialized = false;
		}

		lpfnDll_SteamInternal_FindOrCreateUserInterface = reinterpret_cast<LPFNDLL_SteamInternal_FindOrCreateUserInterface>(GetProcAddress(hinstSteamAPILib, "SteamInternal_FindOrCreateUserInterface"));
		if (lpfnDll_SteamInternal_FindOrCreateUserInterface == NULL)
		{
			g_ModuleInterface->Print(CM_RED, "Couldn't load SteamInternal_FindOrCreateUserInterface from steam_api64.dll");
			isSteamInitialized = false;
		}

		lpfnDll_SteamInternal_FindOrCreateGameServerInterface = reinterpret_cast<LPFNDLL_SteamInternal_FindOrCreateGameServerInterface>(GetProcAddress(hinstSteamAPILib, "SteamInternal_FindOrCreateGameServerInterface"));
		if (lpfnDll_SteamInternal_FindOrCreateGameServerInterface == NULL)
		{
			g_ModuleInterface->Print(CM_RED, "Couldn't load SteamInternal_FindOrCreateGameServerInterface from steam_api64.dll");
			isSteamInitialized = false;
		}

		lpfnDll_SteamAPI_RegisterCallback = reinterpret_cast<LPFNDLL_SteamAPI_RegisterCallback>(GetProcAddress(hinstSteamAPILib, "SteamAPI_RegisterCallback"));
		if (lpfnDll_SteamAPI_RegisterCallback == NULL)
		{
			g_ModuleInterface->Print(CM_RED, "Couldn't load SteamAPI_RegisterCallback from steam_api64.dll");
			isSteamInitialized = false;
		}

		lpfnDll_SteamAPI_UnregisterCallback = reinterpret_cast<LPFNDLL_SteamAPI_UnregisterCallback>(GetProcAddress(hinstSteamAPILib, "SteamAPI_UnregisterCallback"));
		if (lpfnDll_SteamAPI_UnregisterCallback == NULL)
		{
			g_ModuleInterface->Print(CM_RED, "Couldn't load SteamAPI_UnregisterCallback from steam_api64.dll");
			isSteamInitialized = false;
		}

		lpfnDll_SteamAPI_RegisterCallResult = reinterpret_cast<LPFNDLL_SteamAPI_RegisterCallResult>(GetProcAddress(hinstSteamAPILib, "SteamAPI_RegisterCallResult"));
		if (lpfnDll_SteamAPI_RegisterCallResult == NULL)
		{
			g_ModuleInterface->Print(CM_RED, "Couldn't load SteamAPI_RegisterCallResult from steam_api64.dll");
			isSteamInitialized = false;
		}

		lpfnDll_SteamAPI_UnregisterCallResult = reinterpret_cast<LPFNDLL_SteamAPI_UnregisterCallResult>(GetProcAddress(hinstSteamAPILib, "SteamAPI_UnregisterCallResult"));
		if (lpfnDll_SteamAPI_UnregisterCallResult == NULL)
		{
			g_ModuleInterface->Print(CM_RED, "Couldn't load SteamAPI_UnregisterCallResult from steam_api64.dll");
			isSteamInitialized = false;
		}
	}
	else
	{
		g_ModuleInterface->Print(CM_RED, "Couldn't load Steam API dll");
	}
}