#pragma comment(lib, "iphlpapi.lib")
#include <WS2tcpip.h>
#include "CodeEvents.h"
#include <YYToolkit/shared.hpp>
#include "CallbackManager/CallbackManagerInterface.h"
#include "ModuleMain.h"
#include "ScriptFunctions.h"
#include "CommonFunctions.h"
#include "NetworkFunctions.h"
#include <fstream>
#include <iphlpapi.h>
#include <semaphore>

std::vector<destructableData> createDestructableMessagesList;
std::unordered_map<uint32_t, bool> hasClientPlayerDisconnected;

int lastSentPingMessage = 0;
bool hasSent = false;

bool hasUsedSticker = false;

CInstance* playerManagerInstanceVar = nullptr;

std::unordered_map<CInstance*, uint16_t> instanceToIDMap;

std::unordered_map<CInstance*, uint16_t> attackToIDMap;

std::unordered_map<short, RValue> preCreateMap;

std::unordered_map<short, RValue> vfxMap;

std::unordered_map<short, RValue> afterImageMap;

std::unordered_map<ULONG, SOCKET> listenForClientSocketMap;

bool isAnyInteracting = false;
bool hasClientFinishedInteracting = true;

levelUpOption randomWeaponArr[3]{};

std::string collidedStickerID;

bool hasSelectedMap = false;
int curSelectedMap = -1;
int curSelectedGameMode = -1;
int curSelectedStageSprite = -1;
int curCoopOptionMenuIndex = 0;

bool hasHostPaused = false;

extern IP_ADAPTER_ADDRESSES* adapterAddresses;

// TODO: Improve this to assign better IDs
uint32_t curUnusedPlayerID = 1;

std::thread messageHandlerThread;

extern std::binary_semaphore lastTimeReceivedMoveDataMapLock;
extern std::unordered_map<uint32_t, clientMovementQueueData> lastTimeReceivedMoveDataMap;

extern RValue instanceArr[maxNumAvailableInstanceIDs];

// TODO: Make sure that the additional players will teleport accordingly on the infinite maps when crossing the border

void drawTextOutline(CInstance* Self, double xPos, double yPos, std::string text, double outlineWidth, int outlineColor, double numOutline, double linePixelSeparation, double pixelsBeforeLineBreak, int textColor, double alpha)
{
	RValue** args = new RValue*[10];
	args[0] = new RValue(xPos);
	args[1] = new RValue(yPos);
	args[2] = new RValue(text);
	args[3] = new RValue(outlineWidth);
	args[4] = new RValue(static_cast<double>(outlineColor));
	args[5] = new RValue(numOutline);
	args[6] = new RValue(linePixelSeparation);
	args[7] = new RValue(pixelsBeforeLineBreak);
	args[8] = new RValue(static_cast<double>(textColor));
	args[9] = new RValue(alpha);
	RValue returnVal;
	origDrawTextOutlineScript(Self, nullptr, returnVal, 10, args);
}

bool hasInitializedInputManagerSettings = false;
void InputManagerCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (!hasInitializedInputManagerSettings)
	{
		printf("Start initializing\n");
		hasInitializedInputManagerSettings = true;
	}
}

// TODO: Disable player input moving on the client side.
void InputManagerStepAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
}

inline uint32_t getPlayerID(CInstance* currentPlayerPtr)
{
	for (auto& curPlayerData : playerMap)
	{
		uint32_t playerID = curPlayerData.first;
		RValue playerInstance = curPlayerData.second;
		//		printf("%p %p\n", currentPlayerPtr, playerList[playerIndex].m_Object);
		if (currentPlayerPtr == playerInstance.m_Object)
		{
			return playerID;
		}
	}
	// TODO: Should probably designate something else to be the not found ID
	return 100000;
}

void PlayerMouse53After(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
}

void PlayerDrawBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
}

void PlayerDrawAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		CInstance* Self = std::get<0>(Args);
		CInstance* Other = std::get<1>(Args);
		RValue returnVal;
		uint32_t playerID = getPlayerID(getInstanceVariable(Self, GML_id).m_Object);

		if (playerID == 100000)
		{
			return;
		}

		g_ModuleInterface->CallBuiltin("draw_set_halign", { 1 });

		// Draw player num
		double curXPos = getInstanceVariable(Self, GML_x).m_Real;
		double curYPos = getInstanceVariable(Self, GML_y).m_Real;
		drawTextOutline(Self, curXPos, curYPos, std::format("P{}", lobbyPlayerDataMap[playerID].playerName), 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1);

		// Draw ping numbers under all clients for the host and only under the host for the clients
		// Equivalent to xor, but probably not as readable
		if ((isHost && playerID != 0) || (!isHost && playerID == 0))
		{
			drawTextOutline(Self, curXPos, curYPos + 10, std::format("{} ms", playerPingMap[playerID]), 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1);
		}

		if (clientID == playerID)
		{
			double halfScreenWidth = 320;
			double halfScreenHeight = 180;
			double edgeX = 300;
			double edgeY = 160;
			for (auto& curPlayerData : playerMap)
			{
				uint32_t curPlayerID = curPlayerData.first;
				RValue curPlayerInstance = curPlayerData.second;
				if (playerID == curPlayerID)
				{
					continue;
				}
				double xPos = getInstanceVariable(curPlayerInstance, GML_x).m_Real;
				double yPos = getInstanceVariable(curPlayerInstance, GML_y).m_Real;
				RValue arrowDir = g_ModuleInterface->CallBuiltin("point_direction", { curXPos, curYPos, xPos, yPos });
				// Check if the client player isn't on screen
				if (xPos < curXPos - halfScreenWidth - 15 || xPos > curXPos + halfScreenWidth + 15
					|| yPos < curYPos - halfScreenHeight - 15 || yPos > curYPos + halfScreenHeight + 15)
				{
					double clientXPosDiff = xPos - curXPos;
					double clientYPosDiff = yPos - curYPos;
					double clientArrowXPosDiff = 0;
					double clientArrowYPosDiff = 0;
					if (abs(clientXPosDiff / edgeX) > abs(clientYPosDiff / edgeY))
					{
						if (clientXPosDiff > 0)
						{
							clientArrowXPosDiff = edgeX;
						}
						else
						{
							clientArrowXPosDiff = -edgeX;
						}
						clientArrowYPosDiff = clientArrowXPosDiff / clientXPosDiff * clientYPosDiff;
					}
					else
					{
						if (clientYPosDiff > 0)
						{
							clientArrowYPosDiff = edgeY;
						}
						else
						{
							clientArrowYPosDiff = -edgeY;
						}
						clientArrowXPosDiff = clientArrowYPosDiff / clientYPosDiff * clientXPosDiff;
					}
					double clientArrowXPos = curXPos + clientArrowXPosDiff;
					double clientArrowYPos = curYPos + clientArrowYPosDiff;
					g_ModuleInterface->CallBuiltin("draw_sprite_ext", { sprSummonPointerIndex, 0, clientArrowXPos, clientArrowYPos, 1, 1, arrowDir, static_cast<double>(0xFFFFFF), 1 });
					double clientArrowDiffLength = sqrt(clientArrowXPosDiff * clientArrowXPosDiff + clientArrowYPosDiff * clientArrowYPosDiff);
					double clientArrowXDir = clientArrowXPosDiff / clientArrowDiffLength;
					double clientArrowYDir = clientArrowYPosDiff / clientArrowDiffLength;
					drawTextOutline(Self, clientArrowXPos - clientArrowXDir * 30, clientArrowYPos - clientArrowYDir * 30 - 5, std::format("P{}", lobbyPlayerDataMap[curPlayerID].playerName), 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1);

					if ((isHost && curPlayerID != 0) || (!isHost && curPlayerID == 0))
					{
						drawTextOutline(Self, clientArrowXPos - clientArrowXDir * 30, clientArrowYPos - clientArrowYDir * 30 + 5, std::format("{} ms", playerPingMap[curPlayerID]), 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1);
					}
				}
			}
		}
	}
}

void resetAllData()
{
	instanceToIDMap.clear();
	pickupableToIDMap.clear();
	preCreateMap.clear();
	vfxMap.clear();
	interactableMap.clear();
	clientSocketMap.clear();
	playerDataMap.clear();
	isPlayerCreatedMap.clear();
	playerManagerInstanceVar = nullptr;
	hasClientFinishedInteracting = false;
	isAnyInteracting = false;
	collidedStickerID = "";
	curCoopOptionMenuIndex = 0;
	hasSelectedMap = false;
	curSelectedMap = -1;
	curSelectedGameMode = -1;
	curSelectedStageSprite = -1;
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

	clientID = 0;
	timeNum = 0;

	isClientUsingBox = false;
	isClientUsingAnvil = false;
	isClientUsingGoldenAnvil = false;
	isClientUsingStamp = false;

	hasObtainedClientID = false;

	isClientPaused = false;
	playerMap.clear();
	playerWeaponMap.clear();
	playerPerksMap.clear();
	playerItemsMapMap.clear();
	playerItemsMap.clear();
	playerAttackIndexMapMap.clear();
	currentStickersMap.clear();
	playerPingMap.clear();
	lobbyPlayerDataMap.clear();
}

