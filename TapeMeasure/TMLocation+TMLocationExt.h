//
//  TMLocation+TMLocationExt.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/1/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMLocation.h"
#import "TMDataManagerFactory.h"

@interface TMLocation (TMLocationExt)

+ (TMMeasurement*)getNewLocation;
+ (TMMeasurement*)getLocationById:(int)dbid;

@end
