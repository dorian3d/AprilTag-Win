//
//  RCDeviceInfo.m
//  RCCore
//
//  Created by Ben Hirashima on 2/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDeviceInfo.h"

@implementation RCDeviceInfo

+ (NSString*) getOSVersion
{
    return [[UIDevice currentDevice] systemVersion];
}

+ (NSString *) getPlatformString {
    // Gets a string with the device model
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    char *machine = malloc(size);
    sysctlbyname("hw.machine", machine, &size, NULL, 0);
    NSString *platform = [NSString stringWithCString:machine encoding:NSUTF8StringEncoding];
    free(machine);
    
    return platform;
}

+ (DeviceType) getDeviceType
{
    NSString *platform = [self getPlatformString];

    if ([platform isEqualToString:@"iPhone1,1"])    return DeviceTypeUnknown;
    if ([platform isEqualToString:@"iPhone1,2"])    return DeviceTypeUnknown;
    if ([platform isEqualToString:@"iPhone2,1"])    return DeviceTypeUnknown;
    if ([platform isEqualToString:@"iPhone3,1"])    return DeviceTypeUnknown;
    if ([platform isEqualToString:@"iPhone3,2"])    return DeviceTypeUnknown;
    if ([platform isEqualToString:@"iPhone3,3"])    return DeviceTypeUnknown;
    if ([platform isEqualToString:@"iPhone4,1"])    return DeviceTypeiPhone4s;
    if ([platform isEqualToString:@"iPhone5,1"])    return DeviceTypeiPhone5;
    if ([platform isEqualToString:@"iPhone5,2"])    return DeviceTypeiPhone5;

    if ([platform isEqualToString:@"iPod1,1"])      return DeviceTypeUnknown;
    if ([platform isEqualToString:@"iPod2,1"])      return DeviceTypeUnknown;
    if ([platform isEqualToString:@"iPod3,1"])      return DeviceTypeUnknown;
    if ([platform isEqualToString:@"iPod4,1"])      return DeviceTypeUnknown;
    if ([platform isEqualToString:@"iPod5,1"])      return DeviceTypeiPod5;

    if ([platform isEqualToString:@"iPad1,1"])      return DeviceTypeUnknown;
    if ([platform isEqualToString:@"iPad1,2"])      return DeviceTypeUnknown;
    if ([platform isEqualToString:@"iPad2,1"])      return DeviceTypeiPad2;
    if ([platform isEqualToString:@"iPad2,2"])      return DeviceTypeiPad2;
    if ([platform isEqualToString:@"iPad2,3"])      return DeviceTypeiPad2;
    if ([platform isEqualToString:@"iPad2,4"])      return DeviceTypeiPad2;
    if ([platform isEqualToString:@"iPad2,5"])      return DeviceTypeiPadMini;
    if ([platform isEqualToString:@"iPad2,6"])      return DeviceTypeiPadMini;
    if ([platform isEqualToString:@"iPad2,7"])      return DeviceTypeiPadMini;
    if ([platform isEqualToString:@"iPad3,1"])      return DeviceTypeiPad3;
    if ([platform isEqualToString:@"iPad3,2"])      return DeviceTypeiPad3; 
    if ([platform isEqualToString:@"iPad3,3"])      return DeviceTypeiPad3; 
    if ([platform isEqualToString:@"iPad3,4"])      return DeviceTypeiPad4;
    if ([platform isEqualToString:@"iPad3,5"])      return DeviceTypeiPad4; 
    if ([platform isEqualToString:@"iPad3,6"])      return DeviceTypeiPad4; 

    if ([platform isEqualToString:@"i386"])         return DeviceTypeUnknown;
    if ([platform isEqualToString:@"x86_64"])       return DeviceTypeUnknown;

    return DeviceTypeUnknown;
}

//+ (NSString *) getPlatformHumanReadable
//{
//    NSString *platform = [self getPlatformString];
//    
//    if ([platform isEqualToString:@"iPhone1,1"])    return @"iPhone 2G";
//    if ([platform isEqualToString:@"iPhone1,2"])    return @"iPhone 3G";
//    if ([platform isEqualToString:@"iPhone2,1"])    return @"iPhone 3GS";
//    if ([platform isEqualToString:@"iPhone3,1"])    return @"iPhone 4";
//    if ([platform isEqualToString:@"iPhone3,2"])    return @"iPhone 4";
//    if ([platform isEqualToString:@"iPhone3,3"])    return @"iPhone 4 (CDMA)";
//    if ([platform isEqualToString:@"iPhone4,1"])    return @"iPhone 4S";
//    if ([platform isEqualToString:@"iPhone5,1"])    return @"iPhone 5";
//    if ([platform isEqualToString:@"iPhone5,2"])    return @"iPhone 5 (GSM+CDMA)";
//    
//    if ([platform isEqualToString:@"iPod1,1"])      return @"iPod Touch (1 Gen)";
//    if ([platform isEqualToString:@"iPod2,1"])      return @"iPod Touch (2 Gen)";
//    if ([platform isEqualToString:@"iPod3,1"])      return @"iPod Touch (3 Gen)";
//    if ([platform isEqualToString:@"iPod4,1"])      return @"iPod Touch (4 Gen)";
//    if ([platform isEqualToString:@"iPod5,1"])      return @"iPod Touch (5 Gen)";
//    
//    if ([platform isEqualToString:@"iPad1,1"])      return @"iPad";
//    if ([platform isEqualToString:@"iPad1,2"])      return @"iPad 3G";
//    if ([platform isEqualToString:@"iPad2,1"])      return @"iPad 2 (WiFi)";
//    if ([platform isEqualToString:@"iPad2,2"])      return @"iPad 2";
//    if ([platform isEqualToString:@"iPad2,3"])      return @"iPad 2 (CDMA)";
//    if ([platform isEqualToString:@"iPad2,4"])      return @"iPad 2";
//    if ([platform isEqualToString:@"iPad2,5"])      return @"iPad Mini (WiFi)";
//    if ([platform isEqualToString:@"iPad2,6"])      return @"iPad Mini";
//    if ([platform isEqualToString:@"iPad2,7"])      return @"iPad Mini (GSM+CDMA)";
//    if ([platform isEqualToString:@"iPad3,1"])      return @"iPad 3 (WiFi)";
//    if ([platform isEqualToString:@"iPad3,2"])      return @"iPad 3 (GSM+CDMA)";
//    if ([platform isEqualToString:@"iPad3,3"])      return @"iPad 3";
//    if ([platform isEqualToString:@"iPad3,4"])      return @"iPad 4 (WiFi)";
//    if ([platform isEqualToString:@"iPad3,5"])      return @"iPad 4";
//    if ([platform isEqualToString:@"iPad3,6"])      return @"iPad 4 (GSM+CDMA)";
//    
//    if ([platform isEqualToString:@"i386"])         return @"Simulator";
//    if ([platform isEqualToString:@"x86_64"])       return @"Simulator";
//    
//    return platform;
//}

@end
