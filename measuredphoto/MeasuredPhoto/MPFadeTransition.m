//
//  MPFadeTransition.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/25/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPFadeTransition.h"

@implementation MPFadeTransition

static NSTimeInterval const MPAnimatedTransitionDuration = .3f;

- (instancetype)init
{
    if (self = [super init])
    {
        _shouldFadeIn = YES;
        _shouldFadeOut = YES;
    }
    return self;
}

- (void)animateTransition:(id<UIViewControllerContextTransitioning>)transitionContext
{
    if (self.reverse)
    {
        [self animateDismissal:transitionContext];
    }
    else
    {
        [self animatePresentation:transitionContext];
    }
}

- (void) animatePresentation:(id<UIViewControllerContextTransitioning>)transitionContext
{
    UIView* toView;
    UIViewController *toViewController = [transitionContext viewControllerForKey:UITransitionContextToViewControllerKey];
    if ([transitionContext respondsToSelector:@selector(viewForKey:)]) // iOS 8
    {
        toView = [transitionContext viewForKey:UITransitionContextToViewKey];
    }
    else
    {
        toView = toViewController.view;
    }
    
    UIView *container = [transitionContext containerView];
    [container addSubview:toView];
    
    toView.alpha = 0;
    toView.frame = [transitionContext finalFrameForViewController:toViewController];
    
    if (self.shouldFadeIn)
    {
        [UIView animateKeyframesWithDuration:MPAnimatedTransitionDuration delay:0 options:0 animations:^{
            toView.alpha = 1.;
        } completion:^(BOOL finished) {
            
            [transitionContext completeTransition:finished];
        }];
    }
    else
    {
        toView.alpha = 1.;
        [transitionContext completeTransition:YES];
    }
}

- (void) animateDismissal:(id<UIViewControllerContextTransitioning>)transitionContext
{
    UIView* fromView;
    UIViewController*fromViewController = [transitionContext viewControllerForKey:UITransitionContextFromViewControllerKey];
    if ([transitionContext respondsToSelector:@selector(viewForKey:)]) // iOS 8
    {
        fromView = [transitionContext viewForKey:UITransitionContextFromViewKey];
    }
    else
    {
        fromView = fromViewController.view;
    }
    
    UIView* toView;
    UIViewController *toViewController = [transitionContext viewControllerForKey:UITransitionContextToViewControllerKey];
    if ([transitionContext respondsToSelector:@selector(viewForKey:)]) // iOS 8
    {
        toView = [transitionContext viewForKey:UITransitionContextToViewKey];
    }
    else
    {
        toView = toViewController.view;
    }
    
    toView.frame = [transitionContext finalFrameForViewController:toViewController];
    
    if (self.shouldFadeOut)
    {
        UIView *container = [transitionContext containerView];
        [container insertSubview:toView belowSubview:fromView];
        
        fromView.alpha = 1.;
        [UIView animateKeyframesWithDuration:MPAnimatedTransitionDuration delay:0 options:0 animations:^{
            fromView.alpha = 0;
        } completion:^(BOOL finished) {
            [transitionContext completeTransition:finished];
        }];
    }
    else
    {
        [transitionContext completeTransition:YES];
    }
}

- (NSTimeInterval)transitionDuration:(id<UIViewControllerContextTransitioning>)transitionContext
{
    return MPAnimatedTransitionDuration;
}

@end
