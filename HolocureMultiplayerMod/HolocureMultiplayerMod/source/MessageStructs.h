#pragma once
#include "ModuleMain.h"

enum MessageTypes : char
{
	MESSAGE_INPUT_AIM = 0,
	MESSAGE_INPUT_NO_AIM,
	MESSAGE_INPUT_MOUSEFOLLOW,
	MESSAGE_ROOM,
	MESSAGE_INSTANCES_CREATE,
	MESSAGE_INSTANCES_UPDATE,
	MESSAGE_INSTANCES_DELETE,
	MESSAGE_CLIENT_PLAYER_DATA,
	MESSAGE_ATTACK_CREATE,
	MESSAGE_ATTACK_UPDATE,
	MESSAGE_ATTACK_DELETE,
	MESSAGE_CLIENT_ID,
	MESSAGE_PICKUPABLE_CREATE,
	MESSAGE_PICKUPABLE_UPDATE,
	MESSAGE_PICKUPABLE_DELETE,
	MESSAGE_GAME_DATA,
	MESSAGE_LEVEL_UP_OPTIONS,
	MESSAGE_LEVEL_UP_CLIENT_CHOICE,
	MESSAGE_PING,
	MESSAGE_PONG,
	MESSAGE_DESTRUCTABLE_CREATE,
	MESSAGE_DESTRUCTABLE_BREAK,
	MESSAGE_ELIMINATE_LEVEL_UP_CLIENT_CHOICE,
	MESSAGE_CLIENT_SPECIAL_ATTACK,
	MESSAGE_CAUTION_CREATE,
	MESSAGE_PRECREATE_UPDATE,
	MESSAGE_VFX_UPDATE,
	MESSAGE_INTERACTABLE_CREATE,
	MESSAGE_INTERACTABLE_DELETE,
	MESSAGE_INTERACTABLE_PLAYER_INTERACTED,
	MESSAGE_STICKER_PLAYER_INTERACTED,
	MESSAGE_BOX_PLAYER_INTERACTED,
	MESSAGE_INTERACT_FINISHED,
	MESSAGE_BOX_TAKE_OPTION,
	MESSAGE_ANVIL_CHOOSE_OPTION,
	MESSAGE_CLIENT_GAIN_MONEY,
	MESSAGE_CLIENT_ANVIL_ENCHANT,
	MESSAGE_STICKER_CHOOSE_OPTION,
	MESSAGE_CHOOSE_COLLAB,
	MESSAGE_BUFF_DATA,
	MESSAGE_CHAR_DATA,
	MESSAGE_RETURN_TO_LOBBY,
	MESSAGE_LOBBY_PLAYER_DISCONNECTED,
	MESSAGE_HOST_HAS_PAUSED,
	MESSAGE_HOST_HAS_UNPAUSED,
	MESSAGE_KAELA_ORE_AMOUNT,
	MESSAGE_INVALID
};

// messageBuffer param for message constructors won't include the message type at the beginning

inline void writeFloatToByteBuffer(char* outputBuffer, float inputFloat, int& startPos)
{
	uint32_t tempBytes;
	memcpy(&tempBytes, &inputFloat, 4);
	reinterpret_cast<uint32_t*>(&outputBuffer[startPos])[0] = htonl(tempBytes);
	startPos += 4;
}

inline void readByteBufferToFloat(float* outputFloat, char* inputBuffer, int& startPos)
{
	uint32_t tempBytes;
	memcpy(&tempBytes, &inputBuffer[startPos], 4);
	tempBytes = ntohl(tempBytes);
	memcpy(outputFloat, &tempBytes, 4);
	startPos += 4;
}

inline void writeShortToByteBuffer(char* outputBuffer, short inputShort, int& startPos)
{
	reinterpret_cast<uint16_t*>(&outputBuffer[startPos])[0] = htons(inputShort);
	startPos += 2;
}

inline void readByteBufferToShort(short* outputShort, char* inputBuffer, int& startPos)
{
	*outputShort = ntohs(reinterpret_cast<uint16_t*>(&inputBuffer[startPos])[0]);
	startPos += 2;
}

inline void writeLongToByteBuffer(char* outputBuffer, uint32_t inputLong, int& startPos)
{
	reinterpret_cast<uint32_t*>(&outputBuffer[startPos])[0] = htonl(inputLong);
	startPos += 4;
}

inline void readByteBufferToLong(uint32_t* outputLong, char* inputBuffer, int& startPos)
{
	*outputLong = ntohl(reinterpret_cast<uint32_t*>(&inputBuffer[startPos])[0]);
	startPos += 4;
}

inline void writeStringToByteBuffer(char* outputBuffer, std::string inputString, int& startPos)
{
	writeShortToByteBuffer(outputBuffer, static_cast<short>(inputString.size()), startPos);
	memcpy(&outputBuffer[startPos], inputString.data(), inputString.size());
	startPos += static_cast<int>(inputString.size());
}

inline void readByteBufferToString(std::string* outputString, char* inputBuffer, int inputBufferLen, int& startPos)
{
	*outputString = std::string(&inputBuffer[startPos], inputBufferLen);
	startPos += inputBufferLen;
}

inline void writeCharToByteBuffer(char* outputBuffer, char inputChar, int& startPos)
{
	outputBuffer[startPos] = inputChar;
	startPos++;
}

inline void readByteBufferToChar(char* outputChar, char* inputBuffer, int& startPos)
{
	*outputChar = inputBuffer[startPos];
	startPos++;
}

inline bool checkBitInByte(char inputByte, int bitPos)
{
	if (bitPos >= 8)
	{
		LogPrint(CM_RED, "Trying to check index %d outside of valid byte range", bitPos);
		return false;
	}
	return (inputByte & (1 << bitPos)) != 0;
}

inline void setBitInByte(char& inputByte, int bitPos)
{
	if (bitPos >= 8)
	{
		LogPrint(CM_RED, "Trying to set index %d outside of valid byte range", bitPos);
	}
	inputByte |= (1 << bitPos);
}

int receiveString(uint32_t playerID, std::string* outputString);

// Probably should optimize this/make a separate struct for the updates
// TODO: Improve this by reducing the xPos,yPos to a diff + round + truncate.
struct instanceData
{
	float xPos;
	float yPos;
	short xPosDiff;
	short yPosDiff;
	float imageXScale;
	float imageYScale;
	short spriteIndex;
	short instanceID;
	char truncatedImageIndex;
	char hasVarChanged;
	int frameCount;

	instanceData() : xPos(0), yPos(0), xPosDiff(0), yPosDiff(0), imageXScale(0), imageYScale(0), spriteIndex(0), instanceID(0), truncatedImageIndex(0), hasVarChanged(0), frameCount(1)
	{
	}

	instanceData(float xPos, float yPos, short xPosDiff, short yPosDiff, float imageXScale, float imageYScale, short spriteIndex, short instanceID, char truncatedImageIndex, char hasVarChanged, int frameCount) :
		xPos(xPos), yPos(yPos), xPosDiff(xPosDiff), yPosDiff(yPosDiff), imageXScale(imageXScale), imageYScale(imageYScale), spriteIndex(spriteIndex),
		instanceID(instanceID), truncatedImageIndex(truncatedImageIndex), hasVarChanged(hasVarChanged), frameCount(frameCount)
	{
	}
};

