#include "SteamLobbyBrowser.h"
#include "steam/steam_api.h"
#include <windows.h>
#include "ModuleMain.h"
#include "ScriptFunctions.h"
#include "CodeEvents.h"
#include "NetworkFunctions.h"

extern bool hasJoinedSteamLobby;
extern std::thread messageHandlerThread;
extern std::unordered_map<uint32_t, uint64> clientIDToSteamIDMap;

inline void strncpy_safe(char* pDest, char const* pSrc, size_t maxLen)
{
	size_t nCount = maxLen;
	char* pstrDest = pDest;
	const char* pstrSource = pSrc;

	while (0 < nCount && 0 != (*pstrDest++ = *pstrSource++))
		nCount--;

	if (maxLen > 0)
		pstrDest[-1] = 0;
}

CSteamLobbyBrowser::CSteamLobbyBrowser() :
	m_CallbackLobbyDataUpdated(this, &CSteamLobbyBrowser::OnLobbyDataUpdatedCallback),
	m_CallbackChatDataUpdate(this, &CSteamLobbyBrowser::OnLobbyChatUpdate),
	m_CallbackGameJoinRequested(this, &CSteamLobbyBrowser::OnGameJoinRequested),
	m_CallbackPersonaStateChange(this, &CSteamLobbyBrowser::OnPersonaStateChange)
{
	/*
	* // TODO: Use this for getting public lobbies
	SteamAPICall_t hSteamAPICall = SteamMatchmaking()->RequestLobbyList();
	m_SteamCallResultLobbyMatchList.Set(hSteamAPICall, this, &CSteamLobbyBrowser::OnLobbyMatchListCallback);
	printf("Requesting lobby list\n");
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Callback, on a list of lobbies being received from the Steam back-end
//-----------------------------------------------------------------------------
// Seems like this callback occurs when RequestLobbyList is called
void CSteamLobbyBrowser::OnLobbyMatchListCallback(LobbyMatchList_t* pCallback, bool bIOFailure)
{
	if (bIOFailure)
	{
		// we had a Steam I/O failure - we probably timed out talking to the Steam back-end servers
		// doesn't matter in this case, we can just act if no lobbies were received
	}

	printf("Received lobbies %d\n", pCallback->m_nLobbiesMatching);
	// lobbies are returned in order of closeness to the user, so add them to the list in that order
	for (uint32 iLobby = 0; iLobby < pCallback->m_nLobbiesMatching; iLobby++)
	{
		CSteamID steamIDLobby = SteamMatchmaking()->GetLobbyByIndex(iLobby);

		// add the lobby to the list
		Lobby_t lobby;
		lobby.m_steamIDLobby = steamIDLobby;
		// pull the name from the lobby metadata
		const char* pchLobbyName = SteamMatchmaking()->GetLobbyData(steamIDLobby, "name");
		if (pchLobbyName && pchLobbyName[0])
		{
			// retrieved the lobby name
			printf("%s\n", pchLobbyName);
		}
		else
		{
			// we don't have info about the lobby yet, request it
			SteamMatchmaking()->RequestLobbyData(steamIDLobby);
			// results will be returned via LobbyDataUpdate_t callback
			printf("lobby %d\n", steamIDLobby.GetAccountID());
		}
		
	}

}

//-----------------------------------------------------------------------------
// Purpose: Callback, on a list of lobbies being received from the Steam back-end
//-----------------------------------------------------------------------------
// Occurs whenever there's a change in the lobbies
void CSteamLobbyBrowser::OnLobbyDataUpdatedCallback(LobbyDataUpdate_t* pCallback)
{
	CSteamID steamLobbyID = getSteamLobbyID();
	if (steamLobbyID != pCallback->m_ulSteamIDLobby)
		return;

	int cLobbyMembers = SteamMatchmaking()->GetNumLobbyMembers(steamLobbyID);
	m_lobbyMemberList.clear();
	CSteamID curUserID = SteamUser()->GetSteamID();
	for (int i = 0; i < cLobbyMembers; i++)
	{
		CSteamID steamIDLobbyMember = SteamMatchmaking()->GetLobbyMemberByIndex(steamLobbyID, i);

		// ignore yourself.
		if (curUserID == steamIDLobbyMember)
			continue;
		// we get the details of a user from the ISteamFriends interface
		const char* pchName = SteamFriends()->GetFriendPersonaName(steamIDLobbyMember);
		// we may not know the name of the other users in the lobby immediately; but we'll receive
		// a PersonaStateUpdate_t callback when they do, and we'll rebuild the list then
		if (pchName && *pchName)
		{
			LobbyMember_t lobbyMember;
			lobbyMember.m_steamIDMember = steamIDLobbyMember;
			sprintf_s(lobbyMember.m_memberName, "%s", pchName);
			m_lobbyMemberList.push_back(lobbyMember);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles a user in the lobby changing their name or details
//			( note: joining and leaving is handled below by CSteamLobbyBrowser::OnLobbyChatUpdate() )
//-----------------------------------------------------------------------------
void CSteamLobbyBrowser::OnPersonaStateChange(PersonaStateChange_t* pCallback)
{
	CSteamID steamLobbyID = getSteamLobbyID();
	// callbacks are broadcast to all listeners, so we'll get this for every friend who changes state
	// so make sure the user is in the lobby before acting
	if (!SteamFriends()->IsUserInSource(pCallback->m_ulSteamID, steamLobbyID))
		return;

	int cLobbyMembers = SteamMatchmaking()->GetNumLobbyMembers(steamLobbyID);
	m_lobbyMemberList.clear();
	CSteamID curUserID = SteamUser()->GetSteamID();
	for (int i = 0; i < cLobbyMembers; i++)
	{
		CSteamID steamIDLobbyMember = SteamMatchmaking()->GetLobbyMemberByIndex(steamLobbyID, i);

		// ignore yourself.
		if (curUserID == steamIDLobbyMember)
			continue;
		// we get the details of a user from the ISteamFriends interface
		const char* pchName = SteamFriends()->GetFriendPersonaName(steamIDLobbyMember);
		// we may not know the name of the other users in the lobby immediately; but we'll receive
		// a PersonaStateUpdate_t callback when they do, and we'll rebuild the list then
		if (pchName && *pchName)
		{
			LobbyMember_t lobbyMember;
			lobbyMember.m_steamIDMember = steamIDLobbyMember;
			sprintf_s(lobbyMember.m_memberName, "%s", pchName);
			m_lobbyMemberList.push_back(lobbyMember);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles users in the lobby joining or leaving
//-----------------------------------------------------------------------------
void CSteamLobbyBrowser::OnLobbyChatUpdate(LobbyChatUpdate_t* pCallback)
{
	// callbacks are broadcast to all listeners, so we'll get this for every lobby we're requesting
	CSteamID steamLobbyID = getSteamLobbyID();
	if (steamLobbyID != pCallback->m_ulSteamIDLobby)
		return;

	if (pCallback->m_ulSteamIDUserChanged == SteamUser()->GetSteamID().ConvertToUint64() &&
		(pCallback->m_rgfChatMemberStateChange &
			(k_EChatMemberStateChangeLeft |
				k_EChatMemberStateChangeDisconnected |
				k_EChatMemberStateChangeKicked |
				k_EChatMemberStateChangeBanned)))
	{
		// we've left the lobby, so it is now invalid
		setSteamLobbyID(CSteamID());
	}

	printf("Lobby chat update\n");
	int cLobbyMembers = SteamMatchmaking()->GetNumLobbyMembers(steamLobbyID);
	m_lobbyMemberList.clear();
	CSteamID curUserID = SteamUser()->GetSteamID();
	for (int i = 0; i < cLobbyMembers; i++)
	{
		CSteamID steamIDLobbyMember = SteamMatchmaking()->GetLobbyMemberByIndex(steamLobbyID, i);

		// ignore yourself.
		if (curUserID == steamIDLobbyMember)
			continue;
		// we get the details of a user from the ISteamFriends interface
		const char* pchName = SteamFriends()->GetFriendPersonaName(steamIDLobbyMember);
		// we may not know the name of the other users in the lobby immediately; but we'll receive
		// a PersonaStateUpdate_t callback when they do, and we'll rebuild the list then
		if (pchName && *pchName)
		{
			LobbyMember_t lobbyMember;
			lobbyMember.m_steamIDMember = steamIDLobbyMember;
			sprintf_s(lobbyMember.m_memberName, "%s", pchName);
			m_lobbyMemberList.push_back(lobbyMember);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Finishes up entering a lobby
//-----------------------------------------------------------------------------
void CSteamLobbyBrowser::OnLobbyEntered(LobbyEnter_t* pCallback, bool bIOFailure)
{

	if (pCallback->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess)
	{
		// failed, show error
		g_ModuleInterface->Print(CM_RED, "Failed to enter lobby");
		return;
	}
	/*
	* TODO: Move this connection code to when the client actually connects with the host
	printf("relay network status: %d\n", SteamNetworkingUtils()->GetRelayNetworkStatus(nullptr));
	CSteamID lobbyOwner = SteamMatchmaking()->GetLobbyOwner(pCallback->m_ulSteamIDLobby);
	SteamNetworkingIdentity identity;
	memset(&identity, 0, sizeof(SteamNetworkingIdentity));
	identity.m_eType = k_ESteamNetworkingIdentityType_SteamID;
	identity.SetSteamID(lobbyOwner.ConvertToUint64());
	// success
	printf("Entered lobby\n");
	SteamNetworkingSockets()->ConnectP2P(identity, 0, 0, nullptr);
	*/
	// move forward the state

}

