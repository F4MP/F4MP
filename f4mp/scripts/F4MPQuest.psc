Scriptname F4MPQuest extends Quest

int tickTimerID = 10
int updateTimerID = 20
int queryTimerID = 30
int syncTimerID = 40

Actor Property playerRef Auto

ActorBase Property f4mpPlayerBase Auto

ActorValue Property healthAV Auto

Spell Property entitySyncSpell Auto

int[] playerIDs
F4MPPlayer[] players

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

Function OnEntityRemove(int entityID)
	int index = playerIDs.Find(entityID)
	If index < 0
		return
	EndIf
	
	players[index].Delete()
	playerIDs.Remove(index)
	players.Remove(index)
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
	If actorRef != None
		actorRef.AddSpell(entitySyncSpell)
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
	StartTimer(0, updateTimerID)
	StartTimer(0, syncTimerID)
	return F4MP.Connect(client, clientActorBase, address, port)
EndFunction

Event OnKeyDown(int keyCode)
	If keyCode == 112
		Connect("", 7779)

		playerIDs = new int[0]
		players = new F4MPPlayer[0]		

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
		RegisterForExternalEvent("OnEntityRemove", "OnEntityRemove")

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

		ObjectReference[] refs = F4MP.GetRefsInCell(playerRef.GetParentCell())
		Debug.Notification(refs.length)
		int i = 0
		While i < refs.length
			Debug.Trace(refs[i] + " " + refs[i].GetDisplayName())
			i += 1
		EndWhile
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
		F4MP.Tick(playerRef)
		StartTimer(0, aiTimerID)
		return
	EndIf

	If aiTimerID == tickTimerID
		F4MP.Tick(playerRef)
		StartTimer(0, tickTimerID)
	ElseIf aiTimerID == updateTimerID
		;; ***************************************
		;If chosenActor != None
		;	chosenActor.PathToReference(targetRef, 1.0)
		;EndIf
		;
		;; ***************************************

		int playerEntityID = F4MP.GetPlayerEntityID()
		If F4MP.IsEntityValid(playerEntityID)
			F4MP.SetEntVarNum(playerEntityID, "health", playerRef.GetValuePercentage(healthAV))
		EndIf
		StartTimer(0, updateTimerID)
	ElseIf aiTimerID == queryTimerID
		F4MP.SyncWorld()
		StartTimer(0, queryTimerID)
	ElseIf aiTimerID == syncTimerID
		int[] formIDs = F4MP.GetEntitySyncFormIDs(false)
		float[] transforms = F4MP.GetEntitySyncTransforms(true)
	
		int i = 0
		While i < formIDs.length
			ObjectReference ref = Game.GetForm(formIDs[i]) as ObjectReference
			If ref != None
				int offset = i * 6
				float x = transforms[offset]
				float y = transforms[offset + 1]
				float z = transforms[offset + 2]
				float distance = Math.Sqrt(Math.Pow(x - ref.x, 2) + Math.Pow(y - ref.y, 2) + Math.Pow(z - ref.z, 2))
				ref.TranslateTo(x, y, z, transforms[offset + 3], transforms[offset + 4], transforms[offset + 5], distance * 3.0, 500.0)	
			EndIf
			
			i += 1
		EndWhile

		StartTimer(0, syncTimerID)
	EndIf
EndEvent