const int instanceCreateDataLen = 20;

struct messageInstancesCreate
{
	instanceData data[instanceCreateDataLen]{};
	char numInstances;
	messageInstancesCreate() : numInstances(0)
	{
	}

	messageInstancesCreate(char* messageBuffer)
	{
		numInstances = messageBuffer[0];
		for (int i = 0; i < numInstances; i++)
		{
			int startBufferPos = i * sizeof(instanceData) + 1;
			readByteBufferToFloat(&data[i].xPos, messageBuffer, startBufferPos);
			readByteBufferToFloat(&data[i].yPos, messageBuffer, startBufferPos);
			readByteBufferToFloat(&data[i].imageXScale, messageBuffer, startBufferPos);
			readByteBufferToFloat(&data[i].imageYScale, messageBuffer, startBufferPos);
			readByteBufferToShort(&data[i].spriteIndex, messageBuffer, startBufferPos);
			readByteBufferToShort(&data[i].instanceID, messageBuffer, startBufferPos);
			readByteBufferToChar(&data[i].truncatedImageIndex, messageBuffer, startBufferPos);
		}
	}

	void addInstance(instanceData addInstanceData)
	{
		data[numInstances] = std::move(addInstanceData);
		numInstances++;
	}

	void serialize(char* messageBuffer)
	{
		// TODO: Maybe combine this with the constructor to avoid needing a redundant copy?
		messageBuffer[0] = MESSAGE_INSTANCES_CREATE;
		messageBuffer[1] = numInstances;
		for (int i = 0; i < numInstances; i++)
		{
			int startBufferPos = i * sizeof(instanceData) + 2;
			writeFloatToByteBuffer(messageBuffer, data[i].xPos, startBufferPos);
			writeFloatToByteBuffer(messageBuffer, data[i].yPos, startBufferPos);
			writeFloatToByteBuffer(messageBuffer, data[i].imageXScale, startBufferPos);
			writeFloatToByteBuffer(messageBuffer, data[i].imageYScale, startBufferPos);
			writeShortToByteBuffer(messageBuffer, data[i].spriteIndex, startBufferPos);
			writeShortToByteBuffer(messageBuffer, data[i].instanceID, startBufferPos);
			writeCharToByteBuffer(messageBuffer, data[i].truncatedImageIndex, startBufferPos);
		}
	}
};

const int instanceUpdateDataLen = 20;

// hasVarChanged
// byte 0 - has position changed
// byte 1 - is position update a diff or exact (0 = diff) (1 = exact)
// byte 2 - is diff using byte or short (0 = byte) (1 = short)
// byte 3 - has image scale changed
// byte 4 - has sprite index changed
struct messageInstancesUpdate
{
	instanceData data[instanceUpdateDataLen]{};
	char numInstances;
	messageInstancesUpdate() : numInstances(0)
	{
	}

	void addInstance(instanceData addInstanceData)
	{
		data[numInstances] = std::move(addInstanceData);
		numInstances++;
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_INSTANCES_UPDATE, startBufferPos);
		writeCharToByteBuffer(messageBuffer, numInstances, startBufferPos);
		for (int i = 0; i < numInstances; i++)
		{
			writeCharToByteBuffer(messageBuffer, data[i].hasVarChanged, startBufferPos);
			// Check if position has changed
			if (checkBitInByte(data[i].hasVarChanged, 0))
			{
				// Check if position is exact or diff
				if (checkBitInByte(data[i].hasVarChanged, 1))
				{
					writeFloatToByteBuffer(messageBuffer, data[i].xPos, startBufferPos);
					writeFloatToByteBuffer(messageBuffer, data[i].yPos, startBufferPos);
				}
				else
				{
					// Check if diff is short or byte
					if (checkBitInByte(data[i].hasVarChanged, 2))
					{
						writeShortToByteBuffer(messageBuffer, data[i].xPosDiff, startBufferPos);
						writeShortToByteBuffer(messageBuffer, data[i].yPosDiff, startBufferPos);
					}
					else
					{
						writeCharToByteBuffer(messageBuffer, static_cast<char>(data[i].xPosDiff), startBufferPos);
						writeCharToByteBuffer(messageBuffer, static_cast<char>(data[i].yPosDiff), startBufferPos);
					}
				}
			}
			if (checkBitInByte(data[i].hasVarChanged, 3))
			{
				writeFloatToByteBuffer(messageBuffer, data[i].imageXScale, startBufferPos);
				writeFloatToByteBuffer(messageBuffer, data[i].imageYScale, startBufferPos);
			}
			if (checkBitInByte(data[i].hasVarChanged, 4))
			{
				writeShortToByteBuffer(messageBuffer, data[i].spriteIndex, startBufferPos);
			}
			writeShortToByteBuffer(messageBuffer, data[i].instanceID, startBufferPos);
			writeCharToByteBuffer(messageBuffer, data[i].truncatedImageIndex, startBufferPos);
		}
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize++;

		size_t instanceDataSize = 0;
		for (int i = 0; i < numInstances; i++)
		{
			char hasVarChanged = data[i].hasVarChanged;
			instanceDataSize++;
			if (checkBitInByte(data[i].hasVarChanged, 0))
			{
				// Check if position is exact or diff
				if (checkBitInByte(data[i].hasVarChanged, 1))
				{
					instanceDataSize += 4;
					instanceDataSize += 4;
				}
				else
				{
					// Check if diff is short or byte
					if (checkBitInByte(data[i].hasVarChanged, 2))
					{
						instanceDataSize += 2;
						instanceDataSize += 2;
					}
					else
					{
						instanceDataSize++;
						instanceDataSize++;
					}
				}
			}
			if (checkBitInByte(data[i].hasVarChanged, 3))
			{
				instanceDataSize += 4;
				instanceDataSize += 4;
			}
			if (checkBitInByte(data[i].hasVarChanged, 4))
			{
				instanceDataSize += 2;
			}
			instanceDataSize += 2;
			instanceDataSize++;
		}
		curMessageSize += instanceDataSize;

		return curMessageSize;
	}
};

const int instanceDeleteDataLen = 20;

struct messageInstancesDelete
{
	short instanceIDArr[instanceDeleteDataLen]{};
	char numInstances;
	messageInstancesDelete() : numInstances(0)
	{
	}

	messageInstancesDelete(char* messageBuffer)
	{
		numInstances = messageBuffer[0];
		for (int i = 0; i < numInstances; i++)
		{
			int startBufferPos = i * 2 + 1;
			readByteBufferToShort(&instanceIDArr[i], messageBuffer, startBufferPos);
		}
	}

	void addInstance(short instanceID)
	{
		instanceIDArr[numInstances] = instanceID;
		numInstances++;
	}

	void serialize(char* messageBuffer)
	{
		messageBuffer[0] = MESSAGE_INSTANCES_DELETE;
		messageBuffer[1] = numInstances;
		for (int i = 0; i < numInstances; i++)
		{
			int startBufferPos = i * 2 + 2;
			writeShortToByteBuffer(messageBuffer, instanceIDArr[i], startBufferPos);
		}
	}
};

