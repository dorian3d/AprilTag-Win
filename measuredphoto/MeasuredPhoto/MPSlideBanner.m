//
//  MPSlideBanner.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPSlideBanner.h"
#import "UIView+MPConstraints.h"

@interface MPSlideBanner ()
@property (readwrite, nonatomic) MPSlideBannerState state;
@end

@implementation MPSlideBanner
{
    NSLayoutConstraint* topToSuperviewConstraint;
}
@synthesize state;

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:(NSCoder *)aDecoder])
    {
        state = MPSlideBannerStateShowing; // assume view is showing in storyboard
    }
    return self;
}

- (void) layoutSubviews
{
    [super layoutSubviews];
    topToSuperviewConstraint = [self findTopToSuperviewConstraint];
}

- (void) showInstantly
{
    self.hidden = NO;
    if (state == MPSlideBannerStateShowing) return;
    state = MPSlideBannerStateShowing;
    [self moveDown];
}

- (void) showAnimated
{
    if (state != MPSlideBannerStateHidden) return;
    state = MPSlideBannerStateAnimating;
    
    [self.superview layoutIfNeeded];
    [UIView animateWithDuration: .5
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         [self showInstantly];
                         [self.superview layoutIfNeeded];
                     }
                     completion:nil];
}

- (void) hideInstantly
{
    if (state == MPSlideBannerStateHidden) return;
    state = MPSlideBannerStateHidden;
    [self moveUp];
}

- (void) hideWithDelay:(float)secs onCompletion:(void (^)(BOOL finished))completionBlock
{
    if (state != MPSlideBannerStateShowing) return;
    state = MPSlideBannerStateAnimating;
    
    [self.superview layoutIfNeeded];
    [UIView animateWithDuration: .5
                          delay: secs
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         [self hideInstantly];
                         [self.superview layoutIfNeeded];
                     }
                     completion:completionBlock];
}

- (void) moveDown
{
    if (topToSuperviewConstraint) topToSuperviewConstraint.constant = 0;
}

- (void) moveUp
{
    if (topToSuperviewConstraint) topToSuperviewConstraint.constant = -self.bounds.size.height;
}

@end
