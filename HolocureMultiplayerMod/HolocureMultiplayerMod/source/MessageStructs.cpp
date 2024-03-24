#include "MessageStructs.h"
#include "NetworkFunctions.h"

int receiveString(SOCKET socket, std::string* outputString)
{
	char messageLenArr[2];
	short messageLen = -1;
	int result = -1;
	if ((result = receiveBytes(socket, messageLenArr, sizeof(messageLenArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToShort(&messageLen, messageLenArr, startBufferPos);
	if (messageLen == 0)
	{
		*outputString = std::string("");
		return result;
	}
	char* inputMessage = new char[static_cast<int>(messageLen) + 1];
	inputMessage[messageLen] = '\0';
	if ((result = receiveBytes(socket, inputMessage, messageLen)) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToString(outputString, inputMessage, messageLen, startBufferPos);
	// TODO: Unsure if removing this causes a memory leak or not. It seems like deleting this here would cause a use after free error when trying to display the options
//	delete[] inputMessage;
	return result;
}

int receiveStringView(SOCKET socket, std::string_view* outputString)
{
	char messageLenArr[2];
	short messageLen = -1;
	int result = -1;
	if ((result = receiveBytes(socket, messageLenArr, sizeof(messageLenArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToShort(&messageLen, messageLenArr, startBufferPos);
	if (messageLen == 0)
	{
		*outputString = std::string_view("");
		return result;
	}
	char* inputMessage = new char[static_cast<int>(messageLen) + 1];
	inputMessage[messageLen] = '\0';
	if ((result = receiveBytes(socket, inputMessage, messageLen)) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToStringView(outputString, inputMessage, messageLen, startBufferPos);
	// TODO: Unsure if removing this causes a memory leak or not. It seems like deleting this here would cause a use after free error when trying to display the options
//	delete[] inputMessage;
	return result;
}

int levelUpOption::receiveMessage(SOCKET socket)
{
	int messageLen = 0;
	int result = -1;
	char modsListSize = -1;
	if ((result = receiveBytes(socket, &modsListSize, sizeof(modsListSize))) <= 0)
	{
		return result;
	}
	messageLen++;
	for (int i = 0; i < static_cast<int>(modsListSize); i++)
	{
		std::string_view curGainedMod;
		if ((result = receiveStringView(socket, &curGainedMod)) <= 0)
		{
			return result;
		}
		modsList.push_back(std::move(curGainedMod));
		messageLen += result;
	}

	char optionDescriptionSize = -1;
	if ((result = receiveBytes(socket, &optionDescriptionSize, sizeof(optionDescriptionSize))) <= 0)
	{
		return result;
	}
	messageLen++;
	for (int i = 0; i < static_cast<int>(optionDescriptionSize); i++)
	{
		std::string_view curOptionDescription;
		if ((result = receiveStringView(socket, &curOptionDescription)) <= 0)
		{
			return result;
		}
		optionDescription.push_back(std::move(curOptionDescription));
		messageLen += result;
	}

	if ((result = receiveStringView(socket, &optionType)) <= 0)
	{
		return result;
	}
	messageLen += result;
	if ((result = receiveStringView(socket, &optionName)) <= 0)
	{
		return result;
	}
	messageLen += result;
	if ((result = receiveStringView(socket, &optionID)) <= 0)
	{
		return result;
	}
	messageLen += result;

	char optionIconArr[2];
	short optionIconShort = -1;
	if ((result = receiveBytes(socket, optionIconArr, sizeof(optionIconArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToShort(&optionIconShort, optionIconArr, startBufferPos);
	optionIcon = static_cast<uint16_t>(optionIconShort);
	messageLen += result;

	char optionIconSuperArr[2];
	short optionIconSuperShort = -1;
	if ((result = receiveBytes(socket, optionIconSuperArr, sizeof(optionIconSuperArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&optionIconSuperShort, optionIconSuperArr, startBufferPos);
	optionIcon_Super = static_cast<uint16_t>(optionIconSuperShort);
	messageLen += result;
	if ((result = receiveBytes(socket, &weaponAndItemType, sizeof(weaponAndItemType))) <= 0)
	{
		return result;
	}
	messageLen++;
	return messageLen;
}

int messageLevelUpOptions::receiveMessage(SOCKET socket)
{
	int curMessageSize = 0;
	int result = -1;
	for (int i = 0; i < 4; i++)
	{
		if ((result = optionArr[i].receiveMessage(socket)) <= 0)
		{
			return result;
		}
		curMessageSize += result;
	}
	return curMessageSize;
}

int messageLevelUpClientChoice::receiveMessage(SOCKET socket)
{
	int curMessageSize = sizeof(messageLevelUpClientChoice);
	int result = -1;
	char receivedLevelUpOption = 0;
	if ((result = receiveBytes(socket, &receivedLevelUpOption, curMessageSize)) <= 0)
	{
		return result;
	}
	levelUpOption = receivedLevelUpOption;
	return curMessageSize;
}

int messageDestructableCreate::receiveMessage(SOCKET socket)
{
	int result = -1;
	int curMessageSize = 0;
	char messageArr[4];
	if ((result = receiveBytes(socket, messageArr, sizeof(messageArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToFloat(&data.xPos, messageArr, startBufferPos);
	curMessageSize += result;
	if ((result = receiveBytes(socket, messageArr, sizeof(messageArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToFloat(&data.yPos, messageArr, startBufferPos);
	curMessageSize += result;
	char shortArr[2];
	if ((result = receiveBytes(socket, shortArr, sizeof(shortArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&data.id, shortArr, startBufferPos);
	curMessageSize += result;
	if ((result = receiveBytes(socket, &data.pillarType, sizeof(data.pillarType))) <= 0)
	{
		return result;
	}
	curMessageSize += result;
	return curMessageSize;
}

int messageDestructableBreak::receiveMessage(SOCKET socket)
{
	int result = -1;
	int curMessageSize = 0;
	char messageArr[4];
	if ((result = receiveBytes(socket, messageArr, sizeof(messageArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToFloat(&data.xPos, messageArr, startBufferPos);
	curMessageSize += result;
	if ((result = receiveBytes(socket, messageArr, sizeof(messageArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToFloat(&data.yPos, messageArr, startBufferPos);
	curMessageSize += result;
	char shortArr[2];
	if ((result = receiveBytes(socket, shortArr, sizeof(shortArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&data.id, shortArr, startBufferPos);
	curMessageSize += result;
	if ((result = receiveBytes(socket, &data.pillarType, sizeof(data.pillarType))) <= 0)
	{
		return result;
	}
	curMessageSize += result;
	return curMessageSize;
}

int messageEliminateLevelUpClientChoice::receiveMessage(SOCKET socket)
{
	int curMessageSize = sizeof(messageEliminateLevelUpClientChoice);
	int result = -1;
	char receivedLevelUpOption = 0;
	if ((result = receiveBytes(socket, &receivedLevelUpOption, curMessageSize)) <= 0)
	{
		return result;
	}
	levelUpOption = receivedLevelUpOption;
	return curMessageSize;
}

int messageCautionCreate::receiveMessage(SOCKET socket)
{
	const int curMessageSize = sizeof(messageCautionCreate);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytes(socket, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToFloat(&data.xPos, messageBuffer, startBufferPos);
	readByteBufferToFloat(&data.yPos, messageBuffer, startBufferPos);
	readByteBufferToShort(&data.dir, messageBuffer, startBufferPos);
	readByteBufferToChar(&data.cautionType, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messagePreCreateUpdate::receiveMessage(SOCKET socket)
{
	const int curMessageSize = sizeof(messagePreCreateUpdate);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytes(socket, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToFloat(&data.xPos, messageBuffer, startBufferPos);
	readByteBufferToFloat(&data.yPos, messageBuffer, startBufferPos);
	readByteBufferToShort(&data.waitSpawn, messageBuffer, startBufferPos);
	readByteBufferToShort(&data.id, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messageVfxUpdate::receiveMessage(SOCKET socket)
{
	const int curMessageSize = sizeof(messageVfxUpdate);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytes(socket, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToFloat(&data.xPos, messageBuffer, startBufferPos);
	readByteBufferToFloat(&data.yPos, messageBuffer, startBufferPos);
	readByteBufferToFloat(&data.imageXScale, messageBuffer, startBufferPos);
	readByteBufferToFloat(&data.imageYScale, messageBuffer, startBufferPos);
	readByteBufferToFloat(&data.imageAngle, messageBuffer, startBufferPos);
	readByteBufferToFloat(&data.imageAlpha, messageBuffer, startBufferPos);
	readByteBufferToLong(&data.color, messageBuffer, startBufferPos);
	readByteBufferToShort(&data.spriteIndex, messageBuffer, startBufferPos);
	readByteBufferToShort(&data.imageIndex, messageBuffer, startBufferPos);
	readByteBufferToShort(&data.id, messageBuffer, startBufferPos);
	readByteBufferToChar(&data.type, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messageInteractableCreate::receiveMessage(SOCKET socket)
{
	const int curMessageSize = sizeof(messageInteractableCreate);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytes(socket, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToFloat(&data.xPos, messageBuffer, startBufferPos);
	readByteBufferToFloat(&data.yPos, messageBuffer, startBufferPos);
	readByteBufferToShort(&data.id, messageBuffer, startBufferPos);
	readByteBufferToShort(&data.spriteIndex, messageBuffer, startBufferPos);
	readByteBufferToChar(&data.type, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messageInteractableDelete::receiveMessage(SOCKET socket)
{
	const int curMessageSize = sizeof(messageInteractableDelete);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytes(socket, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToShort(&id, messageBuffer, startBufferPos);
	readByteBufferToChar(&type, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messageInteractablePlayerInteracted::receiveMessage(SOCKET socket)
{
	const int curMessageSize = sizeof(messageInteractablePlayerInteracted);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytes(socket, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&playerID, messageBuffer, startBufferPos);
	readByteBufferToShort(&id, messageBuffer, startBufferPos);
	readByteBufferToChar(&type, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messageStickerPlayerInteracted::receiveMessage(SOCKET socket)
{
	int messageLen = 0;
	int result = -1;
	if ((result = receiveStringView(socket, &stickerID)) <= 0)
	{
		return result;
	}
	messageLen += result;

	char playerIDArr[4];
	if ((result = receiveBytes(socket, playerIDArr, sizeof(playerIDArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&playerID, playerIDArr, startBufferPos);
	messageLen += result;

	char idArr[2];
	if ((result = receiveBytes(socket, idArr, sizeof(idArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&id, idArr, startBufferPos);
	messageLen += result;
	return messageLen;
}

int messageBoxPlayerInteracted::receiveMessage(SOCKET socket)
{
	int messageLen = 0;
	int result = -1;
	for (int i = 0; i < 3; i++)
	{
		if ((result = randomWeapons[i].receiveMessage(socket)) <= 0)
		{
			return result;
		}
		messageLen += result;
	}

	char playerIDArr[4];
	if ((result = receiveBytes(socket, playerIDArr, sizeof(playerIDArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&playerID, playerIDArr, startBufferPos);
	messageLen += result;

	char idArr[2];
	if ((result = receiveBytes(socket, idArr, sizeof(idArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&id, idArr, startBufferPos);
	messageLen += result;
	if ((result = receiveBytes(socket, &boxItemAmount, sizeof(boxItemAmount))) <= 0)
	{
		return result;
	}
	messageLen++;
	if ((result = receiveBytes(socket, &isSuperBox, sizeof(isSuperBox))) <= 0)
	{
		return result;
	}
	messageLen++;
	return messageLen;
}

int messageBoxTakeOption::receiveMessage(SOCKET socket)
{
	const int curMessageSize = sizeof(messageBoxTakeOption);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytes(socket, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToChar(&boxItemNum, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messageAnvilChooseOption::receiveMessage(SOCKET socket)
{
	int messageLen = 0;
	int result = -1;
	if ((result = receiveStringView(socket, &optionID)) <= 0)
	{
		return result;
	}
	messageLen += result;
	if ((result = receiveStringView(socket, &optionType)) <= 0)
	{
		return result;
	}
	messageLen += result;

	char coinCostArr[4];
	if ((result = receiveBytes(socket, coinCostArr, sizeof(coinCostArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&coinCost, coinCostArr, startBufferPos);
	messageLen += result;
	if ((result = receiveBytes(socket, &anvilOptionType, sizeof(anvilOptionType))) <= 0)
	{
		return result;
	}
	messageLen++;
	return messageLen;
}

int messageClientGainMoney::receiveMessage(SOCKET socket)
{
	const int curMessageSize = sizeof(messageClientGainMoney);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytes(socket, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&money, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messageClientAnvilEnchant::receiveMessage(SOCKET socket)
{
	int result = -1;
	char numGainedMods = -1;
	int messageLen = 0;
	if ((result = receiveBytes(socket, &numGainedMods, sizeof(numGainedMods))) <= 0)
	{
		return result;
	}
	messageLen++;
	for (int i = 0; i < static_cast<int>(numGainedMods); i++)
	{
		std::string_view curGainedMod;
		if ((result = receiveStringView(socket, &curGainedMod)) <= 0)
		{
			return result;
		}
		gainedMods.push_back(std::move(curGainedMod));
		messageLen += result;
	}
	if ((result = receiveStringView(socket, &optionID)) <= 0)
	{
		return result;
	}
	messageLen += result;
	char coinCostArr[4];
	if ((result = receiveBytes(socket, coinCostArr, sizeof(coinCostArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&coinCost, coinCostArr, startBufferPos);
	messageLen += result;
	return messageLen;
}

int messageStickerChooseOption::receiveMessage(SOCKET socket)
{
	const int curMessageSize = sizeof(messageStickerChooseOption);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytes(socket, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToChar(&stickerOption, messageBuffer, startBufferPos);
	readByteBufferToChar(&stickerOptionType, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messageChooseCollab::receiveMessage(SOCKET socket)
{
	int messageLen = 0;
	int result = -1;
	if ((result = collab.receiveMessage(socket)) <= 0)
	{
		return result;
	}
	messageLen += result;
	return messageLen;
}

int buffData::receiveMessage(SOCKET socket)
{
	int messageLen = 0;
	int result = -1;
	if ((result = receiveStringView(socket, &buffName)) <= 0)
	{
		return result;
	}
	messageLen += result;

	char timerArr[2];
	if ((result = receiveBytes(socket, timerArr, sizeof(timerArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToShort(&timer, timerArr, startBufferPos);
	messageLen += result;

	char stacksArr[2];
	if ((result = receiveBytes(socket, stacksArr, sizeof(stacksArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&stacks, stacksArr, startBufferPos);
	messageLen += result;

	return messageLen;
}

int messageBuffData::receiveMessage(SOCKET socket)
{
	int result = -1;
	int messageLen = 0;
	char numBuffs = -1;
	if ((result = receiveBytes(socket, &numBuffs, sizeof(numBuffs))) <= 0)
	{
		return result;
	}
	messageLen++;
	for (int i = 0; i < static_cast<int>(numBuffs); i++)
	{
		buffData curBuffData;
		if ((result = curBuffData.receiveMessage(socket)) <= 0)
		{
			return result;
		}
		buffDataList.push_back(curBuffData);
		messageLen += result;
	}
	
	return messageLen;
}

int lobbyPlayerData::receiveMessage(SOCKET socket)
{
	int messageLen = 0;
	int result = -1;
	if ((result = receiveStringView(socket, &charName)) <= 0)
	{
		return result;
	}
	messageLen += result;

	if ((result = receiveString(socket, &playerName)) <= 0)
	{
		return result;
	}
	messageLen += result;

	char stageSpriteArr[2];
	if ((result = receiveBytes(socket, stageSpriteArr, sizeof(stageSpriteArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToShort(&stageSprite, stageSpriteArr, startBufferPos);
	messageLen += result;

	if ((result = receiveBytes(socket, &isReady, sizeof(isReady))) <= 0)
	{
		return result;
	}
	messageLen++;

	return messageLen;
}

int messageCharData::receiveMessage(SOCKET socket)
{
	int messageLen = 0;
	int result = -1;
	if ((result = playerData.receiveMessage(socket)) <= 0)
	{
		return result;
	}
	messageLen += result;

	char playerIDArr[4];
	if ((result = receiveBytes(socket, playerIDArr, sizeof(playerIDArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&playerID, playerIDArr, startBufferPos);
	messageLen += result;
	return messageLen;
}

int messageLobbyPlayerDisconnected::receiveMessage(SOCKET socket)
{
	int messageLen = 0;
	int result = -1;

	char playerIDArr[4];
	if ((result = receiveBytes(socket, playerIDArr, sizeof(playerIDArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&playerID, playerIDArr, startBufferPos);
	messageLen += result;
	return messageLen;
}

int messageInstancesUpdate::receiveMessage(SOCKET socket)
{
	int messageLen = 0;
	int result = -1;
	if ((result = receiveBytes(socket, &numInstances, sizeof(numInstances))) <= 0)
	{
		return result;
	}
	messageLen++;

	for (int i = 0; i < numInstances; i++)
	{
		instanceData& curInstance = data[i];
		char xPosArr[4];
		if ((result = receiveBytes(socket, xPosArr, sizeof(xPosArr))) <= 0)
		{
			return result;
		}
		int startBufferPos = 0;
		readByteBufferToFloat(&curInstance.xPos, xPosArr, startBufferPos);
		messageLen += result;

		char yPosArr[4];
		if ((result = receiveBytes(socket, yPosArr, sizeof(yPosArr))) <= 0)
		{
			return result;
		}
		startBufferPos = 0;
		readByteBufferToFloat(&curInstance.yPos, yPosArr, startBufferPos);
		messageLen += result;

		char spriteIndexArr[2];
		if ((result = receiveBytes(socket, spriteIndexArr, sizeof(spriteIndexArr))) <= 0)
		{
			return result;
		}
		startBufferPos = 0;
		readByteBufferToShort(&curInstance.spriteIndex, spriteIndexArr, startBufferPos);
		messageLen += result;

		char instanceIDArr[2];
		if ((result = receiveBytes(socket, instanceIDArr, sizeof(instanceIDArr))) <= 0)
		{
			return result;
		}
		startBufferPos = 0;
		readByteBufferToShort(&curInstance.instanceID, instanceIDArr, startBufferPos);
		messageLen += result;

		if ((result = receiveBytes(socket, &curInstance.truncatedImageIndex, sizeof(curInstance.truncatedImageIndex))) <= 0)
		{
			return result;
		}
		messageLen++;
	}
	return messageLen;
}

int messageAttackUpdate::receiveMessage(SOCKET socket)
{
	int messageLen = 0;
	int result = -1;
	if ((result = receiveBytes(socket, &numAttacks, sizeof(numAttacks))) <= 0)
	{
		return result;
	}
	messageLen++;

	for (int i = 0; i < numAttacks; i++)
	{
		attackData& curAttack = data[i];
		char xPosArr[4];
		if ((result = receiveBytes(socket, xPosArr, sizeof(xPosArr))) <= 0)
		{
			return result;
		}
		int startBufferPos = 0;
		readByteBufferToFloat(&curAttack.xPos, xPosArr, startBufferPos);
		messageLen += result;

		char yPosArr[4];
		if ((result = receiveBytes(socket, yPosArr, sizeof(yPosArr))) <= 0)
		{
			return result;
		}
		startBufferPos = 0;
		readByteBufferToFloat(&curAttack.yPos, yPosArr, startBufferPos);
		messageLen += result;

		char imageAngleApproxArr[2];
		if ((result = receiveBytes(socket, imageAngleApproxArr, sizeof(imageAngleApproxArr))) <= 0)
		{
			return result;
		}
		startBufferPos = 0;
		short imageAngleApprox = 0;
		readByteBufferToShort(&imageAngleApprox, imageAngleApproxArr, startBufferPos);
		curAttack.imageAngle = imageAngleApprox / 10.0f;
		messageLen += result;

		char instanceIDArr[2];
		if ((result = receiveBytes(socket, instanceIDArr, sizeof(instanceIDArr))) <= 0)
		{
			return result;
		}
		startBufferPos = 0;
		readByteBufferToShort(&curAttack.instanceID, instanceIDArr, startBufferPos);
		messageLen += result;

		char imageAlphaApprox = 0;
		if ((result = receiveBytes(socket, &imageAlphaApprox, sizeof(imageAlphaApprox))) <= 0)
		{
			return result;
		}
		curAttack.imageAlpha = ((static_cast<int>(imageAlphaApprox) + 256) % 256) / 256.0f;
		messageLen++;

		if ((result = receiveBytes(socket, &curAttack.truncatedImageIndex, sizeof(curAttack.truncatedImageIndex))) <= 0)
		{
			return result;
		}
		messageLen++;
	}
	return messageLen;
}