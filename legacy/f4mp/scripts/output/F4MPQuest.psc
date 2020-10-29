Scriptname F4MPQuest extends Quest

int tickTimerID = 10
int queryTimerID = 20

ActorBase Property f4mpPlayerBase Auto

ActorValue Property healthAV Auto

int[] playerIDs
F4MPPlayer[] players

Actor[] myNPCs
Actor[] othersNPCs

Event OnInit()
	RegisterForKey(112)
EndEvent

Function OnEntityCreate(int entityID, Form[] itemsToWear)
	Debug.Notification(entityID + " has entered the world.")

	If entityID != F4MP.GetPlayerEntityID()
		Actor player = Game.GetPlayer()
		F4MPPlayer entity = player.PlaceActorAtMe(f4mpPlayerBase) as F4MPPlayer
		entity.entityID = entityID
		
		entity.itemsToWear = itemsToWear
		; SetWornItems(entity, itemsToWear)
	
		playerIDs.Add(entityID)
		players.Add(entity)
	EndIf
EndFunction

Function OnEntityUpdate(int entityID)
	; int index = playerIDs.Find(entityID)
	; If index < 0
	;  	return
	; EndIf
	
	; F4MPPlayer player = players[index];
	; If F4MP.IsEntityValid(player.entityID)
	; 	float[] position = F4MP.GetEntityPosition(player.entityID)
	; 	float angleX = 0.0; F4MP.ReadNumber()
	; 	float angleY = 0.0; F4MP.ReadNumber()
	; 	float angleZ = 0.0; F4MP.ReadNumber()
	; 	Debug.Trace(position[0] + " " + position[1] + " " +  position[2] + " " +  angleX + " " + angleY + " " + angleZ)
	; 	player.TranslateTo(position[0], position[1], position[2], angleX, angleY, angleZ, 200.0, 100.0)
	; EndIf
EndFunction

Function OnEntityRemove(int entityID)
	int index = playerIDs.Find(entityID)
	If index < 0
		return
	EndIf
	
	players[index].Delete()
	playerIDs.Remove(index)
	players.Remove(index)
EndFunction

Function OnClientUpdate(int entityID)
	; Actor player = Game.GetPlayer()

	; int playerEntityID = F4MP.GetPlayerEntityID()
	; If F4MP.IsEntityValid(playerEntityID)
	;	F4MP.SetEntityPosition(playerEntityID, player.GetPositionX(), player.GetPositionY(), player.GetPositionZ())

	;	F4MP.WriteNumber(player.GetAngleX())
	;	F4MP.WriteNumber(player.GetAngleY())
	;	F4MP.WriteNumber(player.GetAngleZ())
	; EndIf
EndFunction

Function OnPlayerHit(float damage)
	Game.GetPlayer().DamageValue(healthAV, damage)
EndFunction

Function OnFireWeapon(int entityID)
	int index = playerIDs.Find(entityID)
	If index < 0
		return
	EndIf
	
	players[index].FireWeapon()
EndFunction

Function OnSpawnEntity(int formID)
	ObjectReference ref = Game.GetForm(formID) as ObjectReference
	If ref == None
		return
	EndIf

	Actor actorRef = ref as Actor
	If actorRef != None && myNPCs.Find(actorRef) < 0
		othersNPCs.Add(actorRef)
	EndIf
EndFunction

Function SetWornItems(Actor dest, Form[] wornItems)
	int i = 0
	While i < wornItems.length
		Debug.Trace(i + ": " + wornItems[i])
		dest.EquipItem(wornItems[i])
		i += 1
	EndWhile
EndFunction

; TODO: mutiple timers
bool Function Connect(string address, int port)
	Actor client = Game.GetPlayer()
	ActorBase clientActorBase = client.GetActorBase()
	
	StartTimer(0, tickTimerID)
	return F4MP.Connect(client, clientActorBase, address, port)
EndFunction

Event OnKeyDown(int keyCode)
	If keyCode == 112
		Connect("", 7779)

		playerIDs = new int[0]
		players = new F4MPPlayer[0]		

		othersNPCs = new Actor[0]
		myNPCs = new Actor[0]
		StartTimer(0, queryTimerID)

		;Actor player = Game.GetPlayer()
		;F4MPPlayer entity = player.PlaceActorAtMe(f4mpPlayerBase) as F4MPPlayer
		
		Actor client = Game.GetPlayer()
		RegisterForAnimationEvent(client, "JumpUp")
		RegisterForAnimationEvent(client, "weaponFire")
		; RegisterForAnimationEvent(client, "JumpFall")
		; RegisterForAnimationEvent(client, "JumpDown")

		; RegisterForExternalEvent("OnCopyWornItems", "OnCopyWornItems")

		RegisterForExternalEvent("OnEntityCreate", "OnEntityCreate")
		RegisterForExternalEvent("OnEntityUpdate", "OnEntityUpdate")
		RegisterForExternalEvent("OnEntityRemove", "OnEntityRemove")
		RegisterForExternalEvent("OnClientUpdate", "OnClientUpdate")

		RegisterForExternalEvent("OnPlayerHit", "OnPlayerHit")
		RegisterForExternalEvent("OnFireWeapon", "OnFireWeapon")
		RegisterForExternalEvent("OnSpawnEntity", "OnSpawnEntity")

		RegisterForKey(113)
		RegisterForKey(114)
	ElseIf keyCode == 113
		F4MP.SetClient(1 - F4MP.GetClientInstanceID())
	ElseIf keyCode == 114
	;	Actor randomActor = Game.FindRandomActorFromRef(Game.GetPlayer(), 1000.0)
	;	If randomActor != None && randomActor != Game.GetPlayer()
	;		chosenActor = randomActor
	;		targetRef = Game.GetPlayer().PlaceAtMe(targetForm)

	;		Debug.Notification(chosenActor.GetDisplayName() + " " + targetRef.GetDisplayName())
	;	EndIf
	EndIf
