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
#ifndef UIHEXINPUT_H
#define UIHEXINPUT_H

#include "ui_UIHexInput.h"
#include "../QtYabause.h"
#include <QValidator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

class HexValidator : public QValidator
{
   Q_OBJECT
private:
   unsigned int t, b;
public:
   //explicit HexValidator(QObject *parent = 0);
   HexValidator(unsigned int top, unsigned int bottom, QObject *parent = 0)
   {
      //QValidator(parent);
      t = top;
      b = bottom;
   }
   virtual void fixup(QString &input) const {
      input = input.toUpper();
   }
   virtual State validate(QString& input, int& pos) const override
   {
     QRegularExpression rxHex("[0-9A-Fa-f]{1,8}");

     fixup(input);

     if (input.isEmpty())
       return Acceptable;

     QRegularExpressionMatch match = rxHex.match(input);
     if (!match.hasMatch())
       return Invalid;

     // ”ÍˆÍ“à‚Å‚ ‚é‚±‚Æ‚ðŠm”F
     bool ok;
     unsigned int val = input.toUInt(&ok, 16);

     if (ok && (val >= t) && (val <= b))
       return Acceptable;

     return Invalid;
   }

   void setBottom(unsigned int bottom)
   {
      b = bottom;
   }
   void setTop(unsigned int top)
   {
      t = top;
   }
   virtual void setRange(unsigned int top, unsigned int bottom)
   {
      setTop(top);
      setBottom(bottom);
   }

   unsigned int bottom() const { return b; }
   unsigned int top() const { return t; }
};

class UIHexInput : public QDialog, public Ui::UIHexInput
{
	Q_OBJECT
public:
	UIHexInput( u32 value, int size, QWidget* parent = 0 );
   u32 getValue();

protected:
   u32 value;
   int size;

protected slots:
	 void accept();
};

#endif // UIHEXINPUT_H
