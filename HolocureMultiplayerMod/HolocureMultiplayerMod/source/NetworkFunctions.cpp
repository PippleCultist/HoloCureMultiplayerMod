#include "NetworkFunctions.h"
#include "ModuleMain.h"
#include "ScriptFunctions.h"
#include "CodeEvents.h"
#include "CommonFunctions.h"
#include "steam/steam_api.h"
#include "SteamLobbyBrowser.h"
#include <semaphore>
#include <thread>

extern menuGrid lobbyMenuGrid;
extern menuGrid selectingCharacterMenuGrid;
extern menuGrid selectingMapMenuGrid;
extern std::unordered_map<uint32_t, uint64> clientIDToSteamIDMap;
extern std::unordered_map<uint64, uint32_t> steamIDToClientIDMap;
extern CSteamLobbyBrowser* steamLobbyBrowser;
extern std::unordered_map<uint64, steamConnection> steamIDToConnectionMap;
extern double foodMultiplier;

messageInstancesCreate instancesCreateMessage;
messageInstancesUpdate instancesUpdateMessage;
messageInstancesDelete instancesDeleteMessage;
messageAttackCreate attackCreateMessage;
messageAttackUpdate attackUpdateMessage;
messageAttackDelete attackDeleteMessage;

std::queue<uint16_t> availableInstanceIDs;
std::queue<uint16_t> availableAttackIDs;
std::queue<uint16_t> availablePickupableIDs;
std::queue<uint16_t> availablePreCreateIDs;
std::queue<uint16_t> availableVFXIDs;
std::queue<uint16_t> availableInteractableIDs;
RValue instanceArr[maxNumAvailableInstanceIDs];

std::unordered_map<short, RValue> attackMap;
RValue pickupableArr[maxNumAvailablePickupableIDs];

std::binary_semaphore lastTimeReceivedMoveDataMapLock(1);
std::unordered_map<uint32_t, clientMovementQueueData> lastTimeReceivedMoveDataMap;

std::binary_semaphore roomMessageLock(1);
bool hasReceivedRoomMessage = false;
messageRoom roomMessage;

std::binary_semaphore createInstancesMessageQueueLock(1);
std::queue<messageInstancesCreate> createInstancesMessageQueue;

std::binary_semaphore updateInstancesMessageQueueLock(1);
std::queue<messageInstancesUpdate> updateInstancesMessageQueue;

std::binary_semaphore deleteInstancesMessageQueueLock(1);
std::queue<messageInstancesDelete> deleteInstancesMessageQueue;

std::binary_semaphore clientPlayerDataMessageQueueLock(1);
std::queue<messageClientPlayerData> clientPlayerDataMessageQueue;

std::binary_semaphore createAttackMessageQueueLock(1);
std::queue<messageAttackCreate> createAttackMessageQueue;

std::binary_semaphore updateAttackMessageQueueLock(1);
std::queue<messageAttackUpdate> updateAttackMessageQueue;

std::binary_semaphore deleteAttackMessageQueueLock(1);
std::queue<messageAttackDelete> deleteAttackMessageQueue;

std::binary_semaphore clientIDMessageQueueLock(1);
std::queue<messageClientID> clientIDMessageQueue;

std::binary_semaphore createPickupableMessageQueueLock(1);
std::queue<messagePickupableCreate> createPickupableMessageQueue;

std::binary_semaphore updatePickupableMessageQueueLock(1);
std::queue<messagePickupableUpdate> updatePickupableMessageQueue;

std::binary_semaphore deletePickupableMessageQueueLock(1);
std::queue<messagePickupableDelete> deletePickupableMessageQueue;

std::binary_semaphore gameDataMessageQueueLock(1);
std::queue<messageGameData> gameDataMessageQueue;

std::binary_semaphore levelUpOptionsMessageQueueLock(1);
std::queue<messageLevelUpOptions> levelUpOptionsMessageQueue;

std::binary_semaphore levelUpClientChoiceMessageQueueLock(1);
std::queue<messageLevelUpClientChoice> levelUpClientChoiceMessageQueue;

std::binary_semaphore destructableCreateMessageQueueLock(1);
std::queue<messageDestructableCreate> destructableCreateMessageQueue;

std::binary_semaphore destructableBreakMessageQueueLock(1);
std::queue<messageDestructableBreak> destructableBreakMessageQueue;

std::binary_semaphore eliminateLevelUpClientChoiceMessageQueueLock(1);
std::queue<messageEliminateLevelUpClientChoice> eliminateLevelUpClientChoiceMessageQueue;

std::binary_semaphore clientSpecialAttackMessageQueueLock(1);
std::queue<uint32_t> clientSpecialAttackMessageQueue;

std::binary_semaphore cautionCreateMessageQueueLock(1);
std::queue<messageCautionCreate> cautionCreateMessageQueue;

std::binary_semaphore preCreateUpdateMessageQueueLock(1);
std::queue<messagePreCreateUpdate> preCreateUpdateMessageQueue;

std::binary_semaphore vfxUpdateMessageQueueLock(1);
std::queue<messageVfxUpdate> vfxUpdateMessageQueue;

std::binary_semaphore interactableCreateMessageQueueLock(1);
std::queue<messageInteractableCreate> interactableCreateMessageQueue;

std::binary_semaphore interactableDeleteMessageQueueLock(1);
std::queue<messageInteractableDelete> interactableDeleteMessageQueue;

std::binary_semaphore interactablePlayerInteractedMessageQueueLock(1);
std::queue<messageInteractablePlayerInteracted> interactablePlayerInteractedMessageQueue;

std::binary_semaphore stickerPlayerInteractedMessageQueueLock(1);
std::queue<messageStickerPlayerInteracted> stickerPlayerInteractedMessageQueue;

std::binary_semaphore boxPlayerInteractedMessageQueueLock(1);
std::queue<messageBoxPlayerInteracted> boxPlayerInteractedMessageQueue;

std::binary_semaphore interactFinishedMessageQueueLock(1);
bool hasInteractFinishedMessage = false;

std::binary_semaphore boxTakeOptionMessageQueueLock(1);
std::queue<messageBoxTakeOption> boxTakeOptionMessageQueue;

std::binary_semaphore anvilChooseOptionMessageQueueLock(1);
std::queue<messageAnvilChooseOption> anvilChooseOptionMessageQueue;

std::binary_semaphore clientGainMoneyMessageQueueLock(1);
std::queue<messageClientGainMoney> clientGainMoneyMessageQueue;

std::binary_semaphore clientAnvilEnchantMessageQueueLock(1);
std::queue<messageClientAnvilEnchant> clientAnvilEnchantMessageQueue;

std::binary_semaphore stickerChooseOptionMessageQueueLock(1);
std::queue<messageStickerChooseOption> stickerChooseOptionMessageQueue;

std::binary_semaphore chooseCollabMessageQueueLock(1);
std::queue<messageChooseCollab> chooseCollabMessageQueue;

std::binary_semaphore buffDataMessageQueueLock(1);
std::queue<messageBuffData> buffDataMessageQueue;

std::binary_semaphore charDataMessageQueueLock(1);
std::queue<messageCharData> charDataMessageQueue;

std::binary_semaphore ReturnToLobbyQueueLock(1);
bool hasReturnToLobby = false;

std::binary_semaphore lobbyPlayerDisconnectedQueueLock(1);
std::queue<messageLobbyPlayerDisconnected> lobbyPlayerDisconnectedQueue;

std::binary_semaphore hostHasPausedQueueLock(1);
bool receivedHostHasPaused = false;

std::binary_semaphore hostHasUnpausedQueueLock(1);
bool receivedHostHasUnpaused = false;

std::binary_semaphore kaelaOreAmountQueueLock(1);
std::queue<messageKaelaOreAmount> kaelaOreAmountQueue;

// Message handler should probably only handle receiving messages. Let the sending be handled somewhere else

void clientReceiveMessageHandler()
{
	while (hasConnected)
	{
		int result = -1;
		do
		{
			result = receiveMessage(0);
			if (result == 0 || (result == -1 && (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAECONNABORTED)))
			{
				g_ModuleInterface->Print(CM_RED, "Server disconnected");
				clientLeaveGame(true);
				return;
			}
		} while (result > 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void hostReceiveMessageHandler()
{
	while (hasConnected)
	{
		std::vector<uint32_t> playerDisconnectedList;
		for (auto& curClientIDMapping : clientIDToSteamIDMap)
		{
			uint32_t clientPlayerID = curClientIDMapping.first;
			int result = -1;
			do
			{
				result = receiveMessage(clientPlayerID);
				if (result == 0 || (result == -1 && (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAECONNABORTED)))
				{
					g_ModuleInterface->Print(CM_RED, "Client disconnected");
					playerDisconnectedList.push_back(clientPlayerID);
				}
			} while (result > 0);
		}
		std::shared_ptr<menuGridData> curMenuGridPtr;
		holoCureMenuInterfacePtr->GetCurrentMenuGrid(MODNAME, curMenuGridPtr);
		if (curMenuGridPtr == lobbyMenuGrid.menuGridPtr || curMenuGridPtr == selectingCharacterMenuGrid.menuGridPtr || curMenuGridPtr == selectingMapMenuGrid.menuGridPtr)
		{
			for (uint32_t disconnectedPlayerID : playerDisconnectedList)
			{
				playerPingMap.erase(disconnectedPlayerID);
				clientUnpausedMap.erase(disconnectedPlayerID);
				clientSocketMap.erase(disconnectedPlayerID);
				lobbyPlayerDataMap.erase(disconnectedPlayerID);
				hasClientPlayerDisconnected.erase(disconnectedPlayerID);
				uint64 steamID = clientIDToSteamIDMap[disconnectedPlayerID];
				clientIDToSteamIDMap.erase(disconnectedPlayerID);
				steamIDToConnectionMap.erase(steamID);
				steamIDToClientIDMap.erase(steamID);
				sendAllLobbyPlayerDisconnectedMessage(disconnectedPlayerID);
			}
		}
		else
		{
			// TODO: Do something if the client disconnects during a game
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void processLevelUp(levelUpPausedData& levelUpData, CInstance* playerManagerInstance)
{
	callbackManagerInterfacePtr->LogToFile(MODNAME, "Processing level up %s for client %d", std::string(levelUpData.levelUpName.ToString()).c_str(), curPlayerID);
	RValue levelUpName = levelUpData.levelUpName;
	RValue returnVal;
	switch (levelUpData.levelUpType)
	{
		case optionType_Weapon:
		{
			RValue** args = new RValue*[1];
			args[0] = &levelUpName;
			origAddAttackPlayerManagerOtherScript(playerManagerInstance, nullptr, returnVal, 1, args);
			break;
		}
		case optionType_Skill:
		{
			RValue** args = new RValue*[1];
			args[0] = &levelUpName;
			origAddPerkScript(playerManagerInstance, nullptr, returnVal, 1, args);
			if (isHost)
			{
				auto curSummon = summonMap[curPlayerID];
				if (curSummon.m_Kind == VALUE_UNDEFINED)
				{
					RValue playerSummon = getInstanceVariable(playerManagerInstance, GML_playerSummon);
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
//						g_ModuleInterface->Print(CM_RED, "Couldn't find player summon %d", playerSummon.m_Kind);
					}
				}
			}
			break;
		}
		case optionType_Item:
		{
			RValue** args = new RValue*[1];
			args[0] = &levelUpName;
			origAddItemScript(playerManagerInstance, nullptr, returnVal, 1, args);
			break;
		}
		case optionType_Consumable:
		{
			RValue** args = new RValue*[1];
			args[0] = &levelUpName;
			origAddConsumableScript(playerManagerInstance, nullptr, returnVal, 1, args);
			break;
		}
		case optionType_StatUp:
		{
			RValue** args = new RValue*[1];
			args[0] = &levelUpName;
			origAddStatScript(playerManagerInstance, nullptr, returnVal, 1, args);
			break;
		}
		case optionType_Enchant:
		{
			RValue** args = new RValue*[2];
			int gainedModsSize = static_cast<int>(levelUpData.gainedMods.size());
			RValue gainedModsArr = g_ModuleInterface->CallBuiltin("array_create", { gainedModsSize });
			for (int i = 0; i < gainedModsSize; i++)
			{
				gainedModsArr[i] = levelUpData.gainedMods[i].c_str();
			}
			args[0] = &levelUpName;
			args[1] = &gainedModsArr;
			origAddEnchantScript(playerManagerInstance, nullptr, returnVal, 2, args);
			break;
		}
		case optionType_Collab:
		{
			RValue** args = new RValue*[1];
			args[0] = &levelUpName;
			origAddCollabScript(playerManagerInstance, nullptr, returnVal, 1, args);
			break;
		}
		case optionType_SuperCollab:
		{
			RValue** args = new RValue*[1];
			args[0] = &levelUpName;
			origAddSuperCollabScript(playerManagerInstance, nullptr, returnVal, 1, args);
			break;
		}
		default:
		{
			g_ModuleInterface->Print(CM_RED, "Unhandled level up type %d for %s", levelUpData.levelUpType, levelUpName.ToString().data());
		}
	}
}

inline void getInputState(const char* inputName, size_t playerIndex, RValue& result)
{
	RValue** args = new RValue*[2];
	args[0] = new RValue(inputName);
	args[1] = new RValue(static_cast<double>(playerIndex));
	origInputCheckScript(globalInstance, nullptr, result, 2, args);
}

int receiveBytesFromPlayer(uint32_t playerID, char* outputBuffer, int length, bool loopUntilDone)
{
	if (steamIDToClientIDMap.empty())
	{
		// Assume that it's not using steam and is using LAN
		SOCKET curSocket = INVALID_SOCKET;
		if (playerID == 0)
		{
			// Assume it's receiving from the host
			curSocket = serverSocket;
		}
		else
		{
			// Assume it's receiving from a client
			curSocket = clientSocketMap[playerID];
		}
		char* curPtr = outputBuffer;
		int numBytesLeft = length;
		while (numBytesLeft > 0)
		{
			int numBytes = recv(curSocket, curPtr, numBytesLeft, 0);
			if (numBytes == 0)
			{
				return 0;
			}
			if (numBytes < 0)
			{
				int error = WSAGetLastError();
				if ((error == WSAECONNRESET || error == WSAECONNABORTED || error == WSAENOTSOCK) || (!loopUntilDone && numBytesLeft == length))
				{
					return -1;
				}
				continue;
			}
			numBytesLeft -= numBytes;
			curPtr += numBytes;
		}
	}
	else
	{
		// Assume that it's using steam
		// TODO: Should probably make it so that it treats the messages the same for steam and the regular socket code
		// TODO: For now, can probably be inefficient with the messages
		// TODO: Also probably need to change this once it starts using unreliable messages since those aren't guaranteed to stay in order
		uint64 steamPlayerID = 0;
		steamConnection* steamPlayerConnection = nullptr;
		if (playerID == 0)
		{
			// Assume it's receiving from the host
			steamPlayerID = steamLobbyBrowser->getSteamLobbyHostID().ConvertToUint64();
			steamPlayerConnection = &(steamLobbyBrowser->getSteamLobbyHostConnection());
		}
		else
		{
			// Assume it's receiving from a client
			steamPlayerID = clientIDToSteamIDMap[playerID];
			steamPlayerConnection = &(steamIDToConnectionMap[steamPlayerID]);
		}
		// Check the queued messages to see if any message is still queued
		if (steamPlayerConnection->curMessage == nullptr)
		{
			// No messages are queued, so get a message from the current connection
			// TODO: Can probably increase the amount of messages received later
			SteamNetworkingMessage_t* messageArr[1];
			int numMessages = 0;
			while (hasConnected && (numMessages = SteamNetworkingSockets()->ReceiveMessagesOnConnection(steamPlayerConnection->curConnection, messageArr, 1)) == 0 && loopUntilDone)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			if (hasConnected && numMessages > 0)
			{
				steamPlayerConnection->curMessage = messageArr[0];
				steamPlayerConnection->curBytePos = 0;
			}
			else
			{
				SteamNetConnectionRealTimeStatus_t status;
				// Check to see if the connection has been closed if no message was received
				if (SteamNetworkingSockets()->GetConnectionRealTimeStatus(steamPlayerConnection->curConnection, &status, 0, NULL) == k_EResultNoConnection)
				{
					return 0;
				}
				if (status.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer)
				{
					return 0;
				}
				return -1;
			}
		}
		// Assume that the receiveBytes won't read past the current message received
		// TODO: Should make this code not reliant on this assumption
		const char* dataBuffer = reinterpret_cast<const char*>(steamPlayerConnection->curMessage->GetData());
		if (steamPlayerConnection->curMessage->GetSize() < static_cast<uint32_t>(length + steamPlayerConnection->curBytePos))
		{
			g_ModuleInterface->Print(CM_RED, "Trying to copy message data past its bounds");
			return -1;
		}
		memcpy(outputBuffer, &dataBuffer[steamPlayerConnection->curBytePos], length);
		if (steamPlayerConnection->curMessage->GetSize() == static_cast<uint32_t>(length + steamPlayerConnection->curBytePos))
		{
			steamPlayerConnection->curMessage->Release();
			steamPlayerConnection->curMessage = nullptr;
			steamPlayerConnection->curBytePos = 0;
		}
		else
		{
			steamPlayerConnection->curBytePos += length;
		}
	}
	
	return length;
}

int sendBytesToPlayer(uint32_t playerID, char* outputBuffer, int length, networkingMessageSendType networkingMessageType, bool loopUntilDone)
{
	if (steamIDToClientIDMap.empty())
	{
		// TODO: Should use UDP for winsocks
		// Assume that it's not using steam and is using LAN
		SOCKET curSocket = INVALID_SOCKET;
		if (playerID == 0)
		{
			// Assume it's receiving from the host
			curSocket = serverSocket;
		}
		else
		{
			// Assume it's receiving from a client
			curSocket = clientSocketMap[playerID];
		}
		char* curPtr = outputBuffer;
		int numBytesLeft = length;
		while (numBytesLeft > 0)
		{
			int numBytes = send(curSocket, curPtr, numBytesLeft, 0);
			if (numBytes == 0)
			{
				return 0;
			}
			if (numBytes < 0)
			{
				int error = WSAGetLastError();
				if ((error == WSAECONNRESET || error == WSAECONNABORTED || error == WSAENOTSOCK) || (!loopUntilDone && numBytesLeft == length))
				{
					return -1;
				}
				continue;
			}
			numBytesLeft -= numBytes;
			curPtr += numBytes;
		}
	}
	else
	{
		// Assume that it's using steam
		// TODO: Should probably make it so that it treats the messages the same for steam and the regular socket code
		// TODO: For now, can probably be inefficient with the messages
		// TODO: Also probably need to change this once it starts using unreliable messages since those aren't guaranteed to stay in order
		uint64 steamPlayerID = 0;
		steamConnection steamPlayerConnection;
		if (playerID == 0)
		{
			// Assume it's sending to the host
			steamPlayerID = steamLobbyBrowser->getSteamLobbyHostID().ConvertToUint64();
			steamPlayerConnection = steamLobbyBrowser->getSteamLobbyHostConnection();
		}
		else
		{
			// Assume it's sending a client
			steamPlayerID = clientIDToSteamIDMap[playerID];
			steamPlayerConnection = steamIDToConnectionMap[steamPlayerID];
		}
		int messageSendFlags = networkingMessageType == NETWORKING_MESSAGE_USING_UDP ? k_nSteamNetworkingSend_UnreliableNoNagle : k_nSteamNetworkingSend_Reliable;
		EResult res = SteamNetworkingSockets()->SendMessageToConnection(steamPlayerConnection.curConnection, outputBuffer, length, messageSendFlags, nullptr);
		if (res != k_EResultOK)
		{
			g_ModuleInterface->Print(CM_RED, "Failed to send message");
			return -1;
		}
	}
	return length;
}

struct messageInputAim
{
	float direction;
	char isDirHeld;

	messageInputAim(char isDirHeld, float direction) : isDirHeld(isDirHeld), direction(direction)
	{
	}

	messageInputAim(char* messageBuffer)
	{
		int startBufferPos = 0;
		readByteBufferToFloat(&direction, messageBuffer, startBufferPos);
		readByteBufferToChar(&isDirHeld, messageBuffer, startBufferPos);
	}

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_INPUT_AIM, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, direction, startBufferPos);
		writeCharToByteBuffer(messageBuffer, isDirHeld, startBufferPos);
	}
};

struct messageInputNoAim
{
	float direction;
	char isDirHeld;

	messageInputNoAim(char isDirHeld, float direction) : isDirHeld(isDirHeld), direction(direction)
	{
	}

	messageInputNoAim(char* messageBuffer)
	{
		int startBufferPos = 0;
		readByteBufferToChar(&isDirHeld, messageBuffer, startBufferPos);
		readByteBufferToFloat(&direction, messageBuffer, startBufferPos);
	}

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_INPUT_NO_AIM, startBufferPos);
		writeCharToByteBuffer(messageBuffer, isDirHeld, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, direction, startBufferPos);
	}
};

struct messageInputMouseFollow
{
	float direction;
	char isDirHeld;

	messageInputMouseFollow(char isDirHeld, float direction) : isDirHeld(isDirHeld), direction(direction)
	{
	}

	messageInputMouseFollow(char* messageBuffer)
	{
		int startBufferPos = 0;
		readByteBufferToFloat(&direction, messageBuffer, startBufferPos);
		readByteBufferToChar(&isDirHeld, messageBuffer, startBufferPos);
	}

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_INPUT_MOUSEFOLLOW, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, direction, startBufferPos);
		writeCharToByteBuffer(messageBuffer, isDirHeld, startBufferPos);
	}
};

