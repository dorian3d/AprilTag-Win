//
//  MPCircleLayerDelegate.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/4/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPCircleLayerDelegate.h"

@implementation MPCircleLayerDelegate

-(void) drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    float const xCenter = layer.frame.size.width / 2;
    float const yCenter = layer.frame.size.height / 2;
    float const circleRadius = 200.;
    
    CGContextMoveToPoint(context, xCenter, yCenter);
    CGContextSetAlpha(context, 0.5);
    CGContextBeginPath(context);
    CGContextSetLineWidth(context, 10.0);
    CGContextSetStrokeColorWithColor(context, [UIColor greenColor].CGColor);
    CGContextAddArc(context, xCenter, yCenter, circleRadius, -M_PI, M_PI, 1);
    CGContextClosePath(context);
    CGContextStrokePath(context);
}

@end
