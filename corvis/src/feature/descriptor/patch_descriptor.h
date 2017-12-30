/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
Pedro Pinies, Lina Paz
*********************************************************************************/
#pragma once
#include "tracker.h"
#include "fast_constants.h"
#include "patch_constants.h"
#include <array>

class patch_descriptor
{
public:
    static const int L = full_patch_width * full_patch_width; // descriptor length
    static const int full_patch_size = full_patch_width;
    static const int half_patch_size = half_patch_width;
    static const int border_size = half_patch_size;
    static constexpr float max_track_distance = patch_max_track_distance;
    static constexpr float good_track_distance = patch_good_track_distance;

    std::array<unsigned char, L> descriptor;
    float mean{0}, variance{0};

    patch_descriptor(float x, float y, const tracker::image& image);
    patch_descriptor(const std::array<unsigned char, L> &d);
    static float distance(const patch_descriptor &a, const patch_descriptor &b);
    float distance(float x, float y, const tracker::image& image) const;
};
