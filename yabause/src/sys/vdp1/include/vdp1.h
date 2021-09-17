/*  Copyright 2003-2005 Guillaume Duhamel
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

#ifndef VDP1_H
#define VDP1_H

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VIDCORE_DEFAULT         -1
#define Vdp1CommandBufferSize 5000

//#define YAB_ASYNC_RENDERING 1

typedef struct {
   u16 TVMR;
   u16 FBCR;
   u16 PTMR;
   u16 EWDR;
   u16 EWLR;
   u16 EWRR;
   u16 ENDR;
   u16 EDSR;
   u16 LOPR;
   u16 COPR;
   u16 MODR;

   u16 lCOPR;

   u32 addr;

   s16 localX;
   s16 localY;

   u16 systemclipX1;
   u16 systemclipY1;
   u16 systemclipX2;
   u16 systemclipY2;

   u16 userclipX1;
   u16 userclipY1;
   u16 userclipX2;
   u16 userclipY2;
} Vdp1;

typedef struct
{
  float G[16];
  u32 priority;
  u32 w;
  u32 h;
  u32 flip;
  u32 type;
  //FIX me those data fields are all 16 bit not 32 accordion to documentation. signed or unsigned has been checked
  //NOTE pls keep in mind that for now we copy the struct memory via mem copy directly over to the GPU memory where the memory has the same layout as this structure. meaning they have to match in memory layout.
  //this is why we can not use 16 bit size filed for now.
  union {
      struct {
          u32 Comm : 4; // Command Select
          u32 Dir : 2; // Character Read Direction
          u32 unused : 2; // unsued
          u32 ZP : 4; // Zoom Point
          u32 JP : 3; // Jump Select
          u32 END : 1; // End Bit
      } part;
      u32 all : 32;
  } CMDCTRL; // Control Words (i.e. Command Select)
  u32 CMDLINK; // Link Specification
  u32 CMDPMOD; // Draw Mode Word
  u32 CMDCOLR; // Color Control Word
  u32 CMDSRCA; // Character Address
  u32 CMDSIZE; // Character Size
  s32 CMDXA; // Vertex Coordinate Data (signed)
  s32 CMDYA; // Vertex Coordinate Data (signed)
  s32 CMDXB; // Vertex Coordinate Data (signed)
  s32 CMDYB; // Vertex Coordinate Data (signed)
  s32 CMDXC; // Vertex Coordinate Data (signed)
  s32 CMDYC; // Vertex Coordinate Data (signed)
  s32 CMDXD; // Vertex Coordinate Data (signed)
  s32 CMDYD; // Vertex Coordinate Data (signed)
  //END FIX me
  s32 B[4];
  u32 COLOR[4];
  u32 CMDGRDA; //Gouraud Shading Table   //FIX me 16bit too
  u32 SPCTL;
  u32 nbStep;
  float uAstepx;
  float uAstepy;
  float uBstepx;
  float uBstepy;
  u32 pad[2];
} vdp1cmd_struct;

typedef struct {
    vdp1cmd_struct cmd;
    int ignitionLine;
    int completionLine;
    u32 start_addr;
    u32 end_addr;
    int dirty;
} vdp1cmdctrl_struct;

extern u8 * Vdp1Ram;

u8 FASTCALL	Vdp1RamReadByte(SH2_struct *context, u8*, u32);
u16 FASTCALL	Vdp1RamReadWord(SH2_struct *context, u8*, u32);
u32 FASTCALL	Vdp1RamReadLong(SH2_struct *context, u8*, u32);
void FASTCALL	Vdp1RamWriteByte(SH2_struct *context, u8*, u32, u8);
void FASTCALL	Vdp1RamWriteWord(SH2_struct *context, u8*, u32, u16);
void FASTCALL	Vdp1RamWriteLong(SH2_struct *context, u8*, u32, u32);
u8 FASTCALL Vdp1FrameBufferReadByte(SH2_struct *context, u8*, u32);
u16 FASTCALL Vdp1FrameBufferReadWord(SH2_struct *context, u8*, u32);
u32 FASTCALL Vdp1FrameBufferReadLong(SH2_struct *context, u8*, u32);
void FASTCALL Vdp1FrameBufferWriteByte(SH2_struct *context, u8*, u32, u8);
void FASTCALL Vdp1FrameBufferWriteWord(SH2_struct *context, u8*, u32, u16);
void FASTCALL Vdp1FrameBufferWriteLong(SH2_struct *context, u8*, u32, u32);

void Vdp1DrawCommands(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void Vdp1FakeDrawCommands(u8 * ram, Vdp1 * regs);

extern Vdp1 * Vdp1Regs;

enum VDP1STATUS {
  VDP1_STATUS_IDLE = 0,
  VDP1_STATUS_RUNNING
};

int Vdp1Init(void);
void Vdp1DeInit(void);
int VideoInit(int coreid);
int VideoChangeCore(int coreid);
void VideoDeInit(void);
void Vdp1Reset(void);
int VideoSetSetting(int type, int value);

u8 FASTCALL	Vdp1ReadByte(SH2_struct *context, u8*, u32);
u16 FASTCALL	Vdp1ReadWord(SH2_struct *context, u8*, u32);
u32 FASTCALL	Vdp1ReadLong(SH2_struct *context, u8*, u32);
void FASTCALL	Vdp1WriteByte(SH2_struct *context, u8*, u32, u8);
void FASTCALL	Vdp1WriteWord(SH2_struct *context, u8*, u32, u16);
void FASTCALL	Vdp1WriteLong(SH2_struct *context, u8*, u32, u32);

int Vdp1SaveState(void ** stream);
int Vdp1LoadState(const void * stream, int version, int size);

void Vdp1HBlankIN(void);
void Vdp1HBlankOUT(void);
void Vdp1VBlankIN(void);
void Vdp1VBlankOUT(void);

#ifdef __cplusplus
}
#endif

#endif