void serverDisconnected()
{
	hasHostPaused = false;
	hasConnected = false;
	isInCoopOptionMenu = false;
	isInLobby = false;
	isSelectingCharacter = false;
	isSelectingMap = false;
	closesocket(serverSocket);
	serverSocket = INVALID_SOCKET;
	closesocket(connectClientSocket);
	connectClientSocket = INVALID_SOCKET;
	closesocket(broadcastSocket);
	broadcastSocket = INVALID_SOCKET;
	if (adapterAddresses != NULL)
	{
		free(adapterAddresses);
	}
	adapterAddresses = NULL;
	if (playerManagerInstanceVar != nullptr)
	{
		g_ModuleInterface->CallBuiltin("instance_destroy", { playerManagerInstanceVar });
	}
	g_ModuleInterface->CallBuiltin("room_restart", {});
	g_ModuleInterface->CallBuiltin("room_goto", { rmTitle });
	g_ModuleInterface->CallBuiltin("variable_global_set", { "resetLevel", true });
	resetAllData();
}

void InputControllerObjectStep1Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	CInstance* Self = std::get<0>(Args);
	CInstance* Other = std::get<1>(Args);
	RValue returnVal;
	if (hasConnected)
	{
		handleCharDataMessage();
		if (isHost)
		{
			handleEliminateLevelUpClientChoiceMessage();
			handleClientSpecialAttackMessage();
			handleInteractFinishedMessage();
			handleBoxTakeOptionMessage();
			handleAnvilChooseOptionMessage();
			handleClientGainMoneyMessage();
			handleClientAnvilEnchantMessage();
			handleStickerChooseOptionMessage();
			handleChooseCollabMessage();
			handleLevelUpClientChoiceMessage();
			for (auto& curClientSocket : clientSocketMap)
			{
				uint32_t clientPlayerID = curClientSocket.first;
				SOCKET clientSocket = curClientSocket.second;
				if (lastSentPingMessage != timeNum && timeNum % 60 == 0)
				{
					sendPing(clientSocket);
				}
			}
			if (lastSentPingMessage != timeNum && timeNum % 60 == 0)
			{
				lastSentPingMessage = timeNum;
			}
			if (!playerMap.empty())
			{
				sendAllClientPlayerDataMessage();
				sendAllGameDataMessage();
				if (!hasSent)
				{
					// TODO: Should probably add retries in case the message failed to send
					sendAllRoomMessage();
					hasSent = true;
				}
				else
				{
					// TODO: Pretty hacky way of doing stuff a frame after the room message is sent. Should probably do this some other way
					// Make sure that the destructable instances are created after switching rooms
					for (auto& destructable : createDestructableMessagesList)
					{
						sendAllDestructableCreateMessage(destructable);
					}
					createDestructableMessagesList.clear();
				}
				sendAllInstanceCreateMessage();
				sendAllInstanceUpdateMessage();
				sendAllInstanceDeleteMessage();

				sendAllAttackCreateMessage();
				sendAllAttackUpdateMessage();
				sendAllAttackDeleteMessage();
			}
		}
		else
		{
			handleRoomMessage();
			handleInstanceCreateMessage();
			handleInstanceUpdateMessage();
			handleInstanceDeleteMessage();
			handleClientPlayerDataMessage();
			handleAttackCreateMessage();
			handleAttackUpdateMessage();
			handleAttackDeleteMessage();
			handleClientIDMessage();
			handlePickupableCreateMessage();
			handlePickupableUpdateMessage();
			handlePickupableDeleteMessage();
			handleGameDataMessage();
			handleDestructableCreateMessage();
			handleDestructableBreakMessage();
			handleCautionCreateMessage();
			handlePreCreateUpdateMessage();
			handleVFXUpdateMessage();
			handleInteractableCreateMessage();
			handleInteractableDeleteMessage();
			handleInteractablePlayerInteractedMessage();
			handleStickerPlayerInteractedMessage();
			handleBoxPlayerInteractedMessage();
			handleBuffDataMessage();
			handleReturnToLobby();
			handleLobbyPlayerDisconnected();
			handleHostHasPaused();
			handleHostHasUnpaused();

			if (lastSentPingMessage != timeNum && timeNum % 60 == 0)
			{
				sendPing(serverSocket);
				lastSentPingMessage = timeNum;
			}
			sendInputMessage(serverSocket);
		}
	}
}

void EnemyStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		CInstance* Self = std::get<0>(Args);
		RValue followTarget = getInstanceVariable(Self, GML_followTarget);
		if (followTarget.m_Kind == VALUE_REF)
		{
			double posX = getInstanceVariable(Self, GML_x).m_Real;
			double posY = getInstanceVariable(Self, GML_y).m_Real;
			// TODO: Probably should cache this since it won't change for the current frame
			double originalPlayerPosX = getInstanceVariable(playerMap[0], GML_x).m_Real;
			double originalPlayerPosY = getInstanceVariable(playerMap[0], GML_y).m_Real;
			double minDis = (posX - originalPlayerPosX) * (posX - originalPlayerPosX) + (posY - originalPlayerPosY) * (posY - originalPlayerPosY);
			uint32_t closestPlayerID = 0;
			for (auto& curPlayer : playerMap)
			{
				uint32_t playerID = curPlayer.first;
				RValue playerInstance = curPlayer.second;
				double playerPosX = getInstanceVariable(playerInstance, GML_x).m_Real;
				double playerPosY = getInstanceVariable(playerInstance, GML_y).m_Real;
				double curDis = (posX - playerPosX) * (posX - playerPosX) + (posY - playerPosY) * (posY - playerPosY);
				if (curDis < minDis)
				{
					minDis = curDis;
					closestPlayerID = playerID;
				}
			}
			setInstanceVariable(Self, GML_followTarget, playerMap[closestPlayerID]);
		}

		if (isHost)
		{
			if (getInstanceVariable(Self, GML_isDead).AsBool())
			{
				return;
			}
			// TODO: Use UDP in order to send these messages
			CInstance* Self = std::get<0>(Args);
			auto mapInstance = instanceToIDMap.find(Self);
			if (mapInstance == instanceToIDMap.end())
			{
				int instanceID = availableInstanceIDs.front();
				instanceToIDMap[Self] = instanceID;
				float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
				float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);
				float imageXScale = static_cast<float>(getInstanceVariable(Self, GML_image_xscale).m_Real);
				float imageYScale = static_cast<float>(getInstanceVariable(Self, GML_image_yscale).m_Real);
				// Probably should change this to uint16_t
				short spriteIndex = static_cast<short>(lround(getInstanceVariable(Self, GML_sprite_index).m_Real));
				char truncatedImageIndex = static_cast<char>(getInstanceVariable(Self, GML_image_index).m_Real);
				// seems like there's something that doesn't have a sprite at the beginning? Not sure what it is
				// Maybe it's the additional player I created?
				// temp code to just make it work for now
				if (spriteIndex < 0)
				{
					spriteIndex = 0;
				}
				instanceData data(xPos, yPos, imageXScale, imageYScale, spriteIndex, instanceID, truncatedImageIndex);
				//			if (spriteIndex >= 0)
				{
					instancesCreateMessage.addInstance(data);
					if (instancesCreateMessage.numInstances >= instanceCreateDataLen)
					{
						sendAllInstanceCreateMessage();
					}
				}

				availableInstanceIDs.pop();
			}
			else
			{
				float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
				float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);
				float imageXScale = static_cast<float>(getInstanceVariable(Self, GML_image_xscale).m_Real);
				float imageYScale = static_cast<float>(getInstanceVariable(Self, GML_image_yscale).m_Real);
				// Probably should change this to uint16_t
				short spriteIndex = static_cast<short>(lround(getInstanceVariable(Self, GML_sprite_index).m_Real));
				char truncatedImageIndex = static_cast<char>(getInstanceVariable(Self, GML_image_index).m_Real);
				instanceData data(xPos, yPos, imageXScale, imageYScale, spriteIndex, mapInstance->second, truncatedImageIndex);
				instancesUpdateMessage.addInstance(data);
				if (instancesUpdateMessage.numInstances >= instanceUpdateDataLen)
				{
					sendAllInstanceUpdateMessage();
				}
			}
		}
	}
}

void PlayerManagerCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
}

void PlayerManagerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		CInstance* Self = std::get<0>(Args);
		playerManagerInstanceVar = Self;
		if (!isHost)
		{
			handleLevelUpOptionsMessage(Self);
			// TODO: send correct player data, box data?, ...
			// TODO: send over messages for coins, boxes, and stamps to the client side
			g_ModuleInterface->CallBuiltin("variable_instance_set", { Self, "blackFlash", 0 });
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
		else
		{
			if (getInstanceVariable(Self, GML_usedSticker).AsBool() && isClientUsingStamp)
			{
				RValue stickerID = getInstanceVariable(Self, GML_stickerID);
				setInstanceVariable(stickerID, GML_stickerID, RValue(collidedStickerID));
				hasUsedSticker = true;
				isClientUsingStamp = false;
			}
			if (!levelUpPausedList.empty())
			{
				RValue playerManager = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerManagerIndex, 0 });
				RValue playerCharacter = getInstanceVariable(playerManager, GML_playerCharacter);
				if (g_ModuleInterface->CallBuiltin("instance_exists", { playerCharacter }).AsBool())
				{
					RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
					// Do the actual level ups now that the game isn't paused
					for (auto& levelUpData : levelUpPausedList)
					{
						uint32_t playerID = levelUpData.playerID;
						swapPlayerData(Self, attackController, playerID);
						processLevelUp(levelUpData, Self);
					}
					swapPlayerData(Self, attackController, 0);
					levelUpPausedList.clear();
				}
			}
		}
	}
}

void PlayerManagerStepAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		if (hasUsedSticker)
		{
			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			for (auto& curPlayer : playerMap)
			{
				uint32_t playerID = curPlayer.first;
				if (playerID == 0)
				{
					continue;
				}
				swapPlayerData(playerManagerInstanceVar, attackController, playerID);
				RValue returnVal;
				origUpdatePlayerPlayerManagerOtherScript(playerManagerInstanceVar, nullptr, returnVal, 0, nullptr);
			}
			swapPlayerData(playerManagerInstanceVar, attackController, 0);
			hasUsedSticker = false;
		}
	}
}

void PlayerManagerCleanUpBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			g_ModuleInterface->CallBuiltin("instance_destroy", { attackController });
			// TODO: Add more cleanup stuff here
		}
		else
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void StageManagerCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && !isHost)
	{
		/*
		CInstance* Self = std::get<0>(Args);
		callbackManagerInterfacePtr->CancelOriginalFunction();
		g_ModuleInterface->CallBuiltin("instance_destroy", { RValue(Self) });
		*/
	}
}

void StageManagerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && !isHost)
	{
		CInstance* Self = std::get<0>(Args);
		// TODO: Probably don't need to set these every frame
		setInstanceVariable(Self, GML_enabledSpawner, RValue(false));
		setInstanceVariable(Self, GML_sequenceSpawn, RValue(false));
	}
}

void CrateSpawnerCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && !isHost)
	{
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}
}
void CrateSpawnerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && !isHost)
	{
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}
}

void CamStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && !isHost)
	{
		CInstance* Self = std::get<0>(Args);
		callbackManagerInterfacePtr->CancelOriginalFunction();
		// temp code
		setInstanceVariable(Self, GML_x, RValue(clientCamPosX));
		setInstanceVariable(Self, GML_y, RValue(clientCamPosY));
	}
}

void CloudMakerAlarmBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && !isHost)
	{
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}
}

void BaseMobCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
}

void AttackCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (!isHost)
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

std::unordered_map<uint16_t, short> attackSpriteIndexMap;
std::unordered_map<uint16_t, std::pair<double, double>> attackImageScaleMap;

void AttackStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			// TODO: Use UDP in order to send these messages
			CInstance* Self = std::get<0>(Args);
			auto mapAttack = attackToIDMap.find(Self);
			if (mapAttack == attackToIDMap.end())
			{
				int attackID = availableAttackIDs.front();
				attackToIDMap[Self] = attackID;
				float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
				float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);
				float imageXScale = static_cast<float>(getInstanceVariable(Self, GML_image_xscale).m_Real);
				float imageYScale = static_cast<float>(getInstanceVariable(Self, GML_image_yscale).m_Real);
				float imageAngle = static_cast<float>(getInstanceVariable(Self, GML_image_angle).m_Real);
				float imageAlpha = static_cast<float>(getInstanceVariable(Self, GML_image_alpha).m_Real);
				// Probably should change this to uint16_t
				short spriteIndex = static_cast<short>(lround(getInstanceVariable(Self, GML_sprite_index).m_Real));
				char truncatedImageIndex = static_cast<char>(getInstanceVariable(Self, GML_image_index).m_Real);
				attackData data(xPos, yPos, imageXScale, imageYScale, imageAngle, imageAlpha, spriteIndex, attackID, truncatedImageIndex);
				attackSpriteIndexMap[attackID] = spriteIndex;
				attackImageScaleMap[attackID] = std::make_pair(imageXScale, imageYScale);
				attackCreateMessage.addAttack(data);
				if (attackCreateMessage.numAttacks >= attackCreateDataLen)
				{
					sendAllAttackCreateMessage();
				}

				availableAttackIDs.pop();
			}
			else
			{
				float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
				float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);
				float imageXScale = static_cast<float>(getInstanceVariable(Self, GML_image_xscale).m_Real);
				float imageYScale = static_cast<float>(getInstanceVariable(Self, GML_image_yscale).m_Real);
				float imageAngle = static_cast<float>(getInstanceVariable(Self, GML_image_angle).m_Real);
				float imageAlpha = static_cast<float>(getInstanceVariable(Self, GML_image_alpha).m_Real);
				// Probably should change this to uint16_t
				short spriteIndex = static_cast<short>(lround(getInstanceVariable(Self, GML_sprite_index).m_Real));
				char truncatedImageIndex = static_cast<char>(getInstanceVariable(Self, GML_image_index).m_Real);
				attackData data(xPos, yPos, imageXScale, imageYScale, imageAngle, imageAlpha, spriteIndex, mapAttack->second, truncatedImageIndex);
				std::pair<double, double> attackImageScale = attackImageScaleMap[mapAttack->second];
				if (attackSpriteIndexMap[mapAttack->second] == spriteIndex && abs(imageXScale - attackImageScale.first) < 1e-3 && abs(imageYScale - attackImageScale.second) < 1e-3)
				{
					attackUpdateMessage.addAttack(data);
					if (attackUpdateMessage.numAttacks >= attackUpdateDataLen)
					{
						sendAllAttackUpdateMessage();
					}
				}
				else
				{
					// Kind of hacky solution to update the sprite index and image scale
					// TODO: Should probably replace this eventually
					attackSpriteIndexMap[mapAttack->second] = spriteIndex;
					attackImageScaleMap[mapAttack->second] = std::make_pair(imageXScale, imageYScale);
					attackCreateMessage.addAttack(data);
					if (attackCreateMessage.numAttacks >= attackCreateDataLen)
					{
						sendAllAttackCreateMessage();
					}
				}
			}
		}
		else
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

// TODO: Should probably change this to set mask_index to an empty sprite and not run collision code at all
void AttackCollisionBaseMobBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (!isHost)
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void AttackCollisionObstacleBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (!isHost)
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void AttackCollisionAttackBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (!isHost)
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void AttackCleanupBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (!isHost)
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void AttackDestroyBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			CInstance* Self = std::get<0>(Args);
			auto attackFind = attackToIDMap.find(Self);
			if (attackFind != attackToIDMap.end())
			{
				uint16_t attackID = attackFind->second;

				attackToIDMap.erase(Self);
				attackSpriteIndexMap.erase(attackID);
				attackImageScaleMap.erase(attackID);

				attackDeleteMessage.addAttack(attackID);
				if (attackDeleteMessage.numAttacks >= attackDeleteDataLen)
				{
					sendAllAttackDeleteMessage();
				}

				availableAttackIDs.push(attackID);
			}
		}
	}
}

void PlayerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (!isHost)
		{
			RValue clientCurHP = getInstanceVariable(playerMap[clientID], GML_currentHP);
			RValue clientMaxHP = getInstanceVariable(playerMap[clientID], GML_HP);
			g_ModuleInterface->CallBuiltin("variable_global_set", { "currentHP", clientCurHP });
			g_ModuleInterface->CallBuiltin("variable_global_set", { "maxHP", clientMaxHP });
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
		else
		{
			// TODO: Can probably make this update every time the timer goes down a second or if it gains a new stack instead
			// TODO: Seems like if it sends the message too early, the player instance might not have been created yet. Could potentially queue up the message until it does exist
			if (timeNum >= 180 && timeNum % 10 == 0)
			{
				CInstance* Self = std::get<0>(Args);
				uint32_t playerID = getPlayerID(getInstanceVariable(Self, GML_id).m_Object);
				if (playerID != 100000 && playerID != 0)
				{
					RValue buffs = getInstanceVariable(Self, GML_buffs);
					RValue buffsNames = g_ModuleInterface->CallBuiltin("variable_instance_get_names", { buffs });
					int buffsNamesLen = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { buffsNames }).m_Real));
					std::vector<buffData> buffDataList;
					for (int i = 0; i < buffsNamesLen; i++)
					{
						RValue curBuffName = buffsNames[i];
						RValue curBuff = g_ModuleInterface->CallBuiltin("variable_instance_get", { buffs, curBuffName });
						RValue config = getInstanceVariable(curBuff, GML_config);
						RValue stacks = getInstanceVariable(config, GML_stacks);
						// TODO: Check if the assumption that stacks can't be negative is true since this is a kind of hacky way to signal that stacks doesn't exist
						short stacksValue = -1;
						if (stacks.m_Kind == VALUE_REAL)
						{
							stacksValue = static_cast<short>(lround(stacks.m_Real));
						}
						short timer = static_cast<short>(lround(getInstanceVariable(curBuff, GML_timer).m_Real));
						buffDataList.push_back(buffData(curBuffName.AsString(), timer, stacksValue));
					}
					sendBuffDataMessage(clientSocketMap[playerID], buffDataList);
				}
			}
		}
	}
}

void PlayerStepAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	// Pretty hacky way of making sure the global hp is set to the host player
	if (hasConnected && isHost)
	{
		CInstance* Self = std::get<0>(Args);
		handleInputMessage(Self);
		RValue hostCurHP = getInstanceVariable(playerMap[0], GML_currentHP);
		RValue hostMaxHP = getInstanceVariable(playerMap[0], GML_HP);
		g_ModuleInterface->CallBuiltin("variable_global_set", { "currentHP", hostCurHP });
		g_ModuleInterface->CallBuiltin("variable_global_set", { "maxHP", hostMaxHP });
	}
}

void PlayerManagerDraw64Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (!isHost)
		{
			if (isClientPaused)
			{
				CInstance* Self = std::get<0>(Args);
				RValue prevDepth = getInstanceVariable(Self, GML_depth);
				setInstanceVariable(Self, GML_depth, RValue(-15000));
				g_ModuleInterface->CallBuiltin("draw_set_alpha", { 0.7 });
				g_ModuleInterface->CallBuiltin("draw_rectangle_color", { 0, 0, 10000, 10000, 0, 0, 0, 0, 0 });
				g_ModuleInterface->CallBuiltin("draw_set_alpha", { 1.0 });
				setInstanceVariable(Self, GML_depth, prevDepth);
			}
		}
	}
}

