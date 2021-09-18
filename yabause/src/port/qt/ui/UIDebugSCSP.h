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
#ifndef UIDEBUGSCSP_H
#define UIDEBUGSCSP_H

#include "ui_UIDebugSCSP.h"
#include "core.h"

class QIODevice;
class QTimer;

#ifdef HAVE_QT_MULTIMEDIA
#include <QAudioOutput>
#endif

class UIDebugSCSP : public QDialog, public Ui::UIDebugSCSP
{
    Q_OBJECT
public:
    UIDebugSCSP(QWidget * parent = nullptr);
    ~UIDebugSCSP() override;

protected slots:
    void on_sbSlotNumber_valueChanged(int i);
    void on_pbSaveAsWav_clicked();
    void on_pbSaveSlotRegisters_clicked();
#ifdef HAVE_QT_MULTIMEDIA
    void on_pbPlaySlot_clicked();
    void notified();
    void audioBufferRefill();
    void stateChanged(QAudio::State state);

protected:
    void initAudio();

private:
    QAudioDeviceInfo audioDeviceInfo;
    QAudioOutput * audioOutput = nullptr;
    QAudioFormat audioFormat;
#endif
    QIODevice * outputDevice = nullptr;
    bool isPlaying;

    u32 * slot_workbuf = nullptr;
    s16 * slot_buf = nullptr;
    QTimer* timer = nullptr;
    QTimer* audioBufferTimer = nullptr;
};

#endif // UIDEBUGSCSP_H
