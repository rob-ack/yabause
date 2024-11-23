//
//  GameViewController.m
//  uoyabause
//
//  Created by MiyamotoShinya on 2016/02/06.
//  Copyright © 2016年 devMiyax. All rights reserved.
//

//#import <QuartzCore/QuartzCore.h>
#import "GameViewController.h"
#include <MetalANGLE/GLES3/gl3.h>
@import GameController;

#import "YabaSnashiro-Swift.h"

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioToolbox/ExtendedAudioFile.h>
#import "GameRevealViewController.h"
#import "SidebarViewController.h"
#import "iOSCoreAudio.h"
#import "YabaInterface.h"


@interface GameViewControllerO () {
   int command;
    int controller_edit_mode;
    BOOL canRotateToAllOrientations;
    BOOL _landscape;
}

@property (strong, nonatomic) MGLContext *context;
@property (strong, nonatomic) MGLContext *share_context;
@property (nonatomic, strong) GCController *controller;
@property (nonatomic, assign) int command;
@property (nonatomic, assign) Boolean _return;
@property (nonatomic,strong) KeyMapper *keyMapper;
@property (nonatomic) BOOL isKeyRemappingMode;
@property (nonatomic,strong) UIAlertController *remapAlertController;
@property (nonatomic) SaturnKey currentlyMappingKey;
@property (nonatomic,strong) NSMutableArray *remapLabelViews;
@property (nonatomic) BOOL _isFirst;
@property (nonatomic, strong) NSMutableArray<PadButton *> *pad_buttons_;

- (void)setup;
- (void)tearDownGL;

@end

static GameViewControllerO *sharedData_ = nil;
static NSDictionary *saturnKeyDescriptions;
static NSDictionary *saturnKeyToViewMappings;


@implementation GameViewControllerO
@synthesize iPodIsPlaying;
@synthesize selected_file;


- (void)saveState{
    self.command = 1;
}

- (void)loadState{
    self.command = 2;
}

/*-------------------------------------------------------------------------------------
 Settings
---------------------------------------------------------------------------------------*/

- (NSString *)GetSettingFilePath {
    
    NSFileManager *filemgr;
    filemgr = [NSFileManager defaultManager];
    NSString * fileName = @"settings.plist";
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
    NSString *libraryDirectory = [paths objectAtIndex:0];
    
    NSString *filePath = [libraryDirectory stringByAppendingPathComponent: fileName];
    NSLog(@"full path name: %@", filePath);
    
    // check if file exists
    if ([filemgr fileExistsAtPath: filePath] == YES){
        NSLog(@"File exists");
        
    }else {
        NSBundle *bundle = [NSBundle mainBundle];
        NSString *path = [bundle pathForResource:@"settings" ofType:@"plist"];
        [filemgr copyItemAtPath:path toPath:filePath error:nil];
    }
    
    return filePath;
}

- (void)loadSettings {

    NSBundle *bundle = [NSBundle mainBundle];
    //読み込むプロパティリストのファイルパスを指定
    NSString *path = [self GetSettingFilePath];
    //プロパティリストの中身データを取得
    NSDictionary *dic = [NSDictionary dictionaryWithContentsOfFile:path];
    _bios = [[dic objectForKey: @"builtin bios"] boolValue];
    _cart = [[dic objectForKey: @"cartridge"] intValue];
    _fps = [[dic objectForKey: @"show fps"]boolValue];
    _frame_skip = [[dic objectForKey: @"frame skip"]boolValue];
    _aspect_rate = [[dic objectForKey: @"keep aspect rate"]boolValue];
    _filter = 0; //[0; //userDefaults boolForKey: @"filter"];
    _sound_engine = [[dic objectForKey: @"sound engine"] intValue];
    _rendering_resolution = [[dic objectForKey: @"rendering resolution"] intValue];
    _rotate_screen = [[dic objectForKey: @"rotate screen"]boolValue];
    
    NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
    NSMutableDictionary *defaults = [NSMutableDictionary dictionary];
    [defaults setObject:@"0.8" forKey:@"controller scale"];
    [defaults setObject:[NSNumber numberWithBool:YES] forKey:@"landscape"];
    [ud registerDefaults:defaults];
    
    _controller_scale = [ud floatForKey:@"controller scale"];
    _landscape = [ud boolForKey:@"landscape"];

}

