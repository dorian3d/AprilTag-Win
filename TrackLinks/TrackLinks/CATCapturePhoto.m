
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "CATConstants.h"
#import "CATCapturePhoto.h"
#import "math.h"
#import "UIImage+RCImageFile.h"
#import "MBProgressHUD.h"
#import "CATEditPhoto.h"
#import "RC3DK.h"
#import "RCSensorDelegate.h"
#import "RCDebugLog.h"
#import <CoreLocation/CoreLocation.h>
#import "RCLocationManager.h"

static UIDeviceOrientation currentUIOrientation = UIDeviceOrientationLandscapeLeft;

@implementation CATCapturePhoto
{
    BOOL useLocation;
    double lastTransitionTime;
    int filterStatusCode;
    BOOL isAligned;
    RCTransformation *initialCamera;
    
    MBProgressHUD *progressView;
    RCSensorFusionData* lastSensorFusionDataWithImage;

    id<RCSensorDelegate> sensorDelegate;
    
    CATMeasuredPhoto* measuredPhoto;
    
    BOOL didGetVisionError;
    RCSensorFusionErrorCode lastErrorCode;
}
@synthesize videoView;

#pragma mark - State Machine

typedef NS_ENUM(int, SpinnerType) {
    SpinnerTypeNone,
    SpinnerTypeDeterminate
};

typedef NS_ENUM(int, MessageColor) {
    ColorGray,
    ColorRed
};

enum state { ST_STARTUP, ST_READY, ST_INITIALIZING, ST_MOVING, ST_CAPTURE, ST_PROCESSING, ST_ERROR, ST_DISK_SPACE, ST_FINISHED, ST_ANY } currentState;
enum event { EV_RESUME, EV_FIRSTTIME, EV_VISIONFAIL, EV_FASTFAIL, EV_FAIL, EV_SHUTTER_TAP, EV_PAUSE, EV_CANCEL, EV_MOVE_DONE, EV_MOVE_UNDONE, EV_PROCESSING_FINISHED, EV_INITIALIZED, EV_STEREOFAIL, EV_DISK_SPACE };

typedef struct { enum state state; enum event event; enum state newstate; } transition;

typedef struct
{
    enum state state;
    bool sensorCapture;
    bool sensorFusion;
    bool showInstructions;
    SpinnerType progress;
    bool stereo;
    MessageColor messageColor;
    const char *message;
} statesetup;

static statesetup setups[] =
{
    //                  sensors fusion   instrct prgrs                     stereo  messageColor  message
    { ST_STARTUP,       false,  false,   false,  SpinnerTypeNone,          false,  ColorGray,    "Loading" },
    { ST_READY,         true,   false,   false,  SpinnerTypeNone,          false,  ColorGray,    "Point the camera at the track link, then press the button." },
    { ST_INITIALIZING,  true,   true,    false,  SpinnerTypeDeterminate,   false,  ColorGray,    "Hold still" },
    { ST_MOVING,        true,   true,    true,   SpinnerTypeNone,          true,   ColorGray,    "Move sideways left or right until the progress bar is full." },
    { ST_CAPTURE,       true,   true,    true,   SpinnerTypeNone,          true,   ColorGray,    "Press the button to finish." },
    { ST_PROCESSING,    false,  false,   false,  SpinnerTypeDeterminate,   false,  ColorGray,    "Please wait" },
    { ST_ERROR,         true,   false,   false,  SpinnerTypeNone,          false,  ColorRed,     "Whoops, something went wrong. Try again." },
    { ST_DISK_SPACE,    true,   false,   false,  SpinnerTypeNone,          false,  ColorRed,     "Your device is low on storage space. Free up some space first." },
    { ST_FINISHED,      false,  false,   false,  SpinnerTypeNone,          false,  ColorGray,    "" }
};

