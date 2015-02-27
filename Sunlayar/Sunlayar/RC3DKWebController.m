//
//  RC3DKWebController.m
//  Sunlayar
//
//  Created by Ben Hirashima on 2/20/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#import "RC3DKWebController.h"
#import "RCSensorDelegate.h"
#import "RCDebugLog.h"
#import "ARNativeAction.h"
#import "RCLocationManager.h"
#import "SLConstants.h"
#import "NSObject+SBJson.h"

@implementation RC3DKWebController
{
    id<RCSensorDelegate> sensorDelegate;
    BOOL useLocation;
}
@synthesize isSensorFusionRunning;

- (instancetype)init
{
    self = [super init];
    if (!self) return nil;
    
    _isVideoViewShowing = NO;
    isSensorFusionRunning = NO;
    _webView = [UIWebView new];
    
    return self;
}

- (void) viewDidLoad
{
    [super viewDidLoad];
    
    [RCHttpInterceptor setDelegate:self];
    
    useLocation = [LOCATION_MANAGER isLocationExplicitlyAllowed] && [NSUserDefaults.standardUserDefaults boolForKey:PREF_USE_LOCATION];
    
    sensorDelegate = [SensorDelegate sharedInstance];
    
    _isVideoViewShowing ? [self showVideoView] : [self hideVideoView];
    
    // setup web view
    self.webView.scalesPageToFit = NO;
    self.webView.delegate = self;
    self.webView.opaque = NO;
    self.webView.backgroundColor = [UIColor clearColor];
    [self.view addSubview:self.webView];
    [self.view bringSubviewToFront:self.webView];
    [self.webView loadRequest:[NSURLRequest requestWithURL:pageURL]];
}

-(void)viewDidLayoutSubviews
{
    self.videoView.frame = self.view.frame;
    self.webView.frame = self.view.frame;
}

- (void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    //register to receive notifications of pause/resume events
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    
    [self handleResume];
}

- (void) viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - App lifecycle

- (void)handlePause
{
    [self stopSensorFusion];
    [self stopSensors];
}

- (void)handleResume
{
    if (useLocation) [LOCATION_MANAGER startLocationUpdates];
}

#pragma mark - Video view

- (void) setIsVideoViewShowing:(BOOL)newValue
{
    if (newValue != _isVideoViewShowing) newValue ? [self showVideoView] : [self hideVideoView];
}

- (void) showVideoView
{
    if (!self.videoView.superview)
    {
        if (!self.videoView) _videoView = [[RCVideoPreview alloc] initWithFrame:self.view.frame];
        self.videoView.orientation = UIInterfaceOrientationLandscapeRight; // TODO handle other orientations
        [self.view insertSubview:self.videoView belowSubview:self.webView];
        
        if (!isSensorFusionRunning) [[sensorDelegate getVideoProvider] setDelegate:self.videoView];
        
        _isVideoViewShowing = YES;
    }
}

- (void) hideVideoView
{
    if (self.videoView.superview)
    {
        [[sensorDelegate getVideoProvider] setDelegate:nil];
        [self.videoView removeFromSuperview];
        _videoView = nil;
        _isVideoViewShowing = NO;
    }
}

#pragma mark - 3DK Stuff

- (void) startSensors
{
    LOGME
    [sensorDelegate startAllSensors];
}

- (void)stopSensors
{
    LOGME
    [sensorDelegate stopAllSensors];
}

- (void)startSensorFusion
{
    LOGME
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    SENSOR_FUSION.delegate = self;
    [[sensorDelegate getVideoProvider] setDelegate:nil];
    if (useLocation) [SENSOR_FUSION setLocation:[LOCATION_MANAGER getStoredLocation]];
    [SENSOR_FUSION startSensorFusionWithDevice:[sensorDelegate getVideoDevice]];
    isSensorFusionRunning = YES;
}

- (void)stopSensorFusion
{
    LOGME
    [SENSOR_FUSION stopSensorFusion];
    [[sensorDelegate getVideoProvider] setDelegate:self.videoView];
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
    isSensorFusionRunning = NO;
}

#pragma mark - RCSensorFusionDelegate

- (void) sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    NSString* statusJson = [[status dictionaryRepresentation] JavascriptObjRepresentation]; // expensive
    NSString* javascript = [NSString stringWithFormat:@"RC3DK.sensorFusionDidChangeStatus(%@);", statusJson];
//    DLog(@"%@", javascript);
    [self.webView stringByEvaluatingJavaScriptFromString: javascript];
}

