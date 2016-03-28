// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "state_vision.h"
#include "../numerics/vec4.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "../numerics/kalman.h"
#include "../numerics/matrix.h"
#include "observation.h"
#include "filter.h"
#include <memory>

const static sensor_clock::duration max_camera_delay = std::chrono::microseconds(200000); //We drop a frame if it arrives at least this late
const static sensor_clock::duration max_inertial_delay = std::chrono::microseconds(100000); //We drop inertial data if it arrives at least this late
const static sensor_clock::duration min_steady_time = std::chrono::microseconds(100000); //time held steady before we start treating it as steady
const static sensor_clock::duration steady_converge_time = std::chrono::microseconds(200000); //time that user needs to hold steady (us)
const static int calibration_converge_samples = 200; //number of accelerometer readings needed to converge in calibration mode
const static f_t accelerometer_steady_var = .15*.15; //variance when held steady, based on std dev measurement of iphone 5s held in hand
const static f_t velocity_steady_var = .1 * .1; //initial var of state.V when steady
const static f_t accelerometer_inertial_var = 2.33*2.33; //variance when in inertial only mode
const static f_t static_sigma = 6.; //how close to mean measurements in static mode need to be
const static f_t steady_sigma = 3.; //how close to mean measurements in steady mode need to be - lower because it is handheld motion, not gaussian noise
const static f_t dynamic_W_thresh_variance = 5.e-2; // variance of W must be less than this to initialize from dynamic mode
//a_bias_var for best results on benchmarks is 6.4e-3
const static f_t min_a_bias_var = 1.e-6; // calibration will finish immediately when variance of a_bias is less than this, and it is reset to this between each run
const static f_t min_w_bias_var = 1.e-8; // variance of w_bias is reset to this between each run
const static f_t max_accel_delta = 20.; //This is biggest jump seen in hard shaking of device
const static f_t max_gyro_delta = 5.; //This is biggest jump seen in hard shaking of device
const static sensor_clock::duration qr_detect_period = std::chrono::microseconds(100000); //Time between checking frames for QR codes to reduce CPU usage
const static f_t convergence_minimum_velocity = 0.3; //Minimum speed (m/s) that the user must have traveled to consider the filter converged
const static f_t convergence_maximum_depth_variance = .001; //Median feature depth must have been under this to consider the filter converged
//TODO: homogeneous coordinates.

/*
void test_time_update(struct filter *f, f_t dt, int statesize)
{
    //test linearization
    MAT_TEMP(ltu, statesize, statesize);
    memset(ltu_data, 0, sizeof(ltu_data));
    for(int i = 0; i < statesize; ++i) {
        ltu(i, i) = 1.;
    }

    MAT_TEMP(save_state, 1, statesize);
    MAT_TEMP(save_new_state, 1, statesize);
    MAT_TEMP(state, 1, statesize);
    f->s.copy_state_to_array(save_state);

    integrate_motion_state_euler(f->s, dt);
    f->s.copy_state_to_array(save_new_state);

    f_t eps = .1;

    for(int i = 0; i < statesize; ++i) {
        memcpy(state_data, save_state_data, sizeof(state_data));
        f_t leps = state[i] * eps + 1.e-7;
        state[i] += leps;
        f->s.copy_state_from_array(state);
        integrate_motion_state_euler(f->s, dt);
        f->s.copy_state_to_array(state);
        for(int j = 0; j < statesize; ++j) {
            f_t delta = state[j] - save_new_state[j];
            f_t ldiff = leps * ltu(j, i);
            if((ldiff * delta < 0.) && (fabs(delta) > 1.e-5)) {
                f->log->warn("{}\t{}\t: sign flip: expected {}, got {}", i, j, ldiff, delta);
                continue;
            }
            f_t error = fabs(ldiff - delta);
            if(fabs(delta)) error /= fabs(delta);
            else error /= 1.e-5;
            if(error > .1) {
                f->log->warn("{}\t{}\t: lin error: expected {}, got {}", i, j, ldiff, delta);
                continue;
            }
        }
    }
    f->s.copy_state_from_array(save_state);
}

void test_meas(struct filter *f, int pred_size, int statesize, int (*predict)(state *, matrix &, matrix *))
{
    //test linearization
    MAT_TEMP(lp, pred_size, statesize);
    MAT_TEMP(save_state, 1, statesize);
    MAT_TEMP(state, 1, statesize);
    f->s.copy_state_to_array(save_state);
    MAT_TEMP(pred, 1, pred_size);
    MAT_TEMP(save_pred, 1, pred_size);
    memset(lp_data, 0, sizeof(lp_data));
    predict(&f->s, save_pred, &lp);

    f_t eps = .1;

    for(int i = 0; i < statesize; ++i) {
        memcpy(state_data, save_state_data, sizeof(state_data));
        f_t leps = state[i] * eps + 1.e-7;
        state[i] += leps;
        f->s.copy_state_from_array(state);
        predict(&f->s, pred, NULL);
        for(int j = 0; j < pred_size; ++j) {
            f_t delta = pred[j] - save_pred[j];
            f_t ldiff = leps * lp(j, i);
            if((ldiff * delta < 0.) && (fabs(delta) > 1.e-5)) {
                f->log->warn("{}\t{}\t: sign flip: expected {}, got {}", i, j, ldiff, delta);
                continue;
            }
            f_t error = fabs(ldiff - delta);
            if(fabs(delta)) error /= fabs(delta);
            else error /= 1.e-5;
            if(error > .1) {
                f->log->warn("{}\t{}\t: lin error: expected {}, got {}", i, j, ldiff, delta);
                continue;
            }
        }
    }
    f->s.copy_state_from_array(save_state);
}
*/

