//
//  TMResultsVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMResultsVC.h"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import "TMMeasurement.h"
#import "TMDistanceFormatter.h"
#import "TMOptionsVC.h"
#import "TMLocation.h"
#import "TMMapVC.h"

@interface TMResultsVC ()

@end

@implementation TMResultsVC

@synthesize theMeasurement, nameBox, theDate, locationLabel, pointToPoint, totalPath, horzDist, vertDist;

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:NO animated:animated];
    [super viewWillAppear:animated];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	
//    NSMutableArray *navigationArray = [[NSMutableArray alloc] initWithArray: self.navigationController.viewControllers];
//    
//    if(navigationArray.count == 3) //if this is third screen in the stack
//    {
//        [navigationArray removeObjectAtIndex: 1];  // remove New Measurement VC from nav array, so back button goes to history instead
//        self.navigationController.viewControllers = navigationArray;
//    }

    nameBox.delegate = self; //handle done button on keyboard
}

- (void)viewDidAppear:(BOOL)animated
{
    [self configureView];
}

- (void)configureView
{
    NSLog(@"configureView");
    
//    [theMeasurement.managedObjectContext refreshObject:theMeasurement mergeChanges:YES];
    
    TMLocation *location = (TMLocation*)theMeasurement.location;
    
    nameBox.text = theMeasurement.name;
    theDate.text = [[NSDateFormatter class] localizedStringFromDate:theMeasurement.timestamp dateStyle:NSDateFormatterShortStyle timeStyle:NSDateFormatterShortStyle];
    pointToPoint.text = [TMDistanceFormatter formattedDistance:theMeasurement.pointToPoint withMeasurement:theMeasurement];
    totalPath.text = [TMDistanceFormatter formattedDistance:theMeasurement.totalPath withMeasurement:theMeasurement];
    horzDist.text = [TMDistanceFormatter formattedDistance:theMeasurement.horzDist withMeasurement:theMeasurement];
    vertDist.text = [TMDistanceFormatter formattedDistance:theMeasurement.vertDist withMeasurement:theMeasurement];
    
    if(location.locationName.length > 0)
    {
        locationLabel.text = location.locationName;
    }
    else
    {
        locationLabel.text = location.address;
    }
}

- (void)viewDidUnload {
    [self setPointToPoint:nil];
    [self setTotalPath:nil];
    [self setHorzDist:nil];
    [self setVertDist:nil];
    [self setTheDate:nil];
    [self setNameBox:nil];
    [self setBtnDone:nil];
    [self setBtnAction:nil];
    [self setLocationLabel:nil];
    [super viewDidUnload];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    [textField resignFirstResponder];
    return NO;
}

- (IBAction)handleDeleteButton:(id)sender {
    NSError *error;
    
    [theMeasurement.managedObjectContext deleteObject:theMeasurement];
    [theMeasurement.managedObjectContext save:&error]; //TODO: Handle save error
    [self.navigationController popToRootViewControllerAnimated:YES];
}

- (IBAction)handleUpgradeButton:(id)sender {
}

- (IBAction)handleDoneButton:(id)sender {
    
    theMeasurement.name = nameBox.text;
    
    [self saveMeasurement];
    
    [self postMeasurement];
    
    [self.navigationController popToRootViewControllerAnimated:YES];
}

- (IBAction)handleKeyboardDone:(id)sender {
    //no need to do anything here
}

- (IBAction)handleActionButton:(id)sender {
    [self showActionSheet];
}

- (void)showActionSheet
{
    sheet = [[UIActionSheet alloc] initWithTitle:@"Share this measurement"
                                        delegate:self
                               cancelButtonTitle:@"Cancel"
                                destructiveButtonTitle:nil
                               otherButtonTitles:@"Facebook", @"Twitter", @"Email", @"Text Message", nil];
    // Show the sheet
    [sheet showFromToolbar:self.navigationController.toolbar];
}

- (void)actionSheet:(UIActionSheet *)actionSheet didDismissWithButtonIndex:(NSInteger)buttonIndex
{
    NSLog(@"Button %d", buttonIndex);
}

-(void)postMeasurement
{
    NSString *bodyData = [NSString stringWithFormat:@"measurement[user_id]=1&measurement[name]=%@&measurement[value]=%@&measurement[location_id]=3", [self urlEncodeString:self.theMeasurement.name], self.theMeasurement.pointToPoint];
    
    NSMutableURLRequest *postRequest = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:@"http://192.168.1.1:3000/measurements.json"]];
    
    [postRequest setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    [postRequest setHTTPMethod:@"POST"];
    [postRequest setHTTPBody:[NSData dataWithBytes:[bodyData UTF8String] length:[bodyData length]]];
    
    NSURLConnection *theConnection=[[NSURLConnection alloc] initWithRequest:postRequest delegate:self];
    
    if (theConnection)
    {
        NSLog(@"Connection ok");
        //        receivedData = [[NSMutableData data] retain];
    }
    else
    {
        NSLog(@"Connection failed");
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    
    // This method is called when the server has determined that it
    // has enough information to create the NSURLResponse.
    
    // It can be called multiple times, for example in the case of a
    // redirect, so each time we reset the data.
    
    // receivedData is an instance variable declared elsewhere.
    
    //    [receivedData setLength:0];
    
    NSLog(@"didReceiveResponse");
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data

{
    // Append the new data to receivedData.
    // receivedData is an instance variable declared elsewhere.
    
    //    [receivedData appendData:data];
    
    NSLog(@"didReceiveData");
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    NSLog(@"Connection failed! Error - %@ %@",
          [error localizedDescription],
          [[error userInfo] objectForKey:NSURLErrorFailingURLStringErrorKey]);
}

-(NSString*)urlEncodeString:(NSString*)input
{
    NSString *urlEncoded = (__bridge_transfer NSString *)CFURLCreateStringByAddingPercentEscapes(
                                                                                                 NULL,
                                                                                                 (__bridge CFStringRef)input,
                                                                                                 NULL,
                                                                                                 (CFStringRef)@"!*'\"();:@&=+$,/?%#[]% ",
                                                                                                 kCFStringEncodingUTF8);
    return urlEncoded;
}

- (void) prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if([[segue identifier] isEqualToString:@"toOptions"])
    {
        TMOptionsVC *optionsVC = [segue destinationViewController];
        optionsVC.theMeasurement = theMeasurement;
        
        [[segue destinationViewController] setDelegate:self];
    }
    else if([[segue identifier] isEqualToString:@"toMap"])
    {
        TMMapVC *mapVC = [segue destinationViewController];
        mapVC.location = (TMLocation*)theMeasurement.location;
        
//        [[segue destinationViewController] setDelegate:self];
    }
}

- (void)didDismissOptions
{
    [self configureView];
}

- (void)saveMeasurement
{
    NSError *error;
    [theMeasurement.managedObjectContext save:&error]; //TODO: Handle save error
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath
{
     NSLog(@"selected: %@", indexPath);
    [self performSegueWithIdentifier:@"toMap" sender:self];
}

@end
