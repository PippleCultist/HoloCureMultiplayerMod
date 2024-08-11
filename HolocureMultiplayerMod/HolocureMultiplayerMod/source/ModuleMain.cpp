#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#include <WS2tcpip.h>
#include <Aurie/shared.hpp>
#include <YYToolkit/Shared.hpp>
#include "CodeEvents.h"
#include "ModuleMain.h"
#include "ScriptFunctions.h"
#include "NetworkFunctions.h"
#include "BuiltinFunctions.h"
#include "SteamLobbyBrowser.h"
#include "CommonFunctions.h"
#include "SteamAPIWrapper.h"
#include "Button.h"
#include <iphlpapi.h>
#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <semaphore>
#include "steam/steam_api.h"
using namespace Aurie;
using namespace YYTK;

extern bool hasJoinedSteamLobby;
extern bool isSteamInitialized;

RValue GMLVarIndexMapGMLHash[1001];

TRoutine origStructGetFromHashFunc;
TRoutine origStructSetFromHashFunc;
TRoutine origSpriteDeleteScript = nullptr;

CallbackManagerInterface* callbackManagerInterfacePtr = nullptr;
YYRunnerInterface g_RunnerInterface;
YYTKInterface* g_ModuleInterface = nullptr;
PFUNC_YYGMLScript origInputBindingSetScript = nullptr;
PFUNC_YYGMLScript origInputCheckScript = nullptr;
PFUNC_YYGMLScript origInputSourceModeSetScript = nullptr;
PFUNC_YYGMLScript origInputSourceClearScript = nullptr;
PFUNC_YYGMLScript origInputJoinParamsSetScript = nullptr;
PFUNC_YYGMLScript origSnapshotPrebuffStatsPlayerCreateScript = nullptr;
PFUNC_YYGMLScript origUpdatePlayerPlayerManagerOtherScript = nullptr;
PFUNC_YYGMLScript origAddAttackPlayerManagerOtherScript = nullptr;
PFUNC_YYGMLScript origInputDirectionScript = nullptr;
PFUNC_YYGMLScript origInputSourceUsingScript = nullptr;
PFUNC_YYGMLScript origInputGlobalScript = nullptr;
PFUNC_YYGMLScript origDrawTextOutlineScript = nullptr;
PFUNC_YYGMLScript origInputSourceIsAvailableScript = nullptr;
PFUNC_YYGMLScript origInputPlayerConnectedCountScript = nullptr;
PFUNC_YYGMLScript origInputSourceGetArrayScript = nullptr;
PFUNC_YYGMLScript origInputSourceDetectNewScript = nullptr;
PFUNC_YYGMLScript origInputGamepadIsConnectedScript = nullptr;
PFUNC_YYGMLScript origGeneratePossibleOptionsScript = nullptr;
PFUNC_YYGMLScript origOptionOneScript = nullptr;
PFUNC_YYGMLScript origOptionTwoScript = nullptr;
PFUNC_YYGMLScript origOptionThreeScript = nullptr;
PFUNC_YYGMLScript origOptionFourScript = nullptr;
PFUNC_YYGMLScript origUnpauseScript = nullptr;
PFUNC_YYGMLScript origAddPerkScript = nullptr;
PFUNC_YYGMLScript origAddItemScript = nullptr;
PFUNC_YYGMLScript origAddConsumableScript = nullptr;
PFUNC_YYGMLScript origAddStatScript = nullptr;
PFUNC_YYGMLScript origVariableStructCopyScript = nullptr;
PFUNC_YYGMLScript origEliminateAttackScript = nullptr;
PFUNC_YYGMLScript origRemoveItemScript = nullptr;
PFUNC_YYGMLScript origRemovePerkScript = nullptr;
PFUNC_YYGMLScript origExecuteSpecialAttackScript = nullptr;
PFUNC_YYGMLScript origDestroyHoloAnvilScript = nullptr;
PFUNC_YYGMLScript origDestroyGoldenAnvilScript = nullptr;
PFUNC_YYGMLScript origDestroyGoldenHammerScript = nullptr;
PFUNC_YYGMLScript origDestroyStickerScript = nullptr;
PFUNC_YYGMLScript origPauseScript = nullptr;
PFUNC_YYGMLScript origGetBoxScript = nullptr;
PFUNC_YYGMLScript origGetAnvilScript = nullptr;
PFUNC_YYGMLScript origGetGoldenAnvilScript = nullptr;
PFUNC_YYGMLScript origGetStickerScript = nullptr;
PFUNC_YYGMLScript origAddEnchantScript = nullptr;
PFUNC_YYGMLScript origAddCollabScript = nullptr;
PFUNC_YYGMLScript origAddSuperCollabScript = nullptr;
PFUNC_YYGMLScript origMouseOverButtonScript = nullptr;

using PFUNC_YYGML_Variable_GetValue = void(*)(int arg1, void* arg2, void* arg3, void* arg4, void* arg5, void* arg6);
PVOID yyGMLVariableGetValueAddress = nullptr;
PFUNC_YYGML_Variable_GetValue origYYGMLVariableGetValueFunc = nullptr;
std::binary_semaphore initYYGMLVariableGetValueFuncSemaphore(1);

using PFUNC_YYGML_ErrCheck_Variable_GetValue = void(*)(int arg1, void* arg2, void* arg3, void* arg4);
PVOID yyGMLErrCheckVariableGetValueAddress = nullptr;
PFUNC_YYGML_ErrCheck_Variable_GetValue origYYGMLErrCheckVariableGetValueFunc = nullptr;
std::binary_semaphore initYYGMLErrCheckVariableGetValueFuncSemaphore(1);

using PFUNC_YYGML_Instance_Destroy = void(*)(CInstance* Self, CInstance* Other, int arg3, void* arg4);
PVOID yyGMLInstanceDestroyAddress = nullptr;
PFUNC_YYGML_Instance_Destroy origYYGMLInstanceDestroyFunc = nullptr;
std::binary_semaphore initYYGMLInstanceDestroyFuncSemaphore(1);

CInstance* globalInstance = nullptr;

