#include "Button.h"
#include "ModuleMain.h"
#include "CodeEvents.h"

extern ButtonIDs curButtonID;

coopMenuButtonsGridData gamemodeSelectButtonsGrid;
coopMenuButtonsGridData networkAdapterDisclaimerButtonsGrid;
coopMenuButtonsGridData networkAdapterNamesButtonsGrid;
coopMenuButtonsGridData coopOptionButtonsGrid;
coopMenuButtonsGridData coopLobbyMenuButtonsGrid;

void initButtonMenus()
{
	gamemodeSelectButtonsGrid = coopMenuButtonsGridData({
		coopMenuButtonsColumnData({
			coopMenuButtonsData(530, 104 + 29 * 0, COOPMENU_GAMEMODESELECT_PlaySinglePlayer, "Play Single Player", true),
			coopMenuButtonsData(530, 104 + 29 * 1, COOPMENU_GAMEMODESELECT_UseSavedNetworkAdapter, "Use saved network adapter", true),
			coopMenuButtonsData(530, 104 + 29 * 2, COOPMENU_GAMEMODESELECT_SelectNetworkAdapter, "Select network adapter", true),
			coopMenuButtonsData(530, 104 + 29 * 3, COOPMENU_GAMEMODESELECT_CreateFriendsSteamLobby, "Create friend Steam lobby", true)
		}, 0)
	}, 0);

	networkAdapterDisclaimerButtonsGrid = coopMenuButtonsGridData({
		coopMenuButtonsColumnData({
			coopMenuButtonsData(320, 250, COOPMENU_NETWORKADAPTER_NetworkAdapterDisclaimerOk, "OK", true)
		}, 0)
	}, 0);

	networkAdapterNamesButtonsGrid = coopMenuButtonsGridData({
		coopMenuButtonsColumnData({
			coopMenuButtonsData(530, 104 + 29 * 0, COOPMENU_NETWORKADAPTER_NetworkAdapterNameOne, "", false),
			coopMenuButtonsData(530, 104 + 29 * 1, COOPMENU_NETWORKADAPTER_NetworkAdapterNameTwo, "", false),
			coopMenuButtonsData(530, 104 + 29 * 2, COOPMENU_NETWORKADAPTER_NetworkAdapterNameThree, "", false),
			coopMenuButtonsData(530, 104 + 29 * 3, COOPMENU_NETWORKADAPTER_NetworkAdapterNameFour, "", false),
			coopMenuButtonsData(530, 104 + 29 * 4, COOPMENU_NETWORKADAPTER_NetworkAdapterNameFive, "", false),
			coopMenuButtonsData(530, 104 + 29 * 5, COOPMENU_NETWORKADAPTER_NetworkAdapterNameNext, "", false),
			coopMenuButtonsData(530, 104 + 29 * 6, COOPMENU_NETWORKADAPTER_NetworkAdapterNamePrev, "", false)
		}, 0)
	}, 0);

	coopOptionButtonsGrid = coopMenuButtonsGridData({
		coopMenuButtonsColumnData({
			coopMenuButtonsData(530, 104 + 29 * 0, COOPMENU_COOPOPTION_HostLanSession, "Host LAN Session", true),
			coopMenuButtonsData(530, 104 + 29 * 1, COOPMENU_COOPOPTION_JoinLanSession, "Join LAN Session", true)
		}, 0)
	}, 0);

	coopLobbyMenuButtonsGrid = coopMenuButtonsGridData({
		coopMenuButtonsColumnData({
			coopMenuButtonsData(160, 14 + 29 * 0, COOPMENU_STEAMLOBBY_LobbyPlayerOne, "", false),
			coopMenuButtonsData(160, 14 + 29 * 1, COOPMENU_STEAMLOBBY_LobbyPlayerTwo, "", false),
			coopMenuButtonsData(160, 14 + 29 * 2, COOPMENU_STEAMLOBBY_LobbyPlayerThree, "", false),
			coopMenuButtonsData(160, 14 + 29 * 3, COOPMENU_STEAMLOBBY_LobbyPlayerFour, "", false),
			coopMenuButtonsData(160, 14 + 29 * 4, COOPMENU_STEAMLOBBY_LobbyPlayerFive, "", false),
			coopMenuButtonsData(160, 14 + 29 * 5, COOPMENU_STEAMLOBBY_LobbyPlayerSix, "", false),
			coopMenuButtonsData(160, 14 + 29 * 6, COOPMENU_STEAMLOBBY_LobbyPlayerSeven, "", false),
			coopMenuButtonsData(160, 14 + 29 * 7, COOPMENU_STEAMLOBBY_LobbyPlayerEight, "", false),
			coopMenuButtonsData(160, 14 + 29 * 8, COOPMENU_STEAMLOBBY_LobbyPlayerNine, "", false),
			coopMenuButtonsData(160, 14 + 29 * 9, COOPMENU_STEAMLOBBY_LobbyPlayerTen, "", false)
		}, 0),
		coopMenuButtonsColumnData({
			coopMenuButtonsData(340, 14 + 29 * 0, COOPMENU_STEAMLOBBY_LobbyPlayerInviteOne, "Invite user", false),
			coopMenuButtonsData(340, 14 + 29 * 1, COOPMENU_STEAMLOBBY_LobbyPlayerInviteTwo, "Invite user", false),
			coopMenuButtonsData(340, 14 + 29 * 2, COOPMENU_STEAMLOBBY_LobbyPlayerInviteThree, "Invite user", false),
			coopMenuButtonsData(340, 14 + 29 * 3, COOPMENU_STEAMLOBBY_LobbyPlayerInviteFour, "Invite user", false),
			coopMenuButtonsData(340, 14 + 29 * 4, COOPMENU_STEAMLOBBY_LobbyPlayerInviteFive, "Invite user", false),
			coopMenuButtonsData(340, 14 + 29 * 5, COOPMENU_STEAMLOBBY_LobbyPlayerInviteSix, "Invite user", false),
			coopMenuButtonsData(340, 14 + 29 * 6, COOPMENU_STEAMLOBBY_LobbyPlayerInviteSeven, "Invite user", false),
			coopMenuButtonsData(340, 14 + 29 * 7, COOPMENU_STEAMLOBBY_LobbyPlayerInviteEight, "Invite user", false),
			coopMenuButtonsData(340, 14 + 29 * 8, COOPMENU_STEAMLOBBY_LobbyPlayerInviteNine, "Invite user", false),
			coopMenuButtonsData(340, 14 + 29 * 9, COOPMENU_STEAMLOBBY_LobbyPlayerInviteTen, "Invite user", false)
		}, 0),
		coopMenuButtonsColumnData({
			coopMenuButtonsData(530, 104 + 29 * 0, COOPMENU_LOBBYMENU_LobbyChooseCharacter, "Choose Character", true),
			coopMenuButtonsData(530, 104 + 29 * 1, COOPMENU_LOBBYMENU_LobbyReady, "Ready", true),
			coopMenuButtonsData(530, 104 + 29 * 2, COOPMENU_LOBBYMENU_SelectMap, "Select Map", true),
			coopMenuButtonsData(530, 104 + 29 * 3, COOPMENU_LOBBYMENU_Start, "Start", true)
		}, 0),
	}, 2);
}

