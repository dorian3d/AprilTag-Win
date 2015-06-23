//
//  capture.h
//
//  Created by Brian Fulkerson on 4/15/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef __RC3DK__capture__
#define __RC3DK__capture__

#include <iostream>
#include <fstream>
#include "../cor/sensor_data.h"
#include "../cor/packet.h"
#include <thread>
#include <atomic>
#include <mutex>

class capture
{
private:
    std::ofstream file;
    std::atomic<uint64_t> packets_written{0};
    std::atomic<uint64_t> bytes_written{0};
    std::atomic<bool> is_running{false};
    std::mutex write_lock;

    void write_packet(packet_t * p);
    void write_image_gray8(uint8_t * image, int width, int height, int stride, uint64_t timestamp);
    void write_accelerometer_data(const float data[3], uint64_t timestamp);
    void write_gyroscope_data(const float data[3], uint64_t timestamp);

public:
    bool start(const char *name);
    void stop();
    void receive_image_gray8(const image_gray8& x);
    void receive_camera(const camera_data& x);
    void receive_accelerometer(const accelerometer_data& x);
    void receive_gyro(const gyro_data& x);
    uint64_t get_bytes_written() { return bytes_written; }
    uint64_t get_packets_written() { return packets_written; }
};

#endif /* defined(__RC3DK__capture__) */
