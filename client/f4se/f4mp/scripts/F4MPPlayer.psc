Scriptname F4MPPlayer extends Actor

int Property entityID = -1 Auto

Idle Property animNull Auto
Idle Property animIdle Auto
Idle Property animJogForward Auto
Idle Property animJogBackward Auto
Idle Property animJogLeft Auto
Idle Property animJogRight Auto
Idle Property animJumpUp Auto
Idle Property animJumpFall Auto
Idle Property animJumpLand Auto
Idle Property animFireWeapon Auto

Idle Property animTest Auto

Action Property actionAnim Auto

ActorValue Property avType Auto

Keyword Property animKeyword Auto
Keyword Property animType Auto

Actor Property playerRef Auto

ActorValue Property HealthAV Auto

int tickTimerID = 10
int animTimerID = 20
int tidyTimerID = 30

string animState = "INIT"

Form[] Property itemsToWear = None Auto

Weapon Property myWeapon Auto
Ammo Property myAmmo Auto
Form Property firePoint Auto
Projectile[] Property myProjectiles Auto

float Property health = 1.0 Auto

bool Property initialized = false Auto

Weapon Property weaponEquipped = None Auto

int stage = 0

Event OnInit()
	StartTimer(0, tickTimerID)
	StartTimer(0, animTimerID)
	StartTimer(0, tidyTimerID)

	RegisterForKey(113)
	
	; self.EnableAI(false, true)
	
	;EquipItem(myWeapon)

	RegisterForHitEvent(self)
EndEvent

Event OnItemEquipped(Form akBaseObject, ObjectReference akReference)
	Weapon equippedWeapon = akBaseObject as Weapon
	If equippedWeapon
		weaponEquipped = equippedWeapon
		
		InstanceData:Owner owner = equippedWeapon.GetInstanceOwner()
		InstanceData.SetAttackDamage(owner, 0)
		InstanceData.SetDamageTypes(owner, new InstanceData:DamageTypeInfo[0])
	EndIf
EndEvent

Event OnHit(ObjectReference akTarget, ObjectReference akAggressor, Form akSource, Projectile akProjectile, bool abPowerAttack, bool abSneakAttack, bool abBashAttack, bool abHitBlocked, string apMaterial)
	If akTarget == self && akAggressor == Game.GetPlayer()
		Weapon sourceWeapon = akSource as Weapon
		If sourceWeapon
			InstanceData:Owner owner = sourceWeapon.GetInstanceOwner()
			float damage = InstanceData.GetAttackDamage(owner)
			F4MP.PlayerHit(F4MP.GetPlayerEntityID(), entityID, damage)
		EndIf
	EndIf

	RegisterForHitEvent(self)
EndEvent

;Event OnAnimationEvent(ObjectReference akSource, string asEventName)
	; Debug.Trace(akSource + " " + asEventName)
	; Debug.Notification(asEventName)
	
	;If akSource == Game.GetPlayer() && asEventName == "weaponFire"
	;	Debug.Trace(asEventName)
	;EndIf
;EndEvent

