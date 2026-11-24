/*
  dz_worldPersist.sqf
  Persistencia de veiculos e tendas (somente servidor).

  Comandos de engine:
    dbWorldAdd <objeto>        -> id persistente (0 = falha)
    dbWorldSave [id, objeto]   -> bool
    dbWorldRemove <id>         -> bool
    dbWorldList                -> array de ids
    dbWorldSpawn <id>          -> objeto criado/restaurado
*/

if (!isServer) exitWith {};

DZ_WORLD_OBJECTS = [];
DZ_WORLD_SAVE_INTERVAL = 10;

// Criados apenas quando o banco esta vazio (primeira execucao).
// [classe, posicao, direcao]
DZ_WORLD_INITIAL_VEHICLES = [
  ["Heli_Light01_H",   [12020, 12700, 0], 0],
  ["Heli_Medium01_H",  [4850, 2450, 0],   90]
];

DZ_WORLD_INITIAL_TENTS = [
  ["Land_tent_east",   [12080, 12640, 0], 45],
  ["Land_tent_east",   [4880, 2420, 0],   180]
];

dz_fnc_worldFind = {
  private ["_obj", "_idx"];
  _obj = _this;
  _idx = -1;
  {
    if ((_x select 0) == _obj) exitWith { _idx = _forEachIndex; };
  } forEach DZ_WORLD_OBJECTS;
  _idx
};

dz_fnc_worldRegister = {
  private ["_obj", "_id", "_idx"];
  _obj = _this;
  if (isNull _obj) exitWith { 0 };
  _idx = _obj call dz_fnc_worldFind;
  if (_idx >= 0) exitWith { (DZ_WORLD_OBJECTS select _idx) select 1 };
  _id = dbWorldAdd _obj;
  if (_id > 0) then {
    DZ_WORLD_OBJECTS set [count DZ_WORLD_OBJECTS, [_obj, _id]];
    diag_log format ["[persist] registrado %1 id %2", typeOf _obj, _id];
  } else {
    diag_log format ["[persist] FALHA ao registrar %1", typeOf _obj];
  };
  _id
};

dz_fnc_worldUnregister = {
  private ["_obj", "_idx", "_id", "_keep"];
  _obj = _this;
  _idx = _obj call dz_fnc_worldFind;
  if (_idx < 0) exitWith {};
  _id = (DZ_WORLD_OBJECTS select _idx) select 1;
  _keep = [];
  {
    if ((_x select 0) != _obj) then { _keep set [count _keep, _x]; };
  } forEach DZ_WORLD_OBJECTS;
  DZ_WORLD_OBJECTS = _keep;
  dbWorldRemove _id;
  diag_log format ["[persist] removido id %1", _id];
};

dz_fnc_worldSaveAll = {
  private ["_keep", "_obj", "_id"];
  _keep = [];
  {
    _obj = _x select 0;
    _id = _x select 1;
    if (isNull _obj) then {
      dbWorldRemove _id;
    } else {
      if ((damage _obj) >= 0.95) then {
        dbWorldRemove _id;
      } else {
        dbWorldSave [_id, _obj];
        _keep set [count _keep, _x];
      };
    };
  } forEach DZ_WORLD_OBJECTS;
  DZ_WORLD_OBJECTS = _keep;
};

dz_fnc_worldRestore = {
  private ["_ids", "_obj"];
  _ids = dbWorldList;
  diag_log format ["[persist] restaurando %1 objeto(s)", count _ids];
  {
    _obj = dbWorldSpawn _x;
    if (!isNull _obj) then {
      DZ_WORLD_OBJECTS set [count DZ_WORLD_OBJECTS, [_obj, _x]];
      diag_log format ["[persist] restaurado %1 id %2", typeOf _obj, _x];
    } else {
      diag_log format ["[persist] FALHA ao restaurar id %1", _x];
    };
  } forEach _ids;
};

dz_fnc_worldSpawnInitial = {
  private ["_def", "_veh"];
  {
    _def = _x;
    _veh = createVehicle [(_def select 0), (_def select 1), [], 0, "NONE"];
    if (!isNull _veh) then {
      _veh setDir (_def select 2);
      _veh setVariable ["allowHeliEngine", true];
      _veh setVariable ["allowAutoHower", true];
      _veh call dz_fnc_worldRegister;
    } else {
      diag_log format ["[persist] FALHA ao criar %1", _def select 0];
    };
  } forEach DZ_WORLD_INITIAL_VEHICLES;

  {
    _def = _x;
    _veh = createVehicle [(_def select 0), (_def select 1), [], 0, "NONE"];
    if (!isNull _veh) then {
      _veh setDir (_def select 2);
      _veh call dz_fnc_worldRegister;
    } else {
      diag_log format ["[persist] FALHA ao criar %1", _def select 0];
    };
  } forEach DZ_WORLD_INITIAL_TENTS;
};

// Autosave periodico (o servidor e morto com taskkill, entao salva seguido).
[] spawn {
  while {true} do {
    sleep DZ_WORLD_SAVE_INTERVAL;
    call dz_fnc_worldSaveAll;
  };
};

// Restauracao no boot, depois do mundo/proxies carregados.
[] spawn {
  sleep 5;
  call dz_fnc_worldRestore;
  if ((count DZ_WORLD_OBJECTS) == 0) then {
    call dz_fnc_worldSpawnInitial;
  };
};

diag_log "[persist] dz_worldPersist inicializado";
