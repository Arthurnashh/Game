setTimeForScripts 90;

call compile preprocessFileLineNumbers "\dz\modulesDayZ\init.sqf";

DZ_MP_CONNECT = true;
DZ_MAX_ZOMBIES = 500;

call dbLoadPlayer;

_humidity = random 1;
//setDate getSystemTime;
setDate [2013, 12, 16, 15, 0];
0 setOvercast _humidity;
simulWeatherSync;

_position = [7500,7500,0];
//exportProxies [_position,200000];
call init_spawnZombies;
sleep 1;
importProxies;
spawnLoot [_position,30000,35000];

dbInitServer;

// Persistent world objects (vehicles / tents)
call compile preprocessFileLineNumbers "dz_worldPersist.sqf";

setTimeForScripts 0.03;
