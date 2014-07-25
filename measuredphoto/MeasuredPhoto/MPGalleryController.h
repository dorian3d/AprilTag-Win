//
//  MPGalleryController.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/16/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPEditPhoto.h"
#import "MPZoomTransitionDelegate.h"

@interface MPGalleryController : UIViewController <UICollectionViewDataSource, UICollectionViewDelegate, MPEditPhotoDelegate, MPZoomTransitionFromView>

@property (weak, nonatomic) IBOutlet UIButton *cameraButton;
@property (weak, nonatomic) IBOutlet UIButton *menuButton;
@property (weak, nonatomic) IBOutlet UICollectionView *collectionView;
@property (nonatomic, readonly) UIView* transitionFromView;
@property (weak, nonatomic) IBOutlet UIView *navBar;

- (IBAction)handleMenuButton:(id)sender;
- (IBAction)handleCameraButton:(id)sender;
- (IBAction)handleImageButton:(id)sender;

@end
