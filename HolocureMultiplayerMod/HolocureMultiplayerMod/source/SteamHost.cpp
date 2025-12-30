#include "SteamHost.h"
#include "ModuleMain.h"
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"
#include "ScriptFunctions.h"
#include "SteamLobbyBrowser.h"
#include "MessageStructs.h"
#include "NetworkFunctions.h"
#include <random>
#include <thread>

extern menuGrid lobbyMenuGrid;
extern CSteamLobbyBrowser* steamLobbyBrowser;
extern std::unordered_map<uint64, uint32_t> steamIDToClientIDMap;
extern std::unordered_map<uint32_t, uint64> clientIDToSteamIDMap;
extern std::unordered_map<uint64, steamConnection> steamIDToConnectionMap;
extern std::unordered_map<uint32_t, bool> hasClientPlayerDisconnected;
extern uint32_t curUnusedPlayerID;
extern std::thread messageHandlerThread;

// Network message types
enum EMessage
{
	// Server messages
	k_EMsgServerBegin = 0,
	k_EMsgServerSendInfo = k_EMsgServerBegin + 1,
	k_EMsgServerFailAuthentication = k_EMsgServerBegin + 2,
	k_EMsgServerPassAuthentication = k_EMsgServerBegin + 3,
	k_EMsgServerUpdateWorld = k_EMsgServerBegin + 4,
	k_EMsgServerExiting = k_EMsgServerBegin + 5,
	k_EMsgServerPingResponse = k_EMsgServerBegin + 6,

	// Client messages
	k_EMsgClientBegin = 500,
	k_EMsgClientBeginAuthentication = k_EMsgClientBegin + 2,
	k_EMsgClientSendLocalUpdate = k_EMsgClientBegin + 3,

	// P2P authentication messages
	k_EMsgP2PBegin = 600,
	k_EMsgP2PSendingTicket = k_EMsgP2PBegin + 1,

	// voice chat messages
	k_EMsgVoiceChatBegin = 700,
	//k_EMsgVoiceChatPing = k_EMsgVoiceChatBegin+1,	// deprecated keep alive message
	k_EMsgVoiceChatData = k_EMsgVoiceChatBegin + 2,	// voice data from another player



	// force 32-bit size enum so the wire protocol doesn't get outgrown later
	k_EForceDWORD = 0x7fffffff,
};

CSteamHost::CSteamHost(bool isNewLobby)
{
	// zero the client connection data
	memset(&m_rgClientData, 0, sizeof(m_rgClientData));
	memset(&m_rgPendingClientData, 0, sizeof(m_rgPendingClientData));
	if (SteamNetworkingSockets())
	{
		printf("relay network status: %d\n", SteamNetworkingUtils()->GetRelayNetworkStatus(nullptr));
		// create the listen socket for listening for players connecting

		m_hListenSocket = SteamNetworkingSockets()->CreateListenSocketP2P(0, 0, nullptr);

		// create the poll group
//		m_hNetPollGroup = SteamNetworkingSockets()->CreatePollGroup();
		DbgPrintEx(LOG_SEVERITY_INFO, "Finished initializing host");
		callbackManagerInterfacePtr->LogToFile(MODNAME, "Initializing host");

		if (isNewLobby)
		{
			callbackManagerInterfacePtr->LogToFile(MODNAME, "Creating lobby");
			SteamAPICall_t hSteamAPICall = SteamMatchmaking()->CreateLobby(k_ELobbyTypeFriendsOnly, MAX_PLAYERS_PER_LOBBY);
			// set the function to call when this completes
			m_SteamCallResultLobbyCreated.Set(hSteamAPICall, this, &CSteamHost::OnLobbyCreated);
		}
	}
	printf("Listen Socket %d\n", m_hListenSocket);
}

CSteamHost::~CSteamHost()
{
	steamLobbyBrowser->leaveLobby();
	SteamNetworkingSockets()->CloseListenSocket(m_hListenSocket);
	SteamNetworkingSockets()->DestroyPollGroup(m_hNetPollGroup);
}