// Probably should optimize this/make a separate struct for the updates
struct attackData
{
	float xPos;
	float yPos;
	short xPosDiff;
	short yPosDiff;
	float imageXScale;
	float imageYScale;
	float imageAngle;
	float imageAlpha;
	short spriteIndex;
	short instanceID;
	char truncatedImageIndex;
	char hasVarChanged;
	int frameCount;

	attackData() : xPos(0), yPos(0), xPosDiff(0), yPosDiff(0), imageXScale(0), imageYScale(0), imageAngle(0), imageAlpha(0), spriteIndex(0), instanceID(0), truncatedImageIndex(0), hasVarChanged(0), frameCount(1)
	{
	}

	attackData(float xPos, float yPos, short xPosDiff, short yPosDiff, float imageXScale, float imageYScale, float imageAngle, float imageAlpha, short spriteIndex, short instanceID, char truncatedImageIndex, char hasVarChanged, int frameCount) :
		xPos(xPos), yPos(yPos), xPosDiff(xPosDiff), yPosDiff(yPosDiff), imageXScale(imageXScale), imageYScale(imageYScale), imageAngle(imageAngle),
		imageAlpha(imageAlpha), spriteIndex(spriteIndex), instanceID(instanceID), truncatedImageIndex(truncatedImageIndex), hasVarChanged(hasVarChanged), frameCount(frameCount)
	{
	}
};

const int attackCreateDataLen = 20;

struct messageAttackCreate
{
	attackData data[attackCreateDataLen]{};
	char numAttacks;
	messageAttackCreate() : numAttacks(0)
	{
	}

	messageAttackCreate(char* messageBuffer)
	{
		numAttacks = messageBuffer[0];
		for (int i = 0; i < numAttacks; i++)
		{
			int startBufferPos = i * sizeof(attackData) + 1;
			readByteBufferToFloat(&data[i].xPos, messageBuffer, startBufferPos);
			readByteBufferToFloat(&data[i].yPos, messageBuffer, startBufferPos);
			readByteBufferToFloat(&data[i].imageXScale, messageBuffer, startBufferPos);
			readByteBufferToFloat(&data[i].imageYScale, messageBuffer, startBufferPos);
			readByteBufferToFloat(&data[i].imageAngle, messageBuffer, startBufferPos);
			readByteBufferToFloat(&data[i].imageAlpha, messageBuffer, startBufferPos);
			readByteBufferToShort(&data[i].spriteIndex, messageBuffer, startBufferPos);
			readByteBufferToShort(&data[i].instanceID, messageBuffer, startBufferPos);
			readByteBufferToChar(&data[i].truncatedImageIndex, messageBuffer, startBufferPos);
		}
	}

	void addAttack(attackData addAttackData)
	{
		data[numAttacks] = std::move(addAttackData);
		numAttacks++;
	}

	void serialize(char* messageBuffer)
	{
		// TODO: Maybe combine this with the constructor to avoid needing a redundant copy?
		messageBuffer[0] = MESSAGE_ATTACK_CREATE;
		messageBuffer[1] = numAttacks;
		for (int i = 0; i < numAttacks; i++)
		{
			int startBufferPos = i * sizeof(attackData) + 2;
			writeFloatToByteBuffer(messageBuffer, data[i].xPos, startBufferPos);
			writeFloatToByteBuffer(messageBuffer, data[i].yPos, startBufferPos);
			writeFloatToByteBuffer(messageBuffer, data[i].imageXScale, startBufferPos);
			writeFloatToByteBuffer(messageBuffer, data[i].imageYScale, startBufferPos);
			writeFloatToByteBuffer(messageBuffer, data[i].imageAngle, startBufferPos);
			writeFloatToByteBuffer(messageBuffer, data[i].imageAlpha, startBufferPos);
			writeShortToByteBuffer(messageBuffer, data[i].spriteIndex, startBufferPos);
			writeShortToByteBuffer(messageBuffer, data[i].instanceID, startBufferPos);
			writeCharToByteBuffer(messageBuffer, data[i].truncatedImageIndex, startBufferPos);
		}
	}
};

const int attackUpdateDataLen = 20;

// hasVarChanged
// byte 0 - has position changed
// byte 1 - is position update a diff or exact (0 = diff) (1 = exact)
// byte 2 - is diff using byte or short (0 = byte) (1 = short)
// byte 3 - has image angle changed
// byte 4 - has image alpha changed
// byte 5 - has image scale changed
// byte 6 - has sprite index changed
struct messageAttackUpdate
{
	attackData data[attackUpdateDataLen]{};
	char numAttacks;
	messageAttackUpdate() : numAttacks(0)
	{
	}

	void addAttack(attackData addAttackData)
	{
		data[numAttacks] = std::move(addAttackData);
		numAttacks++;
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_ATTACK_UPDATE, startBufferPos);
		writeCharToByteBuffer(messageBuffer, numAttacks, startBufferPos);
		for (int i = 0; i < numAttacks; i++)
		{
			writeCharToByteBuffer(messageBuffer, data[i].hasVarChanged, startBufferPos);
			// Check if position has changed
			if (checkBitInByte(data[i].hasVarChanged, 0))
			{
				// Check if position is exact or diff
				if (checkBitInByte(data[i].hasVarChanged, 1))
				{
					writeFloatToByteBuffer(messageBuffer, data[i].xPos, startBufferPos);
					writeFloatToByteBuffer(messageBuffer, data[i].yPos, startBufferPos);
				}
				else
				{
					// Check if diff is short or byte
					if (checkBitInByte(data[i].hasVarChanged, 2))
					{
						writeShortToByteBuffer(messageBuffer, data[i].xPosDiff, startBufferPos);
						writeShortToByteBuffer(messageBuffer, data[i].yPosDiff, startBufferPos);
					}
					else
					{
						writeCharToByteBuffer(messageBuffer, static_cast<char>(data[i].xPosDiff), startBufferPos);
						writeCharToByteBuffer(messageBuffer, static_cast<char>(data[i].yPosDiff), startBufferPos);
					}
				}
			}

			if (checkBitInByte(data[i].hasVarChanged, 3))
			{
				short imageAngleApprox = (static_cast<int>(data[i].imageAngle * 10) % 3600 + 3600) % 3600;
				writeShortToByteBuffer(messageBuffer, imageAngleApprox, startBufferPos);
			}
			if (checkBitInByte(data[i].hasVarChanged, 4))
			{
				char imageAlphaApprox = static_cast<char>(data[i].imageAlpha * 255);
				writeCharToByteBuffer(messageBuffer, imageAlphaApprox, startBufferPos);
			}
			if (checkBitInByte(data[i].hasVarChanged, 5))
			{
				writeFloatToByteBuffer(messageBuffer, data[i].imageXScale, startBufferPos);
				writeFloatToByteBuffer(messageBuffer, data[i].imageYScale, startBufferPos);
			}
			if (checkBitInByte(data[i].hasVarChanged, 6))
			{
				writeShortToByteBuffer(messageBuffer, data[i].spriteIndex, startBufferPos);
			}
			writeShortToByteBuffer(messageBuffer, data[i].instanceID, startBufferPos);
			writeCharToByteBuffer(messageBuffer, data[i].truncatedImageIndex, startBufferPos);
		}
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize++;

