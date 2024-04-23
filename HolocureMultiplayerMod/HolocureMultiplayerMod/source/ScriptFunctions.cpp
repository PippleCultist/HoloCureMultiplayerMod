#pragma comment(lib, "iphlpapi.lib")
#include <WS2tcpip.h>
#include "ScriptFunctions.h"
#include <YYToolkit/shared.hpp>
#include <CallbackManager/CallbackManagerInterface.h>
#include "ModuleMain.h"
#include "CommonFunctions.h"
#include "NetworkFunctions.h"
#include "CodeEvents.h"
#include <iphlpapi.h>
#include <fstream>

#define HOST_INDEX 0

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
	g_RunnerInterface.StructAddRValue(&weapons, attackID.AsString().data(), &mainWeapon);
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

double moneyGainMultiplier = 0;

int curPlayerID = 0;

bool isClientInInitializeCharacter = false;

RValue deepCopyStruct(CInstance* Self, RValue& origStruct, RValue* parentStructPtr);
RValue deepCopyArray(CInstance* Self, RValue& origArray, RValue* parentStructPtr);

RValue deepCopyArray(CInstance* Self, RValue& origArray, RValue* parentStructPtr)
{
	int origArrayLen = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { origArray }).m_Real));
	RValue copiedArray = g_ModuleInterface->CallBuiltin("array_create", { origArrayLen });
	for (int i = 0; i < origArrayLen; i++)
	{
		RValue curArrayVal = origArray[i];
		// Apparently methods are considered as structs also, so the check for is_method needs to go before is_struct
		if (g_ModuleInterface->CallBuiltin("is_array", { curArrayVal }).AsBool())
		{
			RValue copiedCurArrayVal = deepCopyArray(Self, curArrayVal, parentStructPtr);
			copiedArray[i] = copiedCurArrayVal;
		}
		else if (g_ModuleInterface->CallBuiltin("is_method", { curArrayVal }).AsBool())
		{
			RValue methodSelf = g_ModuleInterface->CallBuiltin("method_get_self", { curArrayVal });
			if (methodSelf.m_Kind == VALUE_REAL)
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
		else if (g_ModuleInterface->CallBuiltin("is_struct", { curArrayVal }).AsBool())
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
	int structNamesLen = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { structNames }).m_Real));
	for (int i = 0; i < structNamesLen; i++)
	{
		RValue curStructName = structNames[i];
		RValue curStructVal = g_ModuleInterface->CallBuiltin("variable_instance_get", { origStruct, curStructName });
		if (g_ModuleInterface->CallBuiltin("is_array", { curStructVal }).AsBool())
		{
			RValue copiedCurStructVal = deepCopyArray(Self, curStructVal, parentStructPtr);
			g_RunnerInterface.StructAddRValue(&copiedStruct, curStructName.AsString().data(), &copiedCurStructVal);
		}
		else if (g_ModuleInterface->CallBuiltin("is_method", { curStructVal }).AsBool())
		{
			RValue methodSelf = g_ModuleInterface->CallBuiltin("method_get_self", { curStructVal });
			if (methodSelf.m_Kind == VALUE_REAL)
			{
				// Assume that the method can be safely copied since it's bound to a script function method
				g_RunnerInterface.StructAddRValue(&copiedStruct, curStructName.AsString().data(), &curStructVal);
			}
			else if (methodSelf.m_Kind == VALUE_OBJECT)
			{
				// Make sure to rebind the copied method to the current instance
				// TODO: Seems like the methods defined in a struct will retain information about their parent even if you copy them to a different object of the same type.
				// TODO: Should probably find a way to only copy these functions while assigning the parent as the new struct and not doing that for all the other methods
				RValue copiedMethod = g_ModuleInterface->CallBuiltin("method", { *parentStructPtr, curStructVal });
				g_RunnerInterface.StructAddRValue(&copiedStruct, curStructName.AsString().data(), &copiedMethod);
			}
			else
			{
				g_ModuleInterface->Print(CM_RED, "Unhandled kind %d for method %s", methodSelf.m_Kind, curStructName.AsString().data());
			}
		}
		else if (g_ModuleInterface->CallBuiltin("is_struct", { curStructVal }).AsBool())
		{
			RValue copiedCurStructVal = deepCopyStruct(Self, curStructVal, parentStructPtr);
			g_RunnerInterface.StructAddRValue(&copiedStruct, curStructName.AsString().data(), &copiedCurStructVal);
		}
		else
		{
			g_RunnerInterface.StructAddRValue(&copiedStruct, curStructName.AsString().data(), &curStructVal);
		}
	}
	return copiedStruct;
}

void addCollabData(RValue& weapons, RValue& attacks)
{
	RValue attacksKeysArr = g_ModuleInterface->CallBuiltin("ds_map_keys_to_array", { attacks });
	int attacksKeysArrLength = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { attacksKeysArr }).m_Real));
	for (int i = 0; i < attacksKeysArrLength; i++)
	{
		RValue curKey = attacksKeysArr[i];
		RValue attackObj = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacks, curKey });
		RValue attackObjConfig = g_ModuleInterface->CallBuiltin("variable_instance_get", { attackObj, "config" });
		RValue attackObjConfigOptionType = g_ModuleInterface->CallBuiltin("variable_instance_get", { attackObjConfig, "optionType" });
		RValue attackObjConfigCombos = g_ModuleInterface->CallBuiltin("variable_instance_get", { attackObjConfig, "combos" });
		RValue attackObjAttackID = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attackObj, "attackID" });

		if (attackObjConfigOptionType.AsString().compare("Collab") == 0)
		{
			// TODO: Make a new weaponCollabs struct and add to it
			if (g_ModuleInterface->CallBuiltin("variable_struct_exists", { weapons, attackObjConfigCombos[0] }).AsBool() &&
				g_ModuleInterface->CallBuiltin("variable_struct_exists", { weapons, attackObjConfigCombos[1] }).AsBool())
			{
				RValue weaponOne = g_ModuleInterface->CallBuiltin("variable_instance_get", { weapons, attackObjConfigCombos[0] });
				RValue weaponTwo = g_ModuleInterface->CallBuiltin("variable_instance_get", { weapons, attackObjConfigCombos[1] });
				RValue weaponOneCombos = g_ModuleInterface->CallBuiltin("variable_instance_get", { weaponOne, "combos" });
				RValue weaponTwoCombos = g_ModuleInterface->CallBuiltin("variable_instance_get", { weaponTwo, "combos" });
				g_ModuleInterface->CallBuiltin("variable_instance_set", { weaponOneCombos, attackObjConfigCombos[1], attackObjAttackID });
				g_ModuleInterface->CallBuiltin("variable_instance_set", { weaponTwoCombos, attackObjConfigCombos[0], attackObjAttackID });
			}
		}
		else if (attackObjConfigOptionType.AsString().compare("SuperCollab") == 0)
		{
			// TODO: Add super collab code
		}
	}
}

