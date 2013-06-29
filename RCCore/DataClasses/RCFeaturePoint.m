//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCFeaturePoint.h"
#import "RCPoint.h"

@implementation RCFeaturePoint
{

}

- (id) initWithId:(uint64_t)id withX:(float)x withY:(float)y withDepth:(RCScalar *)depth withWorldPoint:(RCPoint *)worldPoint withInitialized:(bool)initialized
{
    if(self = [super init])
    {
        _x = x;
        _y = y;
        _depth = depth;
        _worldPoint = worldPoint;
        _initialized = initialized;
    }
    return self;
}

@end