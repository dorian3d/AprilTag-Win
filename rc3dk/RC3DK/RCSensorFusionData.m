//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//


#import "RCSensorFusionData.h"

@implementation RCSensorFusionData
{

}

- (id) initWithTransformation:(RCTransformation*)transformation withCameraTransformation:cameraTransformation withCameraParameters:(RCCameraParameters *)cameraParameters withTotalPath:(RCScalar *)totalPath withFeatures:(NSArray*)featurePoints withSampleBuffer:(CMSampleBufferRef)sampleBuffer withTimestamp:(uint64_t)timestamp
{
    if(self = [super init])
    {
        _transformation = transformation;
        _cameraTransformation = cameraTransformation;
        _cameraParameters = cameraParameters;
        _totalPathLength = totalPath;
        _featurePoints = featurePoints;
        if (sampleBuffer) _sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
        _timestamp = timestamp;
    }
    return self;
}

- (void) dealloc
{
    if (_sampleBuffer) CFRelease(_sampleBuffer);
}

@end