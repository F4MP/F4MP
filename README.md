# F4MP
[![Discord](https://img.shields.io/discord/729620961346977862.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/pKDHVvf)

Fallout 4 Multiplayer Mod

Please note that this project is in no way playable at this time. You will encounter game breaking bugs and crashes. 

**Ensure you backup your game before using this mod.**

## Note
*Currently, no other mods are supported and will most likely completely break your game if you use them at the same time as this one.*

### Requirements for install
* Fallout 4 - It is recommended to use a fresh install of the game
* [F4SE](https://f4se.silverlock.org/) - Required to hook the game, make sure to run the game with `f4se_loader.exe`

#### Installing the server
Download the newest release from the Release page
```
https://github.com/F4MP/F4MP/releases/tag/release
```
Extract the server

Create a new file called `server_config.txt` and enter the bind address (Most likely `localhost`)

Open a Console and navigate to the server binary, then enter
```
f4mp_server.exe
```
Success! Anyone on your local network can now connect!

If you want to port forward, open port `7779` and allow both TCP & UDP.
People connecting to your server will need to enter your public IP in their config.txt (`My Games\Fallout4\F4MP\config.txt`)
