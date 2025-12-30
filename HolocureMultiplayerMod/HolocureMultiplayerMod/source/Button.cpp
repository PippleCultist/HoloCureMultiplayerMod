#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#include <WS2tcpip.h> // Apparently important that this gets included before everything else
#include "Button.h"
#include "ModuleMain.h"
#include "CodeEvents.h"
#include "SteamHost.h"
#include "NetworkFunctions.h"
#include "CommonFunctions.h"
#include "SteamLobbyBrowser.h"
#include <Windows.h>
#include <fstream>
#include <iphlpapi.h>
#include <thread>

extern CSteamHost* steamHost;
extern std::unordered_map<uint32_t, int> playerPingMap;
extern std::unordered_map<uint32_t, lobbyPlayerData> lobbyPlayerDataMap;
extern std::unordered_map<uint64, uint32_t> steamIDToClientIDMap;
extern int numClientsInGame;
extern CSteamID curSelectedSteamID;
extern CSteamLobbyBrowser* steamLobbyBrowser;
extern std::thread messageHandlerThread;

std::shared_ptr<menuData> multiplayerMenuUseSavedNetworkAdapter(new menuDataButton(440, 104 + 29 * 0, 180, 29, "MULTIPLAYERMENU_UseSavedNetworkAdapter", "Use saved network adapter", true, clickUseSavedNetworkAdapter, nullptr));
std::shared_ptr<menuData> multiplayerMenuSelectNetworkAdapter(new menuDataButton(440, 104 + 29 * 1, 180, 29, "MULTIPLAYERMENU_SelectNetworkAdapter", "Select network adapter", true, clickSelectNetworkAdapter, nullptr));
std::shared_ptr<menuData> multiplayerMenuCreateFriendsSteamLobby(new menuDataButton(440, 104 + 29 * 2, 180, 29, "MULTIPLAYERMENU_CreateFriendsSteamLobby", "Create friend Steam lobby", true, clickCreateFriendsSteamLobby, nullptr));
menuGrid multiplayerMenuGrid({
	menuColumn({
		multiplayerMenuUseSavedNetworkAdapter,
		multiplayerMenuSelectNetworkAdapter,
		multiplayerMenuCreateFriendsSteamLobby,
	})
}, "Multiplayer", nullptr);

std::shared_ptr<menuData> networkAdapterDisclaimerMenuDisclaimer(new menuDataTextOutline(320, 100, "NETWORKADAPTERDISCLAIMERMENU_Disclaimer", "DISCLAIMER: Please be aware that clicking the OK button will bring up a list of your computer's network adapter names which may contain private/sensitive information.", true, nullptr, nullptr, 1, 0x000000, 14, 20, 440, 0x0FFFFF, 1));
std::shared_ptr<menuData> networkAdapterDisclaimerMenuDisclaimerOK(new menuDataButton(230, 250, 180, 29, "NETWORKADAPTERDISCLAIMERMENU_DisclaimerOK", "OK", true, clickNetworkAdapterDisclaimerOK, nullptr));
menuGrid networkAdapterDisclaimerMenuGrid({
	menuColumn({
		networkAdapterDisclaimerMenuDisclaimer,
		networkAdapterDisclaimerMenuDisclaimerOK,
	})
}, "Network Adapter Disclaimer", &multiplayerMenuGrid, nullptr, networkAdapterDisclaimerMenuGridReturn, nullptr);