void filter_update_outputs(struct filter *f, sensor_clock::time_point time)
{
    if(f->run_state != RCSensorFusionRunStateRunning) return;
    m4
        R = to_rotation_matrix(f->s.Q.v),
        Rt = R.transpose();

    bool old_speedfail = f->speed_failed;
    f->speed_failed = false;
    f_t speed = f->s.V.v.norm();
    if(speed > 3.) { //1.4m/s is normal walking speed
        if (!old_speedfail) f->log->warn("Velocity {} m/s exceeds max bound", speed);
        f->speed_failed = true;
        f->calibration_bad = true;
    } else if(speed > 2.) {
        if (!f->speed_warning) f->log->warn("High velocity ({} m/s) warning", speed);
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    f_t accel = f->s.a.v.norm();
    if(accel > 9.8) { //1g would saturate sensor anyway
        if (!old_speedfail) f->log->warn("Acceleration exceeds max bound");
        f->speed_failed = true;
        f->calibration_bad = true;
    } else if(accel > 5.) { //max in mine is 6.
        if (!f->speed_warning) f->log->warn("High acceleration ({} m/s^2) warning", accel);
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    f_t ang_vel = f->s.w.v.norm();
    if(ang_vel > 5.) { //sensor saturation - 250/180*pi
        if (!old_speedfail) f->log->warn("Angular velocity exceeds max bound");
        f->speed_failed = true;
        f->calibration_bad = true;
    } else if(ang_vel > 2.) { // max in mine is 1.6
        if (!f->speed_warning) f->log->warn("High angular velocity warning");
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    //if(f->speed_warning && filter_converged(f) < 1.) f->speed_failed = true;
    if(time - f->speed_warning_time > std::chrono::microseconds(1000000)) f->speed_warning = false;

    //f->log->trace("{} [{} {} {}] [{} {} {}]", time, output[0], output[1], output[2], output[3], output[4], output[5]);
}

void process_observation_queue(struct filter *f, sensor_clock::time_point time)
{
    f->last_time = time;
    if(!f->observations.process(f->s, time)) {
        f->numeric_failed = true;
        f->calibration_bad = true;
    }
}

void filter_compute_gravity(struct filter *f, double latitude, double altitude)
{
    assert(f); f->s.compute_gravity(latitude, altitude);
}

static bool check_packet_time(struct filter *f, sensor_clock::time_point t, int type)
{
    if(t < f->last_packet_time) {
        f->log->warn("Warning: received packets out of order: {} at {} came first, then {} at {}. delta {}", f->last_packet_type, sensor_clock::tp_to_micros(f->last_packet_time), type, sensor_clock::tp_to_micros(t), (long long)std::chrono::duration_cast<std::chrono::microseconds>(f->last_packet_time - t).count());
        return false;
    }
    f->last_packet_time = t;
    f->last_packet_type = type;
    return true;
}

void update_static_calibration(struct filter *f)
{
    if(f->accel_stability.count < calibration_converge_samples) return;
    v4 var = f->accel_stability.variance;
    f->a_variance = (var[0] + var[1] + var[2]) / 3.;
    var = f->gyro_stability.variance;
    f->w_variance = (var[0] + var[1] + var[2]) / 3.;
    //WARNING HACK - floor set at milhone values
    if(f->w_variance < .00002) f->w_variance = .00002;
    if(f->a_variance < .005) f->a_variance = .005;
    //TODO: get rid of this (figure out how to deal with quantized sensor data)
    //this updates even the one dof that can't converge in the filter for this orientation (since we were static)
    f->s.w_bias.v = f->gyro_stability.mean;
    f->s.w_bias.set_initial_variance(f->gyro_stability.variance[0], f->gyro_stability.variance[1], f->gyro_stability.variance[2]); //Even though the one dof won't have converged in the filter, we know that this is a good value (average across stable meas).
    f->s.w_bias.reset_covariance(f->s.cov);
}

static void reset_stability(struct filter *f)
{
    f->accel_stability = stdev_vector();
    f->gyro_stability = stdev_vector();
    f->stable_start = sensor_clock::time_point(sensor_clock::duration(0));
}

sensor_clock::duration steady_time(struct filter *f, stdev_vector &stdev, const v4 &meas, f_t variance, f_t sigma, sensor_clock::time_point time, const v4 &orientation, bool use_orientation)
{
    bool steady = false;
    if(stdev.count) {
        //hysteresis - tighter tolerance for getting into calibration, but looser for staying in
        f_t sigma2 = sigma * sigma;
        if(time - f->stable_start < min_steady_time) sigma2 *= .5*.5;
        steady = true;
        for(int i = 0; i < 3; ++i) {
            f_t delta = meas[i] - stdev.mean[i];
            f_t d2 = delta * delta;
            if(d2 > variance * sigma2) steady = false;
        }
    }
    if(!steady) {
        reset_stability(f);
        f->stable_start = time;
    }
    if(!stdev.count && use_orientation) {
        if(!f->s.orientation_initialized) return sensor_clock::duration(0);
        v4 local_up = to_rotation_matrix(f->s.Q.v).transpose() * v4(0., 0., 1., 0.);
        //face up -> (0, 0, 1)
        //portrait -> (1, 0, 0)
        //landscape -> (0, 1, 0)
        f_t costheta = orientation.dot(local_up);
        if(fabs(costheta) < .71) return sensor_clock::duration(0); //don't start since we aren't in orientation +/- 6 deg
    }
    stdev.data(meas);
    
    return time - f->stable_start;
}

static void print_calibration(struct filter *f)
{
    f->log->trace() << "w bias is: " << f->s.w_bias.v;
    f->log->trace() << "w bias var is: " << f->s.w_bias.variance();
    f->log->trace() << "a bias is: " << f->s.a_bias.v;
    f->log->trace() << "a bias var is: " << f->s.a_bias.variance();
}

static float var_bounds_to_std_percent(f_t current, f_t begin, f_t end)
{
    return current < end ? 1.f : (float) ((log(begin) - log(current)) / (log(begin) - log(end))); //log here seems to give smoother progress
}

static float get_bias_convergence(const struct filter *f, int dir)
{
    float max_pct = (float)var_bounds_to_std_percent(f->s.a_bias.variance()[dir], f->a_bias_start[dir], min_a_bias_var);
    float pct = (float)f->accel_stability.count / (float)calibration_converge_samples;
    if(f->last_time - f->stable_start < min_steady_time) pct = 0.f;
    if(pct > max_pct) max_pct = pct;
    if(max_pct < 0.f) max_pct = 0.f;
    if(max_pct > 1.f) max_pct = 1.f;
    return max_pct;
}

static f_t get_accelerometer_variance_for_run_state(struct filter *f, const v4 &meas, sensor_clock::time_point time)
{
    if(!f->s.orientation_initialized) return accelerometer_inertial_var; //first measurement is not used, so this doesn't actually matter
    switch(f->run_state)
    {
        case RCSensorFusionRunStateRunning:
        case RCSensorFusionRunStateInactive: //shouldn't happen
            return f->a_variance;
        case RCSensorFusionRunStateDynamicInitialization:
            return accelerometer_inertial_var;
        case RCSensorFusionRunStateInertialOnly:
            return accelerometer_inertial_var;
        case RCSensorFusionRunStateStaticCalibration:
            if(steady_time(f, f->accel_stability, meas, f->a_variance, static_sigma, time, v4(0., 0., 1., 0.), true) > min_steady_time)
            {
                f->s.enable_bias_estimation();
                //base this on # samples instead of variance because we are also estimating a, w variance here
                if(f->accel_stability.count >= calibration_converge_samples && get_bias_convergence(f, 2) >= 1.)
                {
                    update_static_calibration(f);
                    f->run_state = RCSensorFusionRunStatePortraitCalibration;
                    f->a_bias_start = f->s.a_bias.variance();
                    f->w_bias_start = f->s.w_bias.variance();
                    reset_stability(f);
                    f->s.disable_bias_estimation();
                    f->log->trace("When finishing static calibration:");
                    print_calibration(f);
                }
                return f->a_variance * 3 * 3; //pump up this variance because we aren't really perfect here
            }
            else
            {
                f->s.disable_bias_estimation();
                return accelerometer_inertial_var;
            }
        case RCSensorFusionRunStatePortraitCalibration:
        {
            if(steady_time(f, f->accel_stability, meas, accelerometer_steady_var, steady_sigma, time, v4(1, 0, 0, 0), true) > min_steady_time)
            {
                f->s.enable_bias_estimation();
                if(get_bias_convergence(f, 1) >= 1.)
                {
                    f->run_state = RCSensorFusionRunStateLandscapeCalibration;
                    f->a_bias_start = f->s.a_bias.variance();
                    f->w_bias_start = f->s.w_bias.variance();
                    reset_stability(f);
                    f->s.disable_bias_estimation();
                    f->log->trace("When finishing portrait calibration:");
                    print_calibration(f);
                }
                return accelerometer_steady_var;
            }
            else
            {
                f->s.disable_bias_estimation();
                return accelerometer_inertial_var;
            }
        }
        case RCSensorFusionRunStateLandscapeCalibration:
        {
            if(steady_time(f, f->accel_stability, meas, accelerometer_steady_var, steady_sigma, time, v4(0, 1, 0, 0), true) > min_steady_time)
            {
                f->s.enable_bias_estimation();
                if(get_bias_convergence(f, 0) >= 1.)
                {
                    f->run_state = RCSensorFusionRunStateInactive;
                    reset_stability(f);
                    f->s.disable_bias_estimation();
                    f->log->trace("When finishing landscape calibration:");
                    print_calibration(f);
                }
                return accelerometer_steady_var;
            }
            else
            {
                f->s.disable_bias_estimation();
                return accelerometer_inertial_var;
            }
        }
        case RCSensorFusionRunStateSteadyInitialization:
        {
            auto steady = steady_time(f, f->accel_stability, meas, accelerometer_steady_var, steady_sigma, time, v4(), false);
            if(steady > min_steady_time)
            {
                f->s.enable_bias_estimation();
                if(steady > steady_converge_time) {
                    f->s.V.set_initial_variance(velocity_steady_var);
                    f->s.a.set_initial_variance(accelerometer_steady_var);
                }
                return accelerometer_steady_var;
            }
            else
            {
                f->s.disable_bias_estimation();
                return accelerometer_inertial_var;
            }
        }
    }
#ifdef DEBUG
    assert(0); //should never fall through to here;
#endif
    return f->a_variance;
}

void filter_accelerometer_measurement(struct filter *f, const float data[3], sensor_clock::time_point time)
{
    v4 meas_(data[0], data[1], data[2], 0.);
    v4 meas = f->a_alignment * meas_;
    v4 accel_delta = meas - f->last_accel_meas;
    f->last_accel_meas = meas;
    //This will throw away both the outlier measurement and the next measurement, because we update last every time. This prevents setting last to an outlier and never recovering.
    if(f->run_state == RCSensorFusionRunStateInactive) return;
    if(!check_packet_time(f, time, packet_accelerometer)) return;
    if(!f->ignore_lateness) {
        auto current = sensor_clock::now();
        auto delta = current - time;
        if(delta > max_inertial_delay) {
            f->log->warn("Warning, dropped an old accel sample - timestamp {}, now {}", sensor_clock::tp_to_micros(time), sensor_clock::tp_to_micros(current));
            return;
        }
    }
    if(!f->got_accelerometer) { //skip first packet - has been crap from gyro
        f->got_accelerometer = true;
        return;
    }
    if(!f->got_gyroscope) return;

    if(fabs(accel_delta[0]) > max_accel_delta || fabs(accel_delta[1]) > max_accel_delta || fabs(accel_delta[2]) > max_accel_delta)
    {
        f->log->warn("Rejecting an accel sample due to extreme jump {} {} {}", accel_delta[0], accel_delta[1], accel_delta[2]);
        return;
    }
    
    auto obs_a = std::make_unique<observation_accelerometer>(f->s, time, time);
    
    for(int i = 0; i < 3; ++i) {
        obs_a->meas[i] = meas[i];
    }
    
    obs_a->variance = get_accelerometer_variance_for_run_state(f, meas, time);
    f->observations.observations.push_back(std::move(obs_a));

    if(show_tuning) f->log->trace("accelerometer:");
    process_observation_queue(f, time);
    if(show_tuning) {
        f->log->trace() << " actual innov stdev is:\n" <<
        observation_accelerometer::inn_stdev <<
        " signal stdev is:\n" <<
        observation_accelerometer::stdev <<
        " bias is:\n" <<
        f->s.a_bias.v << f->s.a_bias.variance();
    }
    if(!f->gravity_init) {
        f->gravity_init = true;
        if(!f->origin_gravity_aligned)
        {
            f->origin.Q = f->origin.Q * conjugate(f->s.initial_orientation);
        }
    }
}

void filter_gyroscope_measurement(struct filter *f, const float data[3], sensor_clock::time_point time)
{
    v4 meas_(data[0], data[1], data[2], 0.);
    v4 meas = f->w_alignment * meas_;
    v4 gyro_delta = meas - f->last_gyro_meas;
    f->last_gyro_meas = meas;
    //This will throw away both the outlier measurement and the next measurement, because we update last every time. This prevents setting last to an outlier and never recovering.
    if(f->run_state == RCSensorFusionRunStateInactive) return;
    if(!check_packet_time(f, time, packet_gyroscope)) return;
    if(!f->ignore_lateness) {
        auto current = sensor_clock::now();
        auto delta = current - time;
        if(delta > max_inertial_delay) {
            f->log->warn("Warning, dropped an old gyro sample - timestamp {}, now {}", sensor_clock::tp_to_micros(time), sensor_clock::tp_to_micros(current));
            return;
        }
    }
    if(!f->got_gyroscope) { //skip the first piece of data as it seems to be crap
        f->got_gyroscope = true;
        return;
    }
    if(!f->gravity_init) return;

    if(fabs(gyro_delta[0]) > max_gyro_delta || fabs(gyro_delta[1]) > max_gyro_delta || fabs(gyro_delta[2]) > max_gyro_delta)
    {
        f->log->warn("Rejecting a gyro sample due to extreme jump {} {} {}", gyro_delta[0], gyro_delta[1], gyro_delta[2]);
        return;
    }

    auto obs_w = std::make_unique<observation_gyroscope>(f->s, time, time);
    for(int i = 0; i < 3; ++i) {
        obs_w->meas[i] = meas[i];
    }
    obs_w->variance = f->w_variance;

    f->observations.observations.push_back(std::move(obs_w));

    if(f->run_state == RCSensorFusionRunStateStaticCalibration) {
        f->gyro_stability.data(meas);
    }

    if(show_tuning) f->log->trace("gyroscope:");
    process_observation_queue(f, time);
    if(show_tuning) {
        f->log->trace() << " actual innov stdev is:\n" <<
        observation_gyroscope::inn_stdev <<
        " signal stdev is:\n" <<
        observation_gyroscope::stdev <<
        " bias is:\n" <<
        f->s.w_bias.v << f->s.w_bias.variance();
    }
}

void filter_setup_next_frame(struct filter *f, const image_gray8 &image)
{
    size_t feats_used = f->s.features.size();

    if(f->run_state != RCSensorFusionRunStateRunning) return;

    if(feats_used) {
        for(state_vision_group *g : f->s.groups.children) {
            if(!g->status || g->status == group_initializing) continue;
            for(state_vision_feature *i : g->features.children) {
                auto extra_time = std::chrono::duration_cast<sensor_clock::duration>(image.exposure_time * (i->current[1] / (float)image.height));
                auto obs = std::make_unique<observation_vision_feature>(f->s, image.timestamp + extra_time, image.timestamp);
                obs->state_group = g;
                obs->feature = i;
                obs->meas[0] = i->current[0];
                obs->meas[1] = i->current[1];
                obs->image = image.image;
                obs->tracker = f->track;
                obs->feature->dt = image.timestamp - obs->feature->last_seen;
                obs->feature->last_seen = image.timestamp;

                f->observations.observations.push_back(std::move(obs));
            }
        }
    }
    //TODO: implement feature_single ?
}

static uint16_t get_depth_for_point_mm(const image_depth16 &depth, const feature_t & p)
{
    auto x = (int)p.x(), y = (int)p.y();
    if (x >=0 && x < depth.width && y >= 0 && y < depth.height)
        return depth.image[depth.stride / sizeof(uint16_t) * y + x];
    else
        return 0;
}

static float get_stdev_pct_for_depth(float depth_m)
{
    return 0.0023638192164147698f + (0.0015072367800769945f + 0.00044245048102432134f * depth_m) * depth_m;
}

std::unique_ptr<image_depth16> filter_aligned_depth_to_intrinsics(const struct filter *f, const image_depth16 &depth)
{
    if (f->depth.intrinsics.type == rc_CALIBRATION_TYPE_UNKNOWN)
        return std::make_unique<image_depth16>(std::move(depth));

    auto aligned_depth = make_unique<image_depth16>(depth.width, depth.height, depth.stride, std::numeric_limits<uint16_t>::max());

    int i_width =         depth .width, i_height =         depth .height, i_stride =         depth .stride / sizeof(uint16_t);
    int o_width = aligned_depth->width, o_height = aligned_depth->height, o_stride = aligned_depth->stride / sizeof(uint16_t);
    uint16_t *in =        depth .image, *out     = aligned_depth->image;

    if (!(i_width == 320 && i_height == 240))
        printf("Our hardcoded depth intrisics are no good!\n");

    float d_focal_length_x =  f->depth.intrinsics.f_x_px                                            / f->depth.intrinsics.height_px,
          d_focal_length_y =  f->depth.intrinsics.f_y_px                                            / f->depth.intrinsics.height_px,
          d_center_x       = (f->depth.intrinsics.c_x_px - f->depth.intrinsics.width_px  / 2. + .5) / f->depth.intrinsics.height_px,
          d_center_y       = (f->depth.intrinsics.c_y_px - f->depth.intrinsics.height_px / 2. + .5) / f->depth.intrinsics.height_px;

    float i_Cx = d_center_x       * i_height + (i_width  - 1) / 2.,
          i_Cy = d_center_y       * i_height + (i_height - 1) / 2.,
          i_Fx = d_focal_length_x * i_height,
          i_Fy = d_focal_length_y * i_height;

    float o_Cx = f->s.center_x.v     * o_height + (o_width  - 1) / 2.,
          o_Cy = f->s.center_y.v     * o_height + (o_height - 1) / 2.,
          o_Fx = f->s.focal_length.v * o_height,
          o_Fy = f->s.focal_length.v * o_height;

    transformation depth_to_color_m = invert(transformation(f->s.Qc.v,f->s.Tc.v)) * f->depth.extrinsics_wrt_imu_m;
    Eigen::Vector4f x_T_mm = (depth_to_color_m.T * 1000).cast<float>();
    Eigen::Array4i one = {0,1,0,1}, ONE = {0,0,1,1};

    for (int y = 0; y < i_height; y++)
        for (int x = 0; x < i_width; x++) {
            uint16_t z = in[y * i_stride + x];
            if (!z) continue;
            float xf = x, yf = y, zf = z;

            // normalize, undistort(?), unproject and transform
            float ix = zf * (xf - i_Cx) / i_Fx + x_T_mm[0];
            float iy = zf * (yf - i_Cy) / i_Fy + x_T_mm[1];
            float iz = zf                      + x_T_mm[2];

            // project, distort(?), unnormalize
            float ox = o_Fx * ix / iz + o_Cx;
            float oy = o_Fy * iy / iz + o_Cy;
            float oz = iz;

            if (0) {
                int X = (int)roundf(ox), Y = (int)roundf(oy), Z = (int)roundf(oz);
                if (X >= 0 && X < o_width && Y >=0 && Y < o_height)
                    if (Z < out[Y * o_stride + X])
                        out[Y * o_stride + X] = Z;
            } else {
                // ceil() and -1s give the 4 closest grid points
                auto X = static_cast<int>(std::ceil(ox)) - Eigen::Array4i{0,1,0,1};
                auto Y = static_cast<int>(std::ceil(oy)) - Eigen::Array4i{0,0,1,1};
                auto Z = static_cast<int>(roundf(oz));
                auto I = Y * o_stride + X;
                auto within = X >= 0 && X < o_width && Y >= 0 && Y < o_height;
                if (within[0] && oz < static_cast<float>(out[I[0]])) out[I[0]] = static_cast<uint16_t>(oz);
                if (within[1] && oz < static_cast<float>(out[I[1]])) out[I[1]] = static_cast<uint16_t>(oz);
                if (within[2] && oz < static_cast<float>(out[I[2]])) out[I[2]] = static_cast<uint16_t>(oz);
                if (within[3] && oz < static_cast<float>(out[I[3]])) out[I[3]] = static_cast<uint16_t>(oz);
            }
        }

    for (int Y = 0; Y < o_height; Y++)
        for (int X = 0; X < o_width; X++)
            if (out[Y * o_stride + X] == std::numeric_limits<uint16_t>::max())
                out[Y * o_stride + X] = 0;

    return std::move(aligned_depth);
}

std::unique_ptr<image_depth16> filter_aligned_distorted_depth_to_intrinsics(const struct filter *f, const image_depth16 &depth)
{
    std::unique_ptr<image_depth16> aligned_depth = filter_aligned_depth_to_intrinsics(f, depth);

    auto aligned_distorted_depth = make_unique<image_depth16>(depth.width, depth.height, depth.stride, std::numeric_limits<uint16_t>::max());
    auto out = aligned_distorted_depth->image;
    int width = aligned_distorted_depth->width, height = aligned_distorted_depth->height;
    int stride = aligned_distorted_depth->stride / sizeof(uint16_t);
    for(int y_image = 0; y_image < depth.height; y_image++) {
        for(int x_image = 0; x_image < depth.width; x_image++) {
            feature_t kp_i = {(f_t)x_image, (f_t)y_image};
            feature_t kp_d = f->s.unnormalize_feature(f->s.undistort_feature(f->s.normalize_feature(kp_i)));
            uint16_t depth_mm = get_depth_for_point_mm(*aligned_depth.get(), kp_d);
            int x = kp_d.x();
            int y = kp_d.y();
            if(x >= 0 && x < width && y >= 0 && y < height) {
                out[y * stride + x] = std::min(out[y * stride + x], depth_mm);
            }
        }
    }

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            if (out[y * stride + x] == std::numeric_limits<uint16_t>::max())
                out[y * stride + x] = 0;
    return std::move(aligned_distorted_depth);
}

//features are added to the state immediately upon detection - handled with triangulation in observation_vision_feature::predict - but what is happening with the empty row of the covariance matrix during that time?
static void filter_add_features(struct filter *f, const image_gray8 & image, size_t newfeats)
{
#ifdef TEST_POSDEF
    if(!test_posdef(f->s.cov.cov)) f->log->warn("not pos def before adding features");
#endif
    // Filter out features which we already have by masking where
    // existing features are located
    if(!f->mask) f->mask = new scaled_mask(image.width, image.height);
    f->mask->initialize();
    for(state_vision_feature *i : f->s.features) {
        f->mask->clear((int)i->current[0], (int)i->current[1]);
    }

    // Run detector
    vector<xy> &kp = f->track.detect(image.image, f->mask, (int)newfeats, 0, 0, image.width, image.height);

    // Check that the detected features don't collide with the mask
    // and add them to the filter
    if(kp.size() < newfeats) newfeats = kp.size();
    if(newfeats < state_vision_group::min_feats) return;
    state_vision_group *g = f->s.add_group(image.timestamp);
    std::unique_ptr<image_depth16> aligned_undistorted_depth;

    float radius = 16;
    int side_length = 41;

    int found_feats = 0;
    for(int i = 0; i < (int)kp.size(); ++i) {
        feature_t kp_i = {kp[i].x, kp[i].y};
        int x = (int)kp[i].x;
        int y = (int)kp[i].y;
        if(f->track.is_trackable(x, y) && f->mask->test(x, y)) {
            f->mask->clear(x, y);
            state_vision_feature *feat = f->s.add_feature(kp_i);
            //descriptor_compute(camera.image, camera.width, camera.height, camera.stride, x, y, side_length, radius, d);
            //feat->descriptor = d;

            float depth_m = 0;
            if(f->has_depth) {
                if (!aligned_undistorted_depth)
                    aligned_undistorted_depth = std::move(filter_aligned_depth_to_intrinsics(f, f->recent_depth));
                depth_m = 0.001f * get_depth_for_point_mm(*aligned_undistorted_depth.get(), f->s.unnormalize_feature(f->s.undistort_feature(f->s.normalize_feature(kp_i))));
            }
            if(depth_m)
            {
                feat->v.set_depth_meters(depth_m);
                float std_pct = get_stdev_pct_for_depth(depth_m);
                feat->set_initial_variance(std_pct * std_pct); // assumes log depth
                feat->status = feature_normal;
            }
            
            f->track.add_track(image.image, x, y, feat->patch);

            g->features.children.push_back(feat);
            feat->groupid = g->id;
            feat->found_time = image.timestamp;
            feat->last_seen = feat->found_time;
            
            found_feats++;
            if(found_feats == newfeats) break;
        }
    }
    g->status = group_initializing;
    g->make_normal();
    f->s.remap();
#ifdef TEST_POSDEF
    if(!test_posdef(f->s.cov.cov)) f->log->warn("not pos def after adding features");
#endif
}

void filter_set_origin(struct filter *f, const transformation &origin, bool gravity_aligned)
{
    if(gravity_aligned) {
        v4 z_old(0., 0., 1., 0.);
        v4 z_new = origin.Q * z_old;
        quaternion Qd = rotation_between_two_vectors_normalized(z_new, z_old);
        f->origin.Q = Qd * origin.Q;
    } else f->origin.Q = origin.Q;
    f->origin.T = origin.T;
}

void filter_set_reference(struct filter *f)
{
    filter_set_origin(f, transformation(f->s.Q.v, f->s.T.v), true);
    //f->s.reset_position();
}

bool filter_depth_measurement(struct filter *f, const image_depth16 & depth)
{
    f->recent_depth = image_depth16(depth);
    f->has_depth = true;
    return true;
}

bool filter_image_measurement(struct filter *f, const image_gray8 & image)
{
    sensor_clock::time_point time = image.timestamp;

    if(f->run_state == RCSensorFusionRunStateInactive) return false;
    if(!check_packet_time(f, time, packet_camera)) return false;
    if(!f->got_accelerometer || !f->got_gyroscope) return false;
    
#ifdef ENABLE_QR
    if(f->qr.running && (time - f->last_qr_time > qr_detect_period)) {
        f->last_qr_time = time;
        f->qr.process_frame(f, image.image, image.width, image.height);
        if(f->qr.valid)
        {
            filter_set_origin(f, f->qr.origin, f->origin_gravity_aligned);
        }
    }
    if(f->qr_bench.enabled)
        f->qr_bench.process_frame(f, image.image, image.width, image.height);
#endif
    
    f->got_image = true;
    if(f->run_state == RCSensorFusionRunStateDynamicInitialization) {
        if(f->want_start == sensor_clock::micros_to_tp(0)) f->want_start = time;
        bool inertial_converged = (f->s.Q.variance()[0] < dynamic_W_thresh_variance && f->s.Q.variance()[1] < dynamic_W_thresh_variance);
        if(inertial_converged) {
            if(inertial_converged) {
                f->log->debug("Inertial converged at time {}", std::chrono::duration_cast<std::chrono::microseconds>(time - f->want_start).count());
            } else {
                f->log->warn("Inertial did not converge {}, {}", f->s.Q.variance()[0], f->s.Q.variance()[1]);
            }
        } else return true;
    }
    if(f->run_state == RCSensorFusionRunStateSteadyInitialization) {
        if(f->stable_start == sensor_clock::micros_to_tp(0)) return true;
        if(time - f->stable_start < steady_converge_time) return true;
    }
    if(f->run_state != RCSensorFusionRunStateRunning && f->run_state != RCSensorFusionRunStateDynamicInitialization && f->run_state != RCSensorFusionRunStateSteadyInitialization) return true; //frame was "processed" so that callbacks still get called
    
    f->track.width = image.width;
    f->track.height = image.height;
    f->track.stride = image.stride;
    f->track.init();
    f->s.image_width = image.width;
    f->s.image_height = image.height;
    
    if(!f->ignore_lateness) {
        /*thread_info_data_t thinfo;
        mach_msg_type_number_t thinfo_count;
        kern_return_t kr = thread_info(mach_thread_self(), THREAD_BASIC_INFO, thinfo, &thinfo_count);
        float cpu = ((thread_basic_info_t)thinfo)->cpu_usage / (float)TH_USAGE_SCALE;
        f->log->info("cpu usage is {}", cpu);*/
        
        auto current = sensor_clock::now();
        auto delta = current - time;
        if(delta > max_camera_delay) {
            f->log->warn("Warning, dropped an old video frame - timestamp {}, now {}", sensor_clock::tp_to_micros(time), sensor_clock::tp_to_micros(current));
            return false;
        }
        if(!f->valid_delta) {
            f->mindelta = delta;
            f->valid_delta = true;
        }
        if(delta < f->mindelta) {
            f->mindelta = delta;
        }
        auto lateness = delta - f->mindelta;
        auto period = time - f->last_arrival;
        f->last_arrival = time;
        
        if(lateness > period * 2) {
            f->log->warn("old max_state_size was {}", f->s.maxstatesize);
            f->s.maxstatesize = f->s.statesize - 1;
            if(f->s.maxstatesize < MINSTATESIZE) f->s.maxstatesize = MINSTATESIZE;
            f->log->warn("was {} us late, new max state size is {}, current state size is {}", std::chrono::duration_cast<std::chrono::microseconds>(lateness).count(), f->s.maxstatesize, f->s.statesize);
            f->log->warn("dropping a frame!");
            return false;
        }
        if(lateness > period && f->s.maxstatesize > MINSTATESIZE && f->s.statesize < f->s.maxstatesize) {
            f->s.maxstatesize = f->s.statesize - 1;
            if(f->s.maxstatesize < MINSTATESIZE) f->s.maxstatesize = MINSTATESIZE;
            f->log->warn("was {} us late, new max state size is {}, current state size is {}", std::chrono::duration_cast<std::chrono::microseconds>(lateness).count(), f->s.maxstatesize, f->s.statesize);
        }
        if(lateness < period / 4 && f->s.statesize > f->s.maxstatesize - f->min_group_add && f->s.maxstatesize < MAXSTATESIZE - 1) {
            ++f->s.maxstatesize;
            f->log->warn("was {} us late, new max state size is {}, current state size is {}", std::chrono::duration_cast<std::chrono::microseconds>(lateness).count(), f->s.maxstatesize, f->s.statesize);
        }
    }


    filter_setup_next_frame(f, image);

    if(show_tuning) {
        f->log->trace("vision:");
    }
    process_observation_queue(f, time);
    if(show_tuning) {
        f->log->trace(" actual innov stdev is:");
        f->log->trace() << observation_vision_feature::inn_stdev[0];
        f->log->trace() << observation_vision_feature::inn_stdev[1];
    }

    int features_used = f->s.process_features(image, time);
    if(!features_used)
    {
        //Lost all features - reset convergence
        f->has_converged = false;
        f->max_velocity = 0.;
        f->median_depth_variance = 1.;
    }

    int space = f->s.maxstatesize - f->s.statesize - 6;
    if(space > f->max_group_add) space = f->max_group_add;
    if(space >= f->min_group_add) {
        if(f->run_state == RCSensorFusionRunStateDynamicInitialization || f->run_state == RCSensorFusionRunStateSteadyInitialization) {
#ifdef TEST_POSDEF
            if(!test_posdef(f->s.cov.cov)) f->log->warn("not pos def before disabling orient only");
#endif
            f->s.disable_orientation_only();
#ifdef TEST_POSDEF
            if(!test_posdef(f->s.cov.cov)) f->log->warn("not pos def after disabling orient only");
#endif
        }
        filter_add_features(f, image, space);
        if(f->s.features.size() < state_vision_group::min_feats) {
            f->log->warn("detector failure: only {} features after add", f->s.features.size());
            f->detector_failed = true;
            f->calibration_bad = true;
            if(f->run_state == RCSensorFusionRunStateDynamicInitialization || f->run_state == RCSensorFusionRunStateSteadyInitialization) f->s.enable_orientation_only();
        } else {
            //don't go active until we can successfully add features
            if(f->run_state == RCSensorFusionRunStateDynamicInitialization || f->run_state == RCSensorFusionRunStateSteadyInitialization) {
                f->run_state = RCSensorFusionRunStateRunning;
                f->log->trace("When moving from steady init to running:");
                print_calibration(f);
                f->active_time = time;
            }
            f->detector_failed = false;
        }
    }

    f->median_depth_variance = f->s.median_depth_variance();

    float velocity = (float)f->s.V.v.norm();
    if(velocity > f->max_velocity) f->max_velocity = velocity;
    
    if(f->max_velocity > convergence_minimum_velocity && f->median_depth_variance < convergence_maximum_depth_variance) f->has_converged = true;
    
    filter_update_outputs(f, time);
    return true;
}

//This should be called every time we want to initialize or reset the filter
extern "C" void filter_initialize(struct filter *f, device_parameters *device)
{
    auto &imu = device->imu;
    auto &cam = device->color;

    //changing these two doesn't affect much.
    f->min_group_add = 16;
    f->max_group_add = 40;

    f->depth = device->depth;

    f->w_variance = imu.w_noise_var_rad2__s2;
    f->a_variance = imu.a_noise_var_m2__s4;

    f->a_alignment = m4::Identity();
    if (imu.a_alignment != m3::Zero())
        f->a_alignment.block<3,3>(0,0) = imu.a_alignment;

    f->w_alignment = m4::Identity();
    if (imu.w_alignment != m3::Zero())
        f->w_alignment.block<3,3>(0,0) = imu.w_alignment;

#ifdef INITIAL_DEPTH
    state_vision_feature::initial_depth_meters = INITIAL_DEPTH;
#else
    state_vision_feature::initial_depth_meters = M_E;
#endif
    state_vision_feature::initial_var = .75;
    state_vision_feature::initial_process_noise = 1.e-20;
    state_vision_feature::measurement_var = 2 * 2;
    state_vision_feature::outlier_thresh = 2;
    state_vision_feature::outlier_reject = 30.;
    state_vision_feature::max_variance = .10 * .10; //because of log-depth, the standard deviation is approximately a percentage (so .10 * .10 = 10%)
    state_vision_group::ref_noise = 1.e-30;
    state_vision_group::min_feats = 1;
    
    observation_vision_feature::stdev[0] = stdev_scalar();
    observation_vision_feature::stdev[1] = stdev_scalar();
    observation_vision_feature::inn_stdev[0] = stdev_scalar();
    observation_vision_feature::inn_stdev[1] = stdev_scalar();
    observation_accelerometer::stdev = stdev_vector();
    observation_accelerometer::inn_stdev = stdev_vector();
    observation_gyroscope::stdev = stdev_vector();
    observation_gyroscope::inn_stdev = stdev_vector();

    f->last_time = sensor_clock::time_point(sensor_clock::duration(0));
    f->last_packet_time = sensor_clock::time_point(sensor_clock::duration(0));
    f->last_packet_type = 0;
    f->gravity_init = false;
    f->want_start = sensor_clock::time_point(sensor_clock::duration(0));
    f->run_state = RCSensorFusionRunStateInactive;
    f->got_accelerometer = false;
    f->got_gyroscope = false;
    f->got_image = false;
    
    f->detector_failed = false;
    f->tracker_failed = false;
    f->tracker_warned = false;
    f->speed_failed = false;
    f->speed_warning = false;
    f->numeric_failed = false;
    f->speed_warning_time = sensor_clock::time_point(sensor_clock::duration(0));
    f->gyro_stability = stdev_vector();
    f->accel_stability = stdev_vector();
    
    f->stable_start = sensor_clock::time_point(sensor_clock::duration(0));
    f->calibration_bad = false;
    
    f->mindelta = std::chrono::microseconds(0);
    f->valid_delta = false;
    
    f->last_arrival = sensor_clock::time_point(sensor_clock::duration(0));
    f->active_time = sensor_clock::time_point(sensor_clock::duration(0));
    
    if(f->mask)
    {
        delete f->mask;
        f->mask = 0;
    }
    
    f->observations.observations.clear();

    f->s.reset();
    f->s.maxstatesize = MAXSTATESIZE;

    f->s.Tc.v = v4(cam.extrinsics_wrt_imu_m.T[0], cam.extrinsics_wrt_imu_m.T[1], cam.extrinsics_wrt_imu_m.T[2], 0.);
    f->s.Qc.v = cam.extrinsics_wrt_imu_m.Q;

    f->s.Qc.set_initial_variance(cam.extrinsics_var_wrt_imu_m.W[0], cam.extrinsics_var_wrt_imu_m.W[1], cam.extrinsics_var_wrt_imu_m.W[2]);
    f->s.Tc.set_initial_variance(cam.extrinsics_var_wrt_imu_m.T[0], cam.extrinsics_var_wrt_imu_m.T[1], cam.extrinsics_var_wrt_imu_m.T[2]);

    f->s.a_bias.v = v4(imu.a_bias_m__s2[0], imu.a_bias_m__s2[1], imu.a_bias_m__s2[2], 0.);
    f_t tmp[3];
    //TODO: figure out how much drift we need to worry about between runs
    for(int i = 0; i < 3; ++i) tmp[i] = imu.a_bias_var_m2__s4[i] < min_a_bias_var ? min_a_bias_var : imu.a_bias_var_m2__s4[i];
    f->s.a_bias.set_initial_variance(tmp[0], tmp[1], tmp[2]);
    f->s.w_bias.v = v4(imu.w_bias_rad__s[0], imu.w_bias_rad__s[1], imu.w_bias_rad__s[2], 0.);
    for(int i = 0; i < 3; ++i) tmp[i] = imu.w_bias_var_rad2__s2[i] < min_w_bias_var ? min_w_bias_var : imu.w_bias_var_rad2__s2[i];
    f->s.w_bias.set_initial_variance(tmp[0], tmp[1], tmp[2]);

    f->s.focal_length.v = (cam.intrinsics.f_x_px + cam.intrinsics.f_y_px) / 2 / cam.intrinsics.height_px;
    f->s.center_x.v = (cam.intrinsics.c_x_px - cam.intrinsics.width_px / 2. + .5) / cam.intrinsics.height_px;
    f->s.center_y.v = (cam.intrinsics.c_y_px - cam.intrinsics.height_px / 2. + .5) / cam.intrinsics.height_px;
    if (cam.intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE)
        f->s.k1.v = cam.intrinsics.w;
    else
        f->s.k1.v = cam.intrinsics.k1;
    f->s.k2.v = cam.intrinsics.k2;
    f->s.k3.v = cam.intrinsics.k3;
    f->s.fisheye = cam.intrinsics.type == rc_CALIBRATION_TYPE_FISHEYE;

    f->s.g.set_initial_variance(1.e-7);
    
    f->s.T.set_process_noise(0.);
    f->s.Q.set_process_noise(0.);
    f->s.V.set_process_noise(0.);
    f->s.w.set_process_noise(0.);
    f->s.dw.set_process_noise(35. * 35.); // this stabilizes dw.stdev around 5-6
    f->s.a.set_process_noise(.6*.6);
    f->s.g.set_process_noise(1.e-30);
    f->s.Qc.set_process_noise(1.e-30);
    f->s.Tc.set_process_noise(1.e-30);
    f->s.a_bias.set_process_noise(1.e-10);
    f->s.w_bias.set_process_noise(1.e-12);
    //TODO: check this process noise
    f->s.focal_length.set_process_noise(1.e-5 / cam.intrinsics.height_px / cam.intrinsics.height_px);
    f->s.center_x.set_process_noise(1.e-5 / cam.intrinsics.height_px / cam.intrinsics.height_px);
    f->s.center_y.set_process_noise(1.e-5 / cam.intrinsics.height_px / cam.intrinsics.height_px);
    f->s.k1.set_process_noise(1.e-9);
    f->s.k2.set_process_noise(1.e-9);
    f->s.k3.set_process_noise(1.e-9);

    f->s.T.set_initial_variance(1.e-7); // to avoid not being positive definite
    //TODO: This might be wrong. changing this to 10 makes a very different (and not necessarily worse) result.
    f->s.Q.set_initial_variance(10., 10., 1.e-7); // to avoid not being positive definite
    f->s.V.set_initial_variance(1. * 1.);
    f->s.w.set_initial_variance(10);
    f->s.dw.set_initial_variance(10); //observed range of variances in sequences is 1-6
    f->s.a.set_initial_variance(10);

    f->s.focal_length.set_initial_variance(10. / cam.intrinsics.height_px / cam.intrinsics.height_px);
    f->s.center_x.set_initial_variance(2. / cam.intrinsics.height_px / cam.intrinsics.height_px);
    f->s.center_y.set_initial_variance(2. / cam.intrinsics.height_px / cam.intrinsics.height_px);

    f->s.k1.set_initial_variance(f->s.fisheye ? .1*.1 : 2.e-4);
    f->s.k2.set_initial_variance(f->s.fisheye ? .1*.1 : 2.e-4);
    f->s.k3.set_initial_variance(f->s.fisheye ? .1*.1 : 2.e-4);
    
    f->s.image_width = cam.intrinsics.width_px;
    f->s.image_height = cam.intrinsics.height_px;
    
    f->track.width = cam.intrinsics.width_px;
    f->track.height = cam.intrinsics.height_px;
    f->track.stride = f->track.width;
    f->track.init();
#ifdef ENABLE_QR
    f->last_qr_time = sensor_clock::micros_to_tp(0);
#endif
    f->max_velocity = 0.;
    f->median_depth_variance = 1.;
    f->has_converged = false;
    
    f->origin = transformation();
    f->origin_gravity_aligned = true;
    
    f->s.statesize = 0;
    f->s.enable_orientation_only();
    f->s.remap();
}

void filter_get_device_parameters(const struct filter *f, device_parameters *device)
{
    auto &cam = device->color;
    auto &imu = device->imu;

    device->depth = f->depth;

    device->version = CALIBRATION_VERSION;
    cam.intrinsics.width_px  = f->s.image_width;
    cam.intrinsics.height_px = f->s.image_height;
    cam.intrinsics.f_x_px = f->s.focal_length.v * f->s.image_height;
    cam.intrinsics.f_y_px = f->s.focal_length.v * f->s.image_height;
    cam.intrinsics.c_x_px = f->s.center_x.v * f->s.image_height + f->s.image_width / 2. - .5;
    cam.intrinsics.c_y_px = f->s.center_y.v * f->s.image_height + f->s.image_height / 2. - .5;

    if (f->s.fisheye) {
        cam.intrinsics.type = rc_CALIBRATION_TYPE_FISHEYE;
        cam.intrinsics.w = f->s.k1.v;
    } else {
        cam.intrinsics.type = rc_CALIBRATION_TYPE_POLYNOMIAL3;
        cam.intrinsics.k1 = f->s.k1.v;
        cam.intrinsics.k2 = f->s.k2.v;
        cam.intrinsics.k3 = f->s.k3.v;
    }

    cam.extrinsics_wrt_imu_m.Q = f->s.Qc.v;
    cam.extrinsics_wrt_imu_m.T = f->s.Tc.v;
    cam.extrinsics_var_wrt_imu_m.W = f->s.Qc.variance().segment<3>(0);
    cam.extrinsics_var_wrt_imu_m.T = f->s.Tc.variance().segment<3>(0);

    imu.a_bias_m__s2         = f->s.a_bias.v.segment<3>(0);
    imu.w_bias_rad__s        = f->s.w_bias.v.segment<3>(0);
    imu.a_bias_var_m2__s4    = f->s.a_bias.variance().segment<3>(0);
    imu.w_bias_var_rad2__s2  = f->s.w_bias.variance().segment<3>(0);
    imu.w_noise_var_rad2__s2 = f->w_variance;
    imu.a_noise_var_m2__s4   = f->a_variance;
    imu.a_alignment          = f->a_alignment.block<3,3>(0,0);
    imu.w_alignment          = f->w_alignment.block<3,3>(0,0);
}

float filter_converged(const struct filter *f)
{
    if(f->run_state == RCSensorFusionRunStateSteadyInitialization) {
        if(f->stable_start == sensor_clock::micros_to_tp(0)) return 0.;
        float progress = (f->last_time - f->stable_start) / std::chrono::duration_cast<std::chrono::duration<float>>(steady_converge_time);
        if(progress >= .99f) return 0.99f; //If focus takes a long time, we won't know how long it will take
        return progress;
    } else if(f->run_state == RCSensorFusionRunStatePortraitCalibration) {
        return get_bias_convergence(f, 1);
    } else if(f->run_state == RCSensorFusionRunStateLandscapeCalibration) {
        return get_bias_convergence(f, 0);
    } else if(f->run_state == RCSensorFusionRunStateStaticCalibration) {
        return fmin(f->accel_stability.count / (float)calibration_converge_samples, get_bias_convergence(f, 2));
    } else if(f->run_state == RCSensorFusionRunStateRunning || f->run_state == RCSensorFusionRunStateDynamicInitialization) { // TODO: proper progress for dynamic init, if needed.
        return 1.;
    } else return 0.;
}

bool filter_is_steady(const struct filter *f)
{
    return
        f->s.V.v.norm() < .1 &&
        f->s.w.v.norm() < .1;
}

int filter_get_features(const struct filter *f, struct feature_info *features, int max)
{
    int index = 0;
    for(state_vision_feature *i : f->s.features) {
        if(index >= max) break;
        features[index].id = i->id;
        features[index].x = (float)i->current[0];
        features[index].y = (float)i->current[1];
        features[index].wx = (float)i->world[0];
        features[index].wy = (float)i->world[1];
        features[index].wz = (float)i->world[2];
        features[index].stdev = (float)i->v.stdev_meters(sqrt(i->variance()));
        ++index;
    }
    return index;
}

void filter_start_static_calibration(struct filter *f)
{
    reset_stability(f);
    f->a_bias_start = f->s.a_bias.variance();
    f->w_bias_start = f->s.w_bias.variance();
    f->run_state = RCSensorFusionRunStateStaticCalibration;
}

void filter_start_hold_steady(struct filter *f)
{
    reset_stability(f);
    f->run_state = RCSensorFusionRunStateSteadyInitialization;
}

void filter_start_dynamic(struct filter *f)
{
    f->want_start = f->last_time;
    f->run_state = RCSensorFusionRunStateDynamicInitialization;
}

void filter_start_inertial_only(struct filter *f)
{
    f->run_state = RCSensorFusionRunStateInertialOnly;
    f->s.enable_orientation_only();
}

#ifdef ENABLE_QR
void filter_start_qr_detection(struct filter *f, const std::string& data, float dimension, bool use_gravity)
{
    f->origin_gravity_aligned = use_gravity;
    f->qr.start(data, dimension);
    f->last_qr_time = sensor_clock::micros_to_tp(0);
}

void filter_stop_qr_detection(struct filter *f)
{
    f->qr.stop();
}

void filter_start_qr_benchmark(struct filter * f, float qr_size_m)
{
    f->qr_bench.start(qr_size_m);
}
#endif