int receiveInputMessage(uint32_t playerID, MessageTypes messageType)
{
	int curMessageLen = -1;
	bool isPlayerMoving = false;
	bool isDownHeld = false;
	bool isUpHeld = false;
	bool isLeftHeld = false;
	bool isRightHeld = false;
	float direction = 0;
	switch (messageType)
	{
		case MESSAGE_INPUT_AIM:
		{
			const int inputMessageLen = sizeof(messageInputAim);
			char inputMessage[inputMessageLen];
			curMessageLen = inputMessageLen;
			int result = -1;
			if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
			{
				return result;
			}
			messageInputAim curMessage = messageInputAim(inputMessage);
			isDownHeld = curMessage.isDirHeld & 0b0001;
			isUpHeld = curMessage.isDirHeld & 0b0010;
			isLeftHeld = curMessage.isDirHeld & 0b0100;
			isRightHeld = curMessage.isDirHeld & 0b1000;
			isPlayerMoving = isDownHeld || isUpHeld || isLeftHeld || isRightHeld;
			direction = curMessage.direction;
			break;
		}
		case MESSAGE_INPUT_NO_AIM:
		{
			const int inputMessageLen = sizeof(messageInputNoAim);
			char inputMessage[inputMessageLen];
			curMessageLen = inputMessageLen;
			int result = -1;
			if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
			{
				return result;
			}
			messageInputNoAim curMessage = messageInputNoAim(inputMessage);
			isDownHeld = curMessage.isDirHeld & 0b0001;
			isUpHeld = curMessage.isDirHeld & 0b0010;
			isLeftHeld = curMessage.isDirHeld & 0b0100;
			isRightHeld = curMessage.isDirHeld & 0b1000;
			isPlayerMoving = isDownHeld || isUpHeld || isLeftHeld || isRightHeld;
			direction = curMessage.direction;
			break;
		}
		case MESSAGE_INPUT_MOUSEFOLLOW:
		{
			const int inputMessageLen = sizeof(messageInputMouseFollow);
			char inputMessage[inputMessageLen];
			curMessageLen = inputMessageLen;
			int result = -1;
			if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
			{
				return result;
			}
			messageInputMouseFollow curMessage = messageInputMouseFollow(inputMessage);
			isDownHeld = curMessage.isDirHeld & 0b0001;
			isUpHeld = curMessage.isDirHeld & 0b0010;
			isLeftHeld = curMessage.isDirHeld & 0b0100;
			isRightHeld = curMessage.isDirHeld & 0b1000;
			isPlayerMoving = isDownHeld || isUpHeld || isLeftHeld || isRightHeld;
			direction = curMessage.direction;
			break;
		}
	}

	clientMovementData inputData = clientMovementData(messageType, direction, isPlayerMoving, isDownHeld, isUpHeld, isLeftHeld, isRightHeld);
	lastTimeReceivedMoveDataMapLock.acquire();
	auto lastTimeFind = lastTimeReceivedMoveDataMap.find(playerID);
	if (lastTimeFind == lastTimeReceivedMoveDataMap.end())
	{
		lastTimeReceivedMoveDataMap[playerID] = clientMovementQueueData(timeNum, 0);
		lastTimeReceivedMoveDataMap[playerID].data.push(inputData);
	}
	else
	{
		if (lastTimeReceivedMoveDataMap[playerID].data.size() < 6)
		{
			// Only add to the queue if there are less than .1 seconds worth of messages in the queue.
			// Does mean that messages are dropped, but it's better than having the client input being forever delayed
			lastTimeReceivedMoveDataMap[playerID].data.push(inputData);
		}
	}
	lastTimeReceivedMoveDataMapLock.release();
	return curMessageLen;
}

void processInputMessage(clientMovementData data, uint32_t playerID)
{
	if (!playerMap.empty())
	{
		RValue doesPlayerExist = g_ModuleInterface->CallBuiltin("instance_exists", { playerMap[playerID] });
		if (!doesPlayerExist.ToBoolean())
		{
			return;
		}
		VariableNames spriteType = data.isPlayerMoving ? GML_runSprite : GML_idleSprite;
		setInstanceVariable(playerMap[playerID], GML_sprite_index, getInstanceVariable(playerMap[playerID], spriteType));

		double horizontalDiff = data.isRightHeld - data.isLeftHeld;
		double verticalDiff = data.isDownHeld - data.isUpHeld;

		RValue playerX = getInstanceVariable(playerMap[playerID], GML_x);
		RValue playerY = getInstanceVariable(playerMap[playerID], GML_y);
		RValue playerSPD = getInstanceVariable(playerMap[playerID], GML_SPD);
		//		if (!g_ModuleInterface->CallBuiltin("place_meeting", { playerX.ToDouble() + playerSPD.ToDouble() * horizontalDiff, playerY, objObstacleIndex }).ToBoolean())
		{
			playerX.m_Real += playerSPD.ToDouble() * horizontalDiff;
		}
		//		if (!g_ModuleInterface->CallBuiltin("place_meeting", { playerX, playerY.ToDouble() + playerSPD.ToDouble() * verticalDiff, objObstacleIndex }).ToBoolean())
		{
			playerY.m_Real += playerSPD.ToDouble() * verticalDiff;
		}
		setInstanceVariable(playerMap[playerID], GML_x, playerX);
		setInstanceVariable(playerMap[playerID], GML_y, playerY);
		setInstanceVariable(playerMap[playerID], GML_isMoving, RValue(data.isPlayerMoving));

		if (data.isPlayerMoving && data.messageType == MESSAGE_INPUT_NO_AIM)
		{
			RValue playerDirection = getInstanceVariable(playerMap[playerID], GML_direction);
			// TODO: probably should just calculate this instead of depending on GML
			RValue directionMoving = g_ModuleInterface->CallBuiltin("point_direction", { 0.0, 0.0, static_cast<double>(horizontalDiff), static_cast<double>(verticalDiff) });
			RValue directionDifference = g_ModuleInterface->CallBuiltin("angle_difference", { playerDirection, directionMoving });
			playerDirection.m_Real -= std::clamp(directionDifference.ToDouble(), -24.0, 24.0);
			setInstanceVariable(playerMap[playerID], GML_direction, playerDirection);
		}

		if (data.messageType == MESSAGE_INPUT_AIM || data.messageType == MESSAGE_INPUT_MOUSEFOLLOW)
		{
			setInstanceVariable(playerMap[playerID], GML_direction, data.direction);
		}
	}
}

void handleInputMessage(CInstance* Self)
{
	uint32_t playerID = getPlayerID(getInstanceVariable(Self, GML_id).ToInstance());
	clientMovementQueueData* curData = nullptr;
	bool hasFoundData = false;
	auto lastTimeFind = lastTimeReceivedMoveDataMap.find(playerID);
	if (lastTimeFind != lastTimeReceivedMoveDataMap.end())
	{
		curData = &(lastTimeFind->second);
		hasFoundData = true;
	}
	if (hasFoundData)
	{
		int numMessagesToProcess = timeNum - curData->lastTimeNumUpdated + curData->numEarlyUpdates;
		int numMessagesProcessed;
		for (numMessagesProcessed = 0; numMessagesProcessed < numMessagesToProcess; numMessagesProcessed++)
		{
			clientMovementData movementData;
			lastTimeReceivedMoveDataMapLock.acquire();
			if (curData->data.empty())
			{
				lastTimeReceivedMoveDataMapLock.release();
				break;
			}
			movementData = curData->data.front();
			curData->data.pop();
			lastTimeReceivedMoveDataMapLock.release();
			processInputMessage(movementData, playerID);
		}
		curData->lastTimeNumUpdated = timeNum;
		curData->numEarlyUpdates = numMessagesToProcess - numMessagesProcessed;
	}
}

int receiveRoomMessage(uint32_t playerID)
{
	const int inputMessageLen = sizeof(messageRoom);
	char inputMessage[inputMessageLen];
	int curMessageLen = inputMessageLen;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
	{
		return result;
	}
	roomMessageLock.acquire();
	roomMessage = messageRoom(inputMessage);
	roomMessageLock.release();
	hasReceivedRoomMessage = true;
	return curMessageLen;
}

void handleRoomMessage()
{
	// Handle receive room
	if (hasReceivedRoomMessage)
	{
		roomMessageLock.acquire();
		g_ModuleInterface->CallBuiltin("room_goto", { roomMessage.roomNum });
		g_ModuleInterface->CallBuiltin("variable_global_set", { "gameMode", roomMessage.gameMode });
		std::shared_ptr<menuGridData> nullptrMenu = nullptr;
		holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, nullptrMenu);
		steamLobbyBrowser->leaveLobby();
		hasReceivedRoomMessage = false;
		roomMessageLock.release();
	}
}

int receiveInstanceCreateMessage(uint32_t playerID)
{
	const int inputMessageLen = sizeof(messageInstancesCreate);
	char inputMessage[inputMessageLen];
	int curMessageLen = inputMessageLen;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
	{
		return result;
	}
	createInstancesMessageQueueLock.acquire();
	createInstancesMessageQueue.push(messageInstancesCreate(inputMessage));
	createInstancesMessageQueueLock.release();
	return curMessageLen;
}

void handleInstanceCreateMessage()
{
	// Handle receive create Instances
	do
	{
		createInstancesMessageQueueLock.acquire();
		if (createInstancesMessageQueue.empty())
		{
			createInstancesMessageQueueLock.release();
			break;
		}
		messageInstancesCreate curInstances = createInstancesMessageQueue.front();
		createInstancesMessageQueue.pop();
		createInstancesMessageQueueLock.release();

		// temp fix to this not being set
		g_ModuleInterface->CallBuiltin("variable_global_set", { "reflection", false });
		for (int i = 0; i < curInstances.numInstances; i++)
		{
			instanceData curData = curInstances.data[i];
			RValue createdInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { curData.xPos, curData.yPos, -curData.yPos, objBaseMobIndex });
			setInstanceVariable(createdInstance, GML_image_xscale, RValue(curData.imageXScale));
			setInstanceVariable(createdInstance, GML_image_yscale, RValue(curData.imageYScale));
			setInstanceVariable(createdInstance, GML_sprite_index, RValue(curData.spriteIndex));
			setInstanceVariable(createdInstance, GML_completeStop, RValue(true));
			setInstanceVariable(createdInstance, GML_image_speed, RValue(0));
			instanceArr[curData.instanceID] = createdInstance;
		}

	} while (true);
}

int receiveInstanceUpdateMessage(uint32_t playerID)
{
	messageInstancesUpdate curInstances = messageInstancesUpdate();
	int curMessageLen = curInstances.receiveMessage(playerID);
	updateInstancesMessageQueueLock.acquire();
	updateInstancesMessageQueue.push(curInstances);
	updateInstancesMessageQueueLock.release();
	return curMessageLen;
}

void handleInstanceUpdateMessage()
{
	// Handle receive update Instances
	do
	{
		updateInstancesMessageQueueLock.acquire();
		if (updateInstancesMessageQueue.empty())
		{
			updateInstancesMessageQueueLock.release();
			break;
		}
		messageInstancesUpdate curInstances = updateInstancesMessageQueue.front();
		updateInstancesMessageQueue.pop();
		updateInstancesMessageQueueLock.release();

		for (int i = 0; i < curInstances.numInstances; i++)
		{
			instanceData curData = curInstances.data[i];
			if (instanceArr[curData.instanceID].m_Kind == VALUE_UNDEFINED)
			{
				continue;
			}
			RValue instance = instanceArr[curData.instanceID];
			if (checkBitInByte(curData.hasVarChanged, 0))
			{
				if (checkBitInByte(curData.hasVarChanged, 1))
				{
					setInstanceVariable(instance, GML_x, RValue(curData.xPos));
					setInstanceVariable(instance, GML_y, RValue(curData.yPos));
				}
				else
				{
					double xPos = getInstanceVariable(instance, GML_x).ToDouble();
					double yPos = getInstanceVariable(instance, GML_y).ToDouble();
					xPos += curData.xPosDiff / 10.0;
					yPos += curData.yPosDiff / 10.0;
					setInstanceVariable(instance, GML_x, RValue(xPos));
					setInstanceVariable(instance, GML_y, RValue(yPos));
				}
			}

			if (checkBitInByte(curData.hasVarChanged, 3))
			{
				setInstanceVariable(instance, GML_image_xscale, RValue(curData.imageXScale));
				setInstanceVariable(instance, GML_image_yscale, RValue(curData.imageYScale));
			}
			
			if (checkBitInByte(curData.hasVarChanged, 4))
			{
				setInstanceVariable(instance, GML_sprite_index, RValue(curData.spriteIndex));
			}
			setInstanceVariable(instance, GML_image_index, RValue(curData.truncatedImageIndex));
		}

	} while (true);
}

int receiveInstanceDeleteMessage(uint32_t playerID)
{
	const int inputMessageLen = sizeof(messageInstancesDelete);
	char inputMessage[inputMessageLen];
	int curMessageLen = inputMessageLen;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
	{
		return result;
	}
	messageInstancesDelete curInstances = messageInstancesDelete(inputMessage);
	deleteInstancesMessageQueueLock.acquire();
	deleteInstancesMessageQueue.push(curInstances);
	deleteInstancesMessageQueueLock.release();
	return curMessageLen;
}

void handleInstanceDeleteMessage()
{
	// Handle receive delete Instances
	do
	{
		deleteInstancesMessageQueueLock.acquire();
		if (deleteInstancesMessageQueue.empty())
		{
			deleteInstancesMessageQueueLock.release();
			break;
		}
		messageInstancesDelete curInstances = deleteInstancesMessageQueue.front();
		deleteInstancesMessageQueue.pop();
		deleteInstancesMessageQueueLock.release();

		for (int i = 0; i < curInstances.numInstances; i++)
		{
			int instanceID = curInstances.instanceIDArr[i];
			if (instanceArr[instanceID].m_Kind == VALUE_UNDEFINED)
			{
				continue;
			}
			// TODO: Could probably cache the instance or deactivate it instead of destroying it
			g_ModuleInterface->CallBuiltin("instance_destroy", { instanceArr[instanceID] });
			instanceArr[instanceID] = RValue();
		}

	} while (true);
}

float clientCamPosX = 0;
float clientCamPosY = 0;
std::unordered_map<uint32_t, bool> isPlayerCreatedMap;

int receiveClientPlayerDataMessage(uint32_t playerID)
{
	const int inputMessageLen = sizeof(messageClientPlayerData);
	char inputMessage[inputMessageLen];
	int curMessageLen = inputMessageLen;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
	{
		return result;
	}
	messageClientPlayerData clientPlayer = messageClientPlayerData(inputMessage);
	clientPlayerDataMessageQueueLock.acquire();
	clientPlayerDataMessageQueue.push(clientPlayer);
	clientPlayerDataMessageQueueLock.release();

	return curMessageLen;
}

