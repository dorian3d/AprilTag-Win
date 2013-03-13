//
//  TMAccountCredentialsVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMCreateAccountVC.h"

@implementation TMCreateAccountVC

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    NSMutableArray *navigationArray = [[NSMutableArray alloc] initWithArray: self.navigationController.viewControllers];
    
    if(navigationArray.count > 2)
    {
        [navigationArray removeObjectAtIndex: navigationArray.count - 2];  // remove login from nav array, so back button goes to history instead
        self.navigationController.viewControllers = navigationArray;
    }
    
    fieldArray = [NSArray arrayWithObjects: self.emailBox, self.passwordBox, self.passwordAgainBox, self.firstNameBox, self.lastNameBox, nil];
    for (UITextField *field in fieldArray) field.delegate = self;
}

- (void)viewDidUnload
{
    [self setLoginButton:nil];
    [self setActionTypeButton:nil];
    [self setPasswordAgainCell:nil];
    [self setEmailBox:nil];
    [self setPasswordBox:nil];
    [self setPasswordAgainBox:nil];
    [self setPasswordAgainLabel:nil];
    [self setFirstNameBox:nil];
    [self setFirstNameLabel:nil];
    [self setLastNameBox:nil];
    [self setLastNameLabel:nil];
    [super viewDidUnload];
}

//handles next and go buttons on keyboard
- (BOOL) textFieldShouldReturn:(UITextField *) textField {
    BOOL didResign = [textField resignFirstResponder];
    if (!didResign) return NO;
    
    NSUInteger index = [fieldArray indexOfObject:textField];
    if (index == NSNotFound || index + 1 == fieldArray.count)
    {
        [self validateAndSubmit];
        return NO;
    }
    
    id nextField = [fieldArray objectAtIndex:index + 1];
    activeField = nextField;
    [nextField becomeFirstResponder];
    
    return NO;
}

- (IBAction)handleActionTypeButton:(id)sender
{
    if (self.actionTypeButton.selectedSegmentIndex == 1)
    {
        [self performSegueWithIdentifier:@"toLogin" sender:self];
    }
}

- (IBAction)handleCreateAccountButton:(id)sender
{
    [self validateAndSubmit];
}

- (void)validateAndSubmit
{
    if ([self isInputValid])
    {
        [self updateUserAccount];
    }
}

- (BOOL)isInputValid
{
    self.emailBox.text = [self.emailBox.text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];
    
    if (![RCUser isValidEmail:self.emailBox.text])
    {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Whoops"
                                                        message:@"Check your email address"
                                                       delegate:nil
                                              cancelButtonTitle:@"OK"
                                              otherButtonTitles:nil];
        
        [alert show];
        return NO;
    }
    
    if (self.passwordBox.text.length < 6)
    {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Whoops"
                                                        message:@"Please choose a password with at least 6 characters"
                                                       delegate:nil
                                              cancelButtonTitle:@"OK"
                                              otherButtonTitles:nil];
        
        [alert show];
        return NO;
    }
    
    if ([self.passwordBox.text isEqualToString:self.passwordAgainBox.text])
    {
        return YES;
    }
    else
    {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Whoops"
                                                        message:@"Passwords don't match"
                                                       delegate:nil
                                              cancelButtonTitle:@"OK"
                                              otherButtonTitles:nil];
        
        [alert show];
        return NO;
    }
}

- (void)updateUserAccount
{
    HUD = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
	[self.navigationController.view addSubview:HUD];
	HUD.labelText = @"Thinking";
	
	[HUD show:YES];
    
    RCUser *user = [RCUser getStoredUser];
    
    user.username = self.emailBox.text; //we use email as username
    user.password = self.passwordBox.text;
    user.firstName = [self.firstNameBox.text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];
    user.lastName = [self.lastNameBox.text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];
    
    if ([USER_MANAGER isLoggedIn])
    {
        [USER_MANAGER
         updateUser:user
         onSuccess:^()
         {
             [Flurry logEvent:@"User.CreateAccount"];
             
             [HUD hide:YES];
             [user saveUser];
             [self performSegueWithIdentifier:@"toHistory" sender:self];
         }
         onFailure:^(int statusCode)
         {
             [HUD hide:YES];
             NSLog(@"Update user failure callback");
             
             UIAlertView *alert = [self getFailureAlertForStatusCode:statusCode];
             [alert show];
         }
         ];
    }
}

- (UIAlertView*)getFailureAlertForStatusCode:(int)statusCode
{
    NSString *title;
    NSString *msg = @"Whoops, sorry about that! Try again later.";
    if (statusCode >= 500 && statusCode < 600)
    {
        title = @"Server error";
    }
    else if (statusCode == 409)
    {
        title = @"Whoa now";
        msg = @"Looks like that email address has already been registered. Maybe try logging in?";
    }
    else
    {
        title = @"Something went wrong";
    }
    
    return [[UIAlertView alloc] initWithTitle:title
                                      message:msg
                                     delegate:nil
                            cancelButtonTitle:@"OK"
                            otherButtonTitles:nil];
}
@end