std::vector<std::shared_ptr<menuData>> networkAdapterNameList({
	std::shared_ptr<menuData>(new menuDataButton(440, 104 + 29 * 0, 180, 29, "NETWORKADAPTERNAMEMENU_Name1", "", false, clickNetworkAdapterName, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(440, 104 + 29 * 1, 180, 29, "NETWORKADAPTERNAMEMENU_Name2", "", false, clickNetworkAdapterName, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(440, 104 + 29 * 2, 180, 29, "NETWORKADAPTERNAMEMENU_Name3", "", false, clickNetworkAdapterName, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(440, 104 + 29 * 3, 180, 29, "NETWORKADAPTERNAMEMENU_Name4", "", false, clickNetworkAdapterName, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(440, 104 + 29 * 4, 180, 29, "NETWORKADAPTERNAMEMENU_Name5", "", false, clickNetworkAdapterName, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(440, 104 + 29 * 5, 180, 29, "NETWORKADAPTERNAMEMENU_NameNext", "", false, clickNetworkAdapterNameNext, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(440, 104 + 29 * 6, 180, 29, "NETWORKADAPTERNAMEMENU_NamePrev", "", false, clickNetworkAdapterNamePrev, nullptr)),
});
menuGrid networkAdapterNamesMenuGrid({
	menuColumn({
		networkAdapterNameList
	})
}, "Network Adapter Names", &multiplayerMenuGrid, nullptr, networkAdapterDisclaimerMenuGridReturn, nullptr);

std::shared_ptr<menuData> LANMenuHostLanSession(new menuDataButton(440, 104 + 29 * 0, 180, 20, "LANMenu_HostLanSession", "Host LAN Session", true, clickHostLanSession, nullptr));
std::shared_ptr<menuData> LANMenuJoinLanSession(new menuDataButton(440, 104 + 29 * 1, 180, 20, "LANMenu_JoinLanSession", "Join LAN Session", true, clickJoinLanSession, nullptr));
menuGrid lanSessionMenuGrid({
	menuColumn({
		LANMenuHostLanSession,
		LANMenuJoinLanSession,
	})
}, "LAN Session", &multiplayerMenuGrid, nullptr, LANSessionMenuGridReturn, nullptr);

std::vector<std::shared_ptr<menuData>> lobbyMenuLobbyPlayerNameList({
	std::shared_ptr<menuData>(new menuDataTextOutline(20, 14 + 29 * 0, "STEAMLOBBYMENU_LobbyPlayerName1", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(20, 14 + 29 * 1, "STEAMLOBBYMENU_LobbyPlayerName2", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(20, 14 + 29 * 2, "STEAMLOBBYMENU_LobbyPlayerName3", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(20, 14 + 29 * 3, "STEAMLOBBYMENU_LobbyPlayerName4", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(20, 14 + 29 * 4, "STEAMLOBBYMENU_LobbyPlayerName5", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(20, 14 + 29 * 5, "STEAMLOBBYMENU_LobbyPlayerName6", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(20, 14 + 29 * 6, "STEAMLOBBYMENU_LobbyPlayerName7", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(20, 14 + 29 * 7, "STEAMLOBBYMENU_LobbyPlayerName8", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(20, 14 + 29 * 8, "STEAMLOBBYMENU_LobbyPlayerName9", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
});
std::vector<std::shared_ptr<menuData>> lobbyMenuLobbyPlayerReadyList({
	std::shared_ptr<menuData>(new menuDataTextOutline(84, 14 + 29 * 0, "STEAMLOBBYMENU_LobbyPlayerReady1", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(84, 14 + 29 * 1, "STEAMLOBBYMENU_LobbyPlayerReady2", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(84, 14 + 29 * 2, "STEAMLOBBYMENU_LobbyPlayerReady3", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(84, 14 + 29 * 3, "STEAMLOBBYMENU_LobbyPlayerReady4", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(84, 14 + 29 * 4, "STEAMLOBBYMENU_LobbyPlayerReady5", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(84, 14 + 29 * 5, "STEAMLOBBYMENU_LobbyPlayerReady6", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(84, 14 + 29 * 6, "STEAMLOBBYMENU_LobbyPlayerReady7", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(84, 14 + 29 * 7, "STEAMLOBBYMENU_LobbyPlayerReady8", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
	std::shared_ptr<menuData>(new menuDataTextOutline(84, 14 + 29 * 8, "STEAMLOBBYMENU_LobbyPlayerReady9", "", false, nullptr, nullptr, 1, 0x000000, 14, 0, 100, 0xFFFFFF, 1)),
});

std::vector<std::shared_ptr<menuData>> lobbyMenuLobbyPlayerList({
	std::shared_ptr<menuData>(new menuDataButton(70, 14 + 29 * 0, 180, 20, "STEAMLOBBYMENU_LobbyPlayer1", "", false, clickLobbySteamPlayer, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(70, 14 + 29 * 1, 180, 20, "STEAMLOBBYMENU_LobbyPlayer2", "", false, clickLobbySteamPlayer, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(70, 14 + 29 * 2, 180, 20, "STEAMLOBBYMENU_LobbyPlayer3", "", false, clickLobbySteamPlayer, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(70, 14 + 29 * 3, 180, 20, "STEAMLOBBYMENU_LobbyPlayer4", "", false, clickLobbySteamPlayer, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(70, 14 + 29 * 4, 180, 20, "STEAMLOBBYMENU_LobbyPlayer5", "", false, clickLobbySteamPlayer, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(70, 14 + 29 * 5, 180, 20, "STEAMLOBBYMENU_LobbyPlayer6", "", false, clickLobbySteamPlayer, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(70, 14 + 29 * 6, 180, 20, "STEAMLOBBYMENU_LobbyPlayer7", "", false, clickLobbySteamPlayer, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(70, 14 + 29 * 7, 180, 20, "STEAMLOBBYMENU_LobbyPlayer8", "", false, clickLobbySteamPlayer, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(70, 14 + 29 * 8, 180, 20, "STEAMLOBBYMENU_LobbyPlayer9", "", false, clickLobbySteamPlayer, nullptr)),
});

std::vector<std::shared_ptr<menuData>> lobbyMenuLobbyPlayerInviteList({
	std::shared_ptr<menuData>(new menuDataButton(250, 14 + 29 * 0, 180, 20, "STEAMLOBBYMENU_LobbyPlayerInvite1", "Invite User", false, clickLobbySteamPlayerInvite, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(250, 14 + 29 * 1, 180, 20, "STEAMLOBBYMENU_LobbyPlayerInvite2", "Invite User", false, clickLobbySteamPlayerInvite, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(250, 14 + 29 * 2, 180, 20, "STEAMLOBBYMENU_LobbyPlayerInvite3", "Invite User", false, clickLobbySteamPlayerInvite, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(250, 14 + 29 * 3, 180, 20, "STEAMLOBBYMENU_LobbyPlayerInvite4", "Invite User", false, clickLobbySteamPlayerInvite, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(250, 14 + 29 * 4, 180, 20, "STEAMLOBBYMENU_LobbyPlayerInvite5", "Invite User", false, clickLobbySteamPlayerInvite, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(250, 14 + 29 * 5, 180, 20, "STEAMLOBBYMENU_LobbyPlayerInvite6", "Invite User", false, clickLobbySteamPlayerInvite, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(250, 14 + 29 * 6, 180, 20, "STEAMLOBBYMENU_LobbyPlayerInvite7", "Invite User", false, clickLobbySteamPlayerInvite, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(250, 14 + 29 * 7, 180, 20, "STEAMLOBBYMENU_LobbyPlayerInvite8", "Invite User", false, clickLobbySteamPlayerInvite, nullptr)),
	std::shared_ptr<menuData>(new menuDataButton(250, 14 + 29 * 8, 180, 20, "STEAMLOBBYMENU_LobbyPlayerInvite9", "Invite User", false, clickLobbySteamPlayerInvite, nullptr)),
});

std::shared_ptr<menuData> lobbyMenuLobbyChooseCharacter(new menuDataButton(440, 104 + 29 * 0, 180, 20, "LOBBYMENU_LobbyChooseCharacter", "Choose Character", false, clickLobbyChooseCharacter, nullptr));
std::shared_ptr<menuData> lobbyMenuLobbyReady(new menuDataButton(440, 104 + 29 * 1, 180, 20, "LOBBYMENU_LobbyReady", "Ready", false, clickLobbyReady, nullptr));
std::shared_ptr<menuData> lobbyMenuLobbySelectMap(new menuDataButton(440, 104 + 29 * 2, 180, 20, "LOBBYMENU_SelectMap", "Select Map", false, clickLobbySelectMap, nullptr));
std::shared_ptr<menuData> lobbyMenuLobbyStart(new menuDataButton(440, 104 + 29 * 3, 180, 20, "LOBBYMENU_Start", "Start", false, clickLobbyStart, nullptr));

menuGrid lobbyMenuGrid({
	menuColumn({
		lobbyMenuLobbyPlayerNameList
	}),
	menuColumn({
		lobbyMenuLobbyPlayerReadyList
	}),
	menuColumn({
		lobbyMenuLobbyPlayerList
	}),
	menuColumn({
		lobbyMenuLobbyPlayerInviteList
	}),
	menuColumn({
		lobbyMenuLobbyChooseCharacter,
		lobbyMenuLobbyReady,
		lobbyMenuLobbySelectMap,
		lobbyMenuLobbyStart,
	}),
}, "Lobby", &multiplayerMenuGrid, nullptr, lobbyMenuGridReturn, drawLobbyMenu);

menuGrid selectingCharacterMenuGrid({
}, "Selecting Character", &lobbyMenuGrid, nullptr, selectingCharacterMenuGridReturn, nullptr);

menuGrid selectingMapMenuGrid({
}, "Selecting Map", &lobbyMenuGrid, nullptr, selectingMapMenuGridReturn, nullptr);

void initButtonMenus()
{
	multiplayerMenuGrid.initMenuGrid();
	networkAdapterDisclaimerMenuGrid.initMenuGrid();
	networkAdapterNamesMenuGrid.initMenuGrid();
	lanSessionMenuGrid.initMenuGrid();
	lobbyMenuGrid.initMenuGrid();
	selectingCharacterMenuGrid.initMenuGrid();
	selectingMapMenuGrid.initMenuGrid();
}

IP_ADAPTER_ADDRESSES* adapterAddresses(NULL);
SOCKET broadcastSocket = INVALID_SOCKET;
sockaddr* broadcastSocketAddr = nullptr;
size_t broadcastSocketLen = 0;

extern int adapterPageNum;
extern int prevPageButtonNum;
extern int nextPageButtonNum;

void clickUseSavedNetworkAdapter()
{
	// Use saved adapter
	CreateDirectory(L"MultiplayerMod", NULL);
	if (!std::filesystem::exists("MultiplayerMod/lastUsedNetworkAdapter"))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Couldn't find the last used network adapter file");
		return;
	}
	std::ifstream inFile;
	inFile.open("MultiplayerMod/lastUsedNetworkAdapter");
	std::string line;
	if (!std::getline(inFile, line))
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Couldn't read network adapter name from MultiplayerMod/lastUsedNetworkAdapter");
		inFile.close();
		return;
	}

	IP_ADAPTER_ADDRESSES* adapter(NULL);

	DWORD adapterAddressesBufferSize = 16 * 1024;

	for (int i = 0; i < 3; i++)
	{
		adapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(malloc(adapterAddressesBufferSize));
		DWORD error = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME, NULL, adapterAddresses, &adapterAddressesBufferSize);
		if (error == ERROR_SUCCESS)
		{
			break;
		}
		else if (error == ERROR_BUFFER_OVERFLOW)
		{
			free(adapterAddresses);
			adapterAddresses = NULL;
			continue;
		}
		else
		{
			free(adapterAddresses);
			adapterAddresses = NULL;
			continue;
		}
	}

	for (adapter = adapterAddresses; adapter != NULL; adapter = adapter->Next)
	{
		if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
		{
			continue;
		}

		int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, static_cast<int>(wcslen(adapter->FriendlyName)), NULL, 0, NULL, NULL);
		std::string resString(sizeNeeded, 0);
		WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, static_cast<int>(wcslen(adapter->FriendlyName)), &resString[0], sizeNeeded, NULL, NULL);
		if (line.compare(resString) != 0)
		{
			continue;
		}

		holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, lanSessionMenuGrid.menuGridPtr);

		for (IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; address != NULL; address = address->Next)
		{
			auto family = address->Address.lpSockaddr->sa_family;
			if (family == AF_INET)
			{
				SOCKADDR_IN* ipv4 = reinterpret_cast<SOCKADDR_IN*>(address->Address.lpSockaddr);
				ULONG subnetMask;
				ConvertLengthToIpv4Mask(address->OnLinkPrefixLength, &subnetMask);
				ipv4->sin_addr.s_addr |= ~subnetMask;
				inet_ntop(AF_INET, &(ipv4->sin_addr), broadcastAddressBuffer, 16);

				struct addrinfo* res = nullptr, * it;
				struct addrinfo hints;
				memset(&hints, 0, sizeof(struct addrinfo));
				hints.ai_family = AF_INET;
				hints.ai_socktype = SOCK_DGRAM;

				getaddrinfo(broadcastAddressBuffer, BROADCAST_PORT, &hints, &res);

				for (it = res; it != NULL; it = it->ai_next)
				{
					broadcastSocket = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
					char enable = '1';
					setsockopt(broadcastSocket, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));
					u_long mode = 1;
					ioctlsocket(broadcastSocket, FIONBIO, &mode);
					broadcastSocketAddr = it->ai_addr;
					broadcastSocketLen = it->ai_addrlen;
					break;
				}
				break;
			}
		}
		inFile.close();
		free(adapterAddresses);
		adapterAddresses = NULL;
		return;
	}
	free(adapterAddresses);
	adapterAddresses = NULL;

	DbgPrintEx(LOG_SEVERITY_ERROR, "Couldn't find network adapter %s", line);
	inFile.close();
}

void clickSelectNetworkAdapter()
{
	holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, networkAdapterDisclaimerMenuGrid.menuGridPtr);
}

void clickCreateFriendsSteamLobby()
{
	DbgPrintEx(LOG_SEVERITY_INFO, "Hosting via steam");
	steamHost = new CSteamHost(true);
	isHost = true;
	playerPingMap.clear();
	lobbyPlayerDataMap.clear();
	hasClientPlayerDisconnected.clear();
	lobbyPlayerDataMap[0] = lobbyPlayerData();
	// TODO: Let the host decide their own name eventually
	lobbyPlayerDataMap[0].playerName = "0";
	holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, lobbyMenuGrid.menuGridPtr);
}

void reloadNetworkAdapters()
{
	if (adapterAddresses != NULL)
	{
		IP_ADAPTER_ADDRESSES* adapter(NULL);
		
		bool hasPrev = false;
		bool hasNext = false;
		int count = -1;
		int menuPos = 0;
		for (adapter = adapterAddresses; adapter != NULL; adapter = adapter->Next)
		{
			if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
			{
				continue;
			}

			bool isValidAddress = true;
			for (IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; address != NULL; address = address->Next)
			{
				auto family = address->Address.lpSockaddr->sa_family;
				if (family == AF_INET)
				{
					SOCKADDR_IN* ipv4 = reinterpret_cast<SOCKADDR_IN*>(address->Address.lpSockaddr);
					inet_ntop(AF_INET, &(ipv4->sin_addr), broadcastAddressBuffer, 16);

					if (strncmp("169.254", broadcastAddressBuffer, 7) == 0)
					{
						isValidAddress = false;
					}
					break;
				}
			}

			if (!isValidAddress)
			{
				continue;
			}

			count++;
			if (count < adapterPageNum * 5)
			{
				hasPrev = true;
				continue;
			}
			if (count >= (adapterPageNum + 1) * 5)
			{
				hasNext = true;
				break;
			}

			int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, static_cast<int>(wcslen(adapter->FriendlyName)), NULL, 0, NULL, NULL);
			std::string resString(sizeNeeded, 0);
			WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, static_cast<int>(wcslen(adapter->FriendlyName)), &resString[0], sizeNeeded, NULL, NULL);
			networkAdapterNameList[menuPos]->labelName = resString;
			networkAdapterNameList[menuPos]->isVisible = true;
			menuPos++;
		}
		for (int i = menuPos; i < networkAdapterNameList.size(); i++)
		{
			networkAdapterNameList[i]->isVisible = false;
		}
		if (hasPrev)
		{
			networkAdapterNameList[networkAdapterNameList.size() - 1]->isVisible = true;
		}
		else
		{
			prevPageButtonNum = -1;
		}
		if (hasNext)
		{
			networkAdapterNameList[networkAdapterNameList.size() - 2]->isVisible = true;
		}
		else
		{
			nextPageButtonNum = -1;
		}
	}
}

void clickNetworkAdapterDisclaimerOK()
{
	holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, networkAdapterNamesMenuGrid.menuGridPtr);
	adapterPageNum = 0;
	prevPageButtonNum = -1;
	nextPageButtonNum = -1;
	DWORD adapterAddressesBufferSize = 16 * 1024;

	for (int i = 0; i < 3; i++)
	{
		adapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(malloc(adapterAddressesBufferSize));
		DWORD error = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME, NULL, adapterAddresses, &adapterAddressesBufferSize);
		if (error == ERROR_SUCCESS)
		{
			break;
		}
		else if (error == ERROR_BUFFER_OVERFLOW)
		{
			free(adapterAddresses);
			adapterAddresses = NULL;
			continue;
		}
		else
		{
			free(adapterAddresses);
			adapterAddresses = NULL;
			continue;
		}
	}
	reloadNetworkAdapters();
}

void clickNetworkAdapterNameNext()
{
	adapterPageNum++;
	reloadNetworkAdapters();
}

void clickNetworkAdapterNamePrev()
{
	adapterPageNum--;
	reloadNetworkAdapters();
}

void clickNetworkAdapterName()
{
	std::shared_ptr<menuData> selectedMenuData;
	holoCureMenuInterfacePtr->GetSelectedMenuData(MODNAME, selectedMenuData);
	int selectedNameIndex = -1;
	for (int i = 0; i < networkAdapterNameList.size(); i++)
	{
		if (networkAdapterNameList[i].get() == selectedMenuData.get())
		{
			selectedNameIndex = i;
			break;
		}
	}
	
	IP_ADAPTER_ADDRESSES* adapter(NULL);

	int count = -1;
	for (adapter = adapterAddresses; adapter != NULL; adapter = adapter->Next)
	{
		if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
		{
			continue;
		}

		bool isValidAddress = true;
		for (IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; address != NULL; address = address->Next)
		{
			auto family = address->Address.lpSockaddr->sa_family;
			if (family == AF_INET)
			{
				SOCKADDR_IN* ipv4 = reinterpret_cast<SOCKADDR_IN*>(address->Address.lpSockaddr);
				inet_ntop(AF_INET, &(ipv4->sin_addr), broadcastAddressBuffer, 16);

				if (strncmp("169.254", broadcastAddressBuffer, 7) == 0)
				{
					isValidAddress = false;
				}
				break;
			}
		}

		if (!isValidAddress)
		{
			continue;
		}

		count++;
		if (adapterPageNum * 5 + selectedNameIndex != count)
		{
			continue;
		}

		CreateDirectory(L"MultiplayerMod", NULL);
		std::ofstream outFile;
		outFile.open("MultiplayerMod/lastUsedNetworkAdapter");
		int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, static_cast<int>(wcslen(adapter->FriendlyName)), NULL, 0, NULL, NULL);
		std::string resString(sizeNeeded, 0);
		WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, static_cast<int>(wcslen(adapter->FriendlyName)), &resString[0], sizeNeeded, NULL, NULL);
		outFile << resString;
		outFile.close();

		for (IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; address != NULL; address = address->Next)
		{
			auto family = address->Address.lpSockaddr->sa_family;
			if (family == AF_INET)
			{
				SOCKADDR_IN* ipv4 = reinterpret_cast<SOCKADDR_IN*>(address->Address.lpSockaddr);
				ULONG subnetMask;
				ConvertLengthToIpv4Mask(address->OnLinkPrefixLength, &subnetMask);
				ipv4->sin_addr.s_addr |= ~subnetMask;
				inet_ntop(AF_INET, &(ipv4->sin_addr), broadcastAddressBuffer, 16);

				struct addrinfo* res = nullptr, * it;
				struct addrinfo hints;
				memset(&hints, 0, sizeof(struct addrinfo));
				hints.ai_family = AF_INET;
				hints.ai_socktype = SOCK_DGRAM;

				getaddrinfo(broadcastAddressBuffer, BROADCAST_PORT, &hints, &res);

				for (it = res; it != NULL; it = it->ai_next)
				{
					broadcastSocket = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
					char enable = '1';
					setsockopt(broadcastSocket, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));
					u_long mode = 1;
					ioctlsocket(broadcastSocket, FIONBIO, &mode);
					broadcastSocketAddr = it->ai_addr;
					broadcastSocketLen = it->ai_addrlen;
					break;
				}
				break;
			}
		}
		break;
	}

	holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, lanSessionMenuGrid.menuGridPtr);
}

void clickHostLanSession()
{
	playerPingMap.clear();
	lobbyPlayerDataMap.clear();
	hasClientPlayerDisconnected.clear();
	lobbyPlayerDataMap[0] = lobbyPlayerData();
	struct addrinfo hints, * servinfo, * p = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	char optval = '1';

	getaddrinfo(NULL, GAME_PORT, &hints, &servinfo);

	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		connectClientSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		setsockopt(connectClientSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
		bind(connectClientSocket, p->ai_addr, static_cast<int>(p->ai_addrlen));
		listen(connectClientSocket, SOMAXCONN);
		u_long mode = 1;
		ioctlsocket(connectClientSocket, FIONBIO, &mode);
		break;
	}

	freeaddrinfo(servinfo);

	// TODO: Let the host decide their own name eventually
	lobbyPlayerDataMap[0].playerName = "0";
	isHost = true;
	holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, lobbyMenuGrid.menuGridPtr);
}

void clickJoinLanSession()
{
	playerPingMap.clear();
	lobbyPlayerDataMap.clear();
	isHost = false;
	holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, lobbyMenuGrid.menuGridPtr);
}

void clickLobbyChooseCharacter()
{
	// Choose character
	holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, selectingCharacterMenuGrid.menuGridPtr);
	// TODO: Might want to fix this needing to be after the swap to make sure it actually enables the buttons
	holoCureMenuInterfacePtr->EnableActionButtons(MODNAME);
	lobbyPlayerDataMap[clientID].isReady = 0;
	g_ModuleInterface->CallBuiltin("instance_create_depth", { 0, 0, 0, objCharSelectIndex });
}

void clickLobbyReady()
{
	// Ready button
	if (!lobbyPlayerDataMap[clientID].charName.empty())
	{
		lobbyPlayerDataMap[clientID].isReady = 1 - lobbyPlayerDataMap[clientID].isReady;
	}
}

void clickLobbySelectMap()
{
	// Choose map button
	holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, selectingMapMenuGrid.menuGridPtr);
	holoCureMenuInterfacePtr->EnableActionButtons(MODNAME);
	RValue charSelectInstance = g_ModuleInterface->CallBuiltin("instance_create_depth", { 0, 0, 0, objCharSelectIndex });
	RValue characterDataMap = g_ModuleInterface->CallBuiltin("variable_global_get", { "characterData" });
	RValue charData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { characterDataMap, lobbyPlayerDataMap[clientID].charName.c_str() });
	g_ModuleInterface->CallBuiltin("variable_global_set", { "charSelected", charData });
	// TODO: Need to also set the outfit once I add that
	RValue availableOutfitsArr = g_ModuleInterface->CallBuiltin("array_create", { 1, "default" });
	setInstanceVariable(charSelectInstance, GML_availableOutfits, availableOutfitsArr);
}

void clickLobbyStart()
{
	// Start game button
	numClientsInGame = 0;
	std::shared_ptr<menuGridData> nullptrMenuGridData = nullptr;
	holoCureMenuInterfacePtr->SwapToMenuGrid(MODNAME, nullptrMenuGridData);
	closesocket(connectClientSocket);
	connectClientSocket = INVALID_SOCKET;
	closesocket(broadcastSocket);
	broadcastSocket = INVALID_SOCKET;
	g_ModuleInterface->CallBuiltin("room_goto", { curSelectedMap });
}

void clickLobbySteamPlayer()
{
	if (!isHost)
	{
		return;
	}
	std::shared_ptr<menuData> selectedMenuData;
	holoCureMenuInterfacePtr->GetSelectedMenuData(MODNAME, selectedMenuData);
	int selectedSteamPlayerIndex = -1;
	for (int i = 0; i < lobbyMenuLobbyPlayerList.size(); i++)
	{
		if (lobbyMenuLobbyPlayerList[i].get() == selectedMenuData.get())
		{
			selectedSteamPlayerIndex = i;
			break;
		}
	}
	curSelectedSteamID = steamLobbyBrowser->m_lobbyMemberList[selectedSteamPlayerIndex].m_steamIDMember;
	// Don't enable the invite button if player is already in the lobby
	if (steamIDToClientIDMap.find(curSelectedSteamID.ConvertToUint64()) != steamIDToClientIDMap.end())
	{
		curSelectedSteamID = CSteamID();
		return;
	}
	
	for (int i = 0; i < lobbyMenuLobbyPlayerInviteList.size(); i++)
	{
		lobbyMenuLobbyPlayerInviteList[i]->isVisible = false;
	}
	lobbyMenuLobbyPlayerInviteList[selectedSteamPlayerIndex]->isVisible = true;
}

void clickLobbySteamPlayerInvite()
{
	if (!curSelectedSteamID.IsValid())
	{
		DbgPrintEx(LOG_SEVERITY_ERROR, "Steam ID isn't valid");
		return;
	}
	std::shared_ptr<menuData> selectedMenuData;
	holoCureMenuInterfacePtr->GetSelectedMenuData(MODNAME, selectedMenuData);
	int selectedSteamPlayerInviteIndex = -1;
	for (int i = 0; i < lobbyMenuLobbyPlayerInviteList.size(); i++)
	{
		if (lobbyMenuLobbyPlayerInviteList[i].get() == selectedMenuData.get())
		{
			selectedSteamPlayerInviteIndex = i;
			break;
		}
	}
	// TODO: Can probably have a better way to disable buttons
	lobbyMenuLobbyPlayerInviteList[selectedSteamPlayerInviteIndex]->isVisible = false;
	uint64 inviteeSteamID = curSelectedSteamID.ConvertToUint64();
	steamIDToClientIDMap[inviteeSteamID] = 0;
	SteamMatchmaking()->SendLobbyChatMsg(steamLobbyBrowser->getSteamLobbyID(), &inviteeSteamID, sizeof(inviteeSteamID));
	curSelectedSteamID = CSteamID();
	DbgPrintEx(LOG_SEVERITY_INFO, "Pressed invite button");
}

void networkAdapterDisclaimerMenuGridReturn()
{
	if (adapterAddresses != NULL)
	{
		free(adapterAddresses);
	}
	adapterAddresses = NULL;
}

void LANSessionMenuGridReturn()
{
	closesocket(broadcastSocket);
	broadcastSocket = INVALID_SOCKET;
	if (adapterAddresses != NULL)
	{
		free(adapterAddresses);
	}
	adapterAddresses = NULL;
}

void lobbyMenuGridReturn()
{
	if (isHost)
	{
		for (auto& curClientSocket : clientSocketMap)
		{
			closesocket(curClientSocket.second);
		}
		closesocket(connectClientSocket);
		connectClientSocket = INVALID_SOCKET;
	}
	else
	{
		printf("closing server socket\n");
		closesocket(serverSocket);
		serverSocket = INVALID_SOCKET;
	}
	hasConnected = false;
	if (messageHandlerThread.joinable())
	{
		messageHandlerThread.join();
	}
	steamLobbyBrowser->leaveLobby();
	cleanupPlayerGameData();
	cleanupPlayerClientData();
	if (steamHost)
	{
		delete steamHost;
		steamHost = nullptr;
		isHost = false;
	}
	hasSelectedMap = false;
}

void selectingCharacterMenuGridReturn()
{
	holoCureMenuInterfacePtr->DisableActionButtons(MODNAME);
	curSelectedSteamID = CSteamID();
	g_ModuleInterface->CallBuiltin("instance_destroy", { objCharSelectIndex });
}

void selectingMapMenuGridReturn()
{
	holoCureMenuInterfacePtr->DisableActionButtons(MODNAME);
	curSelectedSteamID = CSteamID();
	g_ModuleInterface->CallBuiltin("instance_destroy", { objCharSelectIndex });
}

void drawLobbyMenu()
{
	if (!isHost && !hasObtainedClientID)
	{
		// Player is a client and hasn't connected to a host yet
		// TODO: Kind of hacky way to display the names. Should probably fix this later
		for (int i = 0; i < steamLobbyBrowser->m_lobbyMemberList.size(); i++)
		{
			lobbyMenuLobbyPlayerList[i]->isVisible = true;
			lobbyMenuLobbyPlayerList[i]->labelName = steamLobbyBrowser->m_lobbyMemberList[i].m_memberName;
		}
		for (size_t i = steamLobbyBrowser->m_lobbyMemberList.size(); i < lobbyMenuLobbyPlayerList.size(); i++)
		{
			lobbyMenuLobbyPlayerList[i]->isVisible = false;
		}

		for (auto& curButton : lobbyMenuLobbyPlayerInviteList)
		{
			curButton->isVisible = false;
		}
		return;
	}

	RValue returnVal;
	RValue characterDataMap = g_ModuleInterface->CallBuiltin("variable_global_get", { "characterData" });
	int curPlayerIndex = 0;
	for (int i = 0; i < lobbyMenuLobbyPlayerNameList.size(); i++)
	{
		lobbyMenuLobbyPlayerNameList[i]->isVisible = false;
		lobbyMenuLobbyPlayerReadyList[i]->isVisible = false;
	}
	for (auto& curPlayerData : lobbyPlayerDataMap)
	{
		lobbyPlayerData curLobbyPlayerData = curPlayerData.second;
		lobbyMenuLobbyPlayerNameList[curPlayerIndex]->labelName = std::format("P{}", curLobbyPlayerData.playerName);
		lobbyMenuLobbyPlayerNameList[curPlayerIndex]->isVisible = true;
		double textXPos = 20;
		double textYPos = curPlayerIndex * 32 + 14;

		if (!curLobbyPlayerData.charName.empty())
		{
			// TODO: Maybe convert this to another menu element?
			RValue charData = g_ModuleInterface->CallBuiltin("ds_map_find_value", { characterDataMap, curLobbyPlayerData.charName.c_str()});
			RValue idleSprite = getInstanceVariable(charData, GML_sprite1);
			g_ModuleInterface->CallBuiltin("draw_sprite_ext", { idleSprite, -1, textXPos + 30, textYPos + 20, 1, 1, 0, 0xFFFFFF, 1 });
		}
		if (curLobbyPlayerData.isReady)
		{
			lobbyMenuLobbyPlayerReadyList[curPlayerIndex]->labelName = "Ready";
			lobbyMenuLobbyPlayerReadyList[curPlayerIndex]->isVisible = true;
		}
		curPlayerIndex++;
	}
	if (lobbyPlayerDataMap[0].stageSprite != -1)
	{
		g_ModuleInterface->CallBuiltin("draw_sprite", { lobbyPlayerDataMap[0].stageSprite, 0, 455, 10 });
	}

	lobbyMenuLobbyReady->labelName = lobbyPlayerDataMap[clientID].isReady ? "Unready" : "Ready";
	lobbyMenuLobbyChooseCharacter->isVisible = true;
	lobbyMenuLobbyReady->isVisible = true;
	lobbyMenuLobbySelectMap->isVisible = false;
	lobbyMenuLobbyStart->isVisible = false;
	if (isHost)
	{
		if (!lobbyPlayerDataMap[clientID].charName.empty())
		{
			lobbyMenuLobbySelectMap->isVisible = true;
			if (hasSelectedMap)
			{
				bool areAllPlayersReady = true;
				for (auto& curPlayerData : lobbyPlayerDataMap)
				{
					lobbyPlayerData curLobbyPlayerData = curPlayerData.second;
					if (curLobbyPlayerData.isReady == 0)
					{
						areAllPlayersReady = false;
						break;
					}
				}
				lobbyMenuLobbyStart->isVisible = areAllPlayersReady && lobbyPlayerDataMap.size() >= 2;
			}
		}
	}

	if (isHost)
	{
		for (int i = 0; i < steamLobbyBrowser->m_lobbyMemberList.size(); i++)
		{
			lobbyMenuLobbyPlayerList[i]->isVisible = true;
			lobbyMenuLobbyPlayerList[i]->labelName = steamLobbyBrowser->m_lobbyMemberList[i].m_memberName;
		}
		for (size_t i = steamLobbyBrowser->m_lobbyMemberList.size(); i < lobbyMenuLobbyPlayerList.size(); i++)
		{
			lobbyMenuLobbyPlayerList[i]->isVisible = false;
		}
	}
}