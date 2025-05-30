#pragma comment(lib, "iphlpapi.lib")
#include <WS2tcpip.h>
#include "ScriptFunctions.h"
#include <YYToolkit/YYTK_Shared.hpp>
#include <CallbackManager/CallbackManagerInterface.h>
#include "ModuleMain.h"
#include "CommonFunctions.h"
#include "NetworkFunctions.h"
#include "CodeEvents.h"
#include "SteamHost.h"
#include "SteamClient.h"
#include "SteamLobbyBrowser.h"
#include "Button.h"
#include <iphlpapi.h>
#include <fstream>
#include <thread>

#define HOST_INDEX 0

extern CSteamID curSelectedSteamID;
extern CSteamLobbyBrowser* steamLobbyBrowser;
extern int curSteamLobbyMemberIndex;
extern bool isSteamInitialized;
extern std::thread messageHandlerThread;
extern menuGrid lobbyMenuGrid;
extern menuGrid selectingCharacterMenuGrid;
extern menuGrid selectingMapMenuGrid;

int numClientsInGame = -1;

inline PFUNC_YYGMLScript getScriptFunction(const char* name)
{
	AurieStatus status = AURIE_SUCCESS;
	int scriptIndex = g_RunnerInterface.Script_Find_Id(name) - 100000;
	CScript* scriptPtr = nullptr;
	status = g_ModuleInterface->GetScriptData(scriptIndex, scriptPtr);
	if (!AurieSuccess(status))
	{
		g_ModuleInterface->Print(CM_RED, "Failed obtaining script function data for %s at script index %d with status %d", name, scriptIndex, status);
		return nullptr;
	}
	return scriptPtr->m_Functions->m_ScriptFunction;
}

inline void addWeapon(RValue& weapons, RValue& attacks, RValue& attackID)
{
	RValue mainWeapon;
	RValue attackObj = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, attackID });
	RValue attackObjConfig = g_ModuleInterface->CallBuiltin("variable_instance_get", { attackObj, "config" });
	RValue attackObjWeight = g_ModuleInterface->CallBuiltin("variable_instance_get", { attackObjConfig, "weight" });
	RValue attackObjMaxLevel = g_ModuleInterface->CallBuiltin("variable_instance_get", { attackObjConfig, "maxLevel" });
	g_RunnerInterface.StructCreate(&mainWeapon);
	g_RunnerInterface.StructAddRValue(&mainWeapon, "id", &attackID);
	g_RunnerInterface.StructAddDouble(&mainWeapon, "level", 0);
	g_RunnerInterface.StructAddRValue(&mainWeapon, "weight", &attackObjWeight);
	g_RunnerInterface.StructAddRValue(&mainWeapon, "maxLevel", &attackObjMaxLevel);
	RValue combos;
	g_RunnerInterface.StructCreate(&combos);
	g_RunnerInterface.StructAddRValue(&mainWeapon, "combos", &combos);
	g_RunnerInterface.StructAddRValue(&weapons, attackID.ToString().data(), &mainWeapon);
}

std::unordered_map<uint32_t, RValue> playerMap;
std::unordered_map<uint32_t, RValue> playerWeaponMap;
std::unordered_map<uint32_t, RValue> playerCharPerksMap;
std::unordered_map<uint32_t, RValue> playerItemsMapMap;
std::unordered_map<uint32_t, RValue> playerItemsMap;
std::unordered_map<uint32_t, RValue> playerAttackIndexMapMap;
std::unordered_map<uint32_t, RValue> playerPerksMap;
std::unordered_map<uint32_t, RValue> playerPerksMapMap;
std::unordered_map<uint32_t, RValue> playerStatUpsMap;
std::unordered_map<uint32_t, RValue> playerAvailableWeaponCollabsMap;
std::unordered_map<uint32_t, RValue> playerWeaponCollabsMap;
std::unordered_map<uint32_t, RValue> attacksCopyMap;
std::unordered_map<uint32_t, RValue> removedItemsMap;
std::unordered_map<uint32_t, RValue> eliminatedAttacksMap;
std::unordered_map<uint32_t, RValue> rerollContainerMap;
std::unordered_map<uint32_t, RValue> currentStickersMap;
std::unordered_map<uint32_t, RValue> charSelectedMap;
std::unordered_map<uint32_t, RValue> summonMap;
std::unordered_map<uint32_t, RValue> customDrawScriptMap;
std::unordered_map<uint32_t, int> playerPingMap;
std::unordered_map<uint32_t, lobbyPlayerData> lobbyPlayerDataMap;
std::unordered_map<uint32_t, bool> clientUnpausedMap;

std::unordered_map<uint32_t, levelUpOptionNames> levelUpOptionNamesMap;
std::unordered_map<uint64, uint32_t> steamIDToClientIDMap;
std::unordered_map<uint32_t, uint64> clientIDToSteamIDMap;
std::unordered_map<uint64, steamConnection> steamIDToConnectionMap;

CSteamHost* steamHost = nullptr;

double moneyGainMultiplier = 0;
double foodMultiplier = 0;

int curPlayerID = 0;
std::vector<uint32_t> curPlayerIDStack;

bool isClientInInitializeCharacter = false;

RValue deepCopyStruct(CInstance* Self, RValue& origStruct, RValue* parentStructPtr);
RValue deepCopyArray(CInstance* Self, RValue& origArray, RValue* parentStructPtr);

RValue deepCopyArray(CInstance* Self, RValue& origArray, RValue* parentStructPtr)
{
	int origArrayLen = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { origArray }).ToDouble()));
	RValue copiedArray = g_ModuleInterface->CallBuiltin("array_create", { origArrayLen });
	for (int i = 0; i < origArrayLen; i++)
	{
		RValue curArrayVal = origArray[i];
		// Apparently methods are considered as structs also, so the check for is_method needs to go before is_struct
		if (g_ModuleInterface->CallBuiltin("is_array", { curArrayVal }).ToBoolean())
		{
			RValue copiedCurArrayVal = deepCopyArray(Self, curArrayVal, parentStructPtr);
			copiedArray[i] = copiedCurArrayVal;
		}
		else if (g_ModuleInterface->CallBuiltin("is_method", { curArrayVal }).ToBoolean())
		{
			RValue methodSelf = g_ModuleInterface->CallBuiltin("method_get_self", { curArrayVal });
			if (methodSelf.m_Kind == VALUE_REAL || methodSelf.m_Kind == VALUE_REF)
			{
				// Assume that the method can be safely copied since it's bound to a script function method
				copiedArray[i] = curArrayVal;
			}
			else if (methodSelf.m_Kind == VALUE_OBJECT)
			{
				// Make sure to rebind the copied method to the current instance
				// TODO: Seems like the methods defined in a struct will retain information about their parent even if you copy them to a different object of the same type.
				// TODO: Should probably find a way to only copy these functions while assigning the parent as the new struct and not doing that for all the other methods
				RValue copiedMethod = g_ModuleInterface->CallBuiltin("method", { *parentStructPtr, curArrayVal });
				copiedArray[i] = copiedMethod;
			}
			else
			{
				g_ModuleInterface->Print(CM_RED, "Unhandled kind %d for array index %d", methodSelf.m_Kind, i);
			}
		}
		else if (g_ModuleInterface->CallBuiltin("is_struct", { curArrayVal }).ToBoolean())
		{
			RValue copiedCurArrayVal = deepCopyStruct(Self, curArrayVal, parentStructPtr);
			copiedArray[i] = copiedCurArrayVal;
		}
		else
		{
			copiedArray[i] = curArrayVal;
		}
	}
	return copiedArray;
}

RValue deepCopyStruct(CInstance* Self, RValue& origStruct, RValue* parentStructPtr)
{
	RValue copiedStruct;
	g_RunnerInterface.StructCreate(&copiedStruct);
	if (parentStructPtr == nullptr)
	{
		parentStructPtr = &copiedStruct;
	}
	// TODO: Find a more performant way to iterate through a struct
	RValue structNames = g_ModuleInterface->CallBuiltin("struct_get_names", { origStruct });
	int structNamesLen = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { structNames }).ToDouble()));
	for (int i = 0; i < structNamesLen; i++)
	{
		RValue curStructName = structNames[i];
		RValue curStructVal = g_ModuleInterface->CallBuiltin("variable_instance_get", { origStruct, curStructName });
		if (g_ModuleInterface->CallBuiltin("is_array", { curStructVal }).ToBoolean())
		{
			RValue copiedCurStructVal = deepCopyArray(Self, curStructVal, parentStructPtr);
			g_RunnerInterface.StructAddRValue(&copiedStruct, curStructName.ToString().data(), &copiedCurStructVal);
		}
		else if (g_ModuleInterface->CallBuiltin("is_method", { curStructVal }).ToBoolean())
		{
			RValue methodSelf = g_ModuleInterface->CallBuiltin("method_get_self", { curStructVal });
			if (methodSelf.m_Kind == VALUE_REAL || methodSelf.m_Kind == VALUE_REF)
			{
				// Assume that the method can be safely copied since it's bound to a script function method
				g_RunnerInterface.StructAddRValue(&copiedStruct, curStructName.ToString().data(), &curStructVal);
			}
			else if (methodSelf.m_Kind == VALUE_OBJECT)
			{
				// Make sure to rebind the copied method to the current instance
				// TODO: Seems like the methods defined in a struct will retain information about their parent even if you copy them to a different object of the same type.
				// TODO: Should probably find a way to only copy these functions while assigning the parent as the new struct and not doing that for all the other methods
				RValue copiedMethod = g_ModuleInterface->CallBuiltin("method", { *parentStructPtr, curStructVal });
				g_RunnerInterface.StructAddRValue(&copiedStruct, curStructName.ToString().data(), &copiedMethod);
			}
			else
			{
				g_ModuleInterface->Print(CM_RED, "Unhandled kind %d for method %s", methodSelf.m_Kind, curStructName.ToString().data());
			}
		}
		else if (g_ModuleInterface->CallBuiltin("is_struct", { curStructVal }).ToBoolean())
		{
			RValue copiedCurStructVal = deepCopyStruct(Self, curStructVal, parentStructPtr);
			g_RunnerInterface.StructAddRValue(&copiedStruct, curStructName.ToString().data(), &copiedCurStructVal);
		}
		else
		{
			g_RunnerInterface.StructAddRValue(&copiedStruct, curStructName.ToString().data(), &curStructVal);
		}
	}
	return copiedStruct;
}

