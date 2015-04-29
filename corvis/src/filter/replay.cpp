//
//  replay.cpp
//
//  Created by Eagle Jones on 4/8/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "replay.h"

#include <string.h>
#include "device_parameters.h"
#include "../cor/packet.h"

bool replay::open(const char *name)
{
    file.open(name, ios::binary);
    if(file.bad())
    {
        cerr << "Couldn't open file " << name << " for reading.\n";
        return false;
    }
    file.seekg(0, ios::end);
    auto end = file.tellg();
    file.seekg (0, ios::beg);
    auto begin = file.tellg();
    size = end - begin;
    return true;
}

void replay::set_device(const char *name)
{
    corvis_device_parameters dc;
    if(get_parameters_for_device_name(name, &dc))
        cor_setup = std::make_unique<filter_setup>(&dc);
    else
        cerr << "Error: no device named " << name;
}

void replay::setup_filter()
{
    auto camf = [this](const camera_data &x) { filter_image_measurement(&cor_setup->sfm, x.image, x.width, x.height, x.stride, x.timestamp); };
    auto accf = [this](const accelerometer_data &x) { filter_accelerometer_measurement(&cor_setup->sfm, x.accel_m__s2, x.timestamp); };
    auto gyrf = [this](const gyro_data &x) { filter_gyroscope_measurement(&cor_setup->sfm, x.angvel_rad__s, x.timestamp); };

    queue = make_unique<fusion_queue>(camf, accf, gyrf, fusion_queue::latency_strategy::ELIMINATE_DROPS, std::chrono::microseconds(33000), std::chrono::microseconds(10000), std::chrono::microseconds(5000));
    queue->start_offline(true);
    cor_setup->sfm.ignore_lateness = true;
    filter_start_dynamic(&cor_setup->sfm);
}

void replay::start()
{
    is_running = true;
    path_length = 0;
    length = 0;
    packets_dispatched = 0;
    bytes_dispatched = 0;

    packet_header_t header;
    file.read((char *)&header, 16);
    if(file.bad() || file.eof()) is_running = false;

    auto first_timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
    auto start_time = sensor_clock::now();
    auto now = start_time;
    auto last_progress = now;
    auto realtime_offset = now - first_timestamp;
    if(!is_realtime)
        realtime_offset = std::chrono::microseconds(0);

    while (is_running) {
        while(is_paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        auto phandle = std::unique_ptr<void, void(*)(void *)>(malloc(header.bytes), free);
        auto packet = (packet_t *)phandle.get();
        packet->header = header;
        
        file.read((char *)packet->data, header.bytes - 16);
        if(file.bad() || file.eof())
        {
            is_running = false;
        }
        else
        {
            auto timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time)) + realtime_offset;
            now = sensor_clock::now();
            if(is_realtime && timestamp - now > std::chrono::microseconds(0))
                std::this_thread::sleep_for(timestamp - now);

            switch(header.type)
            {
                case packet_camera:
                {
                    int width, height;
                    char tmp[17];
                    memcpy(tmp, packet->data, 16);
                    tmp[16] = 0;
                    std::stringstream parse(tmp);
                    //pgm header is "P5 x y"
                    parse.ignore(3, ' ') >> width >> height;
                    camera_data d;
                    d.image = packet->data + 16;
                    d.width = width;
                    d.height = height;
                    d.stride = width;
                    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time+16667));
                    d.image_handle = std::move(phandle);
                    queue->receive_camera(std::move(d));
                    break;
                }
                case packet_accelerometer:
                {
                    accelerometer_data d;
                    d.accel_m__s2[0] = ((float *)packet->data)[0];
                    d.accel_m__s2[1] = ((float *)packet->data)[1];
                    d.accel_m__s2[2] = ((float *)packet->data)[2];
                    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    queue->receive_accelerometer(std::move(d));
                    break;
                }
                case packet_gyroscope:
                {
                    gyro_data d;
                    d.angvel_rad__s[0] = ((float *)packet->data)[0];
                    d.angvel_rad__s[1] = ((float *)packet->data)[1];
                    d.angvel_rad__s[2] = ((float *)packet->data)[2];
                    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    queue->receive_gyro(std::move(d));
                    break;
                }
                case packet_imu:
                {
                    accelerometer_data a;
                    auto imu = (packet_imu_t *)packet;
                    a.accel_m__s2[0] = imu->a[0];
                    a.accel_m__s2[1] = imu->a[1];
                    a.accel_m__s2[2] = imu->a[2];
                    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    queue->receive_accelerometer(std::move(a));
                    gyro_data g;
                    g.angvel_rad__s[0] = imu->w[0];
                    g.angvel_rad__s[1] = imu->w[1];
                    g.angvel_rad__s[2] = imu->w[2];
                    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    queue->receive_gyro(std::move(g));
                    break;
                }
                case packet_filter_control:
                {
                    if(header.user == 1)
                    {
                        //start measuring
                        queue->dispatch_sync([this] { filter_set_reference(&cor_setup->sfm); });
                    }
                }
            }
            queue->dispatch_offline(false);
            bytes_dispatched += header.bytes;
            packets_dispatched++;

            if(packet_callback)
                packet_callback(&cor_setup->sfm, (enum packet_type)header.type);

            now = sensor_clock::now();
            // Update progress at most at 30Hz or if we are almost done
            if(progress_callback &&
               (now - last_progress > std::chrono::milliseconds(33) ||
                1.*bytes_dispatched / size > 0.99))
            {
                last_progress = now;
                progress_callback(1.*bytes_dispatched / size);
            }
        }
        
        file.read((char *)&header, 16);
        if(file.bad() || file.eof()) is_running = false;
    }
    while(queue->dispatch_offline(true)) {}
    file.close();

    length = cor_setup->sfm.s.T.v.norm() * 100;
    path_length = cor_setup->sfm.s.total_distance * 100;
}

bool replay::configure_all(const char *filename, const char *devicename, bool realtime, std::function<void (float)> progress, std::function<void (const filter *, enum packet_type)> packet)
{
    if(!open(filename)) return false;
    set_device(devicename);
    setup_filter();
    is_realtime = realtime;
    progress_callback = progress;
    packet_callback = packet;
    return true;
}

void replay::stop()
{
    is_running = false;
}