SOCKET serverSocket = INVALID_SOCKET;
SOCKET connectClientSocket = INVALID_SOCKET;
std::unordered_map<uint32_t, SOCKET> clientSocketMap;
std::unordered_map<uint32_t, playerData> playerDataMap;
bool isHost = false;
bool hasConnected = false;
CSteamLobbyBrowser* steamLobbyBrowser = nullptr;

int objPlayerIndex = -1;
int objBaseMobIndex = -1;
int objAttackIndex = -1;
int objPickupableIndex = -1;
int objSummonIndex = -1;
int objAttackControllerIndex = -1;
int objPlayerManagerIndex = -1;
int objDestructableIndex = -1;
int objYagooPillarIndex = -1;
int objCautionIndex = -1;
int objCautionAttackIndex = -1;
int objPreCreateIndex = -1;
int objVFXIndex = -1;
int objAfterImageIndex = -1;
int objHoloBoxIndex = -1;
int objHoloAnvilIndex = -1;
int objGoldenAnvilIndex = -1;
int objGoldenHammerIndex = -1;
int objStickerIndex = -1;
int objItemLightBeamIndex = -1;
int objOptionsIndex = -1;
int objInputManagerIndex = -1;
int objCharacterDataIndex = -1;
int objCharSelectIndex = -1;
int objObstacleIndex = -1;
int objGetFishIndex = -1;
int sprEmptyIndex = -1;
int sprEmptyMaskIndex = -1;
int sprGameCursorIndex = -1;
int sprGameCursor2Index = -1;
int sprSummonPointerIndex = -1;
int sprHudInitButtonsIndex = -1;
int sprKaelaMinerals = -1;
int sprHudToggleButtonIndex = -1;
int sprHudOptionButtonIndex = -1;
int jpFont = -1;
int rmTitle = -1;
int rmCharSelect = -1;

char broadcastAddressBuffer[16] = { 0 };

std::string ConvertLPCWSTRToString(LPCWSTR lpcwszStr)
{
	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, lpcwszStr, static_cast<int>(wcslen(lpcwszStr)), NULL, 0, NULL, NULL);
	std::string resString(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, lpcwszStr, static_cast<int>(wcslen(lpcwszStr)), &resString[0], sizeNeeded, NULL, NULL);
	return resString;
}

AurieStatus FindMemoryPatternAddress(const unsigned char* Pattern, const char* PatternMask, PVOID& outMemoryAddress)
{
	AurieStatus status = AURIE_SUCCESS;
	std::wstring gameName;
	if (!AurieSuccess(status = MdGetImageFilename(g_ArInitialImage, gameName)))
	{
		return status;
	}

	// Scan for pattern
	size_t patternMatch = MmSigscanModule(
		gameName.c_str(),
		Pattern,
		PatternMask
	);
	if (!patternMatch)
	{
		g_ModuleInterface->Print(CM_RED, "Couldn't find pattern %s", Pattern);
		return AURIE_OBJECT_NOT_FOUND;
	}

	outMemoryAddress = reinterpret_cast<PVOID>(patternMatch);
	return AURIE_SUCCESS;
}

void YYGMLVariableGetValueHookFunc(int arg1, void* arg2, void* arg3, void* arg4, void* arg5, void* arg6)
{
	// Sometimes the hook might happen before the originalFunc is set
	if (origYYGMLVariableGetValueFunc == nullptr)
	{
		initYYGMLVariableGetValueFuncSemaphore.acquire();
		initYYGMLVariableGetValueFuncSemaphore.release();
		if (origYYGMLVariableGetValueFunc == nullptr)
		{
			// Shouldn't ever occur, but might as well check for it
			g_ModuleInterface->Print(CM_RED, "Still couldn't get the original function for YYGMLVariableGetValueHookFunc. Expect undefined behavior.");
			return;
		}
	}
	if (hasConnected)
	{
		if (arg1 == objPlayerIndex)
		{
			// Prevent it from swapping if the map hasn't been initialized yet
			auto playerMapFind = playerMap.find(curPlayerID);
			if (playerMapFind != playerMap.end())
			{
				// swap player to the actual current player
				arg1 = playerMap[curPlayerID].m_i32;
			}
		}
	}
	origYYGMLVariableGetValueFunc(arg1, arg2, arg3, arg4, arg5, arg6);
}

void YYGMLErrCheckVariableGetValueHookFunc(int arg1, void* arg2, void* arg3, void* arg4)
{
	// Sometimes the hook might happen before the originalFunc is set
	if (origYYGMLErrCheckVariableGetValueFunc == nullptr)
	{
		initYYGMLErrCheckVariableGetValueFuncSemaphore.acquire();
		initYYGMLErrCheckVariableGetValueFuncSemaphore.release();
		if (origYYGMLErrCheckVariableGetValueFunc == nullptr)
		{
			// Shouldn't ever occur, but might as well check for it
			g_ModuleInterface->Print(CM_RED, "Still couldn't get the original function for YYGMLErrCheckVariableGetValueFunc. Expect undefined behavior.");
			return;
		}
	}
	if (hasConnected)
	{
		if (arg1 == objPlayerIndex)
		{
			// Prevent it from swapping if the map hasn't been initialized yet
			auto playerMapFind = playerMap.find(curPlayerID);
			if (playerMapFind != playerMap.end())
			{
				// swap player to the actual current player
				arg1 = playerMap[curPlayerID].m_i32;
			}
		}
	}
	origYYGMLErrCheckVariableGetValueFunc(arg1, arg2, arg3, arg4);
}

void YYGMLInstanceDestroyHookFunc(CInstance* Self, CInstance* Other, int arg3, void* arg4)
{
	// Sometimes the hook might happen before the originalFunc is set
	if (origYYGMLInstanceDestroyFunc == nullptr)
	{
		initYYGMLInstanceDestroyFuncSemaphore.acquire();
		initYYGMLInstanceDestroyFuncSemaphore.release();
		if (origYYGMLInstanceDestroyFunc == nullptr)
		{
			// Shouldn't ever occur, but might as well check for it
			g_ModuleInterface->Print(CM_RED, "Still couldn't get the original function for YYGMLInstanceDestroyHookFunc. Expect undefined behavior.");
			return;
		}
	}
	if (hasConnected && isHost)
	{
		if (arg3 == 0)
		{
			// Current instance is being destroyed
			auto mapInstance = instanceToIDMap.find(Self);
			if (mapInstance != instanceToIDMap.end())
			{
				uint16_t instanceID = mapInstance->second;
				instanceToIDMap.erase(Self);

				instancesDeleteMessage.addInstance(instanceID);
				if (instancesDeleteMessage.numInstances >= instanceDeleteDataLen)
				{
					sendAllInstanceDeleteMessage();
				}
				availableInstanceIDs.push(instanceID);
			}
		}
	}
	origYYGMLInstanceDestroyFunc(Self, Other, arg3, arg4);
}

