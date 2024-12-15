#pragma once
#include <YYToolkit/shared.hpp>
#include <CallbackManager/CallbackManagerInterface.h>

#define VERSION_NUM "v1.2.2"
#define MODNAME "Holocure Multiplayer Mod " VERSION_NUM 
#define BROADCAST_PORT "27015"
#define GAME_PORT "27016"

#define SOME_ENUM(DO) \
	DO(x) \
	DO(y) \
	DO(direction) \
	DO(image_xscale) \
	DO(image_yscale) \
	DO(sprite_index) \
	DO(setForDeath) \
	DO(__source_keyboard) \
	DO(id) \
	DO(completeStop) \
	DO(followTarget) \
	DO(mouseFollowMode) \
	DO(runSprite) \
	DO(idleSprite) \
	DO(createdChar) \
	DO(charData) \
	DO(charName) \
	DO(sprite1) \
	DO(sprite2) \
	DO(challenge) \
	DO(baseStats) \
	DO(HP) \
	DO(currentHP) \
	DO(ATK) \
	DO(SPD) \
	DO(crit) \
	DO(haste) \
	DO(pickupRange) \
	DO(prebuffStats) \
	DO(SnapshotPrebuffStats) \
	DO(playerCharacter) \
	DO(specialid) \
	DO(specID) \
	DO(specCD) \
	DO(specUnlock) \
	DO(canControl) \
	DO(image_angle) \
	DO(image_alpha) \
	DO(image_speed) \
	DO(image_index) \
	DO(customDrawScriptBelow) \
	DO(customDrawScriptAbove) \
	DO(transparent) \
	DO(isEnemy) \
	DO(spriteColor) \
	DO(duration) \
	DO(isDead) \
	DO(mask_index) \
	DO(initialSpawn) \
	DO(pickupExp) \
	DO(picked) \
	DO(attackID) \
	DO(depth) \
	DO(paused) \
	DO(followPlayerID) \
	DO(leveled) \
	DO(controlsFree) \
	DO(options) \
	DO(optionIcon) \
	DO(optionType) \
	DO(optionName) \
	DO(optionDescription) \
	DO(weaponType) \
	DO(itemType) \
	DO(weapons) \
	DO(perks) \
	DO(charPerks) \
	DO(ITEMS) \
	DO(items) \
	DO(attackIndex) \
	DO(config) \
	DO(levelOptionSelect) \
	DO(collabListShowing) \
	DO(collabListSelected) \
	DO(optionID) \
	DO(attacksCopy) \
	DO(attacks) \
	DO(alarm) \
	DO(paused_screen_sprite) \
	DO(PERKS) \
	DO(playerStatUps) \
	DO(playerSnapshot) \
	DO(destructableID) \
	DO(eliminateMode) \
	DO(persistent) \
	DO(specialMeter) \
	DO(removedItems) \
	DO(eliminatedAttacks) \
	DO(rerollContainer) \
	DO(eliminatedThisLevel) \
	DO(toNextLevel) \
	DO(sprite3) \
	DO(sprites) \
	DO(lifetime) \
	DO(dir) \
	DO(waitSpawn) \
	DO(preCreateMapIndex) \
	DO(vfxMapIndex) \
	DO(afterimage_color) \
	DO(deathTime) \
	DO(afterImageMapIndex) \
	DO(interactableMapIndex) \
	DO(destroyIfNoneLeft) \
	DO(Destroy) \
	DO(stickerID) \
	DO(gotBox) \
	DO(gotAnvil) \
	DO(gotGoldenAnvil) \
	DO(canCollide) \
	DO(boxItemAmount) \
	DO(superBox) \
	DO(randomWeapon) \
	DO(becomeSuper) \
	DO(boxOpenned) \
	DO(itemBoxTakeOption) \
	DO(currentBoxItem) \
	DO(usedAnvil) \
	DO(anvilOption) \
	DO(loadOutList) \
	DO(level) \
	DO(maxLevel) \
	DO(enhancements) \
	DO(upgradeOption) \
	DO(enhancing) \
	DO(enhanceDone) \
	DO(enhanceResult) \
	DO(anvilID) \
	DO(boxCoinGain) \
	DO(gainedMods) \
	DO(offeredMod) \
	DO(optionIcon_Super) \
	DO(optionIcon_Normal) \
	DO(usedSticker) \
	DO(STICKERS) \
	DO(stickerData) \
	DO(gotSticker) \
	DO(stickerActionSelected) \
	DO(stickerAction) \
	DO(stickerOption) \
	DO(LevelUp) \
	DO(used) \
	DO(moneyGain) \
	DO(collabDone) \
	DO(collabingWeapon) \
	DO(perksMenu) \
	DO(collabsMenu) \
	DO(pauseCurrentMenu) \
	DO(quitConfirm) \
	DO(buffs) \
	DO(stacks) \
	DO(timer) \
	DO(creator) \
	DO(currentOption) \
	DO(mouseMoving) \
	DO(idletime) \
	DO(availableOutfits) \
	DO(availableStages) \
	DO(selectedStage) \
	DO(stageSprite) \
	DO(gameOvered) \
	DO(gameWon) \
	DO(gameDone) \
	DO(gameOverTime) \
	DO(pauseOption) \
	DO(enabledSpawner) \
	DO(sequenceSpawn) \
	DO(quitOption) \
	DO(createdTime) \
	DO(image_number) \
	DO(spritePlaybackSpeed) \
	DO(Confirmed) \
	DO(version) \
	DO(playerSummon) \
	DO(customDrawScript) \
	DO(rarest) \
	DO(oreType) \
	DO(scripts) \
	DO(MaterialGrind) \
	DO(oreA) \
	DO(oreB) \
	DO(oreC) \
	DO(waitTime) \
	DO(moveUpPressed) \
	DO(moveLeftPressed) \
	DO(moveRightPressed) \
	DO(moveDownPressed) \
	DO(actionOnePressed) \
	DO(actionTwoPressed) \
	DO(enterPressed) \
	DO(escPressed) \
	DO(food) \
	DO(playerID) \
	DO(isMoving) \
	DO(availableWeaponCollabs) \
	DO(weaponCollabs) \
	DO(isPlayer) \
	DO(origPlayerCreator) \
	DO(player) \

