//
//  TMAugmentedRealityView.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/glext.h>
#import <CoreVideo/CVOpenGLESTextureCache.h>
#import <AVFoundation/AVFoundation.h>
#import <RCCore/RCCore.h>

@interface MPVideoPreview : RCVideoPreviewCRT <RCVideoFrameDelegate>

- (CGRect) getCrtClosedFrame:(UIDeviceOrientation)orientation;
- (void) setViewTransform:(RCTransformation *)viewTransform withCameraParameters:(RCCameraParameters *)cameraParameters;

@end

