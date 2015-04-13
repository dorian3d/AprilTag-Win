#ifndef __UTILS_H__
#define __UTILS_H__

#include "quaternion.h"
#include "vec4.h"

static inline quaternion initial_orientation_from_gravity_facing(const v4 gravity, const v4 facing)
{
    v4 z(0., 0., 1., 0.);
    quaternion q = rotation_between_two_vectors(gravity, z);
    v4 zt = quaternion_rotate(q, -z/*camera in accelerometer frame*/);
    //project the transformed z vector onto the x-y plane
    zt[2] = 0.;
    f_t len = zt.norm();
    if(len > 1.e-6) // otherwise we're looking straight up or down, so don't make any changes
        {
            v4 a = cross(zt / len, facing.normalized());
            f_t d =  (zt / len).dot(facing.normalized()), s = sqrt(2*(1 + d));
            quaternion dq = a.norm() > F_T_EPS ? normalize(quaternion(s/2, a[0]/s, a[1]/s, a[2]/s))
                                              : d > 0 ? quaternion() : quaternion(0,0,0,1);
            q = dq * q;
        }
    return q;
}

static inline quaternion initial_orientation_from_gravity(const v4 gravity) {
    return initial_orientation_from_gravity_facing(gravity, v4(0,1,0,0)); // camera faces +y
}

#endif//__UTILS_H__
