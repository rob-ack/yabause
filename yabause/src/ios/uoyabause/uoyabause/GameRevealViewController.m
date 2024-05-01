//
//  GameRevealViewController.m
//  uoyabause
//
//  Created by MiyamotoShinya on 2016/09/11.
//  Copyright © 2016年 devMiyax. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "GameRevealViewController.h"
#import "GameViewController.h"

@interface GameRevealViewController () {
    BOOL _landscape;
}
@end

@implementation  GameRevealViewController
@synthesize selected_file;


- (void)loadView
{
    [super loadView];
    NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
    _landscape = [ud boolForKey:@"landscape"];
    
}

//パラメータのセット(option)
-(void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender{
    if ([segue.identifier isEqualToString:@"back_to_game_selection"]) {
        //ViewController *vcA=(ViewAController*)segue.destinationViewController;
    }
    //vcA.foo=bar;
    //[vcA baz];
}

//unwind
-(IBAction)unwindForSegue:(UIStoryboardSegue *)unwindSegue towardsViewController:(UIViewController *)subsequentVC{
}

- (BOOL)shouldAutorotate {
    // _landscape が YES の場合には回転を禁止し、固定向きを保持
    return !_landscape;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return _landscape ? UIInterfaceOrientationMaskLandscape : UIInterfaceOrientationMaskAll;
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation {
    return _landscape ? UIInterfaceOrientationLandscapeRight : UIInterfaceOrientationPortrait;
}

@end
