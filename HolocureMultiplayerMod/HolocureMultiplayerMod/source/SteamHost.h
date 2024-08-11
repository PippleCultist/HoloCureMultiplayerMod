#pragma once
#include "steam/isteamnetworkingsockets.h" 
#include "steam/steamclientpublic.h"
#include "steam/isteamuser.h"
#include "steam/isteammatchmaking.h"

#define MAX_PLAYERS_PER_SERVER 4
#define MAX_PLAYERS_PER_LOBBY 10

struct ClientConnectionData_t
{
	bool m_bActive;					// Is this slot in use? Or is it available for new connections?
	CSteamID m_SteamIDUser;			// What is the steamid of the player?
	uint64 m_ulTickCountLastData;	// What was the last time we got data from the player?
	HSteamNetConnection m_hConn;	// The handle for the connection to the player

	ClientConnectionData_t() {
		m_bActive = false;
		m_ulTickCountLastData = 0;
		m_hConn = 0;
	}
};

class CSteamHost
{
public:
	//Constructor
	CSteamHost(bool isNewLobby);

	// Destructor
	~CSteamHost();
private:

	HSteamListenSocket m_hListenSocket;
	HSteamNetPollGroup m_hNetPollGroup;

	// Vector to keep track of client connections
	ClientConnectionData_t m_rgClientData[MAX_PLAYERS_PER_LOBBY];

	// Vector to keep track of client connections which are pending auth
	ClientConnectionData_t m_rgPendingClientData[MAX_PLAYERS_PER_LOBBY];

	// TODO: Should probably move this to client code instead of server
	// callback for when we're creating a new lobby
	void OnLobbyCreated(LobbyCreated_t* pCallback, bool bIOFailure);
	CCallResult<CSteamHost, LobbyCreated_t> m_SteamCallResultLobbyCreated;

	// client connection state
	// All connection changes are handled through this callback
	struct CCallbackInternal_OnNetConnectionStatusChanged : private CCallbackImpl< sizeof(SteamNetConnectionStatusChangedCallback_t) > {
		CCallbackInternal_OnNetConnectionStatusChanged() {
			SteamAPI_RegisterCallback(this, SteamNetConnectionStatusChangedCallback_t::k_iCallback);
		} CCallbackInternal_OnNetConnectionStatusChanged(const CCallbackInternal_OnNetConnectionStatusChanged&) {
			SteamAPI_RegisterCallback(this, SteamNetConnectionStatusChangedCallback_t::k_iCallback);
		} CCallbackInternal_OnNetConnectionStatusChanged& operator=(const CCallbackInternal_OnNetConnectionStatusChanged&) {
			return *this;
		} private: virtual void Run(void* pvParam) {
			CSteamHost* pOuter = reinterpret_cast<CSteamHost*>(reinterpret_cast<char*>(this) - ((::size_t) & reinterpret_cast<char const volatile&>((((CSteamHost*)0)->m_steamcallback_OnNetConnectionStatusChanged)))); pOuter->OnNetConnectionStatusChanged(reinterpret_cast<SteamNetConnectionStatusChangedCallback_t*>(pvParam));
		}
	} m_steamcallback_OnNetConnectionStatusChanged; void OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pParam);
};