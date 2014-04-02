//
//  MPInstructionsView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/4/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPInstructionsView.h"
#import "MPCircleLayerDelegate.h"
#import "MPDotLayerDelegate.h"
#import <RCCore/RCCore.h>

@implementation MPInstructionsView
{
    CALayer* circleLayer;
    CALayer* dotLayer;
    MPCircleLayerDelegate* circleLayerDel;
    MPDotLayerDelegate* dotLayerDel;
    float circleRadius;
}
@synthesize delegate;

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        self.translatesAutoresizingMaskIntoConstraints = NO;
        
        circleLayer = [CALayer new];
        [self.layer addSublayer:circleLayer];
        
        dotLayer = [CALayer new];
        [self.layer addSublayer:dotLayer];
    }
    return self;
}

- (void) layoutSubviews
{
    [super layoutSubviews];
    
    circleRadius = (self.bounds.size.width - 5) / 2; // TODO: half line width is hard coded
    
    circleLayer.frame = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
    circleLayerDel = [MPCircleLayerDelegate new];
    circleLayer.delegate = circleLayerDel;
    [circleLayer setNeedsDisplay];
    
    dotLayer.frame = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
    dotLayerDel = [MPDotLayerDelegate new];
    dotLayer.delegate = dotLayerDel;
    [dotLayer setNeedsDisplay];
}

- (void) moveDotTo:(CGPoint)point
{
    dotLayer.frame = CGRectMake(point.x, point.y, dotLayer.frame.size.width, dotLayer.frame.size.height);
    [dotLayer setNeedsLayout];
}

- (void) moveDotToCenter
{
    [self moveDotTo:CGPointMake(0, 0)];
}

- (void) updateDotPosition:(RCTransformation*)transformation
{
//    DLog("%0.1f %0.1f %0.1f", transformation.translation.x, transformation.translation.y, transformation.translation.z);
    
    // TODO replace this fake calculation with one that calculates the distance moved in the plane of the photo
    float distFromStartPoint = sqrt(transformation.translation.x * transformation.translation.x + transformation.translation.y * transformation.translation.y + transformation.translation.z * transformation.translation.z);
    float targetDist = .2;
    float progress = distFromStartPoint / targetDist;
    
    if (progress >= 1)
    {
        if ([delegate respondsToSelector:@selector(moveComplete)]) [delegate moveComplete];
    }
    else
    {
        float xPos = circleRadius * progress;
        if (transformation.translation.x < 0) xPos = -xPos;
        [self moveDotTo:CGPointMake(xPos, 0)];
    }
}

@end