// enums for use in 
enum EDisconnectReason
{
	k_EDRClientDisconnect = k_ESteamNetConnectionEnd_App_Min + 1,
	k_EDRServerClosed = k_ESteamNetConnectionEnd_App_Min + 2,
	k_EDRServerReject = k_ESteamNetConnectionEnd_App_Min + 3,
	k_EDRServerFull = k_ESteamNetConnectionEnd_App_Min + 4,
	k_EDRClientKicked = k_ESteamNetConnectionEnd_App_Min + 5
};

//-----------------------------------------------------------------------------
// Purpose: Handle any connection status change
//-----------------------------------------------------------------------------
void CSteamLobbyBrowser::OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pCallback)
{
	// Ignore if it's the host for now
	if (isHost)
	{
		return;
	}
	/// Connection handle
	HSteamNetConnection m_hConn = pCallback->m_hConn;

	/// Full connection info
	SteamNetConnectionInfo_t m_info = pCallback->m_info;

	/// Previous state.  (Current state is in m_info.m_eState)
	ESteamNetworkingConnectionState m_eOldState = pCallback->m_eOldState;

	if ((m_eOldState == k_ESteamNetworkingConnectionState_Connecting || m_eOldState == k_ESteamNetworkingConnectionState_FindingRoute) && m_info.m_eState == k_ESteamNetworkingConnectionState_Connected)
	{
		printf("Connected to Host\n");
		hasConnected = true;
		clientIDToSteamIDMap[0] = getSteamLobbyHostID().ConvertToUint64();
		messageHandlerThread = std::thread(clientReceiveMessageHandler);
	}

	//-----------------------------------------------------------------------------
	// Triggered when a server rejects our connection
	//-----------------------------------------------------------------------------
	if ((m_eOldState == k_ESteamNetworkingConnectionState_Connecting || m_eOldState == k_ESteamNetworkingConnectionState_Connected) &&
		m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer)
	{
		// close the connection with the server
		SteamNetworkingSockets()->CloseConnection(m_hConn, m_info.m_eEndReason, nullptr, false);
		switch (m_info.m_eEndReason)
		{
		case k_EDRServerReject:
//			OnReceiveServerAuthenticationResponse(false, 0);
			break;
		case k_EDRServerFull:
//			OnReceiveServerFullResponse();
			break;
		}
	}
	//-----------------------------------------------------------------------------
	// Triggered if our connection to the server fails
	//-----------------------------------------------------------------------------
	else if ((m_eOldState == k_ESteamNetworkingConnectionState_Connecting || m_eOldState == k_ESteamNetworkingConnectionState_Connected) &&
		m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
	{
		// failed, error out
		g_ModuleInterface->Print(CM_RED, "Failed to make P2P connection, quiting server");
		SteamNetworkingSockets()->CloseConnection(m_hConn, m_info.m_eEndReason, nullptr, false);
//		OnReceiveServerExiting();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Steam is asking us to join a game, based on the user selecting
//			'join game' on a friend in their friends list 
//			the string comes from the "connect" field set in the friends' rich presence
//-----------------------------------------------------------------------------
void CSteamLobbyBrowser::OnGameJoinRequested(GameRichPresenceJoinRequested_t* pCallback)
{
	// parse out the connect 
	const char* pchLobbyID = nullptr;

	const char* pchCmdLine = pCallback->m_rgchConnect;
	printf("On game join requested %s\n", pchCmdLine);
	const char* pchConnectLobbyParam = "+connect_lobby ";
	const char* pchConnectLobby = strstr(pchCmdLine, pchConnectLobbyParam);
	if (pchConnectLobby && strlen(pchCmdLine) > (pchConnectLobby - pchCmdLine) + strlen(pchConnectLobbyParam))
	{
		// lobby ID should be right after the +connect_lobby
		pchLobbyID = pchCmdLine + (pchConnectLobby - pchCmdLine) + strlen(pchConnectLobbyParam);
	}

	if (pchLobbyID == nullptr)
	{
		g_ModuleInterface->Print(CM_RED, "Couldn't find lobby id from connect_lobby");
	}
	else
	{
		// TODO: Bring the player directly to the lobby menu and connect to the lobby
		CSteamID steamIDLobby(static_cast<uint64>(atoll(pchLobbyID)));
		SteamMatchmaking()->JoinLobby(steamIDLobby);
		printf("Joined lobby\n");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Steam is asking us to join a lobby. Seems like this is called when the player
//			clicks 'join game' in their friend list even though the documentation says
//			that should be handled by GameRichPresenceJoinRequested_t
//-----------------------------------------------------------------------------
void CSteamLobbyBrowser::OnGameLobbyJoinRequested(GameLobbyJoinRequested_t* pCallback)
{
	// TODO: Bring the player directly to the lobby menu and connect to the lobby
	// TODO: Should probably check if the player is already in a lobby/other multiplayer menu
	setSteamLobbyID(pCallback->m_steamIDLobby);
	SteamMatchmaking()->JoinLobby(pCallback->m_steamIDLobby);
	isInLobby = true;
	isInSteamLobby = true;
	hasJoinedSteamLobby = true;
	printf("Joined lobby\n");
}

void CSteamLobbyBrowser::setSteamLobbyID(CSteamID steamLobbyID)
{
	m_steamLobbyID = steamLobbyID;
}

CSteamID CSteamLobbyBrowser::getSteamLobbyID()
{
	return m_steamLobbyID;
}

void CSteamLobbyBrowser::setSteamLobbyHostID(CSteamID steamLobbyHostID)
{
	m_steamLobbyHostID = steamLobbyHostID;
}

CSteamID CSteamLobbyBrowser::getSteamLobbyHostID()
{
	return m_steamLobbyHostID;
}

steamConnection& CSteamLobbyBrowser::getSteamLobbyHostConnection()
{
	return m_steamLobbyHostConnection;
}

//-----------------------------------------------------------------------------
// Purpose: Received message from Steam lobby. Is only used for inviting clients
//			to a host game for now
//-----------------------------------------------------------------------------
void CSteamLobbyBrowser::OnLobbyChatMessage(LobbyChatMsg_t* pCallback)
{
	if (pCallback->m_ulSteamIDLobby != getSteamLobbyID() || isHost)
	{
		return;
	}
	uint64 receivedSteamID = 0;
	SteamMatchmaking()->GetLobbyChatEntry(getSteamLobbyID(), pCallback->m_iChatID, nullptr, &receivedSteamID, sizeof(receivedSteamID), nullptr);
	if (receivedSteamID != SteamUser()->GetSteamID())
	{
		return;
	}
	// TODO: Prompt client to accept the connection
	// TODO: Need to replace this
	SteamNetworkingIdentity hostIdentity{};
	m_steamLobbyHostID = CSteamID(pCallback->m_ulSteamIDUser);
	hostIdentity.SetSteamID(m_steamLobbyHostID);
	m_steamLobbyHostConnection = steamConnection(SteamNetworkingSockets()->ConnectP2P(hostIdentity, 0, 0, nullptr));
}