void handleClientPlayerDataMessage()
{
	// Handle receive client player data message
	do
	{
		clientPlayerDataMessageQueueLock.acquire();
		if (clientPlayerDataMessageQueue.empty())
		{
			clientPlayerDataMessageQueueLock.release();
			break;
		}
		messageClientPlayerData clientPlayer = clientPlayerDataMessageQueue.front();
		clientPlayerDataMessageQueue.pop();
		clientPlayerDataMessageQueueLock.release();

		playerData clientPlayerData = clientPlayer.data;
		uint32_t playerID = clientPlayerData.m_playerID;
		memcpy(&playerDataMap[playerID], &clientPlayerData, sizeof(playerData));

		if (playerID == clientID)
		{
			clientCamPosX = clientPlayerData.xPos;
			clientCamPosY = clientPlayerData.yPos;
		}

		if (isPlayerCreatedMap[playerID])
		{
			RValue& instance = playerMap[playerID];
			setInstanceVariable(instance, GML_x, RValue(clientPlayerData.xPos));
			setInstanceVariable(instance, GML_y, RValue(clientPlayerData.yPos));
			setInstanceVariable(instance, GML_image_xscale, RValue(clientPlayerData.imageXScale));
			setInstanceVariable(instance, GML_image_yscale, RValue(clientPlayerData.imageYScale));
			setInstanceVariable(instance, GML_direction, RValue(clientPlayerData.direction));
			setInstanceVariable(instance, GML_sprite_index, RValue(clientPlayerData.spriteIndex));
			setInstanceVariable(instance, GML_image_index, RValue(clientPlayerData.truncatedImageIndex));
			setInstanceVariable(instance, GML_currentHP, RValue(static_cast<double>(clientPlayerData.curHP)));
			setInstanceVariable(instance, GML_HP, RValue(static_cast<double>(clientPlayerData.maxHP)));
			setInstanceVariable(instance, GML_ATK, RValue(clientPlayerData.curAttack));
			setInstanceVariable(instance, GML_SPD, RValue(clientPlayerData.curSpeed));
			setInstanceVariable(instance, GML_crit, RValue(clientPlayerData.curCrit));
			setInstanceVariable(instance, GML_haste, RValue(clientPlayerData.curHaste));
			setInstanceVariable(instance, GML_pickupRange, RValue(clientPlayerData.curPickupRange));
			setInstanceVariable(instance, GML_specialMeter, RValue(clientPlayerData.specialMeter));
			setInstanceVariable(instance, GML_food, RValue(foodMultiplier));
			// Need to set the playersnapshot for some stats to make sure it displays in the pause menu
			// Seems like it is possible for this to still be nullptr if the player manager step doesn't run early enough
			if (playerManagerInstanceVar != nullptr)
			{
				RValue playerSnapshot = getInstanceVariable(playerManagerInstanceVar, GML_playerSnapshot);
				setInstanceVariable(playerSnapshot, GML_ATK, RValue(clientPlayerData.curAttack));
				setInstanceVariable(playerSnapshot, GML_SPD, RValue(clientPlayerData.curSpeed));
				setInstanceVariable(playerSnapshot, GML_crit, RValue(clientPlayerData.curCrit));
				setInstanceVariable(playerSnapshot, GML_haste, RValue(clientPlayerData.curHaste));
				setInstanceVariable(playerSnapshot, GML_pickupRange, RValue(clientPlayerData.curPickupRange));
				setInstanceVariable(playerSnapshot, GML_moneyGain, RValue(moneyGainMultiplier));
			}
		}

	} while (true);
}

int receiveAttackCreateMessage(uint32_t playerID)
{
	const int inputMessageLen = sizeof(messageAttackCreate);
	char inputMessage[inputMessageLen];
	int curMessageLen = inputMessageLen;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
	{
		return result;
	}
	messageAttackCreate curAttacks = messageAttackCreate(inputMessage);
	createAttackMessageQueueLock.acquire();
	createAttackMessageQueue.push(curAttacks);
	createAttackMessageQueueLock.release();

	return curMessageLen;
}

void handleAttackCreateMessage()
{
	do
	{
		createAttackMessageQueueLock.acquire();
		if (createAttackMessageQueue.empty())
		{
			createAttackMessageQueueLock.release();
			break;
		}
		messageAttackCreate curAttacks = createAttackMessageQueue.front();
		createAttackMessageQueue.pop();
		createAttackMessageQueueLock.release();

		for (int i = 0; i < curAttacks.numAttacks; i++)
		{
			attackData curData = curAttacks.data[i];
			auto attackFind = attackMap.find(curData.instanceID);
			// If the instance still exists, assume that it's the hacky solution to update sprite index
			// TODO: There is a potential issue where if an ID is reused to create before the instance is deleted, it could possibly mess up some stuff.
			if (attackFind == attackMap.end())
			{
				RValue createdInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { curData.xPos, curData.yPos, -curData.yPos, objAttackIndex });
				setInstanceVariable(createdInstance, GML_image_xscale, RValue(curData.imageXScale));
				setInstanceVariable(createdInstance, GML_image_yscale, RValue(curData.imageYScale));
				setInstanceVariable(createdInstance, GML_image_angle, RValue(curData.imageAngle));
				setInstanceVariable(createdInstance, GML_image_alpha, RValue(curData.imageAlpha));
				setInstanceVariable(createdInstance, GML_sprite_index, RValue(curData.spriteIndex));
				setInstanceVariable(createdInstance, GML_customDrawScriptAbove, RValue(false));
				setInstanceVariable(createdInstance, GML_customDrawScriptBelow, RValue(false));
				setInstanceVariable(createdInstance, GML_transparent, RValue(false));
				setInstanceVariable(createdInstance, GML_isEnemy, RValue(true));
				setInstanceVariable(createdInstance, GML_spriteColor, RValue(0xFFFFFF));
				setInstanceVariable(createdInstance, GML_duration, RValue(0));
				setInstanceVariable(createdInstance, GML_image_speed, RValue(0));
				attackMap[curData.instanceID] = createdInstance;
			}
			else
			{
				RValue createdInstance = attackFind->second;
				setInstanceVariable(createdInstance, GML_x, RValue(curData.xPos));
				setInstanceVariable(createdInstance, GML_y, RValue(curData.yPos));
				setInstanceVariable(createdInstance, GML_image_xscale, RValue(curData.imageXScale));
				setInstanceVariable(createdInstance, GML_image_yscale, RValue(curData.imageYScale));
				setInstanceVariable(createdInstance, GML_image_angle, RValue(curData.imageAngle));
				setInstanceVariable(createdInstance, GML_image_alpha, RValue(curData.imageAlpha));
				setInstanceVariable(createdInstance, GML_sprite_index, RValue(curData.spriteIndex));
				setInstanceVariable(createdInstance, GML_image_index, RValue(curData.truncatedImageIndex));
			}
		}
	} while (true);
}

int receiveAttackUpdateMessage(uint32_t playerID)
{
	messageAttackUpdate curAttacks = messageAttackUpdate();
	int curMessageLen = curAttacks.receiveMessage(playerID);
	updateAttackMessageQueueLock.acquire();
	updateAttackMessageQueue.push(curAttacks);
	updateAttackMessageQueueLock.release();
	return curMessageLen;
}

void handleAttackUpdateMessage()
{
	do
	{
		updateAttackMessageQueueLock.acquire();
		if (updateAttackMessageQueue.empty())
		{
			updateAttackMessageQueueLock.release();
			break;
		}
		messageAttackUpdate curAttacks = updateAttackMessageQueue.front();
		updateAttackMessageQueue.pop();
		updateAttackMessageQueueLock.release();

		for (int i = 0; i < curAttacks.numAttacks; i++)
		{
			attackData curData = curAttacks.data[i];
			auto attackFind = attackMap.find(curData.instanceID);
			// It might be possible that the update message would be sent before the create message. Just ignore the message if that does happen for now
			if (attackFind != attackMap.end())
			{
				RValue instance = attackFind->second;
				if (checkBitInByte(curData.hasVarChanged, 0))
				{
					if (checkBitInByte(curData.hasVarChanged, 1))
					{
						setInstanceVariable(instance, GML_x, RValue(curData.xPos));
						setInstanceVariable(instance, GML_y, RValue(curData.yPos));
					}
					else
					{
						double xPos = getInstanceVariable(instance, GML_x).ToDouble();
						double yPos = getInstanceVariable(instance, GML_y).ToDouble();
						xPos += curData.xPosDiff / 10.0;
						yPos += curData.yPosDiff / 10.0;
						setInstanceVariable(instance, GML_x, RValue(xPos));
						setInstanceVariable(instance, GML_y, RValue(yPos));
					}
				}
				if (checkBitInByte(curData.hasVarChanged, 3))
				{
					setInstanceVariable(instance, GML_image_angle, RValue(curData.imageAngle));
				}
				if (checkBitInByte(curData.hasVarChanged, 4))
				{
					setInstanceVariable(instance, GML_image_alpha, RValue(curData.imageAlpha));
				}
				if (checkBitInByte(curData.hasVarChanged, 5))
				{
					setInstanceVariable(instance, GML_image_xscale, RValue(curData.imageXScale));
					setInstanceVariable(instance, GML_image_yscale, RValue(curData.imageYScale));
				}
				if (checkBitInByte(curData.hasVarChanged, 6))
				{
					setInstanceVariable(instance, GML_sprite_index, RValue(curData.spriteIndex));
				}
				setInstanceVariable(instance, GML_image_index, RValue(curData.truncatedImageIndex));
			}
		}
	} while (true);
}

int receiveAttackDeleteMessage(uint32_t playerID)
{
	const int inputMessageLen = sizeof(messageAttackDelete);
	char inputMessage[inputMessageLen];
	int curMessageLen = inputMessageLen;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
	{
		return result;
	}
	messageAttackDelete curAttack = messageAttackDelete(inputMessage);
	deleteAttackMessageQueueLock.acquire();
	deleteAttackMessageQueue.push(curAttack);
	deleteAttackMessageQueueLock.release();
	return curMessageLen;
}

void handleAttackDeleteMessage()
{
	do
	{
		deleteAttackMessageQueueLock.acquire();
		if (deleteAttackMessageQueue.empty())
		{
			deleteAttackMessageQueueLock.release();
			break;
		}
		messageAttackDelete curAttack = deleteAttackMessageQueue.front();
		deleteAttackMessageQueue.pop();
		deleteAttackMessageQueueLock.release();

		for (int i = 0; i < curAttack.numAttacks; i++)
		{
			int instanceID = curAttack.attackIDArr[i];
			auto attackFind = attackMap.find(instanceID);
			// TODO: Could probably cache the instance or deactivate it instead of destroying it
			// Seems like it's possible that the delete message would be sent before the create message. Just ignore the message if that does happen for now
			if (attackFind != attackMap.end())
			{
				g_ModuleInterface->CallBuiltin("instance_destroy", { attackFind->second });
			}
			attackMap.erase(instanceID);
		}
	} while (true);
}

bool hasObtainedClientID = false;
int clientID = 0;

int receiveClientIDMessage(uint32_t playerID)
{
	const int inputMessageLen = sizeof(messageClientID);
	char inputMessage[inputMessageLen];
	int curMessageLen = inputMessageLen;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
	{
		return result;
	}
	messageClientID clientNumber = messageClientID(inputMessage);
	clientIDMessageQueueLock.acquire();
	clientIDMessageQueue.push(clientNumber);
	clientIDMessageQueueLock.release();
	return curMessageLen;
}

void handleClientIDMessage()
{
	do
	{
		clientIDMessageQueueLock.acquire();
		if (clientIDMessageQueue.empty())
		{
			clientIDMessageQueueLock.release();
			break;
		}
		messageClientID clientNumber = clientIDMessageQueue.front();
		clientIDMessageQueue.pop();
		clientIDMessageQueueLock.release();

		clientID = clientNumber.clientID;
		callbackManagerInterfacePtr->LogToFile(MODNAME, "Received client id %d", clientID);
		curPlayerID = clientID;
		hasObtainedClientID = true;
		lobbyPlayerDataMap[clientID] = lobbyPlayerData();
		// TODO: Let the client decide their own name eventually
		lobbyPlayerDataMap[clientID].playerName = std::move(std::to_string(clientID));
		printf("clientID: %d\n", clientID);
	} while (true);
}

int receivePickupableCreateMessage(uint32_t playerID)
{
	const int inputMessageLen = sizeof(messagePickupableCreate);
	char inputMessage[inputMessageLen];
	int curMessageLen = inputMessageLen;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
	{
		return result;
	}
	messagePickupableCreate curPickupable = messagePickupableCreate(inputMessage);
	createPickupableMessageQueueLock.acquire();
	createPickupableMessageQueue.push(curPickupable);
	createPickupableMessageQueueLock.release();
	return curMessageLen;
}

void handlePickupableCreateMessage()
{
	do
	{
		createPickupableMessageQueueLock.acquire();
		if (createPickupableMessageQueue.empty())
		{
			createPickupableMessageQueueLock.release();
			break;
		}
		messagePickupableCreate curPickupable = createPickupableMessageQueue.front();
		createPickupableMessageQueue.pop();
		createPickupableMessageQueueLock.release();

		pickupableData curData = curPickupable.data;
		RValue createdInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { curData.xPos, curData.yPos, -curData.yPos, objPickupableIndex });
		setInstanceVariable(createdInstance, GML_sprite_index, RValue(curData.spriteIndex));
		setInstanceVariable(createdInstance, GML_mask_index, RValue(sprEmptyMaskIndex));
		setInstanceVariable(createdInstance, GML_image_speed, RValue(0));
		pickupableArr[curData.pickupableID] = createdInstance;
		setInstanceVariable(createdInstance, GML_createdTime, RValue(static_cast<int>(timeNum)));
		RValue spritePlaybackSpeed = g_ModuleInterface->CallBuiltin("sprite_get_speed", { curData.spriteIndex });
		setInstanceVariable(createdInstance, GML_spritePlaybackSpeed, spritePlaybackSpeed);
	} while (true);
}

int receivePickupableUpdateMessage(uint32_t playerID)
{
	const int inputMessageLen = sizeof(messagePickupableUpdate);
	char inputMessage[inputMessageLen];
	int curMessageLen = inputMessageLen;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
	{
		return result;
	}
	messagePickupableUpdate curPickupable = messagePickupableUpdate(inputMessage);
	updatePickupableMessageQueueLock.acquire();
	updatePickupableMessageQueue.push(curPickupable);
	updatePickupableMessageQueueLock.release();
	return curMessageLen;
}

void handlePickupableUpdateMessage()
{
	do
	{
		updatePickupableMessageQueueLock.acquire();
		if (updatePickupableMessageQueue.empty())
		{
			updatePickupableMessageQueueLock.release();
			break;
		}
		messagePickupableUpdate curPickupable = updatePickupableMessageQueue.front();
		updatePickupableMessageQueue.pop();
		updatePickupableMessageQueueLock.release();

		pickupableData curData = curPickupable.data;
		if (pickupableArr[curData.pickupableID].m_Kind == VALUE_UNDEFINED)
		{
			continue;
		}
		RValue instance = pickupableArr[curData.pickupableID];
		setInstanceVariable(instance, GML_x, RValue(curData.xPos));
		setInstanceVariable(instance, GML_y, RValue(curData.yPos));
		setInstanceVariable(instance, GML_sprite_index, RValue(curData.spriteIndex));
		setInstanceVariable(instance, GML_image_index, RValue(curData.truncatedImageIndex));
	} while (true);
}

int receivePickupableDeleteMessage(uint32_t playerID)
{
	const int inputMessageLen = sizeof(messagePickupableDelete);
	char inputMessage[inputMessageLen];
	int curMessageLen = inputMessageLen;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
	{
		return result;
	}
	messagePickupableDelete curPickupable = messagePickupableDelete(inputMessage);
	deletePickupableMessageQueueLock.acquire();
	deletePickupableMessageQueue.push(curPickupable);
	deletePickupableMessageQueueLock.release();
	return curMessageLen;
}

void handlePickupableDeleteMessage()
{
	do
	{
		deletePickupableMessageQueueLock.acquire();
		if (deletePickupableMessageQueue.empty())
		{
			deletePickupableMessageQueueLock.release();
			break;
		}
		messagePickupableDelete curPickupable = deletePickupableMessageQueue.front();
		deletePickupableMessageQueue.pop();
		deletePickupableMessageQueueLock.release();

		int instanceID = curPickupable.pickupableID;
		// TODO: Could probably cache the instance or deactivate it instead of destroying it
		if (pickupableArr[instanceID].m_Kind == VALUE_UNDEFINED)
		{
			continue;
		}
		g_ModuleInterface->CallBuiltin("instance_destroy", { pickupableArr[instanceID] });
		pickupableArr[instanceID] = RValue();
	} while (true);
}

uint32_t timeNum = 0;

int receiveGameDataMessage(uint32_t playerID)
{
	const int inputMessageLen = sizeof(messageGameData);
	char inputMessage[inputMessageLen];
	int curMessageLen = inputMessageLen;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, inputMessage, inputMessageLen)) <= 0)
	{
		return result;
	}
	messageGameData curMessage = messageGameData(inputMessage);
	gameDataMessageQueueLock.acquire();
	gameDataMessageQueue.push(curMessage);
	gameDataMessageQueueLock.release();
	return curMessageLen;
}

