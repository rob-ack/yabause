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

#ifndef VDP1_PRIVATE_H
#define VDP1_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

// struct for Vdp1 part that shouldn't be saved
typedef struct {
    int disptoggle;
    int manualerase;
    int manualchange;
    int onecyclemode;
    int useVBlankErase;
    int swap_frame_buffer;
    int plot_trigger_line;
    int plot_trigger_done;
    int current_frame;
    int updateVdp1Ram;
    int checkEDSR;
    int status;
} Vdp1External_struct;

extern Vdp1External_struct Vdp1External;

typedef void(*vdp1_hook_fn)(void * data);
extern vdp1_hook_fn vdp1NewCommandsFetchedHook;
extern vdp1_hook_fn vdp1BeforeDrawCallHook;
extern vdp1_hook_fn vdp1DrawCompletedHook;
extern vdp1_hook_fn vdp1FrameCompletedHook;

#ifdef __cplusplus
}
#endif

#endif
