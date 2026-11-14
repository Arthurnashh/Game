#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MOVE_ACTIONS_HPP
#define _MOVE_ACTIONS_HPP

DEFINE_ENUM_BEG(MoveFinishF)
	MFNull, // zero is reserved
	MFGetIn,
	MFReload,
	MFThrow,
  MFThrowItem,
  MFBowDraw,
  MFBowRelease,
  MFBowCancel,
	MFUIAction, // perform UIAction when finished
	MFTakeInHands,
	MFDead,
	MFPreciseGetInEnd,
	MFPreciseGetOutEnd,
	NMoveFinish // list terminator - max. value
DEFINE_ENUM_END(MoveFinishF)

#endif
