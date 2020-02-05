Scriptname F4MP Native Hidden

bool Function Connect(string address, int port) global native
bool Function Disconnect() global native
Function Tick() global native

int Function GetPlayerEntityID() global native

bool Function IsEntityValid(int entityID) global native
float[] Function GetEntityPosition(int entityID) global native
Function SetEntityPosition(int entityID, float x, float y, float z) global native

Function WriteNumber(float number) global native
float Function ReadNumber() global native

Function SetEntVarNum(int entityID, string name, float value) global native
Function SetEntVarAnim(int entityID, string animState) global native
float Function GetEntVarNum(int entityID, string name) global native
string Function GetEntVarAnim(int entityID) global native

Function GetTransforms(Actor actor) global native
Function SetTransforms(Actor actor) global native

Action Function GetAction(string name) global native

float Function Atan2(float y, float x) global native
string Function GetWalkDir(float dX, float dY, float angleZ) global native