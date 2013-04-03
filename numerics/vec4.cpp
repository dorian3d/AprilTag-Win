// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include "vec4.h"
#include <assert.h>
#include "float.h"

#ifdef F_T_IS_DOUBLE
#define EPS DBL_EPSILON
#else
#define EPS FLT_EPSILON
#endif

const m4 m4_identity = { {
        v4(1, 0, 0, 0),
        v4(0, 1, 0, 0),
        v4(0, 0, 1, 0),
        v4(0, 0, 0, 1)
    }
};

const static m4v4 dV_dv = { {
        {{v4(0., 0., 0., 0.), v4(0., 0.,-1., 0.), v4( 0., 1., 0., 0.), v4(0., 0., 0., 0.)}},
        {{v4(0., 0., 1., 0.), v4(0., 0., 0., 0.), v4(-1., 0., 0., 0.), v4(0., 0., 0., 0.)}},
        {{v4(0.,-1., 0., 0.), v4(1., 0., 0., 0.), v4( 0., 0., 0., 0.), v4(0., 0., 0., 0.)}},
        {{v4(0., 0., 0., 0.), v4(0., 0., 0., 0.), v4( 0., 0., 0., 0.), v4(0., 0., 0., 0.)}}
    }
};

const static v4m4 dv_dV = { {
        {{v4(0., 0., 0., 0.),
          v4(0., 0.,-.5, 0.),
          v4(0., .5, 0., 0.),
          v4(0., 0., 0., 0.)}},

        {{v4(0., 0., .5, 0.),
          v4(0., 0., 0., 0.),
          v4(-.5, 0., 0.,0.),
          v4(0., 0., 0., 0.)}},
        
        {{v4(0.,-.5, 0., 0.),
          v4(.5, 0., 0., 0.),
          v4(0., 0., 0., 0.),
          v4(0., 0., 0., 0.)}},
        
        {{v4(0., 0., 0., 0.),
          v4(0., 0., 0., 0.),
          v4(0., 0., 0., 0.),
          v4(0., 0., 0., 0.)}}
    }
};

/*
    if(V2) {

    }
}
*/

                               /*
v4 quaternion_from_axis_angle(const v4 W, m4 *dQ_dW)
{
    v4 W2 = W * W;
    f_t theta2 = sum(W2);
    if(theta2 <= 0.) {
        return (v4) { 1., 0., 0., 0. };
    }
    f_t theta = sqrt(theta2),
        cos_theta = cos(theta / 2.),
        sin_theta = sin(theta / 2.),
        sterm = sin_theta / theta;
    
    return (v4) { cos_theta, W[0] * sterm, W[1] * sterm, W[2] * sterm };
}
                               */

m4 rodrigues(const v4 W, m4v4 *dR_dW)
{
    v4 W2 = W * W;

    f_t theta2 = sum(W2);
    //1/theta ?= 0
    if(theta2 <= 0.) {
        if(dR_dW) {
            *dR_dW = dV_dv;
        }
        return m4_identity;
    }
    f_t theta = sqrt(theta2);
    f_t costheta, sintheta, invtheta, sinterm, costerm;
    v4 dsterm_dW, dcterm_dW;
    bool small = theta2 * theta2 * (1./120.) <= EPS; //120 = 5!, next term of sine expansion
    if(small) {
        //Taylor expansion: sin = x - x^3 / 6; cos = 1 - x^2 / 2 + x^4 / 24
        sinterm = 1. - theta2 * (1./6.);
        costerm = 0.5 - theta2 * (1./24.);
    } else {
        invtheta = 1. / theta;
        sintheta = sin(theta);
        costheta = cos(theta);
        sinterm = sintheta * invtheta;
        costerm = (1. - costheta) / theta2;
    }
    m4 V = skew3(W);
    m4 V2;
    V2[0][0] = -W2[1] - W2[2];
    V2[1][1] = -W2[2] - W2[0];
    V2[2][2] = -W2[0] - W2[1];
    V2[2][1] = V2[1][2] = W[1]*W[2];
    V2[0][2] = V2[2][0] = W[0]*W[2];
    V2[1][0] = V2[0][1] = W[0]*W[1];
 
    if(dR_dW) {
        v4 ds_dW, dc_dW;
        if(small) {
            ds_dW = W * -(1./3.);
            dc_dW = W * -(1./12.);
        } else {
            v4
                ds_dt = (costheta - sinterm) * invtheta,
                dc_dt = (sinterm - 2*costerm) * invtheta,
                dt_dW = W * invtheta;
            ds_dW = ds_dt * dt_dW;
            dc_dW = dc_dt * dt_dW;
        }
        m4v4 dV2_dW;
        dV2_dW[0][0] = v4(0., -2*W[1], -2*W[2], 0.);
        dV2_dW[1][1] = v4(-2*W[0], 0., -2*W[2], 0.);
        dV2_dW[2][2] = v4(-2*W[0], -2*W[1], 0., 0.);
        //don't do multiple assignment to avoid gcc bug
        dV2_dW[2][1] = v4(0., W[2], W[1], 0);
        dV2_dW[1][2] = v4(0., W[2], W[1], 0);
        dV2_dW[0][2] = v4(W[2], 0., W[0], 0);
        dV2_dW[2][0] = v4(W[2], 0., W[0], 0);
        dV2_dW[1][0] = v4(W[1], W[0], 0., 0);
        dV2_dW[0][1] = v4(W[1], W[0], 0., 0);
        
        *dR_dW = dV_dv * sinterm + outer_product(ds_dW, V) + dV2_dW * costerm + outer_product(dc_dW, V2);
    }
    return m4_identity + V * sinterm + V2 * costerm;
}