void handleGameDataMessage()
{
	do
	{
		gameDataMessageQueueLock.acquire();
		if (gameDataMessageQueue.empty())
		{
			gameDataMessageQueueLock.release();
			break;
		}
		messageGameData curMessage = gameDataMessageQueue.front();
		gameDataMessageQueue.pop();
		gameDataMessageQueueLock.release();

		g_ModuleInterface->CallBuiltin("variable_global_set", { "currentRunMoneyGained", static_cast<double>(curMessage.coinCount) });
		g_ModuleInterface->CallBuiltin("variable_global_set", { "enemyDefeated", static_cast<double>(curMessage.enemyDefeated) });
		g_ModuleInterface->CallBuiltin("variable_global_set", { "PLAYERLEVEL", static_cast<double>(curMessage.playerLevel) });
		g_ModuleInterface->CallBuiltin("variable_global_set", { "experience", static_cast<double>(curMessage.experience) });
		g_ModuleInterface->CallBuiltin("variable_global_set", { "goldenHammer", static_cast<double>(curMessage.goldenHammer) });
		g_ModuleInterface->CallBuiltin("variable_global_set", { "goldenHammerPieces", static_cast<double>(curMessage.goldenHammerPieces) });
		setInstanceVariable(playerManagerInstanceVar, GML_toNextLevel, RValue(curMessage.toNextLevel));
		RValue playerSnapshot = getInstanceVariable(playerManagerInstanceVar, GML_playerSnapshot);
		// TODO: Maybe change this in the future to not just overwrite the client data with the host data?
		moneyGainMultiplier = curMessage.moneyGain;
		foodMultiplier = curMessage.food;

		timeNum = curMessage.frameNum;
		int numFrames = timeNum % 60;
		timeNum /= 60;
		int numSeconds = timeNum % 60;
		timeNum /= 60;
		int numMinutes = timeNum % 60;
		timeNum /= 60;
		int numHours = timeNum;
		RValue timeArr = g_ModuleInterface->CallBuiltin("variable_global_get", { "time" });
		if (timeArr.m_Kind == VALUE_ARRAY)
		{
			timeArr[3] = numFrames;
			timeArr[2] = numSeconds;
			timeArr[1] = numMinutes;
			timeArr[0] = numHours;
		}
		timeNum = curMessage.frameNum;
	} while (true);
}

int receiveLevelUpOptionsMessage(uint32_t playerID)
{
	messageLevelUpOptions curMessage = messageLevelUpOptions();
	int curMessageLen = curMessage.receiveMessage(playerID);
	levelUpOptionsMessageQueueLock.acquire();
	levelUpOptionsMessageQueue.push(curMessage);
	levelUpOptionsMessageQueueLock.release();
	return curMessageLen;
}

void handleLevelUpOptionsMessage(CInstance* playerManager)
{
	do
	{
		levelUpOptionsMessageQueueLock.acquire();
		if (levelUpOptionsMessageQueue.empty())
		{
			levelUpOptionsMessageQueueLock.release();
			break;
		}
		messageLevelUpOptions curMessage = levelUpOptionsMessageQueue.front();
		levelUpOptionsMessageQueue.pop();
		levelUpOptionsMessageQueueLock.release();

		setInstanceVariable(playerManager, GML_paused, RValue(true));
		setInstanceVariable(playerManager, GML_leveled, RValue(true));
		setInstanceVariable(playerManager, GML_controlsFree, RValue(true));
		setInstanceVariable(playerMap[clientID], GML_canControl, RValue(false));
		isClientPaused = true;
		RValue options = getInstanceVariable(playerManager, GML_options);
		for (int i = 0; i < 4; i++)
		{
			levelUpOption curOption = curMessage.optionArr[i];
			setInstanceVariable(options[i], GML_optionIcon, RValue(curOption.optionIcon));
			setInstanceVariable(options[i], GML_optionType, RValue(curOption.optionType));
			setInstanceVariable(options[i], GML_optionName, RValue(curOption.optionName));
			setInstanceVariable(options[i], GML_optionID, RValue(curOption.optionID));
			if (curOption.optionDescription.size() == 1)
			{
				setInstanceVariable(options[i], GML_optionDescription, RValue(curOption.optionDescription[0]));
			}
			else
			{
				int optionDescriptionSize = static_cast<int>(curOption.optionDescription.size());
				RValue optionDescriptionArr = g_ModuleInterface->CallBuiltin("array_create", { optionDescriptionSize });
				for (int i = 0; i < optionDescriptionSize; i++)
				{
					optionDescriptionArr[i] = curOption.optionDescription[i].c_str();
				}
				setInstanceVariable(options[i], GML_optionDescription, optionDescriptionArr);
			}
			if (!curOption.modsList.empty())
			{
				if (curOption.isModsListNew == 0)
				{
					int modsListSize = static_cast<int>(curOption.modsList.size());
					RValue gainedModsArr = g_ModuleInterface->CallBuiltin("array_create", { modsListSize });
					for (int i = 0; i < modsListSize; i++)
					{
						gainedModsArr[i] = curOption.modsList[i].c_str();
					}
					setInstanceVariable(options[i], GML_gainedMods, gainedModsArr);
				}
				else if (curOption.isModsListNew == 1)
				{
					setInstanceVariable(options[i], GML_offeredMod, curOption.modsList[0].c_str());
				}
			}
			else
			{
				// Clears out any previous enchants
				setInstanceVariable(options[i], GML_gainedMods, RValue(-1));
				setInstanceVariable(options[i], GML_offeredMod, RValue(-1));
			}
			// TODO: Add mods option as well
			if (curOption.optionType.compare("Weapon") == 0)
			{
				switch (curOption.weaponAndItemType)
				{
				case 0:
				{
					setInstanceVariable(options[i], GML_weaponType, RValue("Melee"));
					break;
				}
				case 1:
				{
					setInstanceVariable(options[i], GML_weaponType, RValue("Ranged"));
					break;
				}
				case 2:
				{
					setInstanceVariable(options[i], GML_weaponType, RValue("MultiShot"));
					break;
				}
				}
			}
			else if (curOption.optionType.compare("Item") == 0)
			{
				switch (curOption.weaponAndItemType)
				{
				case 0:
				{
					setInstanceVariable(options[i], GML_itemType, RValue("Healing"));
					break;
				}
				case 1:
				{
					setInstanceVariable(options[i], GML_itemType, RValue("Stat"));
					break;
				}
				case 2:
				{
					setInstanceVariable(options[i], GML_itemType, RValue("Utility"));
					break;
				}
				}
			}
		}
	} while (true);
}

std::vector<levelUpPausedData> levelUpPausedList;

int receiveLevelUpClientChoiceMessage(uint32_t playerID)
{
	messageLevelUpClientChoice curMessage = messageLevelUpClientChoice();
	int curMessageLen = curMessage.receiveMessage(playerID);
	curMessage.m_playerID = playerID;
	levelUpClientChoiceMessageQueueLock.acquire();
	levelUpClientChoiceMessageQueue.push(curMessage);
	levelUpClientChoiceMessageQueueLock.release();

	return curMessageLen;
}

void handleLevelUpClientChoiceMessage()
{
	do
	{
		levelUpClientChoiceMessageQueueLock.acquire();
		if (levelUpClientChoiceMessageQueue.empty())
		{
			levelUpClientChoiceMessageQueueLock.release();
			break;
		}
		messageLevelUpClientChoice curMessage = levelUpClientChoiceMessageQueue.front();
		levelUpClientChoiceMessageQueue.pop();
		levelUpClientChoiceMessageQueueLock.release();

		int levelUpOption = curMessage.levelUpOption;
		uint32_t playerID = curMessage.m_playerID;

		// Chose an upgrade option
		if (levelUpOption < 4)
		{
			RValue playerManager = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerManagerIndex, 0 });
			RValue options = getInstanceVariable(playerManager, GML_options);
			levelUpPausedList.push_back(levelUpPausedData(playerID, levelUpOptionNamesMap[playerID].optionArr[levelUpOption].first, levelUpOptionNamesMap[playerID].optionArr[levelUpOption].second.c_str()));
			clientUnpausedMap[playerID] = true;
			if (isHostWaitingForClientUnpause)
			{
				// TODO: Can probably just count the number of client players left paused. Not too important though
				bool isAnyClientPaused = false;
				for (auto& clientUnpausedData : clientUnpausedMap)
				{
					bool isClientUnpaused = clientUnpausedData.second;
					if (!isClientUnpaused)
					{
						isAnyClientPaused = true;
						break;
					}
				}
				if (!isAnyClientPaused)
				{
					isHostWaitingForClientUnpause = false;
					unpauseHost();
				}
			}
		}
		else if (levelUpOption == 4) // Chose reroll
		{
			RValue result;
			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			RValue options = getInstanceVariable(playerManagerInstanceVar, GML_options);
			RValue prevOptions[4];
			for (int i = 0; i < 4; i++)
			{
				prevOptions[i] = options[i];
			}
			swapPlayerDataPush(playerManagerInstanceVar, attackController, playerID);

			// Add to reroll container to decrease the chance of skipped upgrades
			for (int i = 0; i < 4; i++)
			{
				optionType levelUpType = levelUpOptionNamesMap[playerID].optionArr[i].first;
				if (levelUpType == optionType_Weapon || levelUpType == optionType_Item || levelUpType == optionType_Skill)
				{
					RValue rerollContainer = getInstanceVariable(playerManagerInstanceVar, GML_rerollContainer);
					RValue rerollContainerOptionCount = g_ModuleInterface->CallBuiltin("variable_instance_get", { rerollContainer, levelUpOptionNamesMap[playerID].optionArr[i].second.c_str() });
					if (rerollContainerOptionCount.m_Kind == VALUE_UNDEFINED || rerollContainerOptionCount.m_Kind == VALUE_UNSET)
					{
						// For some reason, this doesn't work properly if an int is passed in and needs a real number instead. Probably something weird with GML
						g_ModuleInterface->CallBuiltin("variable_instance_set", { rerollContainer, levelUpOptionNamesMap[playerID].optionArr[i].second.c_str(), 1.0});
					}
					else
					{
						g_ModuleInterface->CallBuiltin("variable_instance_set", { rerollContainer, levelUpOptionNamesMap[playerID].optionArr[i].second.c_str(), rerollContainerOptionCount.ToDouble() + 1});
					}
				}
			}

			origGeneratePossibleOptionsScript(playerManagerInstanceVar, nullptr, result, 0, nullptr);
			origOptionOneScript(playerManagerInstanceVar, nullptr, result, 0, nullptr);
			options[0] = result;
			origOptionTwoScript(playerManagerInstanceVar, nullptr, result, 0, nullptr);
			options[1] = result;
			origOptionThreeScript(playerManagerInstanceVar, nullptr, result, 0, nullptr);
			options[2] = result;
			origOptionFourScript(playerManagerInstanceVar, nullptr, result, 0, nullptr);
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

			for (int i = 0; i < 4; i++)
			{
				options[i] = prevOptions[i];
			}
			swapPlayerDataPop(playerManagerInstanceVar, attackController);
		}
	} while (true);
}

std::chrono::high_resolution_clock::time_point startPingTime;

int receivePing(uint32_t playerID)
{
	return sendPong(playerID);
}

int receivePong(uint32_t playerID)
{
	playerPingMap[playerID] = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startPingTime).count());
	return 1;
}

std::unordered_map<short, destructableData> destructableMap;

int receiveDestructableCreateMessage(uint32_t playerID)
{
	messageDestructableCreate curMessage = messageDestructableCreate();
	int curMessageLen = curMessage.receiveMessage(playerID);

	destructableCreateMessageQueueLock.acquire();
	destructableCreateMessageQueue.push(curMessage);
	destructableCreateMessageQueueLock.release();

	return curMessageLen;
}

void handleDestructableCreateMessage()
{
	do
	{
		destructableCreateMessageQueueLock.acquire();
		if (destructableCreateMessageQueue.empty())
		{
			destructableCreateMessageQueueLock.release();
			break;
		}
		messageDestructableCreate curMessage = destructableCreateMessageQueue.front();
		destructableCreateMessageQueue.pop();
		destructableCreateMessageQueueLock.release();

		destructableData curData = curMessage.data;
		RValue obstacleInstance = RValue();
		if (curMessage.data.pillarType == 0)
		{
			obstacleInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { curData.xPos, curData.yPos, 12345, objDestructableIndex });
		}
		else if (curMessage.data.pillarType == 1)
		{
			obstacleInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { curData.xPos, curData.yPos, 12345, objYagooPillarIndex });
		}
		curData.destructableInstance = obstacleInstance;
		destructableMap[curData.id] = curData;
	} while (true);
}

int receiveDestructableBreakMessage(uint32_t playerID)
{
	messageDestructableBreak curMessage = messageDestructableBreak();
	int curMessageLen = curMessage.receiveMessage(playerID);

	destructableBreakMessageQueueLock.acquire();
	destructableBreakMessageQueue.push(curMessage);
	destructableBreakMessageQueueLock.release();

	return curMessageLen;
}

void handleDestructableBreakMessage()
{
	do
	{
		destructableBreakMessageQueueLock.acquire();
		if (destructableBreakMessageQueue.empty())
		{
			destructableBreakMessageQueueLock.release();
			break;
		}
		messageDestructableBreak curMessage = destructableBreakMessageQueue.front();
		destructableBreakMessageQueue.pop();
		destructableBreakMessageQueueLock.release();

		auto destructablePair = destructableMap.find(curMessage.data.id);
		if (destructablePair == destructableMap.end())
		{
			g_ModuleInterface->Print(CM_RED, "Couldn't find broken destructable in map");
		}
		else
		{
			RValue curDestructable = destructablePair->second.destructableInstance;
			g_ModuleInterface->CallBuiltin("variable_instance_set", { curDestructable, "broken", true });
		}
	} while (true);
}

int receiveEliminateLevelUpClientChoiceMessage(uint32_t playerID)
{
	messageEliminateLevelUpClientChoice curMessage = messageEliminateLevelUpClientChoice();
	curMessage.m_playerID = playerID;
	int curMessageLen = curMessage.receiveMessage(playerID);
	eliminateLevelUpClientChoiceMessageQueueLock.acquire();
	eliminateLevelUpClientChoiceMessageQueue.push(curMessage);
	eliminateLevelUpClientChoiceMessageQueueLock.release();

	return curMessageLen;
}

void handleEliminateLevelUpClientChoiceMessage()
{
	do
	{
		eliminateLevelUpClientChoiceMessageQueueLock.acquire();
		if (eliminateLevelUpClientChoiceMessageQueue.empty())
		{
			eliminateLevelUpClientChoiceMessageQueueLock.release();
			break;
		}
		messageEliminateLevelUpClientChoice curMessage = eliminateLevelUpClientChoiceMessageQueue.front();
		eliminateLevelUpClientChoiceMessageQueue.pop();
		eliminateLevelUpClientChoiceMessageQueueLock.release();

		uint32_t playerID = curMessage.m_playerID;
		int levelUpOption = curMessage.levelUpOption;
		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });

		swapPlayerDataPush(playerManagerInstanceVar, attackController, playerID);

		RValue returnVal;
		RValue optionName(levelUpOptionNamesMap[playerID].optionArr[levelUpOption].second);

		switch (levelUpOptionNamesMap[playerID].optionArr[levelUpOption].first)
		{
			case optionType_Weapon:
			{
				RValue** args = new RValue*[1];
				args[0] = &optionName;
				origEliminateAttackScript(playerManagerInstanceVar, nullptr, returnVal, 1, args);
				break;
			}
			case optionType_Item:
			{
				RValue** args = new RValue*[1];
				args[0] = &optionName;
				origRemoveItemScript(playerManagerInstanceVar, nullptr, returnVal, 1, args);
				break;
			}
			case optionType_Skill:
			{
				RValue** args = new RValue*[1];
				args[0] = &optionName;
				origRemovePerkScript(playerManagerInstanceVar, nullptr, returnVal, 1, args);
				break;
			}
			default:
			{
				g_ModuleInterface->Print(CM_RED, "Error while eliminating - Unknown level up type");
			}
		}

		swapPlayerDataPop(playerManagerInstanceVar, attackController);
	} while (true);
}

int receiveClientSpecialAttackMessage(uint32_t playerID)
{
	clientSpecialAttackMessageQueueLock.acquire();
	clientSpecialAttackMessageQueue.push(playerID);
	clientSpecialAttackMessageQueueLock.release();

	return 1;
}

void handleClientSpecialAttackMessage()
{
	do
	{
		clientSpecialAttackMessageQueueLock.acquire();
		if (clientSpecialAttackMessageQueue.empty())
		{
			clientSpecialAttackMessageQueueLock.release();
			break;
		}
		uint32_t playerID = clientSpecialAttackMessageQueue.front();
		clientSpecialAttackMessageQueue.pop();
		clientSpecialAttackMessageQueueLock.release();

		RValue returnVal;
		RValue paused = getInstanceVariable(playerManagerInstanceVar, GML_paused);
		if (!paused.ToBoolean())
		{
			RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
			swapPlayerDataPush(playerManagerInstanceVar, attackController, playerID);

			// Seems like it's okay even if the Self variable is the playerManager
			origExecuteSpecialAttackScript(playerManagerInstanceVar, nullptr, returnVal, 0, nullptr);

			swapPlayerDataPop(playerManagerInstanceVar, attackController);
		}
	} while (true);
}

int receiveCautionCreateMessage(uint32_t playerID)
{
	RValue returnVal;
	messageCautionCreate curMessage = messageCautionCreate();
	int curMessageLen = curMessage.receiveMessage(playerID);

	cautionCreateMessageQueueLock.acquire();
	cautionCreateMessageQueue.push(curMessage);
	cautionCreateMessageQueueLock.release();

	return curMessageLen;
}

void handleCautionCreateMessage()
{
	do
	{
		cautionCreateMessageQueueLock.acquire();
		if (cautionCreateMessageQueue.empty())
		{
			cautionCreateMessageQueueLock.release();
			break;
		}
		messageCautionCreate curMessage = cautionCreateMessageQueue.front();
		cautionCreateMessageQueue.pop();
		cautionCreateMessageQueueLock.release();

		cautionData data = curMessage.data;

		int objIndex = -1;

		if (data.cautionType == 0)
		{
			// Caution
			objIndex = objCautionIndex;
		}
		else if (data.cautionType == 1)
		{
			// Caution attack
			objIndex = objCautionAttackIndex;
		}

		RValue cautionInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { data.xPos, data.yPos, -15000, objIndex });
		setInstanceVariable(cautionInstance, GML_dir, RValue(data.dir));
	} while (true);
}

int receivePreCreateUpdateMessage(uint32_t playerID)
{
	RValue returnVal;
	messagePreCreateUpdate curMessage = messagePreCreateUpdate();
	int curMessageLen = curMessage.receiveMessage(playerID);

	preCreateUpdateMessageQueueLock.acquire();
	preCreateUpdateMessageQueue.push(curMessage);
	preCreateUpdateMessageQueueLock.release();

	return curMessageLen;
}

