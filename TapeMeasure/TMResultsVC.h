//
//  TMResultsVC.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "TMOptionsVC.h"

@class TMMeasurement;

@protocol OptionsDelegate;

@interface TMResultsVC : UITableViewController <NSFetchedResultsControllerDelegate, UITextFieldDelegate, UIActionSheetDelegate, OptionsDelegate>
{
    UIActionSheet *sheet;
    NSURLConnection *theConnection;
}

- (IBAction)handleDeleteButton:(id)sender;
- (IBAction)handleUpgradeButton:(id)sender;
- (IBAction)handleDoneButton:(id)sender;
- (IBAction)handleKeyboardDone:(id)sender;
- (IBAction)handleActionButton:(id)sender;

@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnDone;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnAction;

@property (nonatomic, strong) TMMeasurement *theMeasurement;

@end