v4 invrodrigues(const m4 R, v4m4 *dW_dR)
{
    f_t trc = trace3(R);
    //sin theta can be zero if:
    if(trc >= 3.) { //theta = 0, so sin = 0
        if(dW_dR) {
            *dW_dR = dv_dV;
        }
        return v4(0.);
    }
    if(trc <= -1. + EPS) {//theta = pi - discontinuity as axis flips; off-axis elements don't give a good vector
        assert(0 && "need to implement invrodrigues for theta = pi"); //I don't think we need to do this since we integrate with small time steps
        //pick the largest diagonal - then average off-diagonal elements
        // http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToAngle/
        if(R[0][0] > R[1][1] && R[0][0] > R[2][2]) { //x is largest
        } else if(R[1][1] > R[2][2]) { // y is largest
        } else { // z is largest
        }
    }

    v4 s = invskew3(R);
    f_t
        costheta = (trc - 1.0) / 2.0,
        theta = acos(costheta),
        sintheta = sin(theta);

    if(theta * theta / 6. < EPS) { //theta is small, so we have near-skew-symmetry and discontinuity
        //just use the off-diagonal elements
        if(dW_dR) *dW_dR = dv_dV;
        return s;
    }
    f_t invsintheta = 1. / sintheta,
        invsin2theta = invsintheta * invsintheta,
        thetaf = theta * invsintheta;

    if(dW_dR) {
        *dW_dR = v4m4();
        f_t
            dtheta_dtrc = -0.5 * invsintheta,
            dtf_dt = invsintheta - theta * costheta * invsin2theta;

        m4 dtheta_dR = m4_identity * dtheta_dtrc;

        *dW_dR = dv_dV * thetaf + outer_product(dtheta_dR, s*dtf_dt);
    }
    return s * thetaf;
}

/*
//leave these out for now - these integrate w*q instead of q*w, and i'm pretty uncertain about how to update the simplified expressions and what derivatives should be - not worth it yet
v4 integrate_angular_velocity(const v4 &W, const v4 &w)
{
    f_t theta2 = sum(W * W);
    f_t gamma, eta;
    bool small = theta2 * theta2 * (1./120.) <= EPS;
    if(small) {
        gamma = 2. - theta2 / 6.;
        eta = sum(W * w) * (60. + theta2) / 360.;
    } else {
        f_t theta = sqrt(theta2),
            sinp = sin(.5 * theta),
            cosp = cos(.5 * theta),
            cotp = cosp / sinp,
            invtheta = 1. / theta;
        gamma = theta * cotp,
        eta = sum(W * w) * invtheta * (cotp - 2. * invtheta);
    }
    return .5 * (gamma * w - eta * W + cross(W, w));
}

void linearize_angular_integration(const v4 &W, const v4 &w, m4 &dW_dW, m4 &dW_dw)
{
    f_t theta2 = sum(W * W);
    f_t gamma, eta;
    v4 dg_dW, de_dW, de_dw;
    bool small = theta2 * theta2 * (1./120.) <= EPS;
    if(small) {
        dg_dW = W * -(1./3.);
        de_dW = sum(W * w) * W * (1./180.) + w * (60. + theta2) / 360.;
        de_dw = W * (60. + theta2) / 360.;
    } else {
        f_t theta = sqrt(theta2),
            sinp = sin(.5 * theta),
            cosp = cos(.5 * theta),
            cotp = cosp / sinp,
            invtheta = 1. / theta;

        v4 dt_dW = W * invtheta;
        f_t dcotp_dt = -.5 / (sinp * sinp);
        //v4 dcotp_dW = dcotp_dt * dt_dW;
        v4 dinvtheta_dW = -1. / theta2 * dt_dW;
        f_t dg_dt = cotp + theta * dcotp_dt;
        dg_dW = dg_dt * dt_dW;
        de_dW = w * cotp / theta + w * W * dt_dW * (dcotp_dt / theta - cotp / theta2) - (2 * w / theta2 - w * W * dt_dW / (theta2 * theta));
        de_dw = W * cotp / theta - 2. * W / theta2;
        //de_dW = w * invtheta * (cotp - 2. * invtheta) + sum(W * w) * (dinvtheta_dW * (cotp - 2. * invtheta) + invtheta * (dcotp_dW -2. * dinvtheta_dW));
        //de_dw = W * invtheta * (cotp - 2. * invtheta);
        gamma = theta * cotp;
        eta = sum(W * w) * invtheta * (cotp - 2. * invtheta);
    }

    dW_dW = .5 * ((m4){{dg_dW[0] * w[0], dg_dW[1] * w[1], dg_dW[2] * w[2], dg_dW[3] * w[3]}} - (eta * m4_identity + (m4){{de_dW[0] * W[0], de_dW[1] * W[1], de_dW[2] * W[2], de_dW[3] * W[3]}}) - skew3(w));
    dW_dw = .5 * (gamma * m4_identity - ((m4){{de_dw[0] * W[0], de_dw[1] * W[1], de_dw[2] * W[2], de_dw[3] * W[3]}}) + skew3(W));
}
*/
