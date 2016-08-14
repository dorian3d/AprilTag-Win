//
//  sensor_fusion_queue.cpp
//
//  Created by Eagle Jones on 1/6/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#include "sensor_fusion_queue.h"
#include <cassert>

template<typename T, int size>
sensor_queue<T, size>::sensor_queue(std::mutex &mx, std::condition_variable &cnd, const bool &actv): period(0), mutex(mx), cond(cnd), active(actv), readpos(0), writepos(0), count(0)
{
}

template<typename T, int size>
void sensor_queue<T, size>::reset()
{
    period = std::chrono::duration<double, std::micro>(0);
    last_in = sensor_clock::time_point();
    last_out = sensor_clock::time_point();
    
    drop_full = 0;
    drop_late = 0;
    total_in = 0;
    total_out = 0;
    stats = stdev<1>();
#ifdef DEBUG
    hist = histogram{200};
#endif
    
    for(int i = 0; i < size; ++i)
    {
        storage[i] = T();
    }
    
    readpos = 0;
    writepos = 0;
    count = 0;
}

template<typename T, int size>
bool sensor_queue<T, size>::push(T&& x)
{
    std::unique_lock<std::mutex> lock(mutex);
    if(!active)
    {
        lock.unlock();
        return false;
    }
    
    sensor_clock::time_point time = x.timestamp;
#ifdef DEBUG
    assert(time >= last_in);
#endif
    if(last_in != sensor_clock::time_point())
    {
        sensor_clock::duration delta = time - last_in;
        stats.data(v<1>{(f_t)delta.count()});
#ifdef DEBUG
        hist.data((unsigned int)std::chrono::duration_cast<std::chrono::milliseconds>(delta).count());
#endif
        if(period == std::chrono::duration<double, std::micro>(0)) period = delta;
        else
        {
            const float alpha = .1f;
            period = alpha * delta + (1.f - alpha) * period;
        }
    }
    last_in = time;

    ++total_in;

    if(count == size) {
        pop(lock);
        ++drop_full;
    }

    storage[writepos] = std::move(x);
    writepos = (writepos + 1) % size;
    ++count;
    
    lock.unlock();
    cond.notify_one();
    return true;
}

template<typename T, int size>
sensor_clock::time_point sensor_queue<T, size>::get_next_time(const std::unique_lock<std::mutex> &lock, sensor_clock::time_point last_global_dispatched)
{
    while(count && (storage[readpos].timestamp < last_global_dispatched || storage[readpos].timestamp <= last_out)) {
        pop(lock);
        ++drop_late;
    }
    return count ? storage[readpos].timestamp : sensor_clock::time_point();
}

template<typename T, int size>
T sensor_queue<T, size>::pop(const std::unique_lock<std::mutex> &lock)
{
#ifdef DEBUG
    assert(count);
#endif

    last_out = storage[readpos].timestamp;
    --count;
    ++total_out;

    int oldpos = readpos;
    readpos = (readpos + 1) % size;
    return std::move(storage[oldpos]);
}

fusion_queue::fusion_queue(const std::function<void(image_gray8 &&)> &camera_func,
                           const std::function<void(image_depth16 &&)> &depth_func,
                           const std::function<void(accelerometer_data &&)> &accelerometer_func,
                           const std::function<void(gyro_data &&)> &gyro_func,
                           latency_strategy s,
                           sensor_clock::duration max_jitter):
                strategy(s),
                camera_receiver(camera_func),
                depth_receiver(depth_func),
                accel_receiver(accelerometer_func),
                gyro_receiver(gyro_func),
                camera_queue(mutex, cond, active),
                depth_queue(mutex, cond, active),
                accel_queue(mutex, cond, active),
                gyro_queue(mutex, cond, active),
                control_func(nullptr),
                active(false),
                wait_for_camera(true),
                singlethreaded(false),
                jitter(max_jitter)
{
}

void fusion_queue::reset()
{
    stop_immediately();
    wait_until_finished();
    thread = std::thread();

    accel_queue.reset();
    gyro_queue.reset();
    depth_queue.reset();
    camera_queue.reset();
    control_func = nullptr;
    active = false;
    last_dispatched = sensor_clock::time_point();
}