- (id)init
{
   return [super init];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UITouch *touch in touches)
    {
        for (int btnindex = 0; btnindex < PadButtonsLast; btnindex++) {
            UIView * target = _pad_buttons_[btnindex].target;
            CGPoint point = [touch locationInView:target];
            if( CGRectContainsPoint( target.bounds , point) ) {
                [_pad_buttons_[btnindex] on:touch];
                printf("PAD:%d on\n",btnindex);
            }else{
                //printf("%d: [%d,%d,%d,%d]-[%d,%d]\n", btnindex, (int)target.frame.origin.x, (int)target.frame.origin.y, (int)target.bounds.size.width, (int)target.bounds.size.height, (int)point.x, (int)point.y);
            }
        }
    }
    
    if( [self hasControllerConnected ] ) return;
    for (int btnindex = 0; btnindex < PadButtonsLast; btnindex++) {
        if ([_pad_buttons_[btnindex] isOn] ) {
            PerKeyDown(btnindex);
        } else {
            PerKeyUp(btnindex);
        }
    }
}

- (void)pressesBegan:(NSSet<UIPress *> *)presses
           withEvent:(UIPressesEvent *)event
{
    int handleEvent = 0;
    for( UIPress * press in presses ){
        if( [press.key.charactersIgnoringModifiers isEqualToString:UIKeyInputLeftArrow] ){
            PerKeyDown(PadButtonsLeft);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:UIKeyInputRightArrow] ){
            PerKeyDown(PadButtonsRight);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:UIKeyInputUpArrow] ){
            PerKeyDown(PadButtonsUp);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:UIKeyInputDownArrow] ){
            PerKeyDown(PadButtonsDown);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"\r"] ){
            PerKeyDown(PadButtonsStart);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"z"] ){
            PerKeyDown(PadButtonsA);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"x"] ){
            PerKeyDown(PadButtonsB);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"c"] ){
            PerKeyDown(PadButtonsC);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"a"] ){
            PerKeyDown(PadButtonsX);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"s"] ){
            PerKeyDown(PadButtonsY);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"d"] ){
            PerKeyDown(PadButtonsZ);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"q"] ){
            PerKeyDown(PadButtonsLeftTrigger);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"e"] ){
            PerKeyDown(PadButtonsRightTrigger);
            handleEvent++;
        }
    }
    
    if( handleEvent == 0){
        [super pressesBegan:presses withEvent:event];
    }
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses
           withEvent:(UIPressesEvent *)event
{
    int handleEvent = 0;
    for( UIPress * press in presses ){
        if( [press.key.charactersIgnoringModifiers isEqualToString:UIKeyInputLeftArrow] ){
            PerKeyUp(PadButtonsLeft);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:UIKeyInputRightArrow] ){
            PerKeyUp(PadButtonsRight);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:UIKeyInputUpArrow] ){
            PerKeyUp(PadButtonsUp);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:UIKeyInputDownArrow] ){
            PerKeyUp(PadButtonsDown);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"\r"] ){
            PerKeyUp(PadButtonsStart);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"z"] ){
            PerKeyUp(PadButtonsA);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"x"] ){
            PerKeyUp(PadButtonsB);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"c"] ){
            PerKeyUp(PadButtonsC);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"a"] ){
            PerKeyUp(PadButtonsX);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"s"] ){
            PerKeyUp(PadButtonsY);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"d"] ){
            PerKeyUp(PadButtonsZ);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"q"] ){
            PerKeyUp(PadButtonsLeftTrigger);
            handleEvent++;
        }
        else if( [press.key.charactersIgnoringModifiers isEqualToString:@"e"] ){
            PerKeyUp(PadButtonsRightTrigger);
            handleEvent++;
        }

    }
    
    if( handleEvent == 0){
        [super pressesEnded
         :presses withEvent:event];
    }
}


- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    //NSSet *allTouches = [event allTouches];
    for (UITouch *touch in touches)
    {
        if( [_pad_buttons_[PadButtonsDown] isOn] && _pad_buttons_[PadButtonsDown].getPointId == touch ){

            UIView * target = _pad_buttons_[PadButtonsDown].target;
            CGPoint point = [touch locationInView:target];
            if( point.y < 0 ){
                [_pad_buttons_[PadButtonsDown] off ];
                //printf("PAD:%d OFF\n",(int)BUTTON_DOWN );
            }
        }
        if( [_pad_buttons_[PadButtonsUp] isOn] && _pad_buttons_[PadButtonsUp].getPointId == touch ){
            
            UIView * target = _pad_buttons_[PadButtonsUp].target;
            CGPoint point = [touch locationInView:target];
            //printf("PAD:%d MOVE %d\n",(int)BUTTON_UP,(int)point.y);
            if( point.y > target.bounds.size.height){
                [_pad_buttons_[PadButtonsUp] off ];
                //printf("PAD:%d OFF\n",(int)BUTTON_UP );
            }
        }
        
        UIView * target = _pad_buttons_[PadButtonsDown].target;
        CGPoint point = [touch locationInView:target];
        if( CGRectContainsPoint( target.bounds , point) ) {
            [_pad_buttons_[PadButtonsDown] on:touch];
            //printf("PAD:%d ON\n",(int)BUTTON_DOWN );
        }
        target = _pad_buttons_[PadButtonsUp].target;
        point = [touch locationInView:target];
        if( CGRectContainsPoint( target.bounds , point) ) {
            [_pad_buttons_[PadButtonsUp] on:touch];
            //printf("PAD:%d ON\n",(int)BUTTON_UP );
        }

        if( [_pad_buttons_[PadButtonsRight] isOn] && _pad_buttons_[PadButtonsRight].getPointId == touch ){
            
            UIView * target = _pad_buttons_[PadButtonsRight].target;
            CGPoint point = [touch locationInView:target];
            if( point.x < 0 ){
                [_pad_buttons_[PadButtonsRight] off ];
                //printf("PAD:%d OFF\n",(int)BUTTON_RIGHT );
            }
        }
        if( [_pad_buttons_[PadButtonsLeft] isOn] && _pad_buttons_[PadButtonsLeft].getPointId == touch ){
            
            UIView * target = _pad_buttons_[PadButtonsLeft].target;
            CGPoint point = [touch locationInView:target];
            if( point.x > target.bounds.size.width){
                [_pad_buttons_[PadButtonsLeft] off ];
                //printf("PAD:%d OFF\n",(int)BUTTON_LEFT );
            }
        }
        
        target = _pad_buttons_[PadButtonsLeft].target;
        point = [touch locationInView:target];
        if( CGRectContainsPoint( target.bounds , point) ) {
            [_pad_buttons_[PadButtonsLeft] on:touch];
            //printf("PAD:%d ON\n",(int)BUTTON_LEFT );
        }
        target = _pad_buttons_[PadButtonsRight].target;
        point = [touch locationInView:target];
        if( CGRectContainsPoint( target.bounds , point) ) {
            [_pad_buttons_[PadButtonsRight] on:touch];
            //printf("PAD:%d ON\n",(int)BUTTON_RIGHT );
        }


        for (int btnindex = PadButtonsRightTrigger; btnindex < PadButtonsLast; btnindex++) {

            UIView * target = _pad_buttons_[btnindex].target;
            CGPoint point = [touch locationInView:target];

            if( _pad_buttons_[btnindex].getPointId == touch ) {
                if( !CGRectContainsPoint( target.bounds , point) ) {
                    [_pad_buttons_[btnindex] off];
                    //printf("touchesMoved PAD:%d OFF\n",btnindex);
                }
            }else{
                if( CGRectContainsPoint( target.bounds , point) ) {
                    [_pad_buttons_[btnindex] on:touch];
                    //printf("touchesMoved PAD:%d MOVE\n",btnindex);
                }else{
                    //printf("%d: [%d,%d,%d,%d]-[%d,%d]\n", btnindex, (int)target.frame.origin.x, (int)target.frame.origin.y, (int)target.bounds.size.width, (int)target.bounds.size.height, (int)point.x, (int)point.y);
                }
            }
        }
    }
    
    if( [self hasControllerConnected ] ) return;
    for (int btnindex = 0; btnindex < PadButtonsLast; btnindex++) {
        if ([_pad_buttons_[btnindex] isOn] ) {
            PerKeyDown(btnindex);
        } else {
            PerKeyUp(btnindex);
        }
    }

}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{

    //NSSet *allTouches = [event allTouches];
    for (UITouch *touch in touches)
    {
        for (int btnindex = 0; btnindex < PadButtonsLast; btnindex++) {
            if ( [_pad_buttons_[btnindex] isOn] && [_pad_buttons_[btnindex] getPointId] == touch)  {
                [_pad_buttons_[btnindex] off ];
                //printf("touchesEnded PAD:%d Up\n",btnindex);
                if( self.isKeyRemappingMode ){
                    [self showRemapControlAlertWithSaturnKey:btnindex];
                }
            }
        }
    }
    
    if( [self hasControllerConnected ] ) return;
    for (int btnindex = 0; btnindex < PadButtonsLast; btnindex++) {
        if ([_pad_buttons_[btnindex] isOn] ) {
            PerKeyDown(btnindex);
        } else {
            PerKeyUp(btnindex);
        }
    }

}

#pragma mark AVAudioSession
- (void)handleInterruption:(NSNotification *)notification
{
    UInt8 theInterruptionType = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
    
    NSLog(@"Session interrupted > --- %s ---\n", theInterruptionType == AVAudioSessionInterruptionTypeBegan ? "Begin Interruption" : "End Interruption");
    
    if (theInterruptionType == AVAudioSessionInterruptionTypeBegan) {
        //alcMakeContextCurrent(NULL);
        //if (self.isPlaying) {
        //    self.wasInterrupted = YES;
        //}
    } else if (theInterruptionType == AVAudioSessionInterruptionTypeEnded) {
        // make sure to activate the session
        NSError *error;
        bool success = [[AVAudioSession sharedInstance] setActive:YES error:&error];
        if (!success) NSLog(@"Error setting session active! %@\n", [error localizedDescription]);
        
        //alcMakeContextCurrent(self.context);
        
        //if (self.wasInterrupted)
        //{
         //   [self startSound];
         //   self.wasInterrupted = NO;
        //}
    }
}

