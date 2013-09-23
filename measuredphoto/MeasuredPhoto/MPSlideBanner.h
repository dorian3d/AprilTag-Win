//
//  MPSlideBanner.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

typedef NS_ENUM(int, MPSlideBannerPosition) {
    MPSlideBannerPositionTop,
    MPSlideBannerPositionBottom
};

typedef NS_ENUM(int, MPSlideBannerState) {
    MPSlideBannerStateShowing,
    MPSlideBannerStateAnimating,
    MPSlideBannerStateHidden
};

@interface MPSlideBanner : UIView

@property (nonatomic) MPSlideBannerPosition position;
@property (nonatomic, readonly) MPSlideBannerState state;
@property (nonatomic, readonly) UIInterfaceOrientation orientation;

- (void) showInstantly;
- (void) showAnimated;
- (void) hideWithDelay:(float)secs onCompletion:(void (^)(BOOL finished))completionBlock;
- (void) hideInstantly;
- (void) handleOrientationChange:(UIDeviceOrientation)deviceOrientation;

@end
