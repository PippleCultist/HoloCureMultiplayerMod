#include "MessageStructs.h"
#include "NetworkFunctions.h"

int receiveString(uint32_t playerID, std::string* outputString)
{
	char messageLenArr[2];
	short messageLen = -1;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, messageLenArr, sizeof(messageLenArr))) <= 0)
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
	if ((result = receiveBytesFromPlayer(playerID, inputMessage, messageLen)) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToString(outputString, inputMessage, messageLen, startBufferPos);
	// TODO: Unsure if removing this causes a memory leak or not. It seems like deleting this here would cause a use after free error when trying to display the options
//	delete[] inputMessage;
	return result;
}

int receiveStringView(uint32_t playerID, std::string_view* outputString)
{
	char messageLenArr[2];
	short messageLen = -1;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, messageLenArr, sizeof(messageLenArr))) <= 0)
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
	if ((result = receiveBytesFromPlayer(playerID, inputMessage, messageLen)) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToStringView(outputString, inputMessage, messageLen, startBufferPos);
	// TODO: Unsure if removing this causes a memory leak or not. It seems like deleting this here would cause a use after free error when trying to display the options
//	delete[] inputMessage;
	return result;
}

int levelUpOption::receiveMessage(uint32_t playerID)
{
	int messageLen = 0;
	int result = -1;
	char modsListSize = -1;
	if ((result = receiveBytesFromPlayer(playerID, &modsListSize, sizeof(modsListSize))) <= 0)
	{
		return result;
	}
	messageLen++;
	for (int i = 0; i < static_cast<int>(modsListSize); i++)
	{
		std::string_view curGainedMod;
		if ((result = receiveStringView(playerID, &curGainedMod)) <= 0)
		{
			return result;
		}
		modsList.push_back(std::move(curGainedMod));
		messageLen += result;
	}

	char optionDescriptionSize = -1;
	if ((result = receiveBytesFromPlayer(playerID, &optionDescriptionSize, sizeof(optionDescriptionSize))) <= 0)
	{
		return result;
	}
	messageLen++;
	for (int i = 0; i < static_cast<int>(optionDescriptionSize); i++)
	{
		std::string_view curOptionDescription;
		if ((result = receiveStringView(playerID, &curOptionDescription)) <= 0)
		{
			return result;
		}
		optionDescription.push_back(std::move(curOptionDescription));
		messageLen += result;
	}

	if ((result = receiveStringView(playerID, &optionType)) <= 0)
	{
		return result;
	}
	messageLen += result;
	if ((result = receiveStringView(playerID, &optionName)) <= 0)
	{
		return result;
	}
	messageLen += result;
	if ((result = receiveStringView(playerID, &optionID)) <= 0)
	{
		return result;
	}
	messageLen += result;

	char optionIconArr[2];
	short optionIconShort = -1;
	if ((result = receiveBytesFromPlayer(playerID, optionIconArr, sizeof(optionIconArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToShort(&optionIconShort, optionIconArr, startBufferPos);
	optionIcon = static_cast<uint16_t>(optionIconShort);
	messageLen += result;

	char optionIconSuperArr[2];
	short optionIconSuperShort = -1;
	if ((result = receiveBytesFromPlayer(playerID, optionIconSuperArr, sizeof(optionIconSuperArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&optionIconSuperShort, optionIconSuperArr, startBufferPos);
	optionIcon_Super = static_cast<uint16_t>(optionIconSuperShort);
	messageLen += result;
	if ((result = receiveBytesFromPlayer(playerID, &weaponAndItemType, sizeof(weaponAndItemType))) <= 0)
	{
		return result;
	}
	messageLen++;
	return messageLen;
}

int messageLevelUpOptions::receiveMessage(uint32_t playerID)
{
	int curMessageSize = 0;
	int result = -1;
	for (int i = 0; i < 4; i++)
	{
		if ((result = optionArr[i].receiveMessage(playerID)) <= 0)
		{
			return result;
		}
		curMessageSize += result;
	}
	return curMessageSize;
}

int messageLevelUpClientChoice::receiveMessage(uint32_t playerID)
{
	int curMessageSize = sizeof(levelUpOption);
	int result = -1;
	char receivedLevelUpOption = 0;
	if ((result = receiveBytesFromPlayer(playerID, &receivedLevelUpOption, curMessageSize)) <= 0)
	{
		return result;
	}
	levelUpOption = receivedLevelUpOption;
	return curMessageSize;
}

int messageDestructableCreate::receiveMessage(uint32_t playerID)
{
	int result = -1;
	int curMessageSize = 0;
	char messageArr[4];
	if ((result = receiveBytesFromPlayer(playerID, messageArr, sizeof(messageArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToFloat(&data.xPos, messageArr, startBufferPos);
	curMessageSize += result;
	if ((result = receiveBytesFromPlayer(playerID, messageArr, sizeof(messageArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToFloat(&data.yPos, messageArr, startBufferPos);
	curMessageSize += result;
	char shortArr[2];
	if ((result = receiveBytesFromPlayer(playerID, shortArr, sizeof(shortArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&data.id, shortArr, startBufferPos);
	curMessageSize += result;
	if ((result = receiveBytesFromPlayer(playerID, &data.pillarType, sizeof(data.pillarType))) <= 0)
	{
		return result;
	}
	curMessageSize += result;
	return curMessageSize;
}

int messageDestructableBreak::receiveMessage(uint32_t playerID)
{
	int result = -1;
	int curMessageSize = 0;
	char messageArr[4];
	if ((result = receiveBytesFromPlayer(playerID, messageArr, sizeof(messageArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToFloat(&data.xPos, messageArr, startBufferPos);
	curMessageSize += result;
	if ((result = receiveBytesFromPlayer(playerID, messageArr, sizeof(messageArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToFloat(&data.yPos, messageArr, startBufferPos);
	curMessageSize += result;
	char shortArr[2];
	if ((result = receiveBytesFromPlayer(playerID, shortArr, sizeof(shortArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&data.id, shortArr, startBufferPos);
	curMessageSize += result;
	if ((result = receiveBytesFromPlayer(playerID, &data.pillarType, sizeof(data.pillarType))) <= 0)
	{
		return result;
	}
	curMessageSize += result;
	return curMessageSize;
}

int messageEliminateLevelUpClientChoice::receiveMessage(uint32_t playerID)
{
	int curMessageSize = sizeof(levelUpOption);
	int result = -1;
	char receivedLevelUpOption = 0;
	if ((result = receiveBytesFromPlayer(playerID, &receivedLevelUpOption, curMessageSize)) <= 0)
	{
		return result;
	}
	levelUpOption = receivedLevelUpOption;
	return curMessageSize;
}

int messageCautionCreate::receiveMessage(uint32_t playerID)
{
	const int curMessageSize = sizeof(messageCautionCreate);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytesFromPlayer(playerID, messageBuffer, curMessageSize)) <= 0)
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

int messagePreCreateUpdate::receiveMessage(uint32_t playerID)
{
	const int curMessageSize = sizeof(messagePreCreateUpdate);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytesFromPlayer(playerID, messageBuffer, curMessageSize)) <= 0)
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

int messageVfxUpdate::receiveMessage(uint32_t playerID)
{
	const int curMessageSize = sizeof(messageVfxUpdate);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytesFromPlayer(playerID, messageBuffer, curMessageSize)) <= 0)
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

int messageInteractableCreate::receiveMessage(uint32_t playerID)
{
	const int curMessageSize = sizeof(messageInteractableCreate);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytesFromPlayer(playerID, messageBuffer, curMessageSize)) <= 0)
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

int messageInteractableDelete::receiveMessage(uint32_t playerID)
{
	const int curMessageSize = sizeof(messageInteractableDelete);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytesFromPlayer(playerID, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToShort(&id, messageBuffer, startBufferPos);
	readByteBufferToChar(&type, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messageInteractablePlayerInteracted::receiveMessage(uint32_t playerID)
{
	const int curMessageSize = sizeof(messageInteractablePlayerInteracted);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytesFromPlayer(playerID, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&m_playerID, messageBuffer, startBufferPos);
	readByteBufferToShort(&id, messageBuffer, startBufferPos);
	readByteBufferToChar(&type, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messageStickerPlayerInteracted::receiveMessage(uint32_t playerID)
{
	int messageLen = 0;
	int result = -1;
	if ((result = receiveStringView(playerID, &stickerID)) <= 0)
	{
		return result;
	}
	messageLen += result;

	char playerIDArr[4];
	if ((result = receiveBytesFromPlayer(playerID, playerIDArr, sizeof(playerIDArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&m_playerID, playerIDArr, startBufferPos);
	messageLen += result;

	char idArr[2];
	if ((result = receiveBytesFromPlayer(playerID, idArr, sizeof(idArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&id, idArr, startBufferPos);
	messageLen += result;
	return messageLen;
}

int messageBoxPlayerInteracted::receiveMessage(uint32_t playerID)
{
	int messageLen = 0;
	int result = -1;
	for (int i = 0; i < 3; i++)
	{
		if ((result = randomWeapons[i].receiveMessage(playerID)) <= 0)
		{
			return result;
		}
		messageLen += result;
	}

	char playerIDArr[4];
	if ((result = receiveBytesFromPlayer(playerID, playerIDArr, sizeof(playerIDArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&m_playerID, playerIDArr, startBufferPos);
	messageLen += result;

	char idArr[2];
	if ((result = receiveBytesFromPlayer(playerID, idArr, sizeof(idArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&id, idArr, startBufferPos);
	messageLen += result;
	if ((result = receiveBytesFromPlayer(playerID, &boxItemAmount, sizeof(boxItemAmount))) <= 0)
	{
		return result;
	}
	messageLen++;
	if ((result = receiveBytesFromPlayer(playerID, &isSuperBox, sizeof(isSuperBox))) <= 0)
	{
		return result;
	}
	messageLen++;
	return messageLen;
}

int messageBoxTakeOption::receiveMessage(uint32_t playerID)
{
	const int curMessageSize = sizeof(boxItemNum);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytesFromPlayer(playerID, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToChar(&boxItemNum, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messageAnvilChooseOption::receiveMessage(uint32_t playerID)
{
	int messageLen = 0;
	int result = -1;
	if ((result = receiveStringView(playerID, &optionID)) <= 0)
	{
		return result;
	}
	messageLen += result;
	if ((result = receiveStringView(playerID, &optionType)) <= 0)
	{
		return result;
	}
	messageLen += result;

	char coinCostArr[4];
	if ((result = receiveBytesFromPlayer(playerID, coinCostArr, sizeof(coinCostArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&coinCost, coinCostArr, startBufferPos);
	messageLen += result;
	if ((result = receiveBytesFromPlayer(playerID, &anvilOptionType, sizeof(anvilOptionType))) <= 0)
	{
		return result;
	}
	messageLen++;
	return messageLen;
}

int messageClientGainMoney::receiveMessage(uint32_t playerID)
{
	const int curMessageSize = sizeof(messageClientGainMoney);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytesFromPlayer(playerID, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&money, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messageClientAnvilEnchant::receiveMessage(uint32_t playerID)
{
	int result = -1;
	char numGainedMods = -1;
	int messageLen = 0;
	if ((result = receiveBytesFromPlayer(playerID, &numGainedMods, sizeof(numGainedMods))) <= 0)
	{
		return result;
	}
	messageLen++;
	for (int i = 0; i < static_cast<int>(numGainedMods); i++)
	{
		std::string_view curGainedMod;
		if ((result = receiveStringView(playerID, &curGainedMod)) <= 0)
		{
			return result;
		}
		gainedMods.push_back(std::move(curGainedMod));
		messageLen += result;
	}
	if ((result = receiveStringView(playerID, &optionID)) <= 0)
	{
		return result;
	}
	messageLen += result;
	char coinCostArr[4];
	if ((result = receiveBytesFromPlayer(playerID, coinCostArr, sizeof(coinCostArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&coinCost, coinCostArr, startBufferPos);
	messageLen += result;
	return messageLen;
}

int messageStickerChooseOption::receiveMessage(uint32_t playerID)
{
	const int curMessageSize = sizeof(stickerOption) + sizeof(stickerOptionType);
	int result = -1;
	char messageBuffer[curMessageSize];
	if ((result = receiveBytesFromPlayer(playerID, messageBuffer, curMessageSize)) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToChar(&stickerOption, messageBuffer, startBufferPos);
	readByteBufferToChar(&stickerOptionType, messageBuffer, startBufferPos);
	return curMessageSize;
}

int messageChooseCollab::receiveMessage(uint32_t playerID)
{
	int messageLen = 0;
	int result = -1;
	if ((result = collab.receiveMessage(playerID)) <= 0)
	{
		return result;
	}
	messageLen += result;
	return messageLen;
}

int buffData::receiveMessage(uint32_t playerID)
{
	int messageLen = 0;
	int result = -1;
	if ((result = receiveStringView(playerID, &buffName)) <= 0)
	{
		return result;
	}
	messageLen += result;

	char timerArr[2];
	if ((result = receiveBytesFromPlayer(playerID, timerArr, sizeof(timerArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToShort(&timer, timerArr, startBufferPos);
	messageLen += result;

	char stacksArr[2];
	if ((result = receiveBytesFromPlayer(playerID, stacksArr, sizeof(stacksArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&stacks, stacksArr, startBufferPos);
	messageLen += result;

	return messageLen;
}

int messageBuffData::receiveMessage(uint32_t playerID)
{
	int result = -1;
	int messageLen = 0;
	char numBuffs = -1;
	if ((result = receiveBytesFromPlayer(playerID, &numBuffs, sizeof(numBuffs))) <= 0)
	{
		return result;
	}
	messageLen++;
	for (int i = 0; i < static_cast<int>(numBuffs); i++)
	{
		buffData curBuffData;
		if ((result = curBuffData.receiveMessage(playerID)) <= 0)
		{
			return result;
		}
		buffDataList.push_back(curBuffData);
		messageLen += result;
	}

	return messageLen;
}

int lobbyPlayerData::receiveMessage(uint32_t playerID)
{
	int messageLen = 0;
	int result = -1;
	if ((result = receiveStringView(playerID, &charName)) <= 0)
	{
		return result;
	}
	messageLen += result;

	if ((result = receiveString(playerID, &playerName)) <= 0)
	{
		return result;
	}
	messageLen += result;

	char stageSpriteArr[2];
	if ((result = receiveBytesFromPlayer(playerID, stageSpriteArr, sizeof(stageSpriteArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToShort(&stageSprite, stageSpriteArr, startBufferPos);
	messageLen += result;

	if ((result = receiveBytesFromPlayer(playerID, &isReady, sizeof(isReady))) <= 0)
	{
		return result;
	}
	messageLen++;

	return messageLen;
}

int messageCharData::receiveMessage(uint32_t playerID)
{
	int messageLen = 0;
	int result = -1;
	if ((result = playerData.receiveMessage(playerID)) <= 0)
	{
		return result;
	}
	messageLen += result;

	char playerIDArr[4];
	if ((result = receiveBytesFromPlayer(playerID, playerIDArr, sizeof(playerIDArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&m_playerID, playerIDArr, startBufferPos);
	messageLen += result;
	return messageLen;
}

int messageLobbyPlayerDisconnected::receiveMessage(uint32_t playerID)
{
	int messageLen = 0;
	int result = -1;

	char playerIDArr[4];
	if ((result = receiveBytesFromPlayer(playerID, playerIDArr, sizeof(playerIDArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToLong(&m_playerID, playerIDArr, startBufferPos);
	messageLen += result;
	return messageLen;
}

int messageInstancesUpdate::receiveMessage(uint32_t playerID)
{
	int messageLen = 0;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, &numInstances, sizeof(numInstances))) <= 0)
	{
		return result;
	}
	messageLen++;

	for (int i = 0; i < numInstances; i++)
	{
		instanceData& curInstance = data[i];
		char xPosArr[4];
		if ((result = receiveBytesFromPlayer(playerID, xPosArr, sizeof(xPosArr))) <= 0)
		{
			return result;
		}
		int startBufferPos = 0;
		readByteBufferToFloat(&curInstance.xPos, xPosArr, startBufferPos);
		messageLen += result;

		char yPosArr[4];
		if ((result = receiveBytesFromPlayer(playerID, yPosArr, sizeof(yPosArr))) <= 0)
		{
			return result;
		}
		startBufferPos = 0;
		readByteBufferToFloat(&curInstance.yPos, yPosArr, startBufferPos);
		messageLen += result;

		char spriteIndexArr[2];
		if ((result = receiveBytesFromPlayer(playerID, spriteIndexArr, sizeof(spriteIndexArr))) <= 0)
		{
			return result;
		}
		startBufferPos = 0;
		readByteBufferToShort(&curInstance.spriteIndex, spriteIndexArr, startBufferPos);
		messageLen += result;

		char instanceIDArr[2];
		if ((result = receiveBytesFromPlayer(playerID, instanceIDArr, sizeof(instanceIDArr))) <= 0)
		{
			return result;
		}
		startBufferPos = 0;
		readByteBufferToShort(&curInstance.instanceID, instanceIDArr, startBufferPos);
		messageLen += result;

		if ((result = receiveBytesFromPlayer(playerID, &curInstance.truncatedImageIndex, sizeof(curInstance.truncatedImageIndex))) <= 0)
		{
			return result;
		}
		messageLen++;
	}
	return messageLen;
}

int messageAttackUpdate::receiveMessage(uint32_t playerID)
{
	int messageLen = 0;
	int result = -1;
	if ((result = receiveBytesFromPlayer(playerID, &numAttacks, sizeof(numAttacks))) <= 0)
	{
		return result;
	}
	messageLen++;

	for (int i = 0; i < numAttacks; i++)
	{
		attackData& curAttack = data[i];
		char xPosArr[4];
		if ((result = receiveBytesFromPlayer(playerID, xPosArr, sizeof(xPosArr))) <= 0)
		{
			return result;
		}
		int startBufferPos = 0;
		readByteBufferToFloat(&curAttack.xPos, xPosArr, startBufferPos);
		messageLen += result;

		char yPosArr[4];
		if ((result = receiveBytesFromPlayer(playerID, yPosArr, sizeof(yPosArr))) <= 0)
		{
			return result;
		}
		startBufferPos = 0;
		readByteBufferToFloat(&curAttack.yPos, yPosArr, startBufferPos);
		messageLen += result;

		char imageAngleApproxArr[2];
		if ((result = receiveBytesFromPlayer(playerID, imageAngleApproxArr, sizeof(imageAngleApproxArr))) <= 0)
		{
			return result;
		}
		startBufferPos = 0;
		short imageAngleApprox = 0;
		readByteBufferToShort(&imageAngleApprox, imageAngleApproxArr, startBufferPos);
		curAttack.imageAngle = imageAngleApprox / 10.0f;
		messageLen += result;

		char instanceIDArr[2];
		if ((result = receiveBytesFromPlayer(playerID, instanceIDArr, sizeof(instanceIDArr))) <= 0)
		{
			return result;
		}
		startBufferPos = 0;
		readByteBufferToShort(&curAttack.instanceID, instanceIDArr, startBufferPos);
		messageLen += result;

		char imageAlphaApprox = 0;
		if ((result = receiveBytesFromPlayer(playerID, &imageAlphaApprox, sizeof(imageAlphaApprox))) <= 0)
		{
			return result;
		}
		curAttack.imageAlpha = ((static_cast<int>(imageAlphaApprox) + 256) % 256) / 256.0f;
		messageLen++;

		if ((result = receiveBytesFromPlayer(playerID, &curAttack.truncatedImageIndex, sizeof(curAttack.truncatedImageIndex))) <= 0)
		{
			return result;
		}
		messageLen++;
	}
	return messageLen;
}

int messageKaelaOreAmount::receiveMessage(uint32_t playerID)
{
	int messageLen = 0;
	int result = -1;

	char oreAArr[2];
	if ((result = receiveBytesFromPlayer(playerID, oreAArr, sizeof(oreAArr))) <= 0)
	{
		return result;
	}
	int startBufferPos = 0;
	readByteBufferToShort(&oreA, oreAArr, startBufferPos);
	messageLen += result;

	char oreBArr[2];
	if ((result = receiveBytesFromPlayer(playerID, oreBArr, sizeof(oreBArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&oreB, oreBArr, startBufferPos);
	messageLen += result;

	char oreCArr[2];
	if ((result = receiveBytesFromPlayer(playerID, oreCArr, sizeof(oreCArr))) <= 0)
	{
		return result;
	}
	startBufferPos = 0;
	readByteBufferToShort(&oreC, oreCArr, startBufferPos);
	messageLen += result;

	return messageLen;
}