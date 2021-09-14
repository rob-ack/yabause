#pragma once

/*	Copyright 2012 Theo Berkau <cwx@cyberwarriorx.com>

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
#ifndef UIDEBUGSCSPDSP_H
#define UIDEBUGSCSPDSP_H

#include "UIDebugCPU.h"
#include "QtYabause.h"

class QTimer;

class UIDebugSCSPDSP : public UIDebugCPU
{
    Q_OBJECT

public:
    UIDebugSCSPDSP(YabauseThread * mYabauseThread, QWidget * parent = 0);
    ~UIDebugSCSPDSP() override;
    void updateRegisters() override;
    void updateCodeList(u32 addr) override;
    u32 getRegister(int index, int * size) override;
    void setRegister(int index, u32 value) override;
    bool addCodeBreakpoint(u32 addr) override;
    bool delCodeBreakpoint(u32 addr) override;
    void stepInto() override;
};

#endif // UIDEBUGSCSPDSP_H
