#pragma once
#include <queue>
#include "ModuleMain.h"
#include "MessageStructs.h"
#include "ScriptFunctions.h"


struct clientMovementData
{
	MessageTypes messageType;
	float direction;
	bool isPlayerMoving;
	bool isDownHeld;
	bool isUpHeld;
	bool isLeftHeld;
	bool isRightHeld;

	clientMovementData() : messageType(MESSAGE_INVALID), direction(0), isPlayerMoving(false), isDownHeld(false), isUpHeld(false), isLeftHeld(false), isRightHeld(false)
	{
	}

	clientMovementData(MessageTypes messageType, float direction, bool isPlayerMoving, bool isDownHeld, bool isUpHeld, bool isLeftHeld, bool isRightHeld) :
		messageType(messageType), direction(direction), isPlayerMoving(isPlayerMoving), isDownHeld(isDownHeld), isUpHeld(isUpHeld), isLeftHeld(isLeftHeld), isRightHeld(isRightHeld)
	{
	}
};

struct messageRoom
{
	char roomNum;
	char gameMode;

	messageRoom() : roomNum(0), gameMode(0)
	{
	}

	messageRoom(char roomNum, char gameMode) : roomNum(roomNum), gameMode(gameMode)
	{
	}

	messageRoom(char* messageBuffer)
	{
		int startBufferPos = 0;
		readByteBufferToChar(&roomNum, messageBuffer, startBufferPos);
		readByteBufferToChar(&gameMode, messageBuffer, startBufferPos);
	}

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_ROOM, startBufferPos);
		writeCharToByteBuffer(messageBuffer, roomNum, startBufferPos);
		writeCharToByteBuffer(messageBuffer, gameMode, startBufferPos);
	}
};

struct clientMovementQueueData
{
	std::queue<clientMovementData> data;
	uint32_t lastTimeNumUpdated;
	int numEarlyUpdates;

	clientMovementQueueData() : lastTimeNumUpdated(0), numEarlyUpdates(0)
	{
	}

	clientMovementQueueData(uint32_t lastTimeNumUpdated, int numEarlyUpdates) : lastTimeNumUpdated(lastTimeNumUpdated), numEarlyUpdates(numEarlyUpdates)
	{
	}
};

// TODO: Fix padding causing these structs to be larger than necessary
struct messageClientPlayerData
{
	playerData data;

	messageClientPlayerData(playerData data) : data(data)
	{
	}

	messageClientPlayerData(char* messageBuffer)
	{
		int startBufferPos = 0;
		readByteBufferToLong(&data.playerID, messageBuffer, startBufferPos);
		readByteBufferToFloat(&data.xPos, messageBuffer, startBufferPos);
		readByteBufferToFloat(&data.yPos, messageBuffer, startBufferPos);
		readByteBufferToFloat(&data.imageXScale, messageBuffer, startBufferPos);
		readByteBufferToFloat(&data.imageYScale, messageBuffer, startBufferPos);
		readByteBufferToFloat(&data.direction, messageBuffer, startBufferPos);
		readByteBufferToFloat(&data.curAttack, messageBuffer, startBufferPos);
		readByteBufferToFloat(&data.curSpeed, messageBuffer, startBufferPos);
		readByteBufferToShort(&data.spriteIndex, messageBuffer, startBufferPos);
		readByteBufferToShort(&data.curHP, messageBuffer, startBufferPos);
		readByteBufferToShort(&data.maxHP, messageBuffer, startBufferPos);
		readByteBufferToShort(&data.curCrit, messageBuffer, startBufferPos);
		readByteBufferToShort(&data.curHaste, messageBuffer, startBufferPos);
		readByteBufferToShort(&data.curPickupRange, messageBuffer, startBufferPos);
		readByteBufferToShort(&data.specialMeter, messageBuffer, startBufferPos);
		readByteBufferToChar(&data.truncatedImageIndex, messageBuffer, startBufferPos);
	}

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_CLIENT_PLAYER_DATA, startBufferPos);
		writeLongToByteBuffer(messageBuffer, data.playerID, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.xPos, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.yPos, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.imageXScale, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.imageYScale, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.direction, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.curAttack, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.curSpeed, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.spriteIndex, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.curHP, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.maxHP, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.curCrit, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.curHaste, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.curPickupRange, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.specialMeter, startBufferPos);
		writeCharToByteBuffer(messageBuffer, data.truncatedImageIndex, startBufferPos);
	}
};

