//
//  UIView+RCOrientationRotation.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "UIView+RCOrientationRotation.h"
#import "RCRotatingView.h"

@implementation UIView (RCOrientationRotation)

- (void) rotateChildViews:(UIDeviceOrientation)orientation animated:(BOOL)animated
{
    for (id<RCRotatingView> subView in self.subviews)
    {
        if ([subView respondsToSelector:@selector(handleOrientationChange:animated:)])
        {
            [subView handleOrientationChange:orientation animated:animated];
        }
    }
}

- (void) applyRotationTransformationAnimated:(UIDeviceOrientation)deviceOrientation
{
    [UIView animateWithDuration: .5
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         [self applyRotationTransformation:deviceOrientation];
                     }
                     completion:nil];
}


- (void) applyRotationTransformation:(UIDeviceOrientation)deviceOrientation animated:(BOOL)animated
{
    if (animated)
    {
        [self applyRotationTransformationAnimated:deviceOrientation];
    }
    else
    {
        [self applyRotationTransformation:deviceOrientation];
    }
}

- (void) applyRotationTransformation:(UIDeviceOrientation)deviceOrientation
{
    NSNumber* radians = [self getRotationInRadians:deviceOrientation];
    if (radians)
    {
        if (radians == 0)
            self.transform = CGAffineTransformIdentity;
        else
            self.transform = CGAffineTransformMakeRotation([radians floatValue]);
    }
    
}

- (NSNumber*) getRotationInRadians:(UIDeviceOrientation)deviceOrientation
{
    switch (deviceOrientation)
    {
        case UIDeviceOrientationPortrait: return @0;
        case UIDeviceOrientationPortraitUpsideDown: return @M_PI;
        case UIDeviceOrientationLandscapeLeft: return @M_PI_2;
        case UIDeviceOrientationLandscapeRight: return @-M_PI_2;
        default: return nil;
    }
}

+ (UIImageOrientation) imageOrientationFromDeviceOrientation:(UIDeviceOrientation)deviceOrientation
{
    UIImageOrientation imageOrientation;
    
    switch (deviceOrientation) {
        case UIDeviceOrientationPortrait:
            imageOrientation = UIImageOrientationRight;
            break;
        case UIDeviceOrientationPortraitUpsideDown:
            imageOrientation = UIImageOrientationLeft;
            break;
        case UIDeviceOrientationLandscapeLeft:
            imageOrientation = UIImageOrientationUp;
            break;
        case UIDeviceOrientationLandscapeRight:
            imageOrientation = UIImageOrientationDown;
            break;
            
        default:
            imageOrientation = UIImageOrientationUp;
            break;
    }
    
    return imageOrientation;
}

@end