		size_t instanceDataSize = 0;
		for (int i = 0; i < numAttacks; i++)
		{
			char hasVarChanged = data[i].hasVarChanged;
			instanceDataSize++;
			if (checkBitInByte(data[i].hasVarChanged, 0))
			{
				// Check if position is exact or diff
				if (checkBitInByte(data[i].hasVarChanged, 1))
				{
					instanceDataSize += 4;
					instanceDataSize += 4;
				}
				else
				{
					// Check if diff is short or byte
					if (checkBitInByte(data[i].hasVarChanged, 2))
					{
						instanceDataSize += 2;
						instanceDataSize += 2;
					}
					else
					{
						instanceDataSize++;
						instanceDataSize++;
					}
				}
			}
			if (checkBitInByte(data[i].hasVarChanged, 3))
			{
				instanceDataSize += 2;
			}
			if (checkBitInByte(data[i].hasVarChanged, 4))
			{
				instanceDataSize += 1;
			}
			if (checkBitInByte(data[i].hasVarChanged, 5))
			{
				instanceDataSize += 4;
				instanceDataSize += 4;
			}
			if (checkBitInByte(data[i].hasVarChanged, 6))
			{
				instanceDataSize += 2;
			}
			instanceDataSize += 2;
			instanceDataSize++;
		}
		curMessageSize += instanceDataSize;

		return curMessageSize;
	}
};

const int attackDeleteDataLen = 20;

struct messageAttackDelete
{
	short attackIDArr[attackDeleteDataLen]{};
	char numAttacks;
	messageAttackDelete() : numAttacks(0)
	{
	}

	messageAttackDelete(char* messageBuffer)
	{
		numAttacks = messageBuffer[0];
		for (int i = 0; i < numAttacks; i++)
		{
			int startBufferPos = i * 2 + 1;
			readByteBufferToShort(&attackIDArr[i], messageBuffer, startBufferPos);
		}
	}

	void addAttack(short instanceID)
	{
		attackIDArr[numAttacks] = instanceID;
		numAttacks++;
	}

	void serialize(char* messageBuffer)
	{
		messageBuffer[0] = MESSAGE_ATTACK_DELETE;
		messageBuffer[1] = numAttacks;
		for (int i = 0; i < numAttacks; i++)
		{
			int startBufferPos = i * 2 + 2;
			writeShortToByteBuffer(messageBuffer, attackIDArr[i], startBufferPos);
		}
	}
};

struct messageClientID
{
	uint32_t clientID;
	messageClientID(uint32_t clientID) : clientID(clientID)
	{
	}

	messageClientID(char* messageBuffer)
	{
		int startBufferPos = 0;
		readByteBufferToLong(&clientID, messageBuffer, startBufferPos);
	}

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_CLIENT_ID, startBufferPos);
		writeLongToByteBuffer(messageBuffer, clientID, startBufferPos);
	}
};

// TODO: Maybe consider sending over what frame the game is currently on and then calculating what image index it should have?
// Not sure how to handle it if the imageSpeed changes though
// Can probably just send it separately since the only reason I need it right now is for the enemy death animation anyways
// TODO: Send over player stats as well. Not sure how I should handle sending over the character selected
struct playerData
{
	uint32_t m_playerID;
	float xPos;
	float yPos;
	float imageXScale;
	float imageYScale;
	float direction;
	float curAttack;
	float curSpeed;
	short spriteIndex;
	short curHP;
	short maxHP;
	short curCrit;
	short curHaste;
	short curPickupRange;
	short specialMeter;
	char truncatedImageIndex;

	playerData() : m_playerID(0), xPos(0), yPos(0), imageXScale(0), imageYScale(0), direction(0), spriteIndex(0), curHP(0), maxHP(0),
		curAttack(0), curSpeed(0), curCrit(0), curHaste(0), curPickupRange(0), specialMeter(0), truncatedImageIndex(0)
	{
	}

	playerData(float xPos, float yPos, float imageXScale, float imageYScale, float direction, short spriteIndex, short curHP, short maxHP,
		float curAttack, float curSpeed, short curCrit, short curHaste, short curPickupRange, short specialMeter, char truncatedImageIndex, uint32_t m_playerID) :
		xPos(xPos), yPos(yPos), imageXScale(imageXScale), imageYScale(imageYScale), direction(direction), spriteIndex(spriteIndex), curHP(curHP), maxHP(maxHP),
		curAttack(curAttack), curSpeed(curSpeed), curCrit(curCrit), curHaste(curHaste), curPickupRange(curPickupRange), specialMeter(specialMeter), truncatedImageIndex(truncatedImageIndex), m_playerID(m_playerID)
	{
	}
};

struct pickupableData
{
	float xPos;
	float yPos;
	short spriteIndex;
	short pickupableID;
	char truncatedImageIndex;

	pickupableData() : xPos(0), yPos(0), spriteIndex(0), pickupableID(0), truncatedImageIndex(0)
	{
	}

	pickupableData(float xPos, float yPos, short spriteIndex, short pickupableID, char truncatedImageIndex) : xPos(xPos), yPos(yPos), spriteIndex(spriteIndex), pickupableID(pickupableID), truncatedImageIndex(truncatedImageIndex)
	{
	}
};

struct messagePickupableCreate
{
	pickupableData data{};

	messagePickupableCreate(pickupableData data) : data(data)
	{
	}

	messagePickupableCreate(char* messageBuffer)
	{
		int startBufferPos = 0;
		readByteBufferToFloat(&data.xPos, messageBuffer, startBufferPos);
		readByteBufferToFloat(&data.yPos, messageBuffer, startBufferPos);
		readByteBufferToShort(&data.spriteIndex, messageBuffer, startBufferPos);
		readByteBufferToShort(&data.pickupableID, messageBuffer, startBufferPos);
		readByteBufferToChar(&data.truncatedImageIndex, messageBuffer, startBufferPos);
	}

	void serialize(char* messageBuffer)
	{
		// TODO: Maybe combine this with the constructor to avoid needing a redundant copy?
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_PICKUPABLE_CREATE, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.xPos, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.yPos, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.spriteIndex, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.pickupableID, startBufferPos);
		writeCharToByteBuffer(messageBuffer, data.truncatedImageIndex, startBufferPos);
	}
};

struct messagePickupableUpdate
{
	pickupableData data{};

	messagePickupableUpdate(pickupableData data) : data(data)
	{
	}

	messagePickupableUpdate(char* messageBuffer)
	{
		int startBufferPos = 0;
		readByteBufferToFloat(&data.xPos, messageBuffer, startBufferPos);
		readByteBufferToFloat(&data.yPos, messageBuffer, startBufferPos);
		readByteBufferToShort(&data.spriteIndex, messageBuffer, startBufferPos);
		readByteBufferToShort(&data.pickupableID, messageBuffer, startBufferPos);
		readByteBufferToChar(&data.truncatedImageIndex, messageBuffer, startBufferPos);
	}

	void serialize(char* messageBuffer)
	{
		// TODO: Maybe combine this with the constructor to avoid needing a redundant copy?
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_PICKUPABLE_UPDATE, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.xPos, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.yPos, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.spriteIndex, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.pickupableID, startBufferPos);
		writeCharToByteBuffer(messageBuffer, data.truncatedImageIndex, startBufferPos);
	}
};

