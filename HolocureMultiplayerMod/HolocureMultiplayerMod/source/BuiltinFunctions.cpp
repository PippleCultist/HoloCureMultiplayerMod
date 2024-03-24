#include "BuiltinFunctions.h"
#include "NetworkFunctions.h"
#include "ScriptFunctions.h"
#include "CommonFunctions.h"
#include "CodeEvents.h"

void InstanceCreateLayerBefore(RValue* Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args)
{
	if (hasConnected && !isHost)
	{
		if (abs(Args[3].m_Real - objPlayerIndex) < 1e-3)
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

			*Result = playerMap[clientID];
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void SpriteDeleteBefore(RValue* Result, CInstance* Self, CInstance* Other, int numArgs, RValue* Args)
{
	if (hasConnected && isHost)
	{
		RValue pausedScreenSprite = getInstanceVariable(Self, GML_paused_screen_sprite);
		if (pausedScreenSprite.m_Kind != VALUE_UNSET)
		{
			if (abs(pausedScreenSprite.m_Real - Args[0].m_Real) < 1e-3)
			{
				callbackManagerInterfacePtr->CancelOriginalFunction();
			}
		}
	}
}