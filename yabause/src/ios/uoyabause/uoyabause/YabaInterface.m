//
//  YabaInterface.m
//  YabaSnashiro
//
//  Created by Shinya Miyamoto on 2024/07/20.
//  Copyright © 2024 devMiyax. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <MetalANGLE/GLES3/gl3.h>
#import <MetalANGLE/MGLKViewController.h>

#define CART_NONE            0
#define CART_PAR             1
#define CART_BACKUPRAM4MBIT  2
#define CART_BACKUPRAM8MBIT  3
#define CART_BACKUPRAM16MBIT 4
#define CART_BACKUPRAM32MBIT 5
#define CART_DRAM8MBIT       6
#define CART_DRAM32MBIT      7
#define CART_NETLINK         8
#define CART_ROM16MBIT       9


MGLContext *g_context = nil;
MGLContext *g_share_context = nil;

// Settings
BOOL _bios =YES;
int _cart = 0;
BOOL _fps = NO;
BOOL _frame_skip = NO;
BOOL _aspect_rate = NO;
int _filter = 0;
int _sound_engine = 0;
int _rendering_resolution = 0;
BOOL _rotate_screen = NO;
float _controller_scale = 1.0;
char * currentGamePath = NULL;

const int MSG_SAVE_STATE = 1;
const int MSG_LOAD_STATE = 2;
const int MSG_RESET = 3;
const int MSG_OPEN_TRAY = 4;
const int MSG_CLOSE_TRAY = 5;


GLuint _renderBuffer = 0;
NSObject* _objectForLock;
MGLLayer *glLayer = nil;


int swapAglBuffer ()
{
    if( glLayer == nil ) return 0;
    
    @synchronized (_objectForLock){
        MGLContext* context = [MGLContext currentContext];
        if (![context present:glLayer]) {
        }
    }
    return 0;
}

void RevokeOGLOnThisThread(){
    [MGLContext setCurrentContext:g_share_context forLayer:glLayer];
}

void UseOGLOnThisThread(){
    [MGLContext setCurrentContext:g_context forLayer:glLayer];
}

const char * GetBiosPath(){
#if 1
    return NULL;
#else
    if( _bios == YES ){
        return NULL;
    }
    
    NSFileManager *filemgr;
    filemgr = [NSFileManager defaultManager];
    NSString * fileName = @"bios.bin";
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    
    NSString *filePath = [documentsDirectory stringByAppendingPathComponent: fileName];
    NSLog(@"full path name: %@", filePath);
    
    // check if file exists
    if ([filemgr fileExistsAtPath: filePath] == YES){
        NSLog(@"File exists");
        
    }else {
        NSLog (@"File not found, file will be created");
        return NULL;
    }
    
    return [filePath fileSystemRepresentation];
#endif
}

const char * GetGamePath(){
    
    //if( sharedData_ == nil ){
    //    return nil;
   // }
    //NSString *path = sharedData_.selected_file;
    //return [path cStringUsingEncoding:1];
    return currentGamePath;;
}

const char * GetStateSavePath(){
    BOOL isDir;
    NSFileManager *filemgr;
    filemgr = [NSFileManager defaultManager];
    NSString * fileName = @"state/";
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    
    NSString *filePath = [documentsDirectory stringByAppendingPathComponent: fileName];
    NSLog(@"full path name: %@", filePath);
    
    
    NSString *docDir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    NSString *dirName = [docDir stringByAppendingPathComponent:@"state"];
    
    
    NSFileManager *fm = [NSFileManager defaultManager];
    if(![fm fileExistsAtPath:dirName isDirectory:&isDir])
    {
        if([fm createDirectoryAtPath:dirName withIntermediateDirectories:YES attributes:nil error:nil])
            NSLog(@"Directory Created");
        else
            NSLog(@"Directory Creation Failed");
    }
    else
        NSLog(@"Directory Already Exist");

    return [filePath fileSystemRepresentation];
}

