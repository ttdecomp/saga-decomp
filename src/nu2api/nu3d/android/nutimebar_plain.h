// Time-bar profiling slots — lightweight microsecond stopwatches.
//
// Each "set" is a fixed-size array of slots created by NuTimeBarCreateSet
// (only when profiling is enabled).  The render thread uses set -1
// (aliased to set 0) to publish CPU/GPU timings for the on-screen HUD.
// All calls no-op when no set has been created.  See nutimebar_plain.cpp
// for the double-buffered accumulator layout.
#pragma once

#include "decomp.h"

extern "C" i32 NuTimeBarCreateSetEx(VARIPTR *buffer, VARIPTR unused_buffer, i32 *colours);
extern "C" i32 NuTimeBarCreateSetEx2(VARIPTR *buffer, VARIPTR unused_buffer, i32 field_28);
extern "C" i32 NuTimeBarCreateSet(i32 *colours);
extern "C" void NuTimeBarDestroySet(i32 set);
extern "C" void NuTimeBarInitEx(VARIPTR *buffer, VARIPTR unused_buffer);
extern "C" void NuTimeBarSlotReset(i32 set, i32 slot);
extern "C" void _NuTimeBarSlotBegin(i32 set, i32 slot, const char *name);
extern "C" u32 _NuTimeBarSlotEnd(i32 set, i32 slot);
extern "C" void NuTimeBarSlotSet(i32 set, i32 slot, i32 value);
extern "C" void NuTimeBarSlotSetName(i32 set, i32 slot, const char *name);
extern "C" void NuTimeBarSlotSetEx(i32 set, i32 slot, i32 value, const char *name);
extern "C" i32 NuTimeBarSlotLastValue(i32 set, i32 slot);
extern "C" i32 NuTimeBarSlotLastValueMicroseconds(i32 set, i32 slot);
extern "C" void NuTimeBarResetPeaks(void);
extern "C" void NuTimeBarSetScaleY(void);
extern "C" void NuTimeBarEnable(i32 enabled);
extern "C" void NuTimeBarIndicateGpuFrameOut(i32 enabled);
