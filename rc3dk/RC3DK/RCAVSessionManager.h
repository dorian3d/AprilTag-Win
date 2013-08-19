//
//  RCAVSessionManager.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCSensorFusion.h"
#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

/** Manages the AV session. You must call setupAVSession before sharedInstance or startSession.
 This class automatically handles an app pause and ends the AV session. It does not automatically resume the 
 AV session on an app resume.
 */

@interface RCAVSessionManager : NSObject

@property AVCaptureSession *session;
@property AVCaptureDevice *videoDevice;

- (BOOL)startSession;
- (void)endSession;
- (BOOL)isRunning;
- (BOOL)addOutput:(AVCaptureVideoDataOutput*)output;
- (void)lockFocus;
- (void) focusOnce;
- (void)unlockFocus;
- (bool)isImageClean;

+ (RCAVSessionManager*) sharedInstance;

@end


