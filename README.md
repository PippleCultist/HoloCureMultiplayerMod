# Holocure Multiplayer Mod
A Holocure mod that adds multiplayer to the game. Will probably be buggy and have random crashes since a lot has been modified in the game to get it working.
## Installation Steps
# Follow these instructions if you've used mods before December 2025
- Run `AurieManager.exe` and uninstall Aurie from `HoloCure.exe`
    - The latest version of Aurie is moving away from AurieManager and is instead patching the game to run the mods. This has the benefit of not requiring admin privileges anymore and easily disabling mods by deleting the mods folder or replacing the original exe without crashing.
# Normal installation steps
- Download `HoloCureMultiplayerMod.dll`, `HoloCureMenuMod.dll`, `CallbackManagerMod.dll`, `AurieCore.dll`, and `YYToolkit.dll` from the latest version of the mod https://github.com/PippleCultist/HoloCureMultiplayerMod/releases
- Download `AurieInstaller.exe` from the latest version of Aurie https://github.com/AurieFramework/Aurie/releases
- Launch `AurieInstaller.exe`, click `Find my game!`, and select `HoloCure.exe`
    - You can find `HoloCure.exe` through Steam by clicking `Browse local files`
- Click `Confirm Version`
- Go to the `mods` folder where `HoloCure.exe` is located and locate the `Aurie` folder and `Native` folder.
    - In the `Aurie` folder, replace `YYToolkit.dll` and copy over `HoloCureMultiplayerMod.dll`, `CallbackManagerMod.dll`, and `HoloCureMenuMod.dll`
    - In the `Native` folder, replace `AurieCore.dll`
- Running the game either using the executable or through Steam should now launch the mods as well
## Common Issues
- If you deleted the mods folder and are seeing `Missing game executable`, you most likely didn't click `Uninstall Aurie` in AurieManager before deleting the folder. To fix this issue, uninstall Aurie through AurieManager.
- If you get issues saying `Missing game executable` when trying to run the game, it's likely that Windows Defender or your antivirus is blocking `AurieLoader.exe` from installing. Try putting it in the whitelist or manually downloading it from `https://github.com/AurieFramework/Aurie/releases` and putting it in the mods folder.
- If you get an issue saying `Creating instance for non-existing object: -1`, it's likely that's due to clicking to skip the intro screen before the mod has fully loaded. I'll work on fixing it, but it should work if you wait for a few seconds before clicking to skip the intro screen.
## Playing LAN Multiplayer
- Press `Play` on the title screen
- Press `Multiplayer`
- If you want to use the previous network adapter you've used to play, click `Use saved network adapter`. Otherwise click `Select network adapter` and choose the name of your wireless adapter which is usually `Wi-Fi`.
    - Note: If you want to connect via VPN as a LAN game, then choose the name of the VPN network adapter
- Choose `Host LAN Session` if you're going to host. Everyone else should choose `Join LAN Session`.
## Playing Steam Multiplayer
### Instructions for Host:
- Press `Play` on the title screen
- Press `Multiplayer`
- Press `Create friend Steam lobby`
- When the client has joined your lobby, their Steam name will show up in a list. Click the client you want to invite and click `Invite user` to have them join your game. Anyone in the lobby that hasn't been invited to your game won't be able to play.
### Instructions for Client:
- When the host has clicked `Create friend Steam lobby`, you will be able to see them playing `HoloCure - Save the Fans` in your Steam friends list. While in the HoloCure title screen, right click on their name in the Steam friends list and click `Join Game` to join their lobby.
    - After joining the lobby, you will need to wait for the host to then invite you into their game before playing.
