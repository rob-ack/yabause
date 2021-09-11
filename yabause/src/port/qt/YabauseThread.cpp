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
#include "YabauseThread.h"
#include "Settings.h"
#include "VolatileSettings.h"
#include "ui/UIPortManager.h"
#include "ui/UIYabause.h"
#include "QtYabause.h"
#include "peripheral.h"

#include <QDateTime>
#include <QStringList>
#include <QDebug>
#include <QString>

YabauseThread::YabauseThread( QObject* o )
	: QObject( o )
{
	mPause = true;
	mTimerId = -1;
	mInit = -1;
	memset(&mYabauseConf, 0, sizeof(mYabauseConf));
	showFPS = false;
}

YabauseThread::~YabauseThread()
{
	deInitEmulation();
}

yabauseinit_struct* YabauseThread::yabauseConf()
{
	return &mYabauseConf;
}

void YabauseThread::initEmulation()
{
	reloadSettings();
	mInit = YabauseInit( &mYabauseConf );
	SetOSDToggle(showFPS);
}

void YabauseThread::deInitEmulation()
{
	YabauseDeInit();
	mInit = -1;
}

bool YabauseThread::pauseEmulation( bool pause, bool reset )
{
	if ( mPause == pause && !reset ) {
		return true;
	}

	if ( mInit == 0 && reset ) {
		deInitEmulation();
	}

	if ( mInit < 0 ) {
		initEmulation();
	}

	if ( mInit < 0 )
	{
		auto const & lastErrorString = QtYabause::getErrorStack().top();
		emit error( QtYabause::translate(lastErrorString.ErrorMessage), false );
		return false;
	}

	mPause = pause;

	if ( mPause ) {
		ScspMuteAudio(SCSP_MUTE_SYSTEM);
		killTimer( mTimerId );
		mTimerId = -1;
	}
	else {
                resetSyncVideo();
		ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
		mTimerId = startTimer( 0 );
	}

	VolatileSettings * vs = QtYabause::volatileSettings();

	if (vs->value("autostart").toBool())
	{
		if (vs->value("autostart/load").toBool()) {
			YabLoadStateSlot( QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString().toLatin1().constData(), vs->value("autostart/load/slot").toInt() );
		}
		vs->setValue("autostart", false);
	}

	emit this->pause( mPause );

	return true;
}

bool YabauseThread::resetEmulation()
{
	if ( mInit < 0 ) {
		return false;
	}

	YabauseReset();

	emit reset();

	return true;
}

