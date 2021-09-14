/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau

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

#ifndef SCU_DEBUG_H
#define SCU_DEBUG_H

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  u32 addr;
} scucodebreakpoint_struct;

#define MAX_BREAKPOINTS 10

typedef struct
{
    scucodebreakpoint_struct codebreakpoint[MAX_BREAKPOINTS];
    int numcodebreakpoints;
    void(*BreakpointCallBack)(u32);
    u8 inbreakpoint;
} scubp_struct;

void ScuDspDisasm(u8 addr, char *outstring);
void ScuDspStep(void);
void ScuDspSetBreakpointCallBack(void (*func)(u32));
int ScuDspAddCodeBreakpoint(u32 addr);
int ScuDspDelCodeBreakpoint(u32 addr);
scucodebreakpoint_struct *ScuDspGetBreakpointList(void);
void ScuDspClearCodeBreakpoints(void);
void ScuCheckBreakpoints(void);

#ifdef __cplusplus
}
#endif

#endif
