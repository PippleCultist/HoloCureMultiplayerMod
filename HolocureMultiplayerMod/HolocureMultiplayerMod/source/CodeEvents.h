#pragma once
#include <YYToolkit/shared.hpp>
#include "CallbackManager/CallbackManagerInterface.h"
#include "MessageStructs.h"

inline uint32_t getPlayerID(CInstance* currentPlayerPtr);

extern std::unordered_map<CInstance*, instanceData> instanceToIDMap;
extern std::unordered_map<CInstance*, uint16_t> pickupableToIDMap;
extern std::unordered_map<short, RValue> preCreateMap;
extern std::unordered_map<short, RValue> vfxMap;
extern std::unordered_map<short, RValue> interactableMap;
extern CInstance* playerManagerInstanceVar;
extern bool hasClientFinishedInteracting;
extern bool isAnyInteracting;
extern std::string collidedStickerID;
extern bool hasSelectedMap;
extern int curSelectedMap;
extern int curSelectedGameMode;
extern int curSelectedStageSprite;

extern bool hasSent;
extern bool hasHostPaused;

extern bool isInOreDepositStep;

extern std::unordered_map<uint32_t, bool> hasClientPlayerDisconnected;

extern std::unordered_map<ULONG, SOCKET> listenForClientSocketMap;

extern std::unordered_map<short, std::pair<uint32_t, int>> clientPickupableImageIndexMap;

extern levelUpOption randomWeaponArr[3];

extern uint32_t curUnusedPlayerID;

void clientLeaveGame(bool isHostDisconnected);

void InputManagerCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerMouse53After(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerDrawBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerDrawAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void InputControllerObjectStep1Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void EnemyStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerManagerCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerManagerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerManagerStepAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerManagerCleanUpBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void StageManagerCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void StageManagerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void CrateSpawnerCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void CrateSpawnerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void CamStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void CloudMakerAlarmBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void BaseMobCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AttackCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AttackStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AttackStepAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AttackCollisionBaseMobBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AttackCollisionObstacleBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AttackCollisionAttackBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AttackCleanupBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AttackDestroyBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerStepAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerManagerDraw64Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerManagerDraw64After(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void EXPCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AllPickupableStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AllPickupableCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AllPickupableCollisionSummonBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void EXPCollisionEXPBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void YagooPillarCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void DestructableCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void ObstacleStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void YagooPillarStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AttackControllerCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void CautionStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void CautionAttackStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PreCreateStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void VFXStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AfterImageStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AfterImageAlarmBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void HoloBoxCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void HoloBoxCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void HoloAnvilCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void HoloAnvilCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void GoldenAnvilCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void GoldenAnvilCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void GoldenHammerCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void GoldenHammerCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void StickerCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void HoloBoxCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void HoloAnvilCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void GoldenAnvilCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void GoldenHammerCleanUpBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void StickerCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void StickerStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void TextControllerCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void TitleScreenDrawBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void TitleScreenStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void TitleCharacterDrawBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void TitleScreenCreateAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void TitleScreenMouse53Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void SummonCreateBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void SummonStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void SummonStepAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void OreDepositStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void GetFishAlarm0Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void GetFishAlarm1Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void ShionPortalCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void AcerolaJuiceCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void SaplingCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void LangOrbCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void HololiveMerchCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void CoronetCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerManagerOther23Before(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void PlayerManagerOther23After(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void StickerAlarm1After(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void CocoWeaponStepBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void CocoWeaponStepAfter(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);
void CocoWeaponCollisionPlayerBefore(std::tuple<CInstance*, CInstance*, CCode*, int, RValue*>& Args);