- (void) sensorFusionDidUpdateData:(RCSensorFusionData*)data
{
    [self sendDataToWebView:data];
    
    if(data.sampleBuffer) [self.videoView displaySensorFusionData:data];
}

- (void) sendDataToWebView:(RCSensorFusionData*)data
{
    NSNumber* medianFeatureDepth = [self calculateMedianFeatureDepth:data.featurePoints];
    
    NSString* dataJson = [[data dictionaryRepresentation] JavascriptObjRepresentation]; // expensive
    NSString* javascript = [NSString stringWithFormat:@"RC3DK.sensorFusionDidUpdateData(%@, %f);", dataJson, medianFeatureDepth.floatValue];
//    DLog(@"%@", javascript);
    
    [self.webView stringByEvaluatingJavaScriptFromString: javascript];
}

- (NSNumber*) calculateMedianFeatureDepth:(NSArray*)featurePoints
{
    if (!featurePoints || featurePoints.count == 0) return 0;
    
    NSMutableArray *depths = [[NSMutableArray alloc] init];
    
    for(RCFeaturePoint *feature in featurePoints)
    {
        [depths addObject:[NSNumber numberWithFloat:feature.originalDepth.scalar]];
    }
    
    NSNumber *medianFeatureDepth;
    if([depths count])
    {
        NSArray *sorted = [depths sortedArrayUsingSelector:@selector(compare:)];
        long middle = [sorted count] / 2;
        medianFeatureDepth = [sorted objectAtIndex:middle];
    }
    else
        medianFeatureDepth = [NSNumber numberWithFloat:2.];
    
    return medianFeatureDepth;
}

#pragma mark - UIWebViewDelegate

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
    // report the error inside the webview
    NSString* errorString = [NSString stringWithFormat:
                             @"<html><center><font size=+5 color='red'>An error occurred:<br>%@</font></center></html>",
                             error.localizedDescription];
    [self.webView loadHTMLString:errorString baseURL:nil];
}

// called when user taps a link on the page
- (BOOL) webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    if ([request.URL.scheme isEqualToString:@"file"])
    {
        return YES; // allow loading local files
    }
    else return NO; // disallow loading of http and all other types of links
}

#pragma mark - RCHttpInterceptorDelegate

- (NSDictionary *)handleAction:(ARNativeAction *)nativeAction error:(NSError **)error
{
    DLog(@"%@", nativeAction.request.URL.relativePath);
    
    if ([nativeAction.request.URL.relativePath isEqualToString:@"/startSensors"])
    {
        [self startSensors];
        return @{ @"result": @YES };
    }
    else if ([nativeAction.request.URL.relativePath isEqualToString:@"/stopSensors"])
    {
        [self stopSensors];
        return @{ @"result": @YES };
    }
    else if ([nativeAction.request.URL.relativePath isEqualToString:@"/startSensorFusion"])
    {
        [self startSensorFusion];
        return @{ @"result": @YES };
    }
    else if ([nativeAction.request.URL.relativePath isEqualToString:@"/stopSensorFusion"])
    {
        [self stopSensorFusion];
        return @{ @"result": @YES };
    }
    else if ([nativeAction.request.URL.relativePath isEqualToString:@"/setLicenseKey"] && [nativeAction.method isEqualToString:@"POST"])
    {
        [SENSOR_FUSION setLicenseKey:[nativeAction.params objectForKey:@"licenseKey"]];
        return @{ @"result": @YES };
    }
    else if ([nativeAction.request.URL.relativePath isEqualToString:@"/hasCalibrationData"])
    {
        return @{ @"result": @([SENSOR_FUSION hasCalibrationData]) };
    }
    else if ([nativeAction.request.URL.relativePath isEqualToString:@"/startStaticCalibration"])
    {
        [SENSOR_FUSION startStaticCalibration];
        return @{ @"result": @YES };
    }
    else if ([nativeAction.request.URL.relativePath isEqualToString:@"/log"] && [nativeAction.method isEqualToString:@"POST"])
    {
        [self webViewLog:[nativeAction.params objectForKey:@"message"]];
        return @{ @"result": @YES };
    }
    else if ([nativeAction.request.URL.relativePath isEqualToString:@"/showVideoView"])
    {
        [self showVideoView];
        return @{ @"result": @YES };
    }
    else if ([nativeAction.request.URL.relativePath isEqualToString:@"/hideVideoView"])
    {
        [self hideVideoView];
        return @{ @"result": @YES };
    }
    
    return nil;
}

- (void) webViewLog:(NSString*)message
{
    if (message && message.length > 0) DLog(@"%@", message);
}


@end
