//
//  ReplayViewController.m
//  RCCapture
//
//  Created by Brian on 2/18/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import "ReplayViewController.h"
#import <RCCore/RCCore.h>

@interface ReplayViewController ()
{
    ReplayController * controller;
    BOOL isStarted;
    NSString * replayFilename;
    NSString * calibrationFilename;
}
@end

@implementation ReplayViewController

@synthesize startButton, progressBar, progressText;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void) startReplay
{
    NSLog(@"Start replay");
    [controller startReplay:replayFilename withCalibration:calibrationFilename withRealtime:FALSE];
    [startButton setTitle:@"Stop" forState:UIControlStateNormal];
    isStarted = TRUE;
}

- (void) stopReplay
{
    NSLog(@"Stop replay");
    [controller stopReplay];
    [startButton setTitle:@"Start" forState:UIControlStateNormal];
    isStarted = FALSE;
}

- (IBAction) startStopClicked:(id)sender
{
    if(isStarted)
        [self stopReplay];
    else
        [self startReplay];
}

- (void)setProgressPercentage:(float)progress
{
    float percent = progress*100.;
    [progressBar setProgress:progress];
    [progressText setText:[NSString stringWithFormat:@"%.0f%%", percent]];
}

- (void) replayProgress:(float)progress
{
    [self setProgressPercentage:progress];
}

- (void) replayFinished
{
    NSLog(@"Replay finished");
    [startButton setTitle:@"Start" forState:UIControlStateNormal];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.
    [self setProgressPercentage:0];
    controller = [[ReplayController alloc] init];
    controller.delegate = self;
    replayFilename = [ReplayController getFirstReplayFilename];
    calibrationFilename = [ReplayController getFirstCalibrationFilename];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
