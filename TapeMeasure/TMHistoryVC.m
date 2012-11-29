//
//  TMMeasurementsList.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMHistoryVC.h"
#import "TMMeasurement.h"
#import "TMAppDelegate.h"
#import "TMResultsVC.h"

@interface TMHistoryVC ()

@end

@implementation TMHistoryVC

@synthesize measurementsData;

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)refreshPrefs
{
    unitsPref = [[NSUserDefaults standardUserDefaults] integerForKey:@"Units"];
    fractionalPref = [[NSUserDefaults standardUserDefaults] integerForKey:@"Fractional"];
}

- (void)viewDidLoad
{
    
//    NSLog(@"viewDidLoad");
    [super viewDidLoad];
    
    //register to receive notifications of pause/resume events
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];

    
    [self refreshPrefs];
    
//    [self loadTableData];
}

- (void)viewDidAppear:(BOOL)animated
{
    NSLog(@"viewDidAppear");
    [self loadTableData];
    [self.tableView reloadData];
}

- (void)loadTableData
{
    NSLog(@"loadTableData");
    
    TMAppDelegate* appDel = [[UIApplication sharedApplication] delegate];
    _managedObjectContext = [appDel managedObjectContext];
    
    NSFetchRequest *fetchRequest = [[NSFetchRequest alloc] init];
    NSEntityDescription *entity = [NSEntityDescription entityForName:@"TMMeasurement" inManagedObjectContext:_managedObjectContext];
    
    NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc]
                                        initWithKey:@"timestamp"
                                        ascending:NO];
    
    NSArray *descriptors = [[NSArray alloc] initWithObjects:sortDescriptor, nil];
    
    [fetchRequest setSortDescriptors:descriptors];
    [fetchRequest setEntity:entity];
    
    NSError *error;
    self.measurementsData = [_managedObjectContext executeFetchRequest:fetchRequest error:&error]; //TODO: Handle fetch error
    
    if(error)
    {
        NSLog(@"Error loading table data: %@", [error localizedDescription]);
    }
}

- (void)handleResume
{
//    [self loadTableData];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
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
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier forIndexPath:indexPath];
    
    TMMeasurement *measurement = [measurementsData objectAtIndex:indexPath.row];
    
    if ([measurement.name isEqualToString:@"Untitled"] || measurement.name.length == 0) {
        cell.textLabel.text = [measurement.name stringByAppendingFormat:@" (%@)", [[NSDateFormatter class] localizedStringFromDate:measurement.timestamp dateStyle:NSDateFormatterShortStyle timeStyle:NSDateFormatterShortStyle]];
    } else {
        cell.textLabel.text = measurement.name;
    }
    
    NSString *unitsSymbol;
    
    if(unitsPref == UNITS_PREF_METRIC)
    {
        unitsSymbol = @"m";
    }
    else
    {
        unitsSymbol = @"\"";
    }
    
    cell.detailTextLabel.text = [NSString localizedStringWithFormat:@"%0.1f%@", measurement.pointToPoint.floatValue, unitsSymbol]; 
    
    return cell;
}

/*
// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    return YES;
}
*/

/*
// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    }   
    else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }   
}
*/

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

- (void) prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([[segue identifier] isEqualToString:@"toResult"])
    {
        NSIndexPath *indexPath = (NSIndexPath*)sender;
        TMMeasurement *measurement = [measurementsData objectAtIndex:indexPath.row];
        
        TMResultsVC *resultsVC = [segue destinationViewController];
        resultsVC.theMeasurement = measurement;
    }
    else if([[segue identifier] isEqualToString:@"toOptions"])
    {
        [[segue destinationViewController] setDelegate:self];
    }
}

- (void)didDismissModalView
{
    NSLog(@"didDismissModalView");
    [self refreshPrefs];
    [self.tableView reloadData];
}

- (void)viewDidUnload {
    [self setMeasurementName:nil];
    [self setMeasurementValue:nil];
    [super viewDidUnload];
}

- (IBAction)handleDeleteButton:(id)sender
{
//    NSArray *deleteIndexPaths = [NSArray arrayWithObjects:
//                                 [NSIndexPath indexPathForRow:1 inSection:0],
//                                 nil];
//
//    UITableView *tv = (UITableView *)self.view;
//    
//    [tv beginUpdates];
//    [tv deleteRowsAtIndexPaths:deleteIndexPaths withRowAnimation:UITableViewRowAnimationFade];
//    [tv endUpdates];
//    
//    [self.tableView reloadData];
}
@end
