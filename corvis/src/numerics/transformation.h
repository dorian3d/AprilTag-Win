//
//  transformation.h
//  RC3DK
//
//  Created by Brian Fulkerson
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef __TRANSFORMATION_H
#define __TRANSFORMATION_H

#include "vec4.h"
#include "rotation_vector.h"
#include "quaternion.h"

class transformation {
    public:
        transformation(): Q(quaternion()), T(v4(0,0,0,0)) {};
        transformation(const quaternion & Q_, const v4 & T_) : Q(Q_), T(T_) {};
        transformation(const rotation_vector & v, const v4 & T_) : T(T_) { Q = to_quaternion(v); };
        transformation(const m4 & m, const v4 & T_) : T(T_) { Q = to_quaternion(m); };

        quaternion Q;
        v4 T;
};

static inline std::ostream& operator<<(std::ostream &stream, const transformation &t)
{
    return stream << t.Q << ", " << t.T; 
}

static inline bool operator==(const transformation & a, const transformation &b)
{
    return a.T == b.T && a.Q == b.Q;
}

static inline transformation invert(const transformation & t)
{
    return transformation(conjugate(t.Q), -quaternion_rotate(conjugate(t.Q), t.T));
}

static inline v4 transformation_apply(const transformation & t, const v4 & apply_to)
{
    return quaternion_rotate(t.Q, apply_to) + t.T;
}

static inline transformation compose(const transformation & t1, const transformation & t2)
{
    return transformation(t1.Q * t2.Q, t1.T + quaternion_rotate(t1.Q, t2.T));
}

#endif
