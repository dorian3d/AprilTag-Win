//
//  MPCalibrationVCViewController.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPCalibrationVC.h"

@implementation MPCalibrationVC
{
    BOOL isCalibrating;
    MBProgressHUD *progressView;
}
@synthesize button, messageLabel, delegate;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
	isCalibrating = NO;
}

- (void) viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) handlePause
{
    if (isCalibrating) [self resetCalibration];
}

- (IBAction)handleButton:(id)sender
{
    if (!isCalibrating) [self startCalibration];
}

- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data
{
    if (isCalibrating)
    {
        if (data.status.calibrationProgress >= 1.)
        {
            [self stopCalibration];
            [delegate calibrationDidComplete];
        }
        else
        {
            [self updateProgress:data.status.calibrationProgress];
        }
    }
}

- (void) sensorFusionError:(RCSensorFusionError*)error
{
    DLog(@"ERROR %@", error.debugDescription);
}

- (void) startCalibration
{
    [self showProgressWithTitle:@"Calibrating"];
    SENSOR_FUSION.delegate = self;
    [SENSOR_FUSION startStaticCalibration];
    [button setTitle:@"Calibrating" forState:UIControlStateNormal];
    [messageLabel setText:@"Don't touch the device until calibration has finished"];
    isCalibrating = YES;
}

- (void) stopCalibration
{
    [SENSOR_FUSION stopStaticCalibration];
    SENSOR_FUSION.delegate = nil;
    [self resetCalibration];
}

- (void) resetCalibration
{
    [button setTitle:@"Begin Calibration" forState:UIControlStateNormal];
    [messageLabel setText:@"Your device needs to be calibrated just once. Place it on a flat, stable surface, like a table."];
    isCalibrating = NO;
    [self hideProgress];
}

- (void)showProgressWithTitle:(NSString*)title
{
    progressView = [[MBProgressHUD alloc] initWithView:self.view];
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
    [self.view addSubview:progressView];
    progressView.labelText = title;
    [progressView show:YES];
}

- (void)hideProgress
{
    [progressView hide:YES];
}

- (void)updateProgress:(float)progress
{
    [progressView setProgress:progress];
}

@end
