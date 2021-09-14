#include "sh2.debug.h"

codebreakpoint_struct dummy;

//////////////////////////////////////////////////////////////////////////////

void SH2Step(SH2_struct* context)
{
    if (SH2Core)
    {
        u32 tmp = SH2Core->GetPC(context);

        // Execute 1 instruction
        SH2Exec(context, 1);

        // Sometimes it doesn't always execute one instruction,
        // let's make sure it did
        if (tmp == SH2Core->GetPC(context))
            SH2Exec(context, 1);
    }
}

//////////////////////////////////////////////////////////////////////////////

int SH2StepOver(SH2_struct* context, void (*func)(void*, u32, void*))
{
    //if (SH2Core)
    //{
    //    u32 tmp = SH2Core->GetPC(context);
    //    u16 inst = MappedMemoryReadWord(context->regs.PC, NULL);

    //    // If instruction is jsr, bsr, or bsrf, step over it
    //    if ((inst & 0xF000) == 0xB000 || // BSR 
    //        (inst & 0xF0FF) == 0x0003 || // BSRF
    //        (inst & 0xF0FF) == 0x400B)   // JSR
    //    {
    //        // Set breakpoint after at PC + 4
    //        context->stepOverOut.callBack = func;
    //        context->stepOverOut.type = SH2ST_STEPOVER;
    //        context->stepOverOut.enabled = 1;
    //        context->stepOverOut.address = context->regs.PC + 4;
    //        return 1;
    //    }
    //    else
    //    {
    //        // Execute 1 instruction instead
    //        SH2Exec(context, 1);

    //        // Sometimes it doesn't always execute one instruction,
    //        // let's make sure it did
    //        if (tmp == SH2Core->GetPC(context))
    //            SH2Exec(context, 1);
    //    }
    //}
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2StepOut(SH2_struct* context, void (*func)(void*, u32, void*))
{
    //if (SH2Core)
    //{
    //    context->stepOverOut.callBack = func;
    //    context->stepOverOut.type = SH2ST_STEPOUT;
    //    context->stepOverOut.enabled = 1;
    //    context->stepOverOut.address = 0;
    //}
}

void SH2BreakNow(SH2_struct* context)
{
//    context->bp.breaknow = 1;
}

//////////////////////////////////////////////////////////////////////////////

void SH2SetBreakpointCallBack(SH2_struct* context, void (*func)(void*, u32, void*), void* userdata) {
    //context->bp.BreakpointCallBack = func;
    //context->bp.BreakpointUserData = userdata;
}

//////////////////////////////////////////////////////////////////////////////

int SH2AddCodeBreakpoint(SH2_struct* context, u32 addr) {
    //int i;

    //if (context->bp.numcodebreakpoints < MAX_BREAKPOINTS) {
    //    // Make sure it isn't already on the list
    //    for (i = 0; i < context->bp.numcodebreakpoints; i++)
    //    {
    //        if (addr == context->bp.codebreakpoint[i].addr)
    //            return -1;
    //    }

    //    context->bp.codebreakpoint[context->bp.numcodebreakpoints].addr = addr;
    //    context->bp.numcodebreakpoints++;

    //    return 0;
    //}

    return -1;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2SortCodeBreakpoints(SH2_struct* context) {
    //int i, i2;
    //u32 tmp;

    //for (i = 0; i < (MAX_BREAKPOINTS - 1); i++)
    //{
    //    for (i2 = i + 1; i2 < MAX_BREAKPOINTS; i2++)
    //    {
    //        if (context->bp.codebreakpoint[i].addr == 0xFFFFFFFF &&
    //            context->bp.codebreakpoint[i2].addr != 0xFFFFFFFF)
    //        {
    //            tmp = context->bp.codebreakpoint[i].addr;
    //            context->bp.codebreakpoint[i].addr = context->bp.codebreakpoint[i2].addr;
    //            context->bp.codebreakpoint[i2].addr = tmp;
    //        }
    //    }
    //}
}

//////////////////////////////////////////////////////////////////////////////

int SH2DelCodeBreakpoint(SH2_struct* context, u32 addr) {
    //int i, i2;

    //LOG("Deleting breakpoint %08X...\n", addr);

    //if (context->bp.numcodebreakpoints > 0) {
    //    for (i = 0; i < context->bp.numcodebreakpoints; i++) {
    //        if (context->bp.codebreakpoint[i].addr == addr)
    //        {
    //            context->bp.codebreakpoint[i].addr = 0xFFFFFFFF;
    //            SH2SortCodeBreakpoints(context);
    //            context->bp.numcodebreakpoints--;

    //            LOG("Remaining breakpoints: \n");

    //            for (i2 = 0; i2 < context->bp.numcodebreakpoints; i2++)
    //            {
    //                LOG("%08X", context->bp.codebreakpoint[i2].addr);
    //            }

    //            return 0;
    //        }
    //    }
    //}

    //LOG("Failed deleting breakpoint\n");

    return -1;
}

//////////////////////////////////////////////////////////////////////////////

codebreakpoint_struct* SH2GetBreakpointList(SH2_struct* context) {
    return &dummy;
}

//////////////////////////////////////////////////////////////////////////////

void SH2ClearCodeBreakpoints(SH2_struct* context) {
    //int i;
    //for (i = 0; i < MAX_BREAKPOINTS; i++) {
    //    context->bp.codebreakpoint[i].addr = 0xFFFFFFFF;
    //}

    //context->bp.numcodebreakpoints = 0;
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL SH2MemoryBreakpointReadByte(u32 addr) {
    //int i;

    //for (i = 0; i < CurrentSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (CurrentSH2->bp.memorybreakpoint[i].addr == (addr & 0x0FFFFFFF))
    //    {
    //        if (CurrentSH2->bp.BreakpointCallBack && CurrentSH2->bp.inbreakpoint == 0)
    //        {
    //            CurrentSH2->bp.inbreakpoint = 1;
    //            SH2DumpHistory(CurrentSH2);
    //            CurrentSH2->bp.BreakpointCallBack(CurrentSH2, 0, CurrentSH2->bp.BreakpointUserData);
    //            CurrentSH2->bp.inbreakpoint = 0;
    //        }

    //        return CurrentSH2->bp.memorybreakpoint[i].oldreadbyte(addr);
    //    }
    //}

    //// Use the closest match if address doesn't match
    //for (i = 0; i < MSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (((MSH2->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
    //        return MSH2->bp.memorybreakpoint[i].oldreadbyte(addr);
    //}
    //for (i = 0; i < SSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (((SSH2->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
    //        return SSH2->bp.memorybreakpoint[i].oldreadbyte(addr);
    //}

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL SH2MemoryBreakpointReadWord(u32 addr) {
    //int i;

    //for (i = 0; i < CurrentSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (CurrentSH2->bp.memorybreakpoint[i].addr == (addr & 0x0FFFFFFF))
    //    {
    //        if (CurrentSH2->bp.BreakpointCallBack && CurrentSH2->bp.inbreakpoint == 0)
    //        {
    //            CurrentSH2->bp.inbreakpoint = 1;
    //            SH2DumpHistory(CurrentSH2);
    //            CurrentSH2->bp.BreakpointCallBack(CurrentSH2, 0, CurrentSH2->bp.BreakpointUserData);
    //            CurrentSH2->bp.inbreakpoint = 0;
    //        }

    //        return CurrentSH2->bp.memorybreakpoint[i].oldreadword(addr);
    //    }
    //}

    //// Use the closest match if address doesn't match
    //for (i = 0; i < MSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (((MSH2->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
    //        return MSH2->bp.memorybreakpoint[i].oldreadword(addr);
    //}
    //for (i = 0; i < SSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (((SSH2->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
    //        return SSH2->bp.memorybreakpoint[i].oldreadword(addr);
    //}

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL SH2MemoryBreakpointReadLong(u32 addr) {
    //int i;

    //for (i = 0; i < CurrentSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (CurrentSH2->bp.memorybreakpoint[i].addr == (addr & 0x0FFFFFFF))
    //    {
    //        if (CurrentSH2->bp.BreakpointCallBack && CurrentSH2->bp.inbreakpoint == 0)
    //        {
    //            CurrentSH2->bp.inbreakpoint = 1;
    //            SH2DumpHistory(CurrentSH2);
    //            CurrentSH2->bp.BreakpointCallBack(CurrentSH2, 0, CurrentSH2->bp.BreakpointUserData);
    //            CurrentSH2->bp.inbreakpoint = 0;
    //        }

    //        return CurrentSH2->bp.memorybreakpoint[i].oldreadlong(addr);
    //    }
    //}

    //// Use the closest match if address doesn't match
    //for (i = 0; i < MSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (((MSH2->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
    //        return MSH2->bp.memorybreakpoint[i].oldreadlong(addr);
    //}
    //for (i = 0; i < SSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (((SSH2->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
    //        return SSH2->bp.memorybreakpoint[i].oldreadlong(addr);
    //}

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL SH2MemoryBreakpointWriteByte(u32 addr, u8 val) {
    //int i;

    //for (i = 0; i < CurrentSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (CurrentSH2->bp.memorybreakpoint[i].addr == (addr & 0x0FFFFFFF))
    //    {
    //        if (CurrentSH2->bp.BreakpointCallBack && CurrentSH2->bp.inbreakpoint == 0)
    //        {
    //            CurrentSH2->bp.inbreakpoint = 1;
    //            SH2DumpHistory(CurrentSH2);
    //            CurrentSH2->bp.BreakpointCallBack(CurrentSH2, 0, CurrentSH2->bp.BreakpointUserData);
    //            CurrentSH2->bp.inbreakpoint = 0;
    //        }

    //        CurrentSH2->bp.memorybreakpoint[i].oldwritebyte(addr, val);
    //        return;
    //    }
    //}

    //// Use the closest match if address doesn't match
    //for (i = 0; i < MSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (((MSH2->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
    //    {
    //        MSH2->bp.memorybreakpoint[i].oldwritebyte(addr, val);
    //        return;
    //    }
    //}
    //for (i = 0; i < SSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (((SSH2->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
    //    {
    //        SSH2->bp.memorybreakpoint[i].oldwritebyte(addr, val);
    //        return;
    //    }
    //}
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL SH2MemoryBreakpointWriteWord(u32 addr, u16 val) {
    //int i;

    //for (i = 0; i < CurrentSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (CurrentSH2->bp.memorybreakpoint[i].addr == (addr & 0x0FFFFFFF))
    //    {
    //        if (CurrentSH2->bp.BreakpointCallBack && CurrentSH2->bp.inbreakpoint == 0)
    //        {
    //            CurrentSH2->bp.inbreakpoint = 1;
    //            SH2DumpHistory(CurrentSH2);
    //            CurrentSH2->bp.BreakpointCallBack(CurrentSH2, 0, CurrentSH2->bp.BreakpointUserData);
    //            CurrentSH2->bp.inbreakpoint = 0;
    //        }

    //        CurrentSH2->bp.memorybreakpoint[i].oldwriteword(addr, val);
    //        return;
    //    }
    //}

    //// Use the closest match if address doesn't match
    //for (i = 0; i < MSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (((MSH2->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
    //    {
    //        MSH2->bp.memorybreakpoint[i].oldwriteword(addr, val);
    //        return;
    //    }
    //}
    //for (i = 0; i < SSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (((SSH2->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
    //    {
    //        SSH2->bp.memorybreakpoint[i].oldwriteword(addr, val);
    //        return;
    //    }
    //}
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL SH2MemoryBreakpointWriteLong(u32 addr, u32 val) {
    //int i;

    //for (i = 0; i < CurrentSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (CurrentSH2->bp.memorybreakpoint[i].addr == (addr & 0x0FFFFFFF))
    //    {
    //        if (CurrentSH2->bp.BreakpointCallBack && CurrentSH2->bp.inbreakpoint == 0)
    //        {
    //            CurrentSH2->bp.inbreakpoint = 1;
    //            SH2DumpHistory(CurrentSH2);
    //            CurrentSH2->bp.BreakpointCallBack(CurrentSH2, 0, CurrentSH2->bp.BreakpointUserData);
    //            CurrentSH2->bp.inbreakpoint = 0;
    //        }

    //        CurrentSH2->bp.memorybreakpoint[i].oldwritelong(addr, val);
    //        return;
    //    }
    //}

    //// Use the closest match if address doesn't match
    //for (i = 0; i < MSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (((MSH2->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
    //    {
    //        MSH2->bp.memorybreakpoint[i].oldwritelong(addr, val);
    //        return;
    //    }
    //}
    //for (i = 0; i < SSH2->bp.nummemorybreakpoints; i++)
    //{
    //    if (((SSH2->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
    //    {
    //        SSH2->bp.memorybreakpoint[i].oldwritelong(addr, val);
    //        return;
    //    }
    //}
}

//////////////////////////////////////////////////////////////////////////////

static int CheckForMemoryBreakpointDupes(SH2_struct* context, u32 addr, u32 flag, int* which)
{
    //int i;

    //for (i = 0; i < context->bp.nummemorybreakpoints; i++)
    //{
    //    if (((context->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) ==
    //        ((addr >> 16) & 0xFFF))
    //    {
    //        // See it actually was using the same operation flag
    //        if (context->bp.memorybreakpoint[i].flags & flag)
    //        {
    //            *which = i;
    //            return 1;
    //        }
    //    }
    //}

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

int SH2AddMemoryBreakpoint(SH2_struct* context, u32 addr, u32 flags) {
    //int which;
    //int i;

    //if (flags == 0)
    //    return -1;

    //if (context->bp.nummemorybreakpoints < MAX_BREAKPOINTS) {
    //    // Only regular addresses are supported at this point(Sorry, no onchip!)
    //    switch (addr >> 29) {
    //    case 0x0:
    //    case 0x1:
    //    case 0x5:
    //        break;
    //    default:
    //        return -1;
    //    }

    //    addr &= 0x0FFFFFFF;

    //    // Make sure it isn't already on the list
    //    for (i = 0; i < context->bp.nummemorybreakpoints; i++)
    //    {
    //        if (addr == context->bp.memorybreakpoint[i].addr)
    //            return -1;
    //    }

    //    context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].addr = addr;
    //    context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].flags = flags;

    //    context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldreadbyte = ReadByteList[(addr >> 16) & 0xFFF];
    //    context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldreadword = ReadWordList[(addr >> 16) & 0xFFF];
    //    context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldreadlong = ReadLongList[(addr >> 16) & 0xFFF];
    //    context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldwritebyte = WriteByteList[(addr >> 16) & 0xFFF];
    //    context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldwriteword = WriteWordList[(addr >> 16) & 0xFFF];
    //    context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldwritelong = WriteLongList[(addr >> 16) & 0xFFF];

    //    if (flags & BREAK_BYTEREAD)
    //    {
    //        // Make sure function isn't already being breakpointed by another breakpoint
    //        if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_BYTEREAD, &which))
    //            ReadByteList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointReadByte;
    //        else
    //            // fix old memory access function
    //            context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldreadbyte = context->bp.memorybreakpoint[which].oldreadbyte;
    //    }

    //    if (flags & BREAK_WORDREAD)
    //    {
    //        // Make sure function isn't already being breakpointed by another breakpoint
    //        if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_WORDREAD, &which))
    //            ReadWordList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointReadWord;
    //        else
    //            // fix old memory access function
    //            context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldreadword = context->bp.memorybreakpoint[which].oldreadword;
    //    }

    //    if (flags & BREAK_LONGREAD)
    //    {
    //        // Make sure function isn't already being breakpointed by another breakpoint
    //        if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_LONGREAD, &which))
    //            ReadLongList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointReadLong;
    //        else
    //            // fix old memory access function
    //            context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldreadword = context->bp.memorybreakpoint[which].oldreadword;
    //    }

    //    if (flags & BREAK_BYTEWRITE)
    //    {
    //        // Make sure function isn't already being breakpointed by another breakpoint
    //        if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_BYTEWRITE, &which))
    //            WriteByteList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointWriteByte;
    //        else
    //            // fix old memory access function
    //            context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldwritebyte = context->bp.memorybreakpoint[which].oldwritebyte;
    //    }

    //    if (flags & BREAK_WORDWRITE)
    //    {
    //        // Make sure function isn't already being breakpointed by another breakpoint
    //        if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_WORDWRITE, &which))
    //            WriteWordList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointWriteWord;
    //        else
    //            // fix old memory access function
    //            context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldwriteword = context->bp.memorybreakpoint[which].oldwriteword;
    //    }

    //    if (flags & BREAK_LONGWRITE)
    //    {
    //        // Make sure function isn't already being breakpointed by another breakpoint
    //        if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_LONGWRITE, &which))
    //            WriteLongList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointWriteLong;
    //        else
    //            // fix old memory access function
    //            context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldwritelong = context->bp.memorybreakpoint[which].oldwritelong;
    //    }

    //    context->bp.nummemorybreakpoints++;

    //    return 0;
    //}

    return -1;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2SortMemoryBreakpoints(SH2_struct* context) {
    int i, i2;
    memorybreakpoint_struct tmp;

    //for (i = 0; i < (MAX_BREAKPOINTS - 1); i++)
    //{
    //    for (i2 = i + 1; i2 < MAX_BREAKPOINTS; i2++)
    //    {
    //        if (context->bp.memorybreakpoint[i].addr == 0xFFFFFFFF &&
    //            context->bp.memorybreakpoint[i2].addr != 0xFFFFFFFF)
    //        {
    //            memcpy(&tmp, context->bp.memorybreakpoint + i, sizeof(memorybreakpoint_struct));
    //            memcpy(context->bp.memorybreakpoint + i, context->bp.memorybreakpoint + i2, sizeof(memorybreakpoint_struct));
    //            memcpy(context->bp.memorybreakpoint + i2, &tmp, sizeof(memorybreakpoint_struct));
    //        }
    //    }
    //}
}

//////////////////////////////////////////////////////////////////////////////

int SH2DelMemoryBreakpoint(SH2_struct* context, u32 addr) {
    int i, i2;

    //if (context->bp.nummemorybreakpoints > 0) {
    //    for (i = 0; i < context->bp.nummemorybreakpoints; i++) {
    //        if (context->bp.memorybreakpoint[i].addr == addr)
    //        {
    //            // Remove memory access piggyback function to memory access function table

    //            // Make sure no other breakpoints need the breakpoint functions first
    //            for (i2 = 0; i2 < context->bp.nummemorybreakpoints; i2++)
    //            {
    //                if (((context->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) ==
    //                    ((context->bp.memorybreakpoint[i2].addr >> 16) & 0xFFF) &&
    //                    i != i2)
    //                {
    //                    // Clear the flags
    //                    context->bp.memorybreakpoint[i].flags &= ~context->bp.memorybreakpoint[i2].flags;
    //                }
    //            }

    //            if (context->bp.memorybreakpoint[i].flags & BREAK_BYTEREAD)
    //                ReadByteList[(addr >> 16) & 0xFFF] = context->bp.memorybreakpoint[i].oldreadbyte;

    //            if (context->bp.memorybreakpoint[i].flags & BREAK_WORDREAD)
    //                ReadWordList[(addr >> 16) & 0xFFF] = context->bp.memorybreakpoint[i].oldreadword;

    //            if (context->bp.memorybreakpoint[i].flags & BREAK_LONGREAD)
    //                ReadLongList[(addr >> 16) & 0xFFF] = context->bp.memorybreakpoint[i].oldreadlong;

    //            if (context->bp.memorybreakpoint[i].flags & BREAK_BYTEWRITE)
    //                WriteByteList[(addr >> 16) & 0xFFF] = context->bp.memorybreakpoint[i].oldwritebyte;

    //            if (context->bp.memorybreakpoint[i].flags & BREAK_WORDWRITE)
    //                WriteWordList[(addr >> 16) & 0xFFF] = context->bp.memorybreakpoint[i].oldwriteword;

    //            if (context->bp.memorybreakpoint[i].flags & BREAK_LONGWRITE)
    //                WriteLongList[(addr >> 16) & 0xFFF] = context->bp.memorybreakpoint[i].oldwritelong;

    //            context->bp.memorybreakpoint[i].addr = 0xFFFFFFFF;
    //            SH2SortMemoryBreakpoints(context);
    //            context->bp.nummemorybreakpoints--;
    //            return 0;
    //        }
    //    }
    //}

    return -1;
}

//////////////////////////////////////////////////////////////////////////////

memorybreakpoint_struct* SH2GetMemoryBreakpointList(SH2_struct* context) {
    //return context->bp.memorybreakpoint;
    return &dummy;
}

//////////////////////////////////////////////////////////////////////////////

void SH2ClearMemoryBreakpoints(SH2_struct* context) {
    //int i;
    //for (i = 0; i < MAX_BREAKPOINTS; i++)
    //{
    //    context->bp.memorybreakpoint[i].addr = 0xFFFFFFFF;
    //    context->bp.memorybreakpoint[i].flags = 0;
    //    context->bp.memorybreakpoint[i].oldreadbyte = NULL;
    //    context->bp.memorybreakpoint[i].oldreadword = NULL;
    //    context->bp.memorybreakpoint[i].oldreadlong = NULL;
    //    context->bp.memorybreakpoint[i].oldwritebyte = NULL;
    //    context->bp.memorybreakpoint[i].oldwriteword = NULL;
    //    context->bp.memorybreakpoint[i].oldwritelong = NULL;
    //}
    //context->bp.nummemorybreakpoints = 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2HandleBackTrace(SH2_struct* context)
{
    //u16 inst = context->instruction;
    //if ((inst & 0xF000) == 0xB000 || // BSR 
    //    (inst & 0xF0FF) == 0x0003 || // BSRF
    //    (inst & 0xF0FF) == 0x400B)   // JSR
    //{
    //    if (context->bt.numbacktrace < sizeof(context->bt.addr) / sizeof(u32))
    //    {
    //        context->bt.addr[context->bt.numbacktrace] = context->regs.PC;
    //        context->bt.numbacktrace++;
    //    }
    //}
    //else if (inst == 0x000B) // RTS
    //{
    //    if (context->bt.numbacktrace > 0)
    //        context->bt.numbacktrace--;
    //}
}

//////////////////////////////////////////////////////////////////////////////

u32* SH2GetBacktraceList(SH2_struct* context, int* size)
{
    //*size = context->bt.numbacktrace;
    //return context->bt.addr;
}

//////////////////////////////////////////////////////////////////////////////

void SH2HandleStepOverOut(SH2_struct* context)
{
    //if (context->stepOverOut.enabled)
    //{
    //    switch ((int)context->stepOverOut.type)
    //    {
    //    case SH2ST_STEPOVER: // Step Over
    //        if (context->regs.PC == context->stepOverOut.address)
    //        {
    //            context->stepOverOut.enabled = 0;
    //            context->stepOverOut.callBack(context, context->regs.PC, (void*)context->stepOverOut.type);
    //        }
    //        break;
    //    case SH2ST_STEPOUT: // Step Out
    //    {
    //        u16 inst;

    //        if (context->stepOverOut.levels < 0 && context->regs.PC == context->regs.PR)
    //        {
    //            context->stepOverOut.enabled = 0;
    //            context->stepOverOut.callBack(context, context->regs.PC, (void*)context->stepOverOut.type);
    //            return;
    //        }

    //        inst = context->instruction;;

    //        if ((inst & 0xF000) == 0xB000 || // BSR 
    //            (inst & 0xF0FF) == 0x0003 || // BSRF
    //            (inst & 0xF0FF) == 0x400B)   // JSR
    //            context->stepOverOut.levels++;
    //        else if (inst == 0x000B || // RTS
    //            inst == 0x002B)   // RTE
    //            context->stepOverOut.levels--;

    //        break;
    //    }
    //    default: break;
    //    }
    //}
}

//////////////////////////////////////////////////////////////////////////////

void SH2HandleTrackInfLoop(SH2_struct* context)
{
    //if (context->trackInfLoop.enabled)
    //{
    //    // Look for specific bf/bt/bra instructions that branch to address < PC
    //    if ((context->instruction & 0x8B80) == 0x8B80 || // bf
    //        (context->instruction & 0x8F80) == 0x8F80 || // bf/s 
    //        (context->instruction & 0x8980) == 0x8980 || // bt
    //        (context->instruction & 0x8D80) == 0x8D80 || // bt/s 
    //        (context->instruction & 0xA800) == 0xA800)   // bra
    //    {
    //        int i;

    //        // See if it's already on match list
    //        for (i = 0; i < context->trackInfLoop.num; i++)
    //        {
    //            if (context->regs.PC == context->trackInfLoop.match[i].addr)
    //            {
    //                context->trackInfLoop.match[i].count++;
    //                return;
    //            }
    //        }

    //        if (context->trackInfLoop.num >= context->trackInfLoop.maxNum)
    //        {
    //            context->trackInfLoop.match = realloc(context->trackInfLoop.match, sizeof(tilInfo_struct) * (context->trackInfLoop.maxNum * 2));
    //            context->trackInfLoop.maxNum *= 2;
    //        }

    //        // Add new
    //        i = context->trackInfLoop.num;
    //        context->trackInfLoop.match[i].addr = context->regs.PC;
    //        context->trackInfLoop.match[i].count = 1;
    //        context->trackInfLoop.num++;
    //    }
    //}
}

//////////////////////////////////////////////////////////////////////////////

int SH2TrackInfLoopInit(SH2_struct* context)
{
    //context->trackInfLoop.maxNum = 100;
    //if ((context->trackInfLoop.match = calloc(context->trackInfLoop.maxNum, sizeof(tilInfo_struct))) == NULL)
    //    return -1;

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2TrackInfLoopDeInit(SH2_struct* context)
{
    //if (context->trackInfLoop.match)
    //    free(context->trackInfLoop.match);
}

//////////////////////////////////////////////////////////////////////////////

void SH2TrackInfLoopStart(SH2_struct* context)
{
    //context->trackInfLoop.enabled = 1;
}

//////////////////////////////////////////////////////////////////////////////

void SH2TrackInfLoopStop(SH2_struct* context)
{
    //context->trackInfLoop.enabled = 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2TrackInfLoopClear(SH2_struct* context)
{
    //memset(context->trackInfLoop.match, 0, sizeof(tilInfo_struct) * context->trackInfLoop.maxNum);
    //context->trackInfLoop.num = 0;
}
