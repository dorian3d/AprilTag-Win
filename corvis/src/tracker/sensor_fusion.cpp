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
    return sfm.origin*sfm.s.get_transformation();
}

void sensor_fusion::set_transformation(const transformation &pose_m)
{
    sfm.origin_set = true;
    sfm.origin = pose_m*invert(sfm.s.get_transformation());
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
        sfm.log->error("Numerical error; filter reset.");
        transformation last_transform = get_transformation();
        filter_initialize(&sfm);
        filter_set_origin(&sfm, last_transform, true);
        filter_start_dynamic(&sfm);
    }
    else if(last_status.run_state == RCSensorFusionRunStateStaticCalibration && s.run_state == RCSensorFusionRunStateInactive && s.error == RCSensorFusionErrorCodeNone)
    {
        isSensorFusionRunning = false;
        //TODO: save calibration
    }
    
    if((s.error == RCSensorFusionErrorCodeVision && s.run_state != RCSensorFusionRunStateRunning)) {
    }

    if(status_callback)
        status_callback(s);

    last_status = s;
}

void sensor_fusion::update_data(const sensor_data * data)
{
    if(data_callback)
        data_callback(data);
}

sensor_fusion::sensor_fusion(fusion_queue::latency_strategy strategy)
    : queue([this](sensor_data &&data) { queue_receive_data(std::move(data)); },
            strategy, std::chrono::milliseconds(500)),
      isProcessingVideo(false),
      isSensorFusionRunning(false)
{
}

void sensor_fusion::queue_receive_data(sensor_data &&data)
{
    switch(data.type) {
        case rc_SENSOR_TYPE_IMAGE: {
            bool docallback = true;
            if(isProcessingVideo)
                docallback = filter_image_measurement(&sfm, data);
            else
                //We're not yet processing video, but we do want to send updates for the video preview. Make sure that rotation is initialized.
                docallback = sfm.s.orientation_initialized;

            if (isProcessingVideo && fast_path) {
                auto start = std::chrono::steady_clock::now();
                sfm.catchup->state.copy_from(sfm.s);
                std::unique_lock<std::recursive_mutex> mini_lock(sfm.mini_mutex);
                // hold the mini_mutex while we manipulate the mini
                // state *and* while we manipulate the queue during
                // catchup so that dispatch_buffered is sure to notice
                // any new data we get while we are doing the filter
                // updates on the catchup state
                queue.dispatch_buffered([this,&mini_lock](sensor_data &data) {
                        mini_lock.unlock();
                        switch(data.type) {
                        case rc_SENSOR_TYPE_ACCELEROMETER: filter_mini_accelerometer_measurement(&sfm, sfm.catchup->observations, sfm.catchup->state, data); break;
                        case rc_SENSOR_TYPE_GYROSCOPE:     filter_mini_gyroscope_measurement(&sfm, sfm.catchup->observations, sfm.catchup->state, data); break;
                        default: break;
                        }
                        mini_lock.lock();
                    });
                auto stop = std::chrono::steady_clock::now();
                queue.catchup_stats.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
                std::swap(sfm.mini, sfm.catchup);
            }

            update_status();
            if(docallback)
                update_data(&data);

            if (data.id < sfm.s.cameras.children.size())
                if(sfm.s.cameras.children[data.id]->detecting_group)
                    sfm.s.cameras.children[data.id]->detection_future = std::async(threaded ? std::launch::async : std::launch::deferred,
                        [space=sfm.s.cameras.children[data.id]->detecting_space, this] (struct filter *f, const sensor_data &data) -> const std::vector<tracker::point> & {
                            auto start = std::chrono::steady_clock::now();
                            const std::vector<tracker::point> & res = filter_detect(&sfm, std::move(data), space);
                            auto stop = std::chrono::steady_clock::now();
                            queue.stats.find(data.global_id())->second.bg.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
                            return res;
                        }, &sfm, std::move(data));
        } break;

        case rc_SENSOR_TYPE_DEPTH: {
            update_status();
            if (filter_depth_measurement(&sfm, data))
                update_data(&data);
        } break;

        case rc_SENSOR_TYPE_ACCELEROMETER: {
            if(!isSensorFusionRunning) return;
            update_status();
            if (filter_accelerometer_measurement(&sfm, data))
                update_data(&data);
        } break;

        case rc_SENSOR_TYPE_GYROSCOPE: {
            update_status();
            if (filter_gyroscope_measurement(&sfm, data))
                update_data(&data);
        } break;
    }
}

