//
//  SidebarViewController.h
//  SidebarDemo
//
//  Created by Simon on 29/6/13.
//  Copyright (c) 2013 Appcoda. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface SidebarViewController : UITableViewController  <UITableViewDelegate, UITableViewDataSource>

@property (strong, nonatomic) IBOutlet UILabel *showRemapControlsLabel;

-(void)refreshContents;

@end

