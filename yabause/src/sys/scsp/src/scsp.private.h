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

#ifndef SCSP_PRIVATE_H
#define SCSP_PRIVATE_H

#include "core.h"
#include "scsp.debug.h"
#include "sh2core.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

#define SCSP_FREQ         44100                                     // SCSP frequency

#define SCSP_RAM_SIZE     0x080000                                  // SCSP RAM size
#define SCSP_RAM_MASK     (SCSP_RAM_SIZE - 1)

#define SCSP_MIDI_IN_EMP  0x01                                      // MIDI flags
#define SCSP_MIDI_IN_FUL  0x02
#define SCSP_MIDI_IN_OVF  0x04
#define SCSP_MIDI_OUT_EMP 0x08
#define SCSP_MIDI_OUT_FUL 0x10

#define SCSP_ENV_RELEASE  3                                         // Envelope phase
#define SCSP_ENV_SUSTAIN  2
#define SCSP_ENV_DECAY    1
#define SCSP_ENV_ATTACK   0

#define SCSP_FREQ_HB      19                                        // Freq counter int part
#define SCSP_FREQ_LB      10                                        // Freq counter float part

#define SCSP_ENV_HB       10                                        // Env counter int part
#define SCSP_ENV_LB       10                                        // Env counter float part

#define SCSP_LFO_HB       10                                        // LFO counter int part
#define SCSP_LFO_LB       10                                        // LFO counter float part

#define SCSP_ENV_LEN      (1 << SCSP_ENV_HB)                        // Env table len
#define SCSP_ENV_MASK     (SCSP_ENV_LEN - 1)                        // Env table mask

#define SCSP_FREQ_LEN     (1 << SCSP_FREQ_HB)                       // Freq table len
#define SCSP_FREQ_MASK    (SCSP_FREQ_LEN - 1)                       // Freq table mask

#define SCSP_LFO_LEN      (1 << SCSP_LFO_HB)                        // LFO table len
#define SCSP_LFO_MASK     (SCSP_LFO_LEN - 1)                        // LFO table mask

#define SCSP_ENV_AS       0                                         // Env Attack Start
#define SCSP_ENV_DS       (SCSP_ENV_LEN << SCSP_ENV_LB)             // Env Decay Start
#define SCSP_ENV_AE       (SCSP_ENV_DS - 1)                         // Env Attack End
#define SCSP_ENV_DE       (((2 * SCSP_ENV_LEN) << SCSP_ENV_LB) - 1) // Env Decay End

#define SCSP_ATTACK_R     (u32) (8 * 44100)
#define SCSP_DECAY_R      (u32) (12 * SCSP_ATTACK_R)

typedef struct
{
  u32 scsptiming1;
  u32 scsptiming2;  // 16.16 fixed point
  m68kcodebreakpoint_struct codebreakpoint[MAX_BREAKPOINTS];
  int numcodebreakpoints;
  void (*BreakpointCallBack)(u32);
  int inbreakpoint;
} ScspInternal;

//unknown stored bits are unknown1-5
typedef struct SlotRegs
{
    u8 kx;
    u8 kb;
    u8 sbctl;
    u8 ssctl;
    u8 lpctl;
    u8 pcm8b;
    u32 sa;
    u16 lsa;
    u16 lea;
    u8 d2r;
    u8 d1r;
    u8 hold;
    u8 ar;
    u8 unknown1;
    u8 ls;
    u8 krs;
    u8 dl;
    u8 rr;
    u8 unknown2;
    u8 si;
    u8 sd;
    u16 tl;
    u8 mdl;
    u8 mdxsl;
    u8 mdysl;
    u8 unknown3;
    u8 oct;
    u8 unknown4;
    u16 fns;
    u8 re;
    u8 lfof;
    u8 plfows;
    u8 plfos;
    u8 alfows;
    u8 alfos;
    u8 unknown5;
    u8 isel;
    u8 imxl;
    u8 disdl;
    u8 dipan;
    u8 efsdl;
    u8 efpan;
} SlotRegs;

typedef enum EnvelopeStates
{
    ATTACK = 1,
    DECAY1,
    DECAY2,
    RELEASE
}EnvelopeStates;

typedef struct SlotState
{
    u16 wave;
    int backwards;
    EnvelopeStates envelope;
    s16 output;
    u16 attenuation;
    int step_count;
    u32 sample_counter;
    u32 envelope_steps_taken;
    s32 waveform_phase_value;
    s32 sample_offset;
    u32 address_pointer;
    u32 lfo_counter;
    u32 lfo_pos;

    int num;
    int is_muted;
}SlotState;

typedef struct Slot
{
    //registers
    SlotRegs regs;

    //internal state
    SlotState state;
}Slot;

typedef struct Scsp
{
    Slot slots[32];
    u16 sound_stack[64];
    int debug_mode;
}Scsp;

