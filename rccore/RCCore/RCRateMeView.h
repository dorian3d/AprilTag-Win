//
//  RCRateMeView.h
//  RCCore
//
//  Created by Ben Hirashima on 8/26/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RCSlideBanner.h"

@protocol RCRateMeViewDelegate <NSObject>

- (void) handleRateNowButton;
- (void) handleRateLaterButton;
- (void) handleRateNeverButton;

@end

@interface RCRateMeView : RCSlideBanner

@property (nonatomic) id<RCRateMeViewDelegate> delegate;

@end