void coopMenuButtonsGridData::draw(CInstance* Self)
{
	double textColor[2]{ 0xFFFFFF, 0 };
	RValue inputManager = g_ModuleInterface->CallBuiltin("instance_find", { objInputManagerIndex, 0 });
	//	bool isMouseMoving = getInstanceVariable(inputManager, GML_mouseMoving).AsBool();
	for (int i = 0; i < menuButtonsColumnsList.size(); i++)
	{
		auto& curMenuButtonColumn = menuButtonsColumnsList[i];
		for (int j = 0; j < curMenuButtonColumn.menuButtonsList.size(); j++)
		{
			auto& curMenuButton = curMenuButtonColumn.menuButtonsList[j];
			if (!curMenuButton.isVisible)
			{
				continue;
			}
			//			if (isMouseMoving)
			{
				RValue** args = new RValue*[4];
				RValue mouseOverType = "long";
				RValue curButtonX = curMenuButton.buttonXPos;
				RValue curButtonY = curMenuButton.buttonYPos;
				RValue scale = .5;
				args[0] = &mouseOverType;
				args[1] = &curButtonX;
				args[2] = &curButtonY;
				args[3] = &scale;
				RValue returnVal;
				origMouseOverButtonScript(Self, nullptr, returnVal, 4, args);
				if (returnVal.AsBool())
				{
					curButtonID = curMenuButton.buttonID;
					curSelectedColumnIndex = i;
					curMenuButtonColumn.curSelectedButtonIndex = j;
				}
			}
			int isOptionSelected = (curSelectedColumnIndex == i) && (curMenuButtonColumn.curSelectedButtonIndex == j);
			g_ModuleInterface->CallBuiltin("draw_set_font", { jpFont });
			g_ModuleInterface->CallBuiltin("draw_set_halign", { 1 });
			g_ModuleInterface->CallBuiltin("draw_sprite_ext", { sprHudInitButtonsIndex, isOptionSelected, curMenuButton.buttonXPos, curMenuButton.buttonYPos, 1, 1, 0, static_cast<double>(0xFFFFFF), 1 });
			g_ModuleInterface->CallBuiltin("draw_text_color", { curMenuButton.buttonXPos, curMenuButton.buttonYPos + 9, curMenuButton.buttonText, textColor[isOptionSelected], textColor[isOptionSelected], textColor[isOptionSelected], textColor[isOptionSelected], 1 });
		}
	}
}

