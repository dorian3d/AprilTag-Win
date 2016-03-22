//
//  sensor_fusion.cpp
//  RC3DK
//
//  Created by Eagle Jones on 3/2/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include <future>
#include "sensor_fusion.h"
#include "filter.h"

transformation sensor_fusion::get_transformation() const
{
    transformation filter_transform(sfm.s.Q.v, sfm.s.T.v);
    return compose(sfm.origin, compose(sfm.s.loop_offset, filter_transform));
}

void sensor_fusion::set_transformation(const transformation &pose_m)
{
    sfm.origin = compose(pose_m, invert(transformation(sfm.s.Q.v, sfm.s.T.v)));
}

v4 sensor_fusion::filter_to_external_position(const v4& x) const
{
    return transformation_apply(compose(sfm.origin, sfm.s.loop_offset), x);
}

RCSensorFusionErrorCode sensor_fusion::get_error()
{
    RCSensorFusionErrorCode error = RCSensorFusionErrorCodeNone;
    if(sfm.numeric_failed) error = RCSensorFusionErrorCodeOther;
    else if(sfm.speed_failed) error = RCSensorFusionErrorCodeTooFast;
    else if(sfm.detector_failed) error = RCSensorFusionErrorCodeVision;
    return error;
}

void sensor_fusion::update_status()
{
    status s;
    //Updates happen synchronously in the calling (filter) thread
    s.error = get_error();
    s.progress = filter_converged(&sfm);
    s.run_state = sfm.run_state;
    
    s.confidence = RCSensorFusionConfidenceNone;
    if(s.run_state == RCSensorFusionRunStateRunning)
    {
        if(s.error == RCSensorFusionErrorCodeVision)
        {
            s.confidence = RCSensorFusionConfidenceLow;
        }
        else if(sfm.has_converged)
        {
            s.confidence = RCSensorFusionConfidenceHigh;
        }
        else
        {
            s.confidence = RCSensorFusionConfidenceMedium;
        }
    }
    if(s == last_status) return;
    
    // queue actions related to failures before queuing callbacks to the sdk client.
    if(s.error == RCSensorFusionErrorCodeOther)
    {
        //Sensor fusion has already been reset by get_error, but it could have gotten random data inbetween, so do full reset
        if(threaded) std::async(std::launch::async, [this] { flush_and_reset(); });
        else flush_and_reset();
    }
    else if(last_status.run_state == RCSensorFusionRunStateStaticCalibration && s.run_state == RCSensorFusionRunStateInactive && s.error == RCSensorFusionErrorCodeNone)
    {
        isSensorFusionRunning = false;
        //TODO: save calibration
    }
    
    if((s.error == RCSensorFusionErrorCodeVision && s.run_state != RCSensorFusionRunStateRunning)) {
        //refocus if either we tried to detect and failed, or if we've recently moved during initialization
        isProcessingVideo = false;
        processingVideoRequested = true;
        
        // Request a refocus
        //TODO: is it possible for *this to become invalid before this is called?
        sfm.camera_control.focus_once_and_lock([this](uint64_t timestamp, float position)
                                               {
                                                   if(processingVideoRequested && !isProcessingVideo) {
                                                       isProcessingVideo = true;
                                                       processingVideoRequested = false;
                                                   }
                                               });
    }
    
    if(status_callback) {
        if(threaded) std::async(std::launch::async, status_callback, s);
        else status_callback(s);
    }

    last_status = s;
}

std::vector<sensor_fusion::feature_point> sensor_fusion::get_features() const
{
    std::vector<feature_point> features;
    features.reserve(sfm.s.features.size());
    for(auto i: sfm.s.features)
    {
        if(i->is_valid()) {
            feature_point p;
            p.id = i->id;
            p.x = i->current[0];
            p.y = i->current[1];
            p.original_depth = i->v.depth();
            p.stdev = i->v.stdev_meters(sqrt(i->variance()));
            v4 ext_pos = filter_to_external_position(i->world);
            p.worldx = ext_pos[0];
            p.worldy = ext_pos[1];
            p.worldz = ext_pos[2];
            p.initialized = i->is_initialized();
            features.push_back(p);
        }
    }
    return features;
}

