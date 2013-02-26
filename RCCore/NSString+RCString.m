//
//  NSString+RCString.m
//  RCCore
//
//  Created by Ben Hirashima on 2/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "NSString+RCString.h"

@implementation NSString (RCString)

- (BOOL)containsString:(NSString*)search
{
    return [self rangeOfString:search].location != NSNotFound ? YES : NO;
}

@end