#pragma mark -Audio Session Route Change Notification

- (void)handleRouteChange:(NSNotification *)notification
{
    UInt8 reasonValue = [[notification.userInfo valueForKey:AVAudioSessionRouteChangeReasonKey] intValue];
    AVAudioSessionRouteDescription *routeDescription = [notification.userInfo valueForKey:AVAudioSessionRouteChangePreviousRouteKey];
    
    NSLog(@"Route change:");
    switch (reasonValue) {
        case AVAudioSessionRouteChangeReasonNewDeviceAvailable:
            NSLog(@"     NewDeviceAvailable");
            break;
        case AVAudioSessionRouteChangeReasonOldDeviceUnavailable:
            NSLog(@"     OldDeviceUnavailable");
            break;
        case AVAudioSessionRouteChangeReasonCategoryChange:
            NSLog(@"     CategoryChange");
            NSLog(@" New Category: %@", [[AVAudioSession sharedInstance] category]);
            break;
        case AVAudioSessionRouteChangeReasonOverride:
            NSLog(@"     Override");
            break;
        case AVAudioSessionRouteChangeReasonWakeFromSleep:
            NSLog(@"     WakeFromSleep");
            break;
        case AVAudioSessionRouteChangeReasonNoSuitableRouteForCategory:
            NSLog(@"     NoSuitableRouteForCategory");
            break;
        default:
            NSLog(@"     ReasonUnknown");
    }
    
    NSLog(@"Previous route:\n");
    NSLog(@"%@", routeDescription);
}

- (void)controllerDidConnect:(NSNotification *)notification
{
    @synchronized (_objectForLock){
        [self foundController];
    }
}

- (void)controllerDidDisconnect
{
    @synchronized (_objectForLock){
        [self setControllerOverlayHidden:NO];
        [self updateSideMenu];
    }
}

-(void)completionWirelessControllerDiscovery
{
    
    void (^mfiButtonHandler)(KeyMapMappableButton,BOOL) = ^void(KeyMapMappableButton mfiButton, BOOL pressed) {
        if ( self.isKeyRemappingMode && self.currentlyMappingKey != NSNotFound ) {
            [self.keyMapper mapKey:self.currentlyMappingKey toControl:mfiButton];
            if ( self.remapAlertController != nil ) {
                [self.remapAlertController dismissViewControllerAnimated:YES completion:nil];
            }
            [self.keyMapper saveKeyMapping];
            self.currentlyMappingKey = NSNotFound;
            [self refreshViewsWithKeyRemaps];
        } else {
//            SaturnKey saturnKey = [self.keyMapper getMappedKeyForControl:mfiButton];
            NSNumber *mappedKeyNumber = [self.keyMapper getMappedKeyForControl:mfiButton];
            if (mappedKeyNumber != nil) {
                SaturnKey mappedKey = (SaturnKey)mappedKeyNumber.integerValue;
                if(pressed){
                    PerKeyDown(mappedKey);
                }else{
                    PerKeyUp(mappedKey);
                }
            }
        }
    };
    
    if( [GCController controllers].count >= 1 ){
        self.controller = [GCController controllers][0];
        if (self.controller.gamepad) {

            [self.controller.extendedGamepad.buttonHome setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(KeyMapMappableButtonMFI_BUTTON_HOME, pressed);
            }];

            [self.controller.extendedGamepad.buttonMenu setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(KeyMapMappableButtonMFI_BUTTON_MENU, pressed);
                GameRevealViewController *revealViewController = (GameRevealViewController *)self.revealViewController;
                if ( revealViewController && pressed)
                {
                    [revealViewController revealToggle:(0)];
                }
            }];

            [self.controller.extendedGamepad.buttonOptions setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(KeyMapMappableButtonMFI_BUTTON_OPTION, pressed);
            }];

            
            [self.controller.extendedGamepad.buttonA setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(KeyMapMappableButtonMFI_BUTTON_A, pressed);
            }];
            
            [self.controller.extendedGamepad.rightShoulder setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(KeyMapMappableButtonMFI_BUTTON_RS, pressed);
            }];
            
            [self.controller.extendedGamepad.leftShoulder setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(KeyMapMappableButtonMFI_BUTTON_LS, pressed);
            }];
            
            [self.controller.extendedGamepad.leftTrigger setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(KeyMapMappableButtonMFI_BUTTON_LT, pressed);
            }];
            
            [self.controller.extendedGamepad.rightTrigger setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(KeyMapMappableButtonMFI_BUTTON_RT, pressed);
            }];
            
            [self.controller.extendedGamepad.buttonX setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(KeyMapMappableButtonMFI_BUTTON_X, pressed);
            }];
            [self.controller.extendedGamepad.buttonY setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(KeyMapMappableButtonMFI_BUTTON_Y, pressed);
            }];
            [self.controller.extendedGamepad.buttonB setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                mfiButtonHandler(KeyMapMappableButtonMFI_BUTTON_B, pressed);
            }];
            [self.controller.extendedGamepad.dpad.up setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(SaturnKeyUp);
                }else{
                    PerKeyUp(SaturnKeyUp);
                }
            }];
            [self.controller.extendedGamepad.dpad.down setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(SaturnKeyDown);
                }else{
                    PerKeyUp(SaturnKeyDown);
                }
            }];
            [self.controller.extendedGamepad.dpad.left setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(SaturnKeyLeft);
                }else{
                    PerKeyUp(SaturnKeyLeft);
                }
            }];
            [self.controller.extendedGamepad.dpad.right setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
                if(pressed){
                    PerKeyDown(SaturnKeyRight);
                }else{
                    PerKeyUp(SaturnKeyRight);
                }
            }];
            /*
            [self.controller.extendedGamepad.rightThumbstick setValueChangedHandler:^(GCControllerDirectionPad *dpad, float xValue, float yValue) {
                if(yValue >= 0.5 || yValue <= -0.5 ){
                    PerKeyDown(SaturnKeyStart);
                }else{
                    PerKeyUp(SaturnKeyStart);
                }
            }];
            */
        }
    }
    
}