void YabauseThread::reloadControllers()
{
	PerPortReset();
	QtYabause::clearPadsBits();

	Settings* settings = QtYabause::settings();

	emit toggleEmulateMouse( false );

	for ( uint port = 1; port < 3; port++ )
	{
		settings->beginGroup( QString( "Input/Port/%1/Id" ).arg( port ) );
		QStringList ids = settings->childGroups();
		settings->endGroup();

		ids.sort();
		foreach ( const QString& id, ids )
		{
			uint type = settings->value( QString( UIPortManager::mSettingsType ).arg( port ).arg( id ) ).toUInt();
			switch ( type )
			{
                                case PERCABINET:
                                {
                                  PerCab_struct* padbits = PerCabAdd( NULL );
                                  settings->beginGroup( QString( "Input/Port/%1/Id/%2/Controller/%3/Key" ).arg( port ).arg( id ).arg( type ) );
                                  QStringList padKeys = settings->childKeys();
				  settings->endGroup();
				  padKeys.sort();
                                  foreach ( const QString& padKey, padKeys )
				  {
				    const QString key = settings->value( QString( UIPortManager::mSettingsKey ).arg( port ).arg( id ).arg( type ).arg( padKey ) ).toString();
				    PerSetKey( key.toUInt(), padKey.toUInt(), padbits );
				  }
                                  break;
                                }
				case PERPAD:
				{
					PerPad_struct* padbits = PerPadAdd( port == 1 ? &PORTDATA1 : &PORTDATA2 );

					settings->beginGroup( QString( "Input/Port/%1/Id/%2/Controller/%3/Key" ).arg( port ).arg( id ).arg( type ) );
					QStringList padKeys = settings->childKeys();
					settings->endGroup();

					padKeys.sort();
					foreach ( const QString& padKey, padKeys )
					{
						const QString key = settings->value( QString( UIPortManager::mSettingsKey ).arg( port ).arg( id ).arg( type ).arg( padKey ) ).toString();

						PerSetKey( key.toUInt(), padKey.toUInt(), padbits );
					}
					break;
				}
				case PERWHEEL:
            {
               PerAnalog_struct* analogbits = PerWheelAdd(port == 1 ? &PORTDATA1 : &PORTDATA2);

               settings->beginGroup(QString("Input/Port/%1/Id/%2/Controller/%3/Key").arg(port).arg(id).arg(type));
               QStringList analogKeys = settings->childKeys();
               settings->endGroup();

               analogKeys.sort();
               foreach(const QString& analogKey, analogKeys)
               {
                  const QString key = settings->value(QString(UIPortManager::mSettingsKey).arg(port).arg(id).arg(type).arg(analogKey)).toString();

                  PerSetKey(key.toUInt(), analogKey.toUInt(), analogbits);
               }
               break;
            }
            case PERMISSIONSTICK:
            {
               PerAnalog_struct* analogbits = PerMissionStickAdd(port == 1 ? &PORTDATA1 : &PORTDATA2);

               settings->beginGroup(QString("Input/Port/%1/Id/%2/Controller/%3/Key").arg(port).arg(id).arg(type));
               QStringList analogKeys = settings->childKeys();
               settings->endGroup();

               analogKeys.sort();
               foreach(const QString& analogKey, analogKeys)
               {
                  const QString key = settings->value(QString(UIPortManager::mSettingsKey).arg(port).arg(id).arg(type).arg(analogKey)).toString();

                  PerSetKey(key.toUInt(), analogKey.toUInt(), analogbits);
               }
               break;
            }
            case PERTWINSTICKS:
            {
               PerAnalog_struct* analogbits = PerTwinSticksAdd(port == 1 ? &PORTDATA1 : &PORTDATA2);

               settings->beginGroup(QString("Input/Port/%1/Id/%2/Controller/%3/Key").arg(port).arg(id).arg(type));
               QStringList analogKeys = settings->childKeys();
               settings->endGroup();

               analogKeys.sort();
               foreach(const QString& analogKey, analogKeys)
               {
                  const QString key = settings->value(QString(UIPortManager::mSettingsKey).arg(port).arg(id).arg(type).arg(analogKey)).toString();

                  PerSetKey(key.toUInt(), analogKey.toUInt(), analogbits);
               }
               break;
            }
				case PER3DPAD:
				{
					PerAnalog_struct* analogbits = Per3DPadAdd( port == 1 ? &PORTDATA1 : &PORTDATA2 );

					settings->beginGroup( QString( "Input/Port/%1/Id/%2/Controller/%3/Key" ).arg( port ).arg( id ).arg( type ) );
					QStringList analogKeys = settings->childKeys();
					settings->endGroup();

					analogKeys.sort();
					foreach ( const QString& analogKey, analogKeys )
					{
						const QString key = settings->value( QString( UIPortManager::mSettingsKey ).arg( port ).arg( id ).arg( type ).arg( analogKey ) ).toString();

						PerSetKey( key.toUInt(), analogKey.toUInt(), analogbits );
					}
					break;
				}
				case PERGUN:
				{
					PerGun_struct* gunbits = PerGunAdd( port == 1 ? &PORTDATA1 : &PORTDATA2 );
					settings->beginGroup( QString( "Input/Port/%1/Id/%2/Controller/%3/Key" ).arg( port ).arg( id ).arg( type ) );
					QStringList gunKeys = settings->childKeys();
					settings->endGroup();

					gunKeys.sort();
					foreach ( const QString& gunKey, gunKeys )
					{
						const QString key = settings->value( QString( UIPortManager::mSettingsKey ).arg( port ).arg( id ).arg( type ).arg( gunKey ) ).toString();

						PerSetKey( key.toUInt(), gunKey.toUInt(), gunbits );
					}
					break;
				}
				case PERKEYBOARD:
					QtYabause::appendLog( "Keyboard controller type is not yet supported" );
					break;
				case PERMOUSE:
				{
					PerMouse_struct* mousebits = PerMouseAdd( port == 1 ? &PORTDATA1 : &PORTDATA2 );

					settings->beginGroup( QString( "Input/Port/%1/Id/%2/Controller/%3/Key" ).arg( port ).arg( id ).arg( type ) );
					QStringList mouseKeys = settings->childKeys();
					settings->endGroup();

					mouseKeys.sort();
					foreach ( const QString& mouseKey, mouseKeys )
					{
						const QString key = settings->value( QString( UIPortManager::mSettingsKey ).arg( port ).arg( id ).arg( type ).arg( mouseKey ) ).toString();

						PerSetKey( key.toUInt(), mouseKey.toUInt(), mousebits );
					}

					emit toggleEmulateMouse( true );
					break;
				}
				case 0:
					// Unconnected
					break;
				default:
					QtYabause::appendLog( "Invalid controller type" );
					break;
			}
		}
	}
}