void handlePreCreateUpdateMessage()
{
	do
	{
		preCreateUpdateMessageQueueLock.acquire();
		if (preCreateUpdateMessageQueue.empty())
		{
			preCreateUpdateMessageQueueLock.release();
			break;
		}
		messagePreCreateUpdate curMessage = preCreateUpdateMessageQueue.front();
		preCreateUpdateMessageQueue.pop();
		preCreateUpdateMessageQueueLock.release();

		preCreateData data = curMessage.data;

		if (preCreateMap.find(data.id) == preCreateMap.end())
		{
			RValue cautionInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { data.xPos, data.yPos, -15000, objPreCreateIndex });
			preCreateMap[data.id] = cautionInstance;
			setInstanceVariable(cautionInstance, GML_waitSpawn, RValue(data.waitSpawn));
		}
		else
		{
			setInstanceVariable(preCreateMap[data.id], GML_waitSpawn, RValue(data.waitSpawn));
			if (data.waitSpawn == 1)
			{
				g_ModuleInterface->CallBuiltin("instance_destroy", { preCreateMap[data.id] });
				preCreateMap.erase(data.id);
			}
		}
	} while (true);
}

int receiveVFXUpdateMessage(uint32_t playerID)
{
	RValue returnVal;
	messageVfxUpdate curMessage = messageVfxUpdate();
	int curMessageLen = curMessage.receiveMessage(playerID);

	vfxUpdateMessageQueueLock.acquire();
	vfxUpdateMessageQueue.push(curMessage);
	vfxUpdateMessageQueueLock.release();

	return curMessageLen;
}

void handleVFXUpdateMessage()
{
	do
	{
		vfxUpdateMessageQueueLock.acquire();
		if (vfxUpdateMessageQueue.empty())
		{
			vfxUpdateMessageQueueLock.release();
			break;
		}
		messageVfxUpdate curMessage = vfxUpdateMessageQueue.front();
		vfxUpdateMessageQueue.pop();
		vfxUpdateMessageQueueLock.release();

		vfxData data = curMessage.data;

		if (data.type == 0)
		{
			// obj_vfx
			if (vfxMap.find(data.id) == vfxMap.end())
			{
				RValue vfxInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { data.xPos, data.yPos, -15000, objVFXIndex });
				vfxMap[data.id] = vfxInstance;
				setInstanceVariable(vfxInstance, GML_image_xscale, RValue(data.imageXScale));
				setInstanceVariable(vfxInstance, GML_image_yscale, RValue(data.imageYScale));
				setInstanceVariable(vfxInstance, GML_image_angle, RValue(data.imageAngle));
				setInstanceVariable(vfxInstance, GML_image_alpha, RValue(data.imageAlpha));
				setInstanceVariable(vfxInstance, GML_sprite_index, RValue(data.spriteIndex));
				setInstanceVariable(vfxInstance, GML_image_index, RValue(data.imageIndex));
				setInstanceVariable(vfxInstance, GML_image_speed, RValue(0));
				setInstanceVariable(vfxInstance, GML_spriteColor, RValue(static_cast<int>(data.color)));
			}
			else
			{
				RValue vfxInstance = vfxMap[data.id];
				setInstanceVariable(vfxInstance, GML_x, RValue(data.xPos));
				setInstanceVariable(vfxInstance, GML_y, RValue(data.yPos));
				setInstanceVariable(vfxInstance, GML_image_xscale, RValue(data.imageXScale));
				setInstanceVariable(vfxInstance, GML_image_yscale, RValue(data.imageYScale));
				setInstanceVariable(vfxInstance, GML_image_angle, RValue(data.imageAngle));
				setInstanceVariable(vfxInstance, GML_image_alpha, RValue(data.imageAlpha));
				setInstanceVariable(vfxInstance, GML_sprite_index, RValue(data.spriteIndex));
				setInstanceVariable(vfxInstance, GML_image_index, RValue(data.imageIndex));
				setInstanceVariable(vfxInstance, GML_image_speed, RValue(0));
				setInstanceVariable(vfxInstance, GML_spriteColor, RValue(static_cast<int>(data.color)));
				if (data.imageAlpha < 0)
				{
					g_ModuleInterface->CallBuiltin("instance_destroy", { vfxMap[data.id] });
					vfxMap.erase(data.id);
				}
			}
		}
		else if (data.type == 1)
		{
			// obj_afterimage
			if (vfxMap.find(data.id) == vfxMap.end())
			{
				RValue vfxInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { data.xPos, data.yPos, -15000, objAfterImageIndex });
				vfxMap[data.id] = vfxInstance;
				setInstanceVariable(vfxInstance, GML_image_xscale, RValue(data.imageXScale));
				setInstanceVariable(vfxInstance, GML_image_yscale, RValue(data.imageYScale));
				setInstanceVariable(vfxInstance, GML_image_angle, RValue(data.imageAngle));
				setInstanceVariable(vfxInstance, GML_image_alpha, RValue(data.imageAlpha));
				setInstanceVariable(vfxInstance, GML_sprite_index, RValue(data.spriteIndex));
				setInstanceVariable(vfxInstance, GML_image_index, RValue(data.imageIndex));
				setInstanceVariable(vfxInstance, GML_image_speed, RValue(0));
				setInstanceVariable(vfxInstance, GML_afterimage_color, RValue(static_cast<int>(data.color)));
			}
			else
			{
				RValue vfxInstance = vfxMap[data.id];
				setInstanceVariable(vfxInstance, GML_x, RValue(data.xPos));
				setInstanceVariable(vfxInstance, GML_y, RValue(data.yPos));
				setInstanceVariable(vfxInstance, GML_image_xscale, RValue(data.imageXScale));
				setInstanceVariable(vfxInstance, GML_image_yscale, RValue(data.imageYScale));
				setInstanceVariable(vfxInstance, GML_image_angle, RValue(data.imageAngle));
				setInstanceVariable(vfxInstance, GML_image_alpha, RValue(data.imageAlpha));
				setInstanceVariable(vfxInstance, GML_sprite_index, RValue(data.spriteIndex));
				setInstanceVariable(vfxInstance, GML_image_index, RValue(data.imageIndex));
				setInstanceVariable(vfxInstance, GML_image_speed, RValue(0));
				setInstanceVariable(vfxInstance, GML_afterimage_color, RValue(static_cast<int>(data.color)));
				if (data.imageAlpha < -100000)
				{
					g_ModuleInterface->CallBuiltin("instance_destroy", { vfxMap[data.id] });
					vfxMap.erase(data.id);
				}
			}
		}
	} while (true);
}

int receiveInteractableCreateMessage(uint32_t playerID)
{
	RValue returnVal;
	messageInteractableCreate curMessage = messageInteractableCreate();
	int curMessageLen = curMessage.receiveMessage(playerID);

	interactableCreateMessageQueueLock.acquire();
	interactableCreateMessageQueue.push(curMessage);
	interactableCreateMessageQueueLock.release();

	return curMessageLen;
}

void handleInteractableCreateMessage()
{
	do
	{
		interactableCreateMessageQueueLock.acquire();
		if (interactableCreateMessageQueue.empty())
		{
			interactableCreateMessageQueueLock.release();
			break;
		}
		messageInteractableCreate curMessage = interactableCreateMessageQueue.front();
		interactableCreateMessageQueue.pop();
		interactableCreateMessageQueueLock.release();

		interactableData data = curMessage.data;

		switch (data.type)
		{
		case 0: // holoBox
		{
			interactableMap[data.id] = g_ModuleInterface->CallBuiltin("instance_create_depth", { data.xPos, data.yPos, -15000, objHoloBoxIndex });
			break;
		}
		case 1: // holoAnvil
		{
			interactableMap[data.id] = g_ModuleInterface->CallBuiltin("instance_create_depth", { data.xPos, data.yPos, -15000, objHoloAnvilIndex });
			break;
		}
		case 2: // goldenAnvil
		{
			interactableMap[data.id] = g_ModuleInterface->CallBuiltin("instance_create_depth", { data.xPos, data.yPos, -15000, objGoldenAnvilIndex });
			break;
		}
		case 3: // goldenHammer
		{
			interactableMap[data.id] = g_ModuleInterface->CallBuiltin("instance_create_depth", { data.xPos, data.yPos, -15000, objGoldenHammerIndex });
			break;
		}
		case 4: // stamp
		{
			RValue stickerInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { data.xPos, data.yPos, -15000, objStickerIndex });
			interactableMap[data.id] = stickerInstance;
			setInstanceVariable(stickerInstance, GML_sprite_index, RValue(static_cast<double>(data.spriteIndex)));
			setInstanceVariable(stickerInstance, GML_stickerID, RValue(0.0));
			setInstanceVariable(stickerInstance, GML_stickerData, RValue(0.0));
			break;
		}
		}
	} while (true);
}

int receiveInteractableDeleteMessage(uint32_t playerID)
{
	RValue returnVal;
	messageInteractableDelete curMessage = messageInteractableDelete();
	int curMessageLen = curMessage.receiveMessage(playerID);

	interactableDeleteMessageQueueLock.acquire();
	interactableDeleteMessageQueue.push(curMessage);
	interactableDeleteMessageQueueLock.release();

	return curMessageLen;
}

void handleInteractableDeleteMessage()
{
	do
	{
		interactableDeleteMessageQueueLock.acquire();
		if (interactableDeleteMessageQueue.empty())
		{
			interactableDeleteMessageQueueLock.release();
			break;
		}
		messageInteractableDelete curMessage = interactableDeleteMessageQueue.front();
		interactableDeleteMessageQueue.pop();
		interactableDeleteMessageQueueLock.release();

		switch (curMessage.type)
		{
		case 0: // holoBox
		{
			g_ModuleInterface->CallBuiltin("instance_destroy", { interactableMap[curMessage.id] });
			interactableMap.erase(curMessage.id);
			break;
		}
		case 1: // holoAnvil
		case 2: // goldenAnvil
		case 3: // goldenHammer
		case 4: // stamp
		{
			RValue destroyMethod = getInstanceVariable(interactableMap[curMessage.id], GML_Destroy);
			RValue destroyArr = g_ModuleInterface->CallBuiltin("array_create", { RValue(0.0) });
			g_ModuleInterface->CallBuiltin("method_call", { destroyMethod, destroyArr });
			break;
		}
		}
	} while (true);
}

bool isClientUsingBox = false;
bool isClientUsingAnvil = false;
bool isClientUsingGoldenAnvil = false;
bool isClientUsingStamp = false;

std::vector<std::string> currentAnvilRolledMods;

int receiveInteractablePlayerInteractedMessage(uint32_t playerID)
{
	messageInteractablePlayerInteracted curMessage = messageInteractablePlayerInteracted();
	int curMessageLen = curMessage.receiveMessage(playerID);

	interactablePlayerInteractedMessageQueueLock.acquire();
	interactablePlayerInteractedMessageQueue.push(curMessage);
	interactablePlayerInteractedMessageQueueLock.release();

	return curMessageLen;
}

void handleInteractablePlayerInteractedMessage()
{
	do
	{
		interactablePlayerInteractedMessageQueueLock.acquire();
		if (interactablePlayerInteractedMessageQueue.empty())
		{
			interactablePlayerInteractedMessageQueueLock.release();
			break;
		}
		messageInteractablePlayerInteracted curMessage = interactablePlayerInteractedMessageQueue.front();
		interactablePlayerInteractedMessageQueue.pop();
		interactablePlayerInteractedMessageQueueLock.release();

		// Can probably skip pausing the clients since the host will be paused anyways
		if (curMessage.m_playerID == clientID)
		{
			RValue returnVal;
			setInstanceVariable(playerManagerInstanceVar, GML_paused, RValue(false));
			unsetPauseMenu();
			switch (curMessage.type)
			{
				case 1: // holoAnvil
				{
					currentAnvilRolledMods.clear();
					isClientUsingAnvil = true;
					RValue anvilInstance = interactableMap[curMessage.id];
					RValue** args = new RValue*[1];
					args[0] = &anvilInstance;
					origGetAnvilScript(playerManagerInstanceVar, nullptr, returnVal, 1, args);
					setInstanceVariable(playerManagerInstanceVar, GML_anvilID, RValue(-1));
					break;
				}
				case 2: // goldenAnvil
				{
					isClientUsingGoldenAnvil = true;
					RValue anvilInstance = interactableMap[curMessage.id];
					RValue** args = new RValue*[1];
					args[0] = &anvilInstance;
					origGetGoldenAnvilScript(playerManagerInstanceVar, nullptr, returnVal, 1, args);
					setInstanceVariable(playerManagerInstanceVar, GML_anvilID, RValue(-1));
					break;
				}
			}
		}
	} while (true);
}

int receiveStickerPlayerInteractedMessage(uint32_t playerID)
{
	messageStickerPlayerInteracted curMessage = messageStickerPlayerInteracted();
	int curMessageLen = curMessage.receiveMessage(playerID);

	stickerPlayerInteractedMessageQueueLock.acquire();
	stickerPlayerInteractedMessageQueue.push(curMessage);
	stickerPlayerInteractedMessageQueueLock.release();

	return curMessageLen;
}

void handleStickerPlayerInteractedMessage()
{
	do
	{
		stickerPlayerInteractedMessageQueueLock.acquire();
		if (stickerPlayerInteractedMessageQueue.empty())
		{
			stickerPlayerInteractedMessageQueueLock.release();
			break;
		}
		messageStickerPlayerInteracted curMessage = stickerPlayerInteractedMessageQueue.front();
		stickerPlayerInteractedMessageQueue.pop();
		stickerPlayerInteractedMessageQueueLock.release();

		// Can probably skip pausing the clients since the host will be paused anyways
		if (curMessage.m_playerID == clientID)
		{
			RValue returnVal;
			setInstanceVariable(playerManagerInstanceVar, GML_paused, RValue(false));
			unsetPauseMenu();
			isClientUsingStamp = true;
			RValue stickerInstance = interactableMap[curMessage.id];
			setInstanceVariable(stickerInstance, GML_stickerID, RValue(curMessage.stickerID));
			RValue stickersMap = getInstanceVariable(playerManagerInstanceVar, GML_STICKERS);
			RValue stickerData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { stickersMap, curMessage.stickerID.c_str() });
			setInstanceVariable(stickerInstance, GML_stickerData, stickerData);
			g_ModuleInterface->CallBuiltin("variable_global_set", { "collectedSticker", stickerData });
			RValue** args = new RValue*[1];
			args[0] = &stickerInstance;
			origGetStickerScript(playerManagerInstanceVar, nullptr, returnVal, 1, args);
		}
	} while (true);
}

int receiveBoxPlayerInteractedMessage(uint32_t playerID)
{
	messageBoxPlayerInteracted curMessage = messageBoxPlayerInteracted();
	int curMessageLen = curMessage.receiveMessage(playerID);

	boxPlayerInteractedMessageQueueLock.acquire();
	boxPlayerInteractedMessageQueue.push(curMessage);
	boxPlayerInteractedMessageQueueLock.release();

	return curMessageLen;
}

void handleBoxPlayerInteractedMessage()
{
	do
	{
		boxPlayerInteractedMessageQueueLock.acquire();
		if (boxPlayerInteractedMessageQueue.empty())
		{
			boxPlayerInteractedMessageQueueLock.release();
			break;
		}
		messageBoxPlayerInteracted curMessage = boxPlayerInteractedMessageQueue.front();
		boxPlayerInteractedMessageQueue.pop();
		boxPlayerInteractedMessageQueueLock.release();

		g_ModuleInterface->CallBuiltin("instance_destroy", { interactableMap[curMessage.id] });
		interactableMap.erase(curMessage.id);

		// Can probably skip pausing the clients since the host will be paused anyways
		if (curMessage.m_playerID == clientID)
		{
			RValue returnVal;
			setInstanceVariable(playerManagerInstanceVar, GML_paused, RValue(false));
			unsetPauseMenu();
			isClientUsingBox = true;
			origGetBoxScript(playerManagerInstanceVar, nullptr, returnVal, 0, nullptr);
			if (curMessage.isSuperBox == 1)
			{
				RValue randomWeaponArr = g_ModuleInterface->CallBuiltin("array_create", { 1 });
				setInstanceVariable(playerManagerInstanceVar, GML_superBox, RValue(true));
				setInstanceVariable(playerManagerInstanceVar, GML_boxItemAmount, RValue(1.0));

				RValue optionStruct;
				g_RunnerInterface.StructCreate(&optionStruct);
				levelUpOption superItem = curMessage.randomWeapons[0];

				// Need to do this since the super item check only checks from the items map
				RValue itemsMap = getInstanceVariable(playerManagerInstanceVar, GML_ITEMS);
				RValue curItem = g_ModuleInterface->CallBuiltin("ds_map_find_value", { itemsMap, superItem.optionID.c_str() });
				setInstanceVariable(curItem, GML_becomeSuper, RValue(true));
				setInstanceVariable(curItem, GML_optionIcon_Normal, RValue(superItem.optionIcon));
				setInstanceVariable(curItem, GML_optionIcon, RValue(superItem.optionIcon_Super));

				setInstanceVariable(optionStruct, GML_optionIcon, RValue(superItem.optionIcon_Super));
				setInstanceVariable(optionStruct, GML_optionIcon_Normal, RValue(superItem.optionIcon));
				setInstanceVariable(optionStruct, GML_optionType, RValue(superItem.optionType));
				setInstanceVariable(optionStruct, GML_optionName, RValue(superItem.optionName));
				setInstanceVariable(optionStruct, GML_optionID, RValue(superItem.optionID));
				setInstanceVariable(optionStruct, GML_optionIcon_Super, RValue(superItem.optionIcon_Super));
				if (superItem.optionDescription.size() == 1)
				{
					setInstanceVariable(optionStruct, GML_optionDescription, RValue(superItem.optionDescription[0]));
				}
				else
				{
					int optionDescriptionSize = static_cast<int>(superItem.optionDescription.size());
					RValue optionDescriptionArr = g_ModuleInterface->CallBuiltin("array_create", { optionDescriptionSize });
					for (int i = 0; i < optionDescriptionSize; i++)
					{
						optionDescriptionArr[i] = superItem.optionDescription[i].c_str();
					}
					setInstanceVariable(optionStruct, GML_optionDescription, optionDescriptionArr);
				}
				setInstanceVariable(optionStruct, GML_attackID, RValue(""));
				setInstanceVariable(optionStruct, GML_becomeSuper, RValue(true));
				setInstanceVariable(optionStruct, GML_itemType, RValue(""));
				setInstanceVariable(optionStruct, GML_weaponType, RValue(""));
				// TODO: Check if this is correct in all cases. Not really sure if there is a difference between id and optionID
				setInstanceVariable(optionStruct, GML_id, RValue(superItem.optionID));
				randomWeaponArr[0] = optionStruct;

				setInstanceVariable(playerManagerInstanceVar, GML_randomWeapon, randomWeaponArr);
			}
			else
			{
				setInstanceVariable(playerManagerInstanceVar, GML_superBox, RValue(false));
				int boxItemAmount = static_cast<int>(curMessage.boxItemAmount);
				setInstanceVariable(playerManagerInstanceVar, GML_boxItemAmount, RValue(static_cast<double>(boxItemAmount)));
				RValue randomWeaponArr = g_ModuleInterface->CallBuiltin("array_create", { boxItemAmount });

				for (int i = 0; i < boxItemAmount; i++)
				{
					RValue optionStruct;
					g_RunnerInterface.StructCreate(&optionStruct);
					levelUpOption superItem = curMessage.randomWeapons[i];
					setInstanceVariable(optionStruct, GML_optionIcon, RValue(superItem.optionIcon));
					setInstanceVariable(optionStruct, GML_optionType, RValue(superItem.optionType));
					setInstanceVariable(optionStruct, GML_optionName, RValue(superItem.optionName));
					setInstanceVariable(optionStruct, GML_optionID, RValue(superItem.optionID));
					if (superItem.optionDescription.size() == 1)
					{
						setInstanceVariable(optionStruct, GML_optionDescription, RValue(superItem.optionDescription[0]));
					}
					else
					{
						int optionDescriptionSize = static_cast<int>(superItem.optionDescription.size());
						RValue optionDescriptionArr = g_ModuleInterface->CallBuiltin("array_create", { optionDescriptionSize });
						for (int i = 0; i < optionDescriptionSize; i++)
						{
							optionDescriptionArr[i] = superItem.optionDescription[i].c_str();
						}
						setInstanceVariable(optionStruct, GML_optionDescription, optionDescriptionArr);
					}
					setInstanceVariable(optionStruct, GML_attackID, RValue(""));
					setInstanceVariable(optionStruct, GML_becomeSuper, RValue(false));
					setInstanceVariable(optionStruct, GML_itemType, RValue(""));
					setInstanceVariable(optionStruct, GML_weaponType, RValue(""));
					randomWeaponArr[i] = optionStruct;
				}

				setInstanceVariable(playerManagerInstanceVar, GML_randomWeapon, randomWeaponArr);
			}
		}
	} while (true);
}

