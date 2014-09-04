//
//  TMLocalMoviePlayer.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 8/21/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <MediaPlayer/MediaPlayer.h>
#import "TMViewController.h"
#import <MediaPlayer/MediaPlayer.h>

@protocol TMLocalMoviePlayerDelegate <NSObject>

- (void) moviePlayerDismissed;

@end

@interface TMLocalMoviePlayer : TMViewController

@property (weak, nonatomic) id<TMLocalMoviePlayerDelegate> delegate;
@property (nonatomic) MPMoviePlayerController* moviePlayer;
@property (weak, nonatomic) IBOutlet UIButton *playButton;
@property (strong, nonatomic) IBOutlet UIView *skipButton;
@property (weak, nonatomic) IBOutlet UIButton *playAgainButton;
@property (weak, nonatomic) IBOutlet UIButton *continueButton;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;

- (IBAction)handlePlayButton:(id)sender;
- (IBAction)handleSkipButton:(id)sender;
- (IBAction)handlePlayAgainButton:(id)sender;
- (IBAction)handleContinueButton:(id)sender;

@end