void addCollabData(RValue& weapons, RValue& attacks)
{
	RValue attacksKeysArr = g_ModuleInterface->CallBuiltin("ds_map_keys_to_array", { attacks });
	int attacksKeysArrLength = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { attacksKeysArr }).ToDouble()));
	for (int i = 0; i < attacksKeysArrLength; i++)
	{
		RValue curKey = attacksKeysArr[i];
		RValue attackObj = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, curKey });
		RValue attackObjConfig = g_ModuleInterface->CallBuiltin("variable_instance_get", { attackObj, "config" });
		RValue attackObjConfigOptionType = g_ModuleInterface->CallBuiltin("variable_instance_get", { attackObjConfig, "optionType" });
		RValue attackObjConfigCombos = g_ModuleInterface->CallBuiltin("variable_instance_get", { attackObjConfig, "combos" });
		RValue attackObjAttackID = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attackObj, "attackID" });

		if (attackObjConfigOptionType.ToString().compare("Collab") == 0)
		{
			// TODO: Make a new weaponCollabs struct and add to it
			if (g_ModuleInterface->CallBuiltin("variable_struct_exists", { weapons, attackObjConfigCombos[0] }).ToBoolean() &&
				g_ModuleInterface->CallBuiltin("variable_struct_exists", { weapons, attackObjConfigCombos[1] }).ToBoolean())
			{
				RValue weaponOne = g_ModuleInterface->CallBuiltin("variable_instance_get", { weapons, attackObjConfigCombos[0] });
				RValue weaponTwo = g_ModuleInterface->CallBuiltin("variable_instance_get", { weapons, attackObjConfigCombos[1] });
				RValue weaponOneCombos = g_ModuleInterface->CallBuiltin("variable_instance_get", { weaponOne, "combos" });
				RValue weaponTwoCombos = g_ModuleInterface->CallBuiltin("variable_instance_get", { weaponTwo, "combos" });
				g_ModuleInterface->CallBuiltin("variable_instance_set", { weaponOneCombos, attackObjConfigCombos[1], attackObjAttackID });
				g_ModuleInterface->CallBuiltin("variable_instance_set", { weaponTwoCombos, attackObjConfigCombos[0], attackObjAttackID });
			}
		}
		else if (attackObjConfigOptionType.ToString().compare("SuperCollab") == 0)
		{
			// TODO: Add super collab code
		}
	}
}

RValue deepCopyMap(CInstance* Self, RValue& origMap)
{
	RValue origMapKeys = g_ModuleInterface->CallBuiltin("ds_map_keys_to_array", { origMap });
	int origMapKeysLength = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { origMapKeys }).ToDouble()));
	RValue copiedMap = g_ModuleInterface->CallBuiltin("ds_map_create", {});
	for (int i = 0; i < origMapKeysLength; i++)
	{
		RValue curKey = origMapKeys[i];
		RValue mapValue = g_ModuleInterface->CallBuiltin("ds_map_find_value", { origMap, curKey });

		RValue copiedStruct = deepCopyStruct(Self, mapValue, nullptr);
		g_ModuleInterface->CallBuiltin("ds_map_add", { copiedMap, curKey, copiedStruct });
	}
	return copiedMap;
}

RValue& InitializeCharacterPlayerManagerCreateFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && !isHost)
	{
		isClientInInitializeCharacter = true;
	}
	return ReturnValue;
}