EXPORTED AurieStatus ModuleInitialize(
	IN AurieModule* Module,
	IN const fs::path& ModulePath
)
{
	initButtonMenus();

	AurieStatus status = AURIE_SUCCESS;
	status = ObGetInterface("callbackManager", (AurieInterfaceBase*&)callbackManagerInterfacePtr);
	if (!AurieSuccess(status))
	{
		printf("Failed to get callback manager interface. Make sure that CallbackManagerMod is located in the mods/Aurie directory.\n");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	// Gets a handle to the interface exposed by YYTK
	// You can keep this pointer for future use, as it will not change unless YYTK is unloaded.
	status = ObGetInterface(
		"YYTK_Main",
		(AurieInterfaceBase*&)(g_ModuleInterface)
	);

	// If we can't get the interface, we fail loading.
	if (!AurieSuccess(status))
	{
		printf("Failed to get YYTK Interface\n");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	initSteamAPIWrapperFuncs();

	if (isSteamInitialized)
	{
		if (!SteamAPI_Init())
		{
			g_ModuleInterface->Print(CM_RED, "Couldn't initialize Steam api");
			isSteamInitialized = false;
		}
		else
		{
			SteamNetworkingUtils()->InitRelayNetworkAccess();
		}
	}
	else
	{
		g_ModuleInterface->Print(CM_RED, "Couldn't initialize functions from Steam API dll. Disabling Steam features.");
	}

	steamLobbyBrowser = new CSteamLobbyBrowser();
	int numCommandLineArgs = 0;
	LPWSTR* commandLineArgsArr = CommandLineToArgvW(GetCommandLine(), &numCommandLineArgs);
	
	for (int i = 0; i < numCommandLineArgs; i++)
	{
		std::string commandLine = ConvertLPCWSTRToString(commandLineArgsArr[i]);
		if (commandLine.compare("+connect_lobby") == 0)
		{
			if (i >= numCommandLineArgs - 1)
			{
				g_ModuleInterface->Print(CM_RED, "Couldn't find steam lobby id");
			}
			else
			{
				CSteamID steamIDLobby(static_cast<uint64>(atoll(ConvertLPCWSTRToString(commandLineArgsArr[i + 1]).c_str())));
				if (steamIDLobby.IsValid())
				{
					steamLobbyBrowser->setSteamLobbyID(steamIDLobby);
					SteamMatchmaking()->JoinLobby(steamIDLobby);
					switchToMenu(selectedMenu_Lobby);
					hasJoinedSteamLobby = true;
				}
				else
				{
					g_ModuleInterface->Print(CM_RED, "Trying to join invalid steam lobby id");
				}
			}
		}
	}

	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_InputManager_Step_0", nullptr, InputManagerStepAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_InputManager_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_InputManager_Create_0", InputManagerCreateBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_InputManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Player_Mouse_53", nullptr, PlayerMouse53After)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Player_Mouse_53");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Player_Draw_0", PlayerDrawBefore, PlayerDrawAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Player_Draw_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_input_controller_object_Step_1", InputControllerObjectStep1Before, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_input_controller_object_Step_1");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Enemy_Step_0", EnemyStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Enemy_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PlayerManager_Create_0", PlayerManagerCreateBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PlayerManager_Step_0", PlayerManagerStepBefore, PlayerManagerStepAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_PlayerManager_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PlayerManager_CleanUp_0", PlayerManagerCleanUpBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_PlayerManager_CleanUp_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_StageManager_Create_0", StageManagerCreateBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_StageManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_StageManager_Step_0", StageManagerStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_StageManager_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_CrateSpawner_Create_0", CrateSpawnerCreateBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_CrateSpawner_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_CrateSpawner_Step_0", CrateSpawnerStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_CrateSpawner_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Cam_Step_0", CamStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Cam_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_cloudmaker_Alarm_0", CloudMakerAlarmBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_cloudmaker_Alarm_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_BaseMob_Create_0", nullptr, BaseMobCreateAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_BaseMob_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Attack_Create_0", AttackCreateBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Attack_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Attack_Step_0", AttackStepBefore, AttackStepAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Attack_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Attack_Collision_obj_BaseMob", AttackCollisionBaseMobBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Attack_Collision_obj_BaseMob");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Attack_Collision_obj_Obstacle", AttackCollisionObstacleBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Attack_Collision_obj_Obstacle");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Attack_Collision_obj_Attack", AttackCollisionAttackBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Attack_Collision_obj_Attack");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Attack_CleanUp_0", AttackCleanupBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Attack_CleanUp_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Attack_Destroy_0", AttackDestroyBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Attack_Destroy_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Player_Step_0", PlayerStepBefore, PlayerStepAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Player_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PlayerManager_Draw_64", PlayerManagerDraw64Before, PlayerManagerDraw64After)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_PlayerManager_Draw_64");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_EXP_Create_0", EXPCreateBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_EXP_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_EXP_Step_0", AllPickupableStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_EXP_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_HoloCoinDrop_Step_0", AllPickupableStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_HoloCoinDrop_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PickUpable_Step_0", AllPickupableStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_PickUpable_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PickUpable_Collision_obj_Player", AllPickupableCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_PickUpable_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Hamburger_Collision_obj_Player", AllPickupableCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Hamburger_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_HoloCoinDrop_Collision_obj_Player", AllPickupableCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_HoloCoinDrop_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_IdolPower_Collision_obj_Player", AllPickupableCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_IdolPower_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_EXP_Collision_obj_Summon", AllPickupableCollisionSummonBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_EXP_Collision_obj_Summon");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_HoloCoinDrop_Collision_obj_Summon", AllPickupableCollisionSummonBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_HoloCoinDrop_Collision_obj_Summon");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_EXP_Collision_obj_EXP", EXPCollisionEXPBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_EXP_Collision_obj_EXP");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Destructable_Create_0", DestructableCreateBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Destructable_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_YagooPillar_Create_0", YagooPillarCreateBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_YagooPillar_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Obstacle_Step_0", ObstacleStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Obstacle_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_YagooPillar_Step_0", YagooPillarStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_YagooPillar_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_AttackController_Create_0", AttackControllerCreateBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_AttackController_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PlayerManager_CleanUp_0", PlayerManagerCleanUpBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_PlayerManager_CleanUp_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_cautionattack_Step_0", CautionAttackStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_cautionattack_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_caution_Step_0", CautionStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_caution_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PreCreate_Step_0", PreCreateStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_PreCreate_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_vfx_Step_0", VFXStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_vfx_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_afterImage_Step_0", AfterImageStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_afterImage_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_afterImage_Alarm_0", AfterImageAlarmBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_afterImage_Alarm_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_holoBox_Create_0", HoloBoxCreateBefore, HoloBoxCreateAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_holoBox_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_holoAnvil_Create_0", HoloAnvilCreateBefore, HoloAnvilCreateAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_holoAnvil_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_goldenAnvil_Create_0", GoldenAnvilCreateBefore, GoldenAnvilCreateAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_goldenAnvil_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_GoldenHammer_Create_0", GoldenHammerCreateBefore, GoldenHammerCreateAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_GoldenHammer_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Sticker_Create_0", nullptr, StickerCreateAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Sticker_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_holoBox_Collision_obj_Player", HoloBoxCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_holoBox_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_holoAnvil_Collision_obj_Player", HoloAnvilCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_holoAnvil_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_goldenAnvil_Collision_obj_Player", GoldenAnvilCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_goldenAnvil_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_GoldenHammer_CleanUp_0", GoldenHammerCleanUpBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_GoldenHammer_CleanUp_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Sticker_Collision_obj_Player", StickerCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Sticker_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Sticker_Step_0", StickerStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Sticker_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_TextController_Create_0", nullptr, TextControllerCreateAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_TextController_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_TitleScreen_Draw_0", TitleScreenDrawBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_TitleScreen_Draw_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_TitleScreen_Step_0", TitleScreenStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_TitleScreen_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_TitleCharacter_Draw_0", TitleCharacterDrawBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_TitleCharacter_Draw_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_TitleScreen_Create_0", nullptr, TitleScreenCreateAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_TitleScreen_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_TitleScreen_Mouse_53", TitleScreenMouse53Before, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_TitleScreen_Mouse_53");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Summon_Create_0", SummonCreateBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Summon_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Summon_Step_0", SummonStepBefore, SummonStepAfter)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Summon_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_OreDeposit_Step_0", OreDepositStepBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_OreDeposit_Step_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_GetFish_Alarm_0", GetFishAlarm0Before, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_GetFish_Alarm_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_GetFish_Alarm_1", GetFishAlarm1Before, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_GetFish_Alarm_1");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_ShionPortal_Collision_obj_Player", ShionPortalCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_ShionPortal_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_AcerolaJuice_Collision_obj_Player", AcerolaJuiceCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_AcerolaJuice_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_sapling_Collision_obj_Player", SaplingCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_sapling_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_langOrb_Collision_obj_Player", LangOrbCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_langOrb_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_hololiveMerch_Collision_obj_Player", HololiveMerchCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_hololiveMerch_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Coronet_Collision_obj_Player", CoronetCollisionPlayerBefore, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Coronet_Collision_obj_Player");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_PlayerManager_Other_23", PlayerManagerOther23Before, PlayerManagerOther23After)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_PlayerManager_Other_23");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterCodeEventCallback(MODNAME, "gml_Object_obj_Sticker_Alarm_1", nullptr, StickerAlarm1After)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Object_obj_Sticker_Alarm_1");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	


	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_input_binding_set", nullptr, nullptr, &origInputBindingSetScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_input_binding_set");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_input_check", nullptr, nullptr, &origInputCheckScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_input_check");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_input_source_mode_set", nullptr, nullptr, &origInputSourceModeSetScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_input_source_mode_set");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_input_source_clear", nullptr, nullptr, &origInputSourceClearScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_input_source_clear");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_input_join_params_set", nullptr, nullptr, &origInputJoinParamsSetScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_input_join_params_set");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_InitializeCharacter_gml_Object_obj_PlayerManager_Create_0", InitializeCharacterPlayerManagerCreateFuncBefore, InitializeCharacterPlayerManagerCreateFuncAfter, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_InitializeCharacter_gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_SnapshotPrebuffStats_gml_Object_obj_Player_Create_0", nullptr, nullptr, &origSnapshotPrebuffStatsPlayerCreateScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_SnapshotPrebuffStats_gml_Object_obj_Player_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_UpdatePlayer_gml_Object_obj_PlayerManager_Other_24", UpdatePlayerPlayerManagerOtherBefore, nullptr, &origUpdatePlayerPlayerManagerOtherScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_UpdatePlayer_gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddAttack_gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddAttackPlayerManagerOtherScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_AddAttack_gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_input_direction", nullptr, nullptr, &origInputDirectionScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_input_direction");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_CanSubmitScore_gml_Object_obj_PlayerManager_Create_0", CanSubmitScoreFuncBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_CanSubmitScore_gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_input_source_using", nullptr, nullptr, &origInputSourceUsingScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_input_source_using");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script___input_global", nullptr, nullptr, &origInputGlobalScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script___input_global");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_draw_text_outline", nullptr, nullptr, &origDrawTextOutlineScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_draw_text_outline");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_input_source_is_available", nullptr, nullptr, &origInputSourceIsAvailableScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_input_source_is_available");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_input_player_connected_count", nullptr, nullptr, &origInputPlayerConnectedCountScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_input_player_connected_count");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_input_source_get_array", nullptr, nullptr, &origInputSourceGetArrayScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_input_source_get_array");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_input_source_detect_new", nullptr, nullptr, &origInputSourceDetectNewScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_input_source_detect_new");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_input_gamepad_is_connected", nullptr, nullptr, &origInputGamepadIsConnectedScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_input_gamepad_is_connected");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_Stop_gml_Object_obj_Player_Create_0", StopPlayerCreateFuncBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_Stop_gml_Object_obj_Player_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_Init_gml_Object_obj_PlayerManager_Create_0", InitPlayerManagerCreateFuncBefore, InitPlayerManagerCreateFuncAfter, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_Init_gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_Move_gml_Object_obj_Player_Create_0", MovePlayerCreateFuncBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_Move_gml_Object_obj_Player_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_Pause_gml_Object_obj_PlayerManager_Create_0", PausePlayerManagerCreateFuncBefore, PausePlayerManagerCreateFuncAfter, &origPauseScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_Pause_gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script__CreateTakodachi_gml_Object_obj_AttackController_Other_14", CreateTakodachiAttackControllerOther14FuncBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script__CreateTakodachi_gml_Object_obj_AttackController_Other_14");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_glr_mesh_destroy", GLRMeshDestroyFuncBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_glr_mesh_destroy");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_LevelUp_gml_Object_obj_PlayerManager_Create_0", LevelUpPlayerManagerFuncBefore, LevelUpPlayerManagerFuncAfter, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_LevelUp_gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_Confirmed_gml_Object_obj_PlayerManager_Create_0", ConfirmedPlayerManagerFuncBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_Confirmed_gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_GeneratePossibleOptions_gml_Object_obj_PlayerManager_Other_23", nullptr, nullptr, &origGeneratePossibleOptionsScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_GeneratePossibleOptions_gml_Object_obj_PlayerManager_Other_23");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_OptionOne_gml_Object_obj_PlayerManager_Other_23", nullptr, nullptr, &origOptionOneScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_OptionOne_gml_Object_obj_PlayerManager_Other_23");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_OptionTwo_gml_Object_obj_PlayerManager_Other_23", nullptr, nullptr, &origOptionTwoScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_OptionTwo_gml_Object_obj_PlayerManager_Other_23");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_OptionThree_gml_Object_obj_PlayerManager_Other_23", nullptr, nullptr, &origOptionThreeScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_OptionThree_gml_Object_obj_PlayerManager_Other_23");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_OptionFour_gml_Object_obj_PlayerManager_Other_23", nullptr, nullptr, &origOptionFourScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_OptionFour_gml_Object_obj_PlayerManager_Other_23");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_Unpause_gml_Object_obj_PlayerManager_Create_0", UnpausePlayerManagerFuncBefore, nullptr, &origUnpauseScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_Unpause_gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_ParseAndPushCommandType_gml_Object_obj_PlayerManager_Other_23", nullptr, ParseAndPushCommandTypePlayerManagerFuncAfter, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_ParseAndPushCommandType_gml_Object_obj_PlayerManager_Other_23");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddPerk_gml_Object_obj_PlayerManager_Other_24", nullptr, AddPerkPlayerManagerOtherAfter, &origAddPerkScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_AddPerk_gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddItem_gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddItemScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_AddItem_gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddConsumable_gml_Object_obj_PlayerManager_Other_23", nullptr, nullptr, &origAddConsumableScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_AddConsumable_gml_Object_obj_PlayerManager_Other_23");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddStat_gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddStatScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_AddStat_gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_variable_struct_copy", nullptr, nullptr, &origVariableStructCopyScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_variable_struct_copy");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_ExecuteAttack_gml_Object_obj_AttackController_Create_0", ExecuteAttackBefore, ExecuteAttackAfter, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_ExecuteAttack_gml_Object_obj_AttackController_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_OnCollideWithTarget_gml_Object_obj_Attack_Create_0", OnCollideWithTargetAttackBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_OnCollideWithTarget_gml_Object_obj_Attack_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_Die_gml_Object_obj_Obstacle_Create_0", DieObstacleCreateBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_Die_gml_Object_obj_Obstacle_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_EliminateAttack_gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origEliminateAttackScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_EliminateAttack_gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_RemoveItem_gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origRemoveItemScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_RemoveItem_gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_RemovePerk_gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origRemovePerkScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_RemovePerk_gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_ExecuteSpecialAttack_gml_Object_obj_InputManager_Create_0", ExecuteSpecialAttackBefore, nullptr, &origExecuteSpecialAttackScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_ExecuteSpecialAttack_gml_Object_obj_InputManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_ApplyBuff_gml_Object_obj_AttackController_Other_11", ApplyBuffAttackControllerBefore, ApplyBuffAttackControllerAfter, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_ApplyBuff_gml_Object_obj_AttackController_Other_11");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_Destroy_gml_Object_obj_holoAnvil_Create_0", DestroyHoloAnvilBefore, nullptr, &origDestroyHoloAnvilScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_Destroy_gml_Object_obj_holoAnvil_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_Destroy_gml_Object_obj_goldenAnvil_Create_0", DestroyGoldenAnvilBefore, nullptr, &origDestroyGoldenAnvilScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_Destroy_gml_Object_obj_goldenAnvil_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_getAnvil_gml_Object_obj_PlayerManager_Create_0", nullptr, nullptr, &origGetAnvilScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_getAnvil_gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_getGoldenAnvil_gml_Object_obj_PlayerManager_Create_0", nullptr, nullptr, &origGetGoldenAnvilScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_getGoldenAnvil_gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_getSticker_gml_Object_obj_PlayerManager_Create_0", nullptr, nullptr, &origGetStickerScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_getSticker_gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_Destroy_gml_Object_obj_Sticker_Create_0", DestroyStickerBefore, nullptr, &origDestroyStickerScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_Destroy_gml_Object_obj_Sticker_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_TakeDamage_gml_Object_obj_BaseMob_Create_0", TakeDamageBaseMobCreateBefore, TakeDamageBaseMobCreateAfter, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_TakeDamage_gml_Object_obj_BaseMob_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_RollMod", nullptr, RollModAfter, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_RollMod");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddEnchant_gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddEnchantScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_AddEnchant_gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_getBox_gml_Object_obj_PlayerManager_Create_0", nullptr, nullptr, &origGetBoxScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_getBox_gml_Object_obj_PlayerManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddCollab_gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddCollabScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_AddCollab_gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_AddSuperCollab_gml_Object_obj_PlayerManager_Other_24", nullptr, nullptr, &origAddSuperCollabScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_AddSuperCollab_gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_ApplyBuffs_gml_Object_obj_PlayerManager_Other_24", ApplyBuffsPlayerManagerBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_ApplyBuffs_gml_Object_obj_PlayerManager_Other_24");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_Confirmed_gml_Object_obj_TitleScreen_Create_0", ConfirmedTitleScreenBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_Confirmed_gml_Object_obj_TitleScreen_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_MouseOverButton", nullptr, nullptr, &origMouseOverButtonScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_MouseOverButton");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_ReturnMenu_gml_Object_obj_TitleScreen_Create_0", ReturnMenuTitleScreenBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_ReturnMenu_gml_Object_obj_TitleScreen_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_Return_gml_Object_obj_CharSelect_Create_0", ReturnCharSelectCreateBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_Return_gml_Object_obj_CharSelect_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_Select_gml_Object_obj_CharSelect_Create_0", nullptr, SelectCharSelectCreateAfter, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_Select_gml_Object_obj_CharSelect_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_OnDeath_gml_Object_obj_BaseMob_Create_0", OnDeathBaseMobCreateBefore, OnDeathBaseMobCreateAfter, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_OnDeath_gml_Object_obj_BaseMob_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_ParseAndPushCommandType_gml_Object_obj_PlayerManager_Other_23", ParseAndPushCommandTypePlayerManagerOtherBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_ParseAndPushCommandType_gml_Object_obj_PlayerManager_Other_23");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterScriptFunctionCallback(MODNAME, "gml_Script_CreateSummon_gml_Object_obj_MobManager_Create_0", CreateSummonMobManagerCreateBefore, CreateSummonMobManagerCreateAfter, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "gml_Script_CreateSummon_gml_Object_obj_MobManager_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	
	
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterBuiltinFunctionCallback(MODNAME, "struct_get_from_hash", nullptr, nullptr, &origStructGetFromHashFunc)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "struct_get_from_hash");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterBuiltinFunctionCallback(MODNAME, "struct_set_from_hash", nullptr, nullptr, &origStructSetFromHashFunc)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "struct_set_from_hash");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterBuiltinFunctionCallback(MODNAME, "instance_create_layer", InstanceCreateLayerBefore, InstanceCreateLayerAfter, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "instance_create_layer");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterBuiltinFunctionCallback(MODNAME, "sprite_delete", SpriteDeleteBefore, nullptr, &origSpriteDeleteScript)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "sprite_delete");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterBuiltinFunctionCallback(MODNAME, "instance_exists", InstanceExistsBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "instance_exists");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(callbackManagerInterfacePtr->RegisterBuiltinFunctionCallback(MODNAME, "ds_map_find_value", DsMapFindValueBefore, nullptr, nullptr)))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to register callback for %s", "ds_map_find_value");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	status = FindMemoryPatternAddress(
		UTEXT(
			"\x48\x89\x5C\x24\x08"			// MOV qword ptr [RSP + local_res8], RBX
			"\x48\x89\x6C\x24\x10"			// MOV qword ptr [RSP + local_res10], RBP
			"\x48\x89\x74\x24\x18"			// MOV qword ptr [RSP + local_res18], RSI
			"\x57"							// PUSH RDI
			"\x48\x83\xEC\x40"				// SUB RSP, 0x40
			"\x48\x63\xD9"					// MOVSXD RBX, param_1
			"\x40\x32\xFF"					// XOR DIL, DIL
			"\x41\x8B\xE8"					// MOV EBP, param_3
			"\x8B\xF2"						// MOV ESI, param_2
			"\x83\xFB\xFD"					// CMP EBX, -0x3
			"\x0F\x00\x00\x00\x00\x00"		// JNZ
			"\x48\x8b\x00\x00\x00\x00\x00"	// MOV RAX, qword ptr [Should be ptr to instance array]
			"\x48\x85\xC0"					// TEST RAX, RAX 
		),
		"xxxxx"
		"xxxxx"
		"xxxxx"
		"x"
		"xxxx"
		"xxx"
		"xxx"
		"xxx"
		"xx"
		"xxx"
		"x?????"
		"xx?????"
		"xxx",
		yyGMLVariableGetValueAddress
	);
	if (!AurieSuccess(status))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to find memory address for %s", "YYGML_Variable_GetValue");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	PVOID YYGMLVariableGetValueTrampolineFunc = nullptr;
	// Critical section to make sure origYYGMLVariableGetValueFunc is set before the hooked code runs
	initYYGMLVariableGetValueFuncSemaphore.acquire();
	status = MmCreateHook(Module, "YYGML_Variable_GetValue", yyGMLVariableGetValueAddress, YYGMLVariableGetValueHookFunc, &YYGMLVariableGetValueTrampolineFunc);
	origYYGMLVariableGetValueFunc = static_cast<PFUNC_YYGML_Variable_GetValue>(YYGMLVariableGetValueTrampolineFunc);
	initYYGMLVariableGetValueFuncSemaphore.release();

	status = FindMemoryPatternAddress(
		UTEXT(
			"\x48\x89\x5C\x24\x08"			// MOV qword ptr [RSP + local_res8], RBX
			"\x48\x89\x74\x24\x10"			// MOV qword ptr [RSP + local_res10], RSI
			"\x57"							// PUSH RDI
			"\x48\x83\xEC\x40"				// SUB RSP, 0x40
			"\xC6\x44\x24\x28\x00"			// MOV byte ptr[RSP + local_20], 0x0
			"\x41\x8B\xF0"					// MOV ESI, R8D
			"\xC6\x44\x24\x20\x00"			// MOV byte ptr[RSP + local_28], 0x0
			"\x8B\xFA"						// MOV EDI, EDX
			"\x8B\xD9"						// MOV EBX, ECX
		),
		"xxxxx"
		"xxxxx"
		"x"
		"xxxx"
		"xxxxx"
		"xxx"
		"xxxxx"
		"xx"
		"xx",
		yyGMLErrCheckVariableGetValueAddress
	);
	if (!AurieSuccess(status))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to find memory address for %s", "YYGML_ErrCheck_Variable_GetValue");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	PVOID YYGMLErrCheckVariableGetValueTrampolineFunc = nullptr;
	// Critical section to make sure origYYGMLVariableGetValueFunc is set before the hooked code runs
	initYYGMLErrCheckVariableGetValueFuncSemaphore.acquire();
	status = MmCreateHook(Module, "YYGML_ErrCheck_Variable_GetValue", yyGMLErrCheckVariableGetValueAddress, YYGMLErrCheckVariableGetValueHookFunc, &YYGMLErrCheckVariableGetValueTrampolineFunc);
	origYYGMLErrCheckVariableGetValueFunc = static_cast<PFUNC_YYGML_ErrCheck_Variable_GetValue>(YYGMLErrCheckVariableGetValueTrampolineFunc);
	initYYGMLErrCheckVariableGetValueFuncSemaphore.release();

	status = FindMemoryPatternAddress(
		UTEXT(
			"\x48\x89\x5C\x24\x08"			// MOV qword ptr [RSP + local_res8], RBX
			"\x48\x89\x6C\x24\x10"			// MOV qword ptr [RSP + local_res10], RBP
			"\x48\x89\x74\x24\x18"			// MOV qword ptr [RSP + local_res18], RSI
			"\x57"							// PUSH RDI
			"\x48\x83\xEC\x30"				// SUB RSP, 0x30
			"\x49\x8B\xF9"					// MOV RDI, R9
			"\x48\x8B\xF2"					// MOV RSI, RDX
			"\x48\x8B\xE9"					// MOV RBP, RCX
			"\x41\x83\xF8\x02"				// CMP R8D, 0x2
		),
		"xxxxx"
		"xxxxx"
		"xxxxx"
		"x"
		"xxxx"
		"xxx"
		"xxx"
		"xxx"
		"xxxx",
		yyGMLInstanceDestroyAddress
	);
	if (!AurieSuccess(status))
	{
		g_ModuleInterface->Print(CM_RED, "Failed to find memory address for %s", "YYGML_Instance_Destroy");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	PVOID YYGMLInstanceDestroyTrampolineFunc = nullptr;
	// Critical section to make sure origYYGMLVariableGetValueFunc is set before the hooked code runs
	initYYGMLInstanceDestroyFuncSemaphore.acquire();
	status = MmCreateHook(Module, "YYGML_Instance_Destroy", yyGMLInstanceDestroyAddress, YYGMLInstanceDestroyHookFunc, &YYGMLInstanceDestroyTrampolineFunc);
	origYYGMLInstanceDestroyFunc = static_cast<PFUNC_YYGML_Instance_Destroy>(YYGMLInstanceDestroyTrampolineFunc);
	initYYGMLInstanceDestroyFuncSemaphore.release();
	
	// TODO: Probably should add a check if the players have the same version number when connecting

	// TODO: Fix various crashes
	// TODO: Fix stage ending sometimes not actually pausing (Might be because a player picks up a box at the same time)
	// TODO: Improve ping by changing some messages to be sent via UDP
	// TODO: Probably should reduce attack speed
	// TODO: Need to prevent player from moving outside the map and actually collide with stuff (first need to rewrite the message handler)

	// Lower priority
	// TODO: Fix some stuff not being visible on the client side like polyglot's lang orbs
	// TODO: Should probably scale back the amount of updates being sent when there's a lot of enemies on screen and the connection isn't that good (Seems like the client can still send inputs without delay while it's lagging, so need to investigate further)
	// TODO: Add verbose file logs
	// TODO: Fix some attacks not being deleted properly. Probably due to the attack id being reused and overwriting the last attack using that id before it got deleted
	// TODO: Optimize message sizes for update messages to reduce the amount of upload needed (pickupable, attack, vfx)
	// TODO: Extrapolate positional data to reduce the amount of messages being sent
	// TODO: Optimize pickupable data to not require frequent updates since there can possibly be a lot of it. (Probably either avoid sending updates if it hasn't moved or don't send updates at all and have it all handled locally)
	// TODO: Change the message queues (attack, instances, etc...) back to individual messages since it seems like it's worse to queue them
	// TODO: Look into issue if the player picks up a box at the end of game screen and breaks the paused state
	// TODO: Add better way of assigning ids to players
	// TODO: Profile the mod to see if it's causing a significant increase in lag (show_debug_overlay as well)
	// TODO: Sometimes random crashing when the game starts up. Investigate further
	// TODO: Game crashes when trying to upgrade in addAttack after retrying the game
	// TODO: Restarting a game after the game already restarted crashes
	// TODO: Make the networking system more robust to disconnections/edge cases
	// TODO: Figure out what to do if a client disconnects on the host side. Keep the player numbers the same, or try to rearrange the players? (Can maybe just mark certain players as disconnected. Might potentially allow for reconnecting to a game)
	// TODO: Sending 255 as a char ended up being received as -1. Check to make sure this isn't causing issues anywhere
	// TODO: Fix crash with korone's yubiApply
	// TODO: ^ Fix apply buffs not being able to figure out which player to apply the buff to and thus causing a crash
	// TODO: Get different host and client languages working
	// TODO: Seems like interacting with the holobox didn't remove it for some reason on the client side? Check more into why this happens
	// TODO: Seems like the self destruct red circle is drawn inside the self destruct function through draw functions and isn't a separate thing (gml_Object_obj_MobManager_Other_12)
	// TODO: Probably need to send over the other circle data as well
	// TODO: Seems like the inner expanding circle for precreate doesn't show up?
	// TODO: Remove rainbow exp when it is collected on the client side
	// TODO: Should get sounds working for client. Can't just send over sounds from host since that could play sounds twice
	// TODO: Scale the attack delay speed by the number of players
	// TODO: Get auto player revive working for client and host
	// TODO: Fix the heights that some attacks are being spawned in which blocks other stuff
	// TODO: Maybe should do certain timed stuff based off of the game frame count rather than sending the entire state each frame
	// TODO: Use UDP to reduce latency for some messages. Need to make sure that it only gets used with messages that aren't critical and are okay to be dropped.
	// TODO: Check to make sure that multiple players can't use the same interactable if they collide with it on the same frame
	// TODO: Seems like the holobox can move if they're out of bounds. Make sure to move it accordingly
	// TODO: Should probably cache the attackcontroller rvalue
	// TODO: Check RollMod if it has issues
	// TODO: Fix the coin count fluctuating a bit after getting a holobox (only a minor visual bug)
	// TODO: Send over the money gain multiplier to the client
	// TODO: Do something if the host/client disconnects
	// TODO: Fix enchants not carrying over for collabs
	// TODO: Send over debuffs to the client
	// TODO: Change sending the buff to be every time it ticks a second or gains a stack
	// TODO: Might need to swap player data before hittarget for calculatedamage?
	// TODO: Might also need to swap player data before takeDamage for the obstacles?
	// TODO: Fix some attacks being displayed above other important stuff
	// TODO: Fix vfx not being sent over properly after a while
	// TODO: Add outfit selection in the character selection
	// TODO: Get initial stats different for each character
	// TODO: Add some code to reconnect players if they disconnect during a game
	// TODO: Add player naming feature?
	// TODO: Investigate potential issue if the host disconnects while in a level up. The client gets sent back to the title screen with the level up stuff still there
	// TODO: Attack cleanup seems to crash for client closing game. Not too important if it only happens when closing the game
	// TODO: Starting to see "Couldn't find instance *hex* for glr_mesh_destroy" only on the host. Should probably try to see why this is happening
	// This is happening in the glr_mesh_destroy script hook
	// TODO: Fix receive input message to run while inside a player instance's code function to implement client player collisions
	// TODO: Reenable sending vfx once I get a way to send its data over efficiently
	// TODO: Seems like holobomb isn't being removed correctly?
	// TODO: Fix rainbow exp size
	// TODO: Fix korone's coronet and probably some other pickupables that I missed from not being deleted when picked up
	// TODO: Fix bug with mumei's friend not using the correct player's weapon
	// 
	// TODO: Seems like there might be some minor issues with the hp stat not being calculated properly when upgraded?

	objPlayerIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_Player" }).AsReal());
	objBaseMobIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_BaseMob" }).AsReal());
	objAttackIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_Attack" }).AsReal());
	objPickupableIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_PickUpable" }).AsReal());
	objSummonIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_Summon" }).AsReal());
	objAttackControllerIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_AttackController" }).AsReal());
	objPlayerManagerIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_PlayerManager" }).AsReal());
	objDestructableIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_Destructable" }).AsReal());
	objYagooPillarIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_YagooPillar" }).AsReal());
	objCautionIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_caution" }).AsReal());
	objCautionAttackIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_cautionattack" }).AsReal());
	objPreCreateIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_PreCreate" }).AsReal());
	objVFXIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_vfx" }).AsReal());
	objAfterImageIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_afterImage" }).AsReal());
	objHoloBoxIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_holoBox" }).AsReal());
	objHoloAnvilIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_holoAnvil" }).AsReal());
	objGoldenAnvilIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_goldenAnvil" }).AsReal());
	objGoldenHammerIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_GoldenHammer" }).AsReal());
	objStickerIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_Sticker" }).AsReal());
	objItemLightBeamIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_itemLightBeam" }).AsReal());
	objOptionsIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_Options" }).AsReal());
	objInputManagerIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_InputManager" }).AsReal());
	objCharacterDataIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_CharacterData" }).AsReal());
	objCharSelectIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_CharSelect" }).AsReal());
	objObstacleIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_Obstacle" }).AsReal());
	objGetFishIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "obj_GetFish" }).AsReal());
	sprEmptyIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "spr_empty" }).AsReal());
	sprGameCursorIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "spr_GameCursor" }).AsReal());
	sprGameCursor2Index = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "spr_GameCursor2" }).AsReal());
	sprEmptyMaskIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "spr_emptyMask" }).AsReal());
	sprSummonPointerIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "spr_summonPointer" }).AsReal());
	sprHudInitButtonsIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "hud_initButtons" }).AsReal());
	sprKaelaMinerals = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "spr_KaelaMinerals" }).AsReal());
	sprHudToggleButtonIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "hud_toggleButton" }).AsReal());
	sprHudOptionButtonIndex = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "hud_OptionButton" }).AsReal());
	jpFont = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "jpFont" }).AsReal());
	rmTitle = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "rm_Title" }).AsReal());
	rmCharSelect = static_cast<int>(g_ModuleInterface->CallBuiltin("asset_get_index", { "rm_CharSelect" }).AsReal());
	g_RunnerInterface = g_ModuleInterface->GetRunnerInterface();
	g_ModuleInterface->GetGlobalInstance(&globalInstance);

	for (int i = 0; i < std::extent<decltype(VariableNamesStringsArr)>::value; i++)
	{
		if (!AurieSuccess(status))
		{
			g_ModuleInterface->Print(CM_RED, "Failed to get hash for %s", VariableNamesStringsArr[i]);
		}
		GMLVarIndexMapGMLHash[i] = std::move(g_ModuleInterface->CallBuiltin("variable_get_hash", { VariableNamesStringsArr[i] }));
	}
	printf("Finished hook\n");

	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		g_ModuleInterface->Print(CM_RED, "Failed to initialize winsock: %d\n", iResult);
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	struct addrinfo hints, *servinfo, *p = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	char optval = '1';

	getaddrinfo(NULL, BROADCAST_PORT, &hints, &servinfo);

	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		listenSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
		bind(listenSocket, p->ai_addr, static_cast<int>(p->ai_addrlen));
		u_long mode = 1;
		ioctlsocket(listenSocket, FIONBIO, &mode);
		break;
	}

	freeaddrinfo(servinfo);

	printf("Finished initializing network stuff\n");
	
	return AURIE_SUCCESS;
}