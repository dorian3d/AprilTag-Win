//
//  MPRotatingView.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/6/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

@protocol MPRotatingView <NSObject>

- (void) handleOrientationChange:(UIDeviceOrientation)orientation;

@end