RValue& InitializeCharacterPlayerManagerCreateFuncAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected)
	{
		if (!isHost)
		{
			isClientInInitializeCharacter = false;
		}
		else
		{
			// TODO: Make sure the lists are cleared and all the gml variables are cleaned up every time a new game starts
			// 

			// Seems like the level reset at the beginning may cause the initialize character to run twice, so might have to just clear the lists.
			// TODO: Maybe check if the reset level flag is true and skip all this?
			playerItemsMapMap.clear();
			playerItemsMap.clear();
			playerMap.clear();
			eliminatedAttacksMap.clear();
			removedItemsMap.clear();
			rerollContainerMap.clear();
			currentStickersMap.clear();
			charSelectedMap.clear();
			playerAttackIndexMapMap.clear();
			playerWeaponMap.clear();
			playerCharPerksMap.clear();
			playerPerksMap.clear();
			playerPerksMapMap.clear();
			playerAvailableWeaponCollabsMap.clear();
			playerWeaponCollabsMap.clear();
			playerStatUpsMap.clear();
			summonMap.clear();
			customDrawScriptMap.clear();
			curPlayerIDStack.clear();

			curPlayerIDStack.push_back(0);

			if (availableInstanceIDs.empty())
			{
				for (int i = 0; i < maxNumAvailableInstanceIDs; i++)
				{
					availableInstanceIDs.push(static_cast<uint16_t>(i));
				}
				for (int i = 0; i < maxNumAvailableAttackIDs; i++)
				{
					availableAttackIDs.push(static_cast<uint16_t>(i));
				}
				for (int i = 0; i < maxNumAvailablePickupableIDs; i++)
				{
					availablePickupableIDs.push(static_cast<uint16_t>(i));
				}
				for (int i = 0; i < maxNumAvailablePreCreateIDs; i++)
				{
					availablePreCreateIDs.push(static_cast<uint16_t>(i));
				}
				for (int i = 0; i < maxNumAvailableVFXIDs; i++)
				{
					availableVFXIDs.push(static_cast<uint16_t>(i));
				}
				for (int i = 0; i < maxNumAvailableInteractableIDs; i++)
				{
					availableInteractableIDs.push(static_cast<uint16_t>(i));
				}
			}

			// Array to prevent variables from being garbage collected
			// TODO: Should probably create a second list just for maps to easily clean them up when the game ends
			RValue keepAliveArr = g_ModuleInterface->CallBuiltin("array_create", { 0 });
			g_ModuleInterface->CallBuiltin("variable_global_set", { "keepAliveArr", keepAliveArr });

			// get all the original host player data and put it in the keepalive array
			RValue itemsMap = getInstanceVariable(Self, GML_ITEMS);
			playerItemsMapMap[HOST_INDEX] = itemsMap;
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, itemsMap });

			RValue globalPlayerAttacks = g_ModuleInterface->CallBuiltin("variable_global_get", { "playerAttacks" });

			RValue items = getInstanceVariable(Self, GML_items);
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, items });
			playerItemsMap[HOST_INDEX] = items;

			RValue originalPlayer = getInstanceVariable(Self, GML_createdChar);
			playerMap[HOST_INDEX] = originalPlayer;
			RValue createdCharX = getInstanceVariable(originalPlayer, GML_x);
			RValue createdCharY = getInstanceVariable(originalPlayer, GML_y);

			// Initialize eliminatedAttacks list
			RValue eliminatedAttacks = getInstanceVariable(Self, GML_eliminatedAttacks);
			eliminatedAttacksMap[HOST_INDEX] = eliminatedAttacks;
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, eliminatedAttacks });

			// Initialize removedItems list
			RValue removedItems = getInstanceVariable(Self, GML_removedItems);
			removedItemsMap[HOST_INDEX] = removedItems;
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, removedItems });

			// Initialize rerollContainer list
			RValue rerollContainer = getInstanceVariable(Self, GML_rerollContainer);
			rerollContainerMap[HOST_INDEX] = rerollContainer;

			// Initialize currentStickers list
			RValue currentStickers = g_ModuleInterface->CallBuiltin("variable_global_get", { "currentStickers" });
			currentStickersMap[HOST_INDEX] = currentStickers;
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, currentStickers });

			RValue prevCharData = getInstanceVariable(Self, GML_charData);
			// Initialize charSelected list
			charSelectedMap[HOST_INDEX] = prevCharData;
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, prevCharData });

			RValue prevAttackID = getInstanceVariable(prevCharData, GML_attackID);

			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			RValue attacks = getInstanceVariable(attackController, GML_attackIndex);
			playerAttackIndexMapMap[HOST_INDEX] = attacks;
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, attacks });

			RValue playerSave = g_ModuleInterface->CallBuiltin("variable_global_get", { "PlayerSave" });
			RValue unlockedWeapons = g_ModuleInterface->CallBuiltin("ds_map_find_value", { playerSave, "unlockedWeapons" });
			int unlockedWeaponsLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { unlockedWeapons }).ToDouble());

			RValue prevWeapons = getInstanceVariable(Self, GML_weapons);
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, prevWeapons });
			playerWeaponMap[HOST_INDEX] = prevWeapons;

			RValue prevCharPerks = getInstanceVariable(Self, GML_charPerks);
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, prevCharPerks });
			playerCharPerksMap[HOST_INDEX] = prevCharPerks;

			RValue prevAvailableWeaponCollabs = getInstanceVariable(Self, GML_availableWeaponCollabs);
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, prevAvailableWeaponCollabs });
			playerAvailableWeaponCollabsMap[HOST_INDEX] = prevAvailableWeaponCollabs;

			RValue prevWeaponCollabs = getInstanceVariable(Self, GML_weaponCollabs);
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, prevWeaponCollabs });
			playerWeaponCollabsMap[HOST_INDEX] = prevWeaponCollabs;

			RValue perks = getInstanceVariable(Self, GML_perks);
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, perks });
			playerPerksMap[HOST_INDEX] = perks;

			RValue perksMap = getInstanceVariable(Self, GML_PERKS);
			playerPerksMapMap[HOST_INDEX] = perksMap;
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, perksMap });

			RValue playerStatUps = getInstanceVariable(Self, GML_playerStatUps);
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, playerStatUps });
			playerStatUpsMap[HOST_INDEX] = playerStatUps;

			summonMap[HOST_INDEX] = RValue();

			RValue customDrawScript = getInstanceVariable(Self, GML_customDrawScript);
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, customDrawScript });
			customDrawScriptMap[HOST_INDEX] = customDrawScript;

			RValue playerCharacter = getInstanceVariable(Self, GML_playerCharacter);

			// initialize player data for each client
			for (auto& curClientIDMapping : clientIDToSteamIDMap)
			{
				uint32_t clientID = curClientIDMapping.first;

				summonMap[clientID] = RValue();

				RValue newCustomDrawScript;
				g_RunnerInterface.StructCreate(&newCustomDrawScript);
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, newCustomDrawScript });
				customDrawScriptMap[clientID] = newCustomDrawScript;

				RValue newItemsMap = deepCopyMap(Self, itemsMap);
				playerItemsMapMap[clientID] = newItemsMap;
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, newItemsMap });

				RValue newItems;
				g_RunnerInterface.StructCreate(&newItems);
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, newItems });
				playerItemsMap[clientID] = newItems;

				// set to an invalid index to make the player create not delete the already created attacks map
				g_ModuleInterface->CallBuiltin("variable_global_set", { "playerAttacks", -1.0 });

				RValue newCreatedChar = g_ModuleInterface->CallBuiltin("instance_create_layer", { createdCharX, createdCharY, "Instances", objPlayerIndex });
				playerMap[clientID] = newCreatedChar;

				RValue newEliminatedAttacks = g_ModuleInterface->CallBuiltin("array_create", { 0 });
				eliminatedAttacksMap[clientID] = newEliminatedAttacks;
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, newEliminatedAttacks });

				RValue newRemovedItems = g_ModuleInterface->CallBuiltin("array_create", { 0 });
				removedItemsMap[clientID] = newRemovedItems;
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, newRemovedItems });

				RValue newCurrentStickers = g_ModuleInterface->CallBuiltin("array_create", { 3, -1.0 });
				currentStickersMap[clientID] = newCurrentStickers;
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, newCurrentStickers });

				RValue characterDataMap = g_ModuleInterface->CallBuiltin("variable_global_get", { "characterData" });
				RValue charData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { characterDataMap, lobbyPlayerDataMap[clientID].charName.c_str() });
				charSelectedMap[clientID] = charData;
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, charData });

				RValue charDataSprite1 = getInstanceVariable(charData, GML_sprite1);
				RValue charDataSprite2 = getInstanceVariable(charData, GML_sprite2);
				g_ModuleInterface->CallBuiltin("variable_global_set", { "charSelected", charData });
				setInstanceVariable(newCreatedChar, GML_charName, getInstanceVariable(charData, GML_charName));
				setInstanceVariable(newCreatedChar, GML_idleSprite, charDataSprite1);
				setInstanceVariable(newCreatedChar, GML_runSprite, charDataSprite2);
				RValue charDataSprite3 = getInstanceVariable(charData, GML_sprite3);
				if (charDataSprite3.m_Kind != VALUE_UNDEFINED && charDataSprite3.m_Kind != VALUE_UNSET)
				{
					setInstanceVariable(newCreatedChar, GML_sprite3, charDataSprite3);
				}
				RValue charDataSprites = getInstanceVariable(charData, GML_sprites);
				if (charDataSprites.m_Kind != VALUE_UNDEFINED && charDataSprites.m_Kind != VALUE_UNSET)
				{
					setInstanceVariable(newCreatedChar, GML_sprites, charDataSprites);
				}
				else
				{
					charDataSprites = g_ModuleInterface->CallBuiltin("array_create", { 1 });
					RValue spritesStruct;
					g_RunnerInterface.StructCreate(&spritesStruct);
					setInstanceVariable(spritesStruct, GML_sprite1, charDataSprite1);
					setInstanceVariable(spritesStruct, GML_sprite2, charDataSprite2);
					charDataSprites[0] = spritesStruct;
					setInstanceVariable(newCreatedChar, GML_sprites, charDataSprites);
				}
				// TODO: deal with apply progression stats for the client character
				setInstanceVariable(newCreatedChar, GML_challenge, getInstanceVariable(charData, GML_challenge));
				RValue baseStats = getInstanceVariable(Self, GML_baseStats);
				moneyGainMultiplier = getInstanceVariable(baseStats, GML_moneyGain).ToDouble();
				foodMultiplier = getInstanceVariable(baseStats, GML_food).ToDouble();
				setInstanceVariable(newCreatedChar, GML_HP, getInstanceVariable(baseStats, GML_HP));
				setInstanceVariable(newCreatedChar, GML_currentHP, getInstanceVariable(baseStats, GML_HP));
				setInstanceVariable(newCreatedChar, GML_ATK, getInstanceVariable(baseStats, GML_ATK));
				setInstanceVariable(newCreatedChar, GML_SPD, getInstanceVariable(baseStats, GML_SPD));
				setInstanceVariable(newCreatedChar, GML_crit, getInstanceVariable(baseStats, GML_crit));
				setInstanceVariable(newCreatedChar, GML_haste, getInstanceVariable(baseStats, GML_haste));
				setInstanceVariable(newCreatedChar, GML_pickupRange, getInstanceVariable(baseStats, GML_pickupRange));

				RValue preBuffStats = getInstanceVariable(newCreatedChar, GML_prebuffStats);
				RValue returnVal;
				RValue** args = new RValue*[2];
				args[0] = &baseStats;
				args[1] = &preBuffStats;
				getScriptFunction("gml_Script_variable_struct_copy")(Self, Other, returnVal, 2, args);
				RValue snapshotPrebuffStatsMethod = getInstanceVariable(newCreatedChar, GML_SnapshotPrebuffStats);
				RValue snapshotPrebuffStatsArr = g_ModuleInterface->CallBuiltin("array_create", { RValue(0.0) });
				g_ModuleInterface->CallBuiltin("method_call", { snapshotPrebuffStatsMethod, snapshotPrebuffStatsArr });
				// deal with add attack (calls update player which sets up some stuff)

				// swap the player character to the newly created to be able to utilize the code already set up for updating the player
				setInstanceVariable(Self, GML_playerCharacter, newCreatedChar);

				RValue attackID = getInstanceVariable(charData, GML_attackID);

				// Copy over attackIndex to a different map
				RValue newAttackIndexMap = deepCopyMap(Self, attacks);
				playerAttackIndexMapMap[clientID] = newAttackIndexMap;
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, newAttackIndexMap });

				// Initialize weapons struct for client
				RValue weapons;
				g_RunnerInterface.StructCreate(&weapons);
				args[0] = &prevWeapons;
				args[1] = &weapons;

				getScriptFunction("gml_Script_variable_struct_copy")(Self, Other, returnVal, 2, args);

				g_ModuleInterface->CallBuiltin("struct_remove", { weapons, prevAttackID });

				addWeapon(weapons, newAttackIndexMap, attackID);

				RValue clientMainWeaponStruct = g_ModuleInterface->CallBuiltin("ds_map_find_value", { playerAttackIndexMapMap[clientID], attackID });
				RValue clientMainWeaponConfig = getInstanceVariable(clientMainWeaponStruct, GML_config);
				setInstanceVariable(clientMainWeaponConfig, GML_optionType, RValue("Weapon"));

				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, weapons });
				playerWeaponMap[clientID] = weapons;
				setInstanceVariable(Self, GML_weapons, weapons);

				RValue charPerks;
				g_RunnerInterface.StructCreate(&charPerks);
				RValue perksNamesArr = g_ModuleInterface->CallBuiltin("variable_struct_get_names", { getInstanceVariable(charData, GML_perks) });
				int perksNamesArrLen = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { perksNamesArr }).ToDouble()));
				for (int i = 0; i < perksNamesArrLen; i++)
				{
					g_ModuleInterface->CallBuiltin("variable_instance_set", { charPerks, perksNamesArr[i], 0.0 });
				}
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, charPerks });
				playerCharPerksMap[clientID] = charPerks;

				RValue newPerks;
				g_RunnerInterface.StructCreate(&newPerks);
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, newPerks });
				playerPerksMap[clientID] = newPerks;

				RValue newPerksMap = deepCopyMap(Self, perksMap);
				playerPerksMapMap[clientID] = newPerksMap;
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, newPerksMap });

				RValue newPlayerStatUps = g_ModuleInterface->CallBuiltin("variable_clone", { playerStatUps });
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, newPlayerStatUps });
				playerStatUpsMap[clientID] = newPlayerStatUps;

				RValue newAvailableWeaponCollabs;
				g_RunnerInterface.StructCreate(&newAvailableWeaponCollabs);
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, newAvailableWeaponCollabs });
				playerAvailableWeaponCollabsMap[clientID] = newAvailableWeaponCollabs;

				RValue weaponCollabs;
				g_RunnerInterface.StructCreate(&weaponCollabs);
				args[0] = &prevWeaponCollabs;
				args[1] = &weaponCollabs;

				getScriptFunction("gml_Script_variable_struct_copy")(Self, Other, returnVal, 2, args);
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, weaponCollabs });
				playerWeaponCollabsMap[clientID] = weaponCollabs;

				// swap attackIndex for the client player
				setInstanceVariable(attackController, GML_attackIndex, playerAttackIndexMapMap[clientID]);
				args[0] = &attackID;
				origAddAttackPlayerManagerOtherScript(Self, Other, returnVal, 1, args);

				setInstanceVariable(newCreatedChar, GML_specialid, getInstanceVariable(charData, GML_specID));
				setInstanceVariable(newCreatedChar, GML_specCD, getInstanceVariable(charData, GML_specCD));
				// temp code to set specUnlock
				// TODO: replace with the actual unlock variable
				setInstanceVariable(newCreatedChar, GML_specUnlock, RValue(true));

				// deal with outfit
				// deal with attacks copy
				// deal with playercharacter baseStats
				// deal with onCreate
				// deal with fandom
				// deal with cooking
				// deal with snapshot stats
				// deal with player manager snapshot player

				// override canControl for new player
				// TODO: Fix potential issue with iofi's ult overriding the canControl parameter
				setInstanceVariable(newCreatedChar, GML_canControl, RValue(false));

				clientUnpausedMap[clientID] = true;
			}

			for (auto& curPlayer : playerMap)
			{
				uint32_t playerID = curPlayer.first;
				RValue playerInstance = curPlayer.second;
				attacksCopyMap[playerID] = getInstanceVariable(playerInstance, GML_attacks);
			}

			for (auto& curClientIDMapping : clientIDToSteamIDMap)
			{
				uint32_t playerID = curClientIDMapping.first;
				RValue newRerollContainer;
				g_RunnerInterface.StructCreate(&newRerollContainer);
				rerollContainerMap[playerID] = newRerollContainer;
				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, newRerollContainer });
			}
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, rerollContainer });

			setInstanceVariable(Self, GML_playerCharacter, playerCharacter);

			g_ModuleInterface->CallBuiltin("variable_global_set", { "playerAttacks", globalPlayerAttacks });
			g_ModuleInterface->CallBuiltin("variable_global_set", { "charSelected", prevCharData });
			g_ModuleInterface->CallBuiltin("variable_instance_set", { Self, "weapons", prevWeapons });
			setInstanceVariable(attackController, GML_attackIndex, playerAttackIndexMapMap[HOST_INDEX]);
		}
	}
	return ReturnValue;
}

