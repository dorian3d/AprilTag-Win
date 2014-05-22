//
//  scaled_mask.h
//  RC3DK
//
//  Created by Eagle Jones on 5/22/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef __RC3DK__scaled_mask__
#define __RC3DK__scaled_mask__

#include <assert.h>

//mask is 8x8 pixel blocks
#define MASK_SHIFT 3

class scaled_mask
{
public:
    void clear(int fx, int fy); //clears the mask at this location, indicating that a feature should not be detected there
    void initialize();
    inline bool test(int x, int y) const { return mask[(x >> MASK_SHIFT) + scaled_width * (y >> MASK_SHIFT)]; }
    scaled_mask(int _width, int _height);
    ~scaled_mask();
protected:
    int scaled_width, scaled_height;
    //TODO: should this be a bitfield instead of bytes?
    uint8_t *mask;
};

#endif /* defined(__RC3DK__scaled_mask__) */
