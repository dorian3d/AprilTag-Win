//
//  MPEditTitleController.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPEditTitleController.h"
#import "CoreData+MagicalRecord.h"

@interface MPEditTitleController ()

@end

@implementation MPEditTitleController
{
    BOOL isCanceled;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.titleText.delegate = self;
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleOrientationChange:)
                                                 name:MPUIOrientationDidChangeNotification
                                               object:nil];
}

- (void) viewDidAppear:(BOOL)animated
{
    isCanceled = NO;
    
    [self.titleText becomeFirstResponder];
    
    UIImage* photo = [UIImage imageWithContentsOfFile:self.measuredPhoto.imageFileName];
    [self.photoView setImage: photo];
    self.titleText.text = self.measuredPhoto.name;
}

- (IBAction)handleCancelButton:(id)sender
{
    isCanceled = YES;
    [self.presentingViewController dismissViewControllerAnimated:NO completion:nil];
}

- (void) setMeasuredPhoto:(MPDMeasuredPhoto *)measuredPhoto
{
    _measuredPhoto = measuredPhoto;
}

- (void) handleOrientationChange:(NSNotification*)notification
{
    UIDeviceOrientation orientation;
    
    if (notification.object)
    {
        [((NSValue*)notification.object) getValue:&orientation];
        
        if (orientation == UIDeviceOrientationPortrait || orientation == UIDeviceOrientationPortraitUpsideDown)
        {
//            [self.titleText resignFirstResponder];
        }
    }
}

#pragma mark - UITextFieldDelegate

- (void) textFieldDidEndEditing:(UITextField *)textField
{
    if (!isCanceled)
    {
        self.measuredPhoto.name = textField.text;
        
        [CONTEXT MR_saveToPersistentStoreWithCompletion:^(BOOL success, NSError *error) {
            if (success) {
                DLog(@"Saved CoreData context.");
            } else if (error) {
                DLog(@"Error saving context: %@", error.description);
            }
        }];
        
        [self.presentingViewController dismissViewControllerAnimated:NO completion:nil];
    }
}

- (BOOL) textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return YES;
}

@end