int receiveInteractFinishedMessage(uint32_t playerID)
{
	interactFinishedMessageQueueLock.acquire();
	hasInteractFinishedMessage = true;
	interactFinishedMessageQueueLock.release();
	return 1;
}

void handleInteractFinishedMessage()
{
	do
	{
		interactFinishedMessageQueueLock.acquire();
		if (!hasInteractFinishedMessage)
		{
			interactFinishedMessageQueueLock.release();
			break;
		}
		hasInteractFinishedMessage = false;
		interactFinishedMessageQueueLock.release();

		hasClientFinishedInteracting = true;
		unpauseHost();

		// TODO: Check to make sure this doesn't cause any issues
		if (isClientUsingStamp)
		{
			setInstanceVariable(playerManagerInstanceVar, GML_usedSticker, RValue(true));
		}
	} while (true);
}

int receiveBoxTakeOptionMessage(uint32_t playerID)
{
	RValue returnVal;
	messageBoxTakeOption curMessage = messageBoxTakeOption();
	int curMessageLen = curMessage.receiveMessage(playerID);
	curMessage.m_playerID = playerID;

	boxTakeOptionMessageQueueLock.acquire();
	boxTakeOptionMessageQueue.push(curMessage);
	boxTakeOptionMessageQueueLock.release();

	return curMessageLen;
}

void handleBoxTakeOptionMessage()
{
	do
	{
		boxTakeOptionMessageQueueLock.acquire();
		if (boxTakeOptionMessageQueue.empty())
		{
			boxTakeOptionMessageQueueLock.release();
			break;
		}
		messageBoxTakeOption curMessage = boxTakeOptionMessageQueue.front();
		boxTakeOptionMessageQueue.pop();
		boxTakeOptionMessageQueueLock.release();

		uint32_t playerID = curMessage.m_playerID;
		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });

		// Special case if the client drops a super weapon to unset the super flag for the item
		if (curMessage.boxItemNum == 100)
		{
			RValue itemsMap = playerItemsMapMap[playerID];
			RValue curItem = g_ModuleInterface->CallBuiltin("ds_map_find_value", { itemsMap, randomWeaponArr[0].optionID.c_str() });
			setInstanceVariable(curItem, GML_becomeSuper, RValue(false));
			setInstanceVariable(curItem, GML_optionIcon, getInstanceVariable(curItem, GML_optionIcon_Normal));
			return;
		}

		levelUpPausedData curPausedData = levelUpPausedData(playerID, convertStringOptionTypeToEnum(randomWeaponArr[curMessage.boxItemNum].optionType.c_str()), randomWeaponArr[curMessage.boxItemNum].optionID.c_str());
		levelUpPausedList.push_back(curPausedData);
	} while (true);
}

int receiveAnvilChooseOptionMessage(uint32_t playerID)
{
	RValue returnVal;
	messageAnvilChooseOption curMessage = messageAnvilChooseOption();
	int curMessageLen = curMessage.receiveMessage(playerID);
	curMessage.m_playerID = playerID;

	anvilChooseOptionMessageQueueLock.acquire();
	anvilChooseOptionMessageQueue.push(curMessage);
	anvilChooseOptionMessageQueueLock.release();

	return curMessageLen;
}

void handleAnvilChooseOptionMessage()
{
	do
	{
		anvilChooseOptionMessageQueueLock.acquire();
		if (anvilChooseOptionMessageQueue.empty())
		{
			anvilChooseOptionMessageQueueLock.release();
			break;
		}
		messageAnvilChooseOption curMessage = anvilChooseOptionMessageQueue.front();
		anvilChooseOptionMessageQueue.pop();
		anvilChooseOptionMessageQueueLock.release();
		uint32_t playerID = curMessage.m_playerID;

		if (curMessage.anvilOptionType == 0)
		{
			levelUpPausedData curPausedData = levelUpPausedData(playerID, convertStringOptionTypeToEnum(curMessage.optionType.c_str()), curMessage.optionID.c_str());
			levelUpPausedList.push_back(curPausedData);
		}
		else if (curMessage.anvilOptionType == 1)
		{
			levelUpPausedData curPausedData = levelUpPausedData(playerID, convertStringOptionTypeToEnum(curMessage.optionType.c_str()), curMessage.optionID.c_str());
			levelUpPausedList.push_back(curPausedData);
			double updatedMoney = g_ModuleInterface->CallBuiltin("variable_global_get", { "currentRunMoneyGained" }).ToDouble() - curMessage.coinCost;
			g_ModuleInterface->CallBuiltin("variable_global_set", { "currentRunMoneyGained", updatedMoney });
		}
		setInstanceVariable(playerManagerInstanceVar, GML_usedAnvil, RValue(true));
	} while (true);
}

int receiveClientGainMoneyMessage(uint32_t playerID)
{
	messageClientGainMoney curMessage = messageClientGainMoney();
	int curMessageLen = curMessage.receiveMessage(playerID);

	clientGainMoneyMessageQueueLock.acquire();
	clientGainMoneyMessageQueue.push(curMessage);
	clientGainMoneyMessageQueueLock.release();

	return curMessageLen;
}

void handleClientGainMoneyMessage()
{
	do
	{
		clientGainMoneyMessageQueueLock.acquire();
		if (clientGainMoneyMessageQueue.empty())
		{
			clientGainMoneyMessageQueueLock.release();
			break;
		}
		messageClientGainMoney curMessage = clientGainMoneyMessageQueue.front();
		clientGainMoneyMessageQueue.pop();
		clientGainMoneyMessageQueueLock.release();

		RValue returnVal;
		double updatedMoney = g_ModuleInterface->CallBuiltin("variable_global_get", { "currentRunMoneyGained" }).ToDouble() + curMessage.money;
		g_ModuleInterface->CallBuiltin("variable_global_set", { "currentRunMoneyGained", updatedMoney });
	} while (true);
}

int receiveClientAnvilEnchantMessage(uint32_t playerID)
{
	RValue returnVal;
	messageClientAnvilEnchant curMessage = messageClientAnvilEnchant();
	int curMessageLen = curMessage.receiveMessage(playerID);
	curMessage.m_playerID = playerID;

	clientAnvilEnchantMessageQueueLock.acquire();
	clientAnvilEnchantMessageQueue.push(curMessage);
	clientAnvilEnchantMessageQueueLock.release();

	return curMessageLen;
}

void handleClientAnvilEnchantMessage()
{
	do
	{
		clientAnvilEnchantMessageQueueLock.acquire();
		if (clientAnvilEnchantMessageQueue.empty())
		{
			clientAnvilEnchantMessageQueueLock.release();
			break;
		}
		messageClientAnvilEnchant curMessage = clientAnvilEnchantMessageQueue.front();
		clientAnvilEnchantMessageQueue.pop();
		clientAnvilEnchantMessageQueueLock.release();

		uint32_t playerID = curMessage.m_playerID;
		levelUpPausedData curPausedData = levelUpPausedData(playerID, optionType_Enchant, curMessage.optionID.c_str(), curMessage.gainedMods);
		levelUpPausedList.push_back(curPausedData);

		double updatedMoney = g_ModuleInterface->CallBuiltin("variable_global_get", { "currentRunMoneyGained" }).ToDouble() - curMessage.coinCost;
		g_ModuleInterface->CallBuiltin("variable_global_set", { "currentRunMoneyGained", updatedMoney });

		setInstanceVariable(playerManagerInstanceVar, GML_usedAnvil, RValue(true));
	} while (true);
}

int receiveStickerChooseOptionMessage(uint32_t playerID)
{
	RValue returnVal;
	messageStickerChooseOption curMessage = messageStickerChooseOption();
	int curMessageLen = curMessage.receiveMessage(playerID);
	curMessage.m_playerID = playerID;

	stickerChooseOptionMessageQueueLock.acquire();
	stickerChooseOptionMessageQueue.push(curMessage);
	stickerChooseOptionMessageQueueLock.release();

	return curMessageLen;
}

void handleStickerChooseOptionMessage()
{
	do
	{
		stickerChooseOptionMessageQueueLock.acquire();
		if (stickerChooseOptionMessageQueue.empty())
		{
			stickerChooseOptionMessageQueueLock.release();
			break;
		}
		messageStickerChooseOption curMessage = stickerChooseOptionMessageQueue.front();
		stickerChooseOptionMessageQueue.pop();
		stickerChooseOptionMessageQueueLock.release();

		RValue attackController = g_ModuleInterface->CallBuiltin("instance_find", { objAttackControllerIndex, 0 });
		uint32_t playerID = curMessage.m_playerID;
		swapPlayerDataPush(playerManagerInstanceVar, attackController, playerID);
		switch (curMessage.stickerOptionType)
		{
			case 0:
			{
				// take stamp/level up
				RValue stickersMap = getInstanceVariable(playerManagerInstanceVar, GML_STICKERS);
				RValue stickerData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { stickersMap, collidedStickerID.c_str() });
				RValue currentStickersArr = currentStickersMap[playerID];
				if (curMessage.stickerOption == 0)
				{
					if (currentStickersArr[curMessage.stickerOption - 1].m_Kind == VALUE_OBJECT)
					{
						RValue levelUpMethod = getInstanceVariable(currentStickersArr[curMessage.stickerOption - 1], GML_LevelUp);
						RValue levelUpArr = g_ModuleInterface->CallBuiltin("array_create", { RValue(0.0) });
						g_ModuleInterface->CallBuiltin("method_call", { levelUpMethod, levelUpArr });
					}
					else
					{
						currentStickersArr[curMessage.stickerOption - 1] = stickerData;
					}
				}
				else
				{
					for (int i = 0; i < 3; i++)
					{
						if (currentStickersArr[i].m_Kind != VALUE_OBJECT)
						{
							currentStickersArr[i] = stickerData;
							break;
						}
					}
				}
				collidedStickerID = "";
				g_ModuleInterface->CallBuiltin("variable_global_set", { "collectedSticker", -1.0 });
				break;
			}
			case 1:
			{
				// remove stamp
				RValue stickerData(-1);
				if (!collidedStickerID.empty())
				{
					RValue stickersMap = getInstanceVariable(playerManagerInstanceVar, GML_STICKERS);
					stickerData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { stickersMap, collidedStickerID.c_str() });
				}
				RValue currentStickersArr = currentStickersMap[playerID];
				RValue currentSticker = currentStickersArr[curMessage.stickerOption - 1];
				if (currentSticker.m_Kind == VALUE_OBJECT)
				{
					// Hopefully this stays the same as the key value in the map
					RValue stickerID = getInstanceVariable(currentSticker, GML_optionID);
					collidedStickerID = stickerID.ToString();
				}
				else
				{
					collidedStickerID = "";
				}
				// TODO: Should probably test if it's possible that the host can't have a sticker at the same time a client tries to drop one for this
				g_ModuleInterface->CallBuiltin("variable_global_set", { "collectedSticker", currentSticker });
				setInstanceVariable(playerManagerInstanceVar, GML_usedSticker, true);
				currentStickersArr[curMessage.stickerOption - 1] = stickerData;

				break;
			}
			case 2:
			{
				// sell stamp
				int stickerLevel = 0;
				if (curMessage.stickerOption == 0)
				{
					RValue collectedSticker = g_ModuleInterface->CallBuiltin("variable_global_get", { "collectedSticker" });
					stickerLevel = static_cast<int>(lround(getInstanceVariable(collectedSticker, GML_level).ToDouble()));
					collidedStickerID = "";
					g_ModuleInterface->CallBuiltin("variable_global_set", { "collectedSticker", -1.0 });
				}
				else
				{
					RValue currentStickersArr = currentStickersMap[playerID];
					RValue currentSticker = currentStickersArr[curMessage.stickerOption - 1];
					stickerLevel = static_cast<int>(lround(getInstanceVariable(currentSticker, GML_level).ToDouble()));

					currentStickersArr[curMessage.stickerOption - 1] = -1.0;
				}
				int coinCount = static_cast<int>(g_ModuleInterface->CallBuiltin("variable_global_get", { "currentRunMoneyGained" }).ToDouble());
				double stageCoinBonus = g_ModuleInterface->CallBuiltin("variable_global_get", { "stageCoinBonus" }).ToDouble();
				coinCount += static_cast<int>(floor(stageCoinBonus * 100 * (1 + moneyGainMultiplier)) * (stickerLevel + 1));
				g_ModuleInterface->CallBuiltin("variable_global_set", { "currentRunMoneyGained", static_cast<double>(coinCount) });
				break;
			}
		}
		swapPlayerDataPop(playerManagerInstanceVar, attackController);
	} while (true);
}

int receiveChooseCollabMessage(uint32_t playerID)
{
	messageChooseCollab curMessage = messageChooseCollab();
	int curMessageLen = curMessage.receiveMessage(playerID);
	curMessage.m_playerID = playerID;

	chooseCollabMessageQueueLock.acquire();
	chooseCollabMessageQueue.push(curMessage);
	chooseCollabMessageQueueLock.release();

	return curMessageLen;
}

void handleChooseCollabMessage()
{
	do
	{
		chooseCollabMessageQueueLock.acquire();
		if (chooseCollabMessageQueue.empty())
		{
			chooseCollabMessageQueueLock.release();
			break;
		}
		messageChooseCollab curMessage = chooseCollabMessageQueue.front();
		chooseCollabMessageQueue.pop();
		chooseCollabMessageQueueLock.release();

		uint32_t playerID = curMessage.m_playerID;
		levelUpOption curOption = curMessage.collab;

		levelUpPausedData curPausedData = levelUpPausedData(playerID, convertStringOptionTypeToEnum(curOption.optionType.c_str()), curOption.optionID.c_str());
		levelUpPausedList.push_back(curPausedData);
		setInstanceVariable(playerManagerInstanceVar, GML_usedAnvil, RValue(true));
	} while (true);
}

int receiveBuffDataMessage(uint32_t playerID)
{
	messageBuffData curMessage = messageBuffData();
	int curMessageLen = curMessage.receiveMessage(playerID);

	buffDataMessageQueueLock.acquire();
	buffDataMessageQueue.push(curMessage);
	buffDataMessageQueueLock.release();

	return curMessageLen;
}

void handleBuffDataMessage()
{
	do
	{
		buffDataMessageQueueLock.acquire();
		if (buffDataMessageQueue.empty())
		{
			buffDataMessageQueueLock.release();
			break;
		}
		messageBuffData curMessage = buffDataMessageQueue.front();
		buffDataMessageQueue.pop();
		buffDataMessageQueueLock.release();

		// TODO: Switch from replacing the buffs struct to only replacing buffs that have been added or removed. Will need to rewrite how/when buffs are sent to implement this
		RValue buffStruct;
		g_RunnerInterface.StructCreate(&buffStruct);
		std::vector<buffData> buffDataList = curMessage.buffDataList;
		for (int i = 0; i < buffDataList.size(); i++)
		{
			buffData curBuffData = buffDataList[i];
			RValue buff;
			g_RunnerInterface.StructCreate(&buff);
			setInstanceVariable(buff, GML_timer, static_cast<double>(curBuffData.timer));
			RValue config;
			g_RunnerInterface.StructCreate(&config);
			if (curBuffData.stacks >= 0)
			{
				setInstanceVariable(config, GML_stacks, static_cast<double>(curBuffData.stacks));
			}
			setInstanceVariable(buff, GML_config, config);
			g_ModuleInterface->CallBuiltin("variable_instance_set", { buffStruct, curBuffData.buffName.c_str(), buff });
		}
		setInstanceVariable(playerMap[clientID], GML_buffs, buffStruct);
	} while (true);
}

int receiveCharDataMessage(uint32_t playerID)
{
	messageCharData curMessage = messageCharData();
	int curMessageLen = curMessage.receiveMessage(playerID);

	charDataMessageQueueLock.acquire();
	charDataMessageQueue.push(curMessage);
	charDataMessageQueueLock.release();

	return curMessageLen;
}