static transition transitions[] =
{
    { ST_STARTUP, EV_RESUME, ST_READY },
    { ST_READY, EV_SHUTTER_TAP, ST_INITIALIZING },
    { ST_INITIALIZING, EV_INITIALIZED, ST_MOVING },
    { ST_MOVING, EV_MOVE_DONE, ST_CAPTURE },
    { ST_MOVING, EV_FAIL, ST_ERROR },
    { ST_MOVING, EV_FASTFAIL, ST_ERROR },
    { ST_CAPTURE, EV_SHUTTER_TAP, ST_PROCESSING },
    { ST_CAPTURE, EV_MOVE_UNDONE, ST_MOVING },
    { ST_CAPTURE, EV_FAIL, ST_ERROR },
    { ST_CAPTURE, EV_FASTFAIL, ST_ERROR },
    { ST_PROCESSING, EV_PROCESSING_FINISHED, ST_FINISHED },
    { ST_PROCESSING, EV_STEREOFAIL, ST_ERROR },
    { ST_ERROR, EV_SHUTTER_TAP, ST_READY },
    { ST_FINISHED, EV_SHUTTER_TAP, ST_READY },
    { ST_FINISHED, EV_PAUSE, ST_FINISHED },
    { ST_ANY, EV_PAUSE, ST_STARTUP },
    { ST_ANY, EV_CANCEL, ST_STARTUP },
    { ST_ANY, EV_DISK_SPACE, ST_DISK_SPACE }
};

#define TRANS_COUNT (sizeof(transitions) / sizeof(*transitions))

- (void) validateStateMachine
{
    for(int i = 0; i < ST_ANY; ++i) {
        assert(setups[i].state == i);
    }
}

- (void) transitionToState:(int)newState
{
    if(currentState == newState) return;
    statesetup oldSetup = setups[currentState];
    statesetup newSetup = setups[newState];

    if (![self handleStateTransition:newState]) return;

    if(!oldSetup.sensorCapture && newSetup.sensorCapture)
        [self startSensors];
    if(!oldSetup.sensorFusion && newSetup.sensorFusion)
        [self startSensorFusion];
    if(oldSetup.sensorCapture && !newSetup.sensorCapture)
        [self stopSensors];
    if(oldSetup.sensorFusion && !newSetup.sensorFusion)
        [self stopSensorFusion];
    if(oldSetup.progress != newSetup.progress)
    {
        if (newSetup.progress == SpinnerTypeDeterminate)
            [self showProgressWithTitle:@(newSetup.message)];
        else
            [self hideProgress];
    }
    if(!oldSetup.stereo && newSetup.stereo)
        [[RCStereo sharedInstance] reset];
    if (newSetup.progress == SpinnerTypeNone)
    {
        NSString* message = @(newSetup.message);
        [self showMessage:message withColor:newSetup.messageColor];
    }
    else
    {
        [self hideMessage];
    }
    if(oldSetup.showInstructions && !newSetup.showInstructions)
        self.progressBar.hidden = YES;
    if(!oldSetup.showInstructions && newSetup.showInstructions)
        self.progressBar.hidden = NO;

    lastTransitionTime = CACurrentMediaTime();
    currentState = newState;
}

/**
 @returns NO if transition should be canceled
 */
- (BOOL) handleStateTransition:(int)newState
{
    if (newState == ST_READY)
    {
        lastErrorCode = RCSensorFusionErrorCodeNone;
        didGetVisionError = NO;
    }
    else if(newState == ST_PROCESSING)
    {
        if (IS_DISK_SPACE_LOW)
        {
            [self handleStateEvent:EV_DISK_SPACE];
            return NO;
        }
        else
        {
            [self handleCaptureFinished];
        }
    }
    
    return YES;
}

- (void)handleStateEvent:(int)event
{
    int newState = currentState;
    for(int i = 0; i < TRANS_COUNT; ++i) {
        if(transitions[i].state == currentState || transitions[i].state == ST_ANY) {
            if(transitions[i].event == event) {
                newState = transitions[i].newstate;
                break;
            }
        }
    }
    if(newState != currentState) [self transitionToState:newState];
}