RValue deepCopyMap(CInstance* Self, RValue& origMap)
{
	RValue origMapKeys = g_ModuleInterface->CallBuiltin("ds_map_keys_to_array", { origMap });
	int origMapKeysLength = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { origMapKeys }).m_Real));
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
			playerStatUpsMap.clear();
			summonMap.clear();
			customDrawScriptMap.clear();

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

			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			RValue attacks = getInstanceVariable(attackController, GML_attackIndex);
			playerAttackIndexMapMap[HOST_INDEX] = attacks;
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, attacks });

			RValue playerSave = g_ModuleInterface->CallBuiltin("variable_global_get", { "PlayerSave" });
			RValue unlockedWeapons = g_ModuleInterface->CallBuiltin("ds_map_find_value", { playerSave, "unlockedWeapons" });
			int unlockedWeaponsLength = static_cast<int>(g_ModuleInterface->CallBuiltin("array_length", { unlockedWeapons }).m_Real);

			RValue prevWeapons = getInstanceVariable(Self, GML_weapons);
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, prevWeapons });
			playerWeaponMap[HOST_INDEX] = prevWeapons;

			RValue prevCharPerks = g_ModuleInterface->CallBuiltin("variable_clone", { getInstanceVariable(prevCharData, GML_perks) });
			g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, prevCharPerks });
			playerCharPerksMap[HOST_INDEX] = prevCharPerks;

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
			for (auto& curClientSocket : clientSocketMap)
			{
				uint32_t clientID = curClientSocket.first;

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
				RValue charData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { characterDataMap, lobbyPlayerDataMap[clientID].charName });
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
				moneyGainMultiplier = getInstanceVariable(baseStats, GML_moneyGain).m_Real;
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

				addWeapon(weapons, newAttackIndexMap, attackID);

				for (int i = 0; i < unlockedWeaponsLength; i++)
				{
					addWeapon(weapons, newAttackIndexMap, unlockedWeapons[i]);
				}

				addCollabData(weapons, newAttackIndexMap);

				// TODO: Make this scalable for more players
				RValue clientMainWeaponStruct = g_ModuleInterface->CallBuiltin("ds_map_find_value", { playerAttackIndexMapMap[clientID], attackID });
				RValue clientMainWeaponConfig = getInstanceVariable(clientMainWeaponStruct, GML_config);
				setInstanceVariable(clientMainWeaponConfig, GML_optionType, RValue("Weapon"));

				g_ModuleInterface->CallBuiltin("array_push", { keepAliveArr, weapons });
				playerWeaponMap[clientID] = weapons;
				setInstanceVariable(Self, GML_weapons, weapons);

				RValue charPerks = g_ModuleInterface->CallBuiltin("variable_clone", { getInstanceVariable(charData, GML_perks) });
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

			for (auto& curClientSocket : clientSocketMap)
			{
				uint32_t playerID = curClientSocket.first;
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
		RValue charData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { characterDataMap, lobbyPlayerDataMap[clientID].charName });
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
				swapPlayerData(Self, attackController, 0);
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
				swapPlayerData(Self, attackController, swapPausePlayerID);
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

		uint16_t instanceID = mapInstance->second;
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
	std::string_view optionTypeString = optionType.AsString();
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

