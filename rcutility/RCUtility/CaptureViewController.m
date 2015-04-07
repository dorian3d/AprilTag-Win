//
//  ViewController.m
//  RCCapture
//
//  Created by Brian on 9/27/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <RCCore/RCCore.h>
#import "CaptureViewController.h"
#import "AppDelegate.h"

@interface CaptureViewController ()
{
    bool isStarted;
    AVCaptureVideoPreviewLayer * previewLayer;
    RCCaptureManager * captureController;
    NSURL * fileURL;
    int width;
    int height;
    int framerate;
}

@end

@implementation CaptureViewController

@synthesize previewView, frameRateSelector, resolutionSelector, startStopButton;

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.

	AVCaptureSession *session = [[RCAVSessionManager sharedInstance] session];

	// Make a preview layer so we can see the visual output of an AVCaptureSession
	previewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:session];
	[previewLayer setVideoGravity:AVLayerVideoGravityResizeAspect];

    // add the preview layer to the hierarchy
    CALayer *rootLayer = [previewView layer];
	[rootLayer setBackgroundColor:[[UIColor blackColor] CGColor]];
	[rootLayer addSublayer:previewLayer];

    captureController = [[RCCaptureManager alloc] init];

    isStarted = false;
    framerate = 30;
    width = 640;
    height = 480;

    [[RCAVSessionManager sharedInstance] startSession];
}

- (void) viewDidLayoutSubviews
{
    [self layoutPreviewInView:previewView];
}

-(void)layoutPreviewInView:(UIView *) aView
{
    AVCaptureVideoPreviewLayer *layer = previewLayer;
    if (!layer) return;

    UIDeviceOrientation orientation = [UIDevice currentDevice].orientation;
    CATransform3D transform = CATransform3DIdentity;
    if (orientation == UIDeviceOrientationPortrait) ;
    else if (orientation == UIDeviceOrientationLandscapeLeft)
        transform = CATransform3DMakeRotation(-M_PI_2, 0.0f, 0.0f, 1.0f);
    else if (orientation == UIDeviceOrientationLandscapeRight)
        transform = CATransform3DMakeRotation(M_PI_2, 0.0f, 0.0f, 1.0f);
    else if (orientation == UIDeviceOrientationPortraitUpsideDown)
        transform = CATransform3DMakeRotation(M_PI, 0.0f, 0.0f, 1.0f);

    previewLayer.transform = transform;
    previewLayer.frame = aView.bounds;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void) captureDidStart
{
    [startStopButton setTitle:@"Stop" forState:UIControlStateNormal];
    [frameRateSelector setHidden:true];
    [resolutionSelector setHidden:true];
}