Event OnKeyDown(int keyCode)
	If keyCode == 113
		; Debug.Notification(self.PlayIdle(idleAnim))
		; self.SetValue(avType, 1)
		; self.PushActorAway(self, 0)
		
		; TranslateTo(x + 200.0, y, z, 0, 0, 0, 500.0)	
		; PlayIdleAction(F4MP.GetAction("ActionSprintStart"))
		
		; Debug.Notification(ChangeAnimArchetype(animKeyword))

		
		; Debug.Notification(ChangeAnimArchetype(animKeyword))
		; Game.GetPlayer().PlayIdleAction(F4MP.GetAction("ActionMoveBackward"))
		

		; playerRef .PlayIdleAction(F4MP.GetAction("ActionSprintStart"))
		

		; TranslateTo(x + 1000.0, y, z, 0.0, 0.0, 0.0, 100.0)

		; ChangeAnimArchetype()
		; ChangeAnimArchetype(animType)

		; PlayIdle(animJumpUp)

		; ActorBase base = GetActorBase()
		; F4MP.SetSex(base, base.GetSex() == 0)
		; Debug.Notification(base.GetSex())

		; SetOutfit(Game.GetPlayer().GetActorBase().GetOutfit())
		
		; F4MP.CopyAppearance(Game.GetPlayer().GetActorBase(), GetActorBase())
		; F4MP.CopyWornItems(Game.GetPlayer(), self)

		; StartCombat(Game.GetPlayer())
		; DrawWeapon()

		; Weapon equippedWeapon = GetEquippedWeapon()
		; If equippedWeapon != None
		; 	equippedWeapon.Fire(self, myAmmo)
		; EndIf
		
		; Game.GetPlayer().GetEquippedWeapon().Fire(Game.GetPlayer(), myAmmo)

		; Form[] args = new Form[0];
		; args.Add(animIdle)
		; args.Add(animJogForward)
		; args.Add(animJogBackward)
		; args.Add(animJogLeft)
		; args.Add(animJogRight)
		; args.Add(animJumpUp)
		; F4MP.Inspect(args)

		; F4MPFirePoint newFirePoint = PlaceAtMe(firePoint) as F4MPFirePoint
		; newFirePoint.myWeapon = myWeapon
		; newFirePoint.myAmmo = myAmmo
		; newFirePoint.SetAngle(GetAngleX(), GetAngleY(), GetAngleZ())

		;Debug.Notification(equippedWeaponRef)
		;Weapon equippedWeapon = GetEquippedWeapon()
		;Ammo equippedWeaponAmmo = equippedWeapon.GetAmmo()
		;equippedWeapon.Fire(equippedWeaponRef, equippedWeaponAmmo)
		
		;DrawWeapon()
		;StartTimer(0.5, animTimerID)

		;PlayIdle(animIdle)
		;Utility.Wait(0.5)

		;Idle animToPlay = None

		;If stage == 0
		;	animToPlay = animJogForward
		;ElseIf stage == 1
		;	animToPlay = animFireWeapon
		;ElseIf stage == 2
		;	animToPlay = animJumpUp
		;ElseIf stage == 3
		;	animToPlay = animIdle
		;EndIf
		;stage = (stage + 1) % 4

		;If animToPlay != animIdle
		;	PlayIdle(animNull)
		;	Utility.Wait(0.3)
		;EndIf

		;If !IsWeaponDrawn()
		;	If GetEquippedWeapon() != myWeapon
		;		EquipItem(myWeapon)
		;		Utility.Wait(0.1)
		;	EndIf
		;	DrawWeapon()
		;	Utility.Wait(0.4)
		;EndIf

		;PlayIdle(animToPlay)

		;TranslateTo(x + 10.0, y, z, 0, 0, 0, 0.01)
	EndIf
EndEvent

Event OnTimer(int aiTimerID)
	;int k = 0
	;While k < myProjectiles.length
	;	ObjectReference ref = Game.FindClosestReferenceOfTypeFromRef(myProjectiles[k], Game.GetPlayer(), 100000.0)
	;	If ref != None
	;		float ax = ref.GetAngleX()
	;		float ay = ref.GetAngleY()
	;		float az = ref.GetAngleZ()
	;		If ax != 0.0 && az != 0.0
	;			string msg = k + ": (" + ref.x + ", " + ref.y + ", " + ref.z + "), (" + ax + ", " + ay + ", " + az + ")"
	;			Debug.Trace(msg)
	;			Debug.Notification(msg)

	;			F4MPFirePoint newFirePoint = PlaceAtMe(firePoint) as F4MPFirePoint
	;			newFirePoint.myWeapon = myWeapon
	;			newFirePoint.myAmmo = myAmmo
	;			newFirePoint.SetAngle(ax, ay, az - GetAngleZ())
	;		EndIf
	;		k = myProjectiles.length
	;	EndIf

	;	k += 1
	;EndWhile

	; PlayIdleAction(F4MP.GetAction("ActionMoveStart"))

	; SetAnimationVariableFloat("Speed", 100.0)
	
	; Debug.Trace(Game.GetPlayer().GetAnimationVariableFloat("Speed"))
	
	; Game.GetPlayer().SetAnimationVariableFloat("Speed", 10.0)
	
	; F4MP.SetTransforms(self)

	If aiTimerID == tickTimerID
		If itemsToWear != None
			int i = 0
			While i < itemsToWear.length
				EquipItem(itemsToWear[i])
				i += 1
			EndWhile
			itemsToWear = None
		EndIf
		
		If F4MP.IsEntityValid(entityID)
			If !initialized
				initialized = true
			EndIf
			
			;float[] position = F4MP.GetEntityPosition(entityID)
			;float angleZ = F4MP.GetEntVarNum(entityID, "angleZ")
			;float distance = Math.Sqrt(Math.Pow(position[0] - x, 2) + Math.Pow(position[1] - y, 2) + Math.Pow(position[2] - z, 2))

			;health = F4MP.GetEntVarNum(entityID, "health")

			;TranslateTo(position[0], position[1], position[2], 0.0, 0.0, angleZ, distance * 3.0, 500.0)
			;self.SetPosition(position[0], position[1], position[2])
			;self.SetAngle(0.0, 0.0, angleZ)

			; Debug.Notification(entityID + " " + position[0] + " " + position[1] + " " + position[2])
		Else
			If initialized
				;Debug.Trace("entity with ID " + entityID + " is supposed to be deleted but I saved it for ya! ;)")
				Delete()
			EndIf
			; Debug.Notification(entityID + "!")
		EndIf

		StartTimer(0, tickTimerID)
	ElseIf aiTimerID == animTimerID
		string newAnimState = F4MP.GetEntVarAnim(entityID)
		If newAnimState != animState
			string oldAnimState = animState
			animState = newAnimState

			If !IsWeaponDrawn()
				DrawWeapon()
				Utility.Wait(0.4)
			EndIf

			If F4MP.AnimLoops(oldAnimState)
				PlayIdle(animNull)
				Utility.Wait(0.2)
			EndIf

			PlayAnimByState(newAnimState)
		EndIf
		StartTimer(0, animTimerID)
	Elseif aiTimerID == tidyTimerID
		StartTimer(0, tidyTimerID)

		If GetEquippedWeapon() != weaponEquipped
			EquipItem(weaponEquipped)
		EndIf

		If !IsWeaponDrawn()
			animState = "INIT"
		EndIf

		;string[] animStates = new string[0]
		;animStates.Add("None")
		;animStates.Add("JogForward")
		;animStates.Add("JogBackward")
		;animStates.Add("JogLeft")
		;animStates.Add("JogRight")
		;animStates.Add("JumpUp")
		;animStates.Add("WeaponFire")
		;
		;string newAnimState = animStates[Utility.RandomInt(0, animStates.length - 1)];nextAnimState;
		;If newAnimState != animState
		;	If GetEquippedWeapon() != weaponEquipped
		;		EquipItem(weaponEquipped)
		;	EndIf
		;
		;	If IsWeaponDrawn()
		;		;If newAnimState == "None"
		;		;	PlayAnimByState(newAnimState)
		;		;Else
		;			PlayIdle(animNull)		
		;			;StartTimer(0.3, animTimerID)
		;			Utility.Wait(0.3)
		;			PlayAnimByState(newAnimState)
		;		;EndIf
		;	Else
		;		DrawWeapon()
		;
		;		PlayIdle(animNull)
		;		;StartTimer(0.4, animTimerID)
		;		Utility.Wait(0.4)
		;		PlayAnimByState(newAnimState)
		;	EndIf
		;	animState = newAnimState
		;EndIf

		SetValuePercentage(HealthAV, health)
		
		If IsInCombat()
			StopCombat()
		EndIf
	EndIf
