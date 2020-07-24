# F4MP
[![Discord](https://img.shields.io/discord/729620961346977862.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/pKDHVvf)

 Fallout 4 Multiplayer Mod

Please note that this project is in no way playable at this time. You will encounter game breaking bugs and crashes. 

**Ensure you backup your game before using this mod.**

## Installing 
*Currently, no other mods are supported and will most likely completely break your game if you use them at the same time as this one.*

### Requirements for install
* Fallout 4 - It is recommended to use a fresh install of the game
* [F4SE](https://f4se.silverlock.org/) - Required to hook the game, make sure to run the game with `f4se_loader.exe`

#### Installing the client
1. Download and extract the latest release from [HERE](https://github.com/cokwa/F4MP/releases)
2. Inside there is a file called `f4mp-client.zip`. This is the mod.
3. Drag and drop the zip file into your mod manager or optionally extract its content into your game's `Data` folder.
4. Install F4SE and run the game using `f4se_loader.exe`

#### Installing the server
1. Download and extract the latest release from [HERE](https://github.com/cokwa/F4MP/releases)
2. Inside there is a folder called `f4mp-server`. These are the server files.
3. To run your server, simple just run the batch file or double click the exe.

You should see a window like this:
[console window](https://i.imgur.com/eQVMnjv.png)

*If you want to allow people from the internet to connect, open port `7779` and allow both TCP & UDP through your router settings.
People connecting to your server will need to enter your public IP in their config.txt (`My Games\Fallout4\F4MP\config.txt`)*

## Contributing
This project is open source and we are grateful for any contributions made.

### Setting up an environment for the server
1. Ensure you have Python version **3.7** or higher installed and on the path
2. Fork the repository and clone **YOUR** version using `git clone https://github.com/<your username>/F4MP/`
3. Enter into the server files of the repo: `cd F4MP/server`
4. Optionally create a virtual environment.
5. Switch your branch to `develop`
6. Once you have finished contributing, commit and push your code, then make a pull request.