EndEvent

Form Property targetForm Auto
Actor Property chosenActor Auto
ObjectReference Property targetRef Auto

Event OnAnimationEvent(ObjectReference akSource, string asEventName)
	If !F4MP.IsConnected()
		return
	EndIf

	int playerEntityID = F4MP.GetPlayerEntityID()
	If F4MP.IsEntityValid(playerEntityID)
		If asEventName == "JumpUp"
			F4MP.SetEntVarAnim(playerEntityID, "JumpUp")
		; ElseIf asEventName == "JumpFall"
		; 	F4MP.SetEntVarAnim(playerEntityID, "JumpFall")
		; ElseIf asEventName == "JumpDown"
		; 	F4MP.SetEntVarAnim(playerEntityID, "JumpLand")
		ElseIf asEventName == "weaponFire"
			; F4MP.SetEntVarAnim(playerEntityID, "FireWeapon")
			F4MP.PlayerFireWeapon()
		EndIf
	EndIf
EndEvent

Event OnTimer(int aiTimerID)
	If !F4MP.IsConnected()
		F4MP.Tick()
		StartTimer(0, aiTimerID)
		return
	EndIf

	If aiTimerID == tickTimerID
		StartTimer(0, tickTimerID)
		
		;; ***************************************
		;If chosenActor != None
		;	chosenActor.PathToReference(targetRef, 1.0)
		;EndIf
		;
		;; ***************************************

		Actor player = Game.GetPlayer()

		int playerEntityID = F4MP.GetPlayerEntityID()
		If F4MP.IsEntityValid(playerEntityID)
			F4MP.SetEntityPosition(playerEntityID, player.GetPositionX(), player.GetPositionY(), player.GetPositionZ())
			F4MP.SetEntVarNum(playerEntityID, "angleX", player.GetAngleX())
			F4MP.SetEntVarNum(playerEntityID, "angleY", player.GetAngleY())
			F4MP.SetEntVarNum(playerEntityID, "angleZ", player.GetAngleZ())
			F4MP.SetEntVarNum(playerEntityID, "health", player.GetValuePercentage(healthAV))
		EndIf

		;int i = 0
		;While i < myNPCs.length
		;	Actor myNPC = myNPCs[i]
		;	F4MP.SetEntityPosition(F4MP.GetEntityID(myNPC), myNPC.x, myNPC.y, myNPC.z)
		;	i += 1
		;EndWhile

		F4MP.Tick()

		;i = 0
		;While i < othersNPCs.length
		;	Actor othersNPC = othersNPCs[i]
		;	float[] position = F4MP.GetEntityPosition(F4MP.GetEntityID(othersNPC))
		;	float distance = Math.Sqrt(Math.Pow(position[0] - othersNPC.x, 2) + Math.Pow(position[1] - othersNPC.y, 2) + Math.Pow(position[2] - othersNPC.z, 2))
		;	othersNPC.TranslateTo(position[0], position[1], position[2], 0.0, 0.0, othersNPC.GetAngleZ(), distance * 3.0, 200.0)
		;	i += 1
		;EndWhile

		; Debug.Notification(F4MP.GetPlayerEntityID() + " " + player.GetPositionX() + " " + player.GetPositionY() + " " + player.GetPositionZ())
	ElseIf aiTimerID == queryTimerID
		StartTimer(0, queryTimerID)

		;Actor randomActor = Game.FindRandomActorFromRef(Game.GetPlayer(), 10000.0)
		;If randomActor != None && randomActor as F4MPPlayer == None && randomActor != Game.GetPlayer()
		;	If myNPCs.Find(randomActor) < 0 && othersNPCs.Find(randomActor) < 0
		;		myNPCs.Add(randomActor)
		;		F4MP.SpawnEntity(randomActor, randomActor.x, randomActor.y, randomActor.z, randomActor.GetAngleX(), randomActor.GetAngleY(), randomActor.GetAngleZ())
		;	EndIf
		;EndIf
	EndIf
EndEvent