void sensor_fusion::update_data(image_gray8 &&image)
{
    auto d = std::make_unique<data>();
    
    //perform these operations synchronously in the calling (filter) thread
    d->total_path_m = sfm.s.total_distance;
    camera_parameters cp;
    cp.fx = sfm.s.focal_length.v * sfm.s.image_height;
    cp.fy = sfm.s.focal_length.v * sfm.s.image_height;
    cp.cx = sfm.s.center_x.v * sfm.s.image_height + sfm.s.image_width / 2. - .5;
    cp.cy = sfm.s.center_y.v * sfm.s.image_height + sfm.s.image_height / 2. - .5;
    cp.skew = 0;
    cp.k1 = sfm.s.k1.v;
    cp.k2 = sfm.s.k2.v;
    cp.k3 = sfm.s.k3.v;
    d->camera_intrinsics = cp;
#ifdef ENABLE_QR
    if(sfm.qr.valid)
    {
        d->origin_qr_code = sfm.qr.data;
    }
#endif
    d->transform = get_transformation();
    d->time = sfm.last_time;

    d->features = get_features();

    if(camera_callback) {
        //if(threaded) std::async(std::launch::async, camera_callback, std::move(d), std::move(image));
        //else
        camera_callback(std::move(d), std::move(image));
    }
}

sensor_fusion::sensor_fusion(fusion_queue::latency_strategy strategy)
{
    isSensorFusionRunning = false;
    isProcessingVideo = false;
    auto cam_fn = [this](image_gray8 &&data)
    {
        bool docallback = true;
        if(!isSensorFusionRunning)
        {
        } else if(isProcessingVideo) {
            docallback = filter_image_measurement(&sfm, data);
            update_status();
            if(docallback) update_data(std::move(data));
        } else {
            //We're not yet processing video, but we do want to send updates for the video preview. Make sure that rotation is initialized.
            docallback =  sfm.gravity_init;
            update_status();
            if(docallback) update_data(std::move(data));
        }
    };

    auto depth_fn = [this](image_depth16 &&data)
    {
        if(!isSensorFusionRunning) return;
        //TODO: should I call update_status here?
        filter_depth_measurement(&sfm, data);
    };
    
    auto acc_fn = [this](accelerometer_data &&data)
    {
        if(!isSensorFusionRunning) return;
        filter_accelerometer_measurement(&sfm, data.accel_m__s2, data.timestamp);
        update_status();
    };
    
    auto gyr_fn = [this](gyro_data &&data)
    {
        if(!isSensorFusionRunning) return;
        filter_gyroscope_measurement(&sfm, data.angvel_rad__s, data.timestamp);
    };
    
    queue = std::make_unique<fusion_queue>(cam_fn, depth_fn, acc_fn, gyr_fn, strategy, std::chrono::microseconds(10000)); //Have to make jitter high - ipad air 2 accelerometer has high latency, we lose about 10% of samples with jitter at 8000
}

device_parameters sensor_fusion::get_device() const
{
    device_parameters cal = {};
    filter_get_device_parameters(&sfm, &cal);
    return cal;
}

void sensor_fusion::set_device(const device_parameters &dc)
{
    device = dc;
    filter_initialize(&sfm, &device);
}

void sensor_fusion::set_location(double latitude_degrees, double longitude_degrees, double altitude_meters)
{
    queue->dispatch_async([this, latitude_degrees, altitude_meters]{
        filter_compute_gravity(&sfm, latitude_degrees, altitude_meters);
    });
}

void sensor_fusion::start_calibration(bool thread)
{
    threaded = thread;
    isSensorFusionRunning = true;
    isProcessingVideo = false;
    filter_initialize(&sfm, &device);
    filter_start_static_calibration(&sfm);
    if(threaded) queue->start_async(false);
    else queue->start_singlethreaded(false);
}

void sensor_fusion::start(bool thread)
{
    threaded = thread;
    isSensorFusionRunning = true;
    isProcessingVideo = true;
    filter_initialize(&sfm, &device);
    filter_start_hold_steady(&sfm);
    if(threaded) queue->start_async(true);
    else queue->start_singlethreaded(true);
}

void sensor_fusion::start_unstable(bool thread)
{
    threaded = thread;
    isSensorFusionRunning = true;
    isProcessingVideo = true;
    filter_initialize(&sfm, &device);
    filter_start_dynamic(&sfm);
    if(threaded) queue->start_async(true);
    else queue->start_singlethreaded(true);
}

