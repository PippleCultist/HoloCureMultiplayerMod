#pragma once

#include "YYToolkit/YYTK_Shared.hpp"
using namespace Aurie;
using namespace YYTK;

extern YYTKInterface* g_ModuleInterface;

class menuGridData;
class menuColumnData;
struct menuData;
struct spriteData;

struct HoloCureMenuInterface : AurieInterfaceBase
{
	virtual AurieStatus Create();
	virtual void Destroy();
	virtual void QueryVersion(
		OUT short& Major,
		OUT short& Minor,
		OUT short& Patch
	);

	/*
	* Call this to create a new menu grid. If prevMenuGrid is nullptr, it will add the menu to the title menu and return to the main menu
	*/
	virtual AurieStatus CreateMenuGrid(
		IN const std::string& ModName,
		IN std::string menuGridName,
		IN std::shared_ptr<menuGridData>& prevMenuGridPtr,
		OUT std::shared_ptr<menuGridData>& menuGridPtr
	);

	/*
	* Call this to delete a menu grid
	*/
	virtual AurieStatus DeleteMenuGrid(
		IN const std::string& ModName,
		IN std::shared_ptr<menuGridData>& menuGridPtr
	);

	/*
	* Call this to create a new menu column and add it to the menu grid
	*/
	virtual AurieStatus CreateMenuColumn(
		IN const std::string& ModName,
		IN std::shared_ptr<menuGridData>& menuGridPtr,
		OUT std::shared_ptr<menuColumnData>& menuColumnPtr
	);

	/*
	* Call this to add a menu data to a menu column
	*/
	virtual AurieStatus AddMenuData(
		IN const std::string& ModName,
		IN std::shared_ptr<menuColumnData>& menuColumnPtr,
		IN std::shared_ptr<menuData>& addMenuDataPtr
	);

	/*
	* Call this to swap the current menu grid the one passed in
	*/
	virtual AurieStatus SwapToMenuGrid(
		IN const std::string& ModName,
		IN std::shared_ptr<menuGridData>& menuGridPtr
	);

	/*
	* Call this to get the current selected menu data
	*/
	virtual AurieStatus GetSelectedMenuData(
		IN const std::string& ModName,
		OUT std::shared_ptr<menuData>& menuDataPtr
	);

	/*
	* Call this to get the current menu grid
	*/
	virtual AurieStatus GetCurrentMenuGrid(
		IN const std::string& ModName,
		OUT std::shared_ptr<menuGridData>& menuGridPtr
	);

	/*
	* Disable action buttons
	*/
	virtual AurieStatus DisableActionButtons(
		IN const std::string& ModName
	);

	/*
	* Enable action buttons
	*/
	virtual AurieStatus EnableActionButtons(
		IN const std::string& ModName
	);
};

enum MenuDataType
{
	MENUDATATYPE_NONE,
	MENUDATATYPE_Button,
	MENUDATATYPE_TextBoxField,
	MENUDATATYPE_NumberField,
	MENUDATATYPE_Image,
	MENUDATATYPE_Text,
	MENUDATATYPE_Selection,
	MENUDATATYPE_TextOutline
};

using menuFunc = void (*)(void);

struct menuData
{
	int xPos;
	int yPos;
	int width;
	int height;
	MenuDataType menuDataType;
	std::string labelName;
	std::string menuID;
	bool isVisible;

	menuData(int xPos, int yPos, int width, int height, MenuDataType menuDataType, std::string labelName, std::string menuID, bool isVisible) :
		xPos(xPos), yPos(yPos), width(width), height(height), menuDataType(menuDataType), labelName(labelName), menuID(menuID), isVisible(isVisible), defaultIsVisible(isVisible), defaultLabelName(labelName)
	{
	}

	void resetToDefault();

private:
	std::string defaultLabelName;
	bool defaultIsVisible;
};

struct menuDataButton : menuData
{
	menuFunc clickMenuFunc;
	menuFunc labelNameFunc;
	menuDataButton(int xPos, int yPos, int width, int height, std::string menuID, std::string labelName, bool isVisible, menuFunc clickMenuFunc, menuFunc labelNameFunc) :
		menuData(xPos, yPos, width, height, MENUDATATYPE_Button, labelName, menuID, isVisible),
		clickMenuFunc(clickMenuFunc), labelNameFunc(labelNameFunc)
	{
	}
};