EndEvent

Function FireWeapon()
	Weapon equippedWeapon = GetEquippedWeapon()
	If equippedWeapon
		F4MPFirePoint newFirePoint = PlaceAtMe(firePoint) as F4MPFirePoint
		newFirePoint.myWeapon = equippedWeapon
		newFirePoint.myAmmo = equippedWeapon.GetAmmo()
		newFirePoint.myOwner = self
	EndIf
EndFunction

Function PlayAnimByState(string newAnimState)
	Idle animToPlay = GetAnimByState(newAnimState)
	If animToPlay
		PlayIdle(animToPlay)
	EndIf

	;If newAnimState == "FireWeapon"
	;	Weapon equippedWeapon = GetEquippedWeapon()
	;	If equippedWeapon
	;		F4MPFirePoint newFirePoint = PlaceAtMe(firePoint) as F4MPFirePoint
	;		newFirePoint.myWeapon = equippedWeapon
	;		newFirePoint.myAmmo = equippedWeapon.GetAmmo()
	;		newFirePoint.myOwner = self
	;	EndIf
	;	animState = "INIT"
	;EndIf
EndFunction

Idle Function GetAnimByState(string newAnimState)
	Idle animToPlay = animIdle

	If newAnimState == "None"
		animToPlay = animIdle
	ElseIf newAnimState == "JogForward" 
		animToPlay = animJogForward
	ElseIf newAnimState == "JogBackward" 
		animToPlay = animJogBackward
	ElseIf newAnimState == "JogLeft" 
		animToPlay = animJogLeft
	ElseIf newAnimState == "JogRight" 
		animToPlay = animJogRight
	ElseIf newAnimState == "JumpUp" 
		; animToPlay = animJumpUp
	ElseIf newAnimState == "JumpFall" 
		; animToPlay = animJumpFall
	ElseIf newAnimState == "JumpLand" 
		; animToPlay = animJumpLand
	ElseIf newAnimState == "FireWeapon"
		animToPlay = animFireWeapon
	EndIf

	return animToPlay
EndFunction

Function SetValueActually(ActorValue type, float value)
	float diff = value - GetValue(type)
	If diff < 0
		DamageValue(type, -diff)
	Else
		RestoreValue(type, diff)
	EndIf
EndFunction

Function SetValuePercentage(ActorValue type, float percentage)
	SetValueActually(type, percentage * GetBaseValue(type))
EndFunction