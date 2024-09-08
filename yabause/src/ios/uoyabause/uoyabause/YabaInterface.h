//
//  ChdWrapper.h
//  YabaSnashiro
//
//  Created by Shinya Miyamoto on 2024/07/14.
//  Copyright © 2024 devMiyax. All rights reserved.
//
#import <Foundation/Foundation.h>
#import <MetalANGLE/MGLKViewController.h>

extern BOOL _bios;
extern int _cart;
extern BOOL _fps;
extern BOOL _frame_skip;
extern BOOL _aspect_rate;
extern int _filter;
extern int _sound_engine;
extern int _rendering_resolution;
extern BOOL _rotate_screen;
extern float _controller_scale;
extern const char * currentGamePath;

extern MGLContext *g_context;
extern MGLContext *g_share_context;
extern MGLLayer *glLayer;
extern NSObject* _objectForLock;


extern const int MSG_SAVE_STATE;
extern const int MSG_LOAD_STATE;
extern const int MSG_RESET;
extern const int MSG_OPEN_TRAY;
extern const int MSG_CLOSE_TRAY;

//    MSG_RESUME,
//    MSG_OPEN_TRAY,
//    MSG_CLOSE_TRAY,
//    MSG_RESET,

const unsigned int PERANALOG_AXIS_X = 18; // left to right
const unsigned int PERANALOG_AXIS_Y = 19; // up to down
const unsigned int PERANALOG_AXIS_RTRIGGER = 20; // right trigger
const unsigned int PERANALOG_AXIS_LTRIGGER = 21; // left trigger

void PerKeyDown(unsigned int key);
void PerKeyUp(unsigned int key);
int SetAnalogMode(int mode);
void PerAxisValue(unsigned int, unsigned char val);
int start_emulation( int originx, int originy, int width, int height );
void resize_screen( int x, int y, int width, int height );
int emulation_step( int command );
int enterBackGround();
int MuteSound();
int UnMuteSound();


char * getGameinfoFromChd( const char * path );


NSString* YSGetBackupDevicelist();
NSString* YSGetBackupFilelist( int deviceid );
int YSDeleteBackupFile( int index );
NSString* YSGetBackupFile( int index );
int YSPutFile( NSString* jsonstr  );
int YSCopy( int target, int file  );


