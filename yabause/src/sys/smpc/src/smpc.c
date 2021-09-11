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

/*! \file smpc.c
    \brief SMPC emulation functions.
*/

#include <stdlib.h>
#include <time.h>
#include "smpc.h"
#include "cs2.h"
#include "debug.h"
#include "peripheral.h"
#include "scsp.h"
#include "scu.h"
#include "sh2core.h"
#include "vdp1.h"
#include "vdp2.h"
#include "yabause.h"
#include "movie.h"
#include "eeprom.h"

#ifdef _arch_dreamcast
# include "dreamcast/localtime.h"
#endif
#ifdef PSP
# include "psp/localtime.h"
#endif

Smpc * SmpcRegs;
SmpcInternal * SmpcInternalVars;

static u8 * SmpcRegsT;
static int intback_wait_for_line = 0;
static u8 bustmp = 0;

//#define SMPCLOG printf

//////////////////////////////////////////////////////////////////////////////

int SmpcInit(u8 regionid, int clocksync, u32 basetime, u8 languageid) {
   if ((SmpcRegsT = (u8 *) calloc(1, sizeof(Smpc))) == NULL)
      return -1;
 
   SmpcRegs = (Smpc *) SmpcRegsT;

   if ((SmpcInternalVars = (SmpcInternal *) calloc(1, sizeof(SmpcInternal))) == NULL)
      return -1;
  
   SmpcInternalVars->regionsetting = regionid;
   SmpcInternalVars->regionid = regionid;
   SmpcInternalVars->clocksync = clocksync;
   SmpcInternalVars->basetime = basetime ? basetime : time(NULL);
   SmpcInternalVars->languageid = languageid;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcDeInit(void) {
   if (SmpcRegsT)
      free(SmpcRegsT);
   SmpcRegsT = NULL;

   if (SmpcInternalVars)
      free(SmpcInternalVars);
   SmpcInternalVars = NULL;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcRecheckRegion(void) {
   if (SmpcInternalVars == NULL)
      return;

   if (SmpcInternalVars->regionsetting == REGION_AUTODETECT)
   {
      // Time to autodetect the region using the cd block
      SmpcInternalVars->regionid = Cs2GetRegionID();

      // Since we couldn't detect the region from the CD, let's assume
      // it's japanese
      if (SmpcInternalVars->regionid == 0)
         SmpcInternalVars->regionid = 1;
   }
   else
      Cs2GetIP(0);
}

//////////////////////////////////////////////////////////////////////////////

static void SmpcSetLanguage(void) {
   SmpcInternalVars->SMEM[3] = (SmpcInternalVars->SMEM[3] & 0xF0) | SmpcInternalVars->languageid;
}

//////////////////////////////////////////////////////////////////////////////

int SmpcGetLanguage(void) {
   // TODO : use this in standalone to store currently set language into config
   return SmpcInternalVars->languageid;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcReset(void) {
   memset((void *)SmpcRegs, 0, sizeof(Smpc));
   memset((void *)SmpcInternalVars->SMEM, 0, 4);

   SmpcRecheckRegion();
   SmpcSetLanguage(); // to (re)apply currently stored languageid

   SmpcInternalVars->dotsel = 0;
   SmpcInternalVars->mshnmi = 0;
   SmpcInternalVars->sysres = 0;
   SmpcInternalVars->sndres = 0;
   SmpcInternalVars->cdres = 0;
   SmpcInternalVars->resd = 1;
   SmpcInternalVars->ste = 0;
   SmpcInternalVars->resb = 0;

   SmpcInternalVars->firstPeri=0;

   SmpcInternalVars->timing=0;

   memset((void *)&SmpcInternalVars->port1, 0, sizeof(PortData_struct));
   memset((void *)&SmpcInternalVars->port2, 0, sizeof(PortData_struct));
   SmpcRegs->OREG[31] = 0xD;
}

//////////////////////////////////////////////////////////////////////////////

static void SmpcSSHON(void) {
   YabauseStartSlave();
}

//////////////////////////////////////////////////////////////////////////////

static void SmpcSSHOFF(void) {
   YabauseStopSlave();
}

//////////////////////////////////////////////////////////////////////////////

static void SmpcSNDON(void) {
   if (!yabsys.isSTV) M68KStart(); //C68k wire is controlled by pdr2 on STV
   SmpcRegs->OREG[31] = 0x6;
}

//////////////////////////////////////////////////////////////////////////////

static void SmpcSNDOFF(void) {
   if (!yabsys.isSTV) M68KStop(); //C68k wire is controlled by pdr2 on STV
   SmpcRegs->OREG[31] = 0x7;
}

//////////////////////////////////////////////////////////////////////////////

static void SmpcSYSRES(void) {
  SmpcRegs->OREG[31] = 0xD;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcCKCHG352(void) {
   // Set DOTSEL
   SmpcInternalVars->dotsel = 1;

   // Send NMI
   SH2NMI(MSH2);
   // Reset VDP1, VDP2, SCU, and SCSP
   Vdp1Reset();  
   Vdp2Reset();  
   ScuReset();  
   ScspReset();  

   // Clear VDP1/VDP2 ram

   YabauseStopSlave();

   // change clock
   YabauseChangeTiming(CLKTYPE_28MHZ);
}

//////////////////////////////////////////////////////////////////////////////

void SmpcCKCHG320(void) {

   // Set DOTSEL
   SmpcInternalVars->dotsel = 0;

   // Send NMI
   SH2NMI(MSH2);

   // Reset VDP1, VDP2, SCU, and SCSP
   Vdp1Reset();  
   Vdp2Reset();  
   ScuReset();  
   ScspReset();  

   // Clear VDP1/VDP2 ram

   YabauseStopSlave();

   // change clock
   YabauseChangeTiming(CLKTYPE_26MHZ);
}

struct movietime {

	int tm_year;
	int tm_wday;
	int tm_mon;
	int tm_mday;
	int tm_hour;
	int tm_min;
	int tm_sec;
};

static struct movietime movietime;
int totalseconds;
int noon= 43200;

//////////////////////////////////////////////////////////////////////////////

static void SmpcINTBACKStatus(void) {
   // return time, cartidge, zone, etc. data
   int i;
   struct tm times;
   u8 year[4];
   time_t tmp;

   SmpcRegs->OREG[0] = 0x80 | (SmpcInternalVars->resd << 6);   // goto normal startup
   //SmpcRegs->OREG[0] = 0x0 | (SmpcInternalVars->resd << 6);  // goto setclock/setlanguage screen
    
   // write time data in OREG1-7
   if (SmpcInternalVars->clocksync) {
      tmp = SmpcInternalVars->basetime + ((u64)framecounter * 1001 / 60000);
   } else {
      tmp = time(NULL);
   }
#ifdef WIN32
   memcpy(&times, localtime(&tmp), sizeof(times));
#elif defined(_arch_dreamcast) || defined(PSP)
   internal_localtime_r(&tmp, &times);
#else
   localtime_r(&tmp, &times);
#endif
   year[0] = (1900 + times.tm_year) / 1000;
   year[1] = ((1900 + times.tm_year) % 1000) / 100;
   year[2] = (((1900 + times.tm_year) % 1000) % 100) / 10;
   year[3] = (((1900 + times.tm_year) % 1000) % 100) % 10;
   SmpcRegs->OREG[1] = (year[0] << 4) | year[1];
   SmpcRegs->OREG[2] = (year[2] << 4) | year[3];
   SmpcRegs->OREG[3] = (times.tm_wday << 4) | (times.tm_mon + 1);
   SmpcRegs->OREG[4] = ((times.tm_mday / 10) << 4) | (times.tm_mday % 10);
   SmpcRegs->OREG[5] = ((times.tm_hour / 10) << 4) | (times.tm_hour % 10);
   SmpcRegs->OREG[6] = ((times.tm_min / 10) << 4) | (times.tm_min % 10);
   SmpcRegs->OREG[7] = ((times.tm_sec / 10) << 4) | (times.tm_sec % 10);

   if(Movie.Status == Recording || Movie.Status == Playback) {
	   movietime.tm_year=0x62;
	   movietime.tm_wday=0x04;
	   movietime.tm_mday=0x01;
	   movietime.tm_mon=0;
	   totalseconds = ((framecounter / 60) + noon);

	   movietime.tm_sec=totalseconds % 60;
	   movietime.tm_min=totalseconds/60;
	   movietime.tm_hour=movietime.tm_min/60;

	   //convert to sane numbers
	   movietime.tm_min=movietime.tm_min % 60;
	   movietime.tm_hour=movietime.tm_hour % 24;

	   year[0] = (1900 + movietime.tm_year) / 1000;
	   year[1] = ((1900 + movietime.tm_year) % 1000) / 100;
	   year[2] = (((1900 + movietime.tm_year) % 1000) % 100) / 10;
	   year[3] = (((1900 + movietime.tm_year) % 1000) % 100) % 10;
	   SmpcRegs->OREG[1] = (year[0] << 4) | year[1];
	   SmpcRegs->OREG[2] = (year[2] << 4) | year[3];
	   SmpcRegs->OREG[3] = (movietime.tm_wday << 4) | (movietime.tm_mon + 1);
	   SmpcRegs->OREG[4] = ((movietime.tm_mday / 10) << 4) | (movietime.tm_mday % 10);
	   SmpcRegs->OREG[5] = ((movietime.tm_hour / 10) << 4) | (movietime.tm_hour % 10);
	   SmpcRegs->OREG[6] = ((movietime.tm_min / 10) << 4) | (movietime.tm_min % 10);
	   SmpcRegs->OREG[7] = ((movietime.tm_sec / 10) << 4) | (movietime.tm_sec % 10);
   }

   // write cartidge data in OREG8
   SmpcRegs->OREG[8] = 0; // FIXME : random value
    
   // write zone data in OREG9 bits 0-7
   // 1 -> japan
   // 2 -> asia/ntsc
   // 4 -> north america
   // 5 -> central/south america/ntsc
   // 6 -> corea
   // A -> asia/pal
   // C -> europe + others/pal
   // D -> central/south america/pal
   SmpcRegs->OREG[9] = SmpcInternalVars->regionid;

   // system state, first part in OREG10, bits 0-7
   // bit | value  | comment
   // ---------------------------
   // 7   | 0      |
   // 6   | DOTSEL |
   // 5   | 1      |
   // 4   | 1      |
   // 3   | MSHNMI |
   // 2   | 1      |
   // 1   | SYSRES | 
   // 0   | SNDRES |
   SmpcRegs->OREG[10] = 0x34|(SmpcInternalVars->dotsel<<6)|(SmpcInternalVars->mshnmi<<3)|(SmpcInternalVars->sysres<<1)|SmpcInternalVars->sndres;
    
   // system state, second part in OREG11, bit 6
   // bit 6 -> CDRES
   SmpcRegs->OREG[11] = SmpcInternalVars->cdres << 6; // FIXME

   // SMEM
   for(i = 0;i < 4;i++)
      SmpcRegs->OREG[12+i] = SmpcInternalVars->SMEM[i];
    
   SmpcRegs->OREG[31] = 0x10; // set to intback command
}

//////////////////////////////////////////////////////////////////////////////

static u16 m_pmode = 0;

static void SmpcINTBACKPeripheral(void) {
  int oregoffset;
  PortData_struct *port1, *port2;
  if(PERCore)
       PERCore->HandleEvents();
  if (SmpcInternalVars->firstPeri == 2) {
    SmpcRegs->SR = 0x80 | m_pmode;
    SmpcInternalVars->firstPeri = 0;
  } else {
    SmpcRegs->SR = 0xC0 | m_pmode;
    SmpcInternalVars->firstPeri++;
  }

  /* Port Status:
  0x04 - Sega-tap is connected
  0x16 - Multi-tap is connected
  0x21-0x2F - Clock serial peripheral is connected
  0xF0 - Not Connected or Unknown Device
  0xF1 - Peripheral is directly connected */

  /* PeripheralID:
  0x02 - Digital Device Standard Format
  0x13 - Racing Device Standard Format
  0x15 - Analog Device Standard Format
  0x23 - Pointing Device Standard Format
  0x23 - Shooting Device Standard Format
  0x34 - Keyboard Device Standard Format
  0xE1 - Mega Drive 3-Button Pad
  0xE2 - Mega Drive 6-Button Pad
  0xE3 - Saturn Mouse
  0xFF - Not Connected */

  /* Special Notes(for potential future uses):

  If a peripheral is disconnected from a port, you only return 1 byte for
  that port(which is the port status 0xF0), at the next OREG you then return
  the port status of the next port.

  e.g. If Port 1 has nothing connected, and Port 2 has a controller
       connected:

  OREG0 = 0xF0
  OREG1 = 0xF1
  OREG2 = 0x02
  etc.
  */

  oregoffset=0;

  if (SmpcInternalVars->port1.size == 0 && SmpcInternalVars->port2.size == 0)
  {
     // Request data from the Peripheral Interface
     port1 = &PORTDATA1;
     port2 = &PORTDATA2;
     memcpy(&SmpcInternalVars->port1, port1, sizeof(PortData_struct));
     memcpy(&SmpcInternalVars->port2, port2, sizeof(PortData_struct));
     PerFlush(&PORTDATA1);
     PerFlush(&PORTDATA2);
     SmpcInternalVars->port1.offset = 0;
     SmpcInternalVars->port2.offset = 0;
     LagFrameFlag=0;
  }

  // Port 1
  if (SmpcInternalVars->port1.size > 0)
  {
     if ((SmpcInternalVars->port1.size-SmpcInternalVars->port1.offset) < 32)
     {
        memcpy(SmpcRegs->OREG, SmpcInternalVars->port1.data+SmpcInternalVars->port1.offset, SmpcInternalVars->port1.size-SmpcInternalVars->port1.offset);
        oregoffset += SmpcInternalVars->port1.size-SmpcInternalVars->port1.offset;
        SmpcInternalVars->port1.size = 0;
     }
     else
     {
        memcpy(SmpcRegs->OREG, SmpcInternalVars->port1.data, 32);
        oregoffset += 32;
        SmpcInternalVars->port1.offset += 32;
     }
  }
  // Port 2
  if (SmpcInternalVars->port2.size > 0 && oregoffset < 32)
  {
     if ((SmpcInternalVars->port2.size-SmpcInternalVars->port2.offset) < (32 - oregoffset))
     {
        memcpy(SmpcRegs->OREG + oregoffset, SmpcInternalVars->port2.data+SmpcInternalVars->port2.offset, SmpcInternalVars->port2.size-SmpcInternalVars->port2.offset);
        SmpcInternalVars->port2.size = 0;
     }
     else
     {
        memcpy(SmpcRegs->OREG + oregoffset, SmpcInternalVars->port2.data, 32 - oregoffset);
        SmpcInternalVars->port2.offset += 32 - oregoffset;
     }
  }

/*
  Use this as a reference for implementing other peripherals
  // Port 1
  SmpcRegs->OREG[0] = 0xF1; //Port Status(Directly Connected)
  SmpcRegs->OREG[1] = 0xE3; //PeripheralID(Shuttle Mouse)
  SmpcRegs->OREG[2] = 0x00; //First Data
  SmpcRegs->OREG[3] = 0x00; //Second Data
  SmpcRegs->OREG[4] = 0x00; //Third Data

  // Port 2
  SmpcRegs->OREG[5] = 0xF0; //Port Status(Not Connected)
*/
}

//////////////////////////////////////////////////////////////////////////////

static void SmpcINTBACK(void) {
  if (SmpcInternalVars->firstPeri == 1) {
     //in a continous mode.
      SmpcINTBACKPeripheral();
      SmpcRegs->SF = 0;
      SmpcRegs->OREG[31] = 0x10;
      ScuSendSystemManager();
      return;
  }
  if (SmpcRegs->IREG[0] != 0x0) {
      // Return non-peripheral data
      SmpcInternalVars->firstPeri = ((SmpcRegs->IREG[1] & 0x8) >> 3);
      for(int i=0;i<31;i++) SmpcRegs->OREG[i] = 0xff;
      m_pmode = (SmpcRegs->IREG[0]>>4);
      SmpcINTBACKStatus();
      SmpcRegs->SR = 0x40 | (SmpcInternalVars->firstPeri << 5); // the low nibble is undefined(or 0xF)
      SmpcRegs->SF = 0;
      ScuSendSystemManager();
      return;
  }
  if (SmpcRegs->IREG[1] & 0x8) {
      SmpcInternalVars->firstPeri = ((SmpcRegs->IREG[1] & 0x8) >> 3);
      SmpcINTBACKPeripheral();
      SmpcRegs->OREG[31] = 0x10;
      ScuSendSystemManager();
      SmpcRegs->SF = 0;
  }
  else {
    SMPCLOG("Nothing to do\n");
    SmpcRegs->OREG[31] = 0x10;
    SmpcRegs->SF = 0;
  }
}

//////////////////////////////////////////////////////////////////////////////

static void SmpcSETSMEM(void) {
   int i;

   for(i = 0;i < 4;i++)
      SmpcInternalVars->SMEM[i] = SmpcRegs->IREG[i];

   // language might have changed, let's store the new id
   SmpcInternalVars->languageid = SmpcInternalVars->SMEM[3];

   SmpcRegs->OREG[31] = 0x17;
}

//////////////////////////////////////////////////////////////////////////////

static void SmpcNMIREQ(void) {
   SH2SendInterrupt(MSH2, 0xB, 16);
   SmpcRegs->OREG[31] = 0x18;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcResetButton(void) {
   // If RESD isn't set, send an NMI request to the MSH2.
   if (SmpcInternalVars->resd)
      return;

   SH2SendInterrupt(MSH2, 0xB, 16);
}

//////////////////////////////////////////////////////////////////////////////

static void SmpcRESENAB(void) {
  SmpcInternalVars->resd = 0;
  SmpcRegs->OREG[31] = 0x19;
}

//////////////////////////////////////////////////////////////////////////////

static void SmpcRESDISA(void) {
  SmpcInternalVars->resd = 1;
  SmpcRegs->OREG[31] = 0x1A;
}

//////////////////////////////////////////////////////////////////////////////
static void processCommand(void) {
  intback_wait_for_line = 0;
  switch(SmpcRegs->COMREG) {
     case 0x0:
        SMPCLOG("smpc\t: MSHON not implemented\n");
        SmpcRegs->OREG[31]=0x0;
        SmpcRegs->SF = 0;
        break;
     case 0x2:
        SMPCLOG("smpc\t: SSHON\n");
        SmpcSSHON();
        SmpcRegs->SF = 0;
        break;
     case 0x3:
        SMPCLOG("smpc\t: SSHOFF\n");
        SmpcSSHOFF();
        SmpcRegs->SF = 0;
        break;
     case 0x6:
        SMPCLOG("smpc\t: SNDON\n");
        SmpcSNDON();
        SmpcRegs->SF = 0;
        break;
     case 0x7:
        SMPCLOG("smpc\t: SNDOFF\n");
        SmpcSNDOFF();
        SmpcRegs->SF = 0;
        break;
     case 0x8:
        SMPCLOG("smpc\t: CDON not implemented\n");
        SmpcRegs->SF = 0;
        break;
     case 0x9:
        SMPCLOG("smpc\t: CDOFF not implemented\n");
        SmpcRegs->SF = 0;
        break;
     case 0xD:
        SMPCLOG("smpc\t: SYSRES not implemented\n");
        SmpcSYSRES();
        SmpcRegs->SF = 0;
        break;
     case 0xE:
        SMPCLOG("smpc\t: CKCHG352\n");
        SmpcCKCHG352();
        SmpcRegs->SF = 0;
        break;
     case 0xF:
        SMPCLOG("smpc\t: CKCHG320\n");
        SmpcCKCHG320();
        SmpcRegs->SF = 0;
        break;
     case 0x10:
        SMPCLOG("smpc\t: INTBACK\n");
        SmpcINTBACK();
        break;
     case 0x17:
        SMPCLOG("smpc\t: SETSMEM\n");
        SmpcSETSMEM();
        SmpcRegs->SF = 0;
        break;
     case 0x18:
        SMPCLOG("smpc\t: NMIREQ\n");
        SmpcNMIREQ();
        SmpcRegs->SF = 0;
        break;
     case 0x19:
        SMPCLOG("smpc\t: RESENAB\n");
        SmpcRESENAB();
        SmpcRegs->SF = 0;
        break;
     case 0x1A:
        SMPCLOG("smpc\t: RESDISA\n");
        SmpcRESDISA();
        SmpcRegs->SF = 0;
        break;
     default:
        printf("smpc\t: Command %02X not implemented\n", SmpcRegs->COMREG);
        break;
  }
}

void SmpcExec(s32 t) {
   if (SmpcInternalVars->timing > 0) {

      if (intback_wait_for_line)
      {
         if (yabsys.LineCount == 207)
         {
            SmpcInternalVars->timing = -1;
            intback_wait_for_line = 0;
         }
      }
      SmpcInternalVars->timing -= t;
      if (SmpcInternalVars->timing <= 0) {
         processCommand();
      }
   }
}

//////////////////////////////////////////////////////////////////////////////



void SmpcINTBACKEnd(void) {
}

//////////////////////////////////////////////////////////////////////////////
static u8 m_pdr2_readback = 0;
static u8 m_pdr1_readback = 0;


u8 FASTCALL SmpcReadByte(SH2_struct *context, u8* mem, u32 addr) {
   addr &= 0x7F;
   if (addr == 0x063) {
     bustmp = SmpcRegsT[addr >> 1] & 0xFE;
     bustmp |= SmpcRegs->SF;
     SMPCLOG("Read SMPC[0x63] 0x%x\n", bustmp);
     return bustmp;
   }

   if ((addr == 0x77) && ((SmpcRegs->DDR[1] & 0x7F) == 0x18)) { //PDR2
     u8 val = (((0x67 & ~0x19) | 0x18 | (eeprom_do_read()<<0)) & ~SmpcRegs->DDR[1]) | m_pdr2_readback;
     return val; //Shall use eeprom normally look at mame stv driver
   }
   if ((addr == 0x75) && ((SmpcRegs->DDR[0] & 0x7F) == 0x3f)) { //PDR1
     u8 val = (((0x40 & 0x40) | 0x3f) & ~SmpcRegs->DDR[0]) | m_pdr1_readback;
     return val;
   }

   if ((addr >= 0x21) && (addr <= 0x5D)) { //OREG[0-30]
     if ((SmpcRegs->SF == 1) && (SmpcRegs->COMREG == 0x10)){
       //Output register [0-30] are read but a intback command is pending
       //Force the command processing
       processCommand();
     }
   }

   if ((addr == 0x5F) && (addr <= 0x5D)) { //OREG[31]
     if (SmpcRegs->SF == 1){
       //Output register 31 is read but a command is pending
       //Force the command processing
       processCommand();
     }
   }


   SMPCLOG("Read SMPC[0x%x] = 0x%x\n",addr, SmpcRegsT[addr >> 1]);
   return SmpcRegsT[addr >> 1];
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL SmpcReadWord(SH2_struct *context, UNUSED u8* mem, USED_IF_SMPC_DEBUG u32 addr) {
   // byte access only
   SMPCLOG("smpc\t: SMPC register read word - %08X\n", addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL SmpcReadLong(SH2_struct *context, u8* mem, USED_IF_SMPC_DEBUG u32 addr) {
   // byte access only
   SMPCLOG("smpc\t: SMPC register read long - %08X\n", addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static void SmpcSetTiming(void) {
   switch(SmpcRegs->COMREG) {
      case 0x0:
         SMPCLOG("smpc\t: MSHON not implemented\n");
         SmpcInternalVars->timing = 1;
         return;
      case 0x8:
         SMPCLOG("smpc\t: CDON not implemented\n");
         SmpcInternalVars->timing = 1;
         return;
      case 0x9:
         SMPCLOG("smpc\t: CDOFF not implemented\n");
         SmpcInternalVars->timing = 1;
         return;
      case 0xD:
      case 0xE:
      case 0xF:
         SmpcInternalVars->timing = 1; // this has to be tested on a real saturn
         return;
      case 0x10:
          if (SmpcInternalVars->firstPeri == 1) {
            SmpcInternalVars->timing = 16000;
            intback_wait_for_line = 1;
          } else {
            // Calculate timing based on what data is being retrieved

            if ((SmpcRegs->IREG[0] == 0x01) && (SmpcRegs->IREG[1] & 0x8))
            {
               //status followed by peripheral data
               SmpcInternalVars->timing = 250;
            }
            else if ((SmpcRegs->IREG[0] == 0x01) && ((SmpcRegs->IREG[1] & 0x8) == 0))
            {
               //status only
               SmpcInternalVars->timing = 250;
            }
            else if ((SmpcRegs->IREG[0] == 0) && (SmpcRegs->IREG[1] & 0x8))
            {
               //peripheral only
               SmpcInternalVars->timing = 16000;
               intback_wait_for_line = 1;
            }
            else SmpcInternalVars->timing = 1;
         }
         return;
      case 0x17:
         SmpcInternalVars->timing = 1;
         return;
      case 0x2:
         SmpcInternalVars->timing = 1;
         return;
      case 0x3:
         SmpcInternalVars->timing = 1;                        
         return;
      case 0x6:
      case 0x7:
      case 0x18:
      case 0x19:
      case 0x1A:
         SmpcInternalVars->timing = 1;
         return;
      default:
         SMPCLOG("smpc\t: unimplemented command: %02X\n", SmpcRegs->COMREG);
         SmpcRegs->SF = 0;
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

//acquiring megadrive id
//world heroes perfect wants to find a saturn pad
//id = 0xb
u8 do_th_mode(u8 val)
{
   switch (val & 0x40) {
   case 0x40:
      return 0x70 | ((PORTDATA1.data[3] & 0xF) & 0xc);
      break;
   case 0x00:
      return 0x30 | ((PORTDATA1.data[2] >> 4) & 0xf);
      break;
   }

   //should not happen
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL SmpcWriteByte(SH2_struct *context, u8* mem, u32 addr, u8 val) {
   u8 oldVal;
   if(!(addr & 0x1)) return;
   addr &= 0x7F;
   oldVal = SmpcRegsT[addr >> 1];
   bustmp = val;
   if (addr == 0x1F) {
      //COMREG 
      SmpcRegsT[0xF] = val&0x1F;
   } else
     SmpcRegsT[addr >> 1] = val;

      SMPCLOG("Write SMPC[0x%x] = 0x%x\n",addr, SmpcRegsT[addr >> 1]);

   switch(addr) {
      case 0x01: // Maybe an INTBACK continue/break request
         if (SmpcInternalVars->firstPeri != 0)
         {
            if (SmpcRegs->IREG[0] & 0x40) {
               // Break
               SMPCLOG("INTBACK Break\n");
               SmpcInternalVars->firstPeri = 0;
               SmpcRegs->SR &= 0x0F;
               SmpcRegs->SF = 0;
               break;
            }
            else if (SmpcRegs->IREG[0] & 0x80) {                    
               // Continue
               SMPCLOG("INTBACK Continue\n");
               SmpcSetTiming();
               SmpcRegs->SF = 1;
            }
         }
         return;
      case 0x1F:
         SmpcSetTiming();
         return;
      case 0x63:
         SmpcRegs->SF &= val;
         return;
      case 0x75: // PDR1
         // FIX ME (should support other peripherals)
         switch (SmpcRegs->DDR[0] & 0x7F) { // Which Control Method do we use?
            case 0x00:
               if (PORTDATA1.data[1] == PERGUN && (val & 0x7F) == 0x7F)
                  SmpcRegs->PDR[0] = PORTDATA1.data[2];
               break;
            //th control mode (acquire id)
            case 0x40:
               SmpcRegs->PDR[0] = do_th_mode(val);
               break;
            //th tr control mode
            case 0x60:
               switch (val & 0x60) {
                  case 0x60: // 1st Data
                     val = (val & 0x80) | 0x14 | (PORTDATA1.data[3] & 0x8);
                     break;
                  case 0x20: // 2nd Data
                     val = (val & 0x80) | 0x10 | ((PORTDATA1.data[2] >> 4) & 0xF);
                     break;
                  case 0x40: // 3rd Data
                     val = (val & 0x80) | 0x10 | (PORTDATA1.data[2] & 0xF);
                     break;
                  case 0x00: // 4th Data
                     val = (val & 0x80) | 0x10 | ((PORTDATA1.data[3] >> 4) & 0xF);
                     break;
                  default: break;
               }

               SmpcRegs->PDR[0] = val;
               break;
            case 0x3f:
               m_pdr1_readback = (val & SmpcRegs->DDR[0]) & 0x7f;
	       eeprom_set_clk((val & 0x08) ? 1 : 0);
	       eeprom_set_di((val >> 4) & 1);
	       eeprom_set_cs((val & 0x04) ? 1 : 0);
               SmpcRegs->PDR[0] = m_pdr1_readback;
               m_pdr1_readback |= (val & 0x80);
               break;
            default:
               SMPCLOG("smpc\t: PDR1 Peripheral Unknown Control Method not implemented 0x%x\n", SmpcRegs->DDR[0] & 0x7F);
               break;
         }
	break;
	  case 0x77: // PDR2
		  // FIX ME (should support other peripherals)
		  switch (SmpcRegs->DDR[1] & 0x7F) { // Which Control Method do we use?
		  case 0x00:
			  if (PORTDATA2.data[1] == PERGUN && (val & 0x7F) == 0x7F)
				  SmpcRegs->PDR[1] = PORTDATA2.data[2];
			  break;
		  case 0x60:
			  switch (val & 0x60) {
			  case 0x60: // 1st Data
				  val = (val & 0x80) | 0x14 | (PORTDATA2.data[3] & 0x8);
				  break;
			  case 0x20: // 2nd Data
				  val = (val & 0x80) | 0x10 | ((PORTDATA2.data[2] >> 4) & 0xF);
				  break;
			  case 0x40: // 3rd Data
				  val = (val & 0x80) | 0x10 | (PORTDATA2.data[2] & 0xF);
				  break;
			  case 0x00: // 4th Data
				  val = (val & 0x80) | 0x10 | ((PORTDATA2.data[3] >> 4) & 0xF);
				  break;
			  default: break;
			  }

			  SmpcRegs->PDR[1] = val;
			  break;
                  case 0x18:
                          m_pdr2_readback = ((val & SmpcRegs->DDR[1] ) & 0x7F);
	                  if (m_pdr2_readback & 0x10){
                            M68KStop();
                          } else {
                            M68KStart();
                          }
                          SmpcRegs->PDR[1] = m_pdr2_readback;
	                  m_pdr2_readback |= val & 0x80;
                          break;
		  default:
			  SMPCLOG("smpc\t: PDR2 Peripheral Unknown Control Method not implemented 0x%x\n", SmpcRegs->DDR[1] & 0x7F);
			  break;
		  }
		  break;
	  case 0x79: // DDR1
         switch (SmpcRegs->DDR[0] & 0x7F) { // Which Control Method do we use?
            case 0x00: // Low Nibble of Peripheral ID
            case 0x40: // High Nibble of Peripheral ID
               switch (PORTDATA1.data[0])
               {
                  case 0xA0:
                  {
                     if (PORTDATA1.data[1] == PERGUN)
                        SmpcRegs->PDR[0] = 0x7C;
                           break;
                  }
                  case 0xF0:
                     SmpcRegs->PDR[0] = 0x7F;
                     break;
                  case 0xF1:
                  {
                     switch(PORTDATA1.data[1])
                     {
                        case PERPAD:
                           SmpcRegs->PDR[0] = 0x7C;
                           break;
                        case PER3DPAD:
                        case PERKEYBOARD:
                           SmpcRegs->PDR[0] = 0x71;
                           break;
                        case PERMOUSE:
                           SmpcRegs->PDR[0] = 0x70;
                           break;
                        case PERWHEEL:
                        case PERMISSIONSTICK:
                        case PERTWINSTICKS:
                        default: 
                           SMPCLOG("smpc\t: Peripheral TH Control Method not supported for peripherl id %02X\n", PORTDATA1.data[1]);
                           break;
                     }
                     break;
                  }
                  default: 
                     SmpcRegs->PDR[0] = 0x71;
                     break;
               }

               break;
            default: break;
         }
         break;
         case 0x7B: // DDR2
            SmpcRegs->DDR[1] = (val & 0x7F);
         break;
	  case 0x7D: // IOSEL
		  SmpcRegs->IOSEL = val;
		  break;
	  case 0x7F: // EXLE
		  SmpcRegs->EXLE = val;
		  break;
      default:
         return;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL SmpcWriteWord(SH2_struct *context, UNUSED u8* mem, USED_IF_SMPC_DEBUG u32 addr, UNUSED u16 val) {
   // byte access only
   SMPCLOG("smpc\t: SMPC register write word - %08X\n", addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL SmpcWriteLong(SH2_struct *context, UNUSED u8* mem, USED_IF_SMPC_DEBUG u32 addr, UNUSED u32 val) {
   // byte access only
   SMPCLOG("smpc\t: SMPC register write long - %08X\n", addr);
}

//////////////////////////////////////////////////////////////////////////////

int SmpcSaveState(void ** stream)
{
   int offset;

   offset = MemStateWriteHeader(stream, "SMPC", 3);

   // Write registers
   MemStateWrite((void *)SmpcRegs->IREG, sizeof(u8), 7, stream);
   MemStateWrite((void *)&SmpcRegs->COMREG, sizeof(u8), 1, stream);
   MemStateWrite((void *)SmpcRegs->OREG, sizeof(u8), 32, stream);
   MemStateWrite((void *)&SmpcRegs->SR, sizeof(u8), 1, stream);
   MemStateWrite((void *)&SmpcRegs->SF, sizeof(u8), 1, stream);
   MemStateWrite((void *)SmpcRegs->PDR, sizeof(u8), 2, stream);
   MemStateWrite((void *)SmpcRegs->DDR, sizeof(u8), 2, stream);
   MemStateWrite((void *)&SmpcRegs->IOSEL, sizeof(u8), 1, stream);
   MemStateWrite((void *)&SmpcRegs->EXLE, sizeof(u8), 1, stream);

   // Write internal variables
   MemStateWrite((void *)SmpcInternalVars, sizeof(SmpcInternal), 1, stream);

   // Write ID's of currently emulated peripherals(fix me)

   return MemStateFinishHeader(stream, offset);
}

//////////////////////////////////////////////////////////////////////////////

int SmpcLoadState(const void * stream, int version, int size)
{
   int internalsizev2 = sizeof(SmpcInternal) - 8;

   // Read registers
   MemStateRead((void *)SmpcRegs->IREG, sizeof(u8), 7, stream);
   MemStateRead((void *)&SmpcRegs->COMREG, sizeof(u8), 1, stream);
   MemStateRead((void *)SmpcRegs->OREG, sizeof(u8), 32, stream);
   MemStateRead((void *)&SmpcRegs->SR, sizeof(u8), 1, stream);
   MemStateRead((void *)&SmpcRegs->SF, sizeof(u8), 1, stream);
   MemStateRead((void *)SmpcRegs->PDR, sizeof(u8), 2, stream);
   MemStateRead((void *)SmpcRegs->DDR, sizeof(u8), 2, stream);
   MemStateRead((void *)&SmpcRegs->IOSEL, sizeof(u8), 1, stream);
   MemStateRead((void *)&SmpcRegs->EXLE, sizeof(u8), 1, stream);

   // Read internal variables
   if (version == 1)
   {
      // This handles the problem caused by the version not being incremented
      // when SmpcInternal was changed
      if ((size - 48) == internalsizev2)
         MemStateRead((void *)SmpcInternalVars, internalsizev2, 1, stream);
      else if ((size - 48) == 24)
         MemStateRead((void *)SmpcInternalVars, 24, 1, stream);
      else
         MemStateSetOffset(size - 48);
   }
   else if (version == 2)
      MemStateRead((void *)SmpcInternalVars, internalsizev2, 1, stream);
   else
      MemStateRead((void *)SmpcInternalVars, sizeof(SmpcInternal), 1, stream);

   // Read ID's of currently emulated peripherals(fix me)

   return size;
}

//////////////////////////////////////////////////////////////////////////////