#define MAKE_ENUM(VAR) GML_ ## VAR,
enum VariableNames
{
	SOME_ENUM(MAKE_ENUM)
};

#define MAKE_STRINGS(VAR) #VAR,
const char* const VariableNamesStringsArr[] =
{
	SOME_ENUM(MAKE_STRINGS)
};

extern RValue GMLVarIndexMapGMLHash[1001];

extern CallbackManagerInterface* callbackManagerInterfacePtr;
extern YYTKInterface* g_ModuleInterface;
extern YYRunnerInterface g_RunnerInterface;

extern PFUNC_YYGMLScript origInputBindingSetScript;
extern PFUNC_YYGMLScript origInputCheckScript;
extern PFUNC_YYGMLScript origInputSourceModeSetScript;
extern PFUNC_YYGMLScript origInputSourceClearScript;
extern PFUNC_YYGMLScript origInputJoinParamsSetScript;
extern PFUNC_YYGMLScript origSnapshotPrebuffStatsPlayerCreateScript;
extern PFUNC_YYGMLScript origUpdatePlayerPlayerManagerOtherScript;
extern PFUNC_YYGMLScript origAddAttackPlayerManagerOtherScript;
extern PFUNC_YYGMLScript origInputDirectionScript;
extern PFUNC_YYGMLScript origInputSourceUsingScript;
extern PFUNC_YYGMLScript origInputGlobalScript;
extern PFUNC_YYGMLScript origDrawTextOutlineScript;
extern PFUNC_YYGMLScript origInputSourceIsAvailableScript;
extern PFUNC_YYGMLScript origInputPlayerConnectedCountScript;
extern PFUNC_YYGMLScript origInputSourceGetArrayScript;
extern PFUNC_YYGMLScript origInputSourceDetectNewScript;
extern PFUNC_YYGMLScript origInputGamepadIsConnectedScript;
extern PFUNC_YYGMLScript origGeneratePossibleOptionsScript;
extern PFUNC_YYGMLScript origOptionOneScript;
extern PFUNC_YYGMLScript origOptionTwoScript;
extern PFUNC_YYGMLScript origOptionThreeScript;
extern PFUNC_YYGMLScript origOptionFourScript;
extern PFUNC_YYGMLScript origUnpauseScript;
extern PFUNC_YYGMLScript origAddPerkScript;
extern PFUNC_YYGMLScript origAddItemScript;
extern PFUNC_YYGMLScript origAddConsumableScript;
extern PFUNC_YYGMLScript origAddStatScript;
extern PFUNC_YYGMLScript origVariableStructCopyScript;
extern PFUNC_YYGMLScript origEliminateAttackScript;
extern PFUNC_YYGMLScript origRemoveItemScript;
extern PFUNC_YYGMLScript origRemovePerkScript;
extern PFUNC_YYGMLScript origExecuteSpecialAttackScript;
extern PFUNC_YYGMLScript origDestroyHoloAnvilScript;
extern PFUNC_YYGMLScript origDestroyGoldenAnvilScript;
extern PFUNC_YYGMLScript origDestroyGoldenHammerScript;
extern PFUNC_YYGMLScript origDestroyStickerScript;
extern PFUNC_YYGMLScript origPauseScript;
extern PFUNC_YYGMLScript origGetBoxScript;
extern PFUNC_YYGMLScript origGetAnvilScript;
extern PFUNC_YYGMLScript origGetGoldenAnvilScript;
extern PFUNC_YYGMLScript origGetStickerScript;
extern PFUNC_YYGMLScript origAddEnchantScript;
extern PFUNC_YYGMLScript origAddCollabScript;
extern PFUNC_YYGMLScript origAddSuperCollabScript;
extern PFUNC_YYGMLScript origMouseOverButtonScript;
extern CInstance* globalInstance;
extern int objPlayerIndex;
extern int objBaseMobIndex;
extern int objAttackIndex;
extern int objPickupableIndex;
extern int objSummonIndex;
extern int objAttackControllerIndex;
extern int objPlayerManagerIndex;
extern int objDestructableIndex;
extern int objYagooPillarIndex;
extern int objCautionIndex;
extern int objCautionAttackIndex;
extern int objPreCreateIndex;
extern int objVFXIndex;
extern int objAfterImageIndex;
extern int objHoloBoxIndex;
extern int objHoloAnvilIndex;
extern int objGoldenAnvilIndex;
extern int objGoldenHammerIndex;
extern int objStickerIndex;
extern int objItemLightBeamIndex;
extern int objOptionsIndex;
extern int objInputManagerIndex;
extern int objCharacterDataIndex;
extern int objCharSelectIndex;
extern int objObstacleIndex;
extern int objGetFishIndex;
extern int objCocoWeaponIndex;
extern int sprEmptyIndex;
extern int sprGameCursorIndex;
extern int sprGameCursor2Index;
extern int sprEmptyMaskIndex;
extern int sprSummonPointerIndex;
extern int sprHudInitButtonsIndex;
extern int sprKaelaMinerals;
extern int sprHudToggleButtonIndex;
extern int sprHudOptionButtonIndex;
extern int jpFont;
extern int rmTitle;
extern int rmCharSelect;

extern SOCKET serverSocket;
extern SOCKET connectClientSocket;
extern bool isHost;
extern bool hasConnected;

extern char broadcastAddressBuffer[16];

extern TRoutine origStructGetFromHashFunc;
extern TRoutine origStructSetFromHashFunc;
extern TRoutine origSpriteDeleteScript;

template<typename... Args>
void LogPrint(YYTK::CmColor Color, const char* LogFormat, Args... args)
{
	callbackManagerInterfacePtr->LogToFile(MODNAME, LogFormat, args...);
	g_ModuleInterface->Print(Color, LogFormat, args...);
}

std::string ConvertLPCWSTRToString(LPCWSTR lpcwszStr);