fusion_queue::~fusion_queue()
{
    stop_immediately();
    wait_until_finished();
}

std::string fusion_queue::get_stats()
{
    return "Camera: " + camera_queue.get_stats() + "Depth:" + depth_queue.get_stats() + "Accel: " + accel_queue.get_stats() + "Gyro: " + gyro_queue.get_stats();
}

void fusion_queue::receive_depth(image_depth16&& x)
{
    depth_queue.push(std::move(x));
    if(singlethreaded) dispatch_singlethread(false);
}

void fusion_queue::receive_camera(image_gray8&& x)
{
    camera_queue.push(std::move(x));
    if(singlethreaded) dispatch_singlethread(false);
}

void fusion_queue::receive_accelerometer(accelerometer_data&& x)
{
    accel_queue.push(std::move(x));
    if(singlethreaded) dispatch_singlethread(false);
}

void fusion_queue::receive_gyro(gyro_data&& x)
{
    gyro_queue.push(std::move(x));
    if(singlethreaded) dispatch_singlethread(false);
}

void fusion_queue::dispatch_sync(std::function<void()> fn)
{
    std::unique_lock<std::mutex> lock(mutex);
    fn();
    lock.unlock();
}

void fusion_queue::dispatch_async(std::function<void()> fn)
{
    std::unique_lock<std::mutex> lock(mutex);
    control_func = fn;
    lock.unlock();
    if(singlethreaded) dispatch_singlethread(false);
}

void fusion_queue::start_buffering()
{
    std::unique_lock<std::mutex> lock(mutex);
    active = true;
    lock.unlock();
}

void fusion_queue::start_async(bool expect_camera)
{
    wait_for_camera = expect_camera;
    if(!thread.joinable())
    {
        thread = std::thread(&fusion_queue::runloop, this);
    }
}

void fusion_queue::start_sync(bool expect_camera)
{
    wait_for_camera = expect_camera;
    if(!thread.joinable())
    {
        std::unique_lock<std::mutex> lock(mutex);
        thread = std::thread(&fusion_queue::runloop, this);
        while(!active)
        {
            cond.wait(lock);
        }
    }
}

void fusion_queue::start_singlethreaded(bool expect_camera)
{
    wait_for_camera = expect_camera;
    singlethreaded = true;
    active = true;
    dispatch_singlethread(false); //dispatch any waiting data in case we were buffering
}

void fusion_queue::stop_immediately()
{
    std::unique_lock<std::mutex> lock(mutex);
    active = false;
    lock.unlock();
    cond.notify_one();
}

void fusion_queue::stop_async()
{
    if(singlethreaded)
    {
        //flush any waiting data
        dispatch_singlethread(true);
#ifdef DEBUG
        std::cerr << get_stats();
        //std::cerr << gyro_queue.hist;
#endif
    }
    stop_immediately();
}

void fusion_queue::stop_sync()
{
    stop_async();
    if(!singlethreaded) wait_until_finished();
}

void fusion_queue::wait_until_finished()
{
    if(thread.joinable()) thread.join();
}

bool fusion_queue::run_control()
{
    if(!control_func) return false;
    control_func();
    control_func = nullptr;
    return true;
}

void fusion_queue::runloop()
{
    std::unique_lock<std::mutex> lock(mutex);
    active = true;
    //If we were launched synchronously, wake up the launcher
    lock.unlock();
    cond.notify_one();
    lock.lock();
    while(active)
    {
        while(active && !run_control() && !dispatch_next(lock, false))
        {
            cond.wait(lock);
        }
        while(run_control() || dispatch_next(lock, false)); //we need to be greedy, since we only get woken on new data arriving
    }
    //flush any remaining data
    while (dispatch_next(lock, true));
#ifdef DEBUG
    std::cerr << get_stats();
    //std::cerr << gyro_queue.hist;
#endif
    lock.unlock();
}

