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

/*Saturn Custom Sound Processor (SCSP): Also referred to as Yamaha YMF292, it�s composed of two modules:
A multi-function sound generator: Processes up to 32 channels with PCM samples (up to 16-bit with 44.1 kHz, a.k.a �CD quality�) or FM channels. In the case of the latter, a number of channels are reserved for operators.
A DSP: Applies effects like echo, reverb and chorus. The docs also mention �filters� but I don�t know if it means envelope or frequency filter (i.e. low pass, etc).
*/

#ifndef SCSP_H
#define SCSP_H

#include "core.h"
#include "sh2core.h"
#include "threads.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SNDCORE_DEFAULT -1
#define SNDCORE_DUMMY   0
#define SNDCORE_WAV     10 // should really be 1, but I'll probably break people's stuff

#define SCSP_MUTE_SYSTEM    1
#define SCSP_MUTE_USER      2

#define SCSP_FRACTIONAL_BITS 20

typedef struct
{
   int id;
   const char *Name;
   int (*Init)(void);
   void (*DeInit)(void);
   int (*Reset)(void);
   int (*ChangeVideoFormat)(int vertfreq);
   void (*UpdateAudio)(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples);
   u32 (*GetAudioSpace)(void);
   void (*MuteAudio)(void);
   void (*UnMuteAudio)(void);
   void (*SetVolume)(int volume);
#ifdef USE_SCSPMIDI
	int (*MidiChangePorts)(int inport, int outport);
	u8 (*MidiIn)(int *isdata);
	int (*MidiOut)(u8 data);
#endif
} SoundInterface_struct;

typedef struct
{
   u32 D[8];
   u32 A[8];
   u32 SR;
   u32 PC;
} m68kregs_struct;

//#if defined(ARCH_IS_LINUX)
#define ASYNC_SCSP
//#endif

extern const u16 scsp_frequency; 
extern const u16 scsp_samplecnt; // 11289600/44100

extern SoundInterface_struct SNDDummy;
extern SoundInterface_struct SNDWave;
extern u8 *SoundRam;

u8 FASTCALL SoundRamReadByte(u32 addr);
u16 FASTCALL SoundRamReadWord(u32 addr);
u32 FASTCALL SoundRamReadLong(u32 addr);
void FASTCALL SoundRamWriteByte(u32 addr, u8 val);
void FASTCALL SoundRamWriteWord(u32 addr, u16 val);
void FASTCALL SoundRamWriteLong(u32 addr, u32 val);

int ScspInit(int coreid);
int ScspChangeSoundCore(int coreid);
void ScspDeInit(void);
void M68KStart(void);
void M68KStop(void);
void ScspReset(void);
int ScspChangeVideoFormat(int type);
void M68KExec(s32 cycles);
void SignalScspSync();
void ScspRun(void);
void ScspConvert32uto16s(s32 *srcL, s32 *srcR, s16 *dst, u32 len);
void ScspReceiveCDDA(const u8 *sector);
int SoundSaveState(void ** stream);
int SoundLoadState(const void * stream, int version, int size);
void ScspMuteAudio(int flags);
void ScspUnMuteAudio(int flags);
void ScspSetVolume(int volume);
void ScspAsynMain(void * p);
void FASTCALL scsp_w_b(SH2_struct *context, u8*, u32, u8);
void FASTCALL scsp_w_w(SH2_struct *context, u8*, u32, u16);
void FASTCALL scsp_w_d(SH2_struct *context, u8*, u32, u32);
u8 FASTCALL scsp_r_b(SH2_struct *context, u8*, u32);
u16 FASTCALL scsp_r_w(SH2_struct *context, u8*, u32);
u32 FASTCALL scsp_r_d(SH2_struct *context, u8*, u32);

void scsp_init(u8 *scsp_ram, void (*sint_hand)(u32), void (*mint_hand)(void));
void scsp_shutdown(void);
void scsp_reset(void);

void scsp_midi_in_send(u8 data);
void scsp_midi_out_send(u8 data);
u8 scsp_midi_in_read(void);
u8 scsp_midi_out_read(void);
void scsp_update(s32 *bufL, s32 *bufR, u32 len);
void scsp_update_monitor(void);
void scsp_update_timer(u32 len);

u32 FASTCALL c68k_word_read(const u32 adr);

void M68KSync(void);
void M68KWriteNotify(u32 address, u32 size);
void M68KGetRegisters(m68kregs_struct *regs);
void M68KSetRegisters(m68kregs_struct *regs);

void new_scsp_exec(s32 cycles);

void SyncScsp();

extern void ScspLockThread();
extern void ScspUnLockThread();

void setM68kCounter(u64 counter);

INLINE u32 get_cycles_per_line_division(u32 clock, int frames, int lines, int divisions_per_line)
{
    return ((u64)(clock / frames) << SCSP_FRACTIONAL_BITS) / (lines * divisions_per_line);
}

#ifdef __cplusplus
}
#endif

#endif
