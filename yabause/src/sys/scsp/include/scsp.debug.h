/*  Copyright 2004 Stephane Dallongeville
    Copyright 2004-2006 Theo Berkau

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

/*Saturn Custom Sound Processor (SCSP): Also referred to as Yamaha YMF292, it’s composed of two modules:
A multi-function sound generator: Processes up to 32 channels with PCM samples (up to 16-bit with 44.1 kHz, a.k.a ‘CD quality’) or FM channels. In the case of the latter, a number of channels are reserved for operators.
A DSP: Applies effects like echo, reverb and chorus. The docs also mention ‘filters’ but I don’t know if it means envelope or frequency filter (i.e. low pass, etc).
*/

#ifndef SCSP_DEBUG_H
#define SCSP_DEBUG_H

#include "core.h"
#include "sh2core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BREAKPOINTS 10

typedef struct
{
    u32 addr;
} m68kcodebreakpoint_struct;

void ScspSlotDebugStats(u8 slotnum, char *outstring);
void ScspCommonControlRegisterDebugStats(char *outstring);
int ScspSlotDebugSaveRegisters(u8 slotnum, const char *filename);
u32 ScspSlotDebugAudio (u32 *workbuf, s16 *buf, u32 len);
void ScspSlotResetDebug(u8 slotnum);
int ScspSlotDebugAudioSaveWav(u8 slotnum, const char *filename);

void M68KStep(void);
void M68KSetBreakpointCallBack(void (*func)(u32));
int M68KAddCodeBreakpoint(u32 addr);
void M68KSortCodeBreakpoints(void);
int M68KDelCodeBreakpoint(u32 addr);
m68kcodebreakpoint_struct *M68KGetBreakpointList(void);
void M68KClearCodeBreakpoints(void);

void scsp_debug_instrument_get_data(int i, u32 * sa, int * is_muted);
void scsp_debug_instrument_set_mute(u32 sa, int mute);
void scsp_debug_instrument_clear();
void scsp_debug_get_envelope(int chan, int * env, int * state);
void scsp_debug_set_mode(int mode);

#ifdef __cplusplus
}
#endif

#endif
