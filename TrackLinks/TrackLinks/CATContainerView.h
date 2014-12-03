//  CATContainerView.h
//  TrackLinks
//
//  Created by Ben Hirashima on 2/27/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

@import UIKit;
#import "CATRotatingView.h"

@interface CATContainerView : UIView <CATRotatingView>

@property (nonatomic) UIResponder* delegate;

@end
