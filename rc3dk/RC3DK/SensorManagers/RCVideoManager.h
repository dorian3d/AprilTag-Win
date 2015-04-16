//
//  RCVideoManager.h
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>
#import <CoreImage/CoreImage.h>
#include <stdio.h>
#import <ImageIO/ImageIO.h>
#import "RCAVSessionManager.h"
#import "RCVideoFrameProvider.h"
#import "RC3DK.h"

/** 
 Handles getting video frames from the AV session, and passes them directly to the RCSensorFusion shared instance.
 */
@interface RCVideoManager : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate, RCVideoFrameProvider>

- (void) setupWithSession:(AVCaptureSession*)avSession;
- (bool) startVideoCapture;
- (void) stopVideoCapture;
- (BOOL) isCapturing;
- (void) setDataDelegate:(id <RCSensorDataDelegate>)delegate;

#ifdef DEBUG
- (void) setupWithSession:(AVCaptureSession *)avSession withOutput:(AVCaptureVideoDataOutput *)avOutput;
#endif

@property id<RCVideoFrameDelegate> delegate;
@property (readonly) AVCaptureVideoOrientation videoOrientation;
@property (readonly) AVCaptureSession *session;
@property (readonly) AVCaptureVideoDataOutput *output;

+ (RCVideoManager *) sharedInstance;

@end
