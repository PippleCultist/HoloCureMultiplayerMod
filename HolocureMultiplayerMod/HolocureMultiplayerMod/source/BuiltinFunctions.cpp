#include "BuiltinFunctions.h"
#include "NetworkFunctions.h"
#include "ScriptFunctions.h"
#include "CommonFunctions.h"
#include "CodeEvents.h"

extern bool isInCreateSummon;
extern bool isInPlayerManagerOther23;

void InstanceCreateLayerBefore(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args)
{
	if (hasConnected && !isHost)
	{
		if (abs(Args[3].ToDouble() - objPlayerIndex) < 1e-3)
		{
			RValue currentClientPlayerAttacks;
			// Should probably make the client player first so that all the player stuff already in the code will just refer to that
			{
				RValue createdInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { playerDataMap[clientID].xPos, playerDataMap[clientID].yPos, -playerDataMap[clientID].yPos, objPlayerIndex });
				setInstanceVariable(createdInstance, GML_image_xscale, RValue(playerDataMap[clientID].imageXScale));
				setInstanceVariable(createdInstance, GML_image_yscale, RValue(playerDataMap[clientID].imageYScale));
				setInstanceVariable(createdInstance, GML_sprite_index, RValue(playerDataMap[clientID].spriteIndex));
				setInstanceVariable(createdInstance, GML_mask_index, RValue(sprEmptyMaskIndex));
				setInstanceVariable(createdInstance, GML_completeStop, RValue(true));
				setInstanceVariable(createdInstance, GML_image_speed, RValue(0));
				isPlayerCreatedMap[clientID] = true;
				playerMap[clientID] = createdInstance;
				printf("created client %p\n", createdInstance.m_Object);
				currentClientPlayerAttacks = g_ModuleInterface->CallBuiltin("variable_global_get", { "playerAttacks" });
			}

			for (auto& curPlayerData: playerDataMap)
			{
				uint32_t clientPlayerID = curPlayerData.first;
				playerData clientPlayerData = curPlayerData.second;
				if (clientID == clientPlayerID)
				{
					continue;
				}
				g_ModuleInterface->CallBuiltin("variable_global_set", { "playerAttacks", -1.0 });
				RValue createdInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { clientPlayerData.xPos, clientPlayerData.yPos, -clientPlayerData.yPos, objPlayerIndex });
				setInstanceVariable(createdInstance, GML_image_xscale, RValue(clientPlayerData.imageXScale));
				setInstanceVariable(createdInstance, GML_image_yscale, RValue(clientPlayerData.imageYScale));
				setInstanceVariable(createdInstance, GML_sprite_index, RValue(clientPlayerData.spriteIndex));
				setInstanceVariable(createdInstance, GML_mask_index, RValue(sprEmptyMaskIndex));
				setInstanceVariable(createdInstance, GML_completeStop, RValue(true));
				setInstanceVariable(createdInstance, GML_image_speed, RValue(0));
				setInstanceVariable(createdInstance, GML_canControl, RValue(false));
				printf("created player %d %p\n", clientPlayerID, createdInstance.m_Object);
				isPlayerCreatedMap[clientPlayerID] = true;
				playerMap[clientPlayerID] = createdInstance;
			}
			g_ModuleInterface->CallBuiltin("variable_global_set", { "playerAttacks", currentClientPlayerAttacks });
			g_ModuleInterface->CallBuiltin("variable_instance_set", { Self, "playerCharacter", playerMap[clientID] });

			Result = playerMap[clientID];
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void InstanceCreateLayerAfter(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args)
{
	if (hasConnected && isHost)
	{
		if (isInCreateSummon)
		{
			setInstanceVariable(Result, GML_playerID, RValue(curPlayerID));
		}
	}
}

void SpriteDeleteBefore(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args)
{
	if (hasConnected)
	{
		if (Args[0].ToDouble() < 0)
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
		if (isHost)
		{
			RValue pausedScreenSprite = getInstanceVariable(Self, GML_paused_screen_sprite);
			if (pausedScreenSprite.m_Kind != VALUE_UNSET)
			{
				if (abs(pausedScreenSprite.ToDouble() - Args[0].ToDouble()) < 1e-3)
				{
					callbackManagerInterfacePtr->CancelOriginalFunction();
				}
			}
		}
	}
}

void InstanceExistsBefore(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			if ((Args[0].m_Kind == VALUE_REAL || Args[0].m_Kind == VALUE_REF) && abs(Args[0].ToDouble() - objSummonIndex) < 1e-3)
			{
				// Apparently the instance_exists in the OnApply code for summoning stuff is there to prevent the summon from being summoned again if the perk remained at level 1
				// Need to do a check to make sure it hasn't run before to prevent the summon from being created multiple times
				auto curSummon = summonMap[curPlayerID];
				if (curSummon.m_Kind == VALUE_UNDEFINED)
				{
					Result = false;
				}
				else
				{
					Result = true;
				}
				callbackManagerInterfacePtr->CancelOriginalFunction();
			}
		}
	}
}

void DsMapFindValueBefore(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args)
{
	if (hasConnected && !isHost)
	{
		if (isInPlayerManagerOther23)
		{
			if (Args[1].ToString().compare("unlockedWeapons") == 0)
			{
				RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
				RValue attacksMap = getInstanceVariable(attackController, GML_attackIndex);
				RValue unlockedWeaponsArr = g_ModuleInterface->CallBuiltin("array_create", { RValue(0.0) });
				RValue attacksKeyArr = g_ModuleInterface->CallBuiltin("ds_map_keys_to_array", { attacksMap });
				int attacksKeyArrLen = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { attacksKeyArr }).ToDouble()));
				for (int i = 0; i < attacksKeyArrLen; i++)
				{
					RValue attackID = attacksKeyArr[i];
					RValue curAttack = g_ModuleInterface->CallBuiltin("ds_map_find_value", { attacksMap, attackID });
					RValue attacksConfig = getInstanceVariable(curAttack, GML_config);
					RValue attacksOptionType = getInstanceVariable(attacksConfig, GML_optionType);
					if (attacksOptionType.ToString().compare("Weapon") == 0)
					{
						g_ModuleInterface->CallBuiltin("array_push", { unlockedWeaponsArr, attackID });
					}
				}
				callbackManagerInterfacePtr->CancelOriginalFunction();
				Result = unlockedWeaponsArr;
			}
		}
	}
}

void InstanceCreateDepthAfter(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args)
{
	if (hasConnected && isHost)
	{
		if (numArgs >= 4)
		{
			if (static_cast<int>(lround(Args[3].ToDouble())) == objCocoWeaponIndex)
			{
				setInstanceVariable(Result, GML_player, playerMap[curPlayerID]);
			}
		}
	}
}

void InstanceCreateDepthAfter(RValue* Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args)
{
	if (hasConnected && isHost)
	{
		if (numArgs >= 4)
		{
			if (static_cast<int>(lround(Args[3].ToDouble())) == objCocoWeaponIndex)
			{
				setInstanceVariable(*Result, GML_player, playerMap[curPlayerID]);
			}
		}
	}
}

void InstanceFindBefore(RValue& Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args)
{
	if (hasConnected && isHost)
	{
		if (Args[0].ToInt32() == objPlayerIndex)
		{
			// Prevent it from swapping if the map hasn't been initialized yet
			auto playerMapFind = playerMap.find(curPlayerID);
			if (playerMapFind != playerMap.end())
			{
				// swap player to the actual current player
				Result = playerMap[curPlayerID];
				callbackManagerInterfacePtr->CancelOriginalFunction();
			}
		}
	}
}