void sensor_fusion::queue_receive_data_fast(sensor_data &data)
{
    if(!isSensorFusionRunning || sfm.run_state != RCSensorFusionRunStateRunning || !fast_path) return;
    data.path = rc_DATA_PATH_FAST;
    switch(data.type) {
        case rc_SENSOR_TYPE_ACCELEROMETER: {
            auto start = std::chrono::steady_clock::now();
            if(filter_mini_accelerometer_measurement(&sfm, sfm.mini->observations, sfm.mini->state, data))
                update_data(&data);
            auto stop = std::chrono::steady_clock::now();
            queue.stats.find(data.global_id())->second.bg.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
        } break;

        case rc_SENSOR_TYPE_GYROSCOPE: {
            auto start = std::chrono::steady_clock::now();
            if(filter_mini_gyroscope_measurement(&sfm, sfm.mini->observations, sfm.mini->state, data))
                update_data(&data);
            auto stop = std::chrono::steady_clock::now();
            queue.stats.find(data.global_id())->second.bg.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
        } break;
        default:
            break;
    }
    data.path = rc_DATA_PATH_SLOW;
}

void sensor_fusion::set_location(double latitude_degrees, double longitude_degrees, double altitude_meters)
{
    queue.dispatch_async([this, latitude_degrees, altitude_meters]{
        filter_compute_gravity(&sfm, latitude_degrees, altitude_meters);
    });
}

void sensor_fusion::start_calibration(bool thread)
{
    threaded = thread;
    buffering = false;
    fast_path = false;
    isSensorFusionRunning = true;
    isProcessingVideo = false;
    filter_initialize(&sfm);
    filter_start_static_calibration(&sfm);
    queue.start(threaded);
}

void sensor_fusion::start(bool thread)
{
    threaded = thread;
    buffering = false;
    fast_path = false;
    isSensorFusionRunning = true;
    isProcessingVideo = true;
    filter_initialize(&sfm);
    filter_start_hold_steady(&sfm);
    queue.start(threaded);
}

void sensor_fusion::start_unstable(bool thread, bool fast_path_)
{
    threaded = thread;
    buffering = false;
    fast_path = fast_path_;
    isSensorFusionRunning = true;
    isProcessingVideo = true;
    filter_initialize(&sfm);
    filter_start_dynamic(&sfm);
    queue.start(thread);
}

void sensor_fusion::pause_and_reset_position()
{
    isProcessingVideo = false;
    queue.dispatch_async([this]() { filter_start_inertial_only(&sfm); });
}

void sensor_fusion::unpause()
{
    isProcessingVideo = true;
    queue.dispatch_async([this]() { filter_start_dynamic(&sfm); });
}

void sensor_fusion::start_buffering()
{
    buffering = true;
    queue.start_buffering(std::chrono::milliseconds(200));
}

void sensor_fusion::start_offline(bool fast_path_)
{
    threaded = false;
    buffering = false;
    fast_path = fast_path_;
    filter_initialize(&sfm);
    filter_start_dynamic(&sfm);
    isSensorFusionRunning = true;
    isProcessingVideo = true;
    queue.start(false);
}

bool sensor_fusion::started()
{
    return isSensorFusionRunning;
}

void sensor_fusion::stop()
{
    queue.stop();
    filter_deinitialize(&sfm);
    isSensorFusionRunning = false;
    isProcessingVideo = false;
}

void sensor_fusion::flush_and_reset()
{
    stop();
    queue.reset();
    filter_initialize(&sfm);
}

void sensor_fusion::reset(sensor_clock::time_point time)
{
    flush_and_reset();
    sfm.last_time = time;
    sfm.s.time_update(time); //This initial time update doesn't actually do anything - just sets current time, but it will cause the first measurement to run a time_update relative to this
    sfm.origin_set = false;
}

void sensor_fusion::start_mapping()
{
    if (!sfm.map)
        sfm.map = std::make_unique<mapper>();
    sfm.map->reset();
}

void sensor_fusion::stop_mapping()
{
    sfm.map = nullptr;
}

void sensor_fusion::save_map(void (*write)(void *handle, const void *buffer, size_t length), void *handle)
{
    std::string json;
    if(sfm.map && sfm.map->serialize(json))
        write(handle, json.c_str(), json.length());
}

bool sensor_fusion::load_map(size_t (*read)(void *handle, void *buffer, size_t length), void *handle)
{
    if(!sfm.map)
        return false;

    std::string json;
    char buffer[1024];
    size_t bytes_read;
    while((bytes_read = read(handle, buffer, 1024)) != 0) {
        json.append(buffer, bytes_read);
    }

    return mapper::deserialize(json, *sfm.map);
}

void sensor_fusion::receive_data(sensor_data && data)
{
    std::unique_lock<std::recursive_mutex> mini_lock(sfm.mini_mutex);
    // hold the mini_mutex while we manipulate the mini state *and*
    // while we push data onto the queue so that catchup either
    // updates the mini state before we do or notices that we pushed
    // new data in.
    queue_receive_data_fast(data);
    queue.receive_sensor_data(std::move(data));
}

std::string sensor_fusion::get_timing_stats()
{
     return queue.get_stats() + filter_get_stats(&sfm);
}