void coopMenuButtonsGridData::processInput(bool isActionOnePressed, bool isActionTwoPressed, bool isEnterPressed, bool isEscPressed,
	bool isMoveUpPressed, bool isMoveDownPressed, bool isMoveLeftPressed, bool isMoveRightPressed)
{
	coopMenuButtonsColumnData& curButtonColumn = menuButtonsColumnsList[curSelectedColumnIndex];
	if (isActionOnePressed || isEnterPressed)
	{
		// Confirm
	}
	else if (isActionTwoPressed || isEscPressed)
	{
		// Return
	}
	else if (isMoveUpPressed)
	{
		// Up
		int nextButtonIndex = curButtonColumn.getMaxVisibleButtonIndex(curButtonColumn.curSelectedButtonIndex);
		if (nextButtonIndex != -1)
		{
			curButtonColumn.curSelectedButtonIndex = nextButtonIndex;
			curButtonID = curButtonColumn.menuButtonsList[nextButtonIndex].buttonID;
		}
	}
	else if (isMoveDownPressed)
	{
		// Down
		int nextButtonIndex = curButtonColumn.getMinVisibleButtonIndex(curButtonColumn.curSelectedButtonIndex);
		if (nextButtonIndex != -1)
		{
			curButtonColumn.curSelectedButtonIndex = nextButtonIndex;
			curButtonID = curButtonColumn.menuButtonsList[nextButtonIndex].buttonID;
		}
	}
	else if (isMoveLeftPressed)
	{
		// Left
		int nextColumnIndex = getMaxVisibleButtonColumnIndex(curSelectedColumnIndex);
		if (nextColumnIndex != -1)
		{
			curSelectedColumnIndex = nextColumnIndex;
			auto& curColumnList = menuButtonsColumnsList[curSelectedColumnIndex];
			if (curButtonColumn.curSelectedButtonIndex == -1)
			{
				g_ModuleInterface->Print(CM_RED, "Button index is set to -1");
				return;
			}
			if (!curColumnList.menuButtonsList[curButtonColumn.curSelectedButtonIndex].isVisible)
			{
				curButtonColumn.curSelectedButtonIndex = curButtonColumn.getMinVisibleButtonIndex(-1);
				if (curButtonColumn.curSelectedButtonIndex == -1)
				{
					g_ModuleInterface->Print(CM_RED, "Button index not found");
					return;
				}
			}
			curButtonID = curColumnList.menuButtonsList[curButtonColumn.curSelectedButtonIndex].buttonID;
		}
	}
	else if (isMoveRightPressed)
	{
		// Right
		int nextColumnIndex = getMinVisibleButtonColumnIndex(curSelectedColumnIndex);
		if (nextColumnIndex != -1)
		{
			curSelectedColumnIndex = nextColumnIndex;
			auto& curColumnList = menuButtonsColumnsList[curSelectedColumnIndex];
			if (curButtonColumn.curSelectedButtonIndex == -1)
			{
				g_ModuleInterface->Print(CM_RED, "Button index is set to -1");
				return;
			}
			if (!curColumnList.menuButtonsList[curButtonColumn.curSelectedButtonIndex].isVisible)
			{
				curButtonColumn.curSelectedButtonIndex = curButtonColumn.getMinVisibleButtonIndex(-1);
				if (curButtonColumn.curSelectedButtonIndex == -1)
				{
					g_ModuleInterface->Print(CM_RED, "Button index not found");
					return;
				}
			}
			curButtonID = curColumnList.menuButtonsList[curButtonColumn.curSelectedButtonIndex].buttonID;
		}
	}
}

void coopMenuButtonsGridData::resetMenu()
{
	curSelectedColumnIndex = defaultCurSelectedColumnIndex;
	for (auto& menuButtonColumn : menuButtonsColumnsList)
	{
		menuButtonColumn.resetButtons();
	}
}

void coopMenuButtonsColumnData::resetButtons()
{
	curSelectedButtonIndex = defaultCurSelectedButtonIndex;
	for (auto& menuButton : menuButtonsList)
	{
		menuButton.resetButton();
	}
}

void coopMenuButtonsData::resetButton()
{
	buttonText = defaultButtonText;
	isVisible = defaultIsVisible;
}

int coopMenuButtonsColumnData::getMinVisibleButtonIndex(int curButtonIndex)
{
	for (int i = curButtonIndex + 1; i < menuButtonsList.size(); i++)
	{
		if (menuButtonsList[i].isVisible)
		{
			return i;
		}
	}
	return -1;
}

int coopMenuButtonsColumnData::getMaxVisibleButtonIndex(int curButtonIndex)
{
	for (int i = curButtonIndex - 1; i >= 0; i--)
	{
		if (menuButtonsList[i].isVisible)
		{
			return i;
		}
	}
	return -1;
}

int coopMenuButtonsGridData::getMinVisibleButtonColumnIndex(int curButtonColumnIndex)
{
	for (int i = curButtonColumnIndex + 1; i < menuButtonsColumnsList.size(); i++)
	{
		if (menuButtonsColumnsList[i].getMinVisibleButtonIndex(-1) != -1)
		{
			return i;
		}
	}
	return -1;
}

int coopMenuButtonsGridData::getMaxVisibleButtonColumnIndex(int curButtonColumnIndex)
{
	for (int i = curButtonColumnIndex - 1; i >= 0; i--)
	{
		if (menuButtonsColumnsList[i].getMinVisibleButtonIndex(-1) != -1)
		{
			return i;
		}
	}
	return -1;
}