void handleCharDataMessage()
{
	do
	{
		charDataMessageQueueLock.acquire();
		if (charDataMessageQueue.empty())
		{
			charDataMessageQueueLock.release();
			break;
		}
		messageCharData curMessage = charDataMessageQueue.front();
		charDataMessageQueue.pop();
		charDataMessageQueueLock.release();

		lobbyPlayerDataMap[curMessage.m_playerID] = curMessage.playerData;
	} while (true);
}

int receiveReturnToLobby(uint32_t playerID)
{
	ReturnToLobbyQueueLock.acquire();
	hasReturnToLobby = true;
	ReturnToLobbyQueueLock.release();
	return 1;
}

void handleReturnToLobby()
{
	ReturnToLobbyQueueLock.acquire();
	if (hasReturnToLobby)
	{
//		switchToMenu(selectedMenu_Lobby);
		holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, lobbyMenuGrid.menuGridPtr);
		g_ModuleInterface->CallBuiltin("room_restart", {});
		g_ModuleInterface->CallBuiltin("room_goto", { rmTitle });
		g_ModuleInterface->CallBuiltin("instance_destroy", { playerManagerInstanceVar });
		g_ModuleInterface->CallBuiltin("variable_global_set", { "resetLevel", true });
		cleanupPlayerGameData();
	}
	hasReturnToLobby = false;
	ReturnToLobbyQueueLock.release();
}

int receiveLobbyPlayerDisconnected(uint32_t playerID)
{
	messageLobbyPlayerDisconnected curMessage = messageLobbyPlayerDisconnected();
	int curMessageLen = curMessage.receiveMessage(playerID);

	lobbyPlayerDisconnectedQueueLock.acquire();
	lobbyPlayerDisconnectedQueue.push(curMessage);
	lobbyPlayerDisconnectedQueueLock.release();

	return curMessageLen;
}

void handleLobbyPlayerDisconnected()
{
	do
	{
		lobbyPlayerDisconnectedQueueLock.acquire();
		if (lobbyPlayerDisconnectedQueue.empty())
		{
			lobbyPlayerDisconnectedQueueLock.release();
			break;
		}
		messageLobbyPlayerDisconnected curMessage = lobbyPlayerDisconnectedQueue.front();
		lobbyPlayerDisconnectedQueue.pop();
		lobbyPlayerDisconnectedQueueLock.release();

		playerPingMap.erase(curMessage.m_playerID);
		clientUnpausedMap.erase(curMessage.m_playerID);
		clientSocketMap.erase(curMessage.m_playerID);
		lobbyPlayerDataMap.erase(curMessage.m_playerID);
		hasClientPlayerDisconnected.erase(curMessage.m_playerID);
	} while (true);
}

int receiveHostHasPaused(uint32_t playerID)
{
	hostHasPausedQueueLock.acquire();
	receivedHostHasPaused = true;
	hostHasPausedQueueLock.release();

	return 1;
}

void handleHostHasPaused()
{
	hostHasPausedQueueLock.acquire();
	if (receivedHostHasPaused)
	{
		hasHostPaused = true;
	}
	receivedHostHasPaused = false;
	hostHasPausedQueueLock.release();
}

int receiveHostHasUnpaused(uint32_t playerID)
{
	hostHasUnpausedQueueLock.acquire();
	receivedHostHasUnpaused = true;
	hostHasUnpausedQueueLock.release();

	return 1;
}

void handleHostHasUnpaused()
{
	hostHasUnpausedQueueLock.acquire();
	if (receivedHostHasUnpaused)
	{
		hasHostPaused = false;
	}
	receivedHostHasUnpaused = false;
	hostHasUnpausedQueueLock.release();
}

int receiveKaelaOreAmount(uint32_t playerID)
{
	messageKaelaOreAmount curMessage = messageKaelaOreAmount();
	int curMessageLen = curMessage.receiveMessage(playerID);

	kaelaOreAmountQueueLock.acquire();
	kaelaOreAmountQueue.push(curMessage);
	kaelaOreAmountQueueLock.release();

	return curMessageLen;
}

void handleKaelaOreAmount()
{
	do
	{
		kaelaOreAmountQueueLock.acquire();
		if (kaelaOreAmountQueue.empty())
		{
			kaelaOreAmountQueueLock.release();
			break;
		}
		messageKaelaOreAmount curMessage = kaelaOreAmountQueue.front();
		kaelaOreAmountQueue.pop();
		kaelaOreAmountQueueLock.release();

		RValue scripts = getInstanceVariable(playerMap[clientID], GML_scripts);
		RValue materialGrind = getInstanceVariable(scripts, GML_MaterialGrind);
		RValue config = getInstanceVariable(materialGrind, GML_config);
		setInstanceVariable(config, GML_oreA, RValue(curMessage.oreA));
		setInstanceVariable(config, GML_oreB, RValue(curMessage.oreB));
		setInstanceVariable(config, GML_oreC, RValue(curMessage.oreC));
	} while (true);
}

int receiveMessage(uint32_t playerID)
{
	const int messageTypeLen = 1;
	char messageType[messageTypeLen];
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, messageType, messageTypeLen, false)) <= 0)
	{
		return result;
	}
	switch (messageType[0])
	{
		case MESSAGE_INPUT_AIM:
		{
			return receiveInputMessage(playerID, MESSAGE_INPUT_AIM);
		}
		case MESSAGE_INPUT_NO_AIM:
		{
			return receiveInputMessage(playerID, MESSAGE_INPUT_NO_AIM);
		}
		case MESSAGE_INPUT_MOUSEFOLLOW:
		{
			return receiveInputMessage(playerID, MESSAGE_INPUT_MOUSEFOLLOW);
		}
		case MESSAGE_ROOM:
		{
			return receiveRoomMessage(playerID);
		}
		case MESSAGE_INSTANCES_CREATE:
		{
			return receiveInstanceCreateMessage(playerID);
		}
		case MESSAGE_INSTANCES_UPDATE:
		{
			return receiveInstanceUpdateMessage(playerID);
		}
		case MESSAGE_INSTANCES_DELETE:
		{
			return receiveInstanceDeleteMessage(playerID);
		}
		case MESSAGE_CLIENT_PLAYER_DATA:
		{
			return receiveClientPlayerDataMessage(playerID);
		}
		case MESSAGE_ATTACK_CREATE:
		{
			return receiveAttackCreateMessage(playerID);
		}
		case MESSAGE_ATTACK_UPDATE:
		{
			return receiveAttackUpdateMessage(playerID);
		}
		case MESSAGE_ATTACK_DELETE:
		{
			return receiveAttackDeleteMessage(playerID);
		}
		case MESSAGE_CLIENT_ID:
		{
			return receiveClientIDMessage(playerID);
		}
		case MESSAGE_PICKUPABLE_CREATE:
		{
			return receivePickupableCreateMessage(playerID);
		}
		case MESSAGE_PICKUPABLE_UPDATE:
		{
			return receivePickupableUpdateMessage(playerID);
		}
		case MESSAGE_PICKUPABLE_DELETE:
		{
			return receivePickupableDeleteMessage(playerID);
		}
		case MESSAGE_GAME_DATA:
		{
			return receiveGameDataMessage(playerID);
		}
		case MESSAGE_LEVEL_UP_OPTIONS:
		{
			return receiveLevelUpOptionsMessage(playerID);
		}
		case MESSAGE_LEVEL_UP_CLIENT_CHOICE:
		{
			return receiveLevelUpClientChoiceMessage(playerID);
		}
		case MESSAGE_PING:
		{
			return receivePing(playerID);
		}
		case MESSAGE_PONG:
		{
			return receivePong(playerID);
		}
		case MESSAGE_DESTRUCTABLE_CREATE:
		{
			return receiveDestructableCreateMessage(playerID);
		}
		case MESSAGE_DESTRUCTABLE_BREAK:
		{
			return receiveDestructableBreakMessage(playerID);
		}
		case MESSAGE_ELIMINATE_LEVEL_UP_CLIENT_CHOICE:
		{
			return receiveEliminateLevelUpClientChoiceMessage(playerID);
		}
		case MESSAGE_CLIENT_SPECIAL_ATTACK:
		{
			return receiveClientSpecialAttackMessage(playerID);
		}
		case MESSAGE_CAUTION_CREATE:
		{
			return receiveCautionCreateMessage(playerID);
		}
		case MESSAGE_PRECREATE_UPDATE:
		{
			return receivePreCreateUpdateMessage(playerID);
		}
		case MESSAGE_VFX_UPDATE:
		{
			return receiveVFXUpdateMessage(playerID);
		}
		case MESSAGE_INTERACTABLE_CREATE:
		{
			return receiveInteractableCreateMessage(playerID);
		}
		case MESSAGE_INTERACTABLE_DELETE:
		{
			return receiveInteractableDeleteMessage(playerID);
		}
		case MESSAGE_INTERACTABLE_PLAYER_INTERACTED:
		{
			return receiveInteractablePlayerInteractedMessage(playerID);
		}
		case MESSAGE_STICKER_PLAYER_INTERACTED:
		{
			return receiveStickerPlayerInteractedMessage(playerID);
		}
		case MESSAGE_BOX_PLAYER_INTERACTED:
		{
			return receiveBoxPlayerInteractedMessage(playerID);
		}
		case MESSAGE_INTERACT_FINISHED:
		{
			return receiveInteractFinishedMessage(playerID);
		}
		case MESSAGE_BOX_TAKE_OPTION:
		{
			return receiveBoxTakeOptionMessage(playerID);
		}
		case MESSAGE_ANVIL_CHOOSE_OPTION:
		{
			return receiveAnvilChooseOptionMessage(playerID);
		}
		case MESSAGE_CLIENT_GAIN_MONEY:
		{
			return receiveClientGainMoneyMessage(playerID);
		}
		case MESSAGE_CLIENT_ANVIL_ENCHANT:
		{
			return receiveClientAnvilEnchantMessage(playerID);
		}
		case MESSAGE_STICKER_CHOOSE_OPTION:
		{
			return receiveStickerChooseOptionMessage(playerID);
		}
		case MESSAGE_CHOOSE_COLLAB:
		{
			return receiveChooseCollabMessage(playerID);
		}
		case MESSAGE_BUFF_DATA:
		{
			return receiveBuffDataMessage(playerID);
		}
		case MESSAGE_CHAR_DATA:
		{
			return receiveCharDataMessage(playerID);
		}
		case MESSAGE_RETURN_TO_LOBBY:
		{
			return receiveReturnToLobby(playerID);
		}
		case MESSAGE_LOBBY_PLAYER_DISCONNECTED:
		{
			return receiveLobbyPlayerDisconnected(playerID);
		}
		case MESSAGE_HOST_HAS_PAUSED:
		{
			return receiveHostHasPaused(playerID);
		}
		case MESSAGE_HOST_HAS_UNPAUSED:
		{
			return receiveHostHasUnpaused(playerID);
		}
		case MESSAGE_KAELA_ORE_AMOUNT:
		{
			return receiveKaelaOreAmount(playerID);
		}
	}
	g_ModuleInterface->Print(CM_RED, "Unknown message type received %d", messageType[0]);
	return -1;
}

int sendInputMessage(uint32_t playerID)
{
	if (!hasObtainedClientID || !isPlayerCreatedMap[clientID])
	{
		return -1;
	}

	if (isClientPaused || hasHostPaused)
	{
		return -1;
	}

	RValue isDownHeld;
	RValue isUpHeld;
	RValue isLeftHeld;
	RValue isRightHeld;

	getInputState("down", 0, isDownHeld);
	getInputState("up", 0, isUpHeld);
	getInputState("left", 0, isLeftHeld);
	getInputState("right", 0, isRightHeld);

	RValue returnVal;
	RValue** args = new RValue*[5];
	args[0] = new RValue();
	args[1] = new RValue("aim_left");
	args[2] = new RValue("aim_right");
	args[3] = new RValue("aim_up");
	args[4] = new RValue("aim_down");
	origInputDirectionScript(globalInstance, nullptr, returnVal, 5, args);

	char isDirHeld = (isDownHeld.ToBoolean() << 0) | (isUpHeld.ToBoolean() << 1) | (isLeftHeld.ToBoolean() << 2) | (isRightHeld.ToBoolean() << 3);

	if (returnVal.m_Kind == VALUE_UNDEFINED)
	{
		if (getInstanceVariable(playerMap[clientID], GML_mouseFollowMode).ToBoolean())
		{
			const int inputMessageLen = sizeof(messageInputMouseFollow) + 1;
			char inputMessage[inputMessageLen];
			RValue xPos = getInstanceVariable(playerMap[clientID], GML_x);
			RValue yPos = getInstanceVariable(playerMap[clientID], GML_y);
			RValue mouseX;
			RValue mouseY;
			g_ModuleInterface->GetBuiltin("mouse_x", nullptr, NULL_INDEX, mouseX);
			g_ModuleInterface->GetBuiltin("mouse_y", nullptr, NULL_INDEX, mouseY);
			mouseY.m_Real += 16;
			float direction = static_cast<float>(g_ModuleInterface->CallBuiltin("point_direction", { xPos, yPos, mouseX, mouseY }).ToDouble());
			messageInputMouseFollow sendMessage = messageInputMouseFollow(isDirHeld, direction);
			sendMessage.serialize(inputMessage);
			return sendBytesToPlayer(playerID, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_UDP);
		}
		else
		{
			const int inputMessageLen = sizeof(messageInputNoAim) + 1;
			char inputMessage[inputMessageLen];
			messageInputNoAim sendMessage = messageInputNoAim(isDirHeld, static_cast<float>(getInstanceVariable(playerMap[clientID], GML_direction).ToDouble()));
			sendMessage.serialize(inputMessage);
			return sendBytesToPlayer(playerID, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_UDP);
		}
	}
	else
	{
		const int inputMessageLen = sizeof(messageInputAim) + 1;
		char inputMessage[inputMessageLen];
		messageInputAim sendMessage = messageInputAim(isDirHeld, static_cast<float>(returnVal.ToDouble()));
		sendMessage.serialize(inputMessage);
		return sendBytesToPlayer(playerID, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_UDP);
	}
}

int sendAllRoomMessage()
{
	const int inputMessageLen = sizeof(messageRoom) + 1;
	char inputMessage[inputMessageLen];
	RValue room;
	g_ModuleInterface->GetBuiltin("room", nullptr, NULL_INDEX, room);
	char gameMode = static_cast<char>(lround(g_ModuleInterface->CallBuiltin("variable_global_get", { "gameMode" }).ToDouble()));
	messageRoom sendMessage = messageRoom(static_cast<char>(lround(room.ToDouble())), gameMode);
	sendMessage.serialize(inputMessage);

	for (auto& player : lobbyPlayerDataMap)
	{
		callbackManagerInterfacePtr->LogToFile(MODNAME, "player id %d char name %s", player.first, player.second.charName.data());
	}

	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_TCP);
	}
	return inputMessageLen;
}

int sendAllInstanceCreateMessage()
{
	if (instancesCreateMessage.numInstances == 0)
	{
		return -1;
	}
	const int inputMessageLen = sizeof(messageInstancesCreate) + 1;
	char inputMessage[inputMessageLen];
	instancesCreateMessage.serialize(inputMessage);

	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_TCP);
	}
	instancesCreateMessage.numInstances = 0;
	return inputMessageLen;
}

int sendAllInstanceUpdateMessage()
{
	if (instancesUpdateMessage.numInstances == 0)
	{
		return -1;
	}
	int inputMessageLen = static_cast<int>(instancesUpdateMessage.getMessageSize());
	char* inputMessage = new char[inputMessageLen];
	instancesUpdateMessage.serialize(inputMessage);

	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_UDP);
	}
	instancesUpdateMessage.numInstances = 0;
	delete[] inputMessage;
	return inputMessageLen;
}

int sendAllInstanceDeleteMessage()
{
	if (instancesDeleteMessage.numInstances == 0)
	{
		return -1;
	}
	const int inputMessageLen = sizeof(messageInstancesDelete) + 1;
	char inputMessage[inputMessageLen];
	instancesDeleteMessage.serialize(inputMessage);

	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_TCP);
	}
	instancesDeleteMessage.numInstances = 0;
	return inputMessageLen;
}

int sendAllClientPlayerDataMessage()
{
	const int inputMessageLen = sizeof(messageClientPlayerData) + 1;
	char inputMessage[inputMessageLen];

	for (auto& curPlayer : playerMap)
	{
		RValue curPlayerInstance = curPlayer.second;
		RValue doesPlayerExist = g_ModuleInterface->CallBuiltin("instance_exists", { curPlayerInstance });
		// TODO: probably should handle this better than just skipping it
		if (!doesPlayerExist.ToBoolean())
		{
			continue;
		}

		float xPos = static_cast<float>(getInstanceVariable(curPlayerInstance, GML_x).ToDouble());
		float yPos = static_cast<float>(getInstanceVariable(curPlayerInstance, GML_y).ToDouble());
		float imageXScale = static_cast<float>(getInstanceVariable(curPlayerInstance, GML_image_xscale).ToDouble());
		float imageYScale = static_cast<float>(getInstanceVariable(curPlayerInstance, GML_image_yscale).ToDouble());
		float direction = static_cast<float>(getInstanceVariable(curPlayerInstance, GML_direction).ToDouble());
		// Probably should change this to uint16_t
		short spriteIndex = static_cast<short>(lround(getInstanceVariable(curPlayerInstance, GML_sprite_index).ToDouble()));
		short curHP = static_cast<short>(lround(getInstanceVariable(curPlayerInstance, GML_currentHP).ToDouble()));
		short maxHP = static_cast<short>(lround(getInstanceVariable(curPlayerInstance, GML_HP).ToDouble()));
		float curAttack = static_cast<float>(getInstanceVariable(curPlayerInstance, GML_ATK).ToDouble());
		float curSpeed = static_cast<float>(getInstanceVariable(curPlayerInstance, GML_SPD).ToDouble());
		short curCrit = static_cast<short>(lround(getInstanceVariable(curPlayerInstance, GML_crit).ToDouble()));
		short curHaste = static_cast<short>(lround(getInstanceVariable(curPlayerInstance, GML_haste).ToDouble()));
		short curPickupRange = static_cast<short>(lround(getInstanceVariable(curPlayerInstance, GML_pickupRange).ToDouble()));
		short specialMeter = static_cast<short>(lround(getInstanceVariable(curPlayerInstance, GML_specialMeter).ToDouble()));
		// Not sure why the original player has a sprite index of -1 when it's created
		if (spriteIndex < 0)
		{
			spriteIndex = 0;
		}
		char truncatedImageIndex = static_cast<char>(getInstanceVariable(curPlayerInstance, GML_image_index).ToDouble());
		playerData curData(xPos, yPos, imageXScale, imageYScale, direction, spriteIndex, curHP, maxHP, curAttack, curSpeed, curCrit, curHaste, curPickupRange, specialMeter, truncatedImageIndex, curPlayer.first);
		messageClientPlayerData sendMessage = messageClientPlayerData(curData);
		sendMessage.serialize(inputMessage);
		for (auto& curClientIDMapping : clientIDToSteamIDMap)
		{
			sendBytesToPlayer(curClientIDMapping.first, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_UDP);
		}
	}

	return inputMessageLen;
}