- (BOOL)hasControllerConnected {
    return [[GCController controllers] count] > 0;
}

- (IBAction)onMenu:(id)sender {
    GameRevealViewController *revealViewController = (GameRevealViewController *)self.revealViewController;
    if ( revealViewController )
    {
        [revealViewController revealToggle:(0)];
    }
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    if ([self hasControllerConnected]) {
        NSLog(@"Discovery finished on first pass");
        [self foundController];
    } else {
        NSLog(@"Discovery happening patiently");
        [self patientlyDiscoverController];
    }
    canRotateToAllOrientations = YES;
    [self becomeFirstResponder];
}

- (void)patientlyDiscoverController {
    
    [GCController startWirelessControllerDiscoveryWithCompletionHandler:nil];
    
    int count = [[GCController controllers] count];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(foundController)
                                                 name:GCControllerDidConnectNotification
                                               object:nil];
}   

- (void)foundController {
    NSLog(@"Found Controller");
    

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(controllerDidDisconnect)
                                                 name:GCControllerDidDisconnectNotification
                                               object:nil];
    @synchronized (_objectForLock){
    [self refreshViewsWithKeyRemaps];
    [self setControllerOverlayHidden:YES];
    [self completionWirelessControllerDiscovery];
    [self updateSideMenu];
    }
}