sensor_clock::time_point fusion_queue::global_latest_received() const
{
    if(depth_queue.last_in >= camera_queue.last_in && depth_queue.last_in >= gyro_queue.last_in && camera_queue.last_in >= accel_queue.last_in) return depth_queue.last_in;
    else if(camera_queue.last_in >= gyro_queue.last_in && camera_queue.last_in >= accel_queue.last_in) return camera_queue.last_in;
    else if(accel_queue.last_in >= gyro_queue.last_in) return accel_queue.last_in;
    return gyro_queue.last_in;
}

bool fusion_queue::ok_to_dispatch(sensor_clock::time_point time)
{
    // TODO: Note that we do not handle depth data here, so if it is
    // too far behind we may potentially drop it (by dispatching later
    // camera / imu samples first). This is even true in
    // ELIMINATE_DROPS, since we do not know we have a depth camera to
    // wait for
    if(strategy == latency_strategy::ELIMINATE_LATENCY) return true; //always dispatch if we are eliminating latency
    
    if(depth_queue.full() || camera_queue.full() || accel_queue.full() || gyro_queue.full()) return true;

    if(strategy == latency_strategy::ELIMINATE_DROPS)
    {
        if(camera_queue.empty() || accel_queue.empty() || gyro_queue.empty())
            return false;
        return true;
    }

    // if we are constructed with IMAGE_TRIGGER, but started without
    // wait_for_camera, we may not get camera data (e.g. when
    // calibrating) and should switch to MINIMIZE_DROPS
    latency_strategy dispatch_strategy = strategy;
    if(dispatch_strategy == latency_strategy::IMAGE_TRIGGER && !wait_for_camera)
        dispatch_strategy = latency_strategy::MINIMIZE_DROPS;

    if(dispatch_strategy == latency_strategy::IMAGE_TRIGGER)
    {
        if(camera_queue.empty()) return false;
        else return true;
    }

    bool camera_expected = camera_queue.last_out == sensor_clock::time_point() ||
                           time > camera_queue.last_out + camera_queue.period - std::chrono::milliseconds(1);
    bool accel_expected  = accel_queue.last_out == sensor_clock::time_point() ||
                           time > accel_queue.last_out + accel_queue.period - std::chrono::milliseconds(1);
    bool gyro_expected   = gyro_queue.last_out == sensor_clock::time_point() ||
                           time > gyro_queue.last_out + gyro_queue.period - std::chrono::milliseconds(1);

    bool camera_late = sensor_clock::time_point() != camera_queue.last_out &&
                       global_latest_received()   >= camera_queue.last_out + camera_queue.period + jitter;
    bool accel_late  = sensor_clock::time_point() !=  accel_queue.last_out &&
                       global_latest_received()   >=  accel_queue.last_out +  accel_queue.period + jitter;
    bool gyro_late   = sensor_clock::time_point() !=   gyro_queue.last_out &&
                       global_latest_received()   >=   gyro_queue.last_out +   gyro_queue.period + jitter;

    // The idea of this is to start with the assumption that we are ok
    // to dispatch and invalidate it based on queue status and the
    // latency_strategy.
    //
    // If all queues have something in them we are always safe to
    // dispatch the oldest. If a queue is empty, we will usually only
    // dispatch if the expected item in that queue is late.
    //
    // In MINIMIZE_DROPS we wait for expected data to arrive, but can
    // drop data if other data arrives with a timestamp that is
    // earlier than its _expected time (see above).
    //
    // In BALANCED mode we try to minimize drops of camera packets by
    // waiting for a camera packet before dispatching IMU data. We can
    // still drop IMU data if it comes in later than the camera packet
    //
    // In all other modes, we wait for a sample to show up in a queue
    // unless we have seen data that suggests it is "late" (last_out +
    // period + jitter)
    if(dispatch_strategy == latency_strategy::BALANCED) {
        if(wait_for_camera && camera_expected && camera_queue.empty())
            return false;

        if(accel_expected && accel_queue.empty()) {
            if(wait_for_camera && camera_queue.empty())
                return false;
            if(!accel_late)
                return false;
        }

        if(gyro_expected && gyro_queue.empty()) {
            if(wait_for_camera && camera_queue.empty())
                return false;
            if(!gyro_late)
                return false;
        }
        return true;
    }
    else if (dispatch_strategy == latency_strategy::MINIMIZE_LATENCY) {
        if(wait_for_camera && camera_expected && camera_queue.empty()  && !camera_late)
            return false;
        if(accel_expected && accel_queue.empty() && !accel_late)
            return false;
        if(gyro_expected && gyro_queue.empty() && !gyro_late)
            return false;

        return true;
    }
    else { // if(dispatch_strategy == latency_strategy::MINIMIZE_DROPS) {
        if(wait_for_camera && camera_expected && camera_queue.empty())
            return false;
        if(accel_expected && accel_queue.empty())
            return false;
        if(gyro_expected && gyro_queue.empty())
            return false;

        return true;
    }

    return true;
}

