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
	float angleZ = GetAngleZ()
	SetPosition(position[0] + Math.Sin(angleZ) * 100.0 + Math.Cos(angleZ) * 10.0, position[1] + Math.Cos(angleZ) * 100.0 - Math.Sin(angleZ) * 10.0, position[2] + 90.0)

	float ax = F4MP.GetEntVarNum(myOwner.entityID, "angleX")
	float ay = F4MP.GetEntVarNum(myOwner.entityID, "angleY")
	float az = F4MP.GetEntVarNum(myOwner.entityID, "angleZ")
	SetAngle(ax, ay, az)

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
