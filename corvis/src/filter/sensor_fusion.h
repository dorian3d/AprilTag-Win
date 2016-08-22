//
//  sensor_fusion.h
//  RC3DK
//
//  Created by Eagle Jones on 3/2/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef __RC3DK__sensor_fusion__
#define __RC3DK__sensor_fusion__

#include <string>
#include <vector>
#include "../numerics/transformation.h"
#include "../cor/platform/sensor_clock.h"
#include "../cor/sensor_data.h"
#include "../cor/sensor_fusion_queue.h"
#include "RCSensorFusionInternals.h"
#include "camera_control_interface.h"
#include "filter.h"
#include "sensor.h"

class sensor_fusion
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    struct status
    {
        RCSensorFusionRunState run_state{ RCSensorFusionRunStateInactive };
        RCSensorFusionErrorCode error{ RCSensorFusionErrorCodeNone };
        RCSensorFusionConfidence confidence{ RCSensorFusionConfidenceNone };
        f_t progress{ 0 };
        bool operator==(const struct status & other) { return run_state == other.run_state && error == other.error && confidence == other.confidence && progress == other.progress; }
    };
    
    struct feature_point
    {
        uint64_t id;
        f_t x, y;
        f_t original_depth;
        f_t stdev;
        f_t worldx, worldy, worldz;
        bool initialized;
    };
    
    std::function<void(rc_SensorType type, rc_Sensor id)> data_callback;
    std::function<void(status)> status_callback;
    
    sensor_fusion(fusion_queue::latency_strategy strategy);
    
    /** Sets the current location of the device.
     
     The device's current location (including altitude) is used to account for differences in gravity across the earth. If location is unavailable, results may be less accurate. This should be called before starting sensor fusion or calibration.
     
     @param latitude_degrees Latitude in degrees, with positive north and negative south.
     @param longitude_degrees Longitude in degrees, with positive east and negative west.
     @param altitude_meters Altitude above sea level, in meters.
     */
    void set_location(double latitude_degrees, double longitude_degrees, double altitude_meters);
    
    //These may all run async
    
    void start_calibration(bool threaded);
    
    //void start_inertial_only();
    
    /** Prepares the object to receive video and inertial data, and starts sensor fusion updates.
     
     This method should be called when you are ready to begin receiving sensor fusion updates and the user is aware to point the camera at an appropriate visual scene. After you call this method you should immediately begin passing video, accelerometer, and gyro data using receive_image, receive_accelerometer, and receive_gyro respectively. Full processing will not begin until the user has held the device steady for a brief initialization period (this occurs concurrently with focusing the camera). The device does not need to be perfectly still; minor shake from the device being held in hand is acceptable. If the user moves during this time, the timer will start again. The progress of this timer is provided as a float between 0 and 1 in sensor_fusion_status.progress.
     
     @param device The camera_control_interface to be used for capture. This function will lock the focus on the camera device (if the device is capable of focusing) before starting video processing. No other modifications to the camera settings are made.
     */
    void start(bool threaded);
    
    /** Prepares the object to receive video and inertial data, and starts sensor fusion updates.
     
     This method may be called when you are ready to begin receiving sensor fusion updates and the user is aware to point the camera at an appropriate visual scene. After you call this method you should immediately begin passing video, accelerometer, and gyro data using receive_image, receive_accelerometer, and receive_gyro respectively. It is strongly recommended to call start_sensor_fusion rather than this function, unless it is absolutely impossible for the device to be held steady while initializing (for example, in a moving vehicle). There will be a delay after calling this function before video processing begins, while the camera is focused and sensor fusion is initialized.
     
     @param device The camera_control_interface to be used for capture. This function will lock the focus on the camera device (if the device is capable of focusing) before starting video processing. No other modifications to the camera settings are made.
     @note It is strongly recommended to call start_sensor_fusion rather than this function
     */
    void start_unstable(bool threaded);

    void pause_and_reset_position();
    void unpause();
    
    void start_buffering();
    void start_offline();
    bool started();

    /** Stops the processing of video and inertial data. */
    void stop();

    /**
     Resets system, clearing all history and state, and sets initial pose and time.
     System will be stopped until one of the start_ functions is called.
     */
    void reset(sensor_clock::time_point time);
    
    /** Once sensor fusion has started, video frames should be passed
     in as they are received from the camera. The camera is implied
     by the image format. image_gray8 is a an rgb or fisheye camera
     in grayscale, and image_depth16 is a depth image.
     @param data The image data
     */
    void receive_image(image_gray8 &&data);
    void receive_image(image_depth16 &&data);
    
    /** Once sensor fusion has started, acceleration data should be passed in as it's received from the accelerometer.
     @param x Acceleration along the x axis, in m/s^2
     @param y Acceleration along the y axis, in m/s^2
     @param z Acceleration along the z axis, in m/s^2
     */
    void receive_accelerometer(accelerometer_data &&data);
    
    /** Once sensor fusion has started, angular velocity data should be passed in as it's received from the gyro.
     @param x Angular velocity around the x axis, in rad/s
     @param y Angular velocity around the y axis, in rad/s
     @param z Angular velocity around the z axis, in rad/s
     */
    void receive_gyro(gyro_data &&data);
    
    void start_mapping();
    void stop_mapping();

    void save_map(void (*write)(void *handle, const void *buffer, size_t length), void *handle);

    bool load_map(size_t (*read)(void *handle, void *buffer, size_t length), void *handle);


    //*************Not yet implemented:
    
    
    /** Determine if valid saved calibration data exists from a previous run.
     
     @return If false, it is strongly recommended to perform the calibration procedure by calling start_calibration.
     @note In some cases, calibration data may become invalid or go out of date, in which case this will return false even if it previously returned true. It is recommended to check hasCalibrationData before each use, even if calibration has previously been run successfully.
     */
