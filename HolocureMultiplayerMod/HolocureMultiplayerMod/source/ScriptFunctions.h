#pragma once
#include <YYToolkit/shared.hpp>
#include "ModuleMain.h"

struct lobbyPlayerData;
extern bool isClientPaused;
extern std::unordered_map<uint32_t, RValue> playerMap;
extern std::unordered_map<uint32_t, RValue> playerWeaponMap;
extern std::unordered_map<uint32_t, RValue> playerPerksMap;
extern std::unordered_map<uint32_t, RValue> playerItemsMapMap;
extern std::unordered_map<uint32_t, RValue> playerItemsMap;
extern std::unordered_map<uint32_t, RValue> playerAttackIndexMapMap;
extern std::unordered_map<uint32_t, RValue> currentStickersMap;
extern std::unordered_map<uint32_t, RValue> summonMap;
extern std::unordered_map<uint32_t, int> playerPingMap;
extern std::unordered_map<uint32_t, lobbyPlayerData> lobbyPlayerDataMap;
extern std::unordered_map<uint32_t, bool> clientUnpausedMap;
extern int curPlayerID;

enum optionType
{
	optionType_StatUp = 0,
	optionType_Weapon,
	optionType_RemoveWeapon,
	optionType_RemoveItem,
	optionType_RemoveSkill,
	optionType_Enchant,
	optionType_Collab,
	optionType_SuperCollab,
	optionType_Skill,
	optionType_Item,
	optionType_Consumable,
	optionType_NONE
};

struct levelUpOptionNames
{
	std::pair<optionType, std::string> optionArr[4];

	levelUpOptionNames()
	{
	}

	levelUpOptionNames(std::pair<optionType, std::string> optionOne, std::pair<optionType, std::string> optionTwo, std::pair<optionType, std::string> optionThree, std::pair<optionType, std::string> optionFour)
	{
		optionArr[0] = optionOne;
		optionArr[1] = optionTwo;
		optionArr[2] = optionThree;
		optionArr[3] = optionFour;
	}
};

void swapPlayerData(CInstance* playerManagerInstance, RValue attackController, uint32_t playerIndex);
optionType convertStringOptionTypeToEnum(RValue optionType);

extern std::unordered_map<uint32_t, levelUpOptionNames> levelUpOptionNamesMap;
extern bool isHostWaitingForClientUnpause;
extern bool isInGamemodeSelect;
extern bool isInNetworkAdapterMenu;
extern bool hasReadNetworkAdapterDisclaimer;
extern bool isInCoopOptionMenu;
extern bool isInLobby;
extern bool isSelectingCharacter;
extern bool isSelectingMap;

extern SOCKET listenSocket;

extern double moneyGainMultiplier;

extern bool isInBaseMobOnDeath;

extern SOCKET broadcastSocket;
extern sockaddr* broadcastSocketAddr;
extern size_t broadcastSocketLen;

extern bool isClientInInitializeCharacter;

void cleanupPlayerGameData();
void cleanupPlayerClientData();
void unsetPauseMenu();
void unpauseHost();

RValue& InitializeCharacterPlayerManagerCreateFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& InitializeCharacterPlayerManagerCreateFuncAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& CanSubmitScoreFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& StopPlayerCreateFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& InitPlayerManagerCreateFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& InitPlayerManagerCreateFuncAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& MovePlayerCreateFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& PausePlayerManagerCreateFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& PausePlayerManagerCreateFuncAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& CreateTakodachiAttackControllerOther14FuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& GLRMeshDestroyFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& LevelUpPlayerManagerFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& LevelUpPlayerManagerFuncAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ConfirmedPlayerManagerFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& UnpausePlayerManagerFuncBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ParseAndPushCommandTypePlayerManagerFuncAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ExecuteAttackBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ExecuteAttackAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& OnCollideWithTargetAttackBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& DieObstacleCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ExecuteSpecialAttackBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ApplyBuffAttackControllerBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ApplyBuffAttackControllerAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& DestroyHoloAnvilBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& DestroyGoldenAnvilBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& RollStickerAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& DestroyStickerBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& TakeDamageBaseMobCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& TakeDamageBaseMobCreateAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& RollModAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ApplyBuffsPlayerManagerBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ConfirmedTitleScreenBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ReturnMenuTitleScreenBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ReturnCharSelectCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& SelectCharSelectCreateAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& OnDeathBaseMobCreateBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& OnDeathBaseMobCreateAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& UpdatePlayerPlayerManagerOtherBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& AddPerkPlayerManagerOtherAfter(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);
RValue& ParseAndPushCommandTypePlayerManagerOtherBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int numArgs, RValue** Args);