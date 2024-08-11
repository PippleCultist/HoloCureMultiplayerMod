#pragma once

#include "ScriptFunctions.h"
#include "steam/isteammatchmaking.h"
#include "steam/isteamnetworkingsockets.h"
#include <Windows.h>
#include <vector>

// an item in the list of lobbies we've found to display
struct Lobby_t
{
	CSteamID m_steamIDLobby;
	char m_rgchName[256];
};

struct LobbyMember_t
{
	CSteamID m_steamIDMember;
	char m_memberName[256];
};

class CSteamLobbyBrowser
{
public:
	CSteamLobbyBrowser();

	std::vector<LobbyMember_t> m_lobbyMemberList;

	void setSteamLobbyID(CSteamID steamLobbyID);
	CSteamID getSteamLobbyID();

	void setSteamLobbyHostID(CSteamID steamLobbyHostID);
	CSteamID getSteamLobbyHostID();

	void leaveLobby();

	steamConnection& getSteamLobbyHostConnection();
private:
	CSteamID m_steamLobbyID;

	CSteamID m_steamLobbyHostID;
	steamConnection m_steamLobbyHostConnection;

	CCallResult<CSteamLobbyBrowser, LobbyMatchList_t> m_SteamCallResultLobbyMatchList;
	void OnLobbyMatchListCallback(LobbyMatchList_t* pLobbyMatchList, bool bIOFailure);
	STEAM_CALLBACK(CSteamLobbyBrowser, OnLobbyDataUpdatedCallback, LobbyDataUpdate_t, m_CallbackLobbyDataUpdated);
	//	CCallback< CLobbyBrowser, LobbyDataUpdate_t > m_CallbackLobbyDataUpdated; void OnLobbyDataUpdatedCallback(LobbyDataUpdate_t* pParam);
	// 
	// callback for when we've joined a lobby
	void OnLobbyEntered(LobbyEnter_t* pCallback, bool bIOFailure);
	CCallResult<CSteamLobbyBrowser, LobbyEnter_t> m_SteamCallResultLobbyEntered;

	// user state change handler
	STEAM_CALLBACK(CSteamLobbyBrowser, OnPersonaStateChange, PersonaStateChange_t, m_CallbackPersonaStateChange);

	// Called when we get new connections, or the state of a connection changes
	STEAM_CALLBACK(CSteamLobbyBrowser, OnNetConnectionStatusChanged, SteamNetConnectionStatusChangedCallback_t);

	STEAM_CALLBACK(CSteamLobbyBrowser, OnGameJoinRequested, GameRichPresenceJoinRequested_t, m_CallbackGameJoinRequested);

	STEAM_CALLBACK(CSteamLobbyBrowser, OnLobbyChatUpdate, LobbyChatUpdate_t, m_CallbackChatDataUpdate);

	STEAM_CALLBACK(CSteamLobbyBrowser, OnGameLobbyJoinRequested, GameLobbyJoinRequested_t);

	STEAM_CALLBACK(CSteamLobbyBrowser, OnLobbyChatMessage, LobbyChatMsg_t);
};