struct menuDataTextBoxField : menuData
{
	std::string textField;
	menuFunc clickMenuFunc;
	menuFunc labelNameFunc;
	menuDataTextBoxField(int xPos, int yPos, int width, int height, std::string menuID, std::string labelName, bool isVisible, menuFunc clickMenuFunc, menuFunc labelNameFunc) :
		menuData(xPos, yPos, width, height, MENUDATATYPE_TextBoxField, labelName, menuID, isVisible),
		clickMenuFunc(clickMenuFunc), labelNameFunc(labelNameFunc)
	{
	}
};

struct menuDataNumberField : menuData
{
	std::string textField;
	menuFunc clickMenuFunc;
	menuFunc labelNameFunc;
	menuDataNumberField(int xPos, int yPos, int width, int height, std::string menuID, std::string labelName, bool isVisible, menuFunc clickMenuFunc, menuFunc labelNameFunc) :
		menuData(xPos, yPos, width, height, MENUDATATYPE_NumberField, labelName, menuID, isVisible),
		clickMenuFunc(clickMenuFunc), labelNameFunc(labelNameFunc)
	{
	}
};

struct menuDataImageField : menuData
{
	int fps;
	std::shared_ptr<spriteData> curSprite;
	int curSubImageIndex;
	int curFrameCount;
	menuDataImageField(int xPos, int yPos, int width, int height, std::string menuID, std::string labelName, bool isVisible, menuFunc clickMenuFunc, menuFunc labelNameFunc, int fps) :
		menuData(xPos, yPos, 0, 0, MENUDATATYPE_Image, labelName, menuID, isVisible),
		fps(fps), curSubImageIndex(0), curFrameCount(-1), curSprite(nullptr)
	{
	}
};

struct menuDataText : menuData
{
	menuFunc labelNameFunc;
	menuDataText(int xPos, int yPos, int width, int height, std::string menuID, std::string labelName, bool isVisible, menuFunc clickMenuFunc, menuFunc labelNameFunc) :
		menuData(xPos, yPos, width, height, MENUDATATYPE_Text, labelName, menuID, isVisible),
		labelNameFunc(labelNameFunc)
	{
	}
};

struct menuDataSelection : menuData
{
	int curSelectionTextIndex;
	std::vector<std::string> selectionText;
	menuFunc clickMenuFunc;
	menuFunc labelNameFunc;
	menuDataSelection(int xPos, int yPos, int width, int height, std::string menuID, std::string labelName, bool isVisible, menuFunc clickMenuFunc, menuFunc labelNameFunc, std::vector<std::string> selectionText) :
		menuData(xPos, yPos, 180, 20, MENUDATATYPE_Selection, labelName, menuID, isVisible),
		selectionText(selectionText), clickMenuFunc(clickMenuFunc), labelNameFunc(labelNameFunc), curSelectionTextIndex(0)
	{
	}
};

struct menuDataTextOutline : menuData
{
	int outlineWidth;
	int outlineColor;
	int numOutline;
	int linePixelSeparation;
	int pixelsBeforeLineBreak;
	int textColor;
	int alpha;
	menuFunc labelNameFunc;
	menuDataTextOutline(int xPos, int yPos, std::string menuID, std::string labelName, bool isVisible, menuFunc clickMenuFunc, menuFunc labelNameFunc,
		int outlineWidth, int outlineColor, int numOutline, int linePixelSeparation, int pixelsBeforeLineBreak, int textColor, int alpha) :
		menuData(xPos, yPos, 0, 0, MENUDATATYPE_TextOutline, labelName, menuID, isVisible),
		labelNameFunc(labelNameFunc), outlineWidth(outlineWidth), outlineColor(outlineColor), numOutline(numOutline),
		linePixelSeparation(linePixelSeparation), pixelsBeforeLineBreak(pixelsBeforeLineBreak), textColor(textColor), alpha(alpha)
	{
	}
};

class menuColumnData
{
public:
	std::vector<std::shared_ptr<menuData>> menuDataPtrList;
	int curSelectedIndex;

