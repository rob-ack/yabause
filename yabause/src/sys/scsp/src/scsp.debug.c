/*
 * Copyright 2004 Stephane Dallongeville
 * Copyright 2004-2007 Theo Berkau
 * Copyright 2006 Guillaume Duhamel
 * Copyright 2012 Chris Lord
 *
 * This file is part of Yabause.
 *
 * Yabause is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Yabause is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Yabause; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>

#include "m68kcore.h"
#include "mk68Counter.h"
#include "scu.h"
#include "scsp.h"
#include "scsp.debug.h"
#include "scsp.private.h"
#include "threads.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

struct DebugInstrument
{
    u32 sa;
    int is_muted;
};

//////////////////////////////////////////////////////////////////////////////

static u64 m68k_counter = 0;
static u64 m68k_counter_done = 0;
static YabSem * m68counterCond;
extern ScspInternal * ScspInternalVars;
extern scsp_t scsp; // SCSP structure
extern s32 * scsp_bufL;
extern s32 * scsp_bufR;
extern u8 IsM68KRunning;
extern s32 FASTCALL(*m68kexecptr)(s32 cycles); // M68K->Exec or M68KExecBP
extern s32 savedcycles; // Cycles left over from the last M68KExec() call
extern u8 * scsp_isr;
extern u32 scsp_buf_len;
extern u32 scsp_buf_pos;
extern void (*scsp_slot_update_p[2][2][2][2][2])(slot_t * slot);
extern void scsp_attack_next(slot_t * slot);
extern Scsp new_scsp;
#define NUM_DEBUG_INSTRUMENTS 24

static s32 FASTCALL M68KExecBP(s32 cycles)
{
    s32 cyclestoexec = cycles;
    s32 cyclesexecuted = 0;
    int i;

    while (cyclesexecuted < cyclestoexec)
    {
        // Make sure it isn't one of our breakpoints
        for (i = 0; i < ScspInternalVars->numcodebreakpoints; i++)
        {
            if ((M68K->GetPC() == ScspInternalVars->codebreakpoint[i].addr) &&
                ScspInternalVars->inbreakpoint == 0)
            {
                ScspInternalVars->inbreakpoint = 1;
                if (ScspInternalVars->BreakpointCallBack)
                    ScspInternalVars->BreakpointCallBack(ScspInternalVars->codebreakpoint[i].addr);
                ScspInternalVars->inbreakpoint = 0;
            }
        }

        // execute instructions individually
        cyclesexecuted += M68K->Exec(1);

    }
    return cyclesexecuted;
}

struct DebugInstrument debug_instruments[NUM_DEBUG_INSTRUMENTS] = {0};
int debug_instrument_pos = 0;

void scsp_debug_search_instruments(const u32 sa, int * found, int * offset)
{
    int i = 0;
    *found = 0;
    for (i = 0; i < NUM_DEBUG_INSTRUMENTS; i++)
    {
        if (debug_instruments[i].sa == sa)
        {
            *found = 1;
            break;
        }
    }

    *offset = i;
}

void scsp_debug_add_instrument(u32 sa)
{
    int i = 0, found = 0, offset = 0;

    if (debug_instrument_pos >= NUM_DEBUG_INSTRUMENTS)
        return;

    scsp_debug_search_instruments(sa, &found, &offset);

    //new instrument discovered
    if (!found)
        debug_instruments[debug_instrument_pos++].sa = sa;
}

void scsp_debug_instrument_set_mute(u32 sa, int mute)
{
    int found = 0, offset = 0;
    scsp_debug_search_instruments(sa, &found, &offset);

    if (offset >= NUM_DEBUG_INSTRUMENTS)
        return;

    if (found)
        debug_instruments[offset].is_muted = mute;
}

int scsp_debug_instrument_check_is_muted(u32 sa)
{
    int found = 0, offset = 0;
    scsp_debug_search_instruments(sa, &found, &offset);

    if (offset >= NUM_DEBUG_INSTRUMENTS)
        return 0;

    if (found && debug_instruments[offset].is_muted)
        return 1;

    return 0;
}

void scsp_debug_instrument_get_data(int i, u32 * sa, int * is_muted)
{
    if (i >= NUM_DEBUG_INSTRUMENTS)
        return;

    *sa = debug_instruments[i].sa;
    *is_muted = debug_instruments[i].is_muted;
}

void scsp_debug_set_mode(int mode)
{
    new_scsp.debug_mode = mode;
}

void scsp_debug_instrument_clear()
{
    debug_instrument_pos = 0;
    memset(debug_instruments, 0, sizeof(struct DebugInstrument) * NUM_DEBUG_INSTRUMENTS);
}

void scsp_debug_get_envelope(int chan, int * env, int * state)
{
    *env = new_scsp.slots[chan].state.attenuation;
    *state = new_scsp.slots[chan].state.envelope;
}


//////////////////////////////////////////////////////////////////////////////

void
M68KSetBreakpointCallBack(void (*func)(u32))
{
    ScspInternalVars->BreakpointCallBack = func;
}

//////////////////////////////////////////////////////////////////////////////

int
M68KAddCodeBreakpoint(u32 addr)
{
    int i;

    if (ScspInternalVars->numcodebreakpoints < MAX_BREAKPOINTS)
    {
        // Make sure it isn't already on the list
        for (i = 0; i < ScspInternalVars->numcodebreakpoints; i++)
        {
            if (addr == ScspInternalVars->codebreakpoint[i].addr)
                return -1;
        }

        ScspInternalVars->codebreakpoint[i].addr = addr;
        ScspInternalVars->numcodebreakpoints++;
        m68kexecptr = M68KExecBP;

        return 0;
    }

    return -1;
}

//////////////////////////////////////////////////////////////////////////////

void
M68KSortCodeBreakpoints(void)
{
    int i, i2;
    u32 tmp;

    for (i = 0; i < (MAX_BREAKPOINTS - 1); i++)
    {
        for (i2 = i + 1; i2 < MAX_BREAKPOINTS; i2++)
        {
            if (ScspInternalVars->codebreakpoint[i].addr == 0xFFFFFFFF &&
                ScspInternalVars->codebreakpoint[i2].addr != 0xFFFFFFFF)
            {
                tmp = ScspInternalVars->codebreakpoint[i].addr;
                ScspInternalVars->codebreakpoint[i].addr =
                    ScspInternalVars->codebreakpoint[i2].addr;
                ScspInternalVars->codebreakpoint[i2].addr = tmp;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

int
M68KDelCodeBreakpoint(u32 addr)
{
    int i;
    if (ScspInternalVars->numcodebreakpoints > 0)
    {
        for (i = 0; i < ScspInternalVars->numcodebreakpoints; i++)
        {
            if (ScspInternalVars->codebreakpoint[i].addr == addr)
            {
                ScspInternalVars->codebreakpoint[i].addr = 0xFFFFFFFF;
                M68KSortCodeBreakpoints();
                ScspInternalVars->numcodebreakpoints--;
                if (ScspInternalVars->numcodebreakpoints == 0)
                    m68kexecptr = M68K->Exec;
                return 0;
            }
        }
    }

    return -1;
}

//////////////////////////////////////////////////////////////////////////////

m68kcodebreakpoint_struct * M68KGetBreakpointList()
{
    return ScspInternalVars->codebreakpoint;
}

//////////////////////////////////////////////////////////////////////////////

void
M68KClearCodeBreakpoints()
{
    int i;
    for (i = 0; i < MAX_BREAKPOINTS; i++)
        ScspInternalVars->codebreakpoint[i].addr = 0xFFFFFFFF;

    ScspInternalVars->numcodebreakpoints = 0;
}

//////////////////////////////////////////////////////////////////////////////

static char *
AddSoundLFO(char * outstring, const char * string, u16 level, u16 waveform)
{
    if (level > 0)
    {
        switch (waveform)
        {
            case 0:
                AddString(outstring, "%s Sawtooth\r\n", string);
                break;
            case 1:
                AddString(outstring, "%s Square\r\n", string);
                break;
            case 2:
                AddString(outstring, "%s Triangle\r\n", string);
                break;
            case 3:
                AddString(outstring, "%s Noise\r\n", string);
                break;
        }
    }

    return outstring;
}

//////////////////////////////////////////////////////////////////////////////

void
M68KStep(void)
{
    M68K->Exec(1);
}

//////////////////////////////////////////////////////////////////////////////

static char *
AddSoundPan(char * outstring, u16 pan)
{
    if (pan == 0x0F)
    {
        AddString(outstring, "Left = -MAX dB, Right = -0 dB\r\n");
    } else if (pan == 0x1F)
    {
        AddString(outstring, "Left = -0 dB, Right = -MAX dB\r\n");
    } else
    {
        AddString(outstring, "Left = -%d dB, Right = -%d dB\r\n", (pan & 0xF) * 3, (pan >> 4) * 3);
    }

    return outstring;
}

//////////////////////////////////////////////////////////////////////////////

static char *
AddSoundLevel(char * outstring, u16 level)
{
    if (level == 0)
    {
        AddString(outstring, "-MAX dB\r\n");
    } else
    {
        AddString(outstring, "-%d dB\r\n", (7-level) * 6);
    }

    return outstring;
}

//////////////////////////////////////////////////////////////////////////////

void
ScspSlotDebugStats(u8 slotnum, char * outstring)
{
    u32 slotoffset = slotnum * 0x20;

    AddString(outstring, "Sound Source = ");
    switch (scsp.slot[slotnum].ssctl)
    {
        case 0:
            AddString(outstring, "External DRAM data\r\n");
            break;
        case 1:
            AddString(outstring, "Internal(Noise)\r\n");
            break;
        case 2:
            AddString(outstring, "Internal(0's)\r\n");
            break;
        default:
            AddString(outstring, "Invalid setting\r\n");
            break;
    }

    AddString(outstring, "Source bit = ");
    switch (scsp.slot[slotnum].sbctl)
    {
        case 0:
            AddString(outstring, "No bit reversal\r\n");
            break;
        case 1:
            AddString(outstring, "Reverse other bits\r\n");
            break;
        case 2:
            AddString(outstring, "Reverse sign bit\r\n");
            break;
        case 3:
            AddString(outstring, "Reverse sign and other bits\r\n");
            break;
    }

    // Loop Control
    AddString(outstring, "Loop Mode = ");
    switch (scsp.slot[slotnum].lpctl)
    {
        case 0:
            AddString(outstring, "Off\r\n");
            break;
        case 1:
            AddString(outstring, "Normal\r\n");
            break;
        case 2:
            AddString(outstring, "Reverse\r\n");
            break;
        case 3:
            AddString(outstring, "Alternating\r\n");
            break;
    }

    // PCM8B
    // NOTE: Need curly braces here, as AddString is a macro.
    if (scsp.slot[slotnum].pcm8b)
    {
        AddString(outstring, "8-bit samples\r\n");
    } else
    {
        AddString(outstring, "16-bit samples\r\n");
    }

    AddString(outstring, "Start Address = %05lX\r\n", (unsigned long)scsp.slot[slotnum].sa);
    AddString(outstring, "Loop Start Address = %04lX\r\n", (unsigned long)scsp.slot[slotnum].lsa >> SCSP_FREQ_LB);
    AddString(outstring, "Loop End Address = %04lX\r\n", (unsigned long)scsp.slot[slotnum].lea >> SCSP_FREQ_LB);
    AddString(outstring, "Decay 1 Rate = %ld\r\n", (unsigned long)scsp.slot[slotnum].dr);
    AddString(outstring, "Decay 2 Rate = %ld\r\n", (unsigned long)scsp.slot[slotnum].sr);
    if (scsp.slot[slotnum].eghold)
        AddString(outstring, "EG Hold Enabled\r\n");
    AddString(outstring, "Attack Rate = %ld\r\n", (unsigned long)scsp.slot[slotnum].ar);

    if (scsp.slot[slotnum].lslnk)
        AddString(outstring, "Loop Start Link Enabled\r\n");

    if (scsp.slot[slotnum].krs != 0)
        AddString(outstring, "Key rate scaling = %ld\r\n", (unsigned long)scsp.slot[slotnum].krs);

    AddString(outstring, "Decay Level = %d\r\n", (scsp_r_w(NULL, NULL, slotoffset + 0xA) >> 5) & 0x1F);
    AddString(outstring, "Release Rate = %ld\r\n", (unsigned long)scsp.slot[slotnum].rr);

    if (scsp.slot[slotnum].swe)
        AddString(outstring, "Stack Write Inhibited\r\n");

    if (scsp.slot[slotnum].sdir)
        AddString(outstring, "Sound Direct Enabled\r\n");

    AddString(outstring, "Total Level = %ld\r\n", (unsigned long)scsp.slot[slotnum].tl);

    AddString(outstring, "Modulation Level = %d\r\n", scsp.slot[slotnum].mdl);
    AddString(outstring, "Modulation Input X = %d\r\n", scsp.slot[slotnum].mdx);
    AddString(outstring, "Modulation Input Y = %d\r\n", scsp.slot[slotnum].mdy);

    AddString(outstring, "Octave = %d\r\n", (scsp_r_w(NULL, NULL, slotoffset + 0x10) >> 11) & 0xF);
    AddString(outstring, "Frequency Number Switch = %d\r\n", scsp_r_w(NULL, NULL, slotoffset + 0x10) & 0x3FF);

    AddString(outstring, "LFO Reset = %s\r\n", ((scsp_r_w(NULL, NULL, slotoffset + 0x12) >> 15) & 0x1) ? "TRUE" : "FALSE");
    AddString(outstring, "LFO Frequency = %d\r\n", (scsp_r_w(NULL, NULL, slotoffset + 0x12) >> 10) & 0x1F);
    outstring = AddSoundLFO(outstring, "LFO Frequency modulation waveform = ",
                            (scsp_r_w(NULL, NULL, slotoffset + 0x12) >> 5) & 0x7,
                            (scsp_r_w(NULL, NULL, slotoffset + 0x12) >> 8) & 0x3);
    AddString(outstring, "LFO Frequency modulation level = %d\r\n", (scsp_r_w(NULL, NULL, slotoffset + 0x12) >> 5) & 0x7);
    outstring = AddSoundLFO(outstring, "LFO Amplitude modulation waveform = ",
                            scsp_r_w(NULL, NULL, slotoffset + 0x12) & 0x7,
                            (scsp_r_w(NULL, NULL, slotoffset + 0x12) >> 3) & 0x3);
    AddString(outstring, "LFO Amplitude modulation level = %d\r\n", scsp_r_w(NULL, NULL, slotoffset + 0x12) & 0x7);

    AddString(outstring, "Input mix level = ");
    outstring = AddSoundLevel(outstring, scsp_r_w(NULL, NULL, slotoffset + 0x14) & 0x7);
    AddString(outstring, "Input Select = %d\r\n", (scsp_r_w(NULL, NULL, slotoffset + 0x14) >> 3) & 0x1F);

    AddString(outstring, "Direct data send level = ");
    outstring = AddSoundLevel(outstring, (scsp_r_w(NULL, NULL, slotoffset + 0x16) >> 13) & 0x7);
    AddString(outstring, "Direct data panpot = ");
    outstring = AddSoundPan(outstring, (scsp_r_w(NULL, NULL, slotoffset + 0x16) >> 8) & 0x1F);

    AddString(outstring, "Effect data send level = ");
    outstring = AddSoundLevel(outstring, (scsp_r_w(NULL, NULL, slotoffset + 0x16) >> 5) & 0x7);
    AddString(outstring, "Effect data panpot = ");
    outstring = AddSoundPan(outstring, scsp_r_w(NULL, NULL, slotoffset + 0x16) & 0x1F);
}

//////////////////////////////////////////////////////////////////////////////

void
ScspCommonControlRegisterDebugStats(char * outstring)
{
    AddString(outstring, "Memory: %s\r\n", scsp.mem4b ? "4 Mbit" : "2 Mbit");
    AddString(outstring, "Master volume: %ld\r\n", (unsigned long)scsp.mvol);
    AddString(outstring, "Ring buffer length: %ld\r\n", (unsigned long)scsp.rbl);
    AddString(outstring, "Ring buffer address: %08lX\r\n", (unsigned long)scsp.rbp);
    AddString(outstring, "\r\n");

    AddString(outstring, "Slot Status Registers\r\n");
    AddString(outstring, "-----------------\r\n");
    AddString(outstring, "Monitor slot: %ld\r\n", (unsigned long)scsp.mslc);
    AddString(outstring, "Call address: %ld\r\n", (unsigned long)scsp.ca);
    AddString(outstring, "\r\n");

    AddString(outstring, "DMA Registers\r\n");
    AddString(outstring, "-----------------\r\n");
    AddString(outstring, "DMA memory address start: %08lX\r\n", (unsigned long)scsp.dmea);
    AddString(outstring, "DMA register address start: %08lX\r\n", (unsigned long)scsp.drga);
    AddString(outstring, "DMA Flags: %lX\r\n", (unsigned long)scsp.dmlen);
    AddString(outstring, "\r\n");

    AddString(outstring, "Timer Registers\r\n");
    AddString(outstring, "-----------------\r\n");
    AddString(outstring, "Timer A counter: %02lX\r\n", (unsigned long)scsp.timacnt >> 8);
    AddString(outstring, "Timer A increment: Every %d sample(s)\r\n", (int)pow(2, (double)scsp.timasd));
    AddString(outstring, "Timer B counter: %02lX\r\n", (unsigned long)scsp.timbcnt >> 8);
    AddString(outstring, "Timer B increment: Every %d sample(s)\r\n", (int)pow(2, (double)scsp.timbsd));
    AddString(outstring, "Timer C counter: %02lX\r\n", (unsigned long)scsp.timccnt >> 8);
    AddString(outstring, "Timer C increment: Every %d sample(s)\r\n", (int)pow(2, (double)scsp.timcsd));
    AddString(outstring, "\r\n");

    AddString(outstring, "Interrupt Registers\r\n");
    AddString(outstring, "-----------------\r\n");
    AddString(outstring, "Sound cpu interrupt pending: %04lX\r\n", (unsigned long)scsp.scipd);
    AddString(outstring, "Sound cpu interrupt enable: %04lX\r\n", (unsigned long)scsp.scieb);
    AddString(outstring, "Sound cpu interrupt level 0: %04lX\r\n", (unsigned long)scsp.scilv0);
    AddString(outstring, "Sound cpu interrupt level 1: %04lX\r\n", (unsigned long)scsp.scilv1);
    AddString(outstring, "Sound cpu interrupt level 2: %04lX\r\n", (unsigned long)scsp.scilv2);
    AddString(outstring, "Main cpu interrupt pending: %04lX\r\n", (unsigned long)scsp.mcipd);
    AddString(outstring, "Main cpu interrupt enable: %04lX\r\n", (unsigned long)scsp.mcieb);
    AddString(outstring, "\r\n");
}

//////////////////////////////////////////////////////////////////////////////

int
ScspSlotDebugSaveRegisters(u8 slotnum, const char * filename)
{
    FILE * fp;
    int i;
    IOCheck_struct check = {0, 0};

    if ((fp = fopen(filename, "wb")) == NULL)
        return -1;

    for (i = (slotnum * 0x20); i < ((slotnum + 1) * 0x20); i += 2)
    {
#ifdef WORDS_BIGENDIAN
      ywrite (&check, (void *)&scsp_isr[i ^ 2], 1, 2, fp);
#else
        ywrite(&check, (void *)&scsp_isr[(i + 1) ^ 2], 1, 1, fp);
        ywrite(&check, (void *)&scsp_isr[i ^ 2], 1, 1, fp);
#endif
    }

    fclose(fp);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static slot_t debugslot;

u32
ScspSlotDebugAudio(u32 * workbuf, s16 * buf, u32 len)
{
    u32 * bufL, * bufR;

    bufL = workbuf;
    bufR = workbuf + len;
    scsp_bufL = (s32 *)bufL;
    scsp_bufR = (s32 *)bufR;

    if (debugslot.ecnt >= SCSP_ENV_DE)
    {
        // envelope null...
        memset(buf, 0, sizeof(s16) * 2 * len);
        return 0;
    }

    if (debugslot.ssctl)
    {
        memset(buf, 0, sizeof(s16) * 2 * len);
        return 0; // not yet supported!
    }

    scsp_buf_len = len;
    scsp_buf_pos = 0;

    // take effect sound volume if no direct sound volume...
    if ((debugslot.disll == 31) && (debugslot.dislr == 31))
    {
        debugslot.disll = debugslot.efsll;
        debugslot.dislr = debugslot.efslr;
    }

    memset(bufL, 0, sizeof(u32) * len);
    memset(bufR, 0, sizeof(u32) * len);
    scsp_slot_update_p[(debugslot.lfofms == 31) ? 0 : 1]
        [(debugslot.lfoems == 31) ? 0 : 1]
        [(debugslot.pcm8b == 0) ? 1 : 0]
        [(debugslot.disll == 31) ? 0 : 1]
        [(debugslot.dislr == 31) ? 0 : 1](&debugslot);
    ScspConvert32uto16s((s32 *)bufL, (s32 *)bufR, (s16 *)buf, len);

    return len;
}

//////////////////////////////////////////////////////////////////////////////

typedef struct
{
    char id[4];
    u32 size;
} chunk_struct;

typedef struct
{
    chunk_struct riff;
    char rifftype[4];
} waveheader_struct;

typedef struct
{
    chunk_struct chunk;
    u16 compress;
    u16 numchan;
    u32 rate;
    u32 bytespersec;
    u16 blockalign;
    u16 bitspersample;
} fmt_struct;

//////////////////////////////////////////////////////////////////////////////

void
ScspSlotResetDebug(u8 slotnum)
{
    memcpy(&debugslot, &scsp.slot[slotnum], sizeof(slot_t));

    // Clear out the phase counter, etc.
    debugslot.fcnt = 0;
    debugslot.ecnt = SCSP_ENV_AS;
    debugslot.einc = &debugslot.einca;
    debugslot.ecmp = SCSP_ENV_AE;
    debugslot.ecurp = SCSP_ENV_ATTACK;
    debugslot.enxt = scsp_attack_next;
}

//////////////////////////////////////////////////////////////////////////////

int
ScspSlotDebugAudioSaveWav(u8 slotnum, const char * filename)
{
    u32 workbuf[512 * 2 * 2];
    s16 buf[512 * 2];
    FILE * fp;
    u32 counter = 0;
    waveheader_struct waveheader;
    fmt_struct fmt;
    chunk_struct data;
    long length;
    IOCheck_struct check = {0, 0};

    if (scsp.slot[slotnum].lea == 0)
        return 0;

    if ((fp = fopen(filename, "wb")) == NULL)
        return -1;

    // Do wave header
    memcpy(waveheader.riff.id, "RIFF", 4);
    waveheader.riff.size = 0; // we'll fix this after the file is closed
    memcpy(waveheader.rifftype, "WAVE", 4);
    ywrite(&check, (void *)&waveheader, 1, sizeof(waveheader_struct), fp);

    // fmt chunk
    memcpy(fmt.chunk.id, "fmt ", 4);
    fmt.chunk.size = 16; // we'll fix this at the end
    fmt.compress = 1; // PCM
    fmt.numchan = 2; // Stereo
    fmt.rate = 44100;
    fmt.bitspersample = 16;
    fmt.blockalign = fmt.bitspersample / 8 * fmt.numchan;
    fmt.bytespersec = fmt.rate * fmt.blockalign;
    ywrite(&check, (void *)&fmt, 1, sizeof(fmt_struct), fp);

    // data chunk
    memcpy(data.id, "data", 4);
    data.size = 0; // we'll fix this at the end
    ywrite(&check, (void *)&data, 1, sizeof(chunk_struct), fp);

    ScspSlotResetDebug(slotnum);

    // Mix the audio, and then write it to the file
    for (;;)
    {
        if (ScspSlotDebugAudio(workbuf, buf, 512) == 0)
            break;

        counter += 512;
        ywrite(&check, (void *)buf, 2, 512 * 2, fp);
        if (debugslot.lpctl != 0 && counter >= (44100 * 2 * 5))
            break;
    }

    length = ftell(fp);

    // Let's fix the riff chunk size and the data chunk size
    fseek(fp, sizeof(waveheader_struct) - 0x8, SEEK_SET);
    length -= 0x4;
    ywrite(&check, (void *)&length, 1, 4, fp);

    fseek(fp, sizeof(waveheader_struct) + sizeof(fmt_struct) + 0x4, SEEK_SET);
    length -= sizeof(waveheader_struct) + sizeof(fmt_struct);
    ywrite(&check, (void *)&length, 1, 4, fp);
    fclose(fp);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