//    bool has_calibration_data();

    /** Sets the 3DK license key. Call this once before starting sensor fusion. In most cases, this should be done when your app starts.
     
     @param key A 30 character string. Obtain a license key by contacting RealityCap.
     */
//    void set_license_key(const std::string &key);
    
    
    
    /** Starts to search for a QR code and once detected reports future transformations relative to the observed QR code.
     
     They sensor fusion system will attempt to detect a QR code until one is found or stop_qr_detection is called. Once the code has been detected, sensor_fusion_data.origin_qr_code will be set to the payload of the QR code, and future instances of sensor_fusion_data.transformation and sensor_fusion_data.camera_transformation will be modified with the origin fixed to the center of the QR code. If align_gravity is false, then positive x will point toward the canonical "right" side of the QR code, positive y will point toward the canonical "top" of the QR code, and positive z will point out of the plane of the QR code. If alignGravity is true (recommended), the coordinates will be rotated so that the positive z axis points opposite to gravity.
     
     [ ]  ^+y [ ]
     |
     o--->+x
     
     [ ]
     
     @param data The expected payload of the QR code. If nullptr is passed, the first detected QR code will be used.
     @param size The size of the QR code (width = height) in meters.
     @param align_gravity If true (recommended), the z axis will be aligned with gravity; if false the z axis will be perpindicular to the QR code.
     */
    //void start_qr_detection(const std::string& data, float size, bool align_gravity);
    
    /** Stops searching for QR codes.
     */
    //void stop_qr_detection();
    
    std::string get_timing_stats() { return queue->get_stats(); };

    //public for now
    filter sfm;

    //Gets the current transformation, moving from filter-internal to external coordinates
    //Adjusts for camera vs accel centered and QR offset
    transformation get_transformation() const;
    void set_transformation(const transformation &pose_m);

    std::vector<feature_point> get_features() const;
    
private:
    friend class replay; //Allow replay to access queue directly so it can send the obsolete start measuring signal, which we don't expose elsewhere
    RCSensorFusionErrorCode get_error();
    void update_status();
    void update_data(rc_SensorType type, rc_Sensor id);
    void update_image_data(image_gray8 &&data);
    std::atomic<bool> isProcessingVideo, isSensorFusionRunning, processingVideoRequested;
    std::unique_ptr<fusion_queue> queue;
    status last_status;
    bool threaded;
    
    void flush_and_reset();

    bool buffering = true;
};

#endif /* defined(__RC3DK__sensor_fusion__) */