static struct tm *localtime_qt(const QDateTime &input, struct tm *result)
{
	QDate date(input.date());
	result->tm_year = date.year() - 1900;
	result->tm_mon = date.month();
	result->tm_mday = date.day();
	result->tm_wday = date.dayOfWeek();
	result->tm_yday = date.dayOfYear();

	QTime time(input.time());
	result->tm_sec = time.second();
	result->tm_min = time.minute();
	result->tm_hour = time.hour();

	return result;
}

void YabauseThread::reloadClock()
{
	QString tmp;
	Settings* s = QtYabause::settings();

	if (mYabauseConf.basetime == 0)
		tmp = "";
	else
		tmp = QDateTime::fromTime_t(mYabauseConf.basetime).toString();

	// Clock sync
	mYabauseConf.clocksync = (int)s->value( "General/ClockSync", mYabauseConf.clocksync ).toBool();
	tmp = s->value( "General/FixedBaseTime", tmp ).toString();
	if (!tmp.isEmpty() && mYabauseConf.clocksync)
	{
		QDateTime date = QDateTime::fromString(tmp, Qt::ISODate);
		mYabauseConf.basetime = (long)date.toTime_t();
	}
	else {
		mYabauseConf.basetime = 0;
	}
}

void YabauseThread::reloadSettings()
{
	//QMutexLocker l( &mMutex );
	// get settings pointer
	VolatileSettings* vs = QtYabause::volatileSettings();

	// reset yabause conf
	resetYabauseConf();

	// read & apply settings
	mYabauseConf.m68kcoretype = vs->value("Advanced/68kCore", mYabauseConf.m68kcoretype).toInt();
	mYabauseConf.percoretype = vs->value( "Input/PerCore", mYabauseConf.percoretype ).toInt();
	mYabauseConf.sh2coretype = vs->value( "Advanced/SH2Interpreter", mYabauseConf.sh2coretype ).toInt();
	mYabauseConf.vidcoretype = vs->value( "Video/VideoCore", mYabauseConf.vidcoretype ).toInt();
	mYabauseConf.osdcoretype = vs->value( "Video/OSDCore", mYabauseConf.osdcoretype ).toInt();
	mYabauseConf.sndcoretype = vs->value( "Sound/SoundCore", mYabauseConf.sndcoretype ).toInt();
	mYabauseConf.cdcoretype = vs->value( "General/CdRom", mYabauseConf.cdcoretype ).toInt();
	mYabauseConf.carttype = vs->value( "Cartridge/Type", mYabauseConf.carttype ).toInt();
	mYabauseConf.stvgame = vs->value( "Cartridge/STVGame", mYabauseConf.stvgame ).toInt();
	mYabauseConf.regionid = 0;
	const QString r = vs->value( "STV/Region", mYabauseConf.regionid ).toString();
	if ( r.isEmpty() || r == "Auto" )
		mYabauseConf.stv_favorite_region = 1;
	else
	{
		switch ( r[0].toLatin1() )
		{
			case 'E': mYabauseConf.stv_favorite_region = 1; break;
			case 'U': mYabauseConf.stv_favorite_region = 2; break;
			case 'J': mYabauseConf.stv_favorite_region = 4; break;
			case 'T': mYabauseConf.stv_favorite_region = 8; break;
			default : mYabauseConf.stv_favorite_region = 1; break;
		}
	}
	const QString r1 = vs->value( "General/SystemLanguageID", mYabauseConf.languageid ).toString();
	if ( r1.isEmpty() || r1 == "" )
		mYabauseConf.languageid = 0;
	else
	{
		switch ( r1[0].toLatin1() )
		{
			case '0': mYabauseConf.languageid = 0; break;
			case '1': mYabauseConf.languageid = 1; break;
			case '2': mYabauseConf.languageid = 2; break;
			case '3': mYabauseConf.languageid = 3; break;
			case '4': mYabauseConf.languageid = 4; break;
			case '5': mYabauseConf.languageid = 5; break;
		}
	}
	if (vs->value("General/EnableEmulatedBios", false).toBool())
		mYabauseConf.biospath = strdup( "" );
	else
		mYabauseConf.biospath = strdup( vs->value( "General/Bios", mYabauseConf.biospath ).toString().toLatin1().constData() );
	mYabauseConf.cdpath = strdup( vs->value( "General/CdRomISO", mYabauseConf.cdpath ).toString().toLatin1().constData() );
	showFPS = vs->value( "General/ShowFPS", false ).toBool();
	mYabauseConf.vsyncon = vs->value("General/EnableVSync", true).toBool();
	mYabauseConf.usethreads = (int)vs->value( "General/EnableMultiThreading", mYabauseConf.usethreads ).toBool();
	mYabauseConf.numthreads = vs->value( "General/NumThreads", mYabauseConf.numthreads ).toInt();
	mYabauseConf.buppath = strdup( vs->value( "Memory/Path", mYabauseConf.buppath ).toString().toLatin1().constData() );
	mYabauseConf.mpegpath = strdup( vs->value( "MpegROM/Path", mYabauseConf.mpegpath ).toString().toLatin1().constData() );
	if (vs->value("Memory/ExtendMemory", true).toBool()) {
		mYabauseConf.extend_backup = 1;
	} else {
		mYabauseConf.extend_backup = 0;
	}
	mYabauseConf.cartpath = strdup( vs->value( "Cartridge/Path", mYabauseConf.cartpath ).toString().toLatin1().constData() );
	mYabauseConf.eepromdir = strcat(strdup( mYabauseConf.cartpath ), "/");
	mYabauseConf.modemip = strdup( vs->value( "Cartridge/ModemIP", mYabauseConf.modemip ).toString().toLatin1().constData() );
	mYabauseConf.modemport = strdup( vs->value( "Cartridge/ModemPort", mYabauseConf.modemport ).toString().toLatin1().constData() );

	mYabauseConf.video_filter_type = vs->value("Video/filter_type", mYabauseConf.video_filter_type).toInt();
	mYabauseConf.video_upscale_type = vs->value("Video/upscale_type", mYabauseConf.video_upscale_type).toInt();
	mYabauseConf.polygon_generation_mode = vs->value("Video/polygon_generation_mode", mYabauseConf.polygon_generation_mode).toInt();
	mYabauseConf.resolution_mode = vs->value("Video/resolution_mode", mYabauseConf.resolution_mode).toInt();
	mYabauseConf.use_cs = vs->value("Video/compute_shader_mode", mYabauseConf.use_cs).toInt();
	mYabauseConf.stretch = vs->value("Video/AspectRatio", mYabauseConf.stretch).toInt();
	mYabauseConf.scanline = vs->value("Video/ScanLine", mYabauseConf.scanline).toInt();
	mYabauseConf.wireframe_mode = vs->value("Video/Wireframe", mYabauseConf.wireframe_mode).toInt();
	mYabauseConf.meshmode = vs->value("Video/MeshMode", mYabauseConf.meshmode).toInt();
	mYabauseConf.bandingmode = vs->value("Video/BandingMode", mYabauseConf.bandingmode).toInt();

	// TODO : expose this in qt ui
	mYabauseConf.auto_cart = 1;

	emit requestSize( QSize( vs->value( "Video/WinWidth", 0 ).toInt(), vs->value( "Video/WinHeight", 0 ).toInt() ) );
	emit requestFullscreen( vs->value( "Video/Fullscreen", false ).toBool() );
	emit requestVolumeChange( vs->value( "Sound/Volume", 100 ).toInt() );

	reloadClock();
	reloadControllers();
}

