#include "SteamClient.h"
#include <iostream>

CSteamClient::CSteamClient()
{
	
}

CSteamClient::~CSteamClient()
{

}

/*
// TODO: Implement this later
//-----------------------------------------------------------------------------
// Purpose: Steam is asking us to join a game, based on the user selecting
//			'join game' on a friend in their friends list 
//			the string comes from the "connect" field set in the friends' rich presence
//-----------------------------------------------------------------------------
void CSteamClient::OnGameJoinRequested(GameRichPresenceJoinRequested_t* pCallback)
{
	// parse out the connect 
	const char* pchServerAddress, * pchLobbyID;

	printf("On game join requested %s\n", pCallback->m_rgchConnect);
	if (ParseCommandLine(pCallback->m_rgchConnect, &pchServerAddress, &pchLobbyID))
	{
		// exec
		ExecCommandLineConnect(pchServerAddress, pchLobbyID);
	}
}
*/