void swapPlayerData(CInstance* playerManagerInstance, RValue attackController, uint32_t playerID)
{
	if (playerID == 100000 || curPlayerID == playerID)
	{
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
	setInstanceVariable(attackController, GML_attackIndex, playerAttackIndexMapMap[playerID]);
	g_ModuleInterface->CallBuiltin("variable_global_set", { "currentStickers", currentStickersMap[playerID] });
	g_ModuleInterface->CallBuiltin("variable_global_set", { "charSelected", charSelectedMap[playerID] });
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
		RValue keepAliveRerollContainerArr = g_ModuleInterface->CallBuiltin("array_create", { static_cast<int>(clientSocketMap.size()) });
		g_ModuleInterface->CallBuiltin("variable_global_set", { "keepAliveRerollContainerArr", keepAliveRerollContainerArr });
		rerollContainerMap.clear();
		rerollContainerMap[HOST_INDEX] = getInstanceVariable(Self, GML_rerollContainer);
		int clientSocketIndex = 0;
		for (auto& clientSocket : clientSocketMap)
		{
			RValue newRerollContainer;
			g_RunnerInterface.StructCreate(&newRerollContainer);
			rerollContainerMap[clientSocket.first] = newRerollContainer;
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

		for (auto& clientSocket : clientSocketMap)
		{
			uint32_t playerID = clientSocket.first;
			SOCKET curClientSocket = clientSocket.second;
			swapPlayerData(Self, attackController, playerID);
			origGeneratePossibleOptionsScript(Self, Other, result, 0, nullptr);
			origOptionOneScript(Self, Other, result, 0, nullptr);
			options[0] = result;
			origOptionTwoScript(Self, Other, result, 0, nullptr);
			options[1] = result;
			origOptionThreeScript(Self, Other, result, 0, nullptr);
			options[2] = result;
			origOptionFourScript(Self, Other, result, 0, nullptr);
			options[3] = result;
			sendClientLevelUpOptionsMessage(curClientSocket, playerID);
			optionType optionType0 = convertStringOptionTypeToEnum(getInstanceVariable(options[0], GML_optionType));
			optionType optionType1 = convertStringOptionTypeToEnum(getInstanceVariable(options[1], GML_optionType));
			optionType optionType2 = convertStringOptionTypeToEnum(getInstanceVariable(options[2], GML_optionType));
			optionType optionType3 = convertStringOptionTypeToEnum(getInstanceVariable(options[3], GML_optionType));
			levelUpOptionNamesMap[playerID] = levelUpOptionNames(
				std::make_pair(optionType0, std::string(getInstanceVariable(options[0], GML_optionID).AsString())),
				std::make_pair(optionType1, std::string(getInstanceVariable(options[1], GML_optionID).AsString())),
				std::make_pair(optionType2, std::string(getInstanceVariable(options[2], GML_optionID).AsString())),
				std::make_pair(optionType3, std::string(getInstanceVariable(options[3], GML_optionID).AsString()))
			);
			clientUnpausedMap[playerID] = false;
		}

		for (int i = 0; i < 4; i++)
		{
			options[i] = prevOptions[i];
		}
		swapPlayerData(Self, attackController, 0);
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
			if (!canControl.AsBool())
			{
				callbackManagerInterfacePtr->CancelOriginalFunction();
				return ReturnValue;
			}
			RValue paused = getInstanceVariable(Self, GML_paused);
			if (!paused.AsBool())
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
			if (leveled.AsBool())
			{
				// Not sure if controls free check is necessary
				RValue controlsFree = getInstanceVariable(Self, GML_controlsFree);
				if (!controlsFree.AsBool())
				{
					callbackManagerInterfacePtr->CancelOriginalFunction();
					return ReturnValue;
				}
				RValue collabListShowing = getInstanceVariable(Self, GML_collabListShowing);
				if (collabListShowing.AsBool())
				{
					callbackManagerInterfacePtr->CancelOriginalFunction();
					return ReturnValue;
				}
				RValue collabListSelected = getInstanceVariable(Self, GML_collabListSelected);
				if (collabListSelected.AsBool())
				{
					return ReturnValue;
				}
				RValue levelOptionSelect = getInstanceVariable(Self, GML_levelOptionSelect);
				char selectedOption = static_cast<char>(lround(levelOptionSelect.m_Real));
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
					if (static_cast<int>(lround(rerollTimes.m_Real)) >= 1)
					{
						// TODO: Check if the message failed to send
						sendLevelUpClientChoiceMessage(serverSocket, selectedOption);
						setInstanceVariable(Self, GML_eliminateMode, RValue(false));
						// TODO: Maybe should wait until an acknowledgement is received from the host before reducing the count (low priority)
						g_ModuleInterface->CallBuiltin("variable_global_set", { "rerollTimes", rerollTimes.m_Real - 1 });
						callbackManagerInterfacePtr->CancelOriginalFunction();
					}
					return ReturnValue;
				}
				RValue eliminateMode = getInstanceVariable(Self, GML_eliminateMode);
				if (eliminateMode.AsBool())
				{
					RValue options = getInstanceVariable(Self, GML_options);
					optionType levelUpType = convertStringOptionTypeToEnum(getInstanceVariable(options[selectedOption], GML_optionType));
					if (levelUpType == optionType_Consumable || levelUpType == optionType_StatUp)
					{
						return ReturnValue;
					}
					RValue eliminateTimes = g_ModuleInterface->CallBuiltin("variable_global_get", { "eliminateTimes" });
					g_ModuleInterface->CallBuiltin("variable_global_set", { "eliminateTimes", eliminateTimes.m_Real - 1 });
					setInstanceVariable(Self, GML_eliminateMode, RValue(false));
					setInstanceVariable(Self, GML_eliminatedThisLevel, RValue(true));
					sendEliminateLevelUpClientChoiceMessage(serverSocket, selectedOption);
					setInstanceVariable(options[selectedOption], GML_optionName, RValue(""));
					setInstanceVariable(options[selectedOption], GML_optionIcon, RValue(sprEmptyIndex));
					setInstanceVariable(options[selectedOption], GML_optionDescription, RValue(""));
					setInstanceVariable(options[selectedOption], GML_optionType, RValue(""));
					callbackManagerInterfacePtr->CancelOriginalFunction();
					return ReturnValue;
				}

				RValue options = getInstanceVariable(Self, GML_options);
				if (getInstanceVariable(options[selectedOption], GML_optionName).AsString().empty())
				{
					callbackManagerInterfacePtr->CancelOriginalFunction();
					return ReturnValue;
				}

				setInstanceVariable(Self, GML_eliminatedThisLevel, RValue(false));
				// TODO: Check if the message failed to send
				sendLevelUpClientChoiceMessage(serverSocket, selectedOption);

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
			else if (gotBox.AsBool())
			{
				if (getInstanceVariable(Self, GML_boxOpenned).AsBool())
				{
					if (static_cast<int>(lround(getInstanceVariable(Self, GML_itemBoxTakeOption).m_Real)) == 0)
					{
						sendBoxTakeOptionMessage(serverSocket, static_cast<char>(lround(getInstanceVariable(Self, GML_currentBoxItem).m_Real)));
					}
					else if (getInstanceVariable(Self, GML_superBox).AsBool())
					{
						// Need to do this since the super item check only checks from the items map
						RValue randomWeaponArr = getInstanceVariable(playerManagerInstanceVar, GML_randomWeapon);
						RValue optionID = getInstanceVariable(randomWeaponArr[0], GML_optionID);
						RValue itemsMap = getInstanceVariable(playerManagerInstanceVar, GML_ITEMS);
						RValue curItem = g_ModuleInterface->CallBuiltin("ds_map_find_value", { itemsMap, optionID });
						setInstanceVariable(curItem, GML_becomeSuper, RValue(false));
						setInstanceVariable(curItem, GML_optionIcon, getInstanceVariable(curItem, GML_optionIcon_Normal));
						// Required to tell the host that the client dropped the super item and to set the item back to normal
						sendBoxTakeOptionMessage(serverSocket, 100);
					}
				}
			}
			else if (gotSticker.AsBool())
			{
				RValue stickerActionSelected = getInstanceVariable(Self, GML_stickerActionSelected);
				if (stickerActionSelected.AsBool())
				{
					int stickerAction = static_cast<int>(lround(getInstanceVariable(Self, GML_stickerAction).m_Real));
					int stickerOption = static_cast<int>(lround(getInstanceVariable(Self, GML_stickerOption).m_Real));
					sendStickerChooseOptionMessage(serverSocket, stickerOption, stickerAction);
					if (stickerAction == 1)
					{
						// Check to make sure to swap the sticker sprite after swapping
						RValue selectedSticker = g_ModuleInterface->CallBuiltin("variable_global_get", { "currentStickers" })[stickerOption - 1];
						if (selectedSticker.m_Kind == VALUE_OBJECT)
						{
							RValue stickerID = getInstanceVariable(playerManagerInstanceVar, GML_stickerID);
							setInstanceVariable(stickerID, GML_sprite_index, getInstanceVariable(selectedSticker, GML_optionIcon));
						}
					}
				}
			}
			else if (gotGoldenAnvil.AsBool())
			{
				RValue collabDone = getInstanceVariable(Self, GML_collabDone);
				if (collabDone.AsBool())
				{
					RValue collabingWeapon = getInstanceVariable(Self, GML_collabingWeapon);
					RValue attackID = getInstanceVariable(collabingWeapon, GML_attackID);
					RValue optionType = getInstanceVariable(collabingWeapon, GML_optionType);
					levelUpOption curOption = levelUpOption(optionType.AsString(), "", attackID.AsString(), std::vector<std::string_view>(), 0, 0, 0);
					sendChooseCollabMessage(serverSocket, curOption);
				}
			}
			else if (gameOvered.AsBool() || gameWon.AsBool())
			{
				RValue gameOverTime = getInstanceVariable(Self, GML_gameOverTime);
				if (!(gameOvered.AsBool() && gameOverTime.m_Real < 330) && !(gameWon.AsBool() && gameOverTime.m_Real < 120))
				{
					int pauseOption = static_cast<int>(lround(getInstanceVariable(Self, GML_pauseOption).m_Real));
					if (pauseOption == 2)
					{
						serverDisconnected();
						callbackManagerInterfacePtr->CancelOriginalFunction();
					}
					else
					{
						// TODO: Maybe should hide the retry options from the client since they can't use them anyways?
						callbackManagerInterfacePtr->CancelOriginalFunction();
					}
				}
			}
			else if (quitConfirm.AsBool())
			{
				int quitOption = static_cast<int>(lround(getInstanceVariable(Self, GML_quitOption).m_Real));
				if (quitOption == 0)
				{
					serverDisconnected();
					callbackManagerInterfacePtr->CancelOriginalFunction();
				}
			}
		}
		else
		{
			RValue canControl = getInstanceVariable(Self, GML_canControl);
			if (!canControl.AsBool())
			{
				callbackManagerInterfacePtr->CancelOriginalFunction();
				return ReturnValue;
			}
			RValue paused = getInstanceVariable(Self, GML_paused);
			if (!paused.AsBool())
			{
				callbackManagerInterfacePtr->CancelOriginalFunction();
				return ReturnValue;
			}
			RValue gameOvered = getInstanceVariable(Self, GML_gameOvered);
			RValue gameWon = getInstanceVariable(Self, GML_gameWon);
			RValue quitConfirm = getInstanceVariable(Self, GML_quitConfirm);
			if (gameOvered.AsBool() || gameWon.AsBool())
			{
				RValue gameOverTime = getInstanceVariable(Self, GML_gameOverTime);
				if (!(gameOvered.AsBool() && gameOverTime.m_Real < 330) && !(gameWon.AsBool() && gameOverTime.m_Real < 120))
				{
					int pauseOption = static_cast<int>(lround(getInstanceVariable(Self, GML_pauseOption).m_Real));
					if (pauseOption == 0)
					{
						// TODO: Change retry to just restart the game again with all the clients. For now, just bring everyone back to the lobby
						isInLobby = true;
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
						isInLobby = true;
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
						for (auto& clientSocket : clientSocketMap)
						{
							closesocket(clientSocket.second);
						}
						hasConnected = false;
						cleanupPlayerGameData();
						cleanupPlayerClientData();
						isInLobby = false;
						isInCoopOptionMenu = false;
						hasSelectedMap = false;
						g_ModuleInterface->CallBuiltin("variable_global_set", { "resetLevel", true });
						g_ModuleInterface->CallBuiltin("room_restart", {});
						g_ModuleInterface->CallBuiltin("room_goto", { rmTitle });
						g_ModuleInterface->CallBuiltin("instance_destroy", { Self });
					}
					callbackManagerInterfacePtr->CancelOriginalFunction();
				}
			}
			else if (quitConfirm.AsBool())
			{
				int quitOption = static_cast<int>(lround(getInstanceVariable(Self, GML_quitOption).m_Real));
				if (quitOption == 0)
				{
					for (auto& clientSocket : clientSocketMap)
					{
						closesocket(clientSocket.second);
					}
					hasConnected = false;
					cleanupPlayerGameData();
					cleanupPlayerClientData();
					isInLobby = false;
					isInCoopOptionMenu = false;
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
	origSpriteDeleteScript(&returnVal, playerManagerInstanceVar, nullptr, 1, inputArgs);
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
			if (!paused.AsBool())
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
				sendInteractFinishedMessage(serverSocket);
				uint32_t boxCoinGain = static_cast<uint32_t>(floor(getInstanceVariable(Self, GML_boxCoinGain).m_Real));
				sendClientGainMoneyMessage(serverSocket, boxCoinGain);

				// Need to manually remove certain instances created while paused since the game originally created them in a different room which would automatically destroy them when moved,
				// but that doesn't work now since I prevented it from moving rooms while paused
				g_ModuleInterface->CallBuiltin("instance_destroy", { objItemLightBeamIndex });
			}
			else if (isClientUsingAnvil)
			{
				RValue usedAnvil = getInstanceVariable(Self, GML_usedAnvil);
				if (usedAnvil.AsBool())
				{
					RValue loadOutList = getInstanceVariable(Self, GML_loadOutList);
					int anvilOption = static_cast<int>(lround(getInstanceVariable(Self, GML_anvilOption).m_Real));
					RValue loadOut = loadOutList[anvilOption];
					RValue optionID = getInstanceVariable(loadOut, GML_optionID);
					RValue optionType = getInstanceVariable(loadOut, GML_optionType);

					int upgradeOption = static_cast<int>(lround(getInstanceVariable(Self, GML_upgradeOption).m_Real));
					if (upgradeOption == 0)
					{
						RValue enhancing = getInstanceVariable(Self, GML_enhancing);
						// Enhancing anvil
						if (enhancing.AsBool())
						{
							// Attempted to enhance weapon
							RValue enhanceResult = getInstanceVariable(Self, GML_enhanceResult);
							if (enhanceResult.AsBool())
							{
								RValue enhanceCostMult = g_ModuleInterface->CallBuiltin("variable_global_get", { "enhanceCostMultiplier" });
								RValue enhancements = getInstanceVariable(loadOut, GML_enhancements);
								uint32_t enhanceCost = static_cast<uint32_t>(floor(enhanceCostMult.m_Real * enhancements.m_Real * 50));
								sendAnvilChooseOptionMessage(serverSocket, optionID.AsString(), optionType.AsString(), enhanceCost, 1);
							}
						}
						else
						{
							// Levelled up weapon
							sendAnvilChooseOptionMessage(serverSocket, optionID.AsString(), optionType.AsString(), 0, 0);
						}
					}
					else if (upgradeOption == 1)
					{
						// Enchanting anvil
						RValue enhanceCostMult = g_ModuleInterface->CallBuiltin("variable_global_get", { "enhanceCostMultiplier" });
						uint32_t enhanceCost = static_cast<uint32_t>(floor(enhanceCostMult.m_Real * 250));
						sendClientAnvilEnchantMessage(serverSocket, optionID.AsString(), currentAnvilRolledMods, enhanceCost);
					}
					setInstanceVariable(Self, GML_usedAnvil, RValue(false));
					g_ModuleInterface->CallBuiltin("instance_destroy", { objItemLightBeamIndex });
				}
				isClientUsingAnvil = false;
				sendInteractFinishedMessage(serverSocket);
			}
			else if (isClientUsingGoldenAnvil)
			{
				setInstanceVariable(Self, GML_usedAnvil, RValue(false));
				isClientUsingGoldenAnvil = false;
				sendInteractFinishedMessage(serverSocket);
				g_ModuleInterface->CallBuiltin("instance_destroy", { objItemLightBeamIndex });
			}
			else if (isClientUsingStamp)
			{
				isClientUsingStamp = false;
				sendInteractFinishedMessage(serverSocket);
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
		RValue* creator = Args[1];
		size_t playerID = getPlayerID(getInstanceVariable(*creator, GML_id).m_Object);
		// Check if the index is a valid player or not
		if (playerID != 100000)
		{
			// TODO: Should probably have some more checks for cases where ExecuteAttack uses object number instead of the actual instance id (Eg. summon code)
			// TODO: Check if this works properly in all cases
			RValue* overrideConfig = Args[2];
			// TODO: Seems like this can crash if overrideConfig is nullptr (Mio ult). Maybe consider creating a struct and setting it to the args?
			if (overrideConfig != nullptr && overrideConfig->m_Kind == VALUE_OBJECT)
			{
				g_ModuleInterface->CallBuiltin("variable_instance_set", { *overrideConfig, "summonSource", *creator });
			}
			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			// TODO: Should probably keep track of what is the last player data to avoid needing to do this multiple times.
			// TODO: Could probably move some of this into player step to also avoid swapping player data since it will be the same player
			swapPlayerData(playerManagerInstanceVar, attackController, static_cast<int>(playerID));
		}
	}
	return ReturnValue;
}

RValue& ExecuteAttackAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		RValue* creator = Args[1];
		size_t playerID = getPlayerID(getInstanceVariable(*creator, GML_id).m_Object);
		// Check if the index is a valid player or not
		if (playerID != 100000 && playerID != 0)
		{
			// TODO: Check if this works properly in all cases
			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			swapPlayerData(playerManagerInstanceVar, attackController, 0);
		}
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
		float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
		float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);
		short destructableID = static_cast<short>(lround(getInstanceVariable(Self, GML_destructableID).m_Real));
		sendAllDestructableBreakMessage(destructableData(xPos, yPos, destructableID, 0));
	}

	return ReturnValue;
}

RValue& ExecuteSpecialAttackBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && !isHost)
	{
		sendClientSpecialAttackMessage(serverSocket);
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}

	return ReturnValue;
}

RValue& ApplyBuffAttackControllerBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		RValue* creator = Args[0];
		if (isInBaseMobOnDeath)
		{
			// TODO: Find a good general fix for which player should get the buff
			callbackManagerInterfacePtr->CancelOriginalFunction();
			return ReturnValue;
		}
		if (creator->m_Kind == VALUE_REAL)
		{
			/*
			printf("Applying to first player\n");
			Args[0] = &playerList[curPlayerIndex];
			*/
			// TODO: Find a good general fix for which player should get the buff
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
		else
		{
			uint32_t playerID = getPlayerID(getInstanceVariable(*creator, GML_id).m_Object);
			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			swapPlayerData(playerManagerInstanceVar, attackController, playerID);
		}
	}
	return ReturnValue;
}

RValue& ApplyBuffAttackControllerAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		// TODO: Could probably check the index to see if it's the original player or not to avoid an unnecessary swap
		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
		swapPlayerData(playerManagerInstanceVar, attackController, 0);
	}
	return ReturnValue;
}

RValue& DestroyHoloAnvilBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		short interactableMapIndexVal = static_cast<short>(lround(getInstanceVariable(Self, GML_interactableMapIndex).m_Real));
		sendAllInteractableDeleteMessage(interactableMapIndexVal, 1);
	}
	return ReturnValue;
}

RValue& DestroyGoldenAnvilBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		short interactableMapIndexVal = static_cast<short>(lround(getInstanceVariable(Self, GML_interactableMapIndex).m_Real));
		sendAllInteractableDeleteMessage(interactableMapIndexVal, 2);
	}
	return ReturnValue;
}

RValue& RollStickerAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		if (!getInstanceVariable(Self, GML_destroyIfNoneLeft).AsBool())
		{
			short spriteIndex = static_cast<short>(lround(getInstanceVariable(Self, GML_sprite_index).m_Real));
			float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
			float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);
			RValue interactableMapIndex = getInstanceVariable(Self, GML_interactableMapIndex);
			if (interactableMapIndex.m_Kind == VALUE_UNSET || interactableMapIndex.m_Kind == VALUE_UNDEFINED)
			{
				short interactableID = availableInteractableIDs.front();
				availableInteractableIDs.pop();
				interactableMapIndex = RValue(static_cast<double>(interactableID));
				setInstanceVariable(Self, GML_interactableMapIndex, interactableMapIndex);
			}

			short interactableMapIndexVal = static_cast<short>(lround(interactableMapIndex.m_Real));
			sendAllInteractableCreateMessage(interactableData(xPos, yPos, interactableMapIndexVal, spriteIndex, 4));
		}
	}
	return ReturnValue;
}

