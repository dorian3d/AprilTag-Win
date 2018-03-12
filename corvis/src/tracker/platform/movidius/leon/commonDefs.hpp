/*
 * commonDefs.hpp
 *
 *  Created on: Nov 13, 2016
 *      Author: administrator
 */

#ifndef COMMON_COMMONDEFS_HPP_
#define COMMON_COMMONDEFS_HPP_

#define MAX_CORD 10000
#define PADDING 8
#define MAX_WIDTH 848
#define MAX_HEIGHT 800
#define MAX_PATCH_WIDTH 11/*2*fast_track_radius*/ + 2*PADDING

struct TrackingData {

	unsigned char *patch;
	float x_dx, y_dy, pred_x, pred_y;
};

#endif /* COMMON_COMMONDEFS_HPP_ */
