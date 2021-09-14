#pragma once

/*	Copyright 2012-2013 Theo Berkau <cwx@cyberwarriorx.com>

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
#ifndef UIDEBUGSH2_H
#define UIDEBUGSH2_H

#include "UIDebugCPU.h"
#include "QtYabause.h"

class UIDebugSH2 : public UIDebugCPU
{
    Q_OBJECT
public:
    UIDebugSH2(bool master, YabauseThread * mYabauseThread, QWidget * parent = nullptr);
    void updateRegisters() override;
    void updateCodeList(u32 addr) override;
    void updateBackTrace();
    void updateProgramCounter(u32 & pc_out, bool & changed_out) override;
    void updateTrackInfLoop();
    void updateAll() override;
    u32 getRegister(int index, int * size) override;
    void setRegister(int index, u32 value) override;
    bool addCodeBreakpoint(u32 addr) override;
    bool delCodeBreakpoint(u32 addr) override;
    bool addMemoryBreakpoint(u32 addr, u32 flags) override;
    bool delMemoryBreakpoint(u32 addr) override;
    void stepInto() override;
    void stepOver() override;
    void stepOut() override;
    void reserved1() override;
    void reserved2() override;
    void reserved3() override;

private:
    u32 SH2Dis(u32 addr, char* string);

    SH2_struct * debugSH2 = nullptr;
    sh2regs_struct sh2regs;
    bool m_isMaster;
};

#endif // UIDEBUGSH2_H
