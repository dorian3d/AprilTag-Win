//
//  RCCalibration2.m
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibration2.h"
#import "RCCalibration3.h"
#import "MBProgressHUD.h"

@implementation RCCalibration2
{
    BOOL isCalibrating;
    bool steadyDone;
    MBProgressHUD *progressView;
    NSDate* startTime;
    RCSensorFusion* sensorFusion;
}
@synthesize button, messageLabel, videoPreview;

- (void) viewDidLoad
{
    [super viewDidLoad];
	
    isCalibrating = NO;
    steadyDone = NO;
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleOrientation)
                                                 name:UIDeviceOrientationDidChangeNotification
                                               object:nil];
    
    sensorFusion = [RCSensorFusion sharedInstance];
    sensorFusion.delegate = self;
    
    [RCVideoPreview class]; // keeps this class from being optimized out by the complier, since it isn't referenced anywhere besides in the storyboard
    [self.sensorDelegate getVideoProvider].delegate = self.videoPreview;
    
    [self handleResume];
}

- (void) viewDidAppear:(BOOL)animated
{
    if ([self.calibrationDelegate respondsToSelector:@selector(calibrationScreenDidAppear:)])
        [self.calibrationDelegate calibrationScreenDidAppear: @"Calibration2"];
    [super viewDidAppear:animated];
    [videoPreview setVideoOrientation:AVCaptureVideoOrientationPortrait];
    [self handleOrientation];
}

- (NSUInteger) supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait;
}

- (void) handleOrientation
{
    // must be done on UI thread
    UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
    if (orientation == UIDeviceOrientationPortrait)
    {
        button.enabled = YES;
        [button setTitle:@"Tap here to begin calibration" forState:UIControlStateNormal];
    }
    else
    {
        button.enabled = NO;
        [button setTitle:@"Hold in portrait orientation" forState:UIControlStateNormal];
    }
}
- (void) handlePause
{
    [self stopCalibration];
    [self.sensorDelegate stopVideoSession];
}

- (void) handleResume
{
    [self.sensorDelegate startVideoSession];
}

- (IBAction) handleButton:(id)sender
{
    [self startCalibration];
}

- (void) gotoNextScreen
{
    RCCalibration3* cal3 = [self.storyboard instantiateViewControllerWithIdentifier:@"Calibration3"];
    cal3.calibrationDelegate = self.calibrationDelegate;
    cal3.sensorDelegate = self.sensorDelegate;
    [self presentViewController:cal3 animated:YES completion:nil];
}

- (void) sensorFusionDidUpdateData:(RCSensorFusionData*)data
{
    if (isCalibrating && steadyDone)
    {
        if (!startTime)
            [self startTimer];

        float progress = .5 - [startTime timeIntervalSinceNow] / 4.; // 2 seconds steady followed by 2 seconds of data

        if (progress < 1.)
        {
            [self updateProgressView:progress];
        }
        else
        {
            [self finishCalibration];
        }
    }
    if (data.sampleBuffer) [videoPreview displaySampleBuffer:data.sampleBuffer];
}

- (void) sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    if (isCalibrating && !steadyDone)
    {
        if (status.runState == RCSensorFusionRunStateRunning)
        {
            steadyDone = true;
        }
        else
        {
            [self updateProgressView:status.progress / 2.];
        }
    }
    if(status.errorCode != RCSensorFusionErrorCodeNone)
    {
        NSLog(@"SENSOR FUSION ERROR %li", (long)status.errorCode);
        startTime = nil;
        steadyDone = false;
        [sensorFusion stopSensorFusion];
        [sensorFusion startSensorFusionWithDevice:[self.sensorDelegate getVideoDevice]];
    }
}

- (void) startTimer
{
    startTime = [NSDate date];
}

- (void) startCalibration
{
    LOGME
    [button setTitle:@"Calibrating" forState:UIControlStateNormal];
    [messageLabel setText:@"Hold the device steady and make sure the camera isn't blocked"];
    [self showProgressViewWithTitle:@"Calibrating"];
    
    sensorFusion.delegate = self;
    [sensorFusion startSensorFusionWithDevice:[self.sensorDelegate getVideoDevice]];

    isCalibrating = YES;
    steadyDone = NO;
}

- (void) stopCalibration
{
    if (isCalibrating)
    {
        LOGME
        isCalibrating = NO;
        steadyDone = NO;
        [button setTitle:@"Tap here to begin calibration" forState:UIControlStateNormal];
        [messageLabel setText:@"Hold the iPad steady in portrait orientation. Make sure the camera lens isn't blocked. Step 2 of 3."];
        [self hideProgressView];
        startTime = nil;
        [sensorFusion stopSensorFusion];
    }
}

- (void) finishCalibration
{
    [self stopCalibration];
    [self gotoNextScreen];
}

- (void)showProgressViewWithTitle:(NSString*)title
{
    progressView = [[MBProgressHUD alloc] initWithView:self.view];
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
    [self.view addSubview:progressView];
    progressView.labelText = title;
    [progressView show:YES];
}

- (void)hideProgressView
{
    [progressView hide:YES];
}

- (void)updateProgressView:(float)progress
{
    [progressView setProgress:progress];
}


@end
