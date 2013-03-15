//
//  TMHistoryVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMHistoryVC.h"

@implementation TMHistoryVC

#pragma mark - Event handlers

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self refreshPrefs];
    [self loginOrCreateAnonAccount];
}

- (void)viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.History"];
    [self refreshTableView];
    [self performSelectorInBackground:@selector(setupDataCapture) withObject:nil];
}

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:YES animated:animated];
    [super viewWillAppear:animated];
}

- (void)viewDidUnload {
    [self setActionButton:nil];
    [super viewDidUnload];
}

- (void)didDismissModalView
{
    NSLog(@"didDismissModalView");
    [self refreshPrefs];
    [self.tableView reloadData];
}

- (void) prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([[segue identifier] isEqualToString:@"toResult"])
    {
        NSIndexPath *indexPath = (NSIndexPath*)sender;
        TMMeasurement *measurement = [measurementsData objectAtIndex:indexPath.row];
        
        TMResultsVC *resultsVC = [segue destinationViewController];
        resultsVC.theMeasurement = measurement;
        
        UIBarButtonItem *backBtn = [[UIBarButtonItem alloc] initWithTitle:@"History" style:UIBarButtonItemStyleBordered target:nil action:nil];
        self.navigationItem.backBarButtonItem = backBtn;
    }
    else if([[segue identifier] isEqualToString:@"toType"])
    {
        UIBarButtonItem *backBtn = [[UIBarButtonItem alloc] initWithTitle:@"Cancel" style:UIBarButtonItemStyleBordered target:nil action:nil];
        self.navigationItem.backBarButtonItem = backBtn;
    }
}

#pragma mark - Private methods

- (void) loginOrCreateAnonAccount
{
    if ([USER_MANAGER getLoginState] == LoginStateYes)
    {
        [self syncWithServer];
    }
    else
    {
        if ([USER_MANAGER hasValidStoredCredentials])
        {
            [self login];
        }
        else
        {
            [SERVER_OPS
             createAnonAccount: ^{
                 [self login];
             }
             onFailure: ^{
                 //fail silently. will try again next time app is started.
             }];
        }
    }
}

- (void) login
{
    [SERVER_OPS
     login: ^{
         [self syncWithServer];
     }
     onFailure: ^(int statusCode){
         if (![USER_MANAGER isUsingAnonAccount])
         {
             [self logout];
             
             UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Whoops"
                                                             message:@"Failed to login. Press OK to enter your login details again."
                                                            delegate:self
                                                   cancelButtonTitle:@"Not now"
                                                   otherButtonTitles:@"OK", nil];
             alert.tag = AlertLoginFailure;
             [alert show];
         }
     }];
}

- (void) alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (alertView.tag == AlertLoginFailure)
    {
        if (buttonIndex == 1) [self performSegueWithIdentifier:@"toLogin" sender:self];
    }
}

- (void) syncWithServer
{
    [SERVER_OPS
     syncWithServer: ^{
        [self refreshTableView];
     }
     onFailure: ^{
         NSLog(@"Sync failure callback");
     }];
}

- (void) logout
{
    HUD = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
    [self.navigationController.view addSubview:HUD];
    HUD.labelText = @"Thinking..";
    [HUD show:YES];
    
    [SERVER_OPS logout:^{
        [self handleLogoutDone];
    }];
}

- (void) handleLogoutDone
{
    [self refreshTableView];
    
    [SERVER_OPS createAnonAccount:^{
        [SERVER_OPS login:^{
            [HUD hide:YES];
        }
        onFailure:^(int statusCode){
            NSLog(@"Login failure callback");
            [HUD hide:YES];
        }];
    }
    onFailure:^{
        NSLog(@"Create anon account failure callback");
        [HUD hide:YES];
    }];
}

/** Expensive. Can cause UI to lag if called at the wrong time. */
- (void)setupDataCapture
{
    [RCAVSessionManagerFactory setupAVSession];
    [RCVideoCapManagerFactory setupVideoCapWithSession:[SESSION_MANAGER session]];
}

- (void)refreshPrefs
{
    unitsPref = [[NSUserDefaults standardUserDefaults] objectForKey:@"Units"];
    fractionalPref = [[NSUserDefaults standardUserDefaults] objectForKey:@"Fractional"];
}

