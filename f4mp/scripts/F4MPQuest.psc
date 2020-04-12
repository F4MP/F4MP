Scriptname F4MPQuest extends Quest

int tickTimerID = 10

ActorBase Property f4mpPlayerBase Auto

ActorValue Property healthAV Auto

; int[] playerIDs
; F4MPPlayer[] players

Event OnInit()
	; playerIDs = new int[0]
	; players = new F4MPPlayer[0]	

	RegisterForKey(112)
EndEvent

Function OnEntityCreate(int entityID, Form[] itemsToWear)
	;Debug.Notification(entityID + " has entered the world.")

	If entityID != F4MP.GetPlayerEntityID()
		Actor player = Game.GetPlayer()
		F4MPPlayer entity = player.PlaceActorAtMe(f4mpPlayerBase) as F4MPPlayer
		entity.entityID = entityID
		
		entity.itemsToWear = itemsToWear
		; SetWornItems(entity, itemsToWear)
	
		; playerIDs.Add(entityID)
		; players.Add(entity)
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
	; int index = playerIDs.Find(entityID)
	; If index < 0
	; 	return
	; EndIf
	
	; players[index].KillEssential()
	; playerIDs.Remove(index)
	; players.Remove(index)
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
		Connect("222.105.107.219", 7779)
		; Connect("localhost", 7779)

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

		RegisterForKey(113)
	ElseIf keyCode == 113
		F4MP.SetClient(1 - F4MP.GetClientInstanceID())
	EndIf
EndEvent

Event OnAnimationEvent(ObjectReference akSource, string asEventName)
	int playerEntityID = F4MP.GetPlayerEntityID()
	If F4MP.IsEntityValid(playerEntityID)
		If asEventName == "JumpUp"
			F4MP.SetEntVarAnim(playerEntityID, "JumpUp")
		; ElseIf asEventName == "JumpFall"
		; 	F4MP.SetEntVarAnim(playerEntityID, "JumpFall")
		; ElseIf asEventName == "JumpDown"
		; 	F4MP.SetEntVarAnim(playerEntityID, "JumpLand")
		ElseIf asEventName == "weaponFire"
			F4MP.SetEntVarAnim(playerEntityID, "FireWeapon")
			F4MP.PlayerFireWeapon()
		EndIf
	EndIf
EndEvent

Event OnTimer(int aiTimerID)
	If aiTimerID == tickTimerID
		StartTimer(0, tickTimerID)
		
		Actor player = Game.GetPlayer()

		int playerEntityID = F4MP.GetPlayerEntityID()
		If F4MP.IsEntityValid(playerEntityID)
			F4MP.SetEntityPosition(playerEntityID, player.GetPositionX(), player.GetPositionY(), player.GetPositionZ())
			F4MP.SetEntVarNum(playerEntityID, "angleX", player.GetAngleX())
			F4MP.SetEntVarNum(playerEntityID, "angleY", player.GetAngleY())
			F4MP.SetEntVarNum(playerEntityID, "angleZ", player.GetAngleZ())
			F4MP.SetEntVarNum(playerEntityID, "health", player.GetValuePercentage(healthAV))
		EndIf

		F4MP.Tick()

		; Debug.Notification(F4MP.GetPlayerEntityID() + " " + player.GetPositionX() + " " + player.GetPositionY() + " " + player.GetPositionZ())
	EndIf
EndEvent

