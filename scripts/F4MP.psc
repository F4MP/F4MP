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

Function SetEntityNumber(int entityID, string name, float value) global native
float Function GetEntityNumber(int entityID, string name) global native