	menuColumnData() : curSelectedIndex(0), defaultCurSelectedIndex(0)
	{
	}

	menuColumnData(std::vector< std::shared_ptr<menuData>> menuDataPtrList, int defaultCurSelectedIndex) :
		menuDataPtrList(menuDataPtrList), curSelectedIndex(defaultCurSelectedIndex), defaultCurSelectedIndex(defaultCurSelectedIndex)
	{
	}

	void addMenuData(std::shared_ptr<menuData> curMenuData)
	{
		menuDataPtrList.push_back(curMenuData);
	}

	std::shared_ptr<menuData>& getSelectedMenuData();

	int getMinVisibleMenuDataIndex(int curMenuDataIndex);

	int getMaxVisibleMenuDataIndex(int curMenuDataIndex);

	void resetToDefault();

private:
	int defaultCurSelectedIndex;
};

class menuGridData
{
public:
	std::vector<std::shared_ptr<menuColumnData>> menuColumnsPtrList;
	int curSelectedColumnIndex;
	std::shared_ptr<menuGridData> prevMenu;
	menuFunc onEnterFunc;
	menuFunc onReturnFunc;
	menuFunc drawFunc;

	menuGridData() : curSelectedColumnIndex(0), defaultCurSelectedColumnIndex(0), prevMenu(nullptr), onEnterFunc(nullptr), onReturnFunc(nullptr), drawFunc(nullptr)
	{
	}

	menuGridData(std::vector<std::shared_ptr<menuColumnData>> menuColumnsPtrList, int defaultCurSelectedColumnIndex, std::shared_ptr<menuGridData> prevMenu) :
		menuColumnsPtrList(menuColumnsPtrList), curSelectedColumnIndex(defaultCurSelectedColumnIndex), defaultCurSelectedColumnIndex(defaultCurSelectedColumnIndex), prevMenu(prevMenu), onEnterFunc(nullptr), onReturnFunc(nullptr), drawFunc(nullptr)
	{
	}

	menuGridData(std::vector<std::shared_ptr<menuColumnData>> menuColumnsPtrList, int defaultCurSelectedColumnIndex, std::shared_ptr<menuGridData> prevMenu, menuFunc onEnterFunc, menuFunc onReturnFunc, menuFunc drawFunc) :
		menuColumnsPtrList(menuColumnsPtrList), curSelectedColumnIndex(defaultCurSelectedColumnIndex), defaultCurSelectedColumnIndex(defaultCurSelectedColumnIndex), prevMenu(prevMenu), onEnterFunc(onEnterFunc), onReturnFunc(onReturnFunc), drawFunc(drawFunc)
	{
	}

	void addMenuColumn(std::shared_ptr<menuColumnData> menuColumnPtr)
	{
		menuColumnsPtrList.push_back(menuColumnPtr);
	}

	void draw(CInstance* Self);

	void processInput(bool isMouseLeftPressed, bool isMouseRightPressed, bool isActionOnePressed, bool isActionTwoPressed, bool isEnterPressed, bool isEscPressed,
		bool isMoveUpPressed, bool isMoveDownPressed, bool isMoveLeftPressed, bool isMoveRightPressed);

	std::shared_ptr<menuColumnData>& getSelectedMenuColumn();

	int getMinVisibleMenuColumnIndex(int curMenuColumnIndex);

	int getMaxVisibleMenuColumnIndex(int curMenuColumnIndex);

	void resetMenu();

private:
	int defaultCurSelectedColumnIndex;
};

struct spriteData
{
	std::string spritePath;
	std::string spriteFileName;
	RValue spriteRValue;
	int numFrames;

	spriteData(std::string spritePath, std::string spriteFileName, int numFrames) : spritePath(spritePath), spriteFileName(spriteFileName), numFrames(numFrames)
	{
		spriteRValue = g_ModuleInterface->CallBuiltin("sprite_add", { spritePath.c_str(), numFrames, false, false, 0, 0 });
	}

	~spriteData()
	{
		if (spriteRValue.m_Kind != VALUE_UNDEFINED)
		{
			g_ModuleInterface->CallBuiltin("sprite_delete", { spriteRValue });
		}
	}
};