#pragma mark - View Controller

- (BOOL) prefersStatusBarHidden { return YES; }

- (void)viewDidLoad
{
    LOGME
	[super viewDidLoad];
    
    sensorDelegate = [SensorDelegate sharedInstance];
    
    [self validateStateMachine];
    
    useLocation = [LOCATION_MANAGER isLocationExplicitlyAllowed] && [NSUserDefaults.standardUserDefaults boolForKey:PREF_USE_LOCATION];
    
    [[sensorDelegate getVideoProvider] setDelegate:self.videoView];
    
    progressView = [[MBProgressHUD alloc] initWithView:self.uiContainer];
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
    [self.uiContainer addSubview:progressView];
    
    self.videoView.orientation = UIInterfaceOrientationLandscapeRight;
}

- (void) viewDidAppear:(BOOL)animated
{
    LOGME
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
    LOGME
    [super viewWillDisappear:animated];
    [self handleStateEvent:EV_CANCEL];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (NSUInteger)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscapeRight;
}

#pragma mark - App lifecycle

- (void)handlePause
{
	LOGME
    if(currentState != ST_PROCESSING)
        [self handleStateEvent:EV_PAUSE];
}

- (void)handleResume
{
    LOGME
    
    if (useLocation) [LOCATION_MANAGER startLocationUpdates];
    
    if(currentState != ST_PROCESSING)
    {
        if ([RCDeviceInfo getFreeDiskSpaceInBytes] < 5000000)
        	[self handleStateEvent:EV_DISK_SPACE];
    	else
        	[self handleStateEvent:EV_RESUME];
    }
}

#pragma mark - Event handlers

- (IBAction)handleShutterButton:(id)sender
{
    [self handleStateEvent:EV_SHUTTER_TAP];
}

- (void) handleMoveStart
{
}

- (void) handleCaptureFinished
{
    measuredPhoto = [self saveMeasuredPhoto];

    RCStereo * stereo = [RCStereo sharedInstance];
    stereo.delegate = self;
    [stereo setWorkingDirectory:WORKING_DIRECTORY_URL andGuid:measuredPhoto.id_guid andOrientation:currentUIOrientation];
    [stereo processFrame:lastSensorFusionDataWithImage withFinal:true];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        [measuredPhoto writeImagetoJpeg:lastSensorFusionDataWithImage.sampleBuffer withOrientation:stereo.orientation];
        [stereo preprocess];
    });
}

- (void) gotoEditPhotoScreen
{
    CATEditPhoto* editPhotoController = (CATEditPhoto*)[self.storyboard instantiateViewControllerWithIdentifier:@"EditPhoto"];
    editPhotoController.measuredPhoto = measuredPhoto;
    [self presentViewController:editPhotoController animated:YES completion:nil];
}

- (CATMeasuredPhoto*) saveMeasuredPhoto
{
    return [CATMeasuredPhoto new];
}

#pragma mark - RCStereoDelegate

- (void) stereoDidUpdateProgress:(float)progress
{
    [progressView setProgress:progress];
}

- (void) stereoDidFinish
{
    [self handleStateEvent:EV_PROCESSING_FINISHED];
    [self gotoEditPhotoScreen];
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
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    SENSOR_FUSION.delegate = self;
    [[sensorDelegate getVideoProvider] setDelegate:nil];
    if (useLocation) [SENSOR_FUSION setLocation:[LOCATION_MANAGER getStoredLocation]];
    [SENSOR_FUSION startSensorFusionWithDevice:[sensorDelegate getVideoDevice]];
}

- (void)stopSensorFusion
{
    [SENSOR_FUSION stopSensorFusion];
    [[sensorDelegate getVideoProvider] setDelegate:self.videoView];
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
}