- (void) presentRenameAlert
{
    NSString * path = [fileURL.path stringByDeletingLastPathComponent];
    NSString * filename = [fileURL.path lastPathComponent];
    NSString * basename = nil, *widthText, *heightText, *frameRateText;

    NSRegularExpression * filename_parse = [NSRegularExpression regularExpressionWithPattern:@"(.*)_(\\d+)_(\\d+)_(\\d+)Hz.capture" options:0 error:nil];
    NSArray * matches = [filename_parse matchesInString:filename options:0 range:NSMakeRange(0, filename.length)];
    for(NSTextCheckingResult* match in matches) {
        basename = [filename substringWithRange:[match rangeAtIndex:1]];
        widthText = [filename substringWithRange:[match rangeAtIndex:2]];
        heightText = [filename substringWithRange:[match rangeAtIndex:3]];
        frameRateText = [filename substringWithRange:[match rangeAtIndex:4]];
    }
    NSString * message = [NSString stringWithFormat:@"Add a length, path length, or rename\n%@x%@ @ %@Hz", widthText, heightText, frameRateText];

    UIAlertController * alert = [UIAlertController
                                 alertControllerWithTitle:@"Edit measurement"
                                 message:message
                                 preferredStyle:UIAlertControllerStyleAlert];

    UIAlertAction* save = [UIAlertAction actionWithTitle:@"Save" style:UIAlertActionStyleDefault
                                                 handler:^(UIAlertAction * action)
                           {
                               UITextField * nameTextField = alert.textFields[0];
                               UITextField * lengthTextField = alert.textFields[1];
                               UITextField * plTextField = alert.textFields[2];
                               NSString * newName = basename;
                               if(nameTextField.hasText)
                                   newName = nameTextField.text;
                               NSString * newFilename = [NSString stringWithFormat:@"%@_%@_%@_%@Hz.capture", newName, widthText, heightText, frameRateText];
                               if(lengthTextField.hasText)
                                   newFilename = [NSString stringWithFormat:@"%@_L%@", newFilename, lengthTextField.text];
                               if(plTextField.hasText)
                                   newFilename = [NSString stringWithFormat:@"%@_PL%@", newFilename, plTextField.text];
                               NSString * newPath = [NSString pathWithComponents:[NSArray arrayWithObjects:path, newFilename, nil]];
                               [[NSFileManager defaultManager] moveItemAtPath:fileURL.path toPath:newPath error:nil];
                               [alert dismissViewControllerAnimated:YES completion:nil];
                           }];
    UIAlertAction* delete = [UIAlertAction actionWithTitle:@"Delete" style:UIAlertActionStyleDestructive
                                                   handler:^(UIAlertAction * action)
                             {
                                 [[NSFileManager defaultManager] removeItemAtPath:fileURL.path error:nil];
                                 [alert dismissViewControllerAnimated:YES completion:nil];
                             }];

    [alert addTextFieldWithConfigurationHandler:^(UITextField *textField)
     {
         textField.placeholder = @"Filename";
         textField.text = basename;
     }];

    [alert addTextFieldWithConfigurationHandler:^(UITextField *textField)
     {
         textField.placeholder = @"Length in cm";
         textField.keyboardType = UIKeyboardTypeDecimalPad;
     }];

    [alert addTextFieldWithConfigurationHandler:^(UITextField *textField)
     {
         textField.placeholder = @"Path length in cm";
         textField.keyboardType = UIKeyboardTypeDecimalPad;
     }];

    [alert addAction:save];
    [alert addAction:delete];

    [self presentViewController:alert animated:YES completion:nil];
}

- (void) captureDidStop
{
    [self presentRenameAlert];
    [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
    [startStopButton setEnabled:true];
    [frameRateSelector setHidden:false];
    [resolutionSelector setHidden:false];
}

- (IBAction)cameraConfigureClick:(id)sender
{
    if(frameRateSelector.selectedSegmentIndex == 0)
        framerate = 30;
    else if(frameRateSelector.selectedSegmentIndex == 1)
        framerate = 60;
    if(resolutionSelector.selectedSegmentIndex == 0) {
        width = 640;
        height = 480;
    }
    else if(resolutionSelector.selectedSegmentIndex == 1) {
        width = 1280;
        height = 720;
    }
    else if(resolutionSelector.selectedSegmentIndex == 2) {
        width = 1920;
        height = 1080;
    }
    [[RCAVSessionManager sharedInstance] configureCameraWithFrameRate:framerate withWidth:width withHeight:height];
}

- (IBAction)startStopClicked:(id)sender
{
    if (!isStarted)
    {
        fileURL = [AppDelegate timeStampedURLWithSuffix:[NSString stringWithFormat:@"_%d_%d_%dHz.capture", width, height, framerate]];
        [startStopButton setTitle:@"Starting..." forState:UIControlStateNormal];
        [captureController startCaptureWithPath:fileURL.path withDelegate:self];
    }
    else
    {
        [startStopButton setEnabled:false];
        [startStopButton setTitle:@"Stopping..." forState:UIControlStateNormal];
        [captureController stopCapture];
    }
    isStarted = !isStarted;
}

@end
