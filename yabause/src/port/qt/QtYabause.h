/*  Copyright 2005 Guillaume Duhamel
	Copyright 2005-2006, 2013 Theo Berkau
	Copyright 2008 Filipe Azevedo <pasnox@gmail.com>

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
#ifndef QTYABAUSE_H
#define QTYABAUSE_H

extern "C"
{
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif
	#include "yabause.h"
	#include "peripheral.h"
	#include "sh2core.h"
	#include "sh2int.h"
	#include "vidogl.h"
	#include "vidcs.h"
	#include "vidsoft.h"
	#include "cs0.h"
	#include "cdbase.h"
	#include "scsp.h"
	#include "scspdsp.h"
	#include "scu.h"
	#include "sndal.h"
	#include "sndsdl.h"
	#include "persdljoy.h"
#if defined HAVE_DIRECTINPUT
	#include "perdx.h"
#endif
#if defined HAVE_DIRECTSOUND
	#include "snddx.h"
#endif
	#include "permacjoy.h"
	#include "perlinuxjoy.h"
	#include "debug.h"
	#include "../m68kcore.h"

	#include "vdp1.h"
	#include "vdp2.h"
	#include "cs2.h"

	#include "cheat.h"
	#include "memory.h"
	#include "bios.h"

	#include "vdp2debug.h"

	#include "PerQt.h"
    #include "osdcore.h"

#ifdef ARCH_IS_MACOSX
    #include "../sndmac.h"
#endif
}

#include <QString>
#include <QMap>
#include <QStack>
#include <QDateTime>

class UIYabause;
class Settings;
class VolatileSettings;
class QWidget;

typedef struct
{
	QString file;
	QString name;
} translation_struct;

namespace QtYabause
{
	namespace SettingKeys
	{
		static constexpr char const* const ScreenshotsDirectory = "General/ScreenshotsDirectory";
		static constexpr char const* const ScreenshotsFormat = "General/ScreenshotsFormat";
	};

	namespace VolatileSettingKeys
	{
		static constexpr char const* const CartridgePath = "Cartridge/Path";
	};

	namespace DefaultPaths
	{
		QString Screenshots();
		QString Cartridge();
	};

	struct QtYabauseError
	{
		int ErrorType;
		QString ErrorMessage;
		QDateTime ErrorTimestamp = QDateTime::currentDateTime();
	};

	void appendLog( const char* str );
	UIYabause* mainWindow( bool create = true );
	Settings* settings( bool create = true );
	VolatileSettings* volatileSettings( bool create = true );
	QList <translation_struct> getTranslationList();
	int setTranslationFile();
	int logTranslation();
	void closeTranslation();
	QString translate( const QString& string );
	void retranslateWidgetOnly( QWidget* widget );
	void retranslateWidget( QWidget* widget );
	void retranslateApplication();

	// get cd serial
	const char* getCurrentCdSerial();

	// get core by id
	M68K_struct* getM68KCore( int id );
	SH2Interface_struct* getSH2Core( int id );
	PerInterface_struct* getPERCore( int id );
	CDInterface* getCDCore( int id );
	SoundInterface_struct* getSNDCore( int id );
	VideoInterface_struct* getVDICore( int id );

	// default cores
	CDInterface defaultCDCore();
	SoundInterface_struct defaultSNDCore();
	VideoInterface_struct defaultVIDCore();
	OSD_struct defaultOSDCore();
	PerInterface_struct defaultPERCore();
	SH2Interface_struct defaultSH2Core();
   M68K_struct default68kCore();

	// padsbits
	QMap<uint, PerPad_struct*>* portPadsBits( uint portNumber );
	void clearPadsBits();
        QMap<uint, PerCab_struct*>* portIOBits( );
	void clearIOBits();
	QMap<uint, PerAnalog_struct*>* portAnalogBits( uint portNumber );
	void clear3DAnalogBits();
	QMap<uint, PerGun_struct*>* portGunBits( uint portNumber );
	void clearGunBits();
	QMap<uint, PerMouse_struct*>* portMouseBits( uint portNumber );
	void clearMouseBits();

	QStack<QtYabauseError> & getErrorStack();
};

#endif // QTYABAUSE_H
