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
#include "UIDebugSCUDSP.h"
#include "../CommonDialogs.h"
#include "UIYabause.h"

int SCUDSPDis(u32 addr, char *string)
{
   ScuDspDisasm((u8)addr, string);
   return 1;
}

void SCUDSPBreakpointHandler (u32 addr)
{
   UIYabause* ui = QtYabause::mainWindow( false );

   emit ui->breakpointHandlerSCUDSP();
}

UIDebugSCUDSP::UIDebugSCUDSP( YabauseThread *mYabauseThread, QWidget* p )
	: UIDebugCPU( mYabauseThread, p )
{
   this->setWindowTitle(QtYabause::translate("Debug SCU DSP"));
   gbRegisters->setTitle(QtYabause::translate("DSP Registers"));
	pbMemoryTransfer->setVisible( false );
	gbMemoryBreakpoints->setVisible( false );

   pbReserved1->setText(QtYabause::translate("Save Program"));
   pbReserved2->setText(QtYabause::translate("Save MD0"));
   pbReserved3->setText(QtYabause::translate("Save MD1"));
   pbReserved4->setText(QtYabause::translate("Save MD2"));
   pbReserved5->setText(QtYabause::translate("Save MD3"));

   pbReserved1->setVisible( true );
   pbReserved2->setVisible( true );
   pbReserved3->setVisible( true );
   pbReserved4->setVisible( true );
   pbReserved5->setVisible( true );

   QSize size = lwRegisters->minimumSize();
   size.setWidth(size.width() + lwRegisters->fontMetrics().averageCharWidth());
   lwRegisters->setMinimumSize(size);

   size = lwDisassembledCode->minimumSize();
   size.setWidth(lwRegisters->fontMetrics().averageCharWidth() * 80);
   lwDisassembledCode->setMinimumSize(size);

	if (ScuRegs)
	{
		const scucodebreakpoint_struct *cbp;
		int i;

		cbp = ScuDspGetBreakpointList();

		for (i = 0; i < MAX_BREAKPOINTS; i++)
		{
			QString text;
			if (cbp[i].addr != 0xFFFFFFFF)
			{
        text = QString("%1").arg(static_cast<uint32_t>(cbp[i].addr), 8, 16, QChar('0')).toUpper();
				lwCodeBreakpoints->addItem(text);
			}
		}

		lwDisassembledCode->setDisassembleFunction(SCUDSPDis);
		lwDisassembledCode->setEndAddress(0x100);
		lwDisassembledCode->setMinimumInstructionSize(1);
		ScuDspSetBreakpointCallBack(SCUDSPBreakpointHandler);
	}

	updateAll();
}

void UIDebugSCUDSP::updateRegList()
{
  scudspregs_struct regs;
  QString str;

  if (ScuRegs == NULL)
    return;

  memset(&regs, 0, sizeof(regs));
  ScuDspGetRegisters(&regs);
  lwRegisters->clear();

  str = QString("PR = %1   EP = %2").arg(regs.ProgControlPort.part.PR).arg(regs.ProgControlPort.part.EP);
  lwRegisters->addItem(str);

  str = QString("T0 = %1   S =  %2").arg(regs.ProgControlPort.part.T0).arg(regs.ProgControlPort.part.S);
  lwRegisters->addItem(str);

  str = QString("Z =  %1   C =  %2").arg(regs.ProgControlPort.part.Z).arg(regs.ProgControlPort.part.C);
  lwRegisters->addItem(str);

  str = QString("V =  %1   E =  %2").arg(regs.ProgControlPort.part.V).arg(regs.ProgControlPort.part.E);
  lwRegisters->addItem(str);

  str = QString("ES = %1   EX = %2").arg(regs.ProgControlPort.part.ES).arg(regs.ProgControlPort.part.EX);
  lwRegisters->addItem(str);

  str = QString("LE =          %1").arg(regs.ProgControlPort.part.LE);
  lwRegisters->addItem(str);

  str = QString("P =          %1").arg(regs.ProgControlPort.part.P, 2, 16, QChar('0')).toUpper();
  lwRegisters->addItem(str);

  str = QString("TOP =        %1").arg(regs.TOP, 2, 16, QChar('0')).toUpper();
  lwRegisters->addItem(str);

  str = QString("LOP =        %1").arg(regs.LOP, 2, 16, QChar('0')).toUpper();
  lwRegisters->addItem(str);

  str = QString("CT = %1:%2:%3:%4")
    .arg(regs.CT[0], 2, 16, QChar('0'))
    .arg(regs.CT[1], 2, 16, QChar('0'))
    .arg(regs.CT[2], 2, 16, QChar('0'))
    .arg(regs.CT[3], 2, 16, QChar('0'))
    .toUpper();
  lwRegisters->addItem(str);

  str = QString("RA =   %1").arg(regs.RA0, 8, 16, QChar('0')).toUpper();
  lwRegisters->addItem(str);

  str = QString("WA =   %1").arg(regs.WA0, 8, 16, QChar('0')).toUpper();
  lwRegisters->addItem(str);

  str = QString("RX =   %1").arg(regs.RX, 8, 16, QChar('0')).toUpper();
  lwRegisters->addItem(str);

  str = QString("RY =   %1").arg(regs.RY, 8, 16, QChar('0')).toUpper();
  lwRegisters->addItem(str);

  str = QString("PH =       %1").arg(regs.P.part.H & 0xFFFF, 4, 16, QChar('0')).toUpper();
  lwRegisters->addItem(str);

  str = QString("PL =   %1").arg(static_cast<uint32_t>(regs.P.part.L), 8, 16, QChar('0')).toUpper();
  lwRegisters->addItem(str);

  str = QString("ACH =      %1").arg(regs.AC.part.H & 0xFFFF, 4, 16, QChar('0')).toUpper();
  lwRegisters->addItem(str);

  str = QString("ACL =  %1").arg(static_cast<uint32_t>(regs.AC.part.L), 8, 16, QChar('0')).toUpper();
  lwRegisters->addItem(str);
}
void UIDebugSCUDSP::updateCodeList(u32 addr)
{
   lwDisassembledCode->goToAddress(addr);
   lwDisassembledCode->setPC(addr);
}

