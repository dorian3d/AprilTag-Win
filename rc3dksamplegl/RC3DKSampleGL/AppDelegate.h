//
//  AppDelegate.h
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RCCalibration1.h"
#import "LocationPermissionController.h"

@interface AppDelegate : UIResponder <UIApplicationDelegate, RCCalibrationDelegate, LocationPermissionControllerDelegate>

@property (strong, nonatomic) UIWindow *window;

@end
