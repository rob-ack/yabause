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
#ifndef UIDEBUGM68K_H
#define UIDEBUGM68K_H

#include "UIDebugCPU.h"
#include "QtYabause.h"

class UIDebugM68K : public UIDebugCPU
{
    Q_OBJECT
public:
    UIDebugM68K(YabauseThread * mYabauseThread, QWidget * parent = nullptr);
    void updateRegisters() override;
	void updateProgramCounter(u32 & pc, bool & changed) override;
    u32 getRegister(int index, int * size) override;
    void setRegister(int index, u32 value) override;
    bool addCodeBreakpoint(u32 addr) override;
    bool delCodeBreakpoint(u32 addr) override;
    void stepInto() override;

private :
	m68kregs_struct regs = {};

};

#endif // UIDEBUGM68K_H
