Scriptname F4MPFirePoint extends ObjectReference

Weapon Property myWeapon = None Auto
Ammo Property myAmmo = None Auto

F4MPPlayer Property myOwner = None Auto

Event OnInit()
	; SetPosition(x + 100.0, y + 100.0, z + 100.0)
	SetScale(0.0)
EndEvent

Event OnLoad()
	SetMotionType(Motion_Keyframed)
	
	float[] position = F4MP.GetEntityPosition(myOwner.entityID)
	float ax = F4MP.GetEntVarNum(myOwner.entityID, "angleX")
	float az = F4MP.GetEntVarNum(myOwner.entityID, "angleZ")
	SetPosition(position[0] + Math.Sin(az) * 100.0 + Math.Cos(az) * 10.0, position[1] + Math.Cos(az) * 100.0 - Math.Sin(az) * 10.0, position[2] + 90.0)
	SetAngle(ax, 0.0, az)

	; Actor player = Game.GetPlayer()
	; float dx = player.x - x
	; float dy = player.y - y
	; SetAngle(0.0, 0.0, Math.RadiansToDegrees(F4MP.Atan2(dx, dy)))

	InstanceData:Owner owner = myWeapon.GetInstanceOwner()
	InstanceData.SetAttackDamage(owner, 0)
	InstanceData.SetDamageTypes(owner, new InstanceData:DamageTypeInfo[0])

	myWeapon.Fire(self, myAmmo)
	Delete()
EndEvent
