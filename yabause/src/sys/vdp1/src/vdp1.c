/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2004 Lawrence Sebald
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

/*! \file vdp1.c
    \brief VDP1 emulation functions.
    https://antime.kapsi.fi/sega/files/ST-013-R3-061694.pdf
*/


#include <stdlib.h>
#include <math.h>
#include "yabause.h"
#include "vdp1.h"
#include "debug.h"
#include "scu.h"
#include "vidsoft.h"
#include "threads.h"
#include "sh2core.h"
#include "videoInterface.h"
#include "vdp1.private.h"
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
#include "ygl.h"
#endif

u8 * Vdp1Ram = NULL;
u32 vdp1Ram_update_start;
u32 vdp1Ram_update_end;
int VDP1_MASK = 0xFFFF;

Vdp1 * Vdp1Regs;
Vdp1External_struct Vdp1External;

vdp1cmdctrl_struct cmdBufferBeingProcessed[Vdp1CommandBufferSize];
int nbCmdToProcess = 0;
int vdp1_clock = 0;

static u32 CmdListDrawn = 0;
static u32 CmdListLimit = 0x80000;


static int needVdp1draw = 0;
static void Vdp1NoDraw(void);
static int Vdp1Draw(void);
void FASTCALL Vdp1ReadCommand(vdp1cmd_struct *cmd, u32 addr, u8* ram);

#define DEBUG_BAD_COORD //YuiMsg

#define  CONVERTCMD(A) {\
  s32 toto = (A);\
  if (((A)&0x7000) != 0) (A) |= 0xF000;\
  else (A) &= ~0xF800;\
  ((A) = (s32)(s16)(A));\
  if (((A)) < -1024) { DEBUG_BAD_COORD("Bad(-1024) %x (%d, 0x%x)\n", (A), (A), toto);}\
  if (((A)) > 1023) { DEBUG_BAD_COORD("Bad(1023) %x (%d, 0x%x)\n", (A), (A), toto);}\
}

