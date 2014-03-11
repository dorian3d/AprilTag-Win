//
//  AppDelegate.m
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "AppDelegate.h"
#import <RC3DK/RC3DK.h>
#import "VisualizationController.h"

#define PREF_SHOW_LOCATION_EXPLANATION @"RC_SHOW_LOCATION_EXPLANATION"
#define PREF_IS_CALIBRATED @"PREF_IS_CALIBRATED"

@implementation AppDelegate
{
    UIViewController * mainView;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSNumber numberWithBool:YES], PREF_SHOW_LOCATION_EXPLANATION,
                                 [NSNumber numberWithBool:NO], PREF_IS_CALIBRATED,
                                 nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
    
    BOOL isCalibrated = [[NSUserDefaults standardUserDefaults] boolForKey:PREF_IS_CALIBRATED];
    BOOL hasStoredCalibrationData = [[RCSensorFusion sharedInstance] hasCalibrationData];
    if (!isCalibrated || !hasStoredCalibrationData)
    {
        // If not calibrated, show calibration screen
        mainView = self.window.rootViewController;
        UIViewController * vc = [CalibrationStep1 instantiateViewControllerWithDelegate:self];
        self.window.rootViewController = vc;
    }

    return YES;
}

- (void)calibrationDidFinish
{
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:PREF_IS_CALIBRATED];
    self.window.rootViewController = mainView;
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    LOGME
    [[NSUserDefaults standardUserDefaults] synchronize];

    [self startMotionOnlySensorFusion];

    if ([[LocationManager sharedInstance] isLocationAuthorized])
    {
        // location already authorized. go ahead.
        [self startMotionOnlySensorFusion];
        [LocationManager sharedInstance].delegate = self;
        [[LocationManager sharedInstance] startLocationUpdates];
    }
    else if([self shouldShowLocationExplanation])
    {
        // show explanation, then ask for authorization. if they authorize, then start updating location.
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Location"
                                                        message:@"If you allow the app to use your location, we can improve the accuracy of your measurements by adjusting for altitude and how far you are from the equator."
                                                       delegate:self
                                              cancelButtonTitle:@"Continue"
                                              otherButtonTitles:nil];
        [alert show];
    }
    else
    {
        // location denied. continue without it.
        [self startMotionOnlySensorFusion];
    }
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    LOGME
    if ([MotionManager sharedInstance].isCapturing) [[MotionManager sharedInstance] stopMotionCapture];
    if ([RCSensorFusion sharedInstance].isSensorFusionRunning) [[RCSensorFusion sharedInstance] stopSensorFusion];
}

- (BOOL)shouldShowLocationExplanation
{
    if ([CLLocationManager locationServicesEnabled])
    {
        return [CLLocationManager authorizationStatus] == kCLAuthorizationStatusNotDetermined;
    }
    else
    {
        return [[NSUserDefaults standardUserDefaults] boolForKey:PREF_SHOW_LOCATION_EXPLANATION];
    }
}

- (void) alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (buttonIndex == 0) //the only button
    {
        [[NSUserDefaults standardUserDefaults] setBool:NO forKey:PREF_SHOW_LOCATION_EXPLANATION];
        [[NSUserDefaults standardUserDefaults] synchronize];

        if([[LocationManager sharedInstance] shouldAttemptLocationAuthorization])
        {
            [LocationManager sharedInstance].delegate = self;
            [[LocationManager sharedInstance] startLocationUpdates]; // will show dialog asking user to authorize location
        }
    }
}

- (void) startMotionOnlySensorFusion
{
    LOGME
    [[RCSensorFusion sharedInstance] startInertialOnlyFusion];
    [[RCSensorFusion sharedInstance] setLocation:[[LocationManager sharedInstance] getStoredLocation]];
    [[MotionManager sharedInstance] startMotionCapture];
}

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    [LocationManager sharedInstance].delegate = nil;
    if(![[RCSensorFusion sharedInstance] isSensorFusionRunning]) [self startMotionOnlySensorFusion];
    [[RCSensorFusion sharedInstance] setLocation:[[LocationManager sharedInstance] getStoredLocation]];
}

@end
