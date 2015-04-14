//
//  RC3DK.h
//  RC3DK
//
//  Created by Ben Hirashima on 7/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#ifndef RC3DK_RC3DK_h
#define RC3DK_RC3DK_h

#import <Foundation/Foundation.h>

// singletons
#import "RCSensorFusion.h"
#import "RCCaptureManager.h"
#import "RCReplayManager.h"

// data classes
#import "RCFeaturePoint.h"
#import "RCRotation.h"
#import "RCTranslation.h"
#import "RCSensorFusionData.h"
#import "RCSensorFusionStatus.h"
#import "RCTransformation.h"
#import "RCScalar.h"
#import "RCVector.h"
#import "RCPoint.h"
#import "RCCameraParameters.h"

// error classes
#import "RCLicenseError.h"
#import "RCSensorFusionError.h"

// misc
#import "RCDeviceInfo.h"

// sensor managers
#import "RCAVSessionManager.h"
#import "RCSensorManager.h"
#import "RCVideoManager.h"
#import "RCLocationManager.h"
#import "RCMotionManager.h"
#import "RCVideoFrameProvider.h"

#endif