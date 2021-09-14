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
#include "UIDisasm.h"

#include <QPainter>
#include <QPaintEvent>
#include <QScrollBar>

u32 DisasmInstructionNull(u32 address, char * string)
{
	strcpy(string, " ");
	return 0;
}

UIDisasm::UIDisasm(QWidget * p) : QAbstractScrollArea(p)
{
	disassembleFunction = DisasmInstructionNull;
	address = 0;
	pc = 0xFFFFFFFF;
	instructionSize = 1;

	setSelectionColor(QColor(0x6d, 0x9e, 0xff, 0xff));
	adjustSettings();
}

void UIDisasm::setSelectionColor(const QColor & color)
{
	selectionColor = color;
	viewport()->update();
}

void UIDisasm::setDisassembleFunction(std::function<u32(u32, char *)> func)
{
	disassembleFunction = func;
}

void UIDisasm::setEndAddress(u32 address)
{
	endAddress = address;
	adjustSettings();
}

void UIDisasm::goToAddress(u32 address, bool vCenter)
{
	this->address = address;
	this->vCenter = vCenter;
	
	verticalScrollBar()->setValue(address);
	viewport()->update();
}

void UIDisasm::setPC(u32 address)
{
	this->pc = address;
}

void UIDisasm::setMinimumInstructionSize(u32 instructionSize)
{
	this->instructionSize = instructionSize;
	adjustSettings();
}

void UIDisasm::adjustSettings()
{
	verticalScrollBar()->setRange(0, endAddress);
	verticalScrollBar()->setSingleStep(instructionSize);

	fontWidth = fontMetrics().width(QLatin1Char('9'));
	fontHeight = fontMetrics().height();

	viewport()->update();
}

void UIDisasm::mouseDoubleClickEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton)
	{
		// Calculate address
		QPoint const pos = event->pos();
		QRect const viewportPos = viewport()->rect();
		int const top = viewportPos.top();
		int const bottom = viewportPos.bottom();
		int const posy = pos.y();
		int const line = posy / fontHeight;
		int const scroll_pos = verticalScrollBar()->value();
		int const currentAddress = scroll_pos / instructionSize * instructionSize;

		u32 offset = 0;
		for (int i = 0; i != line; i++)
		{
			char text[256];
			offset += disassembleFunction(currentAddress, text);
		}

		emit toggleCodeBreakpoint(currentAddress + offset);
	}
}

void UIDisasm::paintEvent(QPaintEvent * event)
{
	QPainter painter(viewport());

	// calc position
	int const top = event->rect().top();
	int const bottom = event->rect().bottom();

	verticalScrollBar()->setPageStep(bottom / fontHeight * instructionSize);

	int const pos = verticalScrollBar()->value();
	int const yPosStart = top + fontHeight;

	QBrush const selected = QBrush(selectionColor);
	QPen const colSelected = QPen(Qt::white);
	QPen const colSelected2 = QPen(selectionColor);

	u32 displayAtAdress = pos / instructionSize * instructionSize;
	for (int yPos = yPosStart; yPos < bottom;)
	{
		int constexpr xPos = 2;
		char text[256] = {0};
		auto const offset = disassembleFunction(displayAtAdress, text);

		assert(offset == 0 || offset >= instructionSize);

		QString disText;
		disText.sprintf("0x%05X: %s", displayAtAdress, text);

		if (displayAtAdress == pc && pc != 0xFFFFFFFF)
		{
			int const ascent = fontMetrics().ascent();

			painter.setBackground(selected);
			painter.setBackgroundMode(Qt::OpaqueMode);
			painter.setPen(colSelected2);
			painter.drawRect(0, yPos - ascent, event->rect().width(), fontHeight);
			painter.fillRect(0, yPos - ascent, event->rect().width(), fontHeight, selected);

			painter.setBackgroundMode(Qt::TransparentMode);
			painter.setPen(colSelected);
			painter.setPen(this->palette().color(QPalette::WindowText));
			painter.drawText(xPos, yPos - ascent, event->rect().width(), ascent, Qt::AlignJustify, disText);
		} else
		{
			painter.setPen(this->palette().color(QPalette::WindowText));
			painter.setBackgroundMode(Qt::TransparentMode);
			painter.drawText(xPos, yPos, disText);
		}

		displayAtAdress += offset;
		yPos += fontHeight;
	}
}