//-----------------------------------------------------------------------------
// Purpose: Finishes up entering a lobby of our own creation
//-----------------------------------------------------------------------------
void CSteamHost::OnLobbyCreated(LobbyCreated_t* pCallback, bool bIOFailure)
{
	//	if (m_eGameState != k_EClientCreatingLobby)
	//		return;

		// record which lobby we're in
	if (pCallback->m_eResult == k_EResultOK)
	{
		// success
//		m_steamIDLobby = pCallback->m_ulSteamIDLobby;
//		m_pLobby->SetLobbySteamID(m_steamIDLobby);

		// set the name of the lobby if it's ours
//		char rgchLobbyName[256];
//		sprintf_safe(rgchLobbyName, "%s's lobby", SteamFriends()->GetPersonaName());
		callbackManagerInterfacePtr->LogToFile(MODNAME, "Lobby created");
		printf("Setting lobby data\n");
		// TODO: Probably should improve on the lobby name
		// Currently just generate a random number for the lobby name
		std::random_device rd;
		std::default_random_engine generator(rd());
		std::uniform_int_distribution<uint32_t> distribution(0);
		uint32_t lobbyNum = distribution(generator);
		SteamMatchmaking()->SetLobbyData(pCallback->m_ulSteamIDLobby, "name", std::format("Lobby {}", lobbyNum).c_str());

		if (SteamMatchmaking()->GetLobbyOwner(pCallback->m_ulSteamIDLobby) != SteamUser()->GetSteamID())
		{
			callbackManagerInterfacePtr->LogToFile(MODNAME, "Lobby owner doesn't match current steam id. Potential issue with steamworks not giving the correct lobby id.");
			DbgPrintEx(LOG_SEVERITY_WARNING, "Lobby owner doesn't match current steam id. Potential issue with steamworks not giving the correct lobby id.");
		}

		// Setting this to NULL removes the key pair
		SteamFriends()->SetRichPresence("connect", std::format("+connect_lobby {}", pCallback->m_ulSteamIDLobby).c_str());

		steamLobbyBrowser->setSteamLobbyID(pCallback->m_ulSteamIDLobby);
		holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, lobbyMenuGrid.menuGridPtr);
		// TODO: Problem is if the lobby owner is keeping track of the ids of each player, then that information will be lost if they leave the lobby
		// TODO: Might need to assign a new owner and give the ids to them? Problem is if there is a bad actor that gets assigned as the owner
		// TODO: I guess for now since it's a friend lobby, showing the steam name directly is okay.
		
		// mark that we're in the lobby
//		SetGameState(k_EClientInLobby);
	}
	else
	{
		// failed, show error
		callbackManagerInterfacePtr->LogToFile(MODNAME, "Failed to create lobby (lost connection to Steam back-end servers.");
		DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to create lobby (lost connection to Steam back-end servers.");
		//		SetGameState(k_EClientGameConnectionFailure);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle any connection status change
//-----------------------------------------------------------------------------
void CSteamHost::OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pCallback)
{
	/// Connection handle
	HSteamNetConnection hConn = pCallback->m_hConn;

	/// Full connection info
	SteamNetConnectionInfo_t info = pCallback->m_info;

	/// Previous state.  (Current state is in m_info.m_eState)
	ESteamNetworkingConnectionState eOldState = pCallback->m_eOldState;

	// Parse information to know what was changed

	// Check if a client has connected
	if (info.m_hListenSocket &&
		eOldState == k_ESteamNetworkingConnectionState_None &&
		info.m_eState == k_ESteamNetworkingConnectionState_Connecting)
	{
		CSteamID steamClientID = info.m_identityRemote.GetSteamID();
		auto mapFind = steamIDToClientIDMap.find(steamClientID.ConvertToUint64());
		if (mapFind != steamIDToClientIDMap.end())
		{
			if (mapFind->second == 0)
			{
				EResult res = SteamNetworkingSockets()->AcceptConnection(hConn);
				if (res != k_EResultOK)
				{
					callbackManagerInterfacePtr->LogToFile(MODNAME, "Failed to accept connection");
					DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to accept connection");
					SteamNetworkingSockets()->CloseConnection(hConn, k_ESteamNetConnectionEnd_AppException_Generic, "Failed to accept connection", false);
					return;
				}
				uint32_t newClientID = curUnusedPlayerID;
				curUnusedPlayerID++;
				steamIDToClientIDMap[steamClientID.ConvertToUint64()] = newClientID;
				clientIDToSteamIDMap[newClientID] = steamClientID.ConvertToUint64();
				steamIDToConnectionMap[steamClientID.ConvertToUint64()] = steamConnection(hConn);

				// TODO: Handle if the message isn't 
				playerPingMap[newClientID] = 0;
				hasClientPlayerDisconnected[newClientID] = false;
				lobbyPlayerDataMap[newClientID] = lobbyPlayerData();
				sendClientIDMessage(newClientID);
				if (!hasConnected)
				{
					// Just to make sure that the thread has ended before trying to create a new thread
					if (messageHandlerThread.joinable())
					{
						messageHandlerThread.join();
					}
					hasConnected = true;
					messageHandlerThread = std::thread(hostReceiveMessageHandler);
				}
				callbackManagerInterfacePtr->LogToFile(MODNAME, "Successfully accepted connection");
				printf("Successfully accepted connection\n");
			}
			else
			{
				callbackManagerInterfacePtr->LogToFile(MODNAME, "Client is trying to connect again");
				printf("Client %llu is trying to connect again", steamClientID.ConvertToUint64());
			}
		}
		else
		{
			callbackManagerInterfacePtr->LogToFile(MODNAME, "Connection rejected from client");
			DbgPrintEx(LOG_SEVERITY_ERROR, "Connection rejected from %llu", steamClientID.ConvertToUint64());
		}
		// Connection from a new client
		// Search for an available slot
		/*
		for (uint32 i = 0; i < MAX_PLAYERS_PER_LOBBY; ++i)
		{
			if (!m_rgClientData[i].m_bActive && !m_rgPendingClientData[i].m_hConn)
			{
				// Found one.  "Accept" the connection.
				EResult res = SteamNetworkingSockets()->AcceptConnection(hConn);
				if (res != k_EResultOK)
				{
					DbgPrintEx(LOG_SEVERITY_ERROR, "Failed to accept connection");
					SteamNetworkingSockets()->CloseConnection(hConn, k_ESteamNetConnectionEnd_AppException_Generic, "Failed to accept connection", false);
					return;
				}

				m_rgPendingClientData[i].m_hConn = hConn;

				// add the user to the poll group
				SteamNetworkingSockets()->SetConnectionPollGroup(hConn, m_hNetPollGroup);

				// TODO: Add a lobby menu where people can create their own lobby and invite people



				return;
			}
		}

		// No empty slots.  Server full!
		DbgPrintEx(LOG_SEVERITY_ERROR, "Rejecting connection; server full");
		SteamNetworkingSockets()->CloseConnection(hConn, k_ESteamNetConnectionEnd_AppException_Generic, "Server full!", false);
		*/
	}
	// Check if a client has disconnected
	else if ((eOldState == k_ESteamNetworkingConnectionState_Connecting || eOldState == k_ESteamNetworkingConnectionState_Connected) &&
		info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer)
	{
		// Handle disconnecting a client
		for (uint32 i = 0; i < MAX_PLAYERS_PER_LOBBY; ++i)
		{
			// If there is no ship, skip
			if (!m_rgClientData[i].m_bActive)
				continue;

			if (m_rgClientData[i].m_SteamIDUser == info.m_identityRemote.GetSteamID())//pCallback->m_steamIDRemote)
			{
				// TODO: Disconnect client
				break;
			}
		}
	}
}