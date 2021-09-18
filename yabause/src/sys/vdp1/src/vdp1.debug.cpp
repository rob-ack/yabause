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

#include <cstdlib>
#include <string>
#include <vector>

#include "vdp1.h"
#include "vdp1.private.h"
#include "debug.h"
#include "threads.h"
#include "videoInterface.h"
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
#include "ygl.h"
#endif

extern "C" void FASTCALL Vdp1ReadCommand(vdp1cmd_struct * cmd, u32 addr, u8 * ram);

//////////////////////////////////////////////////////////////////////////////

static u32 ColorRamGetColor(u32 colorindex)
{
    switch (Vdp2Internal.ColorMode)
    {
    case 0:
    case 1:
    {
        u32 tmp;
        colorindex <<= 1;
        tmp = T2ReadWord(Vdp2ColorRam, colorindex & 0xFFF);
        return SAT2YAB1(0xFF, tmp);
    }
    case 2:
    {
        u32 tmp1, tmp2;
        colorindex <<= 2;
        colorindex &= 0xFFF;
        tmp1 = T2ReadWord(Vdp2ColorRam, colorindex);
        tmp2 = T2ReadWord(Vdp2ColorRam, colorindex + 2);
        return SAT2YAB2(0xFF, tmp1, tmp2);
    }
    default: break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE int CheckEndcode(int dot, int endcode, int* code)
{
    if (dot == endcode)
    {
        code[0]++;
        if (code[0] == 2)
        {
            code[0] = 0;
            return 2;
        }
        return 1;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE int DoEndcode(int count, u32* charAddr, u32** textdata, int width, int xoff, int oddpixel, int pixelsize)
{
    if (count > 1)
    {
        float divisor = (float)(8 / pixelsize);

        if (divisor != 0)
            charAddr[0] += (int)((float)(width - xoff + oddpixel) / divisor);
        memset(textdata[0], 0, sizeof(u32) * (width - xoff));
        textdata[0] += (width - xoff);
        return 1;
    }
    else
        *textdata[0]++ = 0;

    return 0;
}

static void printCommand(vdp1cmd_struct *cmd) {
  printf("===== CMD =====\n");
  printf("CMDCTRL = 0x%x\n",cmd->CMDCTRL.all );
  printf("CMDLINK = 0x%x\n",cmd->CMDLINK );
  printf("CMDPMOD = 0x%x\n",cmd->CMDPMOD );
  printf("CMDCOLR = 0x%x\n",cmd->CMDCOLR );
  printf("CMDSRCA = 0x%x\n",cmd->CMDSRCA );
  printf("CMDSIZE = 0x%x\n",cmd->CMDSIZE );
  printf("CMDXA = 0x%x\n",cmd->CMDXA );
  printf("CMDYA = 0x%x\n",cmd->CMDYA );
  printf("CMDXB = 0x%x\n",cmd->CMDXB );
  printf("CMDYB = 0x%x\n",cmd->CMDYB );
  printf("CMDXC = 0x%x\n",cmd->CMDXC );
  printf("CMDYC = 0x%x\n",cmd->CMDYC );
  printf("CMDXD = 0x%x\n",cmd->CMDXD );
  printf("CMDYD = 0x%x\n",cmd->CMDYD );
  printf("CMDGRDA = 0x%x\n",cmd->CMDGRDA );
}

//////////////////////////////////////////////////////////////////////////////

static u32 Vdp1DebugGetCommandNumberAddr(u32 number)
{
   u32 addr = 0;
   u32 returnAddr = 0xFFFFFFFF;
   u32 commandCounter = 0;
   u16 command = T1ReadWord(Vdp1Ram, addr);

   while (!(command & 0x8000) && commandCounter != number)
   {
      // Make sure we're still dealing with a valid command
      if ((command & 0x000C) == 0x000C)
         // Invalid, abort
         return 0xFFFFFFFF;

      // Determine where to go next
      switch ((command & 0x3000) >> 12)
      {
         case 0: // NEXT, jump to following table
            addr += 0x20;
            break;
         case 1: // ASSIGN, jump to CMDLINK
            addr = T1ReadWord(Vdp1Ram, addr + 2) * 8;
            break;
         case 2: // CALL, call a subroutine
            if (returnAddr == 0xFFFFFFFF)
               returnAddr = addr + 0x20;

            addr = T1ReadWord(Vdp1Ram, addr + 2) * 8;
            break;
         case 3: // RETURN, return from subroutine
            if (returnAddr != 0xFFFFFFFF) {
               addr = returnAddr;
               returnAddr = 0xFFFFFFFF;
            }
            else
               addr += 0x20;
            break;
      }

      if (addr > 0x7FFE0)
         return 0xFFFFFFFF;
      command = T1ReadWord(Vdp1Ram, addr);
      commandCounter++;
   }

   if (commandCounter == number)
      return addr;
   else
      return 0xFFFFFFFF;
}

//////////////////////////////////////////////////////////////////////////////

char const * const Vdp1DebugGetCommandName(u16 const command)
{
    if (command & 0x8000)
        return "Draw End";

    // Figure out command name
    switch (command & 0x000F)
    {
    case 0:
        return "Normal Sprite";
    case 1:
        return "Scaled Sprite";
    case 2:
        return "Distorted Sprite";
    case 3:
        return "Distorted Sprite *";
    case 4:
        return "Polygon";
    case 5:
        return "Polyline";
    case 6:
        return "Line";
    case 7:
        return "Polyline *";
    case 8:
        return "User Clipping Coordinates";
    case 9:
        return "System Clipping Coordinates";
    case 10:
        return "Local Coordinates";
    case 11:
        return "User Clipping Coordinates *";
    default:
        return "Bad command";
    }
}

char const* const Vdp1DebugGetCommandNumberName(u32 number)
{
   u32 addr;

   if ((addr = Vdp1DebugGetCommandNumberAddr(number)) != 0xFFFFFFFF)
   {
       u16 const command = T1ReadWord(Vdp1Ram, addr);
       return Vdp1DebugGetCommandName(command);
   }
   else
      return nullptr;
}

//////////////////////////////////////////////////////////////////////////////
void Vdp1DebugCommandToString(vdp1cmd_struct const & cmd, char * outstring);

void Vdp1DebugCommand(u32 number, char* outstring)
{
    u32 addr;
    if ((addr = Vdp1DebugGetCommandNumberAddr(number)) == 0xFFFFFFFF)
        return;

    u16 command = T1ReadWord(Vdp1Ram, addr);

    if (command & 0x8000)
    {
        // Draw End
        outstring[0] = 0x00;
        return;
    }

    if (command & 0x4000)
    {
        AddString(outstring, "Command is skipped\r\n");
        return;
    }

    vdp1cmd_struct cmd;
    Vdp1ReadCommand(&cmd, addr, Vdp1Ram);
    Vdp1DebugCommandToString(cmd, outstring);
}

void Vdp1DebugCommandToString(vdp1cmd_struct const & cmd_, char * outstring)
{
    vdp1cmd_struct cmd = cmd_;
   if ((cmd.CMDCTRL.part.Comm) < 4) {
     int w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
     int h = cmd.CMDSIZE & 0xFF;
   }

   if ((cmd.CMDYA & 0x400)) cmd.CMDYA |= 0xFC00; else cmd.CMDYA &= ~(0xFC00);
   if ((cmd.CMDYC & 0x400)) cmd.CMDYC |= 0xFC00; else cmd.CMDYC &= ~(0xFC00);
   if ((cmd.CMDYB & 0x400)) cmd.CMDYB |= 0xFC00; else cmd.CMDYB &= ~(0xFC00);
   if ((cmd.CMDYD & 0x400)) cmd.CMDYD |= 0xFC00; else cmd.CMDYD &= ~(0xFC00);

   if ((cmd.CMDXA & 0x400)) cmd.CMDXA |= 0xFC00; else cmd.CMDXA &= ~(0xFC00);
   if ((cmd.CMDXC & 0x400)) cmd.CMDXC |= 0xFC00; else cmd.CMDXC &= ~(0xFC00);
   if ((cmd.CMDXB & 0x400)) cmd.CMDXB |= 0xFC00; else cmd.CMDXB &= ~(0xFC00);
   if ((cmd.CMDXD & 0x400)) cmd.CMDXD |= 0xFC00; else cmd.CMDXD &= ~(0xFC00);

   switch (cmd.CMDCTRL.part.Comm)
   {
      case 0:
         AddString(outstring, "Normal Sprite\r\n");
         AddString(outstring, "x = %d, y = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA);
         break;
      case 1:
         AddString(outstring, "Scaled Sprite\r\n");

         AddString(outstring, "Zoom Point: ");

         switch (cmd.CMDCTRL.part.ZP)
         {
            case 0x0:
               AddString(outstring, "Only two coordinates\r\n");
               break;
            case 0x5:
               AddString(outstring, "Upper-left\r\n");
               break;
            case 0x6:
               AddString(outstring, "Upper-center\r\n");
               break;
            case 0x7:
               AddString(outstring, "Upper-right\r\n");
               break;
            case 0x9:
               AddString(outstring, "Center-left\r\n");
               break;
            case 0xA:
               AddString(outstring, "Center-center\r\n");
               break;
            case 0xB:
               AddString(outstring, "Center-right\r\n");
               break;
            case 0xC:
               AddString(outstring, "Lower-left\r\n");
               break;
            case 0xE:
               AddString(outstring, "Lower-center\r\n");
               break;
            case 0xF:
               AddString(outstring, "Lower-right\r\n");
               break;
            default: break;
         }

         if (cmd.CMDCTRL.part.ZP == 0)
         {
            AddString(outstring, "xa = %d, ya = %d, xc = %d, yc = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA, (s16)cmd.CMDXC, (s16)cmd.CMDYC);
         }
         else
         {
            AddString(outstring, "xa = %d, ya = %d, xb = %d, yb = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA, (s16)cmd.CMDXB, (s16)cmd.CMDYB);
         }

         break;
      case 2:
         AddString(outstring, "Distorted Sprite\r\n");
         AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA, (s16)cmd.CMDXB, (s16)cmd.CMDYB);
         AddString(outstring, "x3 = %d, y3 = %d, x4 = %d, y4 = %d\r\n", (s16)cmd.CMDXC, (s16)cmd.CMDYC, (s16)cmd.CMDXD, (s16)cmd.CMDYD);
         break;
      case 3:
         AddString(outstring, "Distorted Sprite *\r\n");
         AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA, (s16)cmd.CMDXB, (s16)cmd.CMDYB);
         AddString(outstring, "x3 = %d, y3 = %d, x4 = %d, y4 = %d\r\n", (s16)cmd.CMDXC, (s16)cmd.CMDYC, (s16)cmd.CMDXD, (s16)cmd.CMDYD);
         break;
      case 4:
         AddString(outstring, "Polygon\r\n");
         AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA, (s16)cmd.CMDXB, (s16)cmd.CMDYB);
         AddString(outstring, "x3 = %d, y3 = %d, x4 = %d, y4 = %d\r\n", (s16)cmd.CMDXC, (s16)cmd.CMDYC, (s16)cmd.CMDXD, (s16)cmd.CMDYD);
         break;
      case 5:
         AddString(outstring, "Polyline\r\n");
         AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA, (s16)cmd.CMDXB, (s16)cmd.CMDYB);
         AddString(outstring, "x3 = %d, y3 = %d, x4 = %d, y4 = %d\r\n", (s16)cmd.CMDXC, (s16)cmd.CMDYC, (s16)cmd.CMDXD, (s16)cmd.CMDYD);
         break;
      case 6:
         AddString(outstring, "Line\r\n");
         AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA, (s16)cmd.CMDXB, (s16)cmd.CMDYB);
         break;
      case 7:
         AddString(outstring, "Polyline *\r\n");
         AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA, (s16)cmd.CMDXB, (s16)cmd.CMDYB);
         AddString(outstring, "x3 = %d, y3 = %d, x4 = %d, y4 = %d\r\n", (s16)cmd.CMDXC, (s16)cmd.CMDYC, (s16)cmd.CMDXD, (s16)cmd.CMDYD);
         break;
      case 8:
         AddString(outstring, "User Clipping\r\n");
         AddString(outstring, "x1 = %d, y1 = %d, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA, (s16)cmd.CMDXC, (s16)cmd.CMDYC);
         break;
      case 9:
         AddString(outstring, "System Clipping\r\n");
         AddString(outstring, "x1 = 0, y1 = 0, x2 = %d, y2 = %d\r\n", (s16)cmd.CMDXC, (s16)cmd.CMDYC);
         break;
      case 10:
         AddString(outstring, "Local Coordinates\r\n");
         AddString(outstring, "x = %d, y = %d\r\n", (s16)cmd.CMDXA, (s16)cmd.CMDYA);
         break;
      default:
         AddString(outstring, "Invalid command\r\n");
         return;
   }

   // Only Sprite commands use CMDSRCA, CMDSIZE
   if (!(cmd.CMDCTRL.all & 0x000C))
   {
      AddString(outstring, "Texture address = %08X\r\n", ((unsigned int)cmd.CMDSRCA) << 3);
      AddString(outstring, "Texture width = %d, height = %d\r\n", MAX(1, (cmd.CMDSIZE & 0x3F00) >> 5), MAX(1,cmd.CMDSIZE & 0xFF));
      if ((((cmd.CMDSIZE & 0x3F00) >> 5)==0) || ((cmd.CMDSIZE & 0xFF)==0)) AddString(outstring, "Texture malformed \r\n");
      AddString(outstring, "Texture read direction: ");

      switch (cmd.CMDCTRL.part.Dir)
      {
         case 0:
            AddString(outstring, "Normal\r\n");
            break;
         case 1:
            AddString(outstring, "Reversed horizontal\r\n");
            break;
         case 2:
            AddString(outstring, "Reversed vertical\r\n");
            break;
         case 3:
            AddString(outstring, "Reversed horizontal and vertical\r\n");
            break;
         default: break;
      }
   }

   // Only draw commands use CMDPMOD
   if (!(cmd.CMDCTRL.all & 0x0008))
   {
      if (cmd.CMDPMOD & 0x8000)
      {
         AddString(outstring, "MSB set\r\n");
      }

      if (cmd.CMDPMOD & 0x1000)
      {
         AddString(outstring, "High Speed Shrink Enabled\r\n");
      }

      if (!(cmd.CMDPMOD & 0x0800))
      {
         AddString(outstring, "Pre-clipping Enabled\r\n");
      }

      if (cmd.CMDPMOD & 0x0400)
      {
         AddString(outstring, "User Clipping Enabled\r\n");
         AddString(outstring, "Clipping Mode = %d\r\n", (cmd.CMDPMOD >> 9) & 0x1);
      }

      if (cmd.CMDPMOD & 0x0100)
      {
         AddString(outstring, "Mesh Enabled\r\n");
      }

      if (!(cmd.CMDPMOD & 0x0080))
      {
         AddString(outstring, "End Code Enabled\r\n");
      }

      if (!(cmd.CMDPMOD & 0x0040))
      {
         AddString(outstring, "Transparent Pixel Enabled\r\n");
      }

      if (cmd.CMDCTRL.all & 0x0004){
          AddString(outstring, "Non-textured color: %04X\r\n", cmd.CMDCOLR);
      } else {
          AddString(outstring, "Color mode: ");

          switch ((cmd.CMDPMOD >> 3) & 0x7)
          {
             case 0:
                AddString(outstring, "4 BPP(16 color bank)\r\n");
                AddString(outstring, "Color bank: %08X\r\n", (cmd.CMDCOLR));
                break;
             case 1:
                AddString(outstring, "4 BPP(16 color LUT)\r\n");
                AddString(outstring, "Color lookup table: %08X\r\n", (cmd.CMDCOLR));
                break;
             case 2:
                AddString(outstring, "8 BPP(64 color bank)\r\n");
                AddString(outstring, "Color bank: %08X\r\n", (cmd.CMDCOLR));
                break;
             case 3:
                AddString(outstring, "8 BPP(128 color bank)\r\n");
                AddString(outstring, "Color bank: %08X\r\n", (cmd.CMDCOLR));
                break;
             case 4:
                AddString(outstring, "8 BPP(256 color bank)\r\n");
                AddString(outstring, "Color bank: %08X\r\n", (cmd.CMDCOLR));
                break;
             case 5:
                AddString(outstring, "15 BPP(RGB)\r\n");
                break;
             default: break;
          }
        }

      AddString(outstring, "Color Calc. mode: ");

      switch (cmd.CMDPMOD & 0x7)
      {
         case 0:
            AddString(outstring, "Replace\r\n");
            break;
         case 1:
            AddString(outstring, "Cannot overwrite/Shadow\r\n");
            break;
         case 2:
            AddString(outstring, "Half-luminance\r\n");
            break;
         case 3:
            AddString(outstring, "Replace/Half-transparent\r\n");
            break;
         case 4:
            AddString(outstring, "Gouraud Shading\r\n");
            AddString(outstring, "Gouraud Shading Table = %08X\r\n", ((unsigned int)cmd.CMDGRDA) << 3);
            break;
         case 6:
            AddString(outstring, "Gouraud Shading + Half-luminance\r\n");
            AddString(outstring, "Gouraud Shading Table = %08X\r\n", ((unsigned int)cmd.CMDGRDA) << 3);
            break;
         case 7:
            AddString(outstring, "Gouraud Shading/Gouraud Shading + Half-transparent\r\n");
            AddString(outstring, "Gouraud Shading Table = %08X\r\n", ((unsigned int)cmd.CMDGRDA) << 3);
            break;
         default: break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

u32 *Vdp1DebugTexture(u32 number, int *w, int *h)
{
   u16 command;
   vdp1cmd_struct cmd;
   u32 addr;
   u32 *texture;
   u32 charAddr;
   u32 dot;
   u8 SPD;
   u32 alpha;
   u32 *textdata;
   int isendcode=0;
   int code=0;
   int ret;

   if ((addr = Vdp1DebugGetCommandNumberAddr(number)) == 0xFFFFFFFF)
      return NULL;

   command = T1ReadWord(Vdp1Ram, addr);

   if (command & 0x8000)
      // Draw End
      return NULL;

   if (command & 0x4000)
      // Command Skipped
      return NULL;

   Vdp1ReadCommand(&cmd, addr, Vdp1Ram);

   switch (cmd.CMDCTRL.part.Comm)
   {
      case 0: // Normal Sprite
      case 1: // Scaled Sprite
      case 2: // Distorted Sprite
      case 3: // Distorted Sprite *
         w[0] = MAX(1, (cmd.CMDSIZE & 0x3F00) >> 5);
         h[0] = MAX(1, cmd.CMDSIZE & 0xFF);

         if ((texture = (u32 *)malloc(sizeof(u32) * w[0] * h[0])) == NULL)
            return NULL;

         if (!(cmd.CMDPMOD & 0x80))
         {
            isendcode = 1;
            code = 0;
         }
         else
            isendcode = 0;
         break;
      case 4: // Polygon
      case 5: // Polyline
      case 6: // Line
      case 7: // Polyline *
         // Do 1x1 pixel
         w[0] = 1;
         h[0] = 1;
         if ((texture = (u32 *)malloc(sizeof(u32))) == NULL)
            return NULL;

         if (cmd.CMDCOLR & 0x8000)
            texture[0] = SAT2YAB1(0xFF, cmd.CMDCOLR);
         else
            texture[0] = ColorRamGetColor(cmd.CMDCOLR);

         return texture;
      case 8: // User Clipping
      case 9: // System Clipping
      case 10: // Local Coordinates
      case 11: // User Clipping *
         return NULL;
      default: // Invalid command
         return NULL;
   }

   charAddr = cmd.CMDSRCA * 8;
   SPD = ((cmd.CMDPMOD & 0x40) != 0);
   alpha = 0xFF;
   textdata = texture;

   switch((cmd.CMDPMOD >> 3) & 0x7)
   {
      case 0:
      {
         // 4 bpp Bank mode
         u32 colorBank = cmd.CMDCOLR;
         u32 colorOffset = (Vdp2Regs->CRAOFB & 0x70) << 4;
         u16 i;

         for(i = 0;i < h[0];i++)
         {
            u16 j;
            j = 0;
            while(j < w[0])
            {
               dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);

               // Pixel 1
               if (isendcode && (ret = CheckEndcode(dot >> 4, 0xF, &code)) > 0)
               {
                  if (DoEndcode(ret, &charAddr, &textdata, w[0], j, 0, 4))
                     break;
               }
               else
               {
                  if (((dot >> 4) == 0) && !SPD) *textdata++ = 0;
                  else *textdata++ = ColorRamGetColor(((dot >> 4) | colorBank) + colorOffset);
               }

               j += 1;

               // Pixel 2
               if (isendcode && (ret = CheckEndcode(dot & 0xF, 0xF, &code)) > 0)
               {
                  if (DoEndcode(ret, &charAddr, &textdata, w[0], j, 1, 4))
                     break;
               }
               else
               {
                  if (((dot & 0xF) == 0) && !SPD) *textdata++ = 0;
                  else *textdata++ = ColorRamGetColor(((dot & 0xF) | colorBank) + colorOffset);
               }

               j += 1;
               charAddr += 1;
            }
         }
         break;
      }
      case 1:
      {
         // 4 bpp LUT mode
         u32 temp;
         u32 colorLut = cmd.CMDCOLR * 8;
         u16 i;

         for(i = 0;i < h[0];i++)
         {
            u16 j;
            j = 0;
            while(j < w[0])
            {
               dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);

               if (isendcode && (ret = CheckEndcode(dot >> 4, 0xF, &code)) > 0)
               {
                  if (DoEndcode(ret, &charAddr, &textdata, w[0], j, 0, 4))
                     break;
               }
               else
               {
                  if (((dot >> 4) == 0) && !SPD)
                     *textdata++ = 0;
                  else
                  {
                     temp = T1ReadWord(Vdp1Ram, ((dot >> 4) * 2 + colorLut) & 0x7FFFF);
                     if (temp & 0x8000)
                        *textdata++ = SAT2YAB1(0xFF, temp);
                     else
                        *textdata++ = ColorRamGetColor(temp);
                  }
               }

               j += 1;

               if (isendcode && (ret = CheckEndcode(dot & 0xF, 0xF, &code)) > 0)
               {
                  if (DoEndcode(ret, &charAddr, &textdata, w[0], j, 1, 4))
                     break;
               }
               else
               {
                  if (((dot & 0xF) == 0) && !SPD)
                     *textdata++ = 0;
                  else
                  {
                     temp = T1ReadWord(Vdp1Ram, ((dot & 0xF) * 2 + colorLut) & 0x7FFFF);
                     if (temp & 0x8000)
                        *textdata++ = SAT2YAB1(0xFF, temp);
                     else
                        *textdata++ = ColorRamGetColor(temp);
                  }
               }

               j += 1;

               charAddr += 1;
            }
         }
         break;
      }
      case 2:
      {
         // 8 bpp(64 color) Bank mode
         u32 colorBank = cmd.CMDCOLR;
         u32 colorOffset = (Vdp2Regs->CRAOFB & 0x70) << 4;

         u16 i, j;

         for(i = 0;i < h[0];i++)
         {
            for(j = 0;j < w[0];j++)
            {
               dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF) & 0x3F;
               charAddr++;

               if ((dot == 0) && !SPD) *textdata++ = 0;
               else *textdata++ = ColorRamGetColor((dot | colorBank) + colorOffset);
            }
         }
         break;
      }
      case 3:
      {
         // 8 bpp(128 color) Bank mode
         u32 colorBank = cmd.CMDCOLR;
         u32 colorOffset = (Vdp2Regs->CRAOFB & 0x70) << 4;
         u16 i, j;

         for(i = 0;i < h[0];i++)
         {
            for(j = 0;j < w[0];j++)
            {
               dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF) & 0x7F;
               charAddr++;

               if ((dot == 0) && !SPD) *textdata++ = 0;
               else *textdata++ = ColorRamGetColor((dot | colorBank) + colorOffset);
            }
         }
         break;
      }
      case 4:
      {
         // 8 bpp(256 color) Bank mode
         u32 colorBank = cmd.CMDCOLR;
         u32 colorOffset = (Vdp2Regs->CRAOFB & 0x70) << 4;
         u16 i, j;

         for(i = 0;i < h[0];i++)
         {
            for(j = 0;j < w[0];j++)
            {
               dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);
               charAddr++;

               if ((dot == 0) && !SPD) *textdata++ = 0;
               else *textdata++ = ColorRamGetColor((dot | colorBank) + colorOffset);
            }
         }
         break;
      }
      case 5:
      {
         // 16 bpp Bank mode
         u16 i, j;

         for(i = 0;i < h[0];i++)
         {
            for(j = 0;j < w[0];j++)
            {
               dot = T1ReadWord(Vdp1Ram, charAddr & 0x7FFFF);

               if (isendcode && (ret = CheckEndcode(dot, 0x7FFF, &code)) > 0)
               {
                  if (DoEndcode(ret, &charAddr, &textdata, w[0], j, 0, 16))
                     break;
               }
               else
               {
                  //if (!(dot & 0x8000) && (Vdp2Regs->SPCTL & 0x20)) printf("mixed mode\n");
                  if (!(dot & 0x8000) && !SPD) *textdata++ = 0;
                  else *textdata++ = SAT2YAB1(0xFF, dot);
               }

               charAddr += 2;
            }
         }
         break;
      }
      default:
         break;
   }

   return texture;
}