bool fusion_queue::dispatch_next(std::unique_lock<std::mutex> &lock, bool force)
{
    sensor_clock::time_point camera_time = camera_queue.get_next_time(lock, last_dispatched);
    sensor_clock::time_point depth_time = depth_queue.get_next_time(lock, last_dispatched);
    sensor_clock::time_point accel_time = accel_queue.get_next_time(lock, last_dispatched);
    sensor_clock::time_point gyro_time = gyro_queue.get_next_time(lock, last_dispatched);
    
    if(!depth_queue.empty() && (camera_queue.empty() || depth_time <= camera_time) && (accel_queue.empty() || depth_time <= accel_time) && (gyro_queue.empty() || depth_time <= gyro_time))
    {
        if(!force && !ok_to_dispatch(depth_time)) return false;

        image_depth16 data = depth_queue.pop(lock);
#ifdef DEBUG
        assert(data.timestamp >= last_dispatched);
#endif
        last_dispatched = data.timestamp;
        lock.unlock();
        depth_receiver(std::move(data));
        lock.lock();
    }
    else if(!camera_queue.empty() && (accel_queue.empty() || camera_time <= accel_time) && (gyro_queue.empty() || camera_time <= gyro_time))
    {
        if(!force && !ok_to_dispatch(camera_time)) return false;

        image_gray8 data = camera_queue.pop(lock);
#ifdef DEBUG
        assert(data.timestamp >= last_dispatched);
#endif
        last_dispatched = data.timestamp;
        lock.unlock();
        camera_receiver(std::move(data));
        
        /* In camera_receiver:
         receiver.process_camera(data);
         sensor_fusion_data = receiver.get_sensor_fusion_data();
         async([] {
            caller.sensor_fusion_did_update(data.image_handle, sensor_fusion_data);
            if(!keeping_image) caller.release_image_handle(data.image_handle);
         });
        
         this should be dispatched asynchronously
         implement it delegate style
         The call to release the platform specific image handle should be independent of the callback for two cases:
         1. Processed frame, but didn't update state (dropped)
         2. Need to hang on the the frame in order to do something (for example, run detector)
         */
        lock.lock();
    }
    else if(!accel_queue.empty() && (gyro_queue.empty() || accel_time <= gyro_time))
    {
        if(!force && !ok_to_dispatch(accel_time)) return false;
        
        accelerometer_data data = accel_queue.pop(lock);
#ifdef DEBUG
        assert(data.timestamp >= last_dispatched);
#endif
        last_dispatched = data.timestamp;
        lock.unlock();
        accel_receiver(std::move(data));
        lock.lock();
    }
    else if(!gyro_queue.empty())
    {
        if(!force && !ok_to_dispatch(gyro_time)) return false;

        gyro_data data = gyro_queue.pop(lock);
#ifdef DEBUG
        assert(data.timestamp >= last_dispatched);
#endif
        last_dispatched = data.timestamp;
        lock.unlock();
        gyro_receiver(std::move(data));
        lock.lock();
    }
    else return false;
    return true;
}

void fusion_queue::dispatch_singlethread(bool force)
{
    std::unique_lock<std::mutex> lock(mutex);
    while(dispatch_next(lock, force)); //always be greedy - could have multiple pieces of data buffered
    lock.unlock();
}