struct levelUpPausedData
{
	uint32_t playerID;
	optionType levelUpType;
	RValue levelUpName;
	// TODO: Check if this doesn't let the string_views get freed
	std::vector<std::string_view> gainedMods;

	levelUpPausedData(uint32_t playerID, optionType levelUpType, RValue levelUpName) : playerID(playerID), levelUpType(levelUpType), levelUpName(levelUpName)
	{
	}

	levelUpPausedData(uint32_t playerID, optionType levelUpType, RValue levelUpName, std::vector<std::string_view> gainedMods) : playerID(playerID), levelUpType(levelUpType), levelUpName(levelUpName), gainedMods(gainedMods)
	{
	}
};

void clientReceiveMessageHandler();
void hostReceiveMessageHandler();

void processLevelUp(levelUpPausedData& levelUpData, CInstance* playerManagerInstance);

const int maxNumAvailableInstanceIDs = 10000;
const int maxNumAvailableAttackIDs = 10000;
const int maxNumAvailablePickupableIDs = 10000;
const int maxNumAvailablePreCreateIDs = 1000;
const int maxNumAvailableVFXIDs = 10000;
const int maxNumAvailableInteractableIDs = 1000;
extern std::queue<uint16_t> availableInstanceIDs;
extern std::queue<uint16_t> availableAttackIDs;
extern std::queue<uint16_t> availablePickupableIDs;
extern std::queue<uint16_t> availablePreCreateIDs;
extern std::queue<uint16_t> availableVFXIDs;
extern std::queue<uint16_t> availableInteractableIDs;
extern std::unordered_map<uint32_t, SOCKET> clientSocketMap;
extern std::unordered_map<uint32_t, playerData> playerDataMap;
extern std::unordered_map<uint32_t, bool> isPlayerCreatedMap;
extern std::vector<levelUpPausedData> levelUpPausedList;
extern std::vector<std::string_view> currentAnvilRolledMods;
extern std::unordered_map<short, destructableData> destructableMap;

extern float clientCamPosX;
extern float clientCamPosY;

extern int clientID;
extern uint32_t timeNum;

extern messageInstancesCreate instancesCreateMessage;
extern messageInstancesUpdate instancesUpdateMessage;
extern messageInstancesDelete instancesDeleteMessage;
extern messageAttackCreate attackCreateMessage;
extern messageAttackUpdate attackUpdateMessage;
extern messageAttackDelete attackDeleteMessage;

extern bool isClientUsingBox;
extern bool isClientUsingAnvil;
extern bool isClientUsingGoldenAnvil;
extern bool isClientUsingStamp;

extern bool hasObtainedClientID;

