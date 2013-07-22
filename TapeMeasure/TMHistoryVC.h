//
//  TMHistoryVC.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"
#import "TMAppDelegate.h"
#import "TMResultsVC.h"
#import "TMAppDelegate.h"
#import <RC3DK/RC3DK.h>
#import "TMDataManagerFactory.h"
#import "TMMeasurement+TMMeasurementExt.h"
#import "TMMeasurement+TMMeasurementSync.h"
#import "TMSyncable+TMSyncableSync.h"
#import "MBProgressHUD.h"
#import "TMServerOpsFactory.h"
#import "RCCore/RCUserManager.h"
#import "TMTableViewController.h"

@protocol ModalViewDelegate

- (void)didDismissModalView;

@end

@interface TMHistoryVC : TMTableViewController <NSFetchedResultsControllerDelegate, ModalViewDelegate, UIActionSheetDelegate, UIAlertViewDelegate>
{
    NSNumber *unitsPref;
    NSNumber *fractionalPref;
    
    NSFetchedResultsController *fetchedResultsController;
    NSManagedObjectContext *managedObjectContext;
    NSArray *measurementsData;
    
    UIActionSheet *actionSheet;
    
    MBProgressHUD *HUD;
}

typedef enum {
    AlertLoginFailure = 100, AlertLocation = 200
} AlertId;

@property (weak, nonatomic) IBOutlet UIBarButtonItem *actionButton;

- (IBAction)handleActionButton:(id)sender;

@end