struct messagePickupableDelete
{
	short pickupableID;

	messagePickupableDelete() : pickupableID(0)
	{
	}

	messagePickupableDelete(short pickupableID) : pickupableID(pickupableID)
	{
	}

	messagePickupableDelete(char* messageBuffer)
	{
		int startBufferPos = 0;
		readByteBufferToShort(&pickupableID, messageBuffer, startBufferPos);
	}

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_PICKUPABLE_DELETE, startBufferPos);
		writeShortToByteBuffer(messageBuffer, pickupableID, startBufferPos);
	}
};

struct messageGameData
{
	uint32_t frameNum;
	uint32_t coinCount;
	uint32_t enemyDefeated;
	float experience;
	float toNextLevel;
	float moneyGain;
	float food;
	short playerLevel;
	short goldenHammer;
	short goldenHammerPieces;

	messageGameData() : frameNum(0), coinCount(0), enemyDefeated(0), experience(0), toNextLevel(0), moneyGain(0), food(0), playerLevel(0), goldenHammer(0), goldenHammerPieces(0)
	{
	}

	messageGameData(char* messageBuffer)
	{
		int startBufferPos = 0;
		readByteBufferToLong(&frameNum, messageBuffer, startBufferPos);
		readByteBufferToLong(&coinCount, messageBuffer, startBufferPos);
		readByteBufferToLong(&enemyDefeated, messageBuffer, startBufferPos);
		readByteBufferToFloat(&experience, messageBuffer, startBufferPos);
		readByteBufferToFloat(&toNextLevel, messageBuffer, startBufferPos);
		readByteBufferToFloat(&moneyGain, messageBuffer, startBufferPos);
		readByteBufferToFloat(&food, messageBuffer, startBufferPos);
		readByteBufferToShort(&playerLevel, messageBuffer, startBufferPos);
		readByteBufferToShort(&goldenHammer, messageBuffer, startBufferPos);
		readByteBufferToShort(&goldenHammerPieces, messageBuffer, startBufferPos);
	}

	messageGameData(uint32_t frameNum, uint32_t coinCount, uint32_t enemyDefeated, float experience, float toNextLevel, float moneyGain, float food, short playerLevel, short goldenHammer, short goldenHammerPieces) :
		frameNum(frameNum), coinCount(coinCount), enemyDefeated(enemyDefeated), experience(experience), toNextLevel(toNextLevel), moneyGain(moneyGain), food(food), playerLevel(playerLevel), goldenHammer(goldenHammer), goldenHammerPieces(goldenHammerPieces)
	{
	}

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_GAME_DATA, startBufferPos);
		writeLongToByteBuffer(messageBuffer, frameNum, startBufferPos);
		writeLongToByteBuffer(messageBuffer, coinCount, startBufferPos);
		writeLongToByteBuffer(messageBuffer, enemyDefeated, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, experience, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, toNextLevel, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, moneyGain, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, food, startBufferPos);
		writeShortToByteBuffer(messageBuffer, playerLevel, startBufferPos);
		writeShortToByteBuffer(messageBuffer, goldenHammer, startBufferPos);
		writeShortToByteBuffer(messageBuffer, goldenHammerPieces, startBufferPos);
	}
};

struct levelUpOption
{
	// TODO: can probably change optionType to a number
	std::string optionType;
	std::string optionName;
	std::string optionID;
	std::vector<std::string> optionDescription;
	std::vector<std::string> modsList;
	uint16_t optionIcon;
	uint16_t optionIcon_Super;
	char weaponAndItemType;
	char isModsListNew; // bool value to determine if the modsList is gainedMods or offeredMod

	levelUpOption() : optionIcon(0), weaponAndItemType(0), isModsListNew(0), optionIcon_Super(0)
	{
	}

	levelUpOption(std::string optionType, std::string optionName, std::string optionID, std::vector<std::string> optionDescription, uint16_t optionIcon, uint16_t optionIcon_Super, char weaponAndItemType)
		: optionType(optionType), optionName(optionName), optionID(optionID), optionDescription(optionDescription), optionIcon(optionIcon), optionIcon_Super(optionIcon_Super), weaponAndItemType(weaponAndItemType), isModsListNew(0)
	{
	}

	levelUpOption(std::string optionType, std::string optionName, std::string optionID, std::vector<std::string> optionDescription, uint16_t optionIcon, uint16_t optionIcon_Super, char weaponAndItemType, std::vector<std::string> modsList, char isModsListNew)
		: optionType(optionType), optionName(optionName), optionID(optionID), optionDescription(optionDescription), optionIcon(optionIcon), optionIcon_Super(optionIcon_Super), weaponAndItemType(weaponAndItemType), modsList(modsList), isModsListNew(isModsListNew)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, static_cast<char>(modsList.size()), startBufferPos);
		for (int i = 0; i < modsList.size(); i++)
		{
			writeStringToByteBuffer(messageBuffer, modsList[i], startBufferPos);
		}
		writeCharToByteBuffer(messageBuffer, static_cast<char>(optionDescription.size()), startBufferPos);
		for (int i = 0; i < optionDescription.size(); i++)
		{
			writeStringToByteBuffer(messageBuffer, optionDescription[i], startBufferPos);
		}
		writeStringToByteBuffer(messageBuffer, optionType, startBufferPos);
		writeStringToByteBuffer(messageBuffer, optionName, startBufferPos);
		writeStringToByteBuffer(messageBuffer, optionID, startBufferPos);
		writeShortToByteBuffer(messageBuffer, static_cast<short>(optionIcon), startBufferPos);
		writeShortToByteBuffer(messageBuffer, static_cast<short>(optionIcon_Super), startBufferPos);
		writeCharToByteBuffer(messageBuffer, weaponAndItemType, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		for (int i = 0; i < modsList.size(); i++)
		{
			curMessageSize += modsList[i].size() + 2;
		}
		curMessageSize++;
		for (int i = 0; i < optionDescription.size(); i++)
		{
			curMessageSize += optionDescription[i].size() + 2;
		}
		curMessageSize += optionType.size() + 2;
		curMessageSize += optionName.size() + 2;
		curMessageSize += optionID.size() + 2;
		curMessageSize += 2;
		curMessageSize += 2;
		curMessageSize++;
		return curMessageSize;
	}
};

struct messageLevelUpOptions
{
	levelUpOption optionArr[4]{};

	messageLevelUpOptions()
	{
	}

	messageLevelUpOptions(levelUpOption* levelUpOptionArr)
	{
		for (int i = 0; i < 4; i++)
		{
			optionArr[i] = levelUpOptionArr[i];
		}
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		size_t curBufferPos = 0;
		messageBuffer[curBufferPos] = MESSAGE_LEVEL_UP_OPTIONS;
		curBufferPos++;
		for (int i = 0; i < 4; i++)
		{
			optionArr[i].serialize(&messageBuffer[curBufferPos]);
			curBufferPos += optionArr[i].getMessageSize();
		}
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		for (int i = 0; i < 4; i++)
		{
			curMessageSize += optionArr[i].getMessageSize();
		}
		return curMessageSize;
	}
};