void PlayerManagerDraw64After(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			if (isHostWaitingForClientUnpause)
			{
				CInstance* Self = std::get<0>(Args);
				g_ModuleInterface->CallBuiltin("draw_set_halign", { 1 });
				drawTextOutline(Self, 320, 50, "Waiting for players...", 1, 0x000000, 14, 0, 440, 0xFFFFFF, 1);
			}
		}
		else
		{
			if (hasHostPaused && !isClientPaused)
			{
				CInstance* Self = std::get<0>(Args);
				g_ModuleInterface->CallBuiltin("draw_set_halign", { 1 });
				drawTextOutline(Self, 320, 50, "Waiting for players...", 1, 0x000000, 14, 0, 440, 0xFFFFFF, 1);
			}
		}
	}
}

void EXPCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && !isHost)
	{
		CInstance* Self = std::get<0>(Args);
		g_ModuleInterface->CallBuiltin("variable_instance_set", { Self, "expVal", 0 });
	}
}

std::unordered_map<CInstance*, uint16_t> pickupableToIDMap;
std::unordered_map<uint16_t, std::pair<double, double>> pickupableLastPosMap;
std::unordered_map<short, std::pair<uint32_t, int>> clientPickupableImageIndexMap;

// TODO: Add code for exp and holocoindrop that will have them follow the closest summon
void AllPickupableStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			CInstance* Self = std::get<0>(Args);
			float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
			float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);

			double minDis = 1e20;
			uint32_t pos = 0;
			for (auto& curPlayer : playerMap)
			{
				uint32_t playerID = curPlayer.first;
				RValue playerInstance = curPlayer.second;
				double playerXPos = getInstanceVariable(playerInstance, GML_x).m_Real;
				double playerYPos = getInstanceVariable(playerInstance, GML_y).m_Real;
				double disSquared = (playerXPos - xPos) * (playerXPos - xPos) + (playerYPos - yPos) * (playerYPos - yPos);
				if (disSquared < minDis)
				{
					minDis = disSquared;
					pos = playerID;
				}
			}
			setInstanceVariable(Self, GML_followPlayerID, playerMap[pos]);

			// TODO: Use UDP in order to send these messages
			auto mapEXP = pickupableToIDMap.find(Self);
			if (mapEXP == pickupableToIDMap.end())
			{
				int expID = availablePickupableIDs.front();
				pickupableToIDMap[Self] = expID;
				// Probably should change this to uint16_t
				short spriteIndex = static_cast<short>(lround(getInstanceVariable(Self, GML_sprite_index).m_Real));
				char truncatedImageIndex = static_cast<char>(getInstanceVariable(Self, GML_image_index).m_Real);
				sendAllPickupableCreateMessage(pickupableData(xPos, yPos, spriteIndex, expID, truncatedImageIndex));
				pickupableLastPosMap[expID] = std::make_pair(xPos, yPos);

				availablePickupableIDs.pop();
			}
			else
			{
				// Probably should change this to uint16_t
				short spriteIndex = static_cast<short>(lround(getInstanceVariable(Self, GML_sprite_index).m_Real));
				char truncatedImageIndex = static_cast<char>(getInstanceVariable(Self, GML_image_index).m_Real);
				auto lastPosFind = pickupableLastPosMap.find(mapEXP->second);
				if (lastPosFind != pickupableLastPosMap.end())
				{
					std::pair<double, double>& lastPos = lastPosFind->second;
					// Skip sending pickupable data if it hasn't moved significantly
					if ((lastPos.first - xPos) * (lastPos.first - xPos) + (lastPos.second - yPos) * (lastPos.second - yPos) > 1e-4)
					{
						pickupableLastPosMap[mapEXP->second] = std::make_pair(xPos, yPos);
						sendAllPickupableUpdateMessage(pickupableData(xPos, yPos, spriteIndex, mapEXP->second, truncatedImageIndex));
					}
				}
				else
				{
					pickupableLastPosMap[mapEXP->second] = std::make_pair(xPos, yPos);
					sendAllPickupableUpdateMessage(pickupableData(xPos, yPos, spriteIndex, mapEXP->second, truncatedImageIndex));
				}
			}
		}
		else
		{
			CInstance* Self = std::get<0>(Args);
			int createdTime = getInstanceVariable(Self, GML_createdTime).m_i32;
			int numSubImages = static_cast<int>(lround(getInstanceVariable(Self, GML_image_number).m_Real));
			double spritePlaybackSpeed = getInstanceVariable(Self, GML_spritePlaybackSpeed).m_Real;
			setInstanceVariable(Self, GML_image_index, RValue((static_cast<int>((timeNum - createdTime) * spritePlaybackSpeed / 60.0) % numSubImages)));
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void AllPickupableCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			CInstance* Self = std::get<0>(Args);
			RValue initialSpawn = getInstanceVariable(Self, GML_initialSpawn);
			if (!initialSpawn.AsBool())
			{
				uint16_t expID = pickupableToIDMap[Self];
				pickupableToIDMap.erase(Self);
				pickupableLastPosMap.erase(expID);

				sendAllPickupableDeleteMessage(expID);

				availablePickupableIDs.push(expID);
			}
		}
		else
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void AllPickupableCollisionSummonBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			CInstance* Self = std::get<0>(Args);
			RValue initialSpawn = getInstanceVariable(Self, GML_initialSpawn);
			if (!initialSpawn.AsBool())
			{
				RValue getSummon = g_ModuleInterface->CallBuiltin("instance_find", { objSummonIndex, 0 });
				if (getInstanceVariable(getSummon, GML_pickupExp).AsBool())
				{
					uint16_t pickupableID = pickupableToIDMap[Self];
					pickupableToIDMap.erase(Self);
					pickupableLastPosMap.erase(pickupableID);

					sendAllPickupableDeleteMessage(pickupableID);

					availablePickupableIDs.push(pickupableID);
				}
			}
		}
		else
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void EXPCollisionEXPBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		CInstance* Self = std::get<0>(Args);
		RValue initialSpawn = getInstanceVariable(Self, GML_initialSpawn);
		if (!initialSpawn.AsBool())
		{
			RValue picked = getInstanceVariable(Self, GML_picked);
			if (!picked.AsBool())
			{
				CInstance* Other = std::get<1>(Args);
				RValue SelfID = getInstanceVariable(Self, GML_id);
				RValue OtherID = getInstanceVariable(Other, GML_id);
				// TODO: Not sure if this is exactly the same behavior as in the code
				if (SelfID.m_Pointer > OtherID.m_Pointer)
				{
					uint16_t expID = pickupableToIDMap[Other];
					pickupableToIDMap.erase(Other);
					pickupableLastPosMap.erase(expID);
					pickupableLastPosMap.erase(pickupableToIDMap[Self]);

					sendAllPickupableDeleteMessage(expID);

					availablePickupableIDs.push(expID);
				}
			}
		}
	}
}

void YagooPillarCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		CInstance* Self = std::get<0>(Args);
		if (!isHost)
		{
			if (abs(getInstanceVariable(Self, GML_depth).m_Real - 12345) > 1e-3)
			{
				setInstanceVariable(Self, GML_sprite_index, RValue(sprEmptyIndex));
				callbackManagerInterfacePtr->CancelOriginalFunction();
			}
		}
		else
		{
			// TODO: Not the best way to do this, but should still work (Low priority improve this)
			float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
			float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);
			short destructableIndex = static_cast<short>(destructableMap.size());
			destructableData data = destructableData(xPos, yPos, destructableIndex, 1);
			destructableMap[destructableIndex] = data;
			setInstanceVariable(Self, GML_destructableID, RValue(destructableIndex));
			createDestructableMessagesList.push_back(data);
		}
	}
}

void DestructableCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		CInstance* Self = std::get<0>(Args);
		if (!isHost)
		{
			if (abs(getInstanceVariable(Self, GML_depth).m_Real - 12345) > 1e-3)
			{
				setInstanceVariable(Self, GML_sprite_index, RValue(sprEmptyIndex));
				callbackManagerInterfacePtr->CancelOriginalFunction();
			}
		}
		else
		{
			// TODO: Not the best way to do this, but should still work (Low priority improve this)
			float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
			float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);
			short destructableIndex = static_cast<short>(destructableMap.size());
			destructableData data = destructableData(xPos, yPos, destructableIndex, 0);
			destructableMap[destructableIndex] = data;
			setInstanceVariable(Self, GML_destructableID, RValue(destructableIndex));
			createDestructableMessagesList.push_back(data);
		}
	}
}

void ObstacleStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && !isHost)
	{
		// TODO: Could probably find some better way to do this (low priority)
		CInstance* Self = std::get<0>(Args);
		RValue spriteIndex = getInstanceVariable(Self, GML_sprite_index);
		if (abs(spriteIndex.m_Real - sprEmptyIndex) < 1e-3)
		{
			g_ModuleInterface->CallBuiltin("instance_destroy", { Self });
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void YagooPillarStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && !isHost)
	{
		// TODO: Could probably find some better way to do this (low priority)
		CInstance* Self = std::get<0>(Args);
		RValue spriteIndex = getInstanceVariable(Self, GML_sprite_index);
		if (abs(spriteIndex.m_Real - sprEmptyIndex) < 1e-3)
		{
			g_ModuleInterface->CallBuiltin("instance_destroy", { Self });
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void AttackControllerCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		CInstance* Self = std::get<0>(Args);
		setInstanceVariable(Self, GML_persistent, RValue(true));
	}
}

void CautionStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		CInstance* Self = std::get<0>(Args);
		RValue lifetime = getInstanceVariable(Self, GML_lifetime);
		if (lifetime.m_Real < 1)
		{
			short dir = static_cast<short>(lround(getInstanceVariable(Self, GML_dir).m_Real));
			float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
			float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);
			sendAllCautionCreateMessage(cautionData(xPos, yPos, dir, 0));
		}
	}
}

void CautionAttackStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		CInstance* Self = std::get<0>(Args);
		RValue lifetime = getInstanceVariable(Self, GML_lifetime);
		if (lifetime.m_Real < 1)
		{
			short dir = static_cast<short>(lround(getInstanceVariable(Self, GML_dir).m_Real));
			float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
			float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);
			sendAllCautionCreateMessage(cautionData(xPos, yPos, dir, 1));
		}
	}
}

void PreCreateStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			CInstance* Self = std::get<0>(Args);
			short waitSpawn = static_cast<short>(lround(getInstanceVariable(Self, GML_waitSpawn).m_Real));
			if (waitSpawn >= 1)
			{
				float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
				float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);
				RValue preCreateMapIndex = getInstanceVariable(Self, GML_preCreateMapIndex);
				if (preCreateMapIndex.m_Kind == VALUE_UNSET)
				{
					short preCreateID = availablePreCreateIDs.front();
					availablePreCreateIDs.pop();
					preCreateMapIndex = RValue(preCreateID);
					setInstanceVariable(Self, GML_preCreateMapIndex, preCreateMapIndex);
				}

				short preCreateMapIndexVal = static_cast<short>(lround(preCreateMapIndex.m_Real));
				if (waitSpawn == 1)
				{
					// Could technically cause issues, but it would require all the IDs to be used which is unlikely
					availablePreCreateIDs.push(preCreateMapIndexVal);
				}

				sendAllPreCreateUpdateMessage(preCreateData(xPos, yPos, waitSpawn, preCreateMapIndexVal));
			}
		}
		else
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void VFXStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			// TODO: Reenable once I optimize the amount of data being uploaded
			/*
			CInstance* Self = std::get<0>(Args);
			short duration = static_cast<short>(lround(getInstanceVariable(Self, GML_duration).m_Real));
			if (duration >= 1)
			{
				float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
				float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);
				float imageXScale = static_cast<float>(getInstanceVariable(Self, GML_image_xscale).m_Real);
				float imageYScale = static_cast<float>(getInstanceVariable(Self, GML_image_yscale).m_Real);
				float imageAngle = static_cast<float>(getInstanceVariable(Self, GML_image_angle).m_Real);
				float imageAlpha = static_cast<float>(getInstanceVariable(Self, GML_image_alpha).m_Real);
				short spriteIndex = static_cast<short>(getInstanceVariable(Self, GML_sprite_index).m_Real);
				short imageIndex = static_cast<short>(getInstanceVariable(Self, GML_image_index).m_Real);
				int spriteColor = static_cast<int>(getInstanceVariable(Self, GML_spriteColor).m_Real);
				RValue vfxMapIndex = getInstanceVariable(Self, GML_vfxMapIndex);
				if (vfxMapIndex.m_Kind == VALUE_UNSET || vfxMapIndex.m_Kind == VALUE_UNDEFINED)
				{
					short vfxID = availableVFXIDs.front();
					availableVFXIDs.pop();
					vfxMapIndex = RValue(static_cast<double>(vfxID));
					setInstanceVariable(Self, GML_vfxMapIndex, vfxMapIndex);
				}

				short vfxMapIndexVal = static_cast<short>(lround(vfxMapIndex.m_Real));
				if (duration <= 2)
				{
					// Could technically cause issues, but it would require all the IDs to be used which is unlikely
					availableVFXIDs.push(vfxMapIndexVal);
					// Hacky way to signal that it should be destroyed
					imageAlpha = -1;
				}

				sendAllVFXUpdateMessage(vfxData(xPos, yPos, imageXScale, imageYScale, imageAngle, imageAlpha, spriteColor, spriteIndex, imageIndex, vfxMapIndexVal, 0));
			}
			*/
		}
		else
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void AfterImageStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			// TODO: Reenable once I optimize the amount of data being uploaded
			/*
			CInstance* Self = std::get<0>(Args);
			short deathTime = static_cast<short>(lround(getInstanceVariable(Self, GML_deathTime).m_Real));
			if (deathTime >= 1)
			{
				float xPos = static_cast<float>(getInstanceVariable(Self, GML_x).m_Real);
				float yPos = static_cast<float>(getInstanceVariable(Self, GML_y).m_Real);
				float imageXScale = static_cast<float>(getInstanceVariable(Self, GML_image_xscale).m_Real);
				float imageYScale = static_cast<float>(getInstanceVariable(Self, GML_image_yscale).m_Real);
				float imageAngle = static_cast<float>(getInstanceVariable(Self, GML_image_angle).m_Real);
				float imageAlpha = static_cast<float>(getInstanceVariable(Self, GML_image_alpha).m_Real);
				short spriteIndex = static_cast<short>(getInstanceVariable(Self, GML_sprite_index).m_Real);
				short imageIndex = static_cast<short>(getInstanceVariable(Self, GML_image_index).m_Real);
				int afterImageColor = static_cast<int>(getInstanceVariable(Self, GML_afterimage_color).m_Real);
				RValue afterImageMapIndex = getInstanceVariable(Self, GML_afterImageMapIndex);
				if (afterImageMapIndex.m_Kind == VALUE_UNSET || afterImageMapIndex.m_Kind == VALUE_UNDEFINED)
				{
					short afterImageID = availableVFXIDs.front();
					availableVFXIDs.pop();
					afterImageMapIndex = RValue(static_cast<double>(afterImageID));
					setInstanceVariable(Self, GML_afterImageMapIndex, afterImageMapIndex);
				}

				short afterImageMapIndexVal = static_cast<short>(lround(afterImageMapIndex.m_Real));
				if (deathTime == 1)
				{
					// Could technically cause issues, but it would require all the IDs to be used which is unlikely
					availableVFXIDs.push(afterImageMapIndexVal);
					// Hacky way to signal that it should be destroyed
					imageAlpha = -200000;
				}

				sendAllVFXUpdateMessage(vfxData(xPos, yPos, imageXScale, imageYScale, imageAngle, imageAlpha, afterImageColor, spriteIndex, imageIndex, afterImageMapIndexVal, 1));
			}
			*/
		}
		else
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void AfterImageAlarmBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		if (isHost)
		{
			// TODO: Reenable once I optimize the amount of data being uploaded
			/*
			CInstance* Self = std::get<0>(Args);
			float imageAlpha = static_cast<float>(getInstanceVariable(Self, GML_image_alpha).m_Real);
			if (imageAlpha < 0)
			{
				short afterImageMapIndex = static_cast<short>(lround(getInstanceVariable(Self, GML_afterImageMapIndex).m_Real));
				// Could technically cause issues, but it would require all the IDs to be used which is unlikely
				availableVFXIDs.push(afterImageMapIndex);
				// Hacky way of sending over a delete message
				sendAllVFXUpdateMessage(vfxData(0, 0, 0, 0, 0, -1, 0, 0, 0, afterImageMapIndex, 1));
			}
			*/
		}
		else
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

std::unordered_map<short, RValue> interactableMap;

void HoloBoxCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		CInstance* Self = std::get<0>(Args);
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

		sendAllInteractableCreateMessage(interactableData(xPos, yPos, interactableMapIndexVal, -1, 0));
	}
}

void HoloBoxCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		CInstance* Self = std::get<0>(Args);
		if (isHost)
		{
			setInstanceVariable(Self, GML_followPlayerID, playerMap[0]);
		}
		else
		{
			setInstanceVariable(Self, GML_followPlayerID, playerMap[clientID]);
		}
	}
}

void HoloAnvilCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		CInstance* Self = std::get<0>(Args);
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

		sendAllInteractableCreateMessage(interactableData(xPos, yPos, interactableMapIndexVal, -1, 1));
	}
}

void HoloAnvilCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		CInstance* Self = std::get<0>(Args);
		if (isHost)
		{
			setInstanceVariable(Self, GML_followPlayerID, playerMap[0]);
		}
		else
		{
			setInstanceVariable(Self, GML_followPlayerID, playerMap[clientID]);
		}
	}
}

void GoldenAnvilCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		CInstance* Self = std::get<0>(Args);
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

		sendAllInteractableCreateMessage(interactableData(xPos, yPos, interactableMapIndexVal, -1, 2));
	}
}

void GoldenAnvilCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		CInstance* Self = std::get<0>(Args);
		if (isHost)
		{
			setInstanceVariable(Self, GML_followPlayerID, playerMap[0]);
		}
		else
		{
			setInstanceVariable(Self, GML_followPlayerID, playerMap[clientID]);
		}
	}
}

void GoldenHammerCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		CInstance* Self = std::get<0>(Args);
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

		sendAllInteractableCreateMessage(interactableData(xPos, yPos, interactableMapIndexVal, -1, 3));
	}
}

void GoldenHammerCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		CInstance* Self = std::get<0>(Args);
		if (isHost)
		{
			setInstanceVariable(Self, GML_followPlayerID, playerMap[0]);
		}
		else
		{
			setInstanceVariable(Self, GML_followPlayerID, playerMap[clientID]);
		}
	}
}

void StickerCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected)
	{
		CInstance* Self = std::get<0>(Args);
		if (isHost)
		{
			setInstanceVariable(Self, GML_followPlayerID, playerMap[0]);
		}
		else
		{
			setInstanceVariable(Self, GML_followPlayerID, playerMap[clientID]);
		}
	}
}

void HoloBoxCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		if (isAnyInteracting)
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
			return;
		}
		RValue gotBox = getInstanceVariable(playerManagerInstanceVar, GML_gotBox);
		RValue gotAnvil = getInstanceVariable(playerManagerInstanceVar, GML_gotAnvil);
		RValue gotGoldenAnvil = getInstanceVariable(playerManagerInstanceVar, GML_gotGoldenAnvil);
		RValue leveled = getInstanceVariable(playerManagerInstanceVar, GML_leveled);
		if (!gotBox.AsBool() && !gotAnvil.AsBool() && !gotGoldenAnvil.AsBool() && !leveled.AsBool())
		{
			CInstance* Self = std::get<0>(Args);
			isAnyInteracting = true;
			CInstance* Other = std::get<1>(Args);
			uint32_t playerID = getPlayerID(getInstanceVariable(Other, GML_id).m_Object);
			if (playerID != 100000 && playerID != 0)
			{
				hasClientFinishedInteracting = false;
				RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
				RValue playerSnapshot = getInstanceVariable(playerManagerInstanceVar, GML_playerSnapshot);
				swapPlayerData(playerManagerInstanceVar, attackController, playerID);

				RValue returnVal;
				origGetBoxScript(playerManagerInstanceVar, nullptr, returnVal, 0, nullptr);
				setInstanceVariable(playerManagerInstanceVar, GML_gotBox, RValue(false));

				char boxItemAmount = static_cast<char>(lround(getInstanceVariable(playerManagerInstanceVar, GML_boxItemAmount).m_Real));
				char isSuperBox = static_cast<char>(lround(getInstanceVariable(playerManagerInstanceVar, GML_superBox).m_Real));
				RValue randomWeapon = getInstanceVariable(playerManagerInstanceVar, GML_randomWeapon);

				for (int i = 0; i < boxItemAmount; i++)
				{
					RValue optionIcon = getInstanceVariable(randomWeapon[i], GML_optionIcon);
					RValue optionType = getInstanceVariable(randomWeapon[i], GML_optionType);
					RValue optionName = getInstanceVariable(randomWeapon[i], GML_optionName);
					RValue optionID = getInstanceVariable(randomWeapon[i], GML_optionID);
					RValue optionDescription = getInstanceVariable(randomWeapon[i], GML_optionDescription);
					std::vector<std::string_view> optionDescriptionList;
					if (optionDescription.m_Kind == VALUE_STRING)
					{
						optionDescriptionList.push_back(optionDescription.AsString());
					}
					else if (optionDescription.m_Kind == VALUE_ARRAY)
					{
						int optionDescriptionLength = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { optionDescription }).m_Real));
						for (int j = 0; j < optionDescriptionLength; j++)
						{
							optionDescriptionList.push_back(optionDescription[j].AsString());
						}
					}
					else
					{
						g_ModuleInterface->Print(CM_RED, "UNEXPECTED TYPE WHEN SENDING OVER OPTION DESCRIPTION");
					}
					uint16_t optionIcon_Super = 0;
					if (isSuperBox)
					{
						optionIcon_Super = static_cast<uint16_t>(lround(getInstanceVariable(randomWeapon[i], GML_optionIcon_Super).m_Real));
						optionIcon = getInstanceVariable(randomWeapon[i], GML_optionIcon_Normal);
					}
					randomWeaponArr[i] = levelUpOption(optionType.AsString(), optionName.AsString(), optionID.AsString(), optionDescriptionList, static_cast<uint16_t>(optionIcon.m_Real), optionIcon_Super, 0);
				}

				short interactableMapIndexVal = static_cast<short>(lround(getInstanceVariable(Self, GML_interactableMapIndex).m_Real));
				sendAllBoxPlayerInteractedMessage(playerID, randomWeaponArr, interactableMapIndexVal, boxItemAmount, isSuperBox);
				swapPlayerData(playerManagerInstanceVar, attackController, 0);
				// Seems like the playersnapshot needs to be swapped because pausing does a player snapshot and swapping players will cause it to snapshot the client player
				setInstanceVariable(playerManagerInstanceVar, GML_playerSnapshot, playerSnapshot);
				g_ModuleInterface->CallBuiltin("instance_destroy", { Self });
				callbackManagerInterfacePtr->CancelOriginalFunction();
			}
			else
			{
				short interactableMapIndexVal = static_cast<short>(lround(getInstanceVariable(Self, GML_interactableMapIndex).m_Real));
				sendAllInteractableDeleteMessage(interactableMapIndexVal, 0);
			}
		}
		else
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void HoloAnvilCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		if (isAnyInteracting)
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
			return;
		}
		CInstance* Self = std::get<0>(Args);
		RValue canCollide = getInstanceVariable(Self, GML_canCollide);
		RValue gotBox = getInstanceVariable(playerManagerInstanceVar, GML_gotBox);
		RValue gotAnvil = getInstanceVariable(playerManagerInstanceVar, GML_gotAnvil);
		RValue gotGoldenAnvil = getInstanceVariable(playerManagerInstanceVar, GML_gotGoldenAnvil);
		RValue leveled = getInstanceVariable(playerManagerInstanceVar, GML_leveled);
		if (!gotBox.AsBool() && !gotAnvil.AsBool() && !gotGoldenAnvil.AsBool() && !leveled.AsBool() && canCollide.AsBool())
		{
			isAnyInteracting = true;
			CInstance* Other = std::get<1>(Args);
			uint32_t playerID = getPlayerID(getInstanceVariable(Other, GML_id).m_Object);
			if (playerID != 100000 && playerID != 0)
			{
				hasClientFinishedInteracting = false;
				setInstanceVariable(playerManagerInstanceVar, GML_paused, RValue(true));
				RValue returnVal;
				origPauseScript(playerManagerInstanceVar, nullptr, returnVal, 0, nullptr);
				sendAllHostHasPausedMessage();

				short interactableMapIndexVal = static_cast<short>(lround(getInstanceVariable(Self, GML_interactableMapIndex).m_Real));
				sendAllInteractablePlayerInteractedMessage(playerID, interactableMapIndexVal, 1);
				setInstanceVariable(Self, GML_canCollide, RValue(false));
				setInstanceVariable(playerManagerInstanceVar, GML_anvilID, RValue(Self));
				RValue alarmTime = 60;
				g_ModuleInterface->SetBuiltin("alarm", Self, 0, alarmTime);
				callbackManagerInterfacePtr->CancelOriginalFunction();
			}
		}
		else
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void GoldenAnvilCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		if (isAnyInteracting)
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
			return;
		}
		CInstance* Self = std::get<0>(Args);
		RValue canCollide = getInstanceVariable(Self, GML_canCollide);
		RValue gotBox = getInstanceVariable(playerManagerInstanceVar, GML_gotBox);
		RValue gotAnvil = getInstanceVariable(playerManagerInstanceVar, GML_gotAnvil);
		RValue gotGoldenAnvil = getInstanceVariable(playerManagerInstanceVar, GML_gotGoldenAnvil);
		RValue leveled = getInstanceVariable(playerManagerInstanceVar, GML_leveled);
		if (!gotBox.AsBool() && !gotAnvil.AsBool() && !gotGoldenAnvil.AsBool() && !leveled.AsBool() && canCollide.AsBool())
		{
			isAnyInteracting = true;
			CInstance* Other = std::get<1>(Args);
			uint32_t playerID = getPlayerID(getInstanceVariable(Other, GML_id).m_Object);
			if (playerID != 100000 && playerID != 0)
			{
				hasClientFinishedInteracting = false;
				setInstanceVariable(playerManagerInstanceVar, GML_paused, RValue(true));
				RValue returnVal;
				origPauseScript(playerManagerInstanceVar, nullptr, returnVal, 0, nullptr);
				sendAllHostHasPausedMessage();

				short interactableMapIndexVal = static_cast<short>(lround(getInstanceVariable(Self, GML_interactableMapIndex).m_Real));
				sendAllInteractablePlayerInteractedMessage(playerID, interactableMapIndexVal, 2);
				setInstanceVariable(Self, GML_canCollide, RValue(false));
				setInstanceVariable(playerManagerInstanceVar, GML_anvilID, RValue(Self));
				RValue alarmTime = 60;
				g_ModuleInterface->SetBuiltin("alarm", Self, 0, alarmTime);
				callbackManagerInterfacePtr->CancelOriginalFunction();
			}
		}
		else
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void GoldenHammerCleanUpBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		CInstance* Self = std::get<0>(Args);
		short interactableMapIndexVal = static_cast<short>(lround(getInstanceVariable(Self, GML_interactableMapIndex).m_Real));
		sendAllInteractableDeleteMessage(interactableMapIndexVal, 3);
	}
}

void StickerCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		if (isAnyInteracting)
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
			return;
		}

		CInstance* Self = std::get<0>(Args);
		RValue canCollide = getInstanceVariable(Self, GML_canCollide);
		RValue gotBox = getInstanceVariable(playerManagerInstanceVar, GML_gotBox);
		RValue gotAnvil = getInstanceVariable(playerManagerInstanceVar, GML_gotAnvil);
		RValue gotGoldenAnvil = getInstanceVariable(playerManagerInstanceVar, GML_gotGoldenAnvil);
		RValue leveled = getInstanceVariable(playerManagerInstanceVar, GML_leveled);
		if (!gotBox.AsBool() && !gotAnvil.AsBool() && !gotGoldenAnvil.AsBool() && !leveled.AsBool() && canCollide.AsBool())
		{
			isAnyInteracting = true;
			CInstance* Other = std::get<1>(Args);
			uint32_t playerID = getPlayerID(getInstanceVariable(Other, GML_id).m_Object);
			if (playerID != 100000 && playerID != 0)
			{
				isClientUsingStamp = true;
				hasClientFinishedInteracting = false;
				setInstanceVariable(playerManagerInstanceVar, GML_paused, RValue(true));
				RValue returnVal;
				origPauseScript(playerManagerInstanceVar, nullptr, returnVal, 0, nullptr);
				sendAllHostHasPausedMessage();

				short interactableMapIndexVal = static_cast<short>(lround(getInstanceVariable(Self, GML_interactableMapIndex).m_Real));
				RValue stickerID = getInstanceVariable(Self, GML_stickerID);
				collidedStickerID = stickerID.AsString();
				g_ModuleInterface->CallBuiltin("variable_global_set", { "collectedSticker", getInstanceVariable(Self, GML_stickerData) });

				setInstanceVariable(playerManagerInstanceVar, GML_stickerID, RValue(Self));

				sendAllStickerPlayerInteractedMessage(playerID, stickerID.AsString(), interactableMapIndexVal);
				setInstanceVariable(Self, GML_canCollide, RValue(false));
				RValue alarmTime = 60;
				g_ModuleInterface->SetBuiltin("alarm", Self, 0, alarmTime);
				callbackManagerInterfacePtr->CancelOriginalFunction();
			}
		}
		else
		{
			callbackManagerInterfacePtr->CancelOriginalFunction();
		}
	}
}

void StickerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (hasConnected && isHost)
	{
		CInstance* Self = std::get<0>(Args);
		if (getInstanceVariable(Self, GML_used).AsBool() || (getInstanceVariable(Self, GML_destroyIfNoneLeft).AsBool() && getInstanceVariable(Self, GML_stickerData).m_Kind != VALUE_OBJECT))
		{
			short interactableMapIndexVal = static_cast<short>(lround(getInstanceVariable(Self, GML_interactableMapIndex).m_Real));
			sendAllInteractableDeleteMessage(interactableMapIndexVal, 4);
		}
	}
}

void TextControllerCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
}

void drawCoopMenuButtons(std::vector<std::string> CoopOptionMenuTextList, CInstance* Self, double buttonX, double buttonY)
{
	double buttonYSpacing = 29;
	double textColor[2]{ 0xFFFFFF, 0 };
	RValue inputManager = g_ModuleInterface->CallBuiltin("instance_find", { objInputManagerIndex, 0 });
	bool isMouseMoving = getInstanceVariable(inputManager, GML_mouseMoving).AsBool();
	for (int i = 0; i < CoopOptionMenuTextList.size(); i++)
	{
		if (isMouseMoving)
		{
			RValue** args = new RValue*[4];
			RValue mouseOverType = "long";
			RValue curButtonX = buttonX;
			RValue curButtonY = buttonY + i * buttonYSpacing;
			RValue scale = .5;
			args[0] = &mouseOverType;
			args[1] = &curButtonX;
			args[2] = &curButtonY;
			args[3] = &scale;
			RValue returnVal;
			origMouseOverButtonScript(Self, nullptr, returnVal, 4, args);
			if (returnVal.AsBool())
			{
				curCoopOptionMenuIndex = i;
			}
		}
		int isOptionSelected = curCoopOptionMenuIndex == i;
		g_ModuleInterface->CallBuiltin("draw_set_font", { jpFont });
		g_ModuleInterface->CallBuiltin("draw_set_halign", { 1 });
		g_ModuleInterface->CallBuiltin("draw_sprite_ext", { sprHudInitButtonsIndex, isOptionSelected, buttonX, buttonY + i * buttonYSpacing, 1, 1, 0, static_cast<double>(0xFFFFFF), 1 });
		g_ModuleInterface->CallBuiltin("draw_text_color", { buttonX, buttonY + 9 + i * buttonYSpacing, CoopOptionMenuTextList[i], textColor[isOptionSelected], textColor[isOptionSelected], textColor[isOptionSelected], textColor[isOptionSelected], 1 });
	}
}

int adapterPageNum = 0;
int prevPageButtonNum = -1;
int nextPageButtonNum = -1;

int menuButtonsNum = 0;
int menuButtonsXPos = 0;
int menuButtonsYPos = 0;
void TitleScreenDrawBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (isInGamemodeSelect)
	{
		drawCoopMenuButtons({ "Play Single Player", "Use saved network adapter", "Select network adapter" }, std::get<0>(Args), 530, 104);
		menuButtonsNum = 3;
		menuButtonsXPos = 530;
		menuButtonsYPos = 104;
	}
	else if (isInNetworkAdapterMenu)
	{
		if (!hasReadNetworkAdapterDisclaimer)
		{
			CInstance* Self = std::get<0>(Args);
			const char* networkAdapterDisclaimer = "DISCLAIMER: Please be aware that clicking the OK button will bring up a list of your computer's network adapter names which may contain private/sensitive information.";
			g_ModuleInterface->CallBuiltin("draw_set_font", { jpFont });
			g_ModuleInterface->CallBuiltin("draw_set_halign", { 1 });
			drawTextOutline(Self, 320, 100, networkAdapterDisclaimer, 1, 0x000000, 14, 20, 440, 0x0FFFFF, 1);

			drawCoopMenuButtons({ "OK" }, std::get<0>(Args), 320, 250);
			menuButtonsNum = 1;
			menuButtonsXPos = 320;
			menuButtonsYPos = 250;
		}
		else
		{
			if (adapterAddresses != NULL)
			{
				std::vector<std::string> adapterNames;
				IP_ADAPTER_ADDRESSES* adapter(NULL);

				bool hasPrev = false;
				bool hasNext = false;
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
					if (count < adapterPageNum * 5)
					{
						hasPrev = true;
						continue;
					}
					if (count >= (adapterPageNum + 1) * 5)
					{
						hasNext = true;
						break;
					}

					int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, static_cast<int>(wcslen(adapter->FriendlyName)), NULL, 0, NULL, NULL);
					std::string resString(sizeNeeded, 0);
					WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, static_cast<int>(wcslen(adapter->FriendlyName)), &resString[0], sizeNeeded, NULL, NULL);
					adapterNames.push_back(resString);
				}
				if (hasPrev)
				{
					prevPageButtonNum = static_cast<int>(adapterNames.size());
					adapterNames.push_back("Prev Page");
				}
				else
				{
					prevPageButtonNum = -1;
				}
				if (hasNext)
				{
					nextPageButtonNum = static_cast<int>(adapterNames.size());
					adapterNames.push_back("Next Page");
				}
				else
				{
					nextPageButtonNum = -1;
				}
				drawCoopMenuButtons(adapterNames, std::get<0>(Args), 530, 104);
				menuButtonsNum = static_cast<int>(adapterNames.size());
				menuButtonsXPos = 530;
				menuButtonsYPos = 104;
			}
		}
	}
	else if (isInCoopOptionMenu)
	{
		drawCoopMenuButtons({ "Host LAN Session", "Join LAN Session" }, std::get<0>(Args), 530, 104);
		menuButtonsNum = 2;
		menuButtonsXPos = 530;
		menuButtonsYPos = 104;
	}
	else if (isInLobby && (isHost || hasObtainedClientID))
	{
		CInstance* Self = std::get<0>(Args);
		RValue returnVal;
		RValue characterDataMap = g_ModuleInterface->CallBuiltin("variable_global_get", { "characterData" });
		int curPlayerIndex = 0;
		for (auto& curPlayerData : lobbyPlayerDataMap)
		{
			lobbyPlayerData curLobbyPlayerData = curPlayerData.second;
			double textXPos = 20;
			double textYPos = curPlayerIndex * 32 + 14;
			drawTextOutline(Self, textXPos, textYPos, std::format("P{}", curLobbyPlayerData.playerName), 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1);

			if (!curLobbyPlayerData.charName.empty())
			{
				RValue charData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { characterDataMap, curLobbyPlayerData.charName });
				RValue idleSprite = getInstanceVariable(charData, GML_sprite1);
				g_ModuleInterface->CallBuiltin("draw_sprite_ext", { idleSprite, -1, textXPos + 30, textYPos + 20, 1, 1, 0, 0xFFFFFF, 1 });
			}
			if (curLobbyPlayerData.isReady)
			{
				drawTextOutline(Self, textXPos + 64, textYPos, "Ready", 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1);
			}
			curPlayerIndex++;
		}
		if (lobbyPlayerDataMap[0].stageSprite != -1)
		{
			g_ModuleInterface->CallBuiltin("draw_sprite", { lobbyPlayerDataMap[0].stageSprite, 0, 455, 10 });
		}
		std::vector<std::string> CoopOptionMenuTextList{ "Choose Character", "Ready" };
		if (lobbyPlayerDataMap[clientID].isReady)
		{
			CoopOptionMenuTextList[1] = "Unready";
		}
		if (isHost)
		{
			if (!lobbyPlayerDataMap[clientID].charName.empty())
			{
				CoopOptionMenuTextList.push_back("Select Map");
				if (hasSelectedMap)
				{
					bool areAllPlayersReady = true;
					for (auto& curPlayerData : lobbyPlayerDataMap)
					{
						lobbyPlayerData curLobbyPlayerData = curPlayerData.second;
						if (curLobbyPlayerData.isReady == 0)
						{
							areAllPlayersReady = false;
							break;
						}
					}
					if (areAllPlayersReady && lobbyPlayerDataMap.size() >= 2)
					{
						CoopOptionMenuTextList.push_back("Start");
					}
				}
			}
		}
		drawCoopMenuButtons(CoopOptionMenuTextList, std::get<0>(Args), 530, 104);
		menuButtonsNum = static_cast<int>(CoopOptionMenuTextList.size());
		menuButtonsXPos = 530;
		menuButtonsYPos = 104;
	}
}

