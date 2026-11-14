#ifdef _MSC_VER
#pragma once
#endif

#ifndef _AI_ABILITIES_HPP
#define _AI_ABILITIES_HPP

#define ABILITY_KIND_ENUM(type, prefix, XX) \
  XX(type, prefix, AimingSpeed) \
  XX(type, prefix, AimingAccuracy) \
  XX(type, prefix, AimingShake) \
  XX(type, prefix, Endurance) \
  XX(type, prefix, SpotDistance) \
  XX(type, prefix, SpotTime) \
  XX(type, prefix, Courage) \
  XX(type, prefix, ReloadSpeed) \
  XX(type, prefix, Commanding) \
  XX(type, prefix, General)

#ifndef DECL_ENUM_ABILITY_KIND
#define DECL_ENUM_ABILITY_KIND
DECL_ENUM(AbilityKind)
#endif
DECLARE_ENUM(AbilityKind, AK, ABILITY_KIND_ENUM)

#endif
