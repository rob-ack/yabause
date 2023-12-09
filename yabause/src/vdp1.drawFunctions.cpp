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
/*
        Copyright 2019 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

        YabaSanshiro is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

YabaSanshiro is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <stdlib.h>
#include "yabause.h"
#include "vdp1.h"
#include "debug.h"
#include "scu.h"
#include "vdp2.h"
#include "vidsoft.h"
#include "threads.h"
#include "sh2core.h"
#include <atomic>
#include <condition_variable>
#include <chrono>

#define DEBUG_BAD_COORD //YuiMsg

#define  CONVERTCMD(A) {\
  s32 toto = (A);\
  if (((A)&0x7000) != 0) (A) |= 0xF000;\
  else (A) &= ~0xF800;\
  ((A) = (s32)(s16)(A));\
  if (((A)) < -1024) { DEBUG_BAD_COORD("Bad(-1024) %x (%d, 0x%x)\n", (A), (A), toto);}\
  if (((A)) > 1023) { DEBUG_BAD_COORD("Bad(1023) %x (%d, 0x%x)\n", (A), (A), toto);}\
}

extern "C" int Vdp1NormalSpriteDraw(vdp1cmd_struct* const cmd, u8 * ram, Vdp1 * regs, u8 * back_framebuffer) {
    Vdp2* varVdp2Regs = &Vdp2Lines[0];
    int ret = 1;
    //    if (emptyCmd(cmd)) {
            // damaged data
    //        yabsys.vdp1cycles += 70;
    //        return -1;
    //    }

    if ((cmd->CMDSIZE & 0x8000)) {
        //        yabsys.vdp1cycles += 70;
        regs->EDSR |= 2;
        return -1; // BAD Command
    }
    if (((cmd->CMDPMOD >> 3) & 0x7) > 5) {
        // damaged data
//        yabsys.vdp1cycles += 70;
        return -1;
    }
    cmd->w = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
    cmd->h = cmd->CMDSIZE & 0xFF;
    if ((cmd->w == 0) || (cmd->h == 0)) {
        //        yabsys.vdp1cycles += 70;
        ret = 0;
    }

    cmd->flip = (cmd->CMDCTRL & 0x30) >> 4;
    cmd->priority = 0;

    CONVERTCMD(cmd->CMDXA);
    CONVERTCMD(cmd->CMDYA);
    cmd->CMDXA += regs->localX;
    cmd->CMDYA += regs->localY;

    cmd->CMDXB = cmd->CMDXA + MAX(1, cmd->w);
    cmd->CMDYB = cmd->CMDYA;
    cmd->CMDXC = cmd->CMDXA + MAX(1, cmd->w);
    cmd->CMDYC = cmd->CMDYA + MAX(1, cmd->h);
    cmd->CMDXD = cmd->CMDXA;
    cmd->CMDYD = cmd->CMDYA + MAX(1, cmd->h);

//    int area = abs((cmd->CMDXA * cmd->CMDYB - cmd->CMDXB * cmd->CMDYA) + (cmd->CMDXB * cmd->CMDYC - cmd->CMDXC * cmd->CMDYB) + (cmd->CMDXC * cmd->CMDYD - cmd->CMDXD * cmd->CMDYC) + (cmd->CMDXD * cmd->CMDYA - cmd->CMDXA * cmd->CMDYD)) / 2;
//    switch ((cmd->CMDPMOD >> 3) & 0x7) {
//    case 0:
//    case 1:
//        // 4 pixels per 16 bits
//        area = area >> 2;
//        break;
//    case 2:
//    case 3:
//    case 4:
//        // 2 pixels per 16 bits
//        area = area >> 1;
//        break;
//    default:
//        break;
//    }
    //    yabsys.vdp1cycles += MIN(1000, 70 + (area));

    memset(cmd->G, 0, sizeof(float) * 16);
    if ((cmd->CMDPMOD & 4))
    {
        //        yabsys.vdp1cycles += 232;
        for (int i = 0; i < 4; i++) {
            u16 color2 = T1ReadWord(Vdp1Ram, (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
            cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 3] = 1.0f;
        }
    }

    VIDCore->Vdp1NormalSpriteDraw(cmd, ram, regs, back_framebuffer);
    return ret;
}

extern "C" int Vdp1ScaledSpriteDraw(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs, u8* back_framebuffer) {
    Vdp2* varVdp2Regs = &Vdp2Lines[0];
    s16 rw = 0, rh = 0;
    s16 x, y;
    int ret = 1;

//    if (emptyCmd(cmd)) {
        // damaged data
//        yabsys.vdp1cycles += 70;
//        return -1;
//    }

    cmd->w = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
    cmd->h = cmd->CMDSIZE & 0xFF;
    if ((cmd->w == 0) || (cmd->h == 0)) {
//        yabsys.vdp1cycles += 70;
        ret = 0;
    }

    cmd->flip = (cmd->CMDCTRL & 0x30) >> 4;
    cmd->priority = 0;

    CONVERTCMD(cmd->CMDXA);
    CONVERTCMD(cmd->CMDYA);
    CONVERTCMD(cmd->CMDXB);
    CONVERTCMD(cmd->CMDYB);
    CONVERTCMD(cmd->CMDXC);
    CONVERTCMD(cmd->CMDYC);

    x = cmd->CMDXA;
    y = cmd->CMDYA;
    // Setup Zoom Point
    switch ((cmd->CMDCTRL & 0xF00) >> 8)
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
    cmd->CMDXB = x + rw + regs->localX;
    cmd->CMDYB = y + regs->localY;
    cmd->CMDXC = x + rw + regs->localX;
    cmd->CMDYC = y + rh + regs->localY;
    cmd->CMDXD = x + regs->localX;
    cmd->CMDYD = y + rh + regs->localY;

    // Setup Zoom Point
    switch ((cmd->CMDCTRL & 0xF00) >> 8)
    {
    case 0x0: // Only two coordinates
        if ((s16)cmd->CMDXC > (s16)cmd->CMDXA) { cmd->CMDXB += 1; cmd->CMDXC += 1; }
        else { cmd->CMDXA += 1; cmd->CMDXB += 1; cmd->CMDXC += 1; cmd->CMDXD += 1; }
        if ((s16)cmd->CMDYC > (s16)cmd->CMDYA) { cmd->CMDYC += 1; cmd->CMDYD += 1; }
        else { cmd->CMDYA += 1; cmd->CMDYB += 1; cmd->CMDYC += 1; cmd->CMDYD += 1; }
        break;
    case 0x5: // Upper-left
    case 0x6: // Upper-Center
    case 0x7: // Upper-Right
    case 0x9: // Center-left
    case 0xA: // Center-center
    case 0xB: // Center-right
    case 0xD: // Lower-left
    case 0xE: // Lower-center
    case 0xF: // Lower-right
        cmd->CMDXB += 1;
        cmd->CMDXC += 1;
        cmd->CMDYC += 1;
        cmd->CMDYD += 1;
        break;
    default: break;
    }

//    int area = abs((cmd->CMDXA * cmd->CMDYB - cmd->CMDXB * cmd->CMDYA) + (cmd->CMDXB * cmd->CMDYC - cmd->CMDXC * cmd->CMDYB) + (cmd->CMDXC * cmd->CMDYD - cmd->CMDXD * cmd->CMDYC) + (cmd->CMDXD * cmd->CMDYA - cmd->CMDXA * cmd->CMDYD)) / 2;
//    switch ((cmd->CMDPMOD >> 3) & 0x7) {
//    case 0:
//    case 1:
//        // 4 pixels per 16 bits
//        area = area >> 2;
//        break;
//    case 2:
//    case 3:
//    case 4:
//        // 2 pixels per 16 bits
//        area = area >> 1;
//        break;
//    default:
//        break;
//    }
//    yabsys.vdp1cycles += MIN(1000, 70 + area);

    //gouraud
    memset(cmd->G, 0, sizeof(float) * 16);
    if ((cmd->CMDPMOD & 4))
    {
//        yabsys.vdp1cycles += 232;
        for (int i = 0; i < 4; i++) {
            u16 color2 = T1ReadWord(Vdp1Ram, (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
            cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 3] = 1.0f;
        }
    }

    VIDCore->Vdp1ScaledSpriteDraw(cmd, ram, regs, back_framebuffer);
    return ret;
}

extern "C" int Vdp1DistortedSpriteDraw(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs, u8* back_framebuffer) {
    Vdp2* varVdp2Regs = &Vdp2Lines[0];
    int ret = 1;

//    if (emptyCmd(cmd)) {
//        // damaged data
//        yabsys.vdp1cycles += 70;
//        return 0;
//    }

    cmd->w = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
    cmd->h = cmd->CMDSIZE & 0xFF;
    if ((cmd->w == 0) || (cmd->h == 0)) {
//        yabsys.vdp1cycles += 70;
        ret = 0;
    }

    cmd->flip = (cmd->CMDCTRL & 0x30) >> 4;
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

//    int area = abs((cmd->CMDXA * cmd->CMDYB - cmd->CMDXB * cmd->CMDYA) + (cmd->CMDXB * cmd->CMDYC - cmd->CMDXC * cmd->CMDYB) + (cmd->CMDXC * cmd->CMDYD - cmd->CMDXD * cmd->CMDYC) + (cmd->CMDXD * cmd->CMDYA - cmd->CMDXA * cmd->CMDYD)) / 2;
//    switch ((cmd->CMDPMOD >> 3) & 0x7) {
//    case 0:
//    case 1:
//        // 4 pixels per 16 bits
//        area = area >> 2;
//        break;
//    case 2:
//    case 3:
//    case 4:
//        // 2 pixels per 16 bits
//        area = area >> 1;
//        break;
//    default:
//        break;
//    }
//    yabsys.vdp1cycles += MIN(1000, 70 + (area * 3));

    memset(cmd->G, 0, sizeof(float) * 16);
    if ((cmd->CMDPMOD & 4))
    {
//        yabsys.vdp1cycles += 232;
        for (int i = 0; i < 4; i++) {
            u16 color2 = T1ReadWord(Vdp1Ram, (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
            cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 3] = 1.0f;
        }
    }

    VIDCore->Vdp1DistortedSpriteDraw(cmd, ram, regs, back_framebuffer);
    return ret;
}

extern "C" int Vdp1PolygonDraw(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs, u8* back_framebuffer) {
//    return 0;
    Vdp2* varVdp2Regs = &Vdp2Lines[0];

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

//    int w = (sqrt((cmd->CMDXA - cmd->CMDXB) * (cmd->CMDXA - cmd->CMDXB)) + sqrt((cmd->CMDXD - cmd->CMDXC) * (cmd->CMDXD - cmd->CMDXC))) / 2;
//    int h = (sqrt((cmd->CMDYA - cmd->CMDYD) * (cmd->CMDYA - cmd->CMDYD)) + sqrt((cmd->CMDYB - cmd->CMDYC) * (cmd->CMDYB - cmd->CMDYC))) / 2;
//    yabsys.vdp1cycles += MIN(1000, 16 + (w * h) + (w * 2));

    //gouraud
    memset(cmd->G, 0, sizeof(float) * 16);
    if ((cmd->CMDPMOD & 4))
    {
//        yabsys.vdp1cycles += 232;
        for (int i = 0; i < 4; i++) {
            u16 color2 = T1ReadWord(Vdp1Ram, (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
            cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 3] = 1.0f;
        }
    }
    cmd->priority = 0;
    cmd->w = 1;
    cmd->h = 1;
    cmd->flip = 0;

    VIDCore->Vdp1PolygonDraw(cmd, ram, regs, back_framebuffer);
    return 1;
}

extern "C" int Vdp1PolylineDraw(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs, u8* back_framebuffer) {

    Vdp2* varVdp2Regs = &Vdp2Lines[0];

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
    memset(cmd->G, 0, sizeof(float) * 16);
    if ((cmd->CMDPMOD & 4))
    {
        for (int i = 0; i < 4; i++) {
            u16 color2 = T1ReadWord(Vdp1Ram, (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
            cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 3] = 1.0f;
        }
    }
    VIDCore->Vdp1PolylineDraw(cmd, ram, regs, back_framebuffer);

    return 1;
}

extern "C" int Vdp1LineDraw(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs, u8* back_framebuffer) {
    Vdp2* varVdp2Regs = &Vdp2Lines[0];


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
    memset(cmd->G, 0, sizeof(float) * 16);
    if ((cmd->CMDPMOD & 4))
    {
        for (int i = 0; i < 4; i++) {
            u16 color2 = T1ReadWord(Vdp1Ram, (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
            cmd->G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
            cmd->G[(i << 2) + 3] = 1.0f;
        }
    }
    cmd->priority = 0;
    cmd->w = 1;
    cmd->h = 1;
    cmd->flip = 0;

    VIDCore->Vdp1LineDraw(cmd, ram, regs, back_framebuffer);

    return 1;
}
