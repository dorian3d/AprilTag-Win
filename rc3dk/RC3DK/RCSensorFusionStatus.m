//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCSensorFusionStatus.h"

@implementation RCSensorFusionStatus
{

}

- (id) initWithRunState:(RCSensorFusionRunState)runState withProgress:(float)progress withErrorCode:(RCSensorFusionErrorCode)errorCode
{
    if(self = [super init])
    {
        _runState = runState;
        _progress = progress;
        _errorCode = errorCode;
    }
    return self;
}

@end