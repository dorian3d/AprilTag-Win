//
//  scaled_mask.cpp
//  RC3DK
//
//  Created by Eagle Jones on 5/22/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#include "scaled_mask.h"
#include <string.h>

scaled_mask::scaled_mask(int _width, int _height) {
    assert((((_width >> MASK_SHIFT) << MASK_SHIFT) == _width) && (((_height >> MASK_SHIFT) << MASK_SHIFT) == _height));
    scaled_width = _width >> MASK_SHIFT;
    scaled_height = _height >> MASK_SHIFT;
    mask = new uint8_t[scaled_width * scaled_height];
}

scaled_mask::~scaled_mask() { delete[] mask; }

void scaled_mask::clear(int fx, int fy)
{
    int x = fx >> MASK_SHIFT;
    int y = fy >> MASK_SHIFT;
    if(x < 0 || y < 0 || x >= scaled_width || y >= scaled_height) return;
    mask[x + y * scaled_width] = 0;
    if(y > 1) {
        //don't worry about horizontal overdraw as this just is the border on the previous row
        for(int i = 0; i < 3; ++i) mask[x-1+i + (y-1)*scaled_width] = 0;
        mask[x-1 + y*scaled_width] = 0;
    } else {
        //don't draw previous row, but need to check pixel to left
        if(x > 1) mask[x-1 + y * scaled_width] = 0;
    }
    if(y < scaled_height - 1) {
        for(int i = 0; i < 3; ++i) mask[x-1+i + (y+1)*scaled_width] = 0;
        mask[x+1 + y*scaled_width] = 0;
    } else {
        if(x < scaled_width - 1) mask[x+1 + y * scaled_width] = 0;
    }
}

void scaled_mask::initialize()
{
    //set up mask - leave a 1-block strip on border off
    //use 8 byte blocks
    memset(mask, 0, scaled_width);
    memset(mask + scaled_width, 1, (scaled_height - 2) * scaled_width);
    memset(mask + (scaled_height - 1) * scaled_width, 0, scaled_width);
    //vertical border
    for(int y = 1; y < scaled_height - 1; ++y) {
        mask[0 + y * scaled_width] = 0;
        mask[scaled_width-1 + y * scaled_width] = 0;
    }
}