RValue& DestroyStickerBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		short interactableMapIndexVal = static_cast<short>(lround(getInstanceVariable(Self, GML_interactableMapIndex).m_Real));
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
		uint32_t playerID = getPlayerID(getInstanceVariable(Self, GML_id).m_Object);
		if (playerID != 100000)
		{
			// Player is taking damage
			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			swapPlayerData(playerManagerInstanceVar, attackController, playerID);
			hasPlayerTakenDamage = true;
		}
		else
		{
			// Enemy is taking damage
			playerID = getPlayerID(getInstanceVariable(*Args[1], GML_creator).m_Object);
			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			swapPlayerData(playerManagerInstanceVar, attackController, playerID);
			hasEnemyTakenDamage = true;
		}
	}
	return ReturnValue;
}

RValue& TakeDamageBaseMobCreateAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && isHost)
	{
		if (hasPlayerTakenDamage)
		{
			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			swapPlayerData(playerManagerInstanceVar, attackController, 0);
			hasPlayerTakenDamage = false;
		}
		if (hasEnemyTakenDamage)
		{
			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			swapPlayerData(playerManagerInstanceVar, attackController, 0);
			hasEnemyTakenDamage = false;
		}
	}
	return ReturnValue;
}

RValue& RollModAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (hasConnected && !isHost)
	{
		// TODO: Check if this will have issues if RollMod returns -1. Not really sure when that happens
		currentAnvilRolledMods.push_back(ReturnValue.AsString());
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

bool isInNetworkAdapterMenu = false;
bool hasReadNetworkAdapterDisclaimer = false;
bool isInGamemodeSelect = false;
bool isInCoopOptionMenu = false;
bool isInLobby = false;
bool isSelectingCharacter = false;
bool isSelectingMap = false;
SOCKET listenSocket;

SOCKET broadcastSocket = INVALID_SOCKET;
sockaddr* broadcastSocketAddr = nullptr;
size_t broadcastSocketLen = 0;

extern int adapterPageNum;
extern int prevPageButtonNum;
extern int nextPageButtonNum;
IP_ADAPTER_ADDRESSES* adapterAddresses(NULL);

RValue& ConfirmedTitleScreenBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (isSelectingCharacter || isSelectingMap)
	{

	}
	else if (isInLobby)
	{
		// In the modded lobby
		if (curCoopOptionMenuIndex == 0)
		{
			// Choose character
			isInLobby = false;
			isSelectingCharacter = true;
			lobbyPlayerDataMap[clientID].isReady = 0;
			g_ModuleInterface->CallBuiltin("instance_create_depth", { 0, 0, 0, objCharSelectIndex });
		}
		else if (curCoopOptionMenuIndex == 1)
		{
			// Ready button
			if (!lobbyPlayerDataMap[clientID].charName.empty())
			{
				lobbyPlayerDataMap[clientID].isReady = 1 - lobbyPlayerDataMap[clientID].isReady;
			}
		}
		else if (curCoopOptionMenuIndex == 2)
		{
			// Choose map button
			isInLobby = false;
			isSelectingMap = true;
			RValue charSelectInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { 0, 0, 0, objCharSelectIndex });
			RValue characterDataMap = g_ModuleInterface->CallBuiltin("variable_global_get", { "characterData" });
			RValue charData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { characterDataMap, lobbyPlayerDataMap[clientID].charName });
			g_ModuleInterface->CallBuiltin("variable_global_set", { "charSelected", charData });
			// TODO: Need to also set the outfit once I add that
			RValue availableOutfitsArr = g_ModuleInterface->CallBuiltin("array_create", { 1, "default" });
			setInstanceVariable(charSelectInstance, GML_availableOutfits, availableOutfitsArr);
		}
		else if (curCoopOptionMenuIndex == 3)
		{
			// Start game button
			hasSelectedMap = false;
			isInLobby = false;
			closesocket(connectClientSocket);
			connectClientSocket = INVALID_SOCKET;
			closesocket(broadcastSocket);
			broadcastSocket = INVALID_SOCKET;
			g_ModuleInterface->CallBuiltin("room_goto", { curSelectedMap });
		}
	}
	else if (isInCoopOptionMenu)
	{
		// In the modded coop option menu
		playerPingMap.clear();
		lobbyPlayerDataMap.clear();
		if (curCoopOptionMenuIndex == 0)
		{
			hasClientPlayerDisconnected.clear();
			lobbyPlayerDataMap[0] = lobbyPlayerData();
			struct addrinfo hints, *servinfo, *p = NULL;
			memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			hints.ai_flags = AI_PASSIVE;

			char optval = '1';

			getaddrinfo(NULL, GAME_PORT, &hints, &servinfo);

			for (p = servinfo; p != NULL; p = p->ai_next)
			{
				connectClientSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
				setsockopt(connectClientSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
				bind(connectClientSocket, p->ai_addr, static_cast<int>(p->ai_addrlen));
				listen(connectClientSocket, SOMAXCONN);
				u_long mode = 1;
				ioctlsocket(connectClientSocket, FIONBIO, &mode);
				break;
			}

			freeaddrinfo(servinfo);

			// TODO: Let the host decide their own name eventually
			lobbyPlayerDataMap[0].playerName = "0";
			isHost = true;
		}
		else
		{
			isHost = false;
		}
		isInLobby = true;
		isInCoopOptionMenu = false;
	}
	else if (isInNetworkAdapterMenu)
	{
		if (!hasReadNetworkAdapterDisclaimer)
		{
			hasReadNetworkAdapterDisclaimer = true;
			adapterPageNum = 0;
			prevPageButtonNum = -1;
			nextPageButtonNum = -1;
			DWORD adapterAddressesBufferSize = 16 * 1024;

			for (int i = 0; i < 3; i++)
			{
				adapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(malloc(adapterAddressesBufferSize));
				DWORD error = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME, NULL, adapterAddresses, &adapterAddressesBufferSize);
				if (error == ERROR_SUCCESS)
				{
					break;
				}
				else if (error == ERROR_BUFFER_OVERFLOW)
				{
					free(adapterAddresses);
					adapterAddresses = NULL;
					continue;
				}
				else
				{
					free(adapterAddresses);
					adapterAddresses = NULL;
					continue;
				}
			}
		}
		else
		{
			if (curCoopOptionMenuIndex == prevPageButtonNum)
			{
				prevPageButtonNum = -1;
				nextPageButtonNum = -1;
				adapterPageNum--;
			}
			else if (curCoopOptionMenuIndex == nextPageButtonNum)
			{
				prevPageButtonNum = -1;
				nextPageButtonNum = -1;
				adapterPageNum++;
			}
			else
			{
				IP_ADAPTER_ADDRESSES* adapter(NULL);

				int count = -1;
				for (adapter = adapterAddresses; adapter != NULL; adapter = adapter->Next)
				{
					if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
					{
						continue;
					}

					bool isValidAddress = true;
					for (IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; address != NULL; address = address->Next)
					{
						auto family = address->Address.lpSockaddr->sa_family;
						if (family == AF_INET)
						{
							SOCKADDR_IN* ipv4 = reinterpret_cast<SOCKADDR_IN*>(address->Address.lpSockaddr);
							inet_ntop(AF_INET, &(ipv4->sin_addr), broadcastAddressBuffer, 16);

							if (strncmp("169.254", broadcastAddressBuffer, 7) == 0)
							{
								isValidAddress = false;
							}
							break;
						}
					}

					if (!isValidAddress)
					{
						continue;
					}

					count++;
					if (adapterPageNum * 5 + curCoopOptionMenuIndex != count)
					{
						continue;
					}

					CreateDirectory(L"MultiplayerMod", NULL);
					std::ofstream outFile;
					outFile.open("MultiplayerMod/lastUsedNetworkAdapter");
					int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, static_cast<int>(wcslen(adapter->FriendlyName)), NULL, 0, NULL, NULL);
					std::string resString(sizeNeeded, 0);
					WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, static_cast<int>(wcslen(adapter->FriendlyName)), &resString[0], sizeNeeded, NULL, NULL);
					outFile << resString;
					outFile.close();

					for (IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; address != NULL; address = address->Next)
					{
						auto family = address->Address.lpSockaddr->sa_family;
						if (family == AF_INET)
						{
							SOCKADDR_IN* ipv4 = reinterpret_cast<SOCKADDR_IN*>(address->Address.lpSockaddr);
							ULONG subnetMask;
							ConvertLengthToIpv4Mask(address->OnLinkPrefixLength, &subnetMask);
							ipv4->sin_addr.s_addr |= ~subnetMask;
							inet_ntop(AF_INET, &(ipv4->sin_addr), broadcastAddressBuffer, 16);

							struct addrinfo* res = nullptr, * it;
							struct addrinfo hints;
							memset(&hints, 0, sizeof(struct addrinfo));
							hints.ai_family = AF_INET;
							hints.ai_socktype = SOCK_DGRAM;

							getaddrinfo(broadcastAddressBuffer, BROADCAST_PORT, &hints, &res);

							for (it = res; it != NULL; it = it->ai_next)
							{
								broadcastSocket = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
								char enable = '1';
								setsockopt(broadcastSocket, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));
								u_long mode = 1;
								ioctlsocket(broadcastSocket, FIONBIO, &mode);
								broadcastSocketAddr = it->ai_addr;
								broadcastSocketLen = it->ai_addrlen;
								break;
							}
							break;
						}
					}
					break;
				}

				isInCoopOptionMenu = true;
				isInNetworkAdapterMenu = false;
				hasReadNetworkAdapterDisclaimer = false;
			}
		}
	}
	else if (isInGamemodeSelect)
	{
		if (curCoopOptionMenuIndex == 0)
		{
			// Single player
			g_ModuleInterface->CallBuiltin("room_goto", { rmCharSelect });
			isInGamemodeSelect = false;
		}
		else if (curCoopOptionMenuIndex == 1)
		{
			// Use saved adapter
			CreateDirectory(L"MultiplayerMod", NULL);
			if (!std::filesystem::exists("MultiplayerMod/lastUsedNetworkAdapter"))
			{
				g_ModuleInterface->Print(CM_RED, "Couldn't find the last used network adapter file");
				return ReturnValue;
			}
			std::ifstream inFile;
			inFile.open("MultiplayerMod/lastUsedNetworkAdapter");
			std::string line;
			if (!std::getline(inFile, line))
			{
				g_ModuleInterface->Print(CM_RED, "Couldn't read network adapter name from MultiplayerMod/lastUsedNetworkAdapter");
				inFile.close();
				return ReturnValue;
			}

			IP_ADAPTER_ADDRESSES* adapter(NULL);

			DWORD adapterAddressesBufferSize = 16 * 1024;

			for (int i = 0; i < 3; i++)
			{
				adapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(malloc(adapterAddressesBufferSize));
				DWORD error = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME, NULL, adapterAddresses, &adapterAddressesBufferSize);
				if (error == ERROR_SUCCESS)
				{
					break;
				}
				else if (error == ERROR_BUFFER_OVERFLOW)
				{
					free(adapterAddresses);
					adapterAddresses = NULL;
					continue;
				}
				else
				{
					free(adapterAddresses);
					adapterAddresses = NULL;
					continue;
				}
			}

			for (adapter = adapterAddresses; adapter != NULL; adapter = adapter->Next)
			{
				if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
				{
					continue;
				}

				int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, static_cast<int>(wcslen(adapter->FriendlyName)), NULL, 0, NULL, NULL);
				std::string resString(sizeNeeded, 0);
				WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, static_cast<int>(wcslen(adapter->FriendlyName)), &resString[0], sizeNeeded, NULL, NULL);
				if (line.compare(resString) != 0)
				{
					continue;
				}

				isInCoopOptionMenu = true;
				isInGamemodeSelect = false;

				for (IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; address != NULL; address = address->Next)
				{
					auto family = address->Address.lpSockaddr->sa_family;
					if (family == AF_INET)
					{
						SOCKADDR_IN* ipv4 = reinterpret_cast<SOCKADDR_IN*>(address->Address.lpSockaddr);
						ULONG subnetMask;
						ConvertLengthToIpv4Mask(address->OnLinkPrefixLength, &subnetMask);
						ipv4->sin_addr.s_addr |= ~subnetMask;
						inet_ntop(AF_INET, &(ipv4->sin_addr), broadcastAddressBuffer, 16);

						struct addrinfo* res = nullptr, * it;
						struct addrinfo hints;
						memset(&hints, 0, sizeof(struct addrinfo));
						hints.ai_family = AF_INET;
						hints.ai_socktype = SOCK_DGRAM;

						getaddrinfo(broadcastAddressBuffer, BROADCAST_PORT, &hints, &res);

						for (it = res; it != NULL; it = it->ai_next)
						{
							broadcastSocket = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
							char enable = '1';
							setsockopt(broadcastSocket, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));
							u_long mode = 1;
							ioctlsocket(broadcastSocket, FIONBIO, &mode);
							broadcastSocketAddr = it->ai_addr;
							broadcastSocketLen = it->ai_addrlen;
							break;
						}
						break;
					}
				}
				inFile.close();
				free(adapterAddresses);
				adapterAddresses = NULL;
				return ReturnValue;
			}
			free(adapterAddresses);
			adapterAddresses = NULL;

			g_ModuleInterface->Print(CM_RED, "Couldn't find network adapter %s", line);
			inFile.close();
		}
		else if (curCoopOptionMenuIndex == 2)
		{
			// Choose an adapter
			isInGamemodeSelect = false;
			isInNetworkAdapterMenu = true;
			hasReadNetworkAdapterDisclaimer = false;
		}
	}
	else
	{
		// On the original title screen
		int currentOption = static_cast<int>(lround(getInstanceVariable(Self, GML_currentOption).m_Real));
		if (currentOption == 0)
		{
			isInGamemodeSelect = true;
			setInstanceVariable(Self, GML_canControl, RValue(false));
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}

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
	attacksCopyMap.clear();
	removedItemsMap.clear();
	eliminatedAttacksMap.clear();
	rerollContainerMap.clear();
	currentStickersMap.clear();
	charSelectedMap.clear();
	playerDataMap.clear();
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
	hasObtainedClientID = false;
	curUnusedPlayerID = 1;
	playerPingMap.clear();
	clientUnpausedMap.clear();
	clientSocketMap.clear();
	lobbyPlayerDataMap.clear();
	hasClientPlayerDisconnected.clear();
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
	curCoopOptionMenuIndex = 0;
	if (isInGamemodeSelect)
	{
		isInGamemodeSelect = false;
		setInstanceVariable(Self, GML_canControl, RValue(true));
	}
	else if (isInNetworkAdapterMenu)
	{
		isInGamemodeSelect = true;
		isInNetworkAdapterMenu = false;
		hasReadNetworkAdapterDisclaimer = false;
		if (adapterAddresses != NULL)
		{
			free(adapterAddresses);
		}
		adapterAddresses = NULL;
	}
	else if (isInCoopOptionMenu)
	{
		isInGamemodeSelect = true;
		isInCoopOptionMenu = false;
		closesocket(broadcastSocket);
		broadcastSocket = INVALID_SOCKET;
		if (adapterAddresses != NULL)
		{
			free(adapterAddresses);
		}
		adapterAddresses = NULL;
	}
	else if (isInLobby)
	{
		if (isHost)
		{
			for (auto& curClientSocket : clientSocketMap)
			{
				closesocket(curClientSocket.second);
			}
			closesocket(connectClientSocket);
			connectClientSocket = INVALID_SOCKET;
		}
		else
		{
			printf("closing server socket\n");
			closesocket(serverSocket);
			serverSocket = INVALID_SOCKET;
		}
		hasConnected = false;
		cleanupPlayerGameData();
		cleanupPlayerClientData();
		isInLobby = false;
		isInCoopOptionMenu = true;
		hasSelectedMap = false;
	}
	else if (isSelectingCharacter)
	{
		if (hasReturnedFromSelectingCharacter)
		{
			hasReturnedFromSelectingCharacter = false;
			isSelectingCharacter = false;
			isInLobby = true;
			curCoopOptionMenuIndex = 0;
			g_ModuleInterface->CallBuiltin("instance_destroy", { objCharSelectIndex });
		}
	}
	else if (isSelectingMap)
	{
		if (hasReturnedFromSelectingMap)
		{
			hasReturnedFromSelectingMap = false;
			isSelectingMap = false;
			isInLobby = true;
			curCoopOptionMenuIndex = 0;
			g_ModuleInterface->CallBuiltin("instance_destroy", { objCharSelectIndex });
		}
	}
	return ReturnValue;
}