void sensor_fusion::pause_and_reset_position()
{
    isProcessingVideo = false;
    queue->dispatch_async([this]() { filter_start_inertial_only(&sfm); });
}

void sensor_fusion::unpause()
{
    isProcessingVideo = true;
    queue->dispatch_async([this]() { filter_start_dynamic(&sfm); });
}

void sensor_fusion::start_buffering()
{
    queue->start_buffering();
}

void sensor_fusion::start_offline()
{
    threaded = false;
    sfm.ignore_lateness = true;
    // TODO: Note that we call filter initialize, and this can change
    // device_parameters (specifically a_bias_var and w_bias_var)
    filter_initialize(&sfm, &device);
    filter_start_dynamic(&sfm);
    isSensorFusionRunning = true;
    isProcessingVideo = true;
    queue->start_singlethreaded(true);
}

void sensor_fusion::stop()
{
    queue->stop_sync();
    isSensorFusionRunning = false;
    isProcessingVideo = false;
    processingVideoRequested = false;
}

void sensor_fusion::flush_and_reset()
{
    stop();
    queue->reset();
    filter_initialize(&sfm, &device);
    sfm.camera_control.focus_unlock();
    sfm.camera_control.release_platform_specific_object();
}

void sensor_fusion::reset(sensor_clock::time_point time, const transformation &initial_pose_m, bool origin_gravity_aligned)
{
    flush_and_reset();
    sfm.last_time = time;
    sfm.s.time_update(time); //This initial time update doesn't actually do anything - just sets current time, but it will cause the first measurement to run a time_update relative to this
    sfm.origin_gravity_aligned = origin_gravity_aligned;
    filter_set_origin(&sfm, initial_pose_m, false);
}

void sensor_fusion::attempt_relocalization()
{
    queue->dispatch_async([this]() { sfm.s.lost_factor = 1.; });
}

void sensor_fusion::start_mapping()
{
    sfm.s.map.reset();
    sfm.s.map_enabled = true;
}

void sensor_fusion::stop_mapping()
{
    sfm.s.map_enabled = false;
}

void sensor_fusion::save_map(void (*write)(void *handle, const void *buffer, size_t length), void *handle)
{
    std::string json;
    if(sfm.s.map_enabled && sfm.s.map.serialize(json)) {
        write(handle, json.c_str(), json.length());
    }
}

bool sensor_fusion::load_map(size_t (*read)(void *handle, void *buffer, size_t length), void *handle)
{
    if(!sfm.s.map_enabled) return false;

    std::string json;
    char buffer[1024];
    size_t bytes_read;
    while((bytes_read = read(handle, buffer, 1024)) != 0) {
        json.append(buffer, bytes_read);
    }
    return sfm.s.map.deserialize(json, sfm.s.map);
}

void sensor_fusion::receive_image(image_gray8 &&data)
{
    //Adjust image timestamps to be in middle of exposure period
    data.timestamp += data.exposure_time / 2;
    queue->receive_camera(std::move(data));
}

void sensor_fusion::receive_image(image_depth16 &&data)
{
    //TODO: Verify time adjustments here
    //Adjust image timestamps to be in middle of exposure period
    data.timestamp += data.exposure_time / 2;
    queue->receive_depth(std::move(data));
}

void sensor_fusion::receive_accelerometer(accelerometer_data &&data)
{
    queue->receive_accelerometer(std::move(data));
}

void sensor_fusion::receive_gyro(gyro_data &&data)
{
    queue->receive_gyro(std::move(data));
}

static sensor_clock::time_point last_log;

// TODO: call trigger_log when we update position if stream is enabled
// or if period has expired (the SOW also mentions a trigger condition
// I think?)
void sensor_fusion::trigger_log() const
{
    if(!log_function) return;

    last_log = sfm.last_time;

    transformation transform = get_transformation();
    
    m4 R = to_rotation_matrix(transform.Q);
    v4 T = transform.T;

    std::stringstream s(std::stringstream::out);
    s << sfm.last_time.time_since_epoch().count() << " " << " " <<
            R(0, 0) << " " << R(0, 1) << " " << R(0, 2) << " " << T(0) << " " <<
            R(1, 0) << " " << R(1, 1) << " " << R(1, 2) << " " << T(1) << " " <<
            R(2, 0) << " " << R(2, 1) << " " << R(2, 2) << " " << T(2);

    log_function(log_handle, s.str().c_str(), s.str().length());
}