struct messageLevelUpClientChoice
{
	uint32_t m_playerID;
	char levelUpOption;

	messageLevelUpClientChoice() : m_playerID(0), levelUpOption(0)
	{
	}

	messageLevelUpClientChoice(char levelUpOption) : m_playerID(0), levelUpOption(levelUpOption)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		size_t curBufferPos = 0;
		messageBuffer[curBufferPos] = MESSAGE_LEVEL_UP_CLIENT_CHOICE;
		curBufferPos++;
		messageBuffer[curBufferPos] = levelUpOption;
		curBufferPos++;
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize++;
		return curMessageSize;
	}
};

struct messagePing
{
	void serialize(char* messageBuffer)
	{
		size_t curBufferPos = 0;
		messageBuffer[curBufferPos] = MESSAGE_PING;
		curBufferPos++;
	}
};

struct messagePong
{
	void serialize(char* messageBuffer)
	{
		size_t curBufferPos = 0;
		messageBuffer[curBufferPos] = MESSAGE_PONG;
		curBufferPos++;
	}
};

struct destructableData
{
	RValue destructableInstance;
	float xPos;
	float yPos;
	short id;
	char pillarType;

	destructableData() : xPos(0), yPos(0), id(0), pillarType(0)
	{
	}

	destructableData(float xPos, float yPos, short id, char pillarType) : xPos(xPos), yPos(yPos), id(id), pillarType(pillarType)
	{
	}

	destructableData(float xPos, float yPos, short id, char pillarType, RValue destructableInstance) : xPos(xPos), yPos(yPos), id(id), destructableInstance(destructableInstance), pillarType(pillarType)
	{
	}
};

struct messageDestructableCreate
{
	destructableData data;

	messageDestructableCreate() : data()
	{
	}

	messageDestructableCreate(destructableData data) : data(data)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_DESTRUCTABLE_CREATE, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.xPos, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.yPos, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.id, startBufferPos);
		writeCharToByteBuffer(messageBuffer, data.pillarType, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += 4;
		curMessageSize += 4;
		curMessageSize += 2;
		curMessageSize++;
		return curMessageSize;
	}
};

struct messageDestructableBreak
{
	destructableData data;

	messageDestructableBreak() : data()
	{
	}

	messageDestructableBreak(destructableData data) : data(data)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_DESTRUCTABLE_BREAK, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.xPos, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.yPos, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.id, startBufferPos);
		writeCharToByteBuffer(messageBuffer, data.pillarType, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += 4;
		curMessageSize += 4;
		curMessageSize += 2;
		curMessageSize++;
		return curMessageSize;
	}
};

struct messageEliminateLevelUpClientChoice
{
	uint32_t m_playerID;
	char levelUpOption;

	messageEliminateLevelUpClientChoice() : m_playerID(0), levelUpOption(0)
	{
	}

	messageEliminateLevelUpClientChoice(char levelUpOption) : m_playerID(0), levelUpOption(levelUpOption)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		size_t curBufferPos = 0;
		messageBuffer[curBufferPos] = MESSAGE_ELIMINATE_LEVEL_UP_CLIENT_CHOICE;
		curBufferPos++;
		messageBuffer[curBufferPos] = levelUpOption;
		curBufferPos++;
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize++;
		return curMessageSize;
	}
};

struct messageClientSpecialAttack
{
	void serialize(char* messageBuffer)
	{
		size_t curBufferPos = 0;
		messageBuffer[curBufferPos] = MESSAGE_CLIENT_SPECIAL_ATTACK;
		curBufferPos++;
	}
};

struct cautionData
{
	float xPos;
	float yPos;
	short dir;
	char cautionType;

	cautionData() : xPos(0), yPos(0), dir(0), cautionType(0)
	{
	}

	cautionData(float xPos, float yPos, short dir, char cautionType) : xPos(xPos), yPos(yPos), dir(dir), cautionType(cautionType)
	{
	}
};

struct messageCautionCreate
{
	cautionData data;

	messageCautionCreate() : data()
	{
	}

	messageCautionCreate(cautionData data) : data(data)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_CAUTION_CREATE, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.xPos, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.yPos, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.dir, startBufferPos);
		writeCharToByteBuffer(messageBuffer, data.cautionType, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += 4;
		curMessageSize += 4;
		curMessageSize += 2;
		curMessageSize++;
		return curMessageSize;
	}
};

struct preCreateData
{
	float xPos;
	float yPos;
	short waitSpawn;
	short id;

	preCreateData() : xPos(0), yPos(0), waitSpawn(0), id(0)
	{
	}

	preCreateData(float xPos, float yPos, short waitSpawn, short id) : xPos(xPos), yPos(yPos), waitSpawn(waitSpawn), id(id)
	{
	}
};

struct messagePreCreateUpdate
{
	preCreateData data;

	messagePreCreateUpdate() : data()
	{
	}

	messagePreCreateUpdate(preCreateData data) : data(data)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_PRECREATE_UPDATE, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.xPos, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.yPos, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.waitSpawn, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.id, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += 4;
		curMessageSize += 4;
		curMessageSize += 2;
		curMessageSize += 2;
		return curMessageSize;
	}
};

struct vfxData
{
	float xPos;
	float yPos;
	float imageXScale;
	float imageYScale;
	float imageAngle;
	float imageAlpha;
	uint32_t color;
	short spriteIndex;
	short imageIndex;
	short id;
	char type;

	vfxData() : xPos(0), yPos(0), imageXScale(0), imageYScale(0), imageAngle(0), imageAlpha(0), color(0), spriteIndex(0), imageIndex(0), id(0), type(0)
	{
	}

	vfxData(float xPos, float yPos, float imageXScale, float imageYScale, float imageAngle, float imageAlpha, uint32_t color, short spriteIndex, short imageIndex, short id, char type) :
		xPos(xPos), yPos(yPos), imageXScale(imageXScale), imageYScale(imageYScale), imageAngle(imageAngle), imageAlpha(imageAlpha), color(color), spriteIndex(spriteIndex), imageIndex(imageIndex), id(id), type(type)
	{
	}
};

struct messageVfxUpdate
{
	vfxData data;

	messageVfxUpdate() : data()
	{
	}

	messageVfxUpdate(vfxData data) : data(data)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_VFX_UPDATE, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.xPos, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.yPos, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.imageXScale, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.imageYScale, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.imageAngle, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.imageAlpha, startBufferPos);
		writeLongToByteBuffer(messageBuffer, data.color, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.spriteIndex, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.imageIndex, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.id, startBufferPos);
		writeCharToByteBuffer(messageBuffer, data.type, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += 4;
		curMessageSize += 4;
		curMessageSize += 4;
		curMessageSize += 4;
		curMessageSize += 4;
		curMessageSize += 4;
		curMessageSize += 4;
		curMessageSize += 2;
		curMessageSize += 2;
		curMessageSize += 2;
		curMessageSize++;
		return curMessageSize;
	}
};

// Combined anvil, box, stamp, and hammer into one struct since they don't need to send any special data
struct interactableData
{
	float xPos;
	float yPos;
	short id;
	short spriteIndex;
	char type;