- (void)loadTableData
{
    NSLog(@"loadTableData");
    
    measurementsData = [TMMeasurement getAllExceptDeleted];
}

- (void)refreshTableView
{
    [self loadTableData];
    [self.tableView reloadData];
}

- (void)deleteMeasurement:(NSIndexPath*)indexPath
{
    [self.tableView beginUpdates];
     
    TMMeasurement *theMeasurement = [measurementsData objectAtIndex:indexPath.row];
    theMeasurement.deleted = YES;
    theMeasurement.syncPending = YES;
    [DATA_MANAGER saveContext];
    
    [Flurry logEvent:@"Measurement.Delete.History"];
    
    [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    [self loadTableData];
    [self.tableView endUpdates];
    
    [theMeasurement
     putToServer:^(int transId) {
         NSLog(@"putMeasurement success callback");
         [theMeasurement deleteFromDb];
         [DATA_MANAGER saveContext];
     }
     onFailure:^(int statusCode) {
         NSLog(@"putMeasurement failure callback");
     }
    ];
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    // Return the number of sections.
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    return measurementsData.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    
    TMMeasurement *measurement = [measurementsData objectAtIndex:indexPath.row];
    
    if (measurement.name.length == 0) {
        cell.textLabel.text = [NSDateFormatter localizedStringFromDate:[NSDate dateWithTimeIntervalSince1970:measurement.timestamp]
                                                             dateStyle:NSDateFormatterShortStyle timeStyle:NSDateFormatterShortStyle];
    } else {
        cell.textLabel.text = measurement.name;
    }
   
    switch (measurement.type)
    {
        case TypeTotalPath:
            cell.detailTextLabel.text = [measurement getFormattedDistance:measurement.totalPath];
            break;
            
        case TypeHorizontal:
            cell.detailTextLabel.text = [measurement getFormattedDistance:measurement.horzDist];
            break;
            
        case TypeVertical:
            cell.detailTextLabel.text = [measurement getFormattedDistance:measurement.zDisp];
            break;
            
        default: //TypePointToPoint
            cell.detailTextLabel.text = [measurement getFormattedDistance:measurement.pointToPoint];
            break;
    }
    
    return cell;
}


// Override to support conditional editing of the table view.
//- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
//{
//    // Return NO if you do not want the specified item to be editable.
//    return YES;
//}


// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete)
    {
        [self deleteMeasurement:indexPath];
    }   
}

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
{
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [self performSegueWithIdentifier:@"toResult" sender:indexPath];
}

#pragma mark - Action sheet

- (IBAction)handleActionButton:(id)sender {
    [self showActionSheet];
}

- (void)showActionSheet
{
    NSString *firstButtonTitle;
    
    if ([USER_MANAGER getLoginState] != LoginStateNo && ![USER_MANAGER isUsingAnonAccount])
    {
        firstButtonTitle = @"Logout";
    }
    else
    {
        firstButtonTitle = @"Create Account or Login";
    }
    
    actionSheet = [[UIActionSheet alloc] initWithTitle:@"Menu"
                                              delegate:self
                                     cancelButtonTitle:@"Cancel"
                                destructiveButtonTitle:nil
                                     otherButtonTitles:firstButtonTitle, @"Tell a friend", @"Refresh List", @"About", nil];
    // Show the sheet
    [actionSheet showFromBarButtonItem:_actionButton animated:YES];
}

- (void)actionSheet:(UIActionSheet *)actionSheet didDismissWithButtonIndex:(NSInteger)buttonIndex
{
    NSLog(@"Button %d", buttonIndex);
    
    switch (buttonIndex)
    {
        case 0:
        {
            NSLog(@"Account button");
            if ([USER_MANAGER getLoginState] != LoginStateNo && ![USER_MANAGER isUsingAnonAccount])
            {
                [self logout];
            }
            else
            {
                [self performSegueWithIdentifier:@"toCreateAccount" sender:self];
            }
            break;
        }
        case 1:
        {
            NSLog(@"Share button");
            break;
        }
        case 2:
        {
            NSLog(@"Refresh button");
            [self syncWithServer];
            break;
        }
        case 3:
        {
            NSLog(@"About button");
            break;
        }
        default:
            break;
    }
}
@end