#pragma mark - RCSensorFusionDelegate

- (void) sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    if (status.error) DLog(@"Sensor fusion error: %@", status.error);
    
    if ([status.error isKindOfClass:[RCSensorFusionError class]])
    {
        lastErrorCode = (RCSensorFusionErrorCode)status.error.code;
        
        if(status.error.code == RCSensorFusionErrorCodeTooFast) {
            [self handleStateEvent:EV_FASTFAIL];
        } else if(status.error.code == RCSensorFusionErrorCodeOther) {
            [self handleStateEvent:EV_FAIL];
        } else if(status.error.code == RCSensorFusionErrorCodeVision) {
            didGetVisionError = YES;
            [self handleStateEvent:EV_VISIONFAIL];
        }
    }
    
    [self updateProgress:status.progress];
    
    if(currentState == ST_INITIALIZING && status.runState == RCSensorFusionRunStateRunning)
    {
        [self handleStateEvent:EV_INITIALIZED];
    }
}

- (void) sensorFusionDidUpdateData:(RCSensorFusionData*)data
{
    if(currentState == ST_INITIALIZING) initialCamera = data.cameraTransformation;
    NSMutableArray *depths = [[NSMutableArray alloc] init];

    for(RCFeaturePoint *feature in data.featurePoints)
    {
        [depths addObject:[NSNumber numberWithFloat:feature.originalDepth.scalar]];
    }
    
    NSNumber *median;
    if([depths count]) {
        NSArray *sorted = [depths sortedArrayUsingSelector:@selector(compare:)];
        long middle = [sorted count] / 2;
        median = [sorted objectAtIndex:middle];
    } else median = [NSNumber numberWithFloat:2.];
    
    if (currentState == ST_MOVING || currentState == ST_CAPTURE)
    {
        //Compute the capture progress here
        float depth = [median floatValue];
        
        RCPoint *projectedpt = [[initialCamera.rotation getInverse] transformPoint:[data.transformation.translation transformPoint:[[RCPoint alloc] initWithX:0. withY:0. withZ:0.]]];
        
        float targetDist = log(depth / 8. + 1.) + .05; // require movement of at least 5cm
        
        float dx = projectedpt.x / targetDist;
        if(dx > 1.) dx = 1.;
        if(dx < -1.) dx = -1.;

        float progress = fabs(dx);
        self.progressBar.progress = progress;
        
        if(currentState == ST_MOVING && progress >= 1.) [self handleStateEvent:EV_MOVE_DONE];
        if(currentState == ST_CAPTURE && progress < 1.) [self handleStateEvent:EV_MOVE_UNDONE];
    }
    
    if(data.sampleBuffer)
    {
        lastSensorFusionDataWithImage = data;
        
        [self.videoView displaySensorFusionData:data];
        
        if(setups[currentState].stereo) [STEREO processFrame:data withFinal:false];
    }
}

#pragma mark - Misc

/** delegate method of MPInstructionsViewDelegate. tells us when the dot has reached the edge of the circle. */
- (void) moveComplete
{
    [self handleStateEvent:EV_MOVE_DONE];
}

- (void)showProgressWithTitle:(NSString*)title
{
    [progressView setProgress:0];
    progressView.labelText = title;
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
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

- (void)showMessage:(NSString*)message withColor:(MessageColor)color
{
    if (message && message.length > 0)
    {
        self.messageLabel.hidden = NO;
        self.messageLabel.alpha = 1;
        self.messageLabel.text = message ? message : @"";
        
        if (color == ColorRed)
            self.messageLabel.backgroundColor = [UIColor colorWithRed:1 green:0 blue:0 alpha:.8];
        else
            self.messageLabel.backgroundColor = [UIColor colorWithRed:0 green:0 blue:0 alpha:.3];
    }
    else
    {
        [self hideMessage];
    }
}

- (void)hideMessage
{
    self.messageLabel.hidden = YES;
}

@end
