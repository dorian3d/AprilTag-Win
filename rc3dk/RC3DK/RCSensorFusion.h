//
//  RCSensorFusion.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreMedia/CoreMedia.h>
#import <CoreLocation/CoreLocation.h>
#import <CoreMotion/CoreMotion.h>
#import "RCMotionManager.h"
#import "RCVideoManager.h"
#import "RCSensorFusionData.h"
#import "RCSensorFusionStatus.h"

/** Contains information about problems encountered by RCSensorFusion.
 
 Not all errors indicate failure, but some may. In that case, you can call [RCSensorFusion resetSensorFusion]. */
@interface RCSensorFusionError : NSError

/** The device moved much faster than expected.
 
 It is possible to proceed normally without addressing this error, but it may also indicate that the output is no longer valid. */
@property (readonly, nonatomic) bool speed;

/** No visual features were detected in the most recent image.
 
 This is normal in some circumstances, such as quick motion or if the device temporarily looks at a blank wall. However, if this is received repeatedly, it may indicate that the camera is covered or it is too dark. */
@property (readonly, nonatomic) bool vision;

/** A fatal internal error has occured.
 
 Please contact RealityCap and provide the code property of the RCSensorFusionError object. */
@property (readonly, nonatomic) bool other;

@end

/** The delegate of RCSensorFusion must implement this protocol in order to receive sensor fusion updates. */
@protocol RCSensorFusionDelegate <NSObject>

/** Sent to the delegate to provide the latest output of sensor fusion.
 
 This is called after each video frame is processed by RCSensorFusion, typically 30 times per second. If video data is not being processed, it will still be called, but at ~10Hz.
 @param data An instance of RCSensorFusionData that contains the latest sensor fusion data.
 */
- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data;

/** Sent to the delegate if RCSensorFusion encounters a problem.
 @param error An instance of RCSensorFusionError indicating the problem encountered. Some conditions indicate a fatal error, meaning that the delegate must take action to continue (typically by calling [RCSensorFusion resetSensorFusion]).
 */
- (void) sensorFusionError:(RCSensorFusionError*)error;

@end

/** This class is the business end of the library, and the only one that you really need to use in order to get data out of it.
 This class is a psuedo-singleton. You shouldn't instantiate this class directly, but rather get an instance of it via the
 sharedInstance class method.
 
 Typical usage of this class would go something like this:

    // Get the sensor fusion object and set the delegate.
    RCSensorFusion* sensorFusion = [RCSensorFusion sharedInstance];
    sensorFusion.delegate = self;
 
    // Initialize sensor fusion.
    [sensorFusion startInertialOnlyFusion];
 
    // Pass in a CLLocation object that represents the device's current location.
    [sensorFusion setLocation:location];

    // Call these methods to repeatedly pass in inertial data.
    [sensorFusion receiveAccelerometerData:accelerometerData];
    [sensorFusion receiveGyroData:gyroData];

    // Begin processing video.
    [sensorFusion startProcessingVideo];

    // Continue calling the above methods to pass in inertial data, and begin passing in video data as well.
    [sensorFusion receiveVideoFrame:sampleBuffer];

    // Implement the RCSensorFusionDelegate protocol methods to receive sensor fusion data.
    - (void) sensorFusionDidUpdate:(RCSensorFusionData*)data {}
    - (void) sensorFusionError:(NSError*)error {}

    // When you no longer want to receive sensor fusion data, stop it.
    // This releases a significant amount of resources, so be sure to call it.
    [sensorFusion stopSensorFusion];
 
 */
@interface RCSensorFusion : NSObject

/** Set this property to a delegate object that will receive the sensor fusion updates. The object must implement the RCSensorFusionDelegate protocol. */
@property (weak) id<RCSensorFusionDelegate> delegate;

/** Use this method to get a shared instance of this class */
+ (RCSensorFusion *) sharedInstance;

/** Prepares the object to receive inertial data and process it in the background to maintain internal state.
 
 This method should be called as early as possible, preferably when your app loads; you should then start passing in accelerometer and gyro data using receiveAccelerometerData and receiveGyroData as soon as possible. This will consume a small amount of CPU in a background thread.
 */
- (void) startInertialOnlyFusion;

/** Sets the current location of the device.
 
 @param location The device's current location (including altitude) is used to account for differences in gravity across the earth. If location is unavailable, results may be less accurate.
*/
- (void) setLocation:(CLLocation*)location;

/** Starts a special one-time static calibration mode.
 
 This method may be called after startInertialOnlyFusion to estimate internal parameters; running it once on a particular device should improve the quality of output for that device. The device should be placed on a solid surface (not held in the hand), and left completely still for the duration of the static calibration. The camera is not used in this mode, so it is OK if the device is placed on its back. Check [RCSensorFusionStatus calibrationProgress] to determine how well the parameters have been calibrated. When finished, call stopStaticCalibration to return to inertial-only fusion and store the resulting device-specific calibration parameters. You do not need to call startProcessingVideo when running static calibration.
 */
- (void) startStaticCalibration;

/** Exits static calibration mode.
 
 After static calibration has finished, calling this method will save the resulting device-specific calibration parameters and return to inertial-only fusion.
 */
- (void) stopStaticCalibration;