-(void)setControllerOverlayHidden:(BOOL)hidden {
    
    [self left_panel ].hidden = hidden;
    [self right_panel ].hidden = hidden;
    [self left_button ].hidden = hidden;
    [self right_button ].hidden = hidden;
    [self up_button ].hidden = hidden;
    [self down_button ].hidden = hidden;
    [self a_button ].hidden = hidden;
    [self b_button ].hidden = hidden;
    [self c_button ].hidden = hidden;
    [self x_button ].hidden = hidden;
    [self y_button ].hidden = hidden;
    [self z_button ].hidden = hidden;
    [self left_trigger ].hidden = hidden;
    [self right_trigger ].hidden = hidden;
    [self start_button ].hidden = hidden;
    [self start_view].hidden = hidden;
    
    [self.remapLabelViews enumerateObjectsUsingBlock:^(UILabel *  _Nonnull obj, NSUInteger idx, BOOL * _Nonnull stop) {
        obj.hidden = [self hasControllerConnected] && hidden == NO ? NO : hidden;
    }];
     
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self loadSettings];
    
    sharedData_ = self;
    _objectForLock = [[NSObject alloc] init];
    self._isFirst = YES;
    
    self.glView.drawableDepthFormat = MGLDrawableDepthFormat24;
    self.glView.drawableStencilFormat = MGLDrawableStencilFormat8;

    GameRevealViewController *revealViewController = (GameRevealViewController *)self.revealViewController;
    if ( revealViewController )
    {
        [self.view addGestureRecognizer:self.revealViewController.panGestureRecognizer];
        self.selected_file = revealViewController.selected_file;
    }
    
    self.view.multipleTouchEnabled = YES;
    self.command = 0;
    
    [self right_view].backgroundColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.0f];
    [self left_view].backgroundColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.0f];
    [self start_view].backgroundColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.0f];
    [self left_button ].alpha = 0.0f;
    [self right_button ].alpha = 0.0f;
    [self up_button ].alpha = 0.0f;
    [self down_button ].alpha = 0.0f;
    [self a_button ].alpha = 0.0f;
    [self b_button ].alpha = 0.0f;
    [self c_button ].alpha = 0.0f;
    [self x_button ].alpha = 0.0f;
    [self y_button ].alpha = 0.0f;
    [self z_button ].alpha = 0.0f;
    [self left_trigger ].alpha = 0.0f;
    [self right_trigger ].alpha = 0.0f;
    [self start_button ].alpha = 0.0f;
 
    _pad_buttons_ = [[NSMutableArray alloc] initWithCapacity:PadButtonsLast];
    // Create and add PadButton objects to the _pad_buttons_ array
    PadButton *upButton = [[PadButton alloc] init];
    upButton.target = self.up_button;
    [_pad_buttons_ addObject:upButton];

    PadButton *rightButton = [[PadButton alloc] init];
    rightButton.target = self.right_button;
    [_pad_buttons_ addObject:rightButton];
    
    PadButton *downButton = [[PadButton alloc] init];
    downButton.target = self.down_button;
    [_pad_buttons_ addObject:downButton];

    PadButton *leftButton = [[PadButton alloc] init];
    leftButton.target = self.left_button;
    [_pad_buttons_ addObject:leftButton];

    PadButton *rightTriggerButton = [[PadButton alloc] init];
    rightTriggerButton.target = self.right_trigger;
    [_pad_buttons_ addObject:rightTriggerButton];

    PadButton *leftTriggerButton = [[PadButton alloc] init];
    leftTriggerButton.target = self.left_trigger;
    [_pad_buttons_ addObject:leftTriggerButton];

    PadButton *startButton = [[PadButton alloc] init];
    startButton.target = self.start_button;
    [_pad_buttons_ addObject:startButton];
    
    PadButton *aButton = [[PadButton alloc] init];
    aButton.target = self.a_button;
    [_pad_buttons_ addObject:aButton];

    PadButton *bButton = [[PadButton alloc] init];
    bButton.target = self.b_button;
    [_pad_buttons_ addObject:bButton];

    PadButton *cButton = [[PadButton alloc] init];
    cButton.target = self.c_button;
    [_pad_buttons_ addObject:cButton];

    PadButton *xButton = [[PadButton alloc] init];
    xButton.target = self.x_button;
    [_pad_buttons_ addObject:xButton];

    PadButton *yButton = [[PadButton alloc] init];
    yButton.target = self.y_button;
    [_pad_buttons_ addObject:yButton];

    PadButton *zButton = [[PadButton alloc] init];
    zButton.target = self.z_button;
    [_pad_buttons_ addObject:zButton];
   
     
    self.preferredFramesPerSecond = 60;
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(didEnterBackground) name:UIApplicationDidEnterBackgroundNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(didBecomeActive) name:UIApplicationDidBecomeActiveNotification object:nil];
    
    self.context = [[MGLContext alloc] initWithAPI:kMGLRenderingAPIOpenGLES3];
    //self.context.multiThreaded = YES;
    self.share_context = [[MGLContext alloc] initWithAPI:kMGLRenderingAPIOpenGLES3 sharegroup:[self.context sharegroup] ];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }

    g_share_context = self.share_context;
    g_context = self.context;
    
    self.glView.context = self.context;
    [MGLContext setCurrentContext:self.context];
    glLayer = self.glView.glLayer;
    
    [self setup];
    
    iOSCoreAudioInit();
    
    
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        saturnKeyDescriptions =
        @{
          [NSNumber numberWithInteger:SaturnKeyA] : @"A",
          [NSNumber numberWithInteger:SaturnKeyB] : @"B",
          [NSNumber numberWithInteger:SaturnKeyC] : @"C",
          [NSNumber numberWithInteger:SaturnKeyX] : @"X",
          [NSNumber numberWithInteger:SaturnKeyY] : @"Y",
          [NSNumber numberWithInteger:SaturnKeyZ] : @"Z",
          [NSNumber numberWithInteger:SaturnKeyLeftTrigger] : @"LT",
          [NSNumber numberWithInteger:SaturnKeyRightTrigger] : @"RT",
          [NSNumber numberWithInteger:SaturnKeyStart] : @"Start",
          };
        
        saturnKeyToViewMappings =
        @{
          [NSNumber numberWithInteger:SaturnKeyA] : _a_button,
          [NSNumber numberWithInteger:SaturnKeyB] : _b_button,
          [NSNumber numberWithInteger:SaturnKeyC] : _c_button,
          [NSNumber numberWithInteger:SaturnKeyX] : _x_button,
          [NSNumber numberWithInteger:SaturnKeyY] : _y_button,
          [NSNumber numberWithInteger:SaturnKeyZ] : _z_button,
          [NSNumber numberWithInteger:SaturnKeyLeftTrigger] : _left_trigger,
          [NSNumber numberWithInteger:SaturnKeyRightTrigger] : _right_trigger,
          [NSNumber numberWithInteger:SaturnKeyStart] : _start_button,
          };
    });
    self.keyMapper = [[KeyMapper alloc] init];
    [self.keyMapper loadFromDefaults];
    self.remapLabelViews = [NSMutableArray array];
    
    self.scale_slider.hidden = YES;
    self.scale_slider.minimumValue = 0.5;
    self.scale_slider.maximumValue = 1.0;
    self.scale_slider.value = _controller_scale;
    [self.scale_slider addTarget:self action:@selector(didValueChanged:) forControlEvents:UIControlEventValueChanged];
    controller_edit_mode = 0;
    [self updateControllScale :_controller_scale];
    
}

