//
//  MPFadeTransition.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/25/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MPFadeTransition : NSObject <UIViewControllerAnimatedTransitioning>

@property (nonatomic) BOOL reverse;
@property (nonatomic) BOOL shouldFadeIn;
@property (nonatomic) BOOL shouldFadeOut;

@end
