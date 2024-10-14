#pragma once
#include "ModuleMain.h"
#include <vector>
#include <string>
#include "HoloCureMenuInterface/HoloCureMenuInterface.h"

extern HoloCureMenuInterface* holoCureMenuInterfacePtr;

void initButtonMenus();
void clickUseSavedNetworkAdapter();
void clickSelectNetworkAdapter();
void clickCreateFriendsSteamLobby();
void clickNetworkAdapterDisclaimerOK();
void clickNetworkAdapterNameNext();
void clickNetworkAdapterNamePrev();
void clickNetworkAdapterName();
void clickHostLanSession();
void clickJoinLanSession();
void clickLobbyChooseCharacter();
void clickLobbyReady();
void clickLobbySelectMap();
void clickLobbyStart();
void clickLobbySteamPlayer();
void clickLobbySteamPlayerInvite();

void networkAdapterDisclaimerMenuGridReturn();
void LANSessionMenuGridReturn();
void lobbyMenuGridReturn();
void selectingCharacterMenuGridReturn();
void selectingMapMenuGridReturn();
void drawLobbyMenu();

struct menuColumn
{
	std::shared_ptr<menuColumnData> menuColumnPtr;
	std::vector<std::shared_ptr<menuData>> menuDataPtrList;

	menuColumn(std::vector<std::shared_ptr<menuData>> menuDataPtrList) : menuColumnPtr(nullptr), menuDataPtrList(menuDataPtrList)
	{
	}

	void initMenuColumn(std::shared_ptr<menuGridData>& menuGridPtr)
	{
		holoCureMenuInterfacePtr->CreateMenuColumn(MODNAME, menuGridPtr, menuColumnPtr);
		for (auto& menuDataPtr : menuDataPtrList)
		{
			holoCureMenuInterfacePtr->AddMenuData(MODNAME, menuColumnPtr, menuDataPtr);
		}
	}
};

struct menuGrid
{
	std::string name;
	menuGrid* prevMenuGridPtr;
	std::shared_ptr<menuGridData> menuGridPtr;
	std::vector<menuColumn> menuColumnList;
	menuFunc onEnterFunc;
	menuFunc onReturnFunc;
	menuFunc drawFunc;

	menuGrid() : menuGridPtr(nullptr), prevMenuGridPtr(nullptr), onEnterFunc(nullptr), onReturnFunc(nullptr), drawFunc(nullptr)
	{
	}

	menuGrid(std::vector<menuColumn> menuColumnList, std::string name, menuGrid* prevMenuGridPtr) :
		menuGridPtr(nullptr), name(name), prevMenuGridPtr(prevMenuGridPtr), menuColumnList(menuColumnList), onEnterFunc(nullptr), onReturnFunc(nullptr), drawFunc(nullptr)
	{
	}

	menuGrid(std::vector<menuColumn> menuColumnList, std::string name, menuGrid* prevMenuGridPtr, menuFunc onEnterFunc, menuFunc onReturnFunc, menuFunc drawFunc) :
		menuGridPtr(nullptr), name(name), prevMenuGridPtr(prevMenuGridPtr), menuColumnList(menuColumnList), onEnterFunc(onEnterFunc), onReturnFunc(onReturnFunc), drawFunc(drawFunc)
	{
	}

	void initMenuGrid()
	{
		std::shared_ptr<menuGridData> menuGridDataPtr = nullptr;
		if (prevMenuGridPtr != nullptr)
		{
			menuGridDataPtr = prevMenuGridPtr->menuGridPtr;
		}
		holoCureMenuInterfacePtr->CreateMenuGrid(MODNAME, name, menuGridDataPtr, menuGridPtr);
		menuGridPtr->onEnterFunc = onEnterFunc;
		menuGridPtr->onReturnFunc = onReturnFunc;
		menuGridPtr->drawFunc = drawFunc;
		for (auto& menuColumn : menuColumnList)
		{
			menuColumn.initMenuColumn(menuGridPtr);
		}
	}
};