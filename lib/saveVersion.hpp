// load / save game
const int WorldSerializeVersion = 24;

// load / save mission
const int MissionsVersion = 11;

// UserInfo.cfg file
const int UserInfoVersion = 2;

#ifndef SERIAL_BRANCH
const int SerializeBranch = 0xffff0000;
#define SERIAL_BRANCH(ver) ((ver) & SerializeBranch)
#define SERIAL_VERSION(ver) ((ver) & ~SerializeBranch)
#endif

// load / save unit status
#ifndef IS_UNIT_STATUS_BRANCH
const int UnitStatusBase = 0x00010000;
#define IS_UNIT_STATUS_BRANCH(ver) (SERIAL_BRANCH(ver) == UnitStatusBase)
#endif
#define MAKE_UNIT_STATUS(ver) ((ver) | UnitStatusBase)
const int UnitStatusVersion = MAKE_UNIT_STATUS(1);

// load / save public variable message
#ifndef IS_PUBLIC_VARIABLE_BRANCH
const int PublicVariableBase = 0x00020000;
#define IS_PUBLIC_VARIABLE_BRANCH(ver) (SERIAL_BRANCH(ver) == PublicVariableBase)
#endif
#define MAKE_PUBLIC_VARIABLE(ver) ((ver) | PublicVariableBase)
const int PublicVariableVersion = MAKE_PUBLIC_VARIABLE(1);

