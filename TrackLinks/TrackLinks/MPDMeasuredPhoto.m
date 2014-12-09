//
//  MPDMeasuredPhoto.m
//  TrackLinks
//
//  Created by Ben Hirashima on 12/8/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "MPDMeasuredPhoto.h"
#import "CATConstants.h"
#import "UIImage+MPImageFile.h"
#import "UIView+RCOrientationRotation.h"
#import "NSString+RCString.h"

@implementation MPDMeasuredPhoto

- (instancetype)init
{
    if (self = [super init])
    {
        NSString* guid = [[NSUUID UUID] UUIDString];
        _id_guid = guid;
        _created_at = [[NSDate date] timeIntervalSince1970];
    }
    return self;
}

- (BOOL) writeImagetoJpeg:(CMSampleBufferRef)sampleBuffer withOrientation:(UIDeviceOrientation)orientation
{
    if (sampleBuffer)
    {
        sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
    }
    else
    {
        return NO;
    }
    
    NSData* jpgData = [UIImage jpegDataFromSampleBuffer:sampleBuffer
                                        withOrientation:[UIView imageOrientationFromDeviceOrientation:orientation]];
    
    NSError* error;
    BOOL success = [jpgData writeToFile:[self imageFileName] options:NSDataWritingFileProtectionNone error:&error];
    
    CFRelease(sampleBuffer);
    
    if (error || !success)
    {
        DLog(@"FAILED TO WRITE JPEG: %@", error); // TODO: better error handling
        return NO;
    }
    else
    {
        return YES;
    }
}

- (NSString*) imageFileName
{
    NSString* fileName = [NSString stringWithFormat:@"%@.jpg", self.id_guid];
    NSURL* fileUrl = [DOCUMENTS_DIRECTORY_URL URLByAppendingPathComponent:fileName];
    return fileUrl.path;
}

- (NSString*) depthFileName
{
    NSString* fileName = [NSString stringWithFormat:@"%@-stereo-remesh.json", self.id_guid];
    NSURL* fileUrl = [DOCUMENTS_DIRECTORY_URL URLByAppendingPathComponent:fileName];
    return fileUrl.path;
}

- (NSString*) annotationsFileName
{
    NSString* fileName = [NSString stringWithFormat:@"%@-annotations.json", self.id_guid];
    NSURL* fileUrl = [DOCUMENTS_DIRECTORY_URL URLByAppendingPathComponent:fileName];
    return fileUrl.path;
}

- (BOOL) writeAnnotationsToFile:(NSString*)jsonString
{
    NSError* error = nil;
    BOOL result = [jsonString writeToFile:[self annotationsFileName] atomically:NO encoding:NSUTF8StringEncoding error:&error];
    if (error) DLog(@"%@", error);
    return result;
}

- (BOOL) deleteAssociatedFiles
{
    BOOL isSuccess = YES;
    
    NSError* error;
    NSArray* dirContents = [[NSFileManager defaultManager] contentsOfDirectoryAtURL:DOCUMENTS_DIRECTORY_URL includingPropertiesForKeys:nil options:0 error:&error];
    
    if (error || dirContents == nil)
    {
        DLog(@"Failed to get documents directory contents: %@", error);
        return NO;
    }
    
    if (dirContents.count == 0)
    {
        DLog(@"Documents directory is empty");
        return YES;
    }
    
    for (NSURL* url in dirContents)
    {
        NSString* fileName = url.pathComponents.lastObject;
        if (url.pathExtension.length > 0 && [fileName beginsWithString:self.id_guid])
        {
            isSuccess = [[NSFileManager defaultManager] removeItemAtURL:url error:&error];
            if (error)
            {
                DLog(@"Error deleting file %@: %@", url, error);
                isSuccess = NO;
            }
        }
    }
    
    return isSuccess;
}

@end