int sendAllAttackCreateMessage()
{
	if (attackCreateMessage.numAttacks == 0)
	{
		return -1;
	}
	const int inputMessageLen = sizeof(messageAttackCreate) + 1;
	char inputMessage[inputMessageLen];
	attackCreateMessage.serialize(inputMessage);

	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_TCP);
	}
	attackCreateMessage.numAttacks = 0;
	return inputMessageLen;
}

int sendAllAttackUpdateMessage()
{
	if (attackUpdateMessage.numAttacks == 0)
	{
		return -1;
	}
	int inputMessageLen = static_cast<int>(attackUpdateMessage.getMessageSize());
	char* inputMessage = new char[inputMessageLen];
	attackUpdateMessage.serialize(inputMessage);

	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_UDP);
	}
	attackUpdateMessage.numAttacks = 0;
	delete[] inputMessage;
	return inputMessageLen;
}

int sendAllAttackDeleteMessage()
{
	if (attackDeleteMessage.numAttacks == 0)
	{
		return -1;
	}
	const int inputMessageLen = sizeof(messageAttackDelete) + 1;
	char inputMessage[inputMessageLen];
	attackDeleteMessage.serialize(inputMessage);

	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_TCP);
	}
	attackDeleteMessage.numAttacks = 0;
	return inputMessageLen;
}

int sendClientIDMessage(uint32_t playerID)
{
	const int inputMessageLen = sizeof(messageClientID) + 1;
	char inputMessage[inputMessageLen];
	messageClientID clientNumberID = messageClientID(playerID);
	clientNumberID.serialize(inputMessage);

	return sendBytesToPlayer(playerID, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_TCP);
}

int sendAllPickupableCreateMessage(pickupableData data)
{
	const int inputMessageLen = sizeof(messagePickupableCreate) + 1;
	char inputMessage[inputMessageLen];
	messagePickupableCreate curMessage = messagePickupableCreate(data);
	curMessage.serialize(inputMessage);

	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_TCP);
	}
	return inputMessageLen;
}

int sendAllPickupableUpdateMessage(pickupableData data)
{
	const int inputMessageLen = sizeof(messagePickupableUpdate) + 1;
	char inputMessage[inputMessageLen];
	messagePickupableUpdate curMessage = messagePickupableUpdate(data);
	curMessage.serialize(inputMessage);

	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_UDP);
	}
	return inputMessageLen;
}

int sendAllPickupableDeleteMessage(short pickupableID)
{
	const int inputMessageLen = sizeof(messagePickupableDelete) + 1;
	char inputMessage[inputMessageLen];
	messagePickupableDelete curMessage = messagePickupableDelete(pickupableID);
	curMessage.serialize(inputMessage);

	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_TCP);
	}
	return inputMessageLen;
}

int sendAllGameDataMessage()
{
	uint32_t coinCount = static_cast<uint32_t>(g_ModuleInterface->CallBuiltin("variable_global_get", { "currentRunMoneyGained" }).ToDouble());
	uint32_t enemyDefeated = static_cast<uint32_t>(g_ModuleInterface->CallBuiltin("variable_global_get", { "enemyDefeated" }).ToDouble());

	RValue timeArr = g_ModuleInterface->CallBuiltin("variable_global_get", { "time" });
	timeNum = static_cast<uint32_t>(lround(timeArr[0].ToDouble()));
	timeNum *= 60;
	timeNum += static_cast<uint32_t>(lround(timeArr[1].ToDouble()));
	timeNum *= 60;
	timeNum += static_cast<uint32_t>(lround(timeArr[2].ToDouble()));
	timeNum *= 60;
	timeNum += static_cast<uint32_t>(lround(timeArr[3].ToDouble()));

	short playerLevel = static_cast<short>(lround(g_ModuleInterface->CallBuiltin("variable_global_get", { "PLAYERLEVEL" }).ToDouble()));
	float experience = static_cast<float>(g_ModuleInterface->CallBuiltin("variable_global_get", { "experience" }).ToDouble());
	float toNextLevel = 0;
	if (playerManagerInstanceVar != nullptr)
	{
		toNextLevel = static_cast<float>(getInstanceVariable(playerManagerInstanceVar, GML_toNextLevel).ToDouble());
	}

	short goldenHammer = static_cast<short>(lround(g_ModuleInterface->CallBuiltin("variable_global_get", { "goldenHammer" }).ToDouble()));
	short goldenHammerPieces = static_cast<short>(lround(g_ModuleInterface->CallBuiltin("variable_global_get", { "goldenHammerPieces" }).ToDouble()));

	messageGameData data(timeNum, coinCount, enemyDefeated, experience, toNextLevel, static_cast<float>(moneyGainMultiplier), static_cast<float>(foodMultiplier), playerLevel, goldenHammer, goldenHammerPieces);
	const int inputMessageLen = sizeof(messageGameData) + 1;
	char inputMessage[inputMessageLen];
	data.serialize(inputMessage);

	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, inputMessage, inputMessageLen, NETWORKING_MESSAGE_USING_TCP);
	}
	return inputMessageLen;
}

int sendClientLevelUpOptionsMessage(uint32_t playerID)
{
	RValue playerManager = g_ModuleInterface->CallBuiltin("instance_find", { objPlayerManagerIndex, 0 });
	RValue options = getInstanceVariable(playerManager, GML_options);
	levelUpOption optionArr[4];
	for (int i = 0; i < 4; i++)
	{
		RValue optionIcon = getInstanceVariable(options[i], GML_optionIcon);
		RValue optionType = getInstanceVariable(options[i], GML_optionType);
		RValue optionName = getInstanceVariable(options[i], GML_optionName);
		RValue optionID = getInstanceVariable(options[i], GML_optionID);
		RValue optionDescription = getInstanceVariable(options[i], GML_optionDescription);
		RValue offeredMod = getInstanceVariable(options[i], GML_offeredMod);
		RValue gainedMods = getInstanceVariable(options[i], GML_gainedMods);
		char weaponAndItemType = 4;
		if (optionType.ToString().compare("Weapon") == 0)
		{
			std::string_view weaponType = getInstanceVariable(options[i], GML_weaponType).ToString();
			if (weaponType.compare("Melee") == 0)
			{
				weaponAndItemType = 0;
			}
			else if (weaponType.compare("Ranged") == 0)
			{
				weaponAndItemType = 1;
			}
			else if (weaponType.compare("MultiShot") == 0)
			{
				weaponAndItemType = 2;
			}
		}
		else if (optionType.ToString().compare("Item") == 0)
		{
			std::string_view itemType = getInstanceVariable(options[i], GML_itemType).ToString();
			if (itemType.compare("Healing") == 0)
			{
				weaponAndItemType = 0;
			}
			else if (itemType.compare("Stat") == 0)
			{
				weaponAndItemType = 1;
			}
			else if (itemType.compare("Utility") == 0)
			{
				weaponAndItemType = 2;
			}
		}
		std::vector<std::string> optionDescriptionList;
		if (optionDescription.m_Kind == VALUE_STRING)
		{
			optionDescriptionList.push_back(optionDescription.ToString());
		}
		else if (optionDescription.m_Kind == VALUE_ARRAY)
		{
			int optionDescriptionLength = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { optionDescription }).ToDouble()));
			for (int j = 0; j < optionDescriptionLength; j++)
			{
				optionDescriptionList.push_back(optionDescription[j].ToString());
			}
		}
		else
		{
			g_ModuleInterface->Print(CM_RED, "UNEXPECTED TYPE WHEN SENDING OVER OPTION DESCRIPTION");
		}
		int gainedModsLength = -1;
		if (gainedMods.m_Kind == VALUE_ARRAY && (gainedModsLength = static_cast<int>(lround(g_ModuleInterface->CallBuiltin("array_length", { gainedMods }).ToDouble()))) > 0)
		{
			std::vector<std::string> modsList;
			for (int j = 0; j < gainedModsLength; j++)
			{
				modsList.push_back(gainedMods[j].ToString());
			}
			optionArr[i] = levelUpOption(optionType.ToString(), optionName.ToString(), optionID.ToString(), optionDescriptionList, static_cast<uint16_t>(optionIcon.ToDouble()), 0, weaponAndItemType, modsList, 0);
		}
		else if (offeredMod.m_Kind == VALUE_STRING)
		{
			std::vector<std::string> modsList;
			modsList.push_back(offeredMod.ToString());
			optionArr[i] = levelUpOption(optionType.ToString(), optionName.ToString(), optionID.ToString(), optionDescriptionList, static_cast<uint16_t>(optionIcon.ToDouble()), 0, weaponAndItemType, modsList, 1);
		}
		else
		{
			optionArr[i] = levelUpOption(optionType.ToString(), optionName.ToString(), optionID.ToString(), optionDescriptionList, static_cast<uint16_t>(optionIcon.ToDouble()), 0, weaponAndItemType);
		}
	}
	messageLevelUpOptions clientNumberMessage = messageLevelUpOptions(optionArr);

	size_t inputMessageLen = clientNumberMessage.getMessageSize();
	char* inputMessage = new char[inputMessageLen];


	clientNumberMessage.serialize(inputMessage);
	int sentLen = sendBytesToPlayer(playerID, inputMessage, static_cast<int>(inputMessageLen), NETWORKING_MESSAGE_USING_TCP);

	delete[] inputMessage;

	return sentLen;
}

int sendLevelUpClientChoiceMessage(uint32_t playerID, char levelUpOption)
{
	messageLevelUpClientChoice curMessage = messageLevelUpClientChoice(levelUpOption);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	delete[] messageBuffer;
	return sentLen;
}

int sendPing(uint32_t playerID)
{
	messagePing curMessage = messagePing();
	const int messageBufferLen = 1;
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_UDP);

	startPingTime = std::chrono::high_resolution_clock::now();
	return sentLen;
}

int sendPong(uint32_t playerID)
{
	messagePong curMessage = messagePong();
	const int messageBufferLen = 1;
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_UDP);
	return sentLen;
}

int sendAllDestructableCreateMessage(destructableData data)
{
	messageDestructableCreate curMessage = messageDestructableCreate(data);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	}

	delete[] messageBuffer;
	return messageBufferLen;
}

int sendAllDestructableBreakMessage(destructableData data)
{
	messageDestructableBreak curMessage = messageDestructableBreak(data);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	}

	delete[] messageBuffer;
	return messageBufferLen;
}

int sendEliminateLevelUpClientChoiceMessage(uint32_t playerID, char levelUpOption)
{
	messageEliminateLevelUpClientChoice curMessage = messageEliminateLevelUpClientChoice(levelUpOption);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	delete[] messageBuffer;
	return sentLen;
}

int sendClientSpecialAttackMessage(uint32_t playerID)
{
	messageClientSpecialAttack curMessage = messageClientSpecialAttack();
	const int messageBufferLen = 1;
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	return sentLen;
}

int sendAllCautionCreateMessage(cautionData data)
{
	messageCautionCreate curMessage = messageCautionCreate(data);
	const int messageBufferLen = static_cast<int>(sizeof(messageCautionCreate) + 1);
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	}
	return messageBufferLen;
}

int sendAllPreCreateUpdateMessage(preCreateData data)
{
	messagePreCreateUpdate curMessage = messagePreCreateUpdate(data);
	const int messageBufferLen = static_cast<int>(sizeof(messagePreCreateUpdate) + 1);
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_UDP);
	}
	return messageBufferLen;
}

int sendAllVFXUpdateMessage(vfxData data)
{
	messageVfxUpdate curMessage = messageVfxUpdate(data);
	const int messageBufferLen = static_cast<int>(sizeof(messageVfxUpdate) + 1);
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_UDP);
	}
	return messageBufferLen;
}

int sendAllInteractableCreateMessage(interactableData data)
{
	messageInteractableCreate curMessage = messageInteractableCreate(data);
	const int messageBufferLen = static_cast<int>(sizeof(messageInteractableCreate) + 1);
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	}
	return messageBufferLen;
}

int sendAllInteractableDeleteMessage(short id, char type)
{
	messageInteractableDelete curMessage = messageInteractableDelete(id, type);
	const int messageBufferLen = static_cast<int>(sizeof(messageInteractableDelete) + 1);
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	}
	return messageBufferLen;
}

int sendAllInteractablePlayerInteractedMessage(uint32_t playerID, short id, char type)
{
	messageInteractablePlayerInteracted curMessage = messageInteractablePlayerInteracted(playerID, id, type);
	const int messageBufferLen = static_cast<int>(sizeof(messageInteractablePlayerInteracted) + 1);
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	}
	return messageBufferLen;
}

int sendAllStickerPlayerInteractedMessage(uint32_t playerID, std::string stickerID, short id)
{
	messageStickerPlayerInteracted curMessage = messageStickerPlayerInteracted(stickerID, playerID, id);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	}
	delete[] messageBuffer;
	return messageBufferLen;
}

int sendAllBoxPlayerInteractedMessage(uint32_t playerID, levelUpOption* levelUpOptionArr, short id, char boxItemAmount, char isSuperBox)
{
	messageBoxPlayerInteracted curMessage = messageBoxPlayerInteracted(levelUpOptionArr, playerID, id, boxItemAmount, isSuperBox);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	}
	delete[] messageBuffer;
	return messageBufferLen;
}

int sendInteractFinishedMessage(uint32_t playerID)
{
	messageInteractFinished curMessage = messageInteractFinished();
	const int messageBufferLen = 1;
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	return sentLen;
}

int sendBoxTakeOptionMessage(uint32_t playerID, char boxItemNum)
{
	messageBoxTakeOption curMessage = messageBoxTakeOption(boxItemNum);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	delete[] messageBuffer;
	return sentLen;
}

int sendAnvilChooseOptionMessage(uint32_t playerID, std::string optionID, std::string optionType, uint32_t coinCost, char anvilOptionType)
{
	messageAnvilChooseOption curMessage = messageAnvilChooseOption(optionID, optionType, coinCost, anvilOptionType);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	delete[] messageBuffer;
	return sentLen;
}

int sendClientGainMoneyMessage(uint32_t playerID, uint32_t money)
{
	messageClientGainMoney curMessage = messageClientGainMoney(money);
	const int messageBufferLen = static_cast<int>(sizeof(messageClientGainMoney) + 1);
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	return sentLen;
}

int sendClientAnvilEnchantMessage(uint32_t playerID, std::string optionID, std::vector<std::string> gainedMods, uint32_t coinCost)
{
	messageClientAnvilEnchant curMessage = messageClientAnvilEnchant(optionID, gainedMods, coinCost);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	delete[] messageBuffer;
	return sentLen;
}

int sendStickerChooseOptionMessage(uint32_t playerID, char stickerOption, char stickerOptionType)
{
	messageStickerChooseOption curMessage = messageStickerChooseOption(stickerOption, stickerOptionType);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	delete[] messageBuffer;
	return sentLen;
}

int sendChooseCollabMessage(uint32_t playerID, levelUpOption collab)
{
	messageChooseCollab curMessage = messageChooseCollab(collab);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	delete[] messageBuffer;
	return sentLen;
}

int sendBuffDataMessage(uint32_t playerID, std::vector<buffData> buffDataList)
{
	messageBuffData curMessage = messageBuffData(buffDataList);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	delete[] messageBuffer;
	return sentLen;
}

int sendCharDataMessage(uint32_t playerID, lobbyPlayerData playerData, uint32_t charPlayerID)
{
	messageCharData curMessage = messageCharData(playerData, charPlayerID);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	delete[] messageBuffer;
	return sentLen;
}

int sendAllReturnToLobbyMessage()
{
	messageReturnToLobby curMessage = messageReturnToLobby();
	const int messageBufferLen = 1;
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	}

	return messageBufferLen;
}

int sendAllLobbyPlayerDisconnectedMessage(uint32_t playerID)
{
	messageLobbyPlayerDisconnected curMessage = messageLobbyPlayerDisconnected();
	const int messageBufferLen = 4;
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	}

	return messageBufferLen;
}

int sendAllHostHasPausedMessage()
{
	messageHostHasPaused curMessage = messageHostHasPaused();
	const int messageBufferLen = 1;
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	}

	return messageBufferLen;
}

int sendAllHostHasUnpausedMessage()
{
	messageHostHasUnpaused curMessage = messageHostHasUnpaused();
	const int messageBufferLen = 1;
	char messageBuffer[messageBufferLen];
	curMessage.serialize(messageBuffer);
	// TODO: Should probably do something to check if it's unable to send to only some sockets
	for (auto& curClientIDMapping : clientIDToSteamIDMap)
	{
		sendBytesToPlayer(curClientIDMapping.first, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	}

	return messageBufferLen;
}

int sendKaelaOreAmountMessage(uint32_t playerID, short oreA, short oreB, short oreC)
{
	messageKaelaOreAmount curMessage = messageKaelaOreAmount(oreA, oreB, oreC);
	int messageBufferLen = static_cast<int>(curMessage.getMessageSize());
	char* messageBuffer = new char[messageBufferLen];
	curMessage.serialize(messageBuffer);
	int sentLen = sendBytesToPlayer(playerID, messageBuffer, messageBufferLen, NETWORKING_MESSAGE_USING_TCP);
	delete[] messageBuffer;
	return sentLen;
}