/** Prepares the object to receive video data, and starts sensor fusion updates.
 
 This method should be called when you are ready to begin receiving sensor fusion updates and your user is aware to point the camera at an appropriate visual scene. It should be called as long after startInertialOnlyFusion as possible to allow time for initialization. If it is called too soon, you may not receive valid updates for a short time. After you call this method you should immediately begin passing video data using receiveVideoFrame.
 */
- (void) startProcessingVideo;

/** Stops processing video data and stops sensor fusion updates.
 
 Returns to inertial-only mode, which reduces CPU usage significantly while maintaining internal filter state. Calling this method between uses of RCSensorFusion rather than stopSensorFusion will improve quality and eliminate any initialization time when starting again.
 */
- (void) stopProcessingVideo;

/** Request that sensor fusion attempt to track a user-selected feature.
 
 If you call this method, the sensor fusion algorithm will make its best effort to detect and track a visual feature near the specified image coordinates. There is no guarantee that such a feature may be identified or tracked for any length of time (for example, if you specify coordinates in the middle of a blank wall, no feature will be found. Any such feature is also not likely to be found at the exact pixel coordinates specified.
 @param x The requested horizontal location, in pixels relative to the image coordinate frame.
 @param x The requested vertical location, in pixels relative to the image coordinate frame.
 */
- (void) selectUserFeatureWithX:(float)x withY:(float)Y;

/** Stops the processing of video and inertial data and releases all related resources. */
- (void) stopSensorFusion;

/** Fully resets the object to the state it would be in after calling startInertialOnlyFusion.
 
 This could be called after receiving certain errors in [RCSensorFusionDelegate sensorFusionError:].*/
- (void) resetSensorFusion;

/** @returns True if startInertialOnlyFusion has been called and stopSensorFusion has not been called. */
- (BOOL) isSensorFusionRunning;

/** Sets the physical origin of the coordinate system to the current location.
 
 Immediately after calling this method, the translation returned to the delegate will be (0, 0, 0).
 */
- (void) resetOrigin;

/** Once sensor fusion has started, video frames should be passed in as they are received from the camera. 
 @param sampleBuffer A CMSampleBufferRef representing a single video frame. You can obtain the sample buffer via the AVCaptureSession class, or you can use RCAVSessionManager to manage the session and pass the frames in for you. In either case, you can retrieve a sample buffer after it has been processed from [RCSensorFusionData sampleBuffer]. If you manage the AVCaptureSession yourself, you must use the 640x480 preset ([AVCaptureSession setSessionPreset:AVCaptureSessionPreset640x480]) and set the output format to 420f ([AVCaptureVideoDataOutput setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:'420f'] forKey:(id)kCVPixelBufferPixelFormatTypeKey]]).
 */
- (void) receiveVideoFrame:(CMSampleBufferRef)sampleBuffer;

/** Once sensor fusion has started, acceleration data should be passed in as it's received from the accelerometer.
 @param accelerometerData The CMAccelerometerData object. You can obtain the CMAccelerometerData object from CMMotionManager, or you can use RCMotionManager to handle the setup and passing of motion data for you. If you manage CMMotionManager yourself, you must set the accelerometer update interval to .01 ([CMMotionManager setAccelerometerUpdateInterval:.01]).
 */
- (void) receiveAccelerometerData:(CMAccelerometerData *)accelerometerData;

/** Once sensor fusion has started, angular velocity data should be passed in as it's received from the gyro.
 @param gyroData The CMGyroData object. You can obtain the CMGyroData object from CMMotionManager, or you can use RCMotionManager to handle the setup and passing of motion data for you. If you manage CMMotionManager yourself, you must set the gyro update interval to .01 ([CMMotionManager setAccelerometerUpdateInterval:.01]).
 */
- (void) receiveGyroData:(CMGyroData *)gyroData;

/** Call this before starting sensor fusion. Wait for the completion block to execute and check the license status before starting sensor fusion. For evaluation licenses, this must be called every time you start sensor fusion. Internet connection required. */
- (void) validateLicense:(NSString*)apiKey withCompletionBlock:(void (^)(int licenseType, int licenseStatus))completionBlock withErrorBlock:(void (^)(NSError*))errorBlock;

/** 
 Represents the type of license validation error 
 */
typedef NS_ENUM(int, RCLicenseError) {
    /** 
     Unknown error. Currently not used. 
     */
    RCLicenseErrorUnknown = 0,
    /** The API key provided was nil or zero length */
    RCLicenseErrorApiKeyMissing = 1,
    /** We weren't able to get the app's bundle ID from the system */
    RCLicenseErrorBundleIdMissing = 2,
    /** We weren't able to get the identifier for vendor from the system */
    RCLicenseErrorVendorIdMissing = 3,
    /** The license server returned an empty response */
    RCLicenseErrorEmptyResponse = 4,
    /** Failed to deserialize the response from the license server */
    RCLicenseErrorDeserialization = 5,
    /** The license server returned invalid data */
    RCLicenseErrorInvalidResponse = 6,
    /** Failed to execute the HTTP request. See underlying error for details. */
    RCLicenseErrorHttpFailure = 7,
    /** We got an HTTP failure status from the license server. */
    RCLicenseErrorHttpError = 8
};
// for info on NS_ENUM, see http://nshipster.com/ns_enum-ns_options/

@end
