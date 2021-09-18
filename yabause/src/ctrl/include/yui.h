/*  Copyright 2004-2005 Guillaume Duhamel
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

#ifndef YUI_H
#define YUI_H

#ifdef __cplusplus
extern "C" {
#endif

//#define TIMING_SWAP

void YuiMsg(const char *format, ...);

/* If Yabause encounters any fatal errors, it sends the error text to this function */
void YuiErrorMsg(const char *string);

/* profide the framebuffer id to render into */
int YuiGetFB(void);
/* Tells the yui to exchange front and back video buffers. This may end
   up being moved to the Video Core - It is called only when frame has been drawn and dirty. */
void YuiSwapBuffers(void);

/* Tells the yui that a frame loop has been processed - It is
reported for each frame. */
void YuiEndOfFrame(void);

/* need to call before glXXXXX call in a thread */
int YuiUseOGLOnThisThread();

/* Bfore rendering in a thread, it needs to revoke current rendering thread */
int YuiRevokeOGLOnThisThread();

#ifndef TIMING_SWAP
#define YuiTimedSwapBuffers YuiSwapBuffers
#else
void YuiTimedSwapBuffers(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
