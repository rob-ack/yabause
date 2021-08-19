#pragma once

#ifndef _VID_INTERFACE_
#define _VID_INTERFACE_

#include "vdp1.h"
#include "vdp2.h"

typedef struct
{
	int id;
	const char* Name;
	int (*Init)(void);
	void (*DeInit)(void);
	void (*Resize)(int, int, unsigned int, unsigned int, int);
	int (*IsFullscreen)(void);
	// VDP1 specific
	int (*Vdp1Reset)(void);
	void (*Vdp1Draw)(void);
	void(*Vdp1NormalSpriteDraw)(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs, u8* back_framebuffer);
	void(*Vdp1ScaledSpriteDraw)(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs, u8* back_framebuffer);
	void(*Vdp1DistortedSpriteDraw)(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs, u8* back_framebuffer);
	void(*Vdp1PolygonDraw)(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs, u8* back_framebuffer);
	void(*Vdp1PolylineDraw)(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs, u8* back_framebuffer);
	void(*Vdp1LineDraw)(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs, u8* back_framebuffer);
	void(*Vdp1UserClipping)(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs);
	void(*Vdp1SystemClipping)(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs);
	void(*Vdp1LocalCoordinate)(vdp1cmd_struct* cmd, u8* ram, Vdp1* regs);
	void(*Vdp1ReadFrameBuffer)(u32 type, u32 addr, void* out);
	void(*Vdp1WriteFrameBuffer)(u32 type, u32 addr, u32 val);
	void(*Vdp1EraseWrite)(int id);
	void(*Vdp1FrameChange)(void);
	void(*Vdp1RegenerateCmd)(vdp1cmd_struct* cmd);
	// VDP2 specific
	int (*Vdp2Reset)(void);
	void (*Vdp2Draw)(void);
	void (*GetGlSize)(int* width, int* height);
	void (*SetSettingValue)(int type, int value);
	void(*Sync)(void);
	void (*GetNativeResolution)(int* width, int* height, int* interlace);
	void(*Vdp2DispOff)(void);
	void (*composeFB)(Vdp2* regs);
	void (*composeVDP1)(void);
	int (*setupFrame)(int);
	void (*FinsihDraw)(void);
} VideoInterface_struct;

extern VideoInterface_struct* VIDCore;
extern VideoInterface_struct* VIDCoreList[];

#endif
