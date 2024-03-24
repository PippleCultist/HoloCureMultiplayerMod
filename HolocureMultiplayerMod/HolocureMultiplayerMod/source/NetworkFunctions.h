#pragma once
#include <queue>
#include "ModuleMain.h"
#include "MessageStructs.h"
#include "ScriptFunctions.h"

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