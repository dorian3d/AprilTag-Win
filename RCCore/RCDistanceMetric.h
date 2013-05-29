//
//  RCDistance.h
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistance.h"

#define METERS_PER_INCH 0.0254
#define METERS_PER_FOOT 0.3048
#define METERS_PER_YARD 0.9144
#define METERS_PER_MILE 1609.344
#define INCHES_PER_METER 39.3700787
#define INCHES_PER_FOOT 12
#define INCHES_PER_YARD 36
#define INCHES_PER_MILE 63360
#define SIXTEENTH_INCH 0.03125
#define THIRTYSECOND_INCH 0.015625

@interface RCDistanceMetric : NSObject <RCDistance>
{
    NSString* stringRep;
    float convertedDist;
}

@property float meters;
@property UnitsScale scale;
@property NSString* unitSymbol;

- (id) initWithMeters:(float)distance withScale:(UnitsScale)scale;
- (NSString*) getString;

@end
