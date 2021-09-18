/*  Copyright 2005-2006 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau
    Copyright 2011-2015 Shinya Miyamoto(devmiyax)

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


#include <stdlib.h>
#include <math.h>
#include "ygl.h"
#include "yui.h"
#include "vidshared.h"
#include "debug.h"
#include "error.h"
#include "osdcore.h"
#include "vdp1_compute.h"
#include "yabause.h"

#define YGLDEBUG

extern int GlHeight;
extern int GlWidth;

extern int rebuild_frame_buffer;
extern void vdp1_wait_regenerate(void);

extern int DrawVDP2Screen(Vdp2 *varVdp2Regs, int id);

extern int YglGenFrameBuffer(int force);
extern void YglUpdateVdp2Reg();
extern SpriteMode setupBlend(Vdp2 *varVdp2Regs, int layer);
extern int setupColorMode(Vdp2 *varVdp2Regs, int layer);
extern int setupShadow(Vdp2 *varVdp2Regs, int layer);
extern int setupBlur(Vdp2 *varVdp2Regs, int layer);
extern int YglDrawBackScreen();

extern u32* vdp1_read();
extern void vdp1_write();
extern u32* manualfb;

//////////////////////////////////////////////////////////////////////////////
void YglEraseWriteCSVDP1(int id) {

  float col[4] = {0.0};
  u16 color;
  int priority;
  u32 alpha = 0;
  int status = 0;
  if (_Ygl->vdp1_pbo == 0) return;
  manualfb = NULL;

  _Ygl->vdp1On[id] = 0;
  _Ygl->vdp1_stencil_mode = 0;

  _Ygl->vdp1levels[id].ux1 = 0;
  _Ygl->vdp1levels[id].uy1 = 0;
  _Ygl->vdp1levels[id].ux2 = 0;
  _Ygl->vdp1levels[id].uy2 = 0;
  _Ygl->vdp1levels[id].uclipcurrent = 0;
  _Ygl->vdp1levels[id].blendmode = 0;

  color = Vdp1Regs->EWDR;

  col[0] = (color & 0xFF) / 255.0f;
  col[1] = ((color >> 8) & 0xFF) / 255.0f;

  if (color != 0x0) {
    _Ygl->vdp1On[id] = 1;
    if (((Vdp1Regs->TVMR & 0x1) == 1) && (col[0] != col[1])){
      YuiMsg("Unsupported clear process\n\tin 8 bits upper part of EWDR is for even coordinates and lower part for odd coordinates\n");
    }
  }


  FRAMELOG("YglEraseWriteVDP1xx: clear %d\n", id);
  vdp1_clear(id, col);

  //Get back to drawframe
  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);

}

void YglCSFinsihDraw(void) {
  vdp1_wait_regenerate();
}

//////////////////////////////////////////////////////////////////////////////

void YglCSRenderVDP1(void) {
  FRAMELOG("YglCSRenderVDP1: drawframe =%d %d\n", _Ygl->drawframe, yabsys.LineCount);
  vdp1_compute();
}

void YglFrameChangeCSVDP1(){
  u32 current_drawframe = 0;
  YglCSRenderVDP1();
  current_drawframe = _Ygl->drawframe;
  _Ygl->drawframe = _Ygl->readframe;
  _Ygl->readframe = current_drawframe;
  manualfb = NULL;

  FRAMELOG("YglFrameChangeVDP1: swap drawframe =%d readframe = %d\n", _Ygl->drawframe, _Ygl->readframe);
}

extern int WinS[enBGMAX+1];
extern int WinS_mode[enBGMAX+1];

static void YglSetVDP1FB(int i){
  if (_Ygl->vdp1IsNotEmpty != 0) {
    _Ygl->vdp1On[i] = 1;
    vdp1_set_directFB();
    _Ygl->vdp1IsNotEmpty = 0;
  }
}

static void YglUpdateVDP1FB(void) {
  YglSetVDP1FB(_Ygl->readframe);
}

static int warning = 0;

void YglCSRender(Vdp2 *varVdp2Regs) {

   GLuint cprg=0;
   GLuint srcTexture;
   GLuint VDP1fb[2];
   int nbPass = 0;
   unsigned int i,j;
   double w = 0;
   double h = 0;
   double x = 0;
   double y = 0;
   float col[4] = {0.0f,0.0f,0.0f,0.0f};
   float colopaque[4] = {0.0f,0.0f,0.0f,1.0f};
   int img[6] = {0};
   int lncl[7] = {0};
   int lncl_draw[7] = {0};
   int winS_draw = 0;
   int winS_mode_draw= 0;
   int win0_draw = 0;
   int win0_mode_draw = 0;
   int win1_draw = 0;
   int win1_mode_draw= 0;
   int win_op_draw = 0;
   int drawScreen[enBGMAX];
   SpriteMode mode;
   GLenum DrawBuffers[8]= {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2,GL_COLOR_ATTACHMENT3,GL_COLOR_ATTACHMENT4,GL_COLOR_ATTACHMENT5,GL_COLOR_ATTACHMENT6,GL_COLOR_ATTACHMENT7};
   double dar = (double)GlWidth/(double)GlHeight;
   double par = 4.0/3.0;
   int Intw = (int)(floor((float)GlWidth/(float)_Ygl->width));
   int Inth = (int)(floor((float)GlHeight/(float)_Ygl->height));
   int Int  = 1;
   int modeScreen = _Ygl->stretch;
   #ifndef __LIBRETRO__
   if (yabsys.isRotated) par = 1.0/par;
   #endif
   if (Intw == 0) {
     if (warning == 0) YuiMsg("Window width is too small - Do not use integer scaling or reduce scaling\n");
     warning = 1;
     modeScreen = 0;
     Intw = 1;
   }
   if (Inth == 0) {
     if (warning == 0) YuiMsg("Window height is too small - Do not use integer scaling or reduce scaling\n");
     warning = 1;
     modeScreen = 0;
     Inth = 1;
   }
   Int = (Inth<Intw)?Inth:Intw;

   YglUpdateVDP1FB();

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);

   glBindVertexArray(_Ygl->vao);

   switch(modeScreen) {
     case 0:
       w = (dar>par)?(double)GlHeight*par:GlWidth;
       h = (dar>par)?(double)GlHeight:(double)GlWidth/par;
       x = (GlWidth-w)/2;
       y = (GlHeight-h)/2;
       break;
     case 1:
       w = GlWidth;
       h = GlHeight;
       x = 0;
       y = 0;
       break;
     case 2:
       w = Int * _Ygl->width;
       h = Int * _Ygl->height;
       x = (GlWidth-w)/2;
       y = (GlHeight-h)/2;
       break;
     default:
        break;
    }


   glViewport(0, 0, GlWidth, GlHeight);

   glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);
   glClearBufferfv(GL_COLOR, 0, col);

   YglGenFrameBuffer(0);

    glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->original_fbo);
    glDrawBuffers(NB_RENDER_LAYER, &DrawBuffers[0]);
    //glClearBufferfv(GL_COLOR, 0, col);
#ifdef DEBUG_BLIT
    //glClearBufferfv(GL_COLOR, 1, col);
    //glClearBufferfv(GL_COLOR, 2, col);
    //glClearBufferfv(GL_COLOR, 3, col);
    //glClearBufferfv(GL_COLOR, 4, col);
#endif

   glDepthMask(GL_FALSE);
   glViewport(0, 0, _Ygl->width, _Ygl->height);
   glGetIntegerv( GL_VIEWPORT, _Ygl->m_viewport );
   glScissor(0, 0, _Ygl->width, _Ygl->height);
   glEnable(GL_SCISSOR_TEST);

   //glClearBufferfv(GL_COLOR, 0, colopaque);
   //glClearBufferfi(GL_DEPTH_STENCIL, 0, 0, 0);
   if ((Vdp2Regs->TVMD & 0x8000) == 0) goto render_finish;
   if (YglTM_vdp2 == NULL) goto render_finish;
   glBindTexture(GL_TEXTURE_2D, YglTM_vdp2->textureID);
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

   YglUpdateVdp2Reg();
   if (_Ygl->needWinUpdate) {
     YglSetWindow(0);
     YglSetWindow(1);
     _Ygl->needWinUpdate = 0;
   }
   cprg = -1;

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, YglTM_vdp2->textureID);
   {
	   
  int min = 8;
  int oldPrio = 0;

  int nbPrio = 0;
  int minPrio = -1;
  int allPrio = 0;

  for (int i = 0; i < SPRITE; i++) {
    if (((i == RBG0) || (i == RBG1)) && (_Ygl->rbg_use_compute_shader)) {
      glViewport(0, 0, _Ygl->width, _Ygl->height);
      glScissor(0, 0, _Ygl->width, _Ygl->height);
      glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->rbg_compute_fbo);
      if ( i == RBG0)
        glDrawBuffers(1, &DrawBuffers[0]);
      else
        glDrawBuffers(1, &DrawBuffers[1]);
    } else {
      glViewport(0, 0, _Ygl->rwidth, _Ygl->rheight);
      glScissor(0, 0, _Ygl->rwidth, _Ygl->rheight);
      glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->screen_fbo);
      glDrawBuffers(1, &DrawBuffers[i]);
    }
    drawScreen[i] = DrawVDP2Screen(varVdp2Regs, i);
  }

  const int vdp2screens[] = {RBG0, RBG1, NBG0, NBG1, NBG2, NBG3};

  int prioscreens[6] = {0};
  int modescreens[7] = {0};
  int useLineColorOffset[6] = {0};
  int isRGB[7] = {0};
  int isBlur[7] = {0};
  int isPerline[8] = {0};
  int isShadow[7] = {0};
  glDisable(GL_BLEND);
  int id = 0;

  lncl[0] = (varVdp2Regs->LNCLEN >> 0)&0x1; //NBG0
  lncl[1] = (varVdp2Regs->LNCLEN >> 1)&0x1; //NBG1
  lncl[2] = (varVdp2Regs->LNCLEN >> 2)&0x1; //NBG2
  lncl[3] = (varVdp2Regs->LNCLEN >> 3)&0x1; //NBG3
  lncl[4] = (varVdp2Regs->LNCLEN >> 4)&0x1; //RBG0
  lncl[5] = (varVdp2Regs->LNCLEN >> 0)&0x1; //RBG1
  lncl[6] = (varVdp2Regs->LNCLEN >> 5)&0x1; //SPRITE

  for (int j=0; j<6; j++) {
    if (drawScreen[vdp2screens[j]] != 0) {
      if (((vdp2screens[j] == RBG0) ||(vdp2screens[j] == RBG1)) && (_Ygl->rbg_use_compute_shader)) {
        if (vdp2screens[j] == RBG0)
        prioscreens[id] = _Ygl->rbg_compute_fbotex[0];
        else
        prioscreens[id] = _Ygl->rbg_compute_fbotex[1];
      } else {
        prioscreens[id] = _Ygl->screen_fbotex[vdp2screens[j]];
      }
      if (vdp2screens[j] == RBG0) useLineColorOffset[id] = _Ygl->useLineColorOffset[0];
      if (vdp2screens[j] == RBG1) useLineColorOffset[id] = _Ygl->useLineColorOffset[1];
      modescreens[id] =  setupBlend(varVdp2Regs, vdp2screens[j]);
      isRGB[id] = setupColorMode(varVdp2Regs, vdp2screens[j]);
      isBlur[id] = setupBlur(varVdp2Regs, vdp2screens[j]);
      isPerline[id] = vdp2screens[j];
      isShadow[id] = setupShadow(varVdp2Regs, vdp2screens[j]);
      lncl_draw[id] = lncl[vdp2screens[j]];
      winS_draw |= (WinS[vdp2screens[j]]<<id);
      winS_mode_draw |= (WinS_mode[vdp2screens[j]]<<id);
      win0_draw |= (_Ygl->Win0[vdp2screens[j]]<<id);
      win0_mode_draw |= (_Ygl->Win0_mode[vdp2screens[j]]<<id);
      win1_draw |= (_Ygl->Win1[vdp2screens[j]]<<id);
      win1_mode_draw |= (_Ygl->Win1_mode[vdp2screens[j]]<<id);
      win_op_draw |= (_Ygl->Win_op[vdp2screens[j]]<<id);
      id++;
    }
  }
  isBlur[6] = setupBlur(varVdp2Regs, SPRITE);
  lncl_draw[6] = lncl[6];
  isPerline[6] = 6;
  isPerline[7] = 7;

  for (int i = 6; i < 8; i++) {
    //Update dedicated sprite window and Color calculation window
    winS_draw |= WinS[i]<<i;
    winS_mode_draw |= WinS_mode[i]<<i;
    win0_draw |= _Ygl->Win0[i]<<i;
    win0_mode_draw |= _Ygl->Win0_mode[i]<<i;
    win1_draw |= _Ygl->Win1[i]<<i;
    win1_mode_draw |= _Ygl->Win1_mode[i]<<i;
    win_op_draw |= _Ygl->Win_op[i]<<i;
  }

  isShadow[6] = setupShadow(varVdp2Regs, SPRITE); //Use sprite index for background suuport

  glViewport(0, 0, _Ygl->width, _Ygl->height);
  glGetIntegerv( GL_VIEWPORT, _Ygl->m_viewport );
  glScissor(0, 0, _Ygl->width, _Ygl->height);

  modescreens[6] =  setupBlend(varVdp2Regs, 6);
  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->back_fbo);
  glDrawBuffers(1, &DrawBuffers[0]);
  //glClearBufferfv(GL_COLOR, 0, col);
  if ((varVdp2Regs->BKTAU & 0x8000) != 0) {
    YglDrawBackScreen();
  }else{
    glClearBufferfv(GL_COLOR, 0, _Ygl->clear);
  }

  VDP1fb[0] = get_vdp1_tex();
  VDP1fb[1] = get_vdp1_mesh();
#ifdef __LIBRETRO__
  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);
#else
  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->original_fbo);
#endif
  glDrawBuffers(NB_RENDER_LAYER, &DrawBuffers[0]);
  glClearBufferfi(GL_DEPTH_STENCIL, 0, 0, 0);
  YglBlitTexture( prioscreens, modescreens, isRGB, isBlur, isPerline, isShadow, lncl_draw, VDP1fb, winS_draw, winS_mode_draw, win0_draw, win0_mode_draw, win1_draw, win1_mode_draw, win_op_draw, useLineColorOffset, varVdp2Regs);
  srcTexture = _Ygl->original_fbotex[0];
#ifndef __LIBRETRO__
   glViewport(x, y, w, h);
   glScissor(x, y, w, h);
   glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);
   YglBlitFramebuffer(srcTexture, _Ygl->width, _Ygl->height, w, h);
#endif
   }

render_finish:

  for (int i=0; i<SPRITE; i++)
    YglReset(_Ygl->vdp2levels[i]);
  glViewport(_Ygl->originx, _Ygl->originy, GlWidth, GlHeight);
  glUseProgram(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_STENCIL_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  OSDDisplayMessages(NULL,0,0);

  _Ygl->sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);
  return;
}

static u32* getVdp1DrawingFBMem() {
	if (manualfb == NULL) {
    YglGenFrameBuffer(0);
		vdp1_compute();
	  manualfb = vdp1_read();
	}
	return manualfb;
}


void YglCSVdp1WriteFrameBuffer(u32 type, u32 addr, u32 val )
{
  u16 full = 0;
  _Ygl->vdp1fb_buf =  getVdp1DrawingFBMem();
  switch (type)
  {
  case 0:
    full = T1ReadLong((u8*)_Ygl->vdp1fb_buf, (addr&(~0x1))*2);
    if (addr & 0x1) full = (full & 0xFF00) | (val& 0xFF);
    else full = (full & 0xFF) | ((val& 0xFF) << 8);
    T1WriteLong((u8 *)_Ygl->vdp1fb_buf, (addr&(~0x1))*2, VDP1COLORFB(full&0xFFFF));
    break;
  case 1:
    T1WriteLong((u8*)_Ygl->vdp1fb_buf, addr*2, VDP1COLORFB(val&0xFFFF));
    break;
  case 2:
    T1WriteLong((u8*)_Ygl->vdp1fb_buf, addr*2+4, VDP1COLORFB(val&0xFFFF));
    T1WriteLong((u8*)_Ygl->vdp1fb_buf, addr*2, VDP1COLORFB((val>>16)&0xFFFF));
    break;
  default:
    break;
  }
  _Ygl->vdp1IsNotEmpty = 1;
}

void YglCSVdp1ReadFrameBuffer(u32 type, u32 addr, void * out) {
  _Ygl->vdp1fb_buf_read =  getVdp1DrawingFBMem();
  switch (type)
  {
  case 0:
    *(u8*)out = 0x0;
    break;
  case 1:
    *(u16*)out = T1ReadLong((u8*)_Ygl->vdp1fb_buf_read, addr*2) & 0xFFFF;
    break;
  case 2:
    *(u32*)out = ((T1ReadLong((u8*)_Ygl->vdp1fb_buf_read, addr*2)&0xFFFF)<<16)|((T1ReadLong((u8*)_Ygl->vdp1fb_buf_read, addr*2+4)&0xFFFF));
    break;
  default:
    break;
  }
}
