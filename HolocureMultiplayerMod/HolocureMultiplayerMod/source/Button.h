#pragma once
#include "ModuleMain.h"
#include <vector>
#include <string>

class coopMenuButtonsGridData;

extern coopMenuButtonsGridData gamemodeSelectButtonsGrid;
extern coopMenuButtonsGridData networkAdapterDisclaimerButtonsGrid;
extern coopMenuButtonsGridData networkAdapterNamesButtonsGrid;
extern coopMenuButtonsGridData coopOptionButtonsGrid;
extern coopMenuButtonsGridData coopLobbyMenuButtonsGrid;

enum ButtonIDs
{
	COOPMENU_NONE,
	COOPMENU_GAMEMODESELECT_PlaySinglePlayer,
	COOPMENU_GAMEMODESELECT_UseSavedNetworkAdapter,
	COOPMENU_GAMEMODESELECT_SelectNetworkAdapter,
	COOPMENU_GAMEMODESELECT_CreateFriendsSteamLobby,
	COOPMENU_NETWORKADAPTER_NetworkAdapterDisclaimerOk,
	COOPMENU_NETWORKADAPTER_NetworkAdapterNameOne,
	COOPMENU_NETWORKADAPTER_NetworkAdapterNameTwo,
	COOPMENU_NETWORKADAPTER_NetworkAdapterNameThree,
	COOPMENU_NETWORKADAPTER_NetworkAdapterNameFour,
	COOPMENU_NETWORKADAPTER_NetworkAdapterNameFive,
	COOPMENU_NETWORKADAPTER_NetworkAdapterNameNext,
	COOPMENU_NETWORKADAPTER_NetworkAdapterNamePrev,
	COOPMENU_COOPOPTION_HostLanSession,
	COOPMENU_COOPOPTION_JoinLanSession,
	COOPMENU_STEAMLOBBY_LobbyPlayerOne,
	COOPMENU_STEAMLOBBY_LobbyPlayerTwo,
	COOPMENU_STEAMLOBBY_LobbyPlayerThree,
	COOPMENU_STEAMLOBBY_LobbyPlayerFour,
	COOPMENU_STEAMLOBBY_LobbyPlayerFive,
	COOPMENU_STEAMLOBBY_LobbyPlayerSix,
	COOPMENU_STEAMLOBBY_LobbyPlayerSeven,
	COOPMENU_STEAMLOBBY_LobbyPlayerEight,
	COOPMENU_STEAMLOBBY_LobbyPlayerNine,
	COOPMENU_STEAMLOBBY_LobbyPlayerTen,
	COOPMENU_STEAMLOBBY_LobbyPlayerInviteOne,
	COOPMENU_STEAMLOBBY_LobbyPlayerInviteTwo,
	COOPMENU_STEAMLOBBY_LobbyPlayerInviteThree,
	COOPMENU_STEAMLOBBY_LobbyPlayerInviteFour,
	COOPMENU_STEAMLOBBY_LobbyPlayerInviteFive,
	COOPMENU_STEAMLOBBY_LobbyPlayerInviteSix,
	COOPMENU_STEAMLOBBY_LobbyPlayerInviteSeven,
	COOPMENU_STEAMLOBBY_LobbyPlayerInviteEight,
	COOPMENU_STEAMLOBBY_LobbyPlayerInviteNine,
	COOPMENU_STEAMLOBBY_LobbyPlayerInviteTen,
	COOPMENU_LOBBYMENU_LobbyChooseCharacter,
	COOPMENU_LOBBYMENU_LobbyReady,
	COOPMENU_LOBBYMENU_SelectMap,
	COOPMENU_LOBBYMENU_Start,
};

struct coopMenuButtonsData
{
	int buttonXPos;
	int buttonYPos;
	ButtonIDs buttonID;
	std::string buttonText;
	bool isVisible;

	coopMenuButtonsData(int buttonXPos, int buttonYPos, ButtonIDs buttonID, std::string buttonText, bool isVisible) :
		buttonXPos(buttonXPos), buttonYPos(buttonYPos), buttonID(buttonID), buttonText(buttonText), isVisible(isVisible), defaultButtonText(buttonText), defaultIsVisible(isVisible)
	{
	}

	void resetButton();

private:
	std::string defaultButtonText;
	bool defaultIsVisible;
};

class coopMenuButtonsColumnData
{
public:
	std::vector<coopMenuButtonsData> menuButtonsList;
	int curSelectedButtonIndex;

	coopMenuButtonsColumnData() : curSelectedButtonIndex(0), defaultCurSelectedButtonIndex(0)
	{
	}

	coopMenuButtonsColumnData(std::vector<coopMenuButtonsData> menuButtonsList, int defaultCurSelectedButtonIndex) :
		menuButtonsList(menuButtonsList), curSelectedButtonIndex(defaultCurSelectedButtonIndex), defaultCurSelectedButtonIndex(defaultCurSelectedButtonIndex)
	{
	}

	void addButton(coopMenuButtonsData button)
	{
		menuButtonsList.push_back(button);
	}

	int getMinVisibleButtonIndex(int curButtonIndex);

	int getMaxVisibleButtonIndex(int curButtonIndex);

	void resetButtons();

private:
	int defaultCurSelectedButtonIndex;
};

class coopMenuButtonsGridData
{
public:
	std::vector<coopMenuButtonsColumnData> menuButtonsColumnsList;
	int curSelectedColumnIndex;

	coopMenuButtonsGridData() : curSelectedColumnIndex(0), defaultCurSelectedColumnIndex(0)
	{
	}

	coopMenuButtonsGridData(std::vector<coopMenuButtonsColumnData> menuButtonsColumnsList, int defaultCurSelectedColumnIndex) :
		menuButtonsColumnsList(menuButtonsColumnsList), curSelectedColumnIndex(defaultCurSelectedColumnIndex), defaultCurSelectedColumnIndex(defaultCurSelectedColumnIndex)
	{
	}

	void addButtonColumn(coopMenuButtonsColumnData buttonColumn)
	{
		menuButtonsColumnsList.push_back(buttonColumn);
	}

	void draw(CInstance* Self);

	void processInput(bool isActionOnePressed, bool isActionTwoPressed, bool isEnterPressed, bool isEscPressed,
		bool isMoveUpPressed, bool isMoveDownPressed, bool isMoveLeftPressed, bool isMoveRightPressed);

	int getMinVisibleButtonColumnIndex(int curButtonColumnIndex);

	int getMaxVisibleButtonColumnIndex(int curButtonColumnIndex);

	void resetMenu();

private:
	int defaultCurSelectedColumnIndex;
};

void initButtonMenus();