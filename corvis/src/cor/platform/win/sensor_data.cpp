//
//  sensor_data.cpp
//  RC3DK
//
//  Created by Eagle Jones on 3/17/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "../sensor_data.h"
#include "pxcsensemanager.h"
#include "pxcmetadata.h"
#include <stdexcept>
#include <iostream>

typedef std::pair<PXCImage *, PXCImage::ImageData> handle_type;

camera_data::camera_data(void *h) : image_handle(new handle_type, [](void *h) { delete (handle_type *)h;  })
{
    auto handle = (handle_type *)image_handle.get();
    handle->first = (PXCImage *)h;
    auto result = handle->first->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_Y8, &handle->second);
    if (result != PXC_STATUS_NO_ERROR || !handle->second.planes[0]) throw std::runtime_error("PXCImage->AcquireAccess failed!"); 
    image = handle->second.planes[0];
    stride = handle->second.pitches[0];
    auto info = handle->first->QueryInfo();
    width = info.width;
    height = info.height;
    auto time = handle->first->QueryTimeStamp();
    std::cerr << "camera timestamp: " << time << "\n";
    //TODO: timestamp
    timestamp = sensor_clock::now();
    handle->first->AddRef();
}

camera_data::~camera_data()
{
    auto handle = (handle_type *)image_handle.get();
    if (handle)
    {
        auto res = handle->first->ReleaseAccess(&handle->second);
        handle->first->Release();
    }
}