RValue& CanSubmitScoreFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected)
	{
		ReturnValue.m_Kind = VALUE_BOOL;
		ReturnValue.m_Real = 0;
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}
	return ReturnValue;
}

RValue& StopPlayerCreateFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && !isHost)
	{
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}
	return ReturnValue;
}

RValue& InitPlayerManagerCreateFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && !isHost)
	{
		// TODO: temp code to get playermanager working
		RValue characterDataMap = g_ModuleInterface->CallBuiltin("variable_global_get", { "characterData" });
		RValue charData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { characterDataMap, lobbyPlayerDataMap[clientID].charName.c_str() });
		g_ModuleInterface->CallBuiltin("variable_global_set", { "charSelected", charData });
		g_ModuleInterface->CallBuiltin("variable_global_set", { "outfitSelected", "default" });
	}
	return ReturnValue;
}

RValue& InitPlayerManagerCreateFuncAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && !isHost)
	{
		RValue options = getInstanceVariable(Self, GML_options);
		for (int i = 0; i < 4; i++)
		{
			RValue optionStruct;
			g_RunnerInterface.StructCreate(&optionStruct);
			g_RunnerInterface.StructAddDouble(&optionStruct, "optionIcon", 0);
			g_RunnerInterface.StructAddString(&optionStruct, "optionType", "");
			g_RunnerInterface.StructAddString(&optionStruct, "optionName", "");
			g_RunnerInterface.StructAddString(&optionStruct, "optionDescription", "");
			g_RunnerInterface.StructAddDouble(&optionStruct, "becomeSuper", 0);
			g_RunnerInterface.StructAddString(&optionStruct, "optionID", "");
			g_RunnerInterface.StructAddString(&optionStruct, "attackID", "");
			g_RunnerInterface.StructAddString(&optionStruct, "itemType", "");
			g_RunnerInterface.StructAddString(&optionStruct, "weaponType", "");
			options[i] = optionStruct;
		}
	}
	return ReturnValue;
}

RValue& MovePlayerCreateFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && !isHost)
	{
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}
	return ReturnValue;
}

bool isClientPaused = false;
int swapPausePlayerID = -1;
RValue& PausePlayerManagerCreateFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			sendAllHostHasPausedMessage();
			swapPausePlayerID = curPlayerID;
			if (curPlayerID != 0)
			{
				RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
				swapPlayerDataPush(Self, attackController, 0);
			}
		}
		else
		{
			isClientPaused = true;
			if (!(!hasObtainedClientID || !isPlayerCreatedMap[clientID]))
			{
				setInstanceVariable(playerMap[clientID], GML_canControl, RValue(false));
			}
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
	return ReturnValue;
}

RValue& PausePlayerManagerCreateFuncAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			if (swapPausePlayerID != -1 && swapPausePlayerID != 0)
			{
				RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
				swapPlayerDataPop(Self, attackController);
			}
			swapPausePlayerID = -1;
		}
	}
	return ReturnValue;
}

RValue& CreateTakodachiAttackControllerOther14FuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		// TODO: Should probably send the flag to make takodachi transparent. Maybe just include it in the message struct itself?
	}
	return ReturnValue;
}

RValue& GLRMeshDestroyFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		auto mapInstance = instanceToIDMap.find(Self);
		if (mapInstance == instanceToIDMap.end())
		{
			printf("Couldn't find instance %p for glr_mesh_destroy\n", Self);
			return ReturnValue;
		}

		uint16_t instanceID = mapInstance->second.instanceID;
		instanceToIDMap.erase(Self);

		instancesDeleteMessage.addInstance(instanceID);
		if (instancesDeleteMessage.numInstances >= instanceDeleteDataLen)
		{
			sendAllInstanceDeleteMessage();
		}
		availableInstanceIDs.push(instanceID);
	}
	return ReturnValue;
}

optionType convertStringOptionTypeToEnum(RValue optionType)
{
	std::string optionTypeString = optionType.ToString();
	if (optionTypeString.compare("StatUp") == 0)
	{
		return optionType_StatUp;
	}
	if (optionTypeString.compare("Weapon") == 0)
	{
		return optionType_Weapon;
	}
	if (optionTypeString.compare("RemoveWeapon") == 0)
	{
		return optionType_RemoveWeapon;
	}
	if (optionTypeString.compare("RemoveItem") == 0)
	{
		return optionType_RemoveItem;
	}
	if (optionTypeString.compare("RemoveSkill") == 0)
	{
		return optionType_RemoveSkill;
	}
	if (optionTypeString.compare("Enchant") == 0)
	{
		return optionType_Enchant;
	}
	if (optionTypeString.compare("Collab") == 0)
	{
		return optionType_Collab;
	}
	if (optionTypeString.compare("SuperCollab") == 0)
	{
		return optionType_SuperCollab;
	}
	if (optionTypeString.compare("Skill") == 0)
	{
		return optionType_Skill;
	}
	if (optionTypeString.compare("Item") == 0)
	{
		return optionType_Item;
	}
	if (optionTypeString.compare("Consumable") == 0)
	{
		return optionType_Consumable;
	}
	return optionType_NONE;
}

void swapPlayerDataHelper(CInstance* playerManagerInstance, RValue attackController, uint32_t playerID)
{
	if (playerID == 100000 || curPlayerID == playerID)
	{
		if (playerID == 100000)
		{
			// TODO: Seems like this occurs pretty often for some weapons. Need to investigate further
//			g_ModuleInterface->Print(CM_RED, "Failed to swap player ID");
//			callbackManagerInterfacePtr->LogToFile(MODNAME, "Failed to swap player ID");
		}
		return;
	}
	curPlayerID = playerID;
	setInstanceVariable(playerManagerInstance, GML_playerCharacter, playerMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_weapons, playerWeaponMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_charPerks, playerCharPerksMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_ITEMS, playerItemsMapMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_items, playerItemsMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_perks, playerPerksMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_attacksCopy, attacksCopyMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_PERKS, playerPerksMapMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_playerStatUps, playerStatUpsMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_eliminatedAttacks, eliminatedAttacksMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_removedItems, removedItemsMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_rerollContainer, rerollContainerMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_playerSummon, summonMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_customDrawScript, customDrawScriptMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_availableWeaponCollabs, playerAvailableWeaponCollabsMap[playerID]);
	setInstanceVariable(playerManagerInstance, GML_weaponCollabs, playerWeaponCollabsMap[playerID]);
	setInstanceVariable(attackController, GML_attackIndex, playerAttackIndexMapMap[playerID]);
	g_ModuleInterface->CallBuiltin("variable_global_set", { "currentStickers", currentStickersMap[playerID] });
	g_ModuleInterface->CallBuiltin("variable_global_set", { "charSelected", charSelectedMap[playerID] });
}

void swapPlayerDataPush(CInstance* playerManagerInstance, RValue attackController, uint32_t playerID)
{
	curPlayerIDStack.push_back(playerID);
	swapPlayerDataHelper(playerManagerInstance, attackController, playerID);
}

void swapPlayerDataPop(CInstance* playerManagerInstance, RValue attackController)
{
	curPlayerIDStack.pop_back();
	uint32_t playerID = curPlayerIDStack.back();
	swapPlayerDataHelper(playerManagerInstance, attackController, playerID);
}

bool isHostInLevelUp = false;

RValue& LevelUpPlayerManagerFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		if (isHostInLevelUp)
		{
			return ReturnValue;
		}
		RValue keepAliveRerollContainerArr = g_ModuleInterface->CallBuiltin("array_create", { static_cast<int>(clientIDToSteamIDMap.size()) });
		g_ModuleInterface->CallBuiltin("variable_global_set", { "keepAliveRerollContainerArr", keepAliveRerollContainerArr });
		rerollContainerMap.clear();
		rerollContainerMap[HOST_INDEX] = getInstanceVariable(Self, GML_rerollContainer);
		int clientSocketIndex = 0;
		for (auto& curClientIDMapping : clientIDToSteamIDMap)
		{
			RValue newRerollContainer;
			g_RunnerInterface.StructCreate(&newRerollContainer);
			rerollContainerMap[curClientIDMapping.first] = newRerollContainer;
			keepAliveRerollContainerArr[clientSocketIndex] = newRerollContainer;
			clientSocketIndex++;
		}
		isHostInLevelUp = true;
		RValue result;
		RValue options = getInstanceVariable(Self, GML_options);
		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });

		RValue prevOptions[4];
		for (int i = 0; i < 4; i++)
		{
			prevOptions[i] = options[i];
		}

		for (auto& curClientIDMapping : clientIDToSteamIDMap)
		{
			uint32_t playerID = curClientIDMapping.first;
			swapPlayerDataPush(Self, attackController, playerID);
			origGeneratePossibleOptionsScript(Self, Other, result, 0, nullptr);
			origOptionOneScript(Self, Other, result, 0, nullptr);
			options[0] = result;
			origOptionTwoScript(Self, Other, result, 0, nullptr);
			options[1] = result;
			origOptionThreeScript(Self, Other, result, 0, nullptr);
			options[2] = result;
			origOptionFourScript(Self, Other, result, 0, nullptr);
			options[3] = result;
			sendClientLevelUpOptionsMessage(playerID);
			optionType optionType0 = convertStringOptionTypeToEnum(getInstanceVariable(options[0], GML_optionType));
			optionType optionType1 = convertStringOptionTypeToEnum(getInstanceVariable(options[1], GML_optionType));
			optionType optionType2 = convertStringOptionTypeToEnum(getInstanceVariable(options[2], GML_optionType));
			optionType optionType3 = convertStringOptionTypeToEnum(getInstanceVariable(options[3], GML_optionType));
			levelUpOptionNamesMap[playerID] = levelUpOptionNames(
				std::make_pair(optionType0, std::string(getInstanceVariable(options[0], GML_optionID).ToString())),
				std::make_pair(optionType1, std::string(getInstanceVariable(options[1], GML_optionID).ToString())),
				std::make_pair(optionType2, std::string(getInstanceVariable(options[2], GML_optionID).ToString())),
				std::make_pair(optionType3, std::string(getInstanceVariable(options[3], GML_optionID).ToString()))
			);
			clientUnpausedMap[playerID] = false;
			swapPlayerDataPop(Self, attackController);
		}

		for (int i = 0; i < 4; i++)
		{
			options[i] = prevOptions[i];
		}
	}
	return ReturnValue;
}

RValue& LevelUpPlayerManagerFuncAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		/*
		for (int i = 0; i < clientSocketList.size(); i++)
		{
			sendClientLevelUpOptionsMessage(clientSocketList[i], i + 1);
		}
		*/
	}
	return ReturnValue;
}

RValue& ConfirmedPlayerManagerFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected)
	{
		if (!isHost)
		{
			// TODO: Could probably run the original code if it's on the pause menu
			// TODO: Could probably run the original code if it's on the quit confirm
			// TODO: Might have to do something specific for game over?
			// TODO: Do something specific for reviving
			RValue canControl = getInstanceVariable(Self, GML_canControl);
			if (!canControl.ToBoolean())
			{
				callbackManagerInterfacePtr->CancelOriginalFunction();
				return ReturnValue;
			}
			RValue paused = getInstanceVariable(Self, GML_paused);
			if (!paused.ToBoolean())
			{
				callbackManagerInterfacePtr->CancelOriginalFunction();
				return ReturnValue;
			}
			RValue leveled = getInstanceVariable(Self, GML_leveled);
			RValue gotBox = getInstanceVariable(Self, GML_gotBox);
			RValue gotSticker = getInstanceVariable(Self, GML_gotSticker);
			RValue gotGoldenAnvil = getInstanceVariable(Self, GML_gotGoldenAnvil);
			RValue gameOvered = getInstanceVariable(Self, GML_gameOvered);
			RValue gameWon = getInstanceVariable(Self, GML_gameWon);
			RValue quitConfirm = getInstanceVariable(Self, GML_quitConfirm);
			if (leveled.ToBoolean())
			{
				// Not sure if controls free check is necessary
				RValue controlsFree = getInstanceVariable(Self, GML_controlsFree);
				if (!controlsFree.ToBoolean())
				{
					callbackManagerInterfacePtr->CancelOriginalFunction();
					return ReturnValue;
				}
				RValue collabListShowing = getInstanceVariable(Self, GML_collabListShowing);
				if (collabListShowing.ToBoolean())
				{
					callbackManagerInterfacePtr->CancelOriginalFunction();
					return ReturnValue;
				}
				RValue collabListSelected = getInstanceVariable(Self, GML_collabListSelected);
				if (collabListSelected.ToBoolean())
				{
					return ReturnValue;
				}
				RValue levelOptionSelect = getInstanceVariable(Self, GML_levelOptionSelect);
				char selectedOption = static_cast<char>(lround(levelOptionSelect.ToDouble()));
				// Run the original eliminate code
				if (selectedOption == 5)
				{
					return ReturnValue;
				}
				// Send a reroll request
				if (selectedOption == 4)
				{
					// TODO: Should probably set the reroll amount and eliminate amount to whatever the host originally has
					RValue rerollTimes = g_ModuleInterface->CallBuiltin("variable_global_get", { "rerollTimes" });
					if (static_cast<int>(lround(rerollTimes.ToDouble())) >= 1)
					{
						// TODO: Check if the message failed to send
						sendLevelUpClientChoiceMessage(0, selectedOption);
						setInstanceVariable(Self, GML_eliminateMode, RValue(false));
						// TODO: Maybe should wait until an acknowledgement is received from the host before reducing the count (low priority)
						g_ModuleInterface->CallBuiltin("variable_global_set", { "rerollTimes", rerollTimes.ToDouble() - 1 });
						callbackManagerInterfacePtr->CancelOriginalFunction();
					}
					return ReturnValue;
				}
				RValue eliminateMode = getInstanceVariable(Self, GML_eliminateMode);
				if (eliminateMode.ToBoolean())
				{
					RValue options = getInstanceVariable(Self, GML_options);
					optionType levelUpType = convertStringOptionTypeToEnum(getInstanceVariable(options[selectedOption], GML_optionType));
					if (levelUpType == optionType_Consumable || levelUpType == optionType_StatUp)
					{
						return ReturnValue;
					}
					RValue eliminateTimes = g_ModuleInterface->CallBuiltin("variable_global_get", { "eliminateTimes" });
					g_ModuleInterface->CallBuiltin("variable_global_set", { "eliminateTimes", eliminateTimes.ToDouble() - 1 });
					setInstanceVariable(Self, GML_eliminateMode, RValue(false));
					setInstanceVariable(Self, GML_eliminatedThisLevel, RValue(true));
					sendEliminateLevelUpClientChoiceMessage(0, selectedOption);
					setInstanceVariable(options[selectedOption], GML_optionName, RValue(""));
					setInstanceVariable(options[selectedOption], GML_optionIcon, RValue(sprEmptyIndex));
					setInstanceVariable(options[selectedOption], GML_optionDescription, RValue(""));
					setInstanceVariable(options[selectedOption], GML_optionType, RValue(""));
					callbackManagerInterfacePtr->CancelOriginalFunction();
					return ReturnValue;
				}

				RValue options = getInstanceVariable(Self, GML_options);
				if (getInstanceVariable(options[selectedOption], GML_optionName).ToString().empty())
				{
					callbackManagerInterfacePtr->CancelOriginalFunction();
					return ReturnValue;
				}

				setInstanceVariable(Self, GML_eliminatedThisLevel, RValue(false));
				// TODO: Check if the message failed to send
				sendLevelUpClientChoiceMessage(0, selectedOption);

				levelUpPausedData levelUpData = levelUpPausedData(
					0,
					convertStringOptionTypeToEnum(getInstanceVariable(options[selectedOption], GML_optionType)),
					getInstanceVariable(options[selectedOption], GML_optionID)
				);
				processLevelUp(levelUpData, Self);

				RValue playerManager = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerManagerIndex, 0 });
				setInstanceVariable(playerManager, GML_paused, RValue(false));
				setInstanceVariable(playerManager, GML_leveled, RValue(false));
				setInstanceVariable(playerManager, GML_controlsFree, RValue(false));
				setInstanceVariable(playerMap[clientID], GML_canControl, RValue(true));
				isClientPaused = false;

				callbackManagerInterfacePtr->CancelOriginalFunction();
			}
			else if (gotBox.ToBoolean())
			{
				if (getInstanceVariable(Self, GML_boxOpenned).ToBoolean())
				{
					if (static_cast<int>(lround(getInstanceVariable(Self, GML_itemBoxTakeOption).ToDouble())) == 0)
					{
						sendBoxTakeOptionMessage(HOST_INDEX, static_cast<char>(lround(getInstanceVariable(Self, GML_currentBoxItem).ToDouble())));
					}
					else if (getInstanceVariable(Self, GML_superBox).ToBoolean())
					{
						// Need to do this since the super item check only checks from the items map
						RValue randomWeaponArr = getInstanceVariable(playerManagerInstanceVar, GML_randomWeapon);
						RValue optionID = getInstanceVariable(randomWeaponArr[0], GML_optionID);
						RValue itemsMap = getInstanceVariable(playerManagerInstanceVar, GML_ITEMS);
						RValue curItem = g_ModuleInterface->CallBuiltin("ds_map_find_value", { itemsMap, optionID });
						setInstanceVariable(curItem, GML_becomeSuper, RValue(false));
						setInstanceVariable(curItem, GML_optionIcon, getInstanceVariable(curItem, GML_optionIcon_Normal));
						// Required to tell the host that the client dropped the super item and to set the item back to normal
						sendBoxTakeOptionMessage(HOST_INDEX, 100);
					}
				}
			}
			else if (gotSticker.ToBoolean())
			{
				RValue stickerActionSelected = getInstanceVariable(Self, GML_stickerActionSelected);
				if (stickerActionSelected.ToBoolean())
				{
					int stickerAction = static_cast<int>(lround(getInstanceVariable(Self, GML_stickerAction).ToDouble()));
					int stickerOption = static_cast<int>(lround(getInstanceVariable(Self, GML_stickerOption).ToDouble()));
					sendStickerChooseOptionMessage(HOST_INDEX, stickerOption, stickerAction);
					if (stickerAction == 1)
					{
						// Remove the sticker locally since it will be recreated on the host side
						g_ModuleInterface->CallBuiltin("variable_global_get", { "currentStickers" })[stickerOption - 1] = -1.0;
					}
				}
			}
			else if (gotGoldenAnvil.ToBoolean())
			{
				RValue collabDone = getInstanceVariable(Self, GML_collabDone);
				if (collabDone.ToBoolean())
				{
					RValue collabingWeapon = getInstanceVariable(Self, GML_collabingWeapon);
					RValue attackID = getInstanceVariable(collabingWeapon, GML_attackID);
					RValue optionType = getInstanceVariable(collabingWeapon, GML_optionType);
					levelUpOption curOption = levelUpOption(optionType.ToString(), "", attackID.ToString(), std::vector<std::string>(), 0, 0, 0);
					sendChooseCollabMessage(HOST_INDEX, curOption);
				}
			}
			else if (gameOvered.ToBoolean() || gameWon.ToBoolean())
			{
				RValue gameOverTime = getInstanceVariable(Self, GML_gameOverTime);
				if (!(gameOvered.ToBoolean() && gameOverTime.ToDouble() < 330) && !(gameWon.ToBoolean() && gameOverTime.ToDouble() < 120))
				{
					int pauseOption = static_cast<int>(lround(getInstanceVariable(Self, GML_pauseOption).ToDouble()));
					if (pauseOption == 2)
					{
						clientLeaveGame(false);
						callbackManagerInterfacePtr->CancelOriginalFunction();
					}
					else
					{
						// TODO: Maybe should hide the retry options from the client since they can't use them anyways?
						callbackManagerInterfacePtr->CancelOriginalFunction();
					}
				}
			}
			else if (quitConfirm.ToBoolean())
			{
				int quitOption = static_cast<int>(lround(getInstanceVariable(Self, GML_quitOption).ToDouble()));
				if (quitOption == 0)
				{
					clientLeaveGame(false);
					callbackManagerInterfacePtr->CancelOriginalFunction();
				}
			}
		}
		else
		{
			RValue canControl = getInstanceVariable(Self, GML_canControl);
			if (!canControl.ToBoolean())
			{
				callbackManagerInterfacePtr->CancelOriginalFunction();
				return ReturnValue;
			}
			RValue paused = getInstanceVariable(Self, GML_paused);
			if (!paused.ToBoolean())
			{
				callbackManagerInterfacePtr->CancelOriginalFunction();
				return ReturnValue;
			}
			RValue gameOvered = getInstanceVariable(Self, GML_gameOvered);
			RValue gameWon = getInstanceVariable(Self, GML_gameWon);
			RValue quitConfirm = getInstanceVariable(Self, GML_quitConfirm);
			if (gameOvered.ToBoolean() || gameWon.ToBoolean())
			{
				RValue gameOverTime = getInstanceVariable(Self, GML_gameOverTime);
				if (!(gameOvered.ToBoolean() && gameOverTime.ToDouble() < 330) && !(gameWon.ToBoolean() && gameOverTime.ToDouble() < 120))
				{
					int pauseOption = static_cast<int>(lround(getInstanceVariable(Self, GML_pauseOption).ToDouble()));
					if (pauseOption == 0)
					{
						// TODO: Change retry to just restart the game again with all the clients. For now, just bring everyone back to the lobby
						holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, lobbyMenuGrid.menuGridPtr);
						lobbyPlayerDataMap[HOST_INDEX].stageSprite = -1;
						g_ModuleInterface->CallBuiltin("variable_global_set", { "resetLevel", true });
						g_ModuleInterface->CallBuiltin("room_restart", {});
						g_ModuleInterface->CallBuiltin("room_goto", { rmTitle });
						g_ModuleInterface->CallBuiltin("instance_destroy", { Self });
						sendAllReturnToLobbyMessage();
						cleanupPlayerGameData();
					}
					else if (pauseOption == 1)
					{
						// TODO: Bring everyone back to the lobby
						holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, lobbyMenuGrid.menuGridPtr);
						lobbyPlayerDataMap[HOST_INDEX].stageSprite = -1;
						g_ModuleInterface->CallBuiltin("variable_global_set", { "resetLevel", true });
						g_ModuleInterface->CallBuiltin("room_restart", {});
						g_ModuleInterface->CallBuiltin("room_goto", { rmTitle });
						g_ModuleInterface->CallBuiltin("instance_destroy", { Self });
						sendAllReturnToLobbyMessage();
						cleanupPlayerGameData();
					}
					else if (pauseOption == 2)
					{
						// TODO: Add code that disconnects steam connections as well
						for (auto& clientSocket : clientSocketMap)
						{
							closesocket(clientSocket.second);
						}
						hasConnected = false;
						if (messageHandlerThread.joinable())
						{
							messageHandlerThread.join();
						}
						steamLobbyBrowser->leaveLobby();
						cleanupPlayerGameData();
						cleanupPlayerClientData();
						if (steamHost)
						{
							delete steamHost;
							steamHost = nullptr;
							isHost = false;
						}
						std::shared_ptr<menuGridData> nullptrMenu = nullptr;
						holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, nullptrMenu);
						hasSelectedMap = false;
						g_ModuleInterface->CallBuiltin("variable_global_set", { "resetLevel", true });
						g_ModuleInterface->CallBuiltin("room_restart", {});
						g_ModuleInterface->CallBuiltin("room_goto", { rmTitle });
						g_ModuleInterface->CallBuiltin("instance_destroy", { Self });
					}
					callbackManagerInterfacePtr->CancelOriginalFunction();
				}
			}
			else if (quitConfirm.ToBoolean())
			{
				int quitOption = static_cast<int>(lround(getInstanceVariable(Self, GML_quitOption).ToDouble()));
				if (quitOption == 0)
				{
					for (auto& clientSocket : clientSocketMap)
					{
						closesocket(clientSocket.second);
					}
					hasConnected = false;
					if (messageHandlerThread.joinable())
					{
						messageHandlerThread.join();
					}
					steamLobbyBrowser->leaveLobby();
					cleanupPlayerGameData();
					cleanupPlayerClientData();
					if (steamHost)
					{
						delete steamHost;
						steamHost = nullptr;
						isHost = false;
					}
					std::shared_ptr<menuGridData> nullptrMenu = nullptr;
					holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, nullptrMenu);
					hasSelectedMap = false;
					g_ModuleInterface->CallBuiltin("variable_global_set", { "resetLevel", true });
					g_ModuleInterface->CallBuiltin("room_restart", {});
					g_ModuleInterface->CallBuiltin("room_goto", { rmTitle });
					g_ModuleInterface->CallBuiltin("instance_destroy", { Self });
				}
			}
		}
	}
	return ReturnValue;
}