	interactableData() : xPos(0), yPos(0), id(0), spriteIndex(0), type(0)
	{
	}

	interactableData(float xPos, float yPos, short id, short spriteIndex, char type) :
		xPos(xPos), yPos(yPos), id(id), spriteIndex(spriteIndex), type(type)
	{
	}
};

struct messageInteractableCreate
{
	interactableData data;

	messageInteractableCreate() : data()
	{
	}

	messageInteractableCreate(interactableData data) : data(data)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_INTERACTABLE_CREATE, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.xPos, startBufferPos);
		writeFloatToByteBuffer(messageBuffer, data.yPos, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.id, startBufferPos);
		writeShortToByteBuffer(messageBuffer, data.spriteIndex, startBufferPos);
		writeCharToByteBuffer(messageBuffer, data.type, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += 4;
		curMessageSize += 4;
		curMessageSize += 2;
		curMessageSize += 2;
		curMessageSize++;
		return curMessageSize;
	}
};

struct messageInteractableDelete
{
	short id;
	char type;

	messageInteractableDelete() : id(0), type(0)
	{
	}

	messageInteractableDelete(short id, char type) : id(id), type(type)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_INTERACTABLE_DELETE, startBufferPos);
		writeShortToByteBuffer(messageBuffer, id, startBufferPos);
		writeCharToByteBuffer(messageBuffer, type, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += 2;
		curMessageSize++;
		return curMessageSize;
	}
};

struct messageInteractablePlayerInteracted
{
	uint32_t m_playerID;
	short id;
	char type;

	messageInteractablePlayerInteracted() : m_playerID(0), id(0), type(0)
	{
	}

	messageInteractablePlayerInteracted(uint32_t m_playerID, short id, char type) : m_playerID(m_playerID), id(id), type(type)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_INTERACTABLE_PLAYER_INTERACTED, startBufferPos);
		writeLongToByteBuffer(messageBuffer, m_playerID, startBufferPos);
		writeShortToByteBuffer(messageBuffer, id, startBufferPos);
		writeCharToByteBuffer(messageBuffer, type, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += 4;
		curMessageSize += 2;
		curMessageSize++;
		return curMessageSize;
	}
};

struct messageStickerPlayerInteracted
{
	std::string stickerID;
	uint32_t m_playerID;
	short id;

	messageStickerPlayerInteracted() : id(0), m_playerID(0)
	{
	}

	messageStickerPlayerInteracted(std::string stickerID, uint32_t m_playerID, short id) : stickerID(stickerID), m_playerID(m_playerID), id(id)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_STICKER_PLAYER_INTERACTED, startBufferPos);
		writeStringToByteBuffer(messageBuffer, stickerID, startBufferPos);
		writeLongToByteBuffer(messageBuffer, m_playerID, startBufferPos);
		writeShortToByteBuffer(messageBuffer, id, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += stickerID.size() + 2;
		curMessageSize += 4;
		curMessageSize += 2;
		return curMessageSize;
	}
};

struct messageBoxPlayerInteracted
{
	levelUpOption randomWeapons[3]{};
	uint32_t m_playerID;
	short id;
	char boxItemAmount;
	char isSuperBox;

	messageBoxPlayerInteracted() : id(0), m_playerID(0), boxItemAmount(0), isSuperBox(0)
	{
	}

	messageBoxPlayerInteracted(levelUpOption* randomWeaponArr, uint32_t m_playerID, short id, char boxItemAmount, char isSuperBox) :
		m_playerID(m_playerID), id(id), boxItemAmount(boxItemAmount), isSuperBox(isSuperBox)
	{
		for (int i = 0; i < 3; i++)
		{
			randomWeapons[i] = randomWeaponArr[i];
		}
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_BOX_PLAYER_INTERACTED, startBufferPos);
		for (int i = 0; i < 3; i++)
		{
			randomWeapons[i].serialize(&messageBuffer[startBufferPos]);
			startBufferPos += static_cast<int>(randomWeapons[i].getMessageSize());
		}
		writeLongToByteBuffer(messageBuffer, m_playerID, startBufferPos);
		writeShortToByteBuffer(messageBuffer, id, startBufferPos);
		writeCharToByteBuffer(messageBuffer, boxItemAmount, startBufferPos);
		writeCharToByteBuffer(messageBuffer, isSuperBox, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		for (int i = 0; i < 3; i++)
		{
			curMessageSize += randomWeapons[i].getMessageSize();
		}
		curMessageSize += 4;
		curMessageSize += 2;
		curMessageSize++;
		curMessageSize++;
		return curMessageSize;
	}
};

struct messageInteractFinished
{
	void serialize(char* messageBuffer)
	{
		size_t curBufferPos = 0;
		messageBuffer[curBufferPos] = MESSAGE_INTERACT_FINISHED;
		curBufferPos++;
	}
};

struct messageBoxTakeOption
{
	uint32_t m_playerID;
	char boxItemNum;

	messageBoxTakeOption() : m_playerID(0), boxItemNum(0)
	{
	}

	messageBoxTakeOption(char boxItemNum) : m_playerID(0), boxItemNum(boxItemNum)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_BOX_TAKE_OPTION, startBufferPos);
		writeCharToByteBuffer(messageBuffer, boxItemNum, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize++;
		return curMessageSize;
	}
};

struct messageAnvilChooseOption
{
	uint32_t m_playerID;
	std::string optionID;
	std::string optionType;
	uint32_t coinCost;
	char anvilOptionType; // 0 is level up, 1 is enhance

	messageAnvilChooseOption() : m_playerID(0), coinCost(0), anvilOptionType(0)
	{
	}

	messageAnvilChooseOption(std::string optionID, std::string optionType, uint32_t coinCost, char anvilOptionType) : m_playerID(0), optionID(optionID), optionType(optionType), coinCost(coinCost), anvilOptionType(anvilOptionType)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_ANVIL_CHOOSE_OPTION, startBufferPos);
		writeStringToByteBuffer(messageBuffer, optionID, startBufferPos);
		writeStringToByteBuffer(messageBuffer, optionType, startBufferPos);
		writeLongToByteBuffer(messageBuffer, coinCost, startBufferPos);
		writeCharToByteBuffer(messageBuffer, anvilOptionType, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += optionID.size() + 2;
		curMessageSize += optionType.size() + 2;
		curMessageSize += 4;
		curMessageSize++;
		return curMessageSize;
	}
};

struct messageClientGainMoney
{
	uint32_t money;

	messageClientGainMoney() : money(0)
	{
	}

	messageClientGainMoney(uint32_t money) : money(money)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_CLIENT_GAIN_MONEY, startBufferPos);
		writeLongToByteBuffer(messageBuffer, money, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += 4;
		return curMessageSize;
	}
};

struct messageClientAnvilEnchant
{
	uint32_t m_playerID;
	std::vector<std::string> gainedMods;
	std::string optionID;
	uint32_t coinCost;

	messageClientAnvilEnchant() : m_playerID(0), coinCost(0)
	{
	}