//////////////////////////////////////////////////////////////////////////////

void ToggleVDP1(void)
{
   Vdp1External.disptoggle ^= 1;
}

//////////////////////////////////////////////////////////////////////////////

constexpr unsigned int cmdBufferSize = Vdp1CommandBufferSize;
extern "C" vdp1cmdctrl_struct cmdBufferBeingProcessed[Vdp1CommandBufferSize];
extern "C" int nbCmdToProcess;

int Vdp1DebugCmdBufferPos()
{
    return nbCmdToProcess;
}

vdp1cmdctrl_struct const * debugGetCommandBuffer(int * currentCommandCounterPosInBuffer_out)
{
    *currentCommandCounterPosInBuffer_out = nbCmdToProcess;
    return cmdBufferBeingProcessed;
}

void Vdp1DebugReadCommandFromCommandBufferAtOffset(int offset, char const *& out_string)
{
    int cmdPos;
    auto const buffer = debugGetCommandBuffer(&cmdPos);
    auto const & entry = buffer[(offset + cmdPos) % cmdBufferSize];
    if (entry.completionLine == 0)
    {
        out_string = nullptr;
        return;
    }

    out_string = Vdp1DebugGetCommandName(entry.cmd.CMDCTRL.all);
}

//////////////////////////////////////////////////////////////////////////////