int numTimesFailedToConnect = 0;
int numFramesSinceLastBroadcast = 0;
void TitleScreenStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (isInCoopOptionMenu || isInLobby || isSelectingCharacter || isSelectingMap)
	{
		CInstance* Self = std::get<0>(Args);
		setInstanceVariable(Self, GML_idletime, RValue(0.0));
		if (isInLobby || isSelectingCharacter || isSelectingMap)
		{
			// TODO: replace this code with something better
			if (isHost)
			{
				for (auto& clientSocketData : clientSocketMap)
				{
					uint32_t clientSocketPlayerID = clientSocketData.first;
					SOCKET clientSocket = clientSocketData.second;
					for (auto& curPlayerData : lobbyPlayerDataMap)
					{
						uint32_t curPlayerID = curPlayerData.first;
						// Skips sending the client's character data to themself
						if (clientSocketPlayerID == curPlayerID)
						{
							continue;
						}
						sendCharDataMessage(clientSocket, curPlayerData.second, curPlayerID);
					}
				}
			}
			else
			{
				if (hasObtainedClientID)
				{
					sendCharDataMessage(serverSocket, lobbyPlayerDataMap[clientID], clientID);
				}
			}
		}
		if (isInLobby)
		{
			if (isHost)
			{
				struct sockaddr_storage clientAddr;
				socklen_t addrLen = sizeof(clientAddr);
				char buffer[6];
				int numbytes = recvfrom(listenSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&clientAddr, &addrLen);
				bool hasSentBroadcast = false;
				while (numbytes > 0)
				{
					buffer[numbytes] = '\0';
					// Received broadcast message from client. Create listening socket and broadcast back to let clients know the host address
					if (!hasSentBroadcast && strncmp("From1", buffer, sizeof(buffer) - 1) == 0)
					{
						// TODO: Probably should set a time limit between how often the host broadcasts back
						const char* message = "From2";
						sendto(broadcastSocket, message, 5, 0, broadcastSocketAddr, static_cast<int>(broadcastSocketLen));
						hasSentBroadcast = true;
					}
					numbytes = recvfrom(listenSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&clientAddr, &addrLen);
				}
				// TODO: Seems like this has some issues sometimes where if the host is set up after the client, it has trouble connecting?
				fd_set readFDSet;
				FD_ZERO(&readFDSet);
				FD_SET(connectClientSocket, &readFDSet);
				timeval timeout{};
				timeout.tv_sec = 0;
				timeout.tv_usec = 100;
				int result = select(0, &readFDSet, NULL, NULL, &timeout);
				if (result > 0 && FD_ISSET(connectClientSocket, &readFDSet))
				{
					SOCKET clientSocket = accept(connectClientSocket, NULL, NULL);
					if (clientSocket == INVALID_SOCKET)
					{
						g_ModuleInterface->Print(CM_RED, "COULDN'T ACCEPT SOCKET %d\n", WSAGetLastError());
					}
					u_long mode = 1;
					ioctlsocket(clientSocket, FIONBIO, &mode);
					if (clientSocket != INVALID_SOCKET)
					{
						printf("client socket obtained\n");
						uint32_t newClientID = curUnusedPlayerID;
						curUnusedPlayerID++;
						// TODO: Handle if the message isn't sent
						sendClientIDMessage(clientSocket, newClientID);
						clientSocketMap[newClientID] = clientSocket;
						playerPingMap[newClientID] = 0;
						hasClientPlayerDisconnected[newClientID] = false;
						lobbyPlayerDataMap[newClientID] = lobbyPlayerData();
						if (!hasConnected)
						{
							hasConnected = true;
							messageHandlerThread = std::thread(hostReceiveMessageHandler);
						}
					}
				}
			}
			else if (!hasConnected)
			{
				if (serverSocket == INVALID_SOCKET)
				{
					if (numFramesSinceLastBroadcast > 60)
					{
						const char* message = "From1";
						sendto(broadcastSocket, message, 5, 0, broadcastSocketAddr, static_cast<int>(broadcastSocketLen));
						numFramesSinceLastBroadcast = 0;
					}
					numFramesSinceLastBroadcast++;
				}

				struct sockaddr_storage receivedAddr;
				socklen_t addrLen = sizeof(receivedAddr);
				char buffer[6];
				int numbytes = recvfrom(listenSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&receivedAddr, &addrLen);
				while (numbytes > 0)
				{
					buffer[numbytes] = '\0';
					// Received broadcast message from host. Try connecting to the host with the received address
					if (serverSocket == INVALID_SOCKET && strncmp("From2", buffer, sizeof(buffer) - 1) == 0)
					{
						auto sinAddr = ((struct sockaddr_in*)&receivedAddr)->sin_addr;
						char strBuffer[16] = { 0 };
						PCSTR hostAddress = inet_ntop(receivedAddr.ss_family, &(sinAddr), strBuffer, 16);
						ULONG addr = sinAddr.S_un.S_addr;

						struct addrinfo* result = NULL, * ptr = NULL, hints;
						ZeroMemory(&hints, sizeof(hints));
						hints.ai_family = AF_INET;
						hints.ai_socktype = SOCK_STREAM;
						hints.ai_protocol = IPPROTO_TCP;
						int iResult = getaddrinfo(hostAddress, GAME_PORT, &hints, &result);

						if (iResult != 0)
						{
							g_ModuleInterface->Print(CM_RED, "Failed to get address info: %d\n", iResult);
							return;
						}

						serverSocket = INVALID_SOCKET;

						for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
						{
							serverSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
							if (serverSocket == INVALID_SOCKET)
							{
								printf("INVALID SOCKET\n");
								continue;
							}
							u_long mode = 1;
							ioctlsocket(serverSocket, FIONBIO, &mode);
							iResult = connect(serverSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
							if (iResult == SOCKET_ERROR)
							{
								if (WSAGetLastError() == WSAEWOULDBLOCK)
								{
									return;
								}
								g_ModuleInterface->Print(CM_RED, "Couldn't connect %d\n", WSAGetLastError());
								closesocket(serverSocket);
								serverSocket = INVALID_SOCKET;
								continue;
							}
							break;
						}
						if (serverSocket != INVALID_SOCKET)
						{
							printf("Connected to host\n");

							numTimesFailedToConnect = 0;
							hasConnected = true;
							messageHandlerThread = std::thread(clientReceiveMessageHandler);
						}
						freeaddrinfo(result);
					}
					numbytes = recvfrom(listenSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&receivedAddr, &addrLen);
				}
				// Check on ongoing connection to see if it has succeeded yet
				if (serverSocket != INVALID_SOCKET)
				{
					if (numTimesFailedToConnect >= 50)
					{
						// Didn't manage to connect, so close the socket and try again
						g_ModuleInterface->Print(CM_RED, "Failed to connect to host");
						closesocket(serverSocket);
						serverSocket = INVALID_SOCKET;
						numTimesFailedToConnect = 0;
						return;
					}
					numTimesFailedToConnect++;
					fd_set writeFDSet;
					FD_ZERO(&writeFDSet);
					FD_SET(serverSocket, &writeFDSet);
					timeval timeout{};
					timeout.tv_sec = 0;
					timeout.tv_usec = 100;
					int result = select(0, NULL, &writeFDSet, NULL, &timeout);
					if (result > 0 && FD_ISSET(serverSocket, &writeFDSet))
					{
						printf("Connected to host\n");

						numTimesFailedToConnect = 0;
						hasConnected = true;
						messageHandlerThread = std::thread(clientReceiveMessageHandler);
						return;
					}
				}
			}
		}
	}
}

void TitleCharacterDrawBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (isInLobby || isSelectingCharacter || isSelectingMap || (isInNetworkAdapterMenu && !hasReadNetworkAdapterDisclaimer))
	{
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}
}

void TitleScreenCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	CInstance* Self = std::get<0>(Args);
	if (isInLobby)
	{
		// Should only happen after the host retries the game when it ends
		setInstanceVariable(Self, GML_canControl, RValue(false));
		RValue disableAlarmVal = -1;
		g_ModuleInterface->SetBuiltin("alarm", Self, 0, disableAlarmVal);
	}
	g_ModuleInterface->CallBuiltin("instance_create_depth", { 0, 0, 0, objCharacterDataIndex });
	RValue strVersion = getInstanceVariable(Self, GML_version);
	RValue newStrVersion = g_ModuleInterface->CallBuiltin("string_concat", { strVersion, " ", MODNAME });
	setInstanceVariable(Self, GML_version, newStrVersion);
}

void TitleScreenMouse53Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args)
{
	if (isInGamemodeSelect || isInNetworkAdapterMenu || isInCoopOptionMenu || (isInLobby && (isHost || hasObtainedClientID)))
	{
		CInstance* Self = std::get<0>(Args);
		for (int i = 0; i < menuButtonsNum; i++)
		{
			RValue** args = new RValue*[4];
			RValue mouseOverType = "long";
			RValue curButtonX = menuButtonsXPos;
			RValue curButtonY = menuButtonsYPos + i * 29 - 3;
			args[0] = &mouseOverType;
			args[1] = &curButtonX;
			args[2] = &curButtonY;
			RValue returnVal;
			origMouseOverButtonScript(Self, nullptr, returnVal, 3, args);
			if (returnVal.AsBool())
			{
				RValue ConfirmedMethod = getInstanceVariable(Self, GML_Confirmed);
				RValue ConfirmedMethodArr = g_ModuleInterface->CallBuiltin("array_create", { RValue(0.0) });
				g_ModuleInterface->CallBuiltin("method_call", { ConfirmedMethod, ConfirmedMethodArr });
			}
		}
		callbackManagerInterfacePtr->CancelOriginalFunction();
	}
}