void unsetPauseMenu()
{
	setInstanceVariable(playerManagerInstanceVar, GML_perksMenu, RValue(false));
	setInstanceVariable(playerManagerInstanceVar, GML_collabsMenu, RValue(false));
	setInstanceVariable(playerManagerInstanceVar, GML_pauseCurrentMenu, RValue(0.0));
	setInstanceVariable(playerManagerInstanceVar, GML_quitConfirm, RValue(false));
	setInstanceVariable(playerManagerInstanceVar, GML_gotSticker, RValue(false));
	setInstanceVariable(playerManagerInstanceVar, GML_canControl, RValue(true));
	g_ModuleInterface->CallBuiltin("instance_destroy", { objOptionsIndex });
}

void unpauseHost()
{
	isAnyInteracting = false;
	unsetPauseMenu();
	RValue returnVal;
	origUnpauseScript(playerManagerInstanceVar, nullptr, returnVal, 0, nullptr);
	setInstanceVariable(playerManagerInstanceVar, GML_paused, RValue(false));
	sendAllHostHasUnpausedMessage();
	// Make sure to actually delete the paused screen
	RValue inputArgs[1];
	inputArgs[0] = getInstanceVariable(playerManagerInstanceVar, GML_paused_screen_sprite);
	origSpriteDeleteScript(returnVal, playerManagerInstanceVar, nullptr, 1, inputArgs);
}

bool isHostWaitingForClientUnpause = false;

RValue& UnpausePlayerManagerFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			// TODO: Prevent the paused menu from showing up as well
			RValue paused = getInstanceVariable(Self, GML_paused);
			// Need to check if the game is now unpaused or not because sometimes this function will be called without actually unpausing the game
			if (!paused.ToBoolean())
			{
				// Probably shouldn't be necessary to check if the host is in the level up menu because it's not paused anymore
				isHostInLevelUp = false;
				bool isAnyClientPaused = false;
				for (auto& curClientUnpaused : clientUnpausedMap)
				{
					bool isClientUnpaused = curClientUnpaused.second;
					if (!isClientUnpaused)
					{
						isAnyClientPaused = true;
						break;
					}
				}
				if (!hasClientFinishedInteracting)
				{
					// Pause the game again until the client has finished interacting
					setInstanceVariable(Self, GML_paused, RValue(true));
					callbackManagerInterfacePtr->CancelOriginalFunction();
					return ReturnValue;
				}
				if (isAnyClientPaused)
				{
					// Pause the game again until all the clients have finished
					setInstanceVariable(Self, GML_paused, RValue(true));
					isHostWaitingForClientUnpause = true;
					callbackManagerInterfacePtr->CancelOriginalFunction();
				}
				else
				{
					unpauseHost();
					callbackManagerInterfacePtr->CancelOriginalFunction();
				}
			}
		}
		else
		{
			isClientPaused = false;
			if (!(!hasObtainedClientID || !isPlayerCreatedMap[clientID]))
			{
				setInstanceVariable(playerMap[clientID], GML_canControl, RValue(true));
			}

			if (isClientUsingBox)
			{
				isClientUsingBox = false;
				sendInteractFinishedMessage(HOST_INDEX);
				uint32_t boxCoinGain = static_cast<uint32_t>(floor(getInstanceVariable(Self, GML_boxCoinGain).ToDouble()));
				sendClientGainMoneyMessage(HOST_INDEX, boxCoinGain);

				// Need to manually remove certain instances created while paused since the game originally created them in a different room which would automatically destroy them when moved,
				// but that doesn't work now since I prevented it from moving rooms while paused
				g_ModuleInterface->CallBuiltin("instance_destroy", { objItemLightBeamIndex });
			}
			else if (isClientUsingAnvil)
			{
				RValue usedAnvil = getInstanceVariable(Self, GML_usedAnvil);
				if (usedAnvil.ToBoolean())
				{
					RValue loadOutList = getInstanceVariable(Self, GML_loadOutList);
					int anvilOption = static_cast<int>(lround(getInstanceVariable(Self, GML_anvilOption).ToDouble()));
					RValue loadOut = loadOutList[anvilOption];
					RValue optionID = getInstanceVariable(loadOut, GML_optionID);
					RValue optionType = getInstanceVariable(loadOut, GML_optionType);

					int upgradeOption = static_cast<int>(lround(getInstanceVariable(Self, GML_upgradeOption).ToDouble()));
					if (upgradeOption == 0)
					{
						RValue enhancing = getInstanceVariable(Self, GML_enhancing);
						// Enhancing anvil
						if (enhancing.ToBoolean())
						{
							// Attempted to enhance weapon
							RValue enhanceResult = getInstanceVariable(Self, GML_enhanceResult);
							if (enhanceResult.ToBoolean())
							{
								RValue enhanceCostMult = g_ModuleInterface->CallBuiltin("variable_global_get", { "enhanceCostMultiplier" });
								RValue enhancements = getInstanceVariable(loadOut, GML_enhancements);
								uint32_t enhanceCost = static_cast<uint32_t>(floor(enhanceCostMult.ToDouble() * enhancements.ToDouble() * 50));
								sendAnvilChooseOptionMessage(HOST_INDEX, optionID.ToString(), optionType.ToString(), enhanceCost, 1);
							}
						}
						else
						{
							// Levelled up weapon
							sendAnvilChooseOptionMessage(HOST_INDEX, optionID.ToString(), optionType.ToString(), 0, 0);
						}
					}
					else if (upgradeOption == 1)
					{
						// Enchanting anvil
						RValue enhanceCostMult = g_ModuleInterface->CallBuiltin("variable_global_get", { "enhanceCostMultiplier" });
						uint32_t enhanceCost = static_cast<uint32_t>(floor(enhanceCostMult.ToDouble() * 250));
						sendClientAnvilEnchantMessage(HOST_INDEX, optionID.ToString(), currentAnvilRolledMods, enhanceCost);
					}
					setInstanceVariable(Self, GML_usedAnvil, RValue(false));
					g_ModuleInterface->CallBuiltin("instance_destroy", { objItemLightBeamIndex });
				}
				isClientUsingAnvil = false;
				sendInteractFinishedMessage(HOST_INDEX);
			}
			else if (isClientUsingGoldenAnvil)
			{
				setInstanceVariable(Self, GML_usedAnvil, RValue(false));
				isClientUsingGoldenAnvil = false;
				sendInteractFinishedMessage(HOST_INDEX);
				g_ModuleInterface->CallBuiltin("instance_destroy", { objItemLightBeamIndex });
			}
			else if (isClientUsingStamp)
			{
				isClientUsingStamp = false;
				sendInteractFinishedMessage(HOST_INDEX);
			}
		}
	}
	return ReturnValue;
}

RValue& ParseAndPushCommandTypePlayerManagerFuncAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		// Make sure that it doesn't try to run the command since the level hasn't switched yet
		if (isHostWaitingForClientUnpause)
		{
			RValue disableAlarmVal = -1;
			g_ModuleInterface->SetBuiltin("alarm", Self, 9, disableAlarmVal);
		}
	}
	return ReturnValue;
}

RValue& ExecuteAttackBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		RValue* attacker = Args[1];
		size_t playerID = 100000;
		if (attacker->m_Kind == VALUE_REAL && static_cast<int>(lround(attacker->m_Real)) == 227)
		{
			playerID = curPlayerID;
			*Args[1] = playerMap[static_cast<uint32_t>(playerID)];
		}
		else
		{
			// Apparently the Arg field can also be the attack itself instead of the creator? Need to check if it's an attack or player as well
			RValue creator = getInstanceVariable(*attacker, GML_creator);
			if (creator.m_Kind != VALUE_UNDEFINED && creator.m_Kind != VALUE_UNSET)
			{
				// Assume that it's an attack if it has a creator
				RValue config = getInstanceVariable(*attacker, GML_config);
				if (config.m_Kind != VALUE_UNDEFINED && config.m_Kind != VALUE_UNSET)
				{
					RValue origPlayerCreator = getInstanceVariable(config, GML_origPlayerCreator);
					if (origPlayerCreator.m_Kind != VALUE_UNDEFINED && origPlayerCreator.m_Kind != VALUE_UNSET)
					{
						playerID = getPlayerID(origPlayerCreator.ToInstance());
					}
					else
					{
						playerID = getPlayerID(getInstanceVariable(creator, GML_id).ToInstance());
					}
				}
				else
				{
					playerID = getPlayerID(getInstanceVariable(creator, GML_id).ToInstance());
				}
			}
			else
			{
				playerID = getPlayerID(getInstanceVariable(*attacker, GML_id).ToInstance());
			}
		}
		// Check if the index is a valid player or not
		if (playerID != 100000)
		{
			// TODO: Should probably have some more checks for cases where ExecuteAttack uses object number instead of the actual instance id (Eg. summon code)
			// TODO: Check if this works properly in all cases
			RValue* overrideConfig = Args[2];
			// TODO: Seems like this can crash if overrideConfig is nullptr (Mio ult). Maybe consider creating a struct and setting it to the args?
			if (numArgs > 2 && overrideConfig != nullptr && overrideConfig->m_Kind == VALUE_OBJECT)
			{
				setInstanceVariable(*Args[2], GML_origPlayerCreator, playerMap[static_cast<uint32_t>(playerID)]);
				g_ModuleInterface->CallBuiltin("variable_instance_set", { *overrideConfig, "summonSource", *attacker });
			}
			// TODO: Should probably keep track of what is the last player data to avoid needing to do this multiple times.
			// TODO: Could probably move some of this into player step to also avoid swapping player data since it will be the same player
		}
		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
		swapPlayerDataPush(playerManagerInstanceVar, attackController, static_cast<int>(playerID));
	}
	return ReturnValue;
}

RValue& ExecuteAttackAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
		swapPlayerDataPop(playerManagerInstanceVar, attackController);
	}
	return ReturnValue;
}

RValue& OnCollideWithTargetAttackBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		// TODO: Replace this code with calculating if the enemy is in the view of all players or not
		g_ModuleInterface->CallBuiltin("variable_instance_set", { *Args[0], "inView", true });
	}
	return ReturnValue;
}

RValue& DieObstacleCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).ToDouble());
		float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).ToDouble());
		short destructableID = static_cast<short>(lround(getInstanceVariable(Self, GML_destructableID).ToDouble()));
		sendAllDestructableBreakMessage(destructableData(xPos, yPos, destructableID, 0));
	}

	return ReturnValue;
}

RValue& ExecuteSpecialAttackBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && !isHost)
	{
		sendClientSpecialAttackMessage(0);
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}

	return ReturnValue;
}

RValue& ApplyBuffAttackControllerBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		RValue* creator = Args[0];
		uint32_t playerID = 100000;
		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
		if (isInBaseMobOnDeath)
		{
			// TODO: Find a good general fix for which player should get the buff
			callbackManagerInterfacePtr->CancelOriginalFunction();
			swapPlayerDataPush(playerManagerInstanceVar, attackController, playerID);
			return ReturnValue;
		}
		RValue isPlayer = getInstanceVariable(*creator, GML_isPlayer);
		if (isPlayer.m_Kind == VALUE_UNDEFINED || isPlayer.m_Kind == VALUE_UNSET)
		{
			swapPlayerDataPush(playerManagerInstanceVar, attackController, playerID);
			return ReturnValue;
		}
		if (creator->m_Kind == VALUE_REAL)
		{
			/*
			printf("Applying to first player\n");
			Args[0] = &playerList[curPlayerIndex];
			*/
			// TODO: Find a good general fix for which player should get the buff
//			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
		else
		{
			playerID = getPlayerID(getInstanceVariable(*creator, GML_id).ToInstance());
		}
		swapPlayerDataPush(playerManagerInstanceVar, attackController, playerID);
	}
	return ReturnValue;
}

RValue& ApplyBuffAttackControllerAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		// TODO: Could probably check the index to see if it's the original player or not to avoid an unnecessary swap
		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
		swapPlayerDataPop(playerManagerInstanceVar, attackController);
	}
	return ReturnValue;
}

RValue& DestroyHoloAnvilBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		short interactableMapIndexVal = static_cast<short>(lround(getInstanceVariable(Self, GML_interactableMapIndex).ToDouble()));
		sendAllInteractableDeleteMessage(interactableMapIndexVal, 1);
	}
	return ReturnValue;
}

RValue& DestroyGoldenAnvilBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		short interactableMapIndexVal = static_cast<short>(lround(getInstanceVariable(Self, GML_interactableMapIndex).ToDouble()));
		sendAllInteractableDeleteMessage(interactableMapIndexVal, 2);
	}
	return ReturnValue;
}

RValue& DestroyStickerBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		short interactableMapIndexVal = static_cast<short>(lround(getInstanceVariable(Self, GML_interactableMapIndex).ToDouble()));
		sendAllInteractableDeleteMessage(interactableMapIndexVal, 4);
	}
	return ReturnValue;
}

bool hasPlayerTakenDamage = false;
bool hasEnemyTakenDamage = false;

RValue& TakeDamageBaseMobCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		uint32_t playerID = getPlayerID(getInstanceVariable(Self, GML_id).ToInstance());
		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
		if (playerID != 100000)
		{
			// Player is taking damage
			hasPlayerTakenDamage = true;
		}
		else
		{
			// Enemy is taking damage
			RValue creator = getInstanceVariable(*Args[1], GML_creator);
			RValue config = getInstanceVariable(*Args[1], GML_config);
			if (config.m_Kind != VALUE_UNDEFINED && config.m_Kind != VALUE_UNSET)
			{
				RValue origPlayerCreator = getInstanceVariable(config, GML_origPlayerCreator);
				if (origPlayerCreator.m_Kind != VALUE_UNDEFINED && origPlayerCreator.m_Kind != VALUE_UNSET)
				{
					creator = origPlayerCreator;
				}
			}
			playerID = getPlayerID(creator.ToInstance());
			hasEnemyTakenDamage = true;
		}
		swapPlayerDataPush(playerManagerInstanceVar, attackController, playerID);
	}
	return ReturnValue;
}

RValue& TakeDamageBaseMobCreateAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
		hasPlayerTakenDamage = false;
		hasEnemyTakenDamage = false;
		swapPlayerDataPop(playerManagerInstanceVar, attackController);
	}
	return ReturnValue;
}

RValue& RollModAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && !isHost)
	{
		// TODO: Check if this will have issues if RollMod returns -1. Not really sure when that happens
		currentAnvilRolledMods.push_back(ReturnValue.ToString());
	}
	return ReturnValue;
}

RValue& ApplyBuffsPlayerManagerBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && !isHost)
	{
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}
	return ReturnValue;
}

SOCKET listenSocket;

extern int adapterPageNum;
extern int prevPageButtonNum;
extern int nextPageButtonNum;

RValue& ConfirmedTitleScreenBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	return ReturnValue;
}

extern std::unordered_map<uint32_t, clientMovementQueueData> lastTimeReceivedMoveDataMap;

void cleanupPlayerGameData()
{
	playerMap.clear();
	playerWeaponMap.clear();
	playerCharPerksMap.clear();
	playerItemsMapMap.clear();
	playerItemsMap.clear();
	playerAttackIndexMapMap.clear();
	playerPerksMap.clear();
	playerPerksMapMap.clear();
	playerStatUpsMap.clear();
	playerAvailableWeaponCollabsMap.clear();
	playerWeaponCollabsMap.clear();
	attacksCopyMap.clear();
	removedItemsMap.clear();
	eliminatedAttacksMap.clear();
	rerollContainerMap.clear();
	currentStickersMap.clear();
	charSelectedMap.clear();
	playerDataMap.clear();
	curPlayerIDStack.clear();
	isPlayerCreatedMap.clear();
	lastTimeReceivedMoveDataMap.clear();
	summonMap.clear();
	customDrawScriptMap.clear();

	instanceToIDMap.clear();
	pickupableToIDMap.clear();
	preCreateMap.clear();
	vfxMap.clear();
	interactableMap.clear();
	hasClientFinishedInteracting = true;
	isAnyInteracting = false;
	collidedStickerID = "";
	availableInstanceIDs = {};
	availableAttackIDs = {};
	availablePickupableIDs = {};
	availablePreCreateIDs = {};
	availableVFXIDs = {};
	availableInteractableIDs = {};
	levelUpPausedList = {};
	currentAnvilRolledMods = {};
	destructableMap.clear();

	clientCamPosX = 0;
	clientCamPosY = 0;

	curPlayerID = 0;
	curSelectedStageSprite = -1;

	timeNum = 0;

	isClientUsingBox = false;
	isClientUsingAnvil = false;
	isClientUsingGoldenAnvil = false;
	isClientUsingStamp = false;
	hasSent = false;
	hasHostPaused = false;
}

void cleanupPlayerClientData()
{
	numClientsInGame = -1;
	hasObtainedClientID = false;
	curUnusedPlayerID = 1;
	playerPingMap.clear();
	clientUnpausedMap.clear();
	clientSocketMap.clear();
	lobbyPlayerDataMap.clear();
	hasClientPlayerDisconnected.clear();
	steamIDToClientIDMap.clear();
	clientIDToSteamIDMap.clear();
	if (isHost)
	{
		for (auto& messageQueue : steamIDToConnectionMap)
		{
			if (messageQueue.second.curMessage != nullptr)
			{
				messageQueue.second.curMessage->Release();
			}
			SteamNetworkingSockets()->CloseConnection(messageQueue.second.curConnection, 0, nullptr, false);
		}
	}
	else
	{
		steamConnection& curSteamConnection = steamLobbyBrowser->getSteamLobbyHostConnection();
		if (curSteamConnection.curMessage != nullptr)
		{
			curSteamConnection.curMessage->Release();
		}
		SteamNetworkingSockets()->CloseConnection(curSteamConnection.curConnection, 0, nullptr, false);
	}
	steamIDToConnectionMap.clear();
	for (auto& listenForClientSocket : listenForClientSocketMap)
	{
		closesocket(listenForClientSocket.second);
	}
	listenForClientSocketMap.clear();
}

bool hasReturnedFromSelectingCharacter = false;
bool hasReturnedFromSelectingMap = false;
RValue& ReturnMenuTitleScreenBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	return ReturnValue;
}

RValue& ReturnCharSelectCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	callbackManagerInterfacePtr->CancelOriginalFunction();
	return ReturnValue;
}

RValue& SelectCharSelectCreateAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	RValue holoHouseMode = g_ModuleInterface->CallBuiltin("variable_global_get", { "holoHouseMode" });
	if (!holoHouseMode.ToBoolean())
	{
		std::shared_ptr<menuGridData> curMenuGridDataPtr;
		holoCureMenuInterfacePtr->GetCurrentMenuGrid(MODNAME, curMenuGridDataPtr);
		if (curMenuGridDataPtr.get() == selectingCharacterMenuGrid.menuGridPtr.get())
		{
			RValue charSelected = g_ModuleInterface->CallBuiltin("variable_global_get", { "charSelected" });
			if (charSelected.m_Kind == VALUE_OBJECT)
			{
				// Send player back to the lobby after selecting a character
				// TODO: Allow for outfit selection later
				// TODO: There might be an issue if the player left clicks and right clicks at the same time?
				lobbyPlayerDataMap[clientID].charName = getInstanceVariable(charSelected, GML_id).ToString();
				callbackManagerInterfacePtr->LogToFile(MODNAME, "Selected character %s", lobbyPlayerDataMap[clientID].charName.data());
				lobbyPlayerDataMap[clientID].stageSprite = curSelectedStageSprite;
				holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, lobbyMenuGrid.menuGridPtr);
				curSelectedSteamID = CSteamID();
				curSteamLobbyMemberIndex = 0;
				g_ModuleInterface->CallBuiltin("instance_destroy", { objCharSelectIndex });
			}
		}
		else if (curMenuGridDataPtr.get() == selectingMapMenuGrid.menuGridPtr.get())
		{
			int stageSelected = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("variable_global_get", { "playingStage" }).ToDouble()));
			if (stageSelected != -1)
			{
				// Send player back to the lobby after selecting a map
				hasSelectedMap = true;
				curSelectedMap = stageSelected;
				curSelectedGameMode = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("variable_global_get", { "gameMode" }).ToDouble()));
				RValue whichSet = getInstanceVariable(Self, GML_availableStages);
				int selectedStage = static_cast<int>(lround(getInstanceVariable(Self, GML_selectedStage).ToDouble()));
				if (curSelectedGameMode == 0 || curSelectedGameMode == 1)
				{
					curSelectedStageSprite = static_cast<int>(lround(getInstanceVariable(whichSet[selectedStage], GML_stageSprite).ToDouble()));
					lobbyPlayerDataMap[HOST_INDEX].stageSprite = curSelectedStageSprite;
				}
				holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, lobbyMenuGrid.menuGridPtr);
				curSelectedSteamID = CSteamID();
				curSteamLobbyMemberIndex = 0;
				g_ModuleInterface->CallBuiltin("instance_destroy", { objCharSelectIndex });
			}
		}
	}
	return ReturnValue;
}

bool isInBaseMobOnDeath = false;
RValue& OnDeathBaseMobCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	isInBaseMobOnDeath = true;
	return ReturnValue;
}

RValue& OnDeathBaseMobCreateAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	isInBaseMobOnDeath = false;
	return ReturnValue;
}

RValue& UpdatePlayerPlayerManagerOtherBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected)
	{
		if (!isHost)
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
		else
		{
			
		}
	}
	return ReturnValue;
}

RValue& AddPerkPlayerManagerOtherAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			auto curSummon = summonMap[curPlayerID];
			if (curSummon.m_Kind == VALUE_UNDEFINED)
			{
				RValue playerManager = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerManagerIndex, 0 });
				RValue playerSummon = getInstanceVariable(playerManager, GML_playerSummon);
				if (playerSummon.m_Kind == VALUE_OBJECT || playerSummon.m_Kind == VALUE_REF)
				{
					summonMap[curPlayerID] = playerSummon;
				}
				else if (playerSummon.m_Kind == VALUE_ARRAY)
				{
					// Add array to some keep alive array
					RValue keepAliveArr = g_ModuleInterface->CallBuiltin("variable_global_get", { "keepAliveArr" });
					g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, playerSummon });
					summonMap[curPlayerID] = playerSummon;
				}
				else
				{
					// Seems like the host couldn't find the summon?
//					g_ModuleInterface->Print(CM_RED, "Couldn't find player summon %d", playerSummon.m_Kind);
				}
			}
		}
	}
	return ReturnValue;
}

RValue& ParseAndPushCommandTypePlayerManagerOtherBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			callbackManagerInterfacePtr->LogToFile(MODNAME, "ParseAndPush %s", Args[0]->ToString().data());
		}
		else
		{
			if (isClientInInitializeCharacter)
			{
				callbackManagerInterfacePtr->CancelOriginalFunction();
			}
		}
	}
	return ReturnValue;
}

bool isInCreateSummon = false;
RValue& CreateSummonMobManagerCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		isInCreateSummon = true;
	}
	return ReturnValue;
}

RValue& CreateSummonMobManagerCreateAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		isInCreateSummon = false;
	}
	return ReturnValue;
}

RValue& HealBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		uint32_t playerID = getPlayerID(getInstanceVariable(*Args[0], GML_id).ToInstance());
		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
		swapPlayerDataPush(playerManagerInstanceVar, attackController, static_cast<int>(playerID));
	}
	return ReturnValue;
}

RValue& HealAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		uint32_t playerID = getPlayerID(getInstanceVariable(*Args[0], GML_id).ToInstance());
		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
		swapPlayerDataPop(playerManagerInstanceVar, attackController);
	}
	return ReturnValue;
}