bool YabauseThread::emulationRunning()
{
	//QMutexLocker l( &mMutex );
	return mTimerId != -1 && !mPause;
}

bool YabauseThread::emulationPaused()
{
	//QMutexLocker l( &mMutex );
	return mPause;
}

void YabauseThread::OpenTray(){
	Cs2ForceOpenTray();
}

int YabauseThread::CloseTray(){

	VolatileSettings* vs = QtYabause::volatileSettings();
	mYabauseConf.cdcoretype = vs->value("General/CdRom", mYabauseConf.cdcoretype).toInt();
	mYabauseConf.cdpath = strdup(vs->value("General/CdRomISO", mYabauseConf.cdpath).toString().toLatin1().constData());

	return Cs2ForceCloseTray(mYabauseConf.cdcoretype, mYabauseConf.cdpath);
}

void YabauseThread::resetYabauseConf()
{
	// free structure
	memset( &mYabauseConf, 0, sizeof( yabauseinit_struct ) );
	// fill default structure
	mYabauseConf.m68kcoretype = M68KCORE_C68K;
	mYabauseConf.percoretype = QtYabause::defaultPERCore().id;
	mYabauseConf.sh2coretype = SH2CORE_DEFAULT;
	mYabauseConf.vidcoretype = QtYabause::defaultVIDCore().id;
	mYabauseConf.sndcoretype = QtYabause::defaultSNDCore().id;
	mYabauseConf.cdcoretype = QtYabause::defaultCDCore().id;
	mYabauseConf.carttype = CART_NONE;
	mYabauseConf.regionid = 0;
	mYabauseConf.languageid = 0;
	mYabauseConf.biospath = 0;
	mYabauseConf.cdpath = 0;
	mYabauseConf.buppath = 0;
	mYabauseConf.mpegpath = 0;
	mYabauseConf.cartpath = 0;
        mYabauseConf.stvgame = -1;
	mYabauseConf.skip_load = 0;
	int numThreads = QThread::idealThreadCount();
	mYabauseConf.usethreads = numThreads <= 1 ? 0 : 1;
	mYabauseConf.numthreads = numThreads < 0 ? 1 : numThreads;
	mYabauseConf.video_filter_type = 0;
	mYabauseConf.video_upscale_type = 0;
	mYabauseConf.polygon_generation_mode = 0;
	mYabauseConf.use_cs = 0;
        mYabauseConf.resolution_mode = 1;
        mYabauseConf.stretch = 0;
}

void YabauseThread::timerEvent( QTimerEvent* )
{
	//mPause = true;
	//mRunning = true;
	//while ( mRunning )
	{
		if ( !mPause ) {
			YabauseExec();
                }
		//else
			//msleep( 25 );
		//sleep( 0 );
	}
}