const char * GetMemoryPath(){
    BOOL isDir;
    NSFileManager *filemgr;
    filemgr = [NSFileManager defaultManager];
    NSString * fileName = @"backup/memory.bin";
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    
    NSString *filePath = [documentsDirectory stringByAppendingPathComponent: fileName];
    NSLog(@"full path name: %@", filePath);
    
    
    NSString *docDir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    NSString *dirName = [docDir stringByAppendingPathComponent:@"backup"];
    
    
    NSFileManager *fm = [NSFileManager defaultManager];
    if(![fm fileExistsAtPath:dirName isDirectory:&isDir])
    {
        if([fm createDirectoryAtPath:dirName withIntermediateDirectories:YES attributes:nil error:nil])
            NSLog(@"Directory Created");
        else
            NSLog(@"Directory Creation Failed");
    }
    else
        NSLog(@"Directory Already Exist");
    
    // check if file exists
    if ([filemgr fileExistsAtPath: filePath] == YES){
        NSLog(@"File exists");
        
    }else {
        NSLog (@"File not found, file will be created");
    }
    
    return [filePath fileSystemRepresentation];
}

int GetCartridgeType(){
    return _cart;
}

int GetVideoInterface(){
    return 0;
}

int GetEnableFPS(){
    if( _fps == YES )
        return 1;
    
    return 0;
}

int GetIsRotateScreen() {
    if( _rotate_screen == YES )
        return 1;
    
    return 0;
}

int GetEnableFrameSkip(){
    if( _frame_skip == YES ){
        return 1;
    }
    return 0;
}

int GetUseNewScsp(){
    return _sound_engine;
}

int GetVideFilterType(){
    return _filter;
}

int GetResolutionType(){
    NSLog (@"GetResolutionType %d",_rendering_resolution);
    return _rendering_resolution;
}

const char * GetCartridgePath(){
    BOOL isDir;
    NSFileManager *filemgr;
    filemgr = [NSFileManager defaultManager];
    NSString * fileName = @"cart/invalid.ram";
    
    switch(_cart) {
        case CART_NONE:
            fileName = @"cart/none.ram";
        case CART_PAR:
            fileName = @"cart/par.ram";
        case CART_BACKUPRAM4MBIT:
            fileName = @"cart/backup4.ram";
        case CART_BACKUPRAM8MBIT:
            fileName = @"cart/backup8.ram";
        case CART_BACKUPRAM16MBIT:
            fileName = @"cart/backup16.ram";
        case CART_BACKUPRAM32MBIT:
            fileName = @"cart/backup32.ram";
        case CART_DRAM8MBIT:
            fileName = @"cart/dram8.ram";
        case CART_DRAM32MBIT:
            fileName = @"cart/dram32.ram";
        case CART_NETLINK:
            fileName = @"cart/netlink.ram";
        case CART_ROM16MBIT:
            fileName = @"cart/om16.ram";
        default:
            fileName = @"cart/invalid.ram";
    }
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    
    NSString *filePath = [documentsDirectory stringByAppendingPathComponent: fileName];
    NSLog(@"full path name: %@", filePath);
    
    
    NSString *docDir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    NSString *dirName = [docDir stringByAppendingPathComponent:@"cart"];
    
    
    NSFileManager *fm = [NSFileManager defaultManager];
    if(![fm fileExistsAtPath:dirName isDirectory:&isDir])
    {
        if([fm createDirectoryAtPath:dirName withIntermediateDirectories:YES attributes:nil error:nil])
            NSLog(@"Directory Created");
        else
            NSLog(@"Directory Creation Failed");
    }
    else
        NSLog(@"Directory Already Exist");
    
    // check if file exists
    if ([filemgr fileExistsAtPath: filePath] == YES){
        NSLog(@"File exists");
        
    }else {
        NSLog (@"File not found, file will be created");
    }
    return [filePath fileSystemRepresentation];
}

int GetPlayer2Device(){
    return -1;
}

