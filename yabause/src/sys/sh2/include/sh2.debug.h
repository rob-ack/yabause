/*  Copyright 2004 Guillaume Duhamel

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

#ifndef SH2_DEBUG_H
#define SH2_DEBUG_H

#include "core.h"
#include "memory.h"
#include "sh2core.h"

#ifdef  __cplusplus
extern "C"
{
#endif

#define MAX_BREAKPOINTS 10
#define BREAK_BYTEREAD  0x1
#define BREAK_WORDREAD  0x2
#define BREAK_LONGREAD  0x4
#define BREAK_BYTEWRITE 0x8
#define BREAK_WORDWRITE 0x10
#define BREAK_LONGWRITE 0x20

typedef struct
{
    u32 addr;
} codebreakpoint_struct;

typedef struct
{
    u32 addr;
    u32 flags;
    readbytefunc oldreadbyte;
    readwordfunc oldreadword;
    readlongfunc oldreadlong;
    writebytefunc oldwritebyte;
    writewordfunc oldwriteword;
    writelongfunc oldwritelong;
} memorybreakpoint_struct;

typedef struct
{
    u8 enabled;
    void (*callBack)(void *, u32, void *);
    enum SH2STEPTYPE type;

    union
    {
        s32 levels;
        u32 address;
    };
}stepOverOut;

typedef struct
{
    codebreakpoint_struct codebreakpoint[MAX_BREAKPOINTS];
    int numcodebreakpoints;
    memorybreakpoint_struct memorybreakpoint[MAX_BREAKPOINTS];
    int nummemorybreakpoints;
    void (*BreakpointCallBack)(void *, u32, void *);
    void * BreakpointUserData;
    int inbreakpoint;
    int breaknow;
} breakpoint_struct;

typedef struct
{
    u32 addr[256];
    int numbacktrace;
} backtrace_struct;

enum SH2STEPTYPE
{
    SH2ST_STEPOVER,
    SH2ST_STEPOUT
};

void SH2Step(SH2_struct * context);
int SH2StepOver(SH2_struct * context, void (*func)(void *, u32, void *));
void SH2StepOut(SH2_struct * context, void (*func)(void *, u32, void *));

int SH2TrackInfLoopInit(SH2_struct * context);
void SH2TrackInfLoopDeInit(SH2_struct * context);
void SH2TrackInfLoopStart(SH2_struct * context);
void SH2TrackInfLoopStop(SH2_struct * context);
void SH2TrackInfLoopClear(SH2_struct * context);

void SH2GetRegisters(SH2_struct * context, sh2regs_struct * r);
void SH2SetRegisters(SH2_struct * context, sh2regs_struct * r);

void SH2SetBreakpointCallBack(SH2_struct * context, void (*func)(void *, u32, void *), void * userdata);
int SH2AddCodeBreakpoint(SH2_struct * context, u32 addr);
int SH2DelCodeBreakpoint(SH2_struct * context, u32 addr);
codebreakpoint_struct * SH2GetBreakpointList(SH2_struct * context);
void SH2ClearCodeBreakpoints(SH2_struct * context);
void SH2Disasm(u32 v_addr, u16 op, int mode, sh2regs_struct * r, char * string);
void SH2DumpHistory(SH2_struct * context);

void SH2HandleBreakpoints(SH2_struct * context);
void SH2BreakNow(SH2_struct * context);

int SH2AddMemoryBreakpoint(SH2_struct * context, u32 addr, u32 flags);
int SH2DelMemoryBreakpoint(SH2_struct * context, u32 addr);
memorybreakpoint_struct * SH2GetMemoryBreakpointList(SH2_struct * context);
void SH2ClearMemoryBreakpoints(SH2_struct * context);
void SH2HandleBackTrace(SH2_struct * context);
u32 * SH2GetBacktraceList(SH2_struct * context, int * size);
void SH2HandleStepOverOut(SH2_struct * context);
void SH2HandleTrackInfLoop(SH2_struct * context);

#ifdef  __cplusplus
}
#endif

#endif
