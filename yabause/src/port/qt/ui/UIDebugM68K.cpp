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

#include "UIDebugM68K.h"
#include "CommonDialogs.h"
#include "scsp.debug.h"
#include "UIYabause.h"

extern "C"
{
#include "m68k.h"
}

u32 M68KDis(u32 addr, char * string)
{
    if (SoundRam)
    {
        if (m68k_is_valid_instruction(addr, M68K_CPU_TYPE_68000))
        {
            return m68k_disassemble(string, addr, M68K_CPU_TYPE_68000);
        }
        return 2;
    }
    return 0;
}

void M68KBreakpointHandler(u32 addr)
{
    UIYabause * ui = QtYabause::mainWindow(false);
    emit ui->breakpointHandlerM68K();
}

UIDebugM68K::UIDebugM68K(YabauseThread * mYabauseThread, QWidget * p) : UIDebugCPU(mYabauseThread, p)
{
    this->setWindowTitle(QtYabause::translate("Debug M68K"));
    gbRegisters->setTitle(QtYabause::translate("M68K Registers"));
    gbMemoryBreakpoints->setVisible(false);

    if (SoundRam) //TODO: fix me breakpoint init list needs to be done when the subsystem is running at the moment. meaning this will not work when this window is opened before the emulation was started.
    {
        const m68kcodebreakpoint_struct* cbp = M68KGetBreakpointList();
        for (int i = 0; i < MAX_BREAKPOINTS; i++)
        {
            if (cbp[i].addr != 0xFFFFFFFF)
            {
                QString text;
                text.sprintf("%08X", (int)cbp[i].addr);
                lwCodeBreakpoints->addItem(text);
            }
        }
        M68KSetBreakpointCallBack(M68KBreakpointHandler);
    }

    lwDisassembledCode->setDisassembleFunction(M68KDis);
    lwDisassembledCode->setEndAddress(0x80000); //REVIEW: this was 0x100000 but sound ram is 0x80000 big (half the size).
    lwDisassembledCode->setMinimumInstructionSize(2);
    updateRegisters();
    updateCodeList(regs.PC);
}

void UIDebugM68K::updateRegisters()
{
    if (SoundRam == nullptr)
        return;

    M68KGetRegisters(&regs);
    lwRegisters->clear();

    QString str;
    // Data registers
    for (int i = 0; i < 8; i++)
    {
        str.sprintf("D%d =   %08X", i, (int)regs.D[i]);
        lwRegisters->addItem(str);
    }

    // Address registers
    for (int i = 0; i < 8; i++)
    {
        str.sprintf("A%d =   %08X", i, (int)regs.A[i]);
        lwRegisters->addItem(str);
    }

    // SR
    str.sprintf("SR =   %08X", (int)regs.SR);
    lwRegisters->addItem(str);

    // PC
    str.sprintf("PC =   %08X", (int)regs.PC);
    lwRegisters->addItem(str);
}

void UIDebugM68K::updateProgramCounter(u32 & pc, bool & changed)
{
    pc = regs.PC;
    changed = true;
}

u32 UIDebugM68K::getRegister(int index, int * size)
{
    M68KGetRegisters(&regs);

    u32 value;
    switch (index)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            value = regs.D[index];
            break;
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            value = regs.A[index - 8];
            break;
        case 16:
            value = regs.SR;
            break;
        case 17:
            value = regs.PC;
            break;
        default:
            value = 0;
            break;
    }

    *size = 4;
    return value;
}

void UIDebugM68K::setRegister(int index, u32 value)
{
    M68KGetRegisters(&regs);

    switch (index)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            regs.D[index] = value;
            break;
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            regs.A[index - 8] = value;
            break;
        case 16:
            regs.SR = value;
            break;
        case 17:
            regs.PC = value;
            updateCodeList(regs.PC);
            break;
        default:
            break;
    }

    M68KSetRegisters(&regs);
}

bool UIDebugM68K::addCodeBreakpoint(u32 addr)
{
    if (!SoundRam)
        return false;
    return M68KAddCodeBreakpoint(addr) == 0;
}

bool UIDebugM68K::delCodeBreakpoint(u32 addr)
{
    return M68KDelCodeBreakpoint(addr) == 0;
}

void UIDebugM68K::stepInto()
{
    if (M68K)
        M68KStep();

    M68KGetRegisters(&regs);
    updateCodeList(regs.PC);
    updateRegisters();
}