RValue& ReturnCharSelectCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	if (isSelectingCharacter || isSelectingMap)
	{
		RValue charSelected = g_ModuleInterface->CallBuiltin("variable_global_get", { "charSelected" });
		if (charSelected.m_Kind != VALUE_OBJECT)
		{
			hasReturnedFromSelectingCharacter = true;
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
		else
		{
			int gameMode = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("variable_global_get", { "gameMode" }).m_Real));
			if (gameMode == -1)
			{
				hasReturnedFromSelectingMap = true;
				callbackManagerInterfacePtr->CancelOriginalFunction();
			}
		}
	}
	return ReturnValue;
}

RValue& SelectCharSelectCreateAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args)
{
	RValue holoHouseMode = g_ModuleInterface->CallBuiltin("variable_global_get", { "holoHouseMode" });
	if (!holoHouseMode.AsBool())
	{
		if (isSelectingCharacter)
		{
			RValue charSelected = g_ModuleInterface->CallBuiltin("variable_global_get", { "charSelected" });
			if (charSelected.m_Kind == VALUE_OBJECT)
			{
				// Send player back to the lobby after selecting a character
				// TODO: Allow for outfit selection later
				// TODO: There might be an issue if the player left clicks and right clicks at the same time?
				lobbyPlayerDataMap[clientID].charName = getInstanceVariable(charSelected, GML_id).AsString();
				lobbyPlayerDataMap[clientID].stageSprite = curSelectedStageSprite;
				isSelectingCharacter = false;
				isInLobby = true;
				curCoopOptionMenuIndex = 0;
				g_ModuleInterface->CallBuiltin("instance_destroy", { objCharSelectIndex });
			}
		}
		else if (isSelectingMap)
		{
			int stageSelected = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("variable_global_get", { "playingStage" }).m_Real));
			if (stageSelected != -1)
			{
				// Send player back to the lobby after selecting a map
				hasSelectedMap = true;
				curSelectedMap = stageSelected;
				curSelectedGameMode = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("variable_global_get", { "gameMode" }).m_Real));
				RValue whichSet = getInstanceVariable(Self, GML_availableStages);
				int selectedStage = static_cast<int>(lround(getInstanceVariable(Self, GML_selectedStage).m_Real));
				if (curSelectedGameMode == 0 || curSelectedGameMode == 1)
				{
					curSelectedStageSprite = static_cast<int>(lround(getInstanceVariable(whichSet[selectedStage], GML_stageSprite).m_Real));
					lobbyPlayerDataMap[HOST_INDEX].stageSprite = curSelectedStageSprite;
				}
				isSelectingMap = false;
				isInLobby = true;
				curCoopOptionMenuIndex = 0;
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
	if (hasConnected && !isHost)
	{
		if (isClientInInitializeCharacter)
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
	return ReturnValue;
}