- (BOOL)shouldAutorotate {
    return !_landscape;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return _landscape ? UIInterfaceOrientationMaskLandscape : UIInterfaceOrientationMaskAll;
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation {
    return _landscape ? UIInterfaceOrientationLandscapeRight : UIInterfaceOrientationPortrait;
}

- (void)updateControllScale:( float ) scale{
    
    UIView * lfv = self.left_view;
    CGAffineTransform tf = CGAffineTransformMakeScale(1.0,1.0);
    tf = CGAffineTransformTranslate(tf, -(258.0/2.0), (340.0/2.0));
    tf = CGAffineTransformScale(tf, scale, scale);
    tf = CGAffineTransformTranslate(tf, (258.0/2.0), -(340.0/2.0));
    lfv.transform = tf;
    
    UIView * rfv = self.right_view;
    tf = CGAffineTransformMakeScale(1.0,1.0);
    tf = CGAffineTransformTranslate(tf, (258.0/2.0), (340.0/2.0));
    tf = CGAffineTransformScale(tf, scale, scale);
    tf = CGAffineTransformTranslate(tf, -(258.0/2.0), -(340.0/2.0));
    rfv.transform = tf;
    
    UIView * sfv = self.start_view;
    tf = CGAffineTransformMakeScale(1.0,1.0);
    //tf = CGAffineTransformTranslate(tf, (258.0/2.0), (340.0/2.0));
    tf = CGAffineTransformScale(tf, scale, scale);
    //tf = CGAffineTransformTranslate(tf, -(258.0/2.0), -(340.0/2.0));
    sfv.transform = tf;
    
    NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
    [ud setFloat:scale forKey:@"controller scale"];
    [ud synchronize];

}


- (void)didValueChanged:( UISlider *)slider
{
    float scale_val = slider.value;
    [self updateControllScale :scale_val ];
}

- (void)toggleControllerEditMode {
    if(controller_edit_mode==0){
        self.scale_slider.hidden = NO;
        controller_edit_mode = 1;
    }else{
        self.scale_slider.hidden = YES;
        controller_edit_mode = 0;
    }
}

- (void)dealloc
{
    iOSCoreAudioShutdown();
    
    [self tearDownGL];
    
    if ([MGLContext currentContext] == self.context) {
        [MGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([MGLContext currentContext] == self.context) {
            [MGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)setup
{
    [MGLContext setCurrentContext:self.context];
    CGFloat scale = [[UIScreen mainScreen] scale];
    CGRect newFrame = self.view.frame;
    UIEdgeInsets safeArea = self.view.safeAreaInsets;
    
    if( self._isFirst == YES){
        start_emulation(safeArea.left*scale,safeArea.bottom*scale, (newFrame.size.width-safeArea.right-safeArea.left)*scale  , (newFrame.size.height - safeArea.top-safeArea.bottom) *scale );
        self._isFirst = NO;
    }else{
        resize_screen(safeArea.left*scale,safeArea.bottom*scale, (newFrame.size.width-safeArea.right-safeArea.left)*scale  , (newFrame.size.height - safeArea.top-safeArea.bottom) *scale );
    }
    self._return = YES;
}

- (void)tearDownGL
{
    [MGLContext setCurrentContext:self.context];
   
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)emulate_one_frame
{
    if( self._return == YES ){
        self._return = NO;
    }
    emulation_step(self.command);
    self.command = 0;
}

- (void)viewDidLayoutSubviews
{
    CGFloat scale = [[UIScreen mainScreen] scale];
    CGRect newFrame = self.view.frame;
    UIEdgeInsets safeArea = self.view.safeAreaInsets;
    @synchronized (_objectForLock){
        resize_screen(safeArea.left*scale,safeArea.bottom*scale, (newFrame.size.width-safeArea.right-safeArea.left)*scale  , (newFrame.size.height - safeArea.top-safeArea.bottom) *scale );
    }
}


- (void)didEnterBackground {
    enterBackGround();
    [self setPaused:true];
}

- (void)didBecomeActive {
    [self.view becomeFirstResponder];
    [self setPaused:false];
    self._return = YES;
}

- (BOOL)canBecomeFirstResponder {
    return YES;
}

#pragma mark -
#pragma mark UIKeyInput Protocol Methods

- (BOOL)hasText {
    return NO;
}

- (void)insertText:(NSString *)text {
    NSLog(@"Key Input %@\n", text);
}

- (void)deleteBackward {

}

#pragma mark -
#pragma mark Key Remapping

-(IBAction)startKeyRemapping:(id)sender {
    [self refreshViewsWithKeyRemaps];
    if ( self.isKeyRemappingMode ) {
        self.isKeyRemappingMode = NO;
        [self setControllerOverlayHidden:YES];
    } else {
        self.isKeyRemappingMode = YES;
        [self setControllerOverlayHidden:NO];
    }
}

-(void)refreshViewsWithKeyRemaps {
    if ( self.remapLabelViews.count > 0 ) {
        [self.remapLabelViews makeObjectsPerformSelector:@selector(removeFromSuperview)];
    }
    [self.remapLabelViews removeAllObjects];
    
    for (NSNumber *button in saturnKeyToViewMappings.allKeys) {
        NSMutableString *buttonNames = [NSMutableString string];
        NSArray *mfiButtons = [self.keyMapper getControlsForMappedKey:button.integerValue];
        NSUInteger mfiButtonIndex = 0;
        for (NSNumber *mfiButton in mfiButtons) {
            if ( mfiButtonIndex++ > 0 ) {
                [buttonNames appendString:@" / "];
            }
            [buttonNames appendString:[KeyMapper controlToDisplayName:mfiButton.integerValue]];
        }
        if ( mfiButtons.count > 0 ) {
            UILabel *mappedLabel = [[UILabel alloc] initWithFrame:CGRectZero];
            mappedLabel.text = buttonNames;
            mappedLabel.alpha = 0.6f;
            mappedLabel.font = [UIFont boldSystemFontOfSize:16.0];
            mappedLabel.textColor = [UIColor redColor];
            mappedLabel.translatesAutoresizingMaskIntoConstraints = NO;
            UIView *saturnButtonView = [saturnKeyToViewMappings objectForKey:button];
            [self.view addSubview:mappedLabel];
            [self.view addConstraint:[NSLayoutConstraint constraintWithItem:mappedLabel attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:saturnButtonView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0.0]];
            [self.view addConstraint:[NSLayoutConstraint constraintWithItem:mappedLabel attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:saturnButtonView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0.0]];
            [self.view bringSubviewToFront:mappedLabel];
            [self.remapLabelViews addObject:mappedLabel];
            [self.view setNeedsLayout];
        }
    }
}


-(void)showRemapControlAlertWithSaturnKey:(SaturnKey)saturnKey {
    self.remapAlertController = [UIAlertController alertControllerWithTitle:@"Remap Key"                                                                message:[NSString stringWithFormat:@"Press a button to map the [%@] key",[saturnKeyDescriptions objectForKey:[NSNumber numberWithInteger:saturnKey]]]
        preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction *cancel = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
        self.isKeyRemappingMode = NO;
        [self setControllerOverlayHidden:YES];
        [self.remapAlertController dismissViewControllerAnimated:YES completion:nil];
        self.currentlyMappingKey = NSNotFound;
    }];
    UIAlertAction *unbind = [UIAlertAction actionWithTitle:@"Unbind" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        [self.keyMapper unmapKey:saturnKey];
        self.currentlyMappingKey = NSNotFound;
        [self refreshViewsWithKeyRemaps];
        [self.remapAlertController dismissViewControllerAnimated:YES completion:nil];
    }];
    self.isKeyRemappingMode = YES;
    [self.remapAlertController addAction:cancel];
    [self.remapAlertController addAction:unbind];
    self.currentlyMappingKey = saturnKey;
    [self presentViewController:self.remapAlertController animated:YES completion:nil];
}

-(void)updateSideMenu {
    GameRevealViewController *revealViewController = (GameRevealViewController *)self.revealViewController;
    if ( revealViewController )
    {
        SidebarViewController *svc = (SidebarViewController*) revealViewController.rearViewController;
        [svc refreshContents];
    }
}

-(BOOL)isCurrentlyRemappingControls {
    return self.isKeyRemappingMode;
}

- (CGSize)size
{
    return self.glView.drawableSize;
}

- (BOOL)update
{
    [self emulate_one_frame];
    return FALSE;
}

- (void)mglkView:(MGLKView *)view drawInRect:(CGRect)rect
{
  

}


@end
