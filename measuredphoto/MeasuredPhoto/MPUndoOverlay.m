//
//  MPUndoOverlay.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/1/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPUndoOverlay.h"

@interface MPUndoOverlay ()

@property (nonatomic) UILabel* messageLabel;
@property (nonatomic) MPTintTouchButton* undoButton;

@end

@implementation MPUndoOverlay
{
    BOOL isUndone;
}

- (id)initWithMessage:(NSString*)message
{
    self = [super init];
    if (self)
    {
        isUndone = NO;
        
        self.backgroundColor = [UIColor lightGrayColor];
        self.layer.cornerRadius = 10.;
        self.layer.masksToBounds = YES; // necessary to prevent drawRect from messing with cornerRadius
        self.alpha = 0;
        
        [self addWidthConstraint:250 andHeightConstraint:60];
        
        _messageLabel = [UILabel new];
        self.messageLabel.text = message;
        self.messageLabel.textColor = [UIColor whiteColor];
        [self addSubview:self.messageLabel];
        [self.messageLabel addCenterYInSuperviewConstraints];
        [self.messageLabel addLeadingSpaceToSuperviewConstraint:20.];
        
        _undoButton = [MPTintTouchButton new];
        [self.undoButton setTitle:@"Undo" forState:UIControlStateNormal];
        self.undoButton.titleLabel.textColor = [UIColor whiteColor];
        [self addSubview:self.undoButton];
        [self.undoButton addTrailingSpaceToSuperviewConstraint:20];
        [self.undoButton addCenterYInSuperviewConstraints];
        [self.undoButton addTarget:self action:@selector(handleButton:) forControlEvents:UIControlEventTouchUpInside];
    }
    return self;
}

- (void) didMoveToSuperview
{
    [self addCenterXInSuperviewConstraints];
    [self addBottomSpaceToSuperviewConstraint:40.];
}

- (void) drawRect:(CGRect)rect
{
    [super drawRect:rect];
    
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextMoveToPoint(context, 168, 10);
    CGContextAddLineToPoint(context, 168, 50);
    
    CGContextSetAlpha(context, 1.0);
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [[UIColor whiteColor] CGColor]);
    CGContextStrokePath(context);
}

- (void) handleButton:(id)sender
{
    isUndone = YES;
    
    if ([self.delegate respondsToSelector:@selector(handleUndoButton)])
    {
        [self.delegate handleUndoButton];
    }
    
    [UIView animateWithDuration:.5 animations:^{
        self.alpha = 0;
    }];
}

- (void) showWithDuration:(NSTimeInterval)duration
{
    self.alpha = 0;
    isUndone = NO;
    
    [UIView animateWithDuration:.3 animations:^{
        
        self.alpha = 1.;
        
    } completion:^(BOOL finished) {
        
        NSTimeInterval fadeDuration = duration < 4. ? duration : 4.;
        NSTimeInterval delay = duration <= 4. ? 0 : duration - 4.;
        
        [UIView animateWithDuration:fadeDuration delay:delay options:UIViewAnimationOptionAllowUserInteraction animations:^{
            self.alpha = .1; // animate to .1 because going to 0 disables user interaction
        } completion:^(BOOL finished) {
            self.alpha = 0;
            
            if (!isUndone && [self.delegate respondsToSelector:@selector(handleUndoPeriodExpired)])
            {
                [self.delegate handleUndoPeriodExpired];
            }
        }];
        
    }];
}

@end