void UIDebugSCUDSP::updateAll()
{
	updateRegList();
	if (ScuRegs)
	{
		scudspregs_struct regs;
		ScuDspGetRegisters(&regs);
		updateCodeList(regs.PC);
	}
}

u32 UIDebugSCUDSP::getRegister(int index, int *size)
{
   *size = 0;
   return 0;
}

void UIDebugSCUDSP::setRegister(int index, u32 value)
{
}

bool UIDebugSCUDSP::addCodeBreakpoint(u32 addr)
{
	if (!ScuRegs)
		return false;
   return ScuDspAddCodeBreakpoint(addr) == 0;     
}

bool UIDebugSCUDSP::delCodeBreakpoint(u32 addr)
{
	if (!ScuRegs)
		return false;
   return ScuDspDelCodeBreakpoint(addr) == 0;
}

void UIDebugSCUDSP::stepInto()
{
   ScuDspStep();
   updateAll();
}

void UIDebugSCUDSP::reserved1()
{
   const QString s = CommonDialogs::getSaveFileName( QString(), QtYabause::translate( "Choose a location for binary file" ), QtYabause::translate( "Binary Files (*.bin)" ) );
	if (!ScuRegs)
		return;
   if ( !s.isNull() )
      ScuDspSaveProgram(s.toLatin1());
}

void UIDebugSCUDSP::reserved2()
{
   const QString s = CommonDialogs::getSaveFileName( QString(), QtYabause::translate( "Choose a location for binary file" ), QtYabause::translate( "Binary Files (*.bin)" ) );
	if (!ScuRegs)
		return;
   if ( !s.isNull() )
      ScuDspSaveMD(s.toLatin1(), 0);
}

void UIDebugSCUDSP::reserved3()
{
   const QString s = CommonDialogs::getSaveFileName( QString(), QtYabause::translate( "Choose a location for binary file" ), QtYabause::translate( "Binary Files (*.bin)" ) );
	if (!ScuRegs)
		return;
   if ( !s.isNull() )
      ScuDspSaveMD(s.toLatin1(), 1);
}

void UIDebugSCUDSP::reserved4()
{
   const QString s = CommonDialogs::getSaveFileName( QString(), QtYabause::translate( "Choose a location for binary file" ), QtYabause::translate( "Binary Files (*.bin)" ) );
	if (!ScuRegs)
		return;
   if ( !s.isNull() )
      ScuDspSaveMD(s.toLatin1(), 2);
}

void UIDebugSCUDSP::reserved5()
{
   const QString s = CommonDialogs::getSaveFileName( QString(), QtYabause::translate( "Choose a location for binary file" ), QtYabause::translate( "Binary Files (*.bin)" ) );
	if (!ScuRegs)
		return;
   if ( !s.isNull() )
      ScuDspSaveMD(s.toLatin1(), 3);
}