typedef struct slot_t
{
    u8 swe;      // stack write enable
    u8 sdir;     // sound direct
    u8 pcm8b;    // PCM sound format

    u8 sbctl;    // source bit control
    u8 ssctl;    // sound source control
    u8 lpctl;    // loop control

    u8 key;      // KEY_ state
    u8 keyx;     // still playing regardless the KEY_ state (hold, decay)

    s8* buf8;    // sample buffer 8 bits
    s16* buf16;  // sample buffer 16 bits

    u32 fcnt;    // phase counter
    u32 finc;    // phase step adder
    u32 finct;   // non adjusted phase step

    s32 ecnt;    // envelope counter
    s32* einc;    // envelope current step adder
    s32 einca;   // envelope step adder for attack
    s32 eincd;   // envelope step adder for decay 1
    s32 eincs;   // envelope step adder for decay 2
    s32 eincr;   // envelope step adder for release
    s32 ecmp;    // envelope compare to raise next phase
    u32 ecurp;   // envelope current phase (attack / decay / release ...)
    s32 env;     // envelope multiplier (at time of last update)

    void (*enxt)(struct slot_t*);  // envelope function pointer for next phase event

    u32 lfocnt;   // lfo counter
    s32 lfoinc;   // lfo step adder

    u32 sa;       // start address
    u32 lsa;      // loop start address
    u32 lea;      // loop end address

    s32 tl;       // total level
    s32 sl;       // sustain level

    s32 ar;       // attack rate
    s32 dr;       // decay rate
    s32 sr;       // sustain rate
    s32 rr;       // release rate

    s32* arp;     // attack rate table pointer
    s32* drp;     // decay rate table pointer
    s32* srp;     // sustain rate table pointer
    s32* rrp;     // release rate table pointer

    u32 krs;      // key rate scale

    s32* lfofmw;  // lfo frequency modulation waveform pointer
    s32* lfoemw;  // lfo envelope modulation waveform pointer
    u8 lfofms;    // lfo frequency modulation sensitivity
    u8 lfoems;    // lfo envelope modulation sensitivity
    u8 fsft;      // frequency shift (used for freq lfo)

    u8 mdl;       // modulation level
    u8 mdx;       // modulation source X
    u8 mdy;       // modulation source Y

    u8 imxl;      // input sound level
    u8 disll;     // direct sound level left
    u8 dislr;     // direct sound level right
    u8 efsll;     // effect sound level left
    u8 efslr;     // effect sound level right

    u8 eghold;    // eg type envelope hold
    u8 lslnk;     // loop start link (start D1R when start loop adr is reached)

    // NOTE: Previously there were u8 pads here to maintain 4-byte alignment.
    //       There are current 22 u8's in this struct and 1 u16. This makes 24
    //       bytes, so there are no pads at the moment.
    //
    //       I'm not sure this is at all necessary either, but keeping this note
    //       in case.
} slot_t;

typedef struct scsp_t
{
    u32 mem4b;            // 4mbit memory
    u32 mvol;             // master volume

    u32 rbl;              // ring buffer lenght
    u32 rbp;              // ring buffer address (pointer)

    u32 mslc;             // monitor slot
    u32 ca;               // call address
    u32 sgc;              // phase
    u32 eg;               // envelope

    u32 dmea;             // dma memory address start
    u32 drga;             // dma register address start
    u32 dmfl;             // dma flags (direction / gate 0 ...)
    u32 dmlen;            // dma transfer len

    u8 midinbuf[4];       // midi in buffer
    u8 midoutbuf[4];      // midi out buffer
    u8 midincnt;          // midi in buffer size
    u8 midoutcnt;         // midi out buffer size
    u8 midflag;           // midi flag (empty, full, overflow ...)
    u8 midflag2;          // midi flag 2 (here only for alignement)

    s32 timacnt;          // timer A counter
    u32 timasd;           // timer A step diviser
    s32 timbcnt;          // timer B counter
    u32 timbsd;           // timer B step diviser
    s32 timccnt;          // timer C counter
    u32 timcsd;           // timer C step diviser

    u32 scieb;            // allow sound cpu interrupt
    u32 scipd;            // pending sound cpu interrupt

    u32 scilv0;           // IL0 M68000 interrupt pin state
    u32 scilv1;           // IL1 M68000 interrupt pin state
    u32 scilv2;           // IL2 M68000 interrupt pin state

    u32 mcieb;            // allow main cpu interrupt
    u32 mcipd;            // pending main cpu interrupt

    u8* scsp_ram;         // scsp ram pointer
    void (*mintf)(void);  // main cpu interupt function pointer
    void (*sintf)(u32);   // sound cpu interrupt function pointer

    s32 stack[32 * 2];    // two last generation slot output (SCSP STACK)
    slot_t slot[32];      // 32 slots
} scsp_t;

#endif
