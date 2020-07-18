Scriptname F4MPEntitySync extends activemagiceffect

;int updateTimer = 10
;
;int entityID = -1
;bool mine = false
;
;Event OnEffectStart(Actor akTarget, Actor akCaster)
;    entityID = F4MP.GetEntityID(akTarget)
;    mine = F4MP.IsEntityMine(entityID)
;
;    StartTimer(0, updateTimer)
;EndEvent
;
;Event OnTimer(int timerID)
;    If timerID == updateTimer
;        Actor entity = GetCasterActor()
;        If F4MP.IsEntityValid(entityID)
;            If mine
;                F4MP.SetEntityPosition(entityID, entity.x, entity.y, entity.z)
;            Else
;                float[] position = F4MP.GetEntityPosition(entityID)
;                float angleZ = F4MP.GetEntVarNum(entityID, "angleZ")
;                float distance = Math.Sqrt(Math.Pow(position[0] - entity.x, 2) + Math.Pow(position[1] - entity.y, 2) + Math.Pow(position[2] - entity.z, 2))
;                entity.TranslateTo(position[0], position[1], position[2], 0.0, 0.0, angleZ, distance * 3.0, 500.0)
;            EndIf
;        EndIf
;        StartTimer(0, updateTimer)
;    EndIf
;EndEvent