void handleInputMessage(CInstance* Self);
void handleRoomMessage();
void handleInstanceCreateMessage();
void handleInstanceUpdateMessage();
void handleInstanceDeleteMessage();
void handleClientPlayerDataMessage();
void handleAttackCreateMessage();
void handleAttackUpdateMessage();
void handleAttackDeleteMessage();
void handleClientIDMessage();
void handlePickupableCreateMessage();
void handlePickupableUpdateMessage();
void handlePickupableDeleteMessage();
void handleGameDataMessage();
void handleLevelUpOptionsMessage(CInstance* playerManager);
void handleLevelUpClientChoiceMessage();
void handleDestructableCreateMessage();
void handleDestructableBreakMessage();
void handleEliminateLevelUpClientChoiceMessage();
void handleClientSpecialAttackMessage();
void handleCautionCreateMessage();
void handlePreCreateUpdateMessage();
void handleVFXUpdateMessage();
void handleInteractableCreateMessage();
void handleInteractableDeleteMessage();
void handleInteractablePlayerInteractedMessage();
void handleStickerPlayerInteractedMessage();
void handleBoxPlayerInteractedMessage();
void handleInteractFinishedMessage();
void handleBoxTakeOptionMessage();
void handleAnvilChooseOptionMessage();
void handleClientGainMoneyMessage();
void handleClientAnvilEnchantMessage();
void handleStickerChooseOptionMessage();
void handleChooseCollabMessage();
void handleBuffDataMessage();
void handleCharDataMessage();
void handleReturnToLobby();
void handleLobbyPlayerDisconnected();
void handleHostHasPaused();
void handleHostHasUnpaused();
void handleKaelaOreAmount();
int receiveBytes(SOCKET socket, char* outputBuffer, int length, bool loopUntilDone = true);
int sendBytes(SOCKET socket, char* outputBuffer, int length, bool loopUntilDone = true);
int receiveMessage(SOCKET socket, uint32_t playerID = 0);
int sendInputMessage(SOCKET socket);
int sendAllRoomMessage();
int sendAllInstanceCreateMessage();
int sendAllInstanceUpdateMessage();
int sendAllInstanceDeleteMessage();
int sendAllClientPlayerDataMessage();
int sendAllAttackCreateMessage();
int sendAllAttackUpdateMessage();
int sendAllAttackDeleteMessage();
int sendClientIDMessage(SOCKET socket, uint32_t playerID);
int sendAllPickupableCreateMessage(pickupableData data);
int sendAllPickupableUpdateMessage(pickupableData data);
int sendAllPickupableDeleteMessage(short pickupableID);
int sendAllGameDataMessage();
int sendClientLevelUpOptionsMessage(SOCKET socket, uint32_t playerID);
int sendLevelUpClientChoiceMessage(SOCKET socket, char levelUpOption);
int sendPing(SOCKET socket);
int sendPong(SOCKET socket);
int sendAllDestructableCreateMessage(destructableData data);
int sendAllDestructableBreakMessage(destructableData data);
int sendEliminateLevelUpClientChoiceMessage(SOCKET socket, char levelUpOption);
int sendClientSpecialAttackMessage(SOCKET socket);
int sendAllCautionCreateMessage(cautionData data);
int sendAllPreCreateUpdateMessage(preCreateData data);
int sendAllVFXUpdateMessage(vfxData data);
int sendAllInteractableCreateMessage(interactableData data);
int sendAllInteractableDeleteMessage(short id, char type);
int sendAllInteractablePlayerInteractedMessage(uint32_t playerID, short id, char type);
int sendAllStickerPlayerInteractedMessage(uint32_t playerID, std::string_view stickerID, short id);
int sendAllBoxPlayerInteractedMessage(uint32_t playerID, levelUpOption* levelUpOptionArr, short id, char boxItemAmount, char isSuperBox);
int sendInteractFinishedMessage(SOCKET socket);
int sendBoxTakeOptionMessage(SOCKET socket, char boxItemNum);
int sendAnvilChooseOptionMessage(SOCKET socket, std::string_view optionID, std::string_view optionType, uint32_t coinCost, char anvilOptionType);
int sendClientGainMoneyMessage(SOCKET socket, uint32_t money);
int sendClientAnvilEnchantMessage(SOCKET socket, std::string_view optionID, std::vector<std::string_view> gainedMods, uint32_t coinCost);
int sendStickerChooseOptionMessage(SOCKET socket, char stickerOption, char stickerOptionType);
int sendChooseCollabMessage(SOCKET socket, levelUpOption collab);
int sendBuffDataMessage(SOCKET socket, std::vector<buffData> buffDataList);
int sendCharDataMessage(SOCKET socket, lobbyPlayerData playerData, uint32_t playerID);
int sendAllReturnToLobbyMessage();
int sendAllLobbyPlayerDisconnectedMessage(uint32_t playerID);
int sendAllHostHasPausedMessage();
int sendAllHostHasUnpausedMessage();
int sendKaelaOreAmountMessage(SOCKET socket, short oreA, short oreB, short oreC);