	messageClientAnvilEnchant(std::string optionID, std::vector<std::string> gainedMods, uint32_t coinCost) : m_playerID(0), optionID(optionID), gainedMods(gainedMods), coinCost(coinCost)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_CLIENT_ANVIL_ENCHANT, startBufferPos);
		if (gainedMods.size() >= 256)
		{
			g_ModuleInterface->Print(CM_RED, "TOO MANY ENCHANTS. MESSAGE WILL BE SENT INCORRECTLY");
		}
		writeCharToByteBuffer(messageBuffer, static_cast<char>(gainedMods.size()), startBufferPos);
		for (int i = 0; i < gainedMods.size(); i++)
		{
			writeStringToByteBuffer(messageBuffer, gainedMods[i], startBufferPos);
		}
		writeStringToByteBuffer(messageBuffer, optionID, startBufferPos);
		writeLongToByteBuffer(messageBuffer, coinCost, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize++;
		for (int i = 0; i < gainedMods.size(); i++)
		{
			curMessageSize += gainedMods[i].size() + 2;
		}
		curMessageSize += optionID.size() + 2;
		curMessageSize += 4;
		return curMessageSize;
	}
};

struct messageStickerChooseOption
{
	uint32_t m_playerID;
	char stickerOption;
	char stickerOptionType; // 0 is take stamp, 1 is remove stamp, 2 is sell stamp

	messageStickerChooseOption() : m_playerID(0), stickerOption(0), stickerOptionType(0)
	{
	}

	messageStickerChooseOption(char stickerOption, char stickerOptionType) : m_playerID(0), stickerOption(stickerOption), stickerOptionType(stickerOptionType)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_STICKER_CHOOSE_OPTION, startBufferPos);
		writeCharToByteBuffer(messageBuffer, stickerOption, startBufferPos);
		writeCharToByteBuffer(messageBuffer, stickerOptionType, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize++;
		curMessageSize++;
		return curMessageSize;
	}
};

struct messageChooseCollab
{
	levelUpOption collab;
	uint32_t m_playerID;

	messageChooseCollab() : m_playerID(0)
	{
	}

	messageChooseCollab(levelUpOption collab) : m_playerID(0), collab(collab)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_CHOOSE_COLLAB, startBufferPos);
		collab.serialize(&messageBuffer[startBufferPos]);
		startBufferPos += static_cast<int>(collab.getMessageSize());
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += collab.getMessageSize();
		return curMessageSize;
	}
};

struct buffData
{
	std::string buffName;
	short timer;
	short stacks;

	buffData() : timer(0), stacks(0)
	{
	}

	buffData(std::string buffName, short timer, short stacks) : buffName(buffName), timer(timer), stacks(stacks)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeStringToByteBuffer(messageBuffer, buffName, startBufferPos);
		writeShortToByteBuffer(messageBuffer, timer, startBufferPos);
		writeShortToByteBuffer(messageBuffer, stacks, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize += buffName.size() + 2;
		curMessageSize += 2;
		curMessageSize += 2;
		return curMessageSize;
	}
};

struct messageBuffData
{
	std::vector<buffData> buffDataList;

	messageBuffData()
	{
	}

	messageBuffData(std::vector<buffData> buffDataList) : buffDataList(buffDataList)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_BUFF_DATA, startBufferPos);
		if (buffDataList.size() >= 256)
		{
			g_ModuleInterface->Print(CM_RED, "TOO MANY BUFFS. MESSAGE WILL BE SENT INCORRECTLY");
		}
		writeCharToByteBuffer(messageBuffer, static_cast<char>(buffDataList.size()), startBufferPos);
		for (int i = 0; i < buffDataList.size(); i++)
		{
			buffDataList[i].serialize(&messageBuffer[startBufferPos]);
			startBufferPos += static_cast<int>(buffDataList[i].getMessageSize());
		}
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize++;
		for (int i = 0; i < buffDataList.size(); i++)
		{
			curMessageSize += buffDataList[i].getMessageSize();
		}
		return curMessageSize;
	}
};

struct lobbyPlayerData
{
	std::string charName;
	std::string playerName;
	short stageSprite;
	char isReady;

	lobbyPlayerData() : isReady(0), stageSprite(-1)
	{
	}

	lobbyPlayerData(std::string charName, std::string playerName, short stageSprite, char isReady) : charName(charName), playerName(playerName), stageSprite(stageSprite), isReady(isReady)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeStringToByteBuffer(messageBuffer, charName, startBufferPos);
		writeStringToByteBuffer(messageBuffer, playerName, startBufferPos);
		writeShortToByteBuffer(messageBuffer, stageSprite, startBufferPos);
		writeCharToByteBuffer(messageBuffer, isReady, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize += charName.size() + 2;
		curMessageSize += playerName.size() + 2;
		curMessageSize += 2;
		curMessageSize++;
		return curMessageSize;
	}
};

struct messageCharData
{
	lobbyPlayerData playerData;
	uint32_t m_playerID;

	messageCharData() : m_playerID(0)
	{
	}

	messageCharData(lobbyPlayerData playerData, uint32_t m_playerID) : playerData(playerData), m_playerID(m_playerID)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_CHAR_DATA, startBufferPos);
		playerData.serialize(&messageBuffer[startBufferPos]);
		startBufferPos += static_cast<int>(playerData.getMessageSize());
		writeLongToByteBuffer(messageBuffer, m_playerID, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += playerData.getMessageSize();
		curMessageSize += 4;
		return curMessageSize;
	}
};

struct messageReturnToLobby
{
	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_RETURN_TO_LOBBY, startBufferPos);
	}
};

struct messageLobbyPlayerDisconnected
{
	uint32_t m_playerID;

	messageLobbyPlayerDisconnected() : m_playerID(0)
	{
	}

	messageLobbyPlayerDisconnected(uint32_t m_playerID) : m_playerID(m_playerID)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_LOBBY_PLAYER_DISCONNECTED, startBufferPos);
		writeLongToByteBuffer(messageBuffer, m_playerID, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += 4;
		return curMessageSize;
	}
};

struct messageHostHasPaused
{
	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_HOST_HAS_PAUSED, startBufferPos);
	}
};

struct messageHostHasUnpaused
{
	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_HOST_HAS_UNPAUSED, startBufferPos);
	}
};

struct messageKaelaOreAmount
{
	short oreA;
	short oreB;
	short oreC;

	messageKaelaOreAmount() : oreA(0), oreB(0), oreC(0)
	{
	}

	messageKaelaOreAmount(short oreA, short oreB, short oreC) : oreA(oreA), oreB(oreB), oreC(oreC)
	{
	}

	int receiveMessage(uint32_t playerID);

	void serialize(char* messageBuffer)
	{
		int startBufferPos = 0;
		writeCharToByteBuffer(messageBuffer, MESSAGE_KAELA_ORE_AMOUNT, startBufferPos);
		writeShortToByteBuffer(messageBuffer, oreA, startBufferPos);
		writeShortToByteBuffer(messageBuffer, oreB, startBufferPos);
		writeShortToByteBuffer(messageBuffer, oreC, startBufferPos);
	}

	size_t getMessageSize()
	{
		size_t curMessageSize = 0;
		curMessageSize++;
		curMessageSize += 2;
		curMessageSize += 2;
		curMessageSize += 2;
		return curMessageSize;
	}
};