static void abortVdp1() {
  if (Vdp1External.status == VDP1_STATUS_RUNNING) {
    // The vdp1 is still running and a new draw command request has been received
    // Abort the current command list
    Vdp1External.status = VDP1_STATUS_IDLE;
    vdp1_clock = 0;
  }
}
//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp1RamReadByte(SH2_struct *context, u8* mem, u32 addr) {
  // if (context != NULL){
  //   context->cycles += 50;
  // }
   addr &= 0x7FFFF;
   return T1ReadByte(mem, addr);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp1RamReadWord(SH2_struct *context, u8* mem, u32 addr) {
  // if (context != NULL){
  //   context->cycles += 50;
  // }
    addr &= 0x07FFFF;
    return T1ReadWord(mem, addr);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp1RamReadLong(SH2_struct *context, u8* mem, u32 addr) {
  // if (context != NULL){
  //   context->cycles += 50;
  // }
   addr &= 0x7FFFF;
   return T1ReadLong(mem, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1RamWriteByte(SH2_struct *context, u8* mem, u32 addr, u8 val) {
  // if (context != NULL){
  //   context->cycles += 2;
  // }
   addr &= 0x7FFFF;
   if (CmdListLimit >= addr) {
     CmdListDrawn = 0;
   }
   Vdp1External.updateVdp1Ram = 1;
   if( Vdp1External.status == VDP1_STATUS_RUNNING) vdp1_clock -= 1;
   if (vdp1Ram_update_start > addr) vdp1Ram_update_start = addr;
   if (vdp1Ram_update_end < addr+1) vdp1Ram_update_end = addr + 1;
   T1WriteByte(mem, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1RamWriteWord(SH2_struct *context, u8* mem, u32 addr, u16 val) {
  // if (context != NULL){
  //   context->cycles += 2;
  // }
   addr &= 0x7FFFF;
   if (CmdListLimit >= addr) {
     CmdListDrawn = 0;
   }
   Vdp1External.updateVdp1Ram = 1;
   if( Vdp1External.status == VDP1_STATUS_RUNNING) vdp1_clock -= 2;
   if (vdp1Ram_update_start > addr) vdp1Ram_update_start = addr;
   if (vdp1Ram_update_end < addr+2) vdp1Ram_update_end = addr + 2;
   T1WriteWord(mem, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1RamWriteLong(SH2_struct *context, u8* mem, u32 addr, u32 val) {
  // if (context != NULL){
  //   context->cycles += 2;
  // }
   addr &= 0x7FFFF;
   if (CmdListLimit >= addr) {
     CmdListDrawn = 0;
   }
   Vdp1External.updateVdp1Ram = 1;
   if( Vdp1External.status == VDP1_STATUS_RUNNING) vdp1_clock -= 4;
   if (vdp1Ram_update_start > addr) vdp1Ram_update_start = addr;
   if (vdp1Ram_update_end < addr+4) vdp1Ram_update_end = addr + 4;
   T1WriteLong(mem, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp1FrameBufferReadByte(SH2_struct *context, u8* mem, u32 addr) {
   addr &= 0x3FFFF;
   if (VIDCore->Vdp1ReadFrameBuffer){
     u8 val;
     VIDCore->Vdp1ReadFrameBuffer(0, addr, &val);
     return val;
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp1FrameBufferReadWord(SH2_struct *context, u8* mem, u32 addr) {
   addr &= 0x3FFFF;
   if (VIDCore->Vdp1ReadFrameBuffer){
     u16 val;
     VIDCore->Vdp1ReadFrameBuffer(1, addr, &val);
     return val;
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp1FrameBufferReadLong(SH2_struct *context, u8* mem, u32 addr) {
   addr &= 0x3FFFF;
   if (VIDCore->Vdp1ReadFrameBuffer){
     u32 val;
     VIDCore->Vdp1ReadFrameBuffer(2, addr, &val);
     return val;
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1FrameBufferWriteByte(SH2_struct *context, u8* mem, u32 addr, u8 val) {
   addr &= 0x7FFFF;

   if (VIDCore->Vdp1WriteFrameBuffer)
   {
      if (addr < 0x40000) VIDCore->Vdp1WriteFrameBuffer(0, addr, val);
      return;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1FrameBufferWriteWord(SH2_struct *context, u8* mem, u32 addr, u16 val) {
  addr &= 0x7FFFF;

   if (VIDCore->Vdp1WriteFrameBuffer)
   {
      if (addr < 0x40000) VIDCore->Vdp1WriteFrameBuffer(1, addr, val);
      return;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1FrameBufferWriteLong(SH2_struct *context, u8* mem, u32 addr, u32 val) {
  addr &= 0x7FFFF;

   if (VIDCore->Vdp1WriteFrameBuffer)
   {
     if (addr < 0x40000) VIDCore->Vdp1WriteFrameBuffer(2, addr, val);
     return;
   }
}

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

int Vdp1Init(void) {
   if ((Vdp1Regs = (Vdp1 *) malloc(sizeof(Vdp1))) == NULL)
      return -1;

   if ((Vdp1Ram = T1MemoryInit(0x80000)) == NULL)
      return -1;

   Vdp1External.disptoggle = 1;

   Vdp1Regs->TVMR = 0;
   Vdp1Regs->FBCR = 0;
   Vdp1Regs->PTMR = 0;

   Vdp1Regs->userclipX1=0;
   Vdp1Regs->userclipY1=0;
   Vdp1Regs->userclipX2=1024;
   Vdp1Regs->userclipY2=512;

   Vdp1Regs->localX=0;
   Vdp1Regs->localY=0;

   VDP1_MASK = 0xFFFF;

   vdp1Ram_update_start = 0x80000;
   vdp1Ram_update_end = 0x0;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1DeInit(void) {
   if (Vdp1Regs)
      free(Vdp1Regs);
   Vdp1Regs = NULL;

   if (Vdp1Ram)
      T1MemoryDeInit(Vdp1Ram);
   Vdp1Ram = NULL;

}

//////////////////////////////////////////////////////////////////////////////

int VideoInit(int coreid) {
   return VideoChangeCore(coreid);
}

//////////////////////////////////////////////////////////////////////////////

int VideoChangeCore(int coreid)
{
   int i;

   // Make sure the old core is freed
   VideoDeInit();

   // So which core do we want?
   if (coreid == VIDCORE_DEFAULT)
      coreid = 0; // Assume we want the first one

   VIDCore = VIDCoreList[0]; //always ensure at least we tried the first core if the search fails to get the core.

   // Go through core list and find the id
   for (i = 0; VIDCoreList[i] != NULL; i++)
   {
      if (VIDCoreList[i]->id == coreid)
      {
         // Set to current core
         VIDCore = VIDCoreList[i];
         break;
      }
   }

   if (VIDCore == NULL)
      return -1;

   if (VIDCore->Init() != 0)
      return -1;

   // Reset resolution/priority variables
   if (Vdp2Regs)
   {
      VIDCore->Vdp1Reset();
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VideoDeInit(void) {
   if (VIDCore)
      VIDCore->DeInit();
   VIDCore = NULL;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1Reset(void) {
   Vdp1Regs->PTMR = 0;
   Vdp1Regs->MODR = 0x1000; // VDP1 Version 1
   Vdp1Regs->TVMR = 0;
   Vdp1Regs->EWDR = 0;
   Vdp1Regs->EWLR = 0;
   Vdp1Regs->EWRR = 0;
   Vdp1Regs->ENDR = 0;
   VDP1_MASK = 0xFFFF;
   VIDCore->Vdp1Reset();
   vdp1_clock = 0;
}

int VideoSetSetting( int type, int value )
{
	if (VIDCore) VIDCore->SetSettingValue( type, value );
	return 0;
}


//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp1ReadByte(SH2_struct *context, u8* mem, u32 addr) {
   addr &= 0xFF;
   LOG("trying to byte-read a Vdp1 register\n");
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
u16 FASTCALL Vdp1ReadWord(SH2_struct *context, u8* mem, u32 addr) {
   addr &= 0xFF;
   switch(addr) {
      case 0x10:
        FRAMELOG("Read EDSR %X line = %d\n", Vdp1Regs->EDSR, yabsys.LineCount);
        if (Vdp1External.checkEDSR == 0) {
          if (VIDCore != NULL)
            if (VIDCore->FinsihDraw != NULL)
              VIDCore->FinsihDraw();
        }
        Vdp1External.checkEDSR = 1;
        return Vdp1Regs->EDSR;
      case 0x12:
        FRAMELOG("Read LOPR %X line = %d\n", Vdp1Regs->LOPR, yabsys.LineCount);
         return Vdp1Regs->LOPR;
      case 0x14:
        FRAMELOG("Read COPR %X line = %d\n", Vdp1Regs->COPR, yabsys.LineCount);
         return Vdp1Regs->COPR;
      case 0x16:
         return 0x1000 | ((Vdp1Regs->PTMR & 2) << 7) | ((Vdp1Regs->FBCR & 0x1E) << 3) | (Vdp1Regs->TVMR & 0xF);
      default:
         LOG("trying to read a Vdp1 write-only register\n");
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp1ReadLong(SH2_struct *context, u8* mem, u32 addr) {
   addr &= 0xFF;
   LOG("trying to long-read a Vdp1 register - %08X\n", addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1WriteByte(SH2_struct *context, u8* mem, u32 addr, UNUSED u8 val) {
   addr &= 0xFF;
   LOG("trying to byte-write a Vdp1 register - %08X\n", addr);
}

//////////////////////////////////////////////////////////////////////////////
static int needVBlankErase() {
  return (Vdp1External.useVBlankErase != 0);
}
static void updateTVMRMode() {
  Vdp1External.useVBlankErase = 0;
  if (((Vdp1Regs->FBCR & 3) == 3) && (((Vdp1Regs->TVMR >> 3) & 0x01) == 1)) {
    Vdp1External.useVBlankErase = 1;
  }
}

static void updateFBCRMode() {
  Vdp1External.manualchange = 0;
  Vdp1External.onecyclemode = 0;
  Vdp1External.useVBlankErase = 0;
  if (((Vdp1Regs->TVMR >> 3) & 0x01) == 1){
    Vdp1External.manualchange = ((Vdp1Regs->FBCR & 3) == 3);
    Vdp1External.useVBlankErase = 1;
  } else {
    //Manual erase shall not be reseted but need to save its current value
    // Only at frame change the order is executed.
    //This allows to have both a manual clear and a manual change at the same frame without continuously clearing the VDP1
    //The mechanism is used by the official bios animation
    Vdp1External.onecyclemode = ((Vdp1Regs->FBCR & 3) == 0) || ((Vdp1Regs->FBCR & 3) == 1);
    Vdp1External.manualerase |= ((Vdp1Regs->FBCR & 3) == 2);
    Vdp1External.manualchange = ((Vdp1Regs->FBCR & 3) == 3);
  }
}

static void Vdp1TryDraw(void) {
  if ((needVdp1draw == 1)) {
      if (CmdListDrawn == 0 && vdp1BeforeDrawCallHook) vdp1BeforeDrawCallHook(NULL);
    needVdp1draw = Vdp1Draw();
  }
}

void FASTCALL Vdp1WriteWord(SH2_struct *context, u8* mem, u32 addr, u16 val) {
  addr &= 0xFF;
  switch(addr) {
    case 0x0:
      if ((Vdp1Regs->FBCR & 3) != 3) val = (val & (~0x4));
      Vdp1Regs->TVMR = val;
      updateTVMRMode();
      FRAMELOG("TVMR => Write VBE=%d FCM=%d FCT=%d line = %d\n", (Vdp1Regs->TVMR >> 3) & 0x01, (Vdp1Regs->FBCR & 0x02) >> 1, (Vdp1Regs->FBCR & 0x01),  yabsys.LineCount);
    break;
    case 0x2:
      Vdp1Regs->FBCR = val;
      FRAMELOG("FBCR => Write VBE=%d FCM=%d FCT=%d line = %d\n", (Vdp1Regs->TVMR >> 3) & 0x01, (Vdp1Regs->FBCR & 0x02) >> 1, (Vdp1Regs->FBCR & 0x01),  yabsys.LineCount);
      updateFBCRMode();
      break;
    case 0x4:
      FRAMELOG("Write PTMR %X line = %d %d\n", val, yabsys.LineCount, yabsys.VBlankLineCount);
      if ((val & 0x3)==0x3) {
        //Skeleton warriors is writing 0xFFF to PTMR. It looks like the behavior is 0x2
          val = 0x2;
      }
      Vdp1Regs->PTMR = val;
      Vdp1External.plot_trigger_line = -1;
      Vdp1External.plot_trigger_done = 0;
      if (val == 1){
        FRAMELOG("VDP1: VDPEV_DIRECT_DRAW\n");
        Vdp1External.plot_trigger_line = yabsys.LineCount;
        abortVdp1();
        vdp1_clock = 0;
        needVdp1draw = 1;
        Vdp1TryDraw();
        Vdp1External.plot_trigger_done = 1;
      }
      break;
      case 0x6:
         Vdp1Regs->EWDR = val;
         break;
      case 0x8:
         Vdp1Regs->EWLR = val;
         break;
      case 0xA:
         Vdp1Regs->EWRR = val;
         break;
      case 0xC:
         Vdp1Regs->ENDR = val;
      	 Vdp1External.status = VDP1_STATUS_IDLE;
         break;
      default:
         LOG("trying to write a Vdp1 read-only register - %08X\n", addr);
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1WriteLong(SH2_struct *context, u8* mem, u32 addr, UNUSED u32 val) {
   addr &= 0xFF;
   LOG("trying to long-write a Vdp1 register - %08X\n", addr);
}

static int emptyCmd(vdp1cmd_struct *cmd) {
  return (
    (cmd->CMDCTRL.all == 0) &&
    (cmd->CMDLINK == 0) &&
    (cmd->CMDPMOD == 0) &&
    (cmd->CMDCOLR == 0) &&
    (cmd->CMDSRCA == 0) &&
    (cmd->CMDSIZE == 0) &&
    (cmd->CMDXA == 0) &&
    (cmd->CMDYA == 0) &&
    (cmd->CMDXB == 0) &&
    (cmd->CMDYB == 0) &&
    (cmd->CMDXC == 0) &&
    (cmd->CMDYC == 0) &&
    (cmd->CMDXD == 0) &&
    (cmd->CMDYD == 0) &&
    (cmd->CMDGRDA == 0));
}

//////////////////////////////////////////////////////////////////////////////

static void checkClipCmd(vdp1cmd_struct **sysClipCmd, vdp1cmd_struct **usrClipCmd, vdp1cmd_struct **localCoordCmd, u8 * ram, Vdp1 * regs) {
  if (sysClipCmd != NULL) {
    if (*sysClipCmd != NULL) {
      VIDCore->Vdp1SystemClipping(*sysClipCmd, ram, regs);
      free(*sysClipCmd);
      *sysClipCmd = NULL;
    }
  }
  if (usrClipCmd != NULL) {
    if (*usrClipCmd != NULL) {
      VIDCore->Vdp1UserClipping(*usrClipCmd, ram, regs);
      free(*usrClipCmd);
      *usrClipCmd = NULL;
    }
  }
  if (localCoordCmd != NULL) {
    if (*localCoordCmd != NULL) {
      VIDCore->Vdp1LocalCoordinate(*localCoordCmd, ram, regs);
      free(*localCoordCmd);
      *localCoordCmd = NULL;
    }
  }
}

static int Vdp1NormalSpriteDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer){
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];
  int ret = 1;
  if (emptyCmd(cmd)) {
    // damaged data
    yabsys.vdp1cycles += 70;
    return -1;
  }

  if ((cmd->CMDSIZE & 0x8000)) {
    yabsys.vdp1cycles += 70;
    regs->EDSR |= 2;
    return -1; // BAD Command
  }
  if (((cmd->CMDPMOD >> 3) & 0x7) > 5) {
    // damaged data
    yabsys.vdp1cycles += 70;
    return -1;
  }
  cmd->w = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
  cmd->h = cmd->CMDSIZE & 0xFF;
  if ((cmd->w == 0) || (cmd->h == 0)) {
    yabsys.vdp1cycles += 70;
    ret = 0;
  }

  cmd->flip = cmd->CMDCTRL.part.Dir;
  cmd->priority = 0;

  CONVERTCMD(cmd->CMDXA);
  CONVERTCMD(cmd->CMDYA);
  cmd->CMDXA += regs->localX;
  cmd->CMDYA += regs->localY;

  cmd->CMDXB = cmd->CMDXA + MAX(1,cmd->w);
  cmd->CMDYB = cmd->CMDYA;
  cmd->CMDXC = cmd->CMDXA + MAX(1,cmd->w);
  cmd->CMDYC = cmd->CMDYA + MAX(1,cmd->h);
  cmd->CMDXD = cmd->CMDXA;
  cmd->CMDYD = cmd->CMDYA + MAX(1,cmd->h);

  int const area = abs((cmd->CMDXA*cmd->CMDYB - cmd->CMDXB*cmd->CMDYA) + (cmd->CMDXB*cmd->CMDYC - cmd->CMDXC*cmd->CMDYB) + (cmd->CMDXC*cmd->CMDYD - cmd->CMDXD*cmd->CMDYC) + (cmd->CMDXD*cmd->CMDYA - cmd->CMDXA *cmd->CMDYD))/2;
  yabsys.vdp1cycles+= MIN(1000, 70 + (area));

  memset(cmd->G, 0, sizeof(float)*16);
  if ((cmd->CMDPMOD & 4))
  {
    yabsys.vdp1cycles+= 232;
    for (int i = 0; i < 4; i++){
      u16 color2 = Vdp1RamReadWord(NULL, ram, (Vdp1RamReadWord(NULL, ram, regs->addr + 0x1C) << 3) + (i << 1));
      cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
      cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
      cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
    }
  }

  VIDCore->Vdp1NormalSpriteDraw(cmd, ram, regs, back_framebuffer);
  return ret;
}

static int Vdp1ScaledSpriteDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer) {
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];
  s16 rw = 0, rh = 0;
  int ret = 1;

  if (emptyCmd(cmd)) {
    // damaged data
    yabsys.vdp1cycles += 70;
    return -1;
  }

  cmd->w = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
  cmd->h = cmd->CMDSIZE & 0xFF;
  if ((cmd->w == 0) || (cmd->h == 0)) {
    yabsys.vdp1cycles += 70;
    ret = 0;
  }

  cmd->flip = cmd->CMDCTRL.part.Dir;
  cmd->priority = 0;

  CONVERTCMD(cmd->CMDXA);
  CONVERTCMD(cmd->CMDYA);
  CONVERTCMD(cmd->CMDXB);
  CONVERTCMD(cmd->CMDYB);
  CONVERTCMD(cmd->CMDXC);
  CONVERTCMD(cmd->CMDYC);

  s16 x = cmd->CMDXA;
  s16 y = cmd->CMDYA;
  // Setup Zoom Point
  switch (cmd->CMDCTRL.part.ZP)
  {
  case 0x0: // Only two coordinates
    rw = cmd->CMDXC - cmd->CMDXA;
    rh = cmd->CMDYC - cmd->CMDYA;
    break;
  case 0x5: // Upper-left
    rw = cmd->CMDXB;
    rh = cmd->CMDYB;
    break;
  case 0x6: // Upper-Center
    rw = cmd->CMDXB;
    rh = cmd->CMDYB;
    x = x - rw / 2;
    break;
  case 0x7: // Upper-Right
    rw = cmd->CMDXB;
    rh = cmd->CMDYB;
    x = x - rw;
    break;
  case 0x9: // Center-left
    rw = cmd->CMDXB;
    rh = cmd->CMDYB;
    y = y - rh / 2;
    break;
  case 0xA: // Center-center
    rw = cmd->CMDXB;
    rh = cmd->CMDYB;
    x = x - rw / 2;
    y = y - rh / 2;
    break;
  case 0xB: // Center-right
    rw = cmd->CMDXB;
    rh = cmd->CMDYB;
    x = x - rw;
    y = y - rh / 2;
    break;
  case 0xD: // Lower-left
    rw = cmd->CMDXB;
    rh = cmd->CMDYB;
    y = y - rh;
    break;
  case 0xE: // Lower-center
    rw = cmd->CMDXB;
    rh = cmd->CMDYB;
    x = x - rw / 2;
    y = y - rh;
    break;
  case 0xF: // Lower-right
    rw = cmd->CMDXB;
    rh = cmd->CMDYB;
    x = x - rw;
    y = y - rh;
    break;
  default: break;
  }

  cmd->CMDXA = x + regs->localX;
  cmd->CMDYA = y + regs->localY;
  cmd->CMDXB = x + rw  + regs->localX;
  cmd->CMDYB = y + regs->localY;
  cmd->CMDXC = x + rw  + regs->localX;
  cmd->CMDYC = y + rh + regs->localY;
  cmd->CMDXD = x + regs->localX;
  cmd->CMDYD = y + rh + regs->localY;

  int area = abs((cmd->CMDXA*cmd->CMDYB - cmd->CMDXB*cmd->CMDYA) + (cmd->CMDXB*cmd->CMDYC - cmd->CMDXC*cmd->CMDYB) + (cmd->CMDXC*cmd->CMDYD - cmd->CMDXD*cmd->CMDYC) + (cmd->CMDXD*cmd->CMDYA - cmd->CMDXA *cmd->CMDYD))/2;
  yabsys.vdp1cycles+= MIN(1000, 70 + area);

  //gouraud
  memset(cmd->G, 0, sizeof(float)*16);
  if ((cmd->CMDPMOD & 4))
  {
    yabsys.vdp1cycles+= 232;
    for (int i = 0; i < 4; i++){
      u16 color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, regs->addr + 0x1C) << 3) + (i << 1));
      cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
      cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
      cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
    }
  }

  VIDCore->Vdp1ScaledSpriteDraw(cmd, ram, regs, back_framebuffer);
  return ret;
}

static int Vdp1DistortedSpriteDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer) {
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];
  int ret = 1;

  if (emptyCmd(cmd)) {
    // damaged data
    yabsys.vdp1cycles += 70;
    return 0;
  }

  cmd->w = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
  cmd->h = cmd->CMDSIZE & 0xFF;
  if ((cmd->w == 0) || (cmd->h == 0)) {
    yabsys.vdp1cycles += 70;
    ret = 0;
  }

  cmd->flip = cmd->CMDCTRL.part.Dir;
  cmd->priority = 0;

  CONVERTCMD(cmd->CMDXA);
  CONVERTCMD(cmd->CMDYA);
  CONVERTCMD(cmd->CMDXB);
  CONVERTCMD(cmd->CMDYB);
  CONVERTCMD(cmd->CMDXC);
  CONVERTCMD(cmd->CMDYC);
  CONVERTCMD(cmd->CMDXD);
  CONVERTCMD(cmd->CMDYD);

  cmd->CMDXA += regs->localX;
  cmd->CMDYA += regs->localY;
  cmd->CMDXB += regs->localX;
  cmd->CMDYB += regs->localY;
  cmd->CMDXC += regs->localX;
  cmd->CMDYC += regs->localY;
  cmd->CMDXD += regs->localX;
  cmd->CMDYD += regs->localY;

  int const area = abs((cmd->CMDXA*cmd->CMDYB - cmd->CMDXB*cmd->CMDYA) + (cmd->CMDXB*cmd->CMDYC - cmd->CMDXC*cmd->CMDYB) + (cmd->CMDXC*cmd->CMDYD - cmd->CMDXD*cmd->CMDYC) + (cmd->CMDXD*cmd->CMDYA - cmd->CMDXA *cmd->CMDYD))/2;
  yabsys.vdp1cycles+= MIN(1000, 70 + (area*3));

  memset(cmd->G, 0, sizeof(float)*16);
  if ((cmd->CMDPMOD & 4))
  {
    yabsys.vdp1cycles+= 232;
    for (int i = 0; i < 4; i++){
      u16 color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, regs->addr + 0x1C) << 3) + (i << 1));
      cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
      cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
      cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
    }
  }

  VIDCore->Vdp1DistortedSpriteDraw(cmd, ram, regs, back_framebuffer);
  return ret;
}

static int Vdp1PolygonDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer) {
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];

  CONVERTCMD(cmd->CMDXA);
  CONVERTCMD(cmd->CMDYA);
  CONVERTCMD(cmd->CMDXB);
  CONVERTCMD(cmd->CMDYB);
  CONVERTCMD(cmd->CMDXC);
  CONVERTCMD(cmd->CMDYC);
  CONVERTCMD(cmd->CMDXD);
  CONVERTCMD(cmd->CMDYD);

  cmd->CMDXA += regs->localX;
  cmd->CMDYA += regs->localY;
  cmd->CMDXB += regs->localX;
  cmd->CMDYB += regs->localY;
  cmd->CMDXC += regs->localX;
  cmd->CMDYC += regs->localY;
  cmd->CMDXD += regs->localX;
  cmd->CMDYD += regs->localY;

  int w = (sqrt((cmd->CMDXA - cmd->CMDXB)*(cmd->CMDXA - cmd->CMDXB)) + sqrt((cmd->CMDXD - cmd->CMDXC)*(cmd->CMDXD - cmd->CMDXC)))/2;
  int h = (sqrt((cmd->CMDYA - cmd->CMDYD)*(cmd->CMDYA - cmd->CMDYD)) + sqrt((cmd->CMDYB - cmd->CMDYC)*(cmd->CMDYB - cmd->CMDYC)))/2;
  yabsys.vdp1cycles += MIN(1000, 16 + (w * h) + (w * 2));

  //gouraud
  memset(cmd->G, 0, sizeof(float)*16);
  if ((cmd->CMDPMOD & 4))
  {
    yabsys.vdp1cycles+= 232;
    for (int i = 0; i < 4; i++){
      u16 color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, regs->addr + 0x1C) << 3) + (i << 1));
      cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
      cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
      cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
    }
  }
  cmd->priority = 0;
  cmd->w = 1;
  cmd->h = 1;
  cmd->flip = 0;

  VIDCore->Vdp1PolygonDraw(cmd, ram, regs, back_framebuffer);
  return 1;
}

static int Vdp1PolylineDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer) {

  Vdp2 *varVdp2Regs = &Vdp2Lines[0];

  cmd->priority = 0;
  cmd->w = 1;
  cmd->h = 1;
  cmd->flip = 0;

  CONVERTCMD(cmd->CMDXA);
  CONVERTCMD(cmd->CMDYA);
  CONVERTCMD(cmd->CMDXB);
  CONVERTCMD(cmd->CMDYB);
  CONVERTCMD(cmd->CMDXC);
  CONVERTCMD(cmd->CMDYC);
  CONVERTCMD(cmd->CMDXD);
  CONVERTCMD(cmd->CMDYD);

  cmd->CMDXA += regs->localX;
  cmd->CMDYA += regs->localY;
  cmd->CMDXB += regs->localX;
  cmd->CMDYB += regs->localY;
  cmd->CMDXC += regs->localX;
  cmd->CMDYC += regs->localY;
  cmd->CMDXD += regs->localX;
  cmd->CMDYD += regs->localY;

  //gouraud
  memset(cmd->G, 0, sizeof(float)*16);
  if ((cmd->CMDPMOD & 4))
  {
    for (int i = 0; i < 4; i++){
      u16 color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, regs->addr + 0x1C) << 3) + (i << 1));
      cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
      cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
      cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
    }
  }
  VIDCore->Vdp1PolylineDraw(cmd, ram, regs, back_framebuffer);

  return 1;
}

static int Vdp1LineDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer) {
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];


  CONVERTCMD(cmd->CMDXA);
  CONVERTCMD(cmd->CMDYA);
  CONVERTCMD(cmd->CMDXB);
  CONVERTCMD(cmd->CMDYB);

  cmd->CMDXA += regs->localX;
  cmd->CMDYA += regs->localY;
  cmd->CMDXB += regs->localX;
  cmd->CMDYB += regs->localY;
  cmd->CMDXC = cmd->CMDXB;
  cmd->CMDYC = cmd->CMDYB;
  cmd->CMDXD = cmd->CMDXA;
  cmd->CMDYD = cmd->CMDYA;

  //gouraud
  memset(cmd->G, 0, sizeof(float)*16);
  if ((cmd->CMDPMOD & 4))
  {
  for (int i = 0; i < 4; i++){
    u16 color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, regs->addr + 0x1C) << 3) + (i << 1));
    cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
    cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
    cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
  }
  }
  cmd->priority = 0;
  cmd->w = 1;
  cmd->h = 1;
  cmd->flip = 0;

  VIDCore->Vdp1LineDraw(cmd, ram, regs, back_framebuffer);

  return 1;
}

static void setupSpriteLimit(vdp1cmdctrl_struct *ctrl){
  vdp1cmd_struct *cmd = &ctrl->cmd;
  u32 dot;
  switch ((cmd->CMDPMOD >> 3) & 0x7)
  {
  case 0:
  {
    // 4 bpp Bank mode
    ctrl->start_addr = cmd->CMDSRCA * 8;
    ctrl->end_addr = ctrl->start_addr + MAX(1,cmd->h)*MAX(1,cmd->w)/2;
    break;
  }
  case 1:
  {
    // 4 bpp LUT mode
    u32 colorLut = cmd->CMDCOLR * 8;
    u32 charAddr = cmd->CMDSRCA * 8;
    ctrl->start_addr = cmd->CMDSRCA * 8;
    ctrl->end_addr = ctrl->start_addr + MAX(1,cmd->h)*MAX(1,cmd->w)/2;

    for (int i = 0; i < MAX(1,cmd->h); i++)
    {
      u16 j;
      j = 0;
      while (j < MAX(1,cmd->w)/2)
      {
        dot = Vdp1RamReadByte(NULL, Vdp1Ram, charAddr);
        int lutaddr = (dot >> 4) * 2 + colorLut;
        ctrl->start_addr = (ctrl->start_addr > lutaddr)?lutaddr:ctrl->start_addr;
        ctrl->end_addr = (ctrl->end_addr < lutaddr)?lutaddr:ctrl->end_addr;
        charAddr += 1;
        j+=1;
      }
    }
    break;
  }
  case 2:
  case 3:
  case 4:
  {
    // 8 bpp(64 color) Bank mode
    ctrl->start_addr = cmd->CMDSRCA * 8;
    ctrl->end_addr = ctrl->start_addr + MAX(1,cmd->h)*MAX(1,cmd->w);
    break;
  }
  case 5:
  {
    // 16 bpp Bank mode
    // 8 bpp(64 color) Bank mode
    ctrl->start_addr = cmd->CMDSRCA * 8;
    ctrl->end_addr = ctrl->start_addr + MAX(1,cmd->h)*MAX(1,cmd->w)*2;
    break;
  }
  default:
    VDP1LOG("Unimplemented sprite color mode: %X\n", (cmd->CMDPMOD >> 3) & 0x7);
    break;
   }
}

static int getVdp1CyclesPerLine(void)
{
  int clock = 26842600;
  //Using p37, Table 4.2 of vdp1 official doc
  if (yabsys.IsPal) {
    // Horizontal Resolution
    switch (Vdp2Lines[0].TVMD & 0x7)
    {
    case 0:
    case 2:
    case 4:
    case 6:
      //W is 320 or 640
      clock = 26656400;
      break;
    case 1:
    case 3:
    case 5:
    case 7:
      //W is 352 or 704
      clock = 28437500;
      break;
    }
  } else {
    // Horizontal Resolution
    switch (Vdp2Lines[0].TVMD & 0x7)
    {
    case 0:
    case 2:
    case 4:
    case 6:
      //W is 320 or 640
      clock = 26842600;
      break;
    case 1:
    case 3:
    case 5:
    case 7:
      //W is 352 or 704
      clock = 28636400;
      break;
    }
  }
  return clock/(yabsys.fps * yabsys.MaxLineCount);
}

static u32 returnAddr = 0xffffffff;
static vdp1cmd_struct * usrClipCmd = NULL;
static vdp1cmd_struct * sysClipCmd = NULL;
static vdp1cmd_struct * localCoordCmd = NULL;

void Vdp1DrawCommands(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  int cylesPerLine  = getVdp1CyclesPerLine();

  if (CmdListDrawn != 0) return; //The command list has already been drawn for the current frame
  CmdListLimit = 0;

  if (Vdp1External.status == VDP1_STATUS_IDLE) {
    returnAddr = 0xffffffff;
    if (usrClipCmd != NULL) free(usrClipCmd);
    if (sysClipCmd != NULL) free(sysClipCmd);
    if (localCoordCmd != NULL) free(localCoordCmd);
    usrClipCmd = NULL;
    sysClipCmd = NULL;
    localCoordCmd = NULL;
  }

   Vdp1External.status = VDP1_STATUS_RUNNING;
   if (regs->addr > 0x7FFFF) {
      Vdp1External.status = VDP1_STATUS_IDLE;
      return; // address error
    }

   u16 command = Vdp1RamReadWord(NULL, ram, regs->addr);
   u32 commandCounter = 0;

   Vdp1External.updateVdp1Ram = 0;
   vdp1Ram_update_start = 0x80000;
   vdp1Ram_update_end = 0x0;
   Vdp1External.checkEDSR = 0;

   nbCmdToProcess = 0;
   yabsys.vdp1cycles = 0;
   while (!(command & 0x8000) && commandCounter < Vdp1CommandBufferSize) {
      regs->COPR = (regs->addr & 0x7FFFF) >> 3;
      // First, process the command
      if (!(command & 0x4000)) { // if (!skip)
         if (vdp1_clock <= 0) {
           //No more clock cycle, wait next line
             if (vdp1NewCommandsFetchedHook) vdp1NewCommandsFetchedHook(&commandCounter);
                 return;
         }
         int ret;
         vdp1cmdctrl_struct * ctrl = NULL;
         switch (command & 0x000F) {
         case 0: // normal sprite draw
            ctrl = &cmdBufferBeingProcessed[nbCmdToProcess];
            ctrl->dirty = 0;
            Vdp1ReadCommand(&ctrl->cmd, regs->addr, Vdp1Ram);
            checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
            ctrl->ignitionLine = MIN(yabsys.LineCount + yabsys.vdp1cycles/cylesPerLine,yabsys.MaxLineCount-1);
            ret = Vdp1NormalSpriteDraw(&ctrl->cmd, ram, regs, back_framebuffer);
            if (ret == 0) 
            {
                vdp1_clock = 0; //Incorrect command, wait next line to continue
            }
            if (ret == 1) nbCmdToProcess++;
            ctrl->completionLine = MIN(yabsys.LineCount + yabsys.vdp1cycles/cylesPerLine,yabsys.MaxLineCount-1);
            setupSpriteLimit(ctrl);
            break;
         case 1: // scaled sprite draw
            ctrl = &cmdBufferBeingProcessed[nbCmdToProcess];
            ctrl->dirty = 0;
            Vdp1ReadCommand(&ctrl->cmd, regs->addr, Vdp1Ram);
            ctrl->ignitionLine = MIN(yabsys.LineCount + yabsys.vdp1cycles/cylesPerLine,yabsys.MaxLineCount-1);
            checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
            ret = Vdp1ScaledSpriteDraw(&ctrl->cmd, ram, regs, back_framebuffer);
            if (ret == 0)
            {
                vdp1_clock = 0; //Incorrect command, wait next line to continue
            }
            if (ret == 1) nbCmdToProcess++;
            ctrl->completionLine = MIN(yabsys.LineCount + yabsys.vdp1cycles/cylesPerLine,yabsys.MaxLineCount-1);
            setupSpriteLimit(ctrl);
            break;
         case 2: // distorted sprite draw
         case 3: /* this one should be invalid, but some games
                 (Hardcore 4x4 for instance) use it instead of 2 */
            ctrl = &cmdBufferBeingProcessed[nbCmdToProcess];
            ctrl->dirty = 0;
            Vdp1ReadCommand(&ctrl->cmd, regs->addr, Vdp1Ram);
            ctrl->ignitionLine = MIN(yabsys.LineCount + yabsys.vdp1cycles/cylesPerLine,yabsys.MaxLineCount-1);
            checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
            ret = Vdp1DistortedSpriteDraw(&ctrl->cmd, ram, regs, back_framebuffer);
            if (ret == 0) 
            {
                vdp1_clock = 0; //Incorrect command, wait next line to continue
            }
            if (ret == 1) nbCmdToProcess++;
            ctrl->completionLine = MIN(yabsys.LineCount + yabsys.vdp1cycles/cylesPerLine,yabsys.MaxLineCount-1);
            setupSpriteLimit(ctrl);
            break;
         case 4: // polygon draw
            ctrl = &cmdBufferBeingProcessed[nbCmdToProcess];
            ctrl->dirty = 0;
            Vdp1ReadCommand(&ctrl->cmd, regs->addr, Vdp1Ram);
            ctrl->ignitionLine = MIN(yabsys.LineCount + yabsys.vdp1cycles/cylesPerLine,yabsys.MaxLineCount-1);
            checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
            Vdp1PolygonDraw(&ctrl->cmd, ram, regs, back_framebuffer);
            nbCmdToProcess++;
            ctrl->completionLine = MIN(yabsys.LineCount + yabsys.vdp1cycles/cylesPerLine,yabsys.MaxLineCount-1);
            setupSpriteLimit(ctrl);
            break;
         case 5: // polyline draw
         case 7: // undocumented mirror
            ctrl = &cmdBufferBeingProcessed[nbCmdToProcess];
            ctrl->dirty = 0;
            Vdp1ReadCommand(&ctrl->cmd, regs->addr, Vdp1Ram);
            ctrl->ignitionLine = MIN(yabsys.LineCount + yabsys.vdp1cycles/cylesPerLine,yabsys.MaxLineCount-1);
            checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
            Vdp1PolylineDraw(&ctrl->cmd, ram, regs, back_framebuffer);
            nbCmdToProcess++;
            ctrl->completionLine = MIN(yabsys.LineCount + yabsys.vdp1cycles/cylesPerLine,yabsys.MaxLineCount-1);
            setupSpriteLimit(ctrl);
            break;
         case 6: // line draw
            ctrl = &cmdBufferBeingProcessed[nbCmdToProcess];
            ctrl->dirty = 0;
            Vdp1ReadCommand(&ctrl->cmd, regs->addr, Vdp1Ram);
            ctrl->ignitionLine = MIN(yabsys.LineCount + yabsys.vdp1cycles/cylesPerLine,yabsys.MaxLineCount-1);
            checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
            Vdp1LineDraw(&ctrl->cmd, ram, regs, back_framebuffer);
            nbCmdToProcess++;
            ctrl->completionLine = MIN(yabsys.LineCount + yabsys.vdp1cycles/cylesPerLine,yabsys.MaxLineCount-1);
            setupSpriteLimit(ctrl);
            break;
         case 8: // user clipping coordinates
         case 11: // undocumented mirror
            checkClipCmd(&sysClipCmd, NULL, &localCoordCmd, ram, regs);
            yabsys.vdp1cycles += 16;
            usrClipCmd = (vdp1cmd_struct *)malloc(sizeof(vdp1cmd_struct));
            Vdp1ReadCommand(usrClipCmd, regs->addr, ram);
            break;
         case 9: // system clipping coordinates
            checkClipCmd(NULL, &usrClipCmd, &localCoordCmd, ram, regs);
            yabsys.vdp1cycles += 16;
            sysClipCmd = (vdp1cmd_struct *)malloc(sizeof(vdp1cmd_struct));
            Vdp1ReadCommand(sysClipCmd, regs->addr, ram);
            break;
         case 10: // local coordinate
            checkClipCmd(&sysClipCmd, &usrClipCmd, NULL, ram, regs);
            yabsys.vdp1cycles += 16;
            localCoordCmd = (vdp1cmd_struct *)malloc(sizeof(vdp1cmd_struct));
            Vdp1ReadCommand(localCoordCmd, regs->addr, ram);
            break;
         default: // Abort
            VDP1LOG("vdp1\t: Bad command: %x\n", command);
            checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
      	    Vdp1External.status = VDP1_STATUS_IDLE;
            regs->EDSR |= 2;
            regs->COPR = (regs->addr & 0x7FFFF) >> 3;
            CmdListDrawn = 1;
            CmdListLimit = MAX((regs->addr & 0x7FFFF), regs->addr);
            return;
         }
      } else {
        yabsys.vdp1cycles += 16;
      }
      vdp1_clock -= yabsys.vdp1cycles;
      yabsys.vdp1cycles = 0;

	  // Force to quit internal command error( This technic(?) is used by BATSUGUN )
	  if (regs->EDSR & 0x02){
		  checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
		  Vdp1External.status = VDP1_STATUS_IDLE;
		  regs->COPR = (regs->addr & 0x7FFFF) >> 3;
      CmdListDrawn = 1;
      CmdListLimit = MAX((regs->addr & 0x7FFFF), regs->addr);
		  return;
	  }

      // Next, determine where to go next
      switch ((command & 0x3000) >> 12) {
      case 0: // NEXT, jump to following table
         regs->addr += 0x20;
         break;
      case 1: // ASSIGN, jump to CMDLINK
         regs->addr = T1ReadWord(ram, regs->addr + 2) * 8;
         break;
      case 2: // CALL, call a subroutine
         if (returnAddr == 0xFFFFFFFF)
            returnAddr = regs->addr + 0x20;

         regs->addr = T1ReadWord(ram, regs->addr + 2) * 8;
         break;
      case 3: // RETURN, return from subroutine
         if (returnAddr != 0xFFFFFFFF) {
            regs->addr = returnAddr;
            returnAddr = 0xFFFFFFFF;
         }
         else
            regs->addr += 0x20;
         break;
      }

      command = Vdp1RamReadWord(NULL,ram, regs->addr);
      CmdListLimit = MAX((regs->addr & 0x7FFFF), regs->addr);
      //If we change directly CPR to last value, scorcher will not boot.
      //If we do not change it, Noon will not start
      //So store the value and update COPR with last value at VBlank In
      regs->lCOPR = (regs->addr & 0x7FFFF) >> 3;
      commandCounter++;
   }
   if (vdp1NewCommandsFetchedHook) vdp1NewCommandsFetchedHook(&commandCounter);
   if (command & 0x8000) {
        LOG("VDP1: Command Finished! count = %d @ %08X", command_count, regs->addr);
        Vdp1External.status = VDP1_STATUS_IDLE;
   }
   CmdListDrawn = 1;
   CmdListLimit = MAX((regs->addr & 0x7FFFF), regs->addr);
   checkClipCmd(&sysClipCmd, &usrClipCmd, &localCoordCmd, ram, regs);
}

//ensure that registers are set correctly
void Vdp1FakeDrawCommands(u8 * ram, Vdp1 * regs)
{
   u16 command = T1ReadWord(ram, regs->addr);
   u32 commandCounter = 0;
   u32 returnAddr = 0xffffffff;
   vdp1cmd_struct cmd;

   while (!(command & 0x8000) && commandCounter < Vdp1CommandBufferSize) { // fix me
      // First, process the command
      if (!(command & 0x4000)) { // if (!skip)
         switch (command & 0x000F) {
         case 0: // normal sprite draw
         case 1: // scaled sprite draw
         case 2: // distorted sprite draw
         case 3: /* this one should be invalid, but some games
                 (Hardcore 4x4 for instance) use it instead of 2 */
         case 4: // polygon draw
         case 5: // polyline draw
         case 6: // line draw
         case 7: // undocumented polyline draw mirror
            break;
         case 8: // user clipping coordinates
         case 11: // undocumented mirror
            Vdp1ReadCommand(&cmd, regs->addr, Vdp1Ram);
            VIDCore->Vdp1UserClipping(&cmd, ram, regs);
            break;
         case 9: // system clipping coordinates
            Vdp1ReadCommand(&cmd, regs->addr, Vdp1Ram);
            VIDCore->Vdp1SystemClipping(&cmd, ram, regs);
            break;
         case 10: // local coordinate
            Vdp1ReadCommand(&cmd, regs->addr, Vdp1Ram);
            VIDCore->Vdp1LocalCoordinate(&cmd, ram, regs);
            break;
         default: // Abort
            VDP1LOG("vdp1\t: Bad command: %x\n", command);
            regs->EDSR |= 2;
            regs->COPR = regs->addr >> 3;
            return;
         }
      }

      // Next, determine where to go next
      switch ((command & 0x3000) >> 12) {
      case 0: // NEXT, jump to following table
         regs->addr += 0x20;
         break;
      case 1: // ASSIGN, jump to CMDLINK
         regs->addr = T1ReadWord(ram, regs->addr + 2) * 8;
         break;
      case 2: // CALL, call a subroutine
         if (returnAddr == 0xFFFFFFFF)
            returnAddr = regs->addr + 0x20;

         regs->addr = T1ReadWord(ram, regs->addr + 2) * 8;
         break;
      case 3: // RETURN, return from subroutine
         if (returnAddr != 0xFFFFFFFF) {
            regs->addr = returnAddr;
            returnAddr = 0xFFFFFFFF;
         }
         else
            regs->addr += 0x20;
         break;
      }

      command = T1ReadWord(ram, regs->addr);
      commandCounter++;
   }
}

static int Vdp1Draw(void)
{
    FRAMELOG("Vdp1Draw\n");
    if (!Vdp1External.disptoggle)
    {
        Vdp1Regs->EDSR >>= 1;
        Vdp1NoDraw();
    } else
    {
        if (Vdp1External.status == VDP1_STATUS_IDLE)
        {
            Vdp1Regs->EDSR >>= 1;
            Vdp1Regs->addr = 0;

            // beginning of a frame
            // BEF <- CEF
            // CEF <- 0
            //Vdp1Regs->EDSR >>= 1;
            /* this should be done after a frame change or a plot trigger */
            Vdp1Regs->COPR = 0;
            Vdp1Regs->lCOPR = 0;
        }
        VIDCore->Vdp1Draw();
    }
    if (Vdp1External.status == VDP1_STATUS_IDLE)
    {
        FRAMELOG("Vdp1Draw end at %d line\n", yabsys.LineCount);
        Vdp1Regs->EDSR |= 2;
        ScuSendDrawEnd();
        if (vdp1DrawCompletedHook)
            vdp1DrawCompletedHook(NULL);
    }
    if (Vdp1External.status == VDP1_STATUS_IDLE)
        return 0;
    else
        return 1;
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp1NoDraw(void) {
   // beginning of a frame (ST-013-R3-061694 page 53)
   // BEF <- CEF
   // CEF <- 0
   //Vdp1Regs->EDSR >>= 1;
   /* this should be done after a frame change or a plot trigger */
   Vdp1Regs->COPR = 0;
   Vdp1Regs->lCOPR = 0;
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
   _Ygl->vdp1On[_Ygl->drawframe] = 0;
#endif
   Vdp1FakeDrawCommands(Vdp1Ram, Vdp1Regs);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1ReadCommand(vdp1cmd_struct *cmd, u32 addr, u8* ram) {
   cmd->CMDCTRL.all = T1ReadWord(ram, addr);
   cmd->CMDLINK = T1ReadWord(ram, addr + 0x2);
   cmd->CMDPMOD = T1ReadWord(ram, addr + 0x4);
   cmd->CMDCOLR = T1ReadWord(ram, addr + 0x6);
   cmd->CMDSRCA = T1ReadWord(ram, addr + 0x8);
   cmd->CMDSIZE = T1ReadWord(ram, addr + 0xA);
   cmd->CMDXA = T1ReadWord(ram, addr + 0xC);
   cmd->CMDYA = T1ReadWord(ram, addr + 0xE);
   cmd->CMDXB = T1ReadWord(ram, addr + 0x10);
   cmd->CMDYB = T1ReadWord(ram, addr + 0x12);
   cmd->CMDXC = T1ReadWord(ram, addr + 0x14);
   cmd->CMDYC = T1ReadWord(ram, addr + 0x16);
   cmd->CMDXD = T1ReadWord(ram, addr + 0x18);
   cmd->CMDYD = T1ReadWord(ram, addr + 0x1A);
   cmd->CMDGRDA = T1ReadWord(ram, addr + 0x1C);
}

//////////////////////////////////////////////////////////////////////////////

int Vdp1SaveState(void ** stream)
{
   int offset;
#ifdef IMPROVED_SAVESTATES
   int i = 0;
   u8 back_framebuffer[0x40000] = { 0 };
#endif

   offset = MemStateWriteHeader(stream, "VDP1", 1);

   // Write registers
   MemStateWrite((void *)Vdp1Regs, sizeof(Vdp1), 1, stream);

   // Write VDP1 ram
   MemStateWrite((void *)Vdp1Ram, 0x80000, 1, stream);

#ifdef IMPROVED_SAVESTATES
   for (i = 0; i < 0x40000; i++)
      back_framebuffer[i] = Vdp1FrameBufferReadByte(NULL, NULL, i);

   MemStateWrite((void *)back_framebuffer, 0x40000, 1, stream);
#endif
   return MemStateFinishHeader(stream, offset);
}

//////////////////////////////////////////////////////////////////////////////

int Vdp1LoadState(const void * stream, UNUSED int version, int size)
{
#ifdef IMPROVED_SAVESTATES
   int i = 0;
   u8 back_framebuffer[0x40000] = { 0 };
#endif

   // Read registers
   MemStateRead((void *)Vdp1Regs, sizeof(Vdp1), 1, stream);

   // Read VDP1 ram
   MemStateRead((void *)Vdp1Ram, 0x80000, 1, stream);
   vdp1Ram_update_start = 0x80000;
   vdp1Ram_update_end = 0x0;
#ifdef IMPROVED_SAVESTATES
   MemStateRead((void *)back_framebuffer, 0x40000, 1, stream);

   for (i = 0; i < 0x40000; i++)
      Vdp1FrameBufferWriteByte(NULL, NULL, i, back_framebuffer[i]);
#endif
   return size;
}

//////////////////////////////////////////////////////////////////////////////
static void startField(void) {
  int isrender = 0;
  yabsys.wait_line_count = -1;
  FRAMELOG("StartField ***** VOUT(T) %d FCM=%d FCT=%d VBE=%d PTMR=%d (%d, %d, %d, %d)*****\n", Vdp1External.swap_frame_buffer, (Vdp1Regs->FBCR & 0x02) >> 1, (Vdp1Regs->FBCR & 0x01), (Vdp1Regs->TVMR >> 3) & 0x01, Vdp1Regs->PTMR, Vdp1External.onecyclemode, Vdp1External.manualchange, Vdp1External.manualerase, needVBlankErase());

  // Manual Change
  Vdp1External.swap_frame_buffer |= (Vdp1External.manualchange == 1);
  Vdp1External.swap_frame_buffer |= (Vdp1External.onecyclemode == 1);

  // Frame Change
  if (Vdp1External.swap_frame_buffer == 1)
  {
    FRAMELOG("Swap Line %d\n", yabsys.LineCount);
    if ((Vdp1External.manualerase == 1) || (Vdp1External.onecyclemode == 1))
    {
      int id = 0;
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
      if (_Ygl != NULL) id = _Ygl->readframe;
#endif
      VIDCore->Vdp1EraseWrite(id);
      CmdListDrawn = 0;
      Vdp1External.manualerase = 0;
    }

    VIDCore->Vdp1FrameChange();
    CmdListDrawn = 0;
    FRAMELOG("Change readframe %d to %d (%d)\n", _Ygl->drawframe, _Ygl->readframe, yabsys.LineCount);
    Vdp1External.current_frame = !Vdp1External.current_frame;
    Vdp1Regs->LOPR = Vdp1Regs->COPR;
    Vdp1Regs->COPR = 0;
    Vdp1Regs->lCOPR = 0;
    Vdp1Regs->EDSR >>= 1;

    FRAMELOG("[VDP1] Displayed framebuffer changed. EDSR=%02X", Vdp1Regs->EDSR);

    Vdp1External.swap_frame_buffer = 0;

    // if Plot Trigger mode == 0x02 draw start
    if ((Vdp1Regs->PTMR == 0x2)){
      FRAMELOG("[VDP1] PTMR == 0x2 start drawing immidiatly\n");
      abortVdp1();
      vdp1_clock = 0;
      needVdp1draw = 1;
    }
  }
  else {
    if ( Vdp1External.status == VDP1_STATUS_RUNNING) {
      LOG("[VDP1] Start Drawing continue");
      needVdp1draw = 1;
    }
  }

  if (Vdp1Regs->PTMR == 0x1) Vdp1External.plot_trigger_done = 0;

  FRAMELOG("End StartField\n");

  Vdp1External.manualchange = 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1HBlankIN(void)
{
  if (nbCmdToProcess > 0) {
    for (int i = 0; i<nbCmdToProcess; i++) {
      if (cmdBufferBeingProcessed[i].ignitionLine == yabsys.LineCount+1) {
        if (!((cmdBufferBeingProcessed[i].start_addr >= vdp1Ram_update_end) ||
            (cmdBufferBeingProcessed[i].end_addr <= vdp1Ram_update_start))) {
          if (Vdp1External.checkEDSR == 0) {
            if (VIDCore->Vdp1RegenerateCmd != NULL)
              VIDCore->Vdp1RegenerateCmd(&cmdBufferBeingProcessed[i].cmd);
          }
        }
        cmdBufferBeingProcessed[i].ignitionLine = -1;
      }
    }
    if (cmdBufferBeingProcessed[nbCmdToProcess-1].ignitionLine == -1) {
      vdp1Ram_update_start = 0x80000;
      vdp1Ram_update_end = 0x0;
      if (VIDCore != NULL) {
        if (VIDCore->composeVDP1 != NULL) VIDCore->composeVDP1();
      }
      Vdp1Regs->COPR = Vdp1Regs->lCOPR;
    }
  }
  if(yabsys.LineCount == 0) {
    startField();
  }
  if (Vdp1Regs->PTMR == 0x1){
    if (Vdp1External.plot_trigger_line == yabsys.LineCount){
      if(Vdp1External.plot_trigger_done == 0) {
        vdp1_clock = 0;
        needVdp1draw = 1;
        Vdp1External.plot_trigger_done = 1;
      }
    }
  }
  #if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
    if (VIDCore != NULL && VIDCore->id != VIDCORE_SOFT) YglTMCheck();
  #endif
}
//////////////////////////////////////////////////////////////////////////////

void Vdp1HBlankOUT(void)
{
  vdp1_clock += getVdp1CyclesPerLine();
  Vdp1TryDraw();

}

//////////////////////////////////////////////////////////////////////////////

void Vdp1VBlankIN(void)
{
  // if (VIDCore != NULL) {
  //   if (VIDCore->composeVDP1 != NULL) VIDCore->composeVDP1();
  // }
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1VBlankOUT(void)
{
  //Out of VBlankOut : Break Batman
  if (needVBlankErase()) {
    int id = 0;
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
    if (_Ygl != NULL) id = _Ygl->readframe;
#endif
    CmdListDrawn = 0;
    VIDCore->Vdp1EraseWrite(id);
    if (vdp1FrameCompletedHook)
    {
        vdp1FrameCompletedHook(NULL);
    }
  }
}
