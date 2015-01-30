// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#include <algorithm>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
extern "C" {
#include "../cor/cor.h"
}
#include "state_vision.h"
#include "../numerics/vec4.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "../numerics/kalman.h"
#include "../numerics/matrix.h"
#include "observation.h"
#include "filter.h"

int state_node::statesize;
int state_node::maxstatesize;

const static uint64_t min_steady_time = 100000; //time held steady before we start treating it as steady
const static uint64_t steady_converge_time = 200000; //time that user needs to hold steady (us)
const static int calibration_converge_samples = 200; //number of accelerometer readings needed to converge in calibration mode
const static f_t accelerometer_steady_var = .15*.15; //variance when held steady, based on std dev measurement of iphone 5s held in hand
const static f_t velocity_steady_var = .1 * .1; //initial var of state.V when steady
const static f_t accelerometer_inertial_var = 2.33*2.33; //variance when in inertial only mode
const static f_t static_sigma = 6.; //how close to mean measurements in static mode need to be
const static f_t steady_sigma = 3.; //how close to mean measurements in steady mode need to be - lower because it is handheld motion, not gaussian noise
const static f_t dynamic_W_thresh_variance = 5.e-2; // variance of W must be less than this to initialize from dynamic mode
//a_bias_var for best results on benchmarks is 6.4e-3
const static f_t min_a_bias_var = 1.e-4; // calibration will finish immediately when variance of a_bias is less than this, and it is reset to this between each run
const static f_t min_w_bias_var = 1.e-6; // variance of w_bias is reset to this between each run
const static f_t max_accel_delta = 10.; //This is biggest jump seen in hard shaking of device
const static f_t max_gyro_delta = 5.; //This is biggest jump seen in hard shaking of device
const static uint64_t qr_detect_period = 100000; //Time between checking frames for QR codes to reduce CPU usage
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
                if (log_enabled) fprintf(stderr, "%d\t%d\t: sign flip: expected %e, got %e\n", i, j, ldiff, delta);
                continue;
            }
            f_t error = fabs(ldiff - delta);
            if(fabs(delta)) error /= fabs(delta);
            else error /= 1.e-5;
            if(error > .1) {
                if (log_enabled) fprintf(stderr, "%d\t%d\t: lin error: expected %e, got %e\n", i, j, ldiff, delta);
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
                if (log_enabled) fprintf(stderr, "%d\t%d\t: sign flip: expected %e, got %e\n", i, j, ldiff, delta);
                continue;
            }
            f_t error = fabs(ldiff - delta);
            if(fabs(delta)) error /= fabs(delta);
            else error /= 1.e-5;
            if(error > .1) {
                if (log_enabled) fprintf(stderr, "%d\t%d\t: lin error: expected %e, got %e\n", i, j, ldiff, delta);
                continue;
            }
        }
    }
    f->s.copy_state_from_array(save_state);
}
*/

void filter_update_outputs(struct filter *f, uint64_t time)
{
    if(f->run_state != RCSensorFusionRunStateRunning) return;
    if(f->output) {
        packet_t *packet = mapbuffer_alloc(f->output, packet_filter_position, 6 * sizeof(float));
        float *output = (float *)packet->data;
        output[0] = f->s.T.v[0];
        output[1] = f->s.T.v[1];
        output[2] = f->s.T.v[2];
        output[3] = f->s.W.v.raw_vector()[0];
        output[4] = f->s.W.v.raw_vector()[1];
        output[5] = f->s.W.v.raw_vector()[2];
        mapbuffer_enqueue(f->output, packet, time);
    }
    m4 
        R = to_rotation_matrix(f->s.W.v),
        Rt = transpose(R),
        Rbc = to_rotation_matrix(f->s.Wc.v),
        Rcb = transpose(Rbc),
        RcbRt = Rcb * Rt;

    f->s.camera_matrix = RcbRt;
    v4 T = Rcb * ((Rt * -f->s.T.v) - f->s.Tc.v);
    f->s.camera_matrix[0][3] = T[0];
    f->s.camera_matrix[1][3] = T[1];
    f->s.camera_matrix[2][3] = T[2];
    f->s.camera_matrix[3][3] = 1.;

    bool old_speedfail = f->speed_failed;
    f->speed_failed = false;
    f_t speed = norm(f->s.V.v);
    if(speed > 3.) { //1.4m/s is normal walking speed
        if (log_enabled && !old_speedfail) fprintf(stderr, "Velocity %f m/s exceeds max bound\n", speed);
        f->speed_failed = true;
        f->calibration_bad = true;
    } else if(speed > 2.) {
        if (log_enabled && !f->speed_warning) fprintf(stderr, "High velocity (%f m/s) warning\n", speed);
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    f_t accel = norm(f->s.a.v);
    if(accel > 9.8) { //1g would saturate sensor anyway
        if (log_enabled && !old_speedfail) fprintf(stderr, "Acceleration exceeds max bound\n");
        f->speed_failed = true;
        f->calibration_bad = true;
    } else if(accel > 5.) { //max in mine is 6.
        if (log_enabled && !f->speed_warning) fprintf(stderr, "High acceleration (%f m/s^2) warning\n", accel);
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    f_t ang_vel = norm(f->s.w.v);
    if(ang_vel > 5.) { //sensor saturation - 250/180*pi
        if (log_enabled && !old_speedfail) fprintf(stderr, "Angular velocity exceeds max bound\n");
        f->speed_failed = true;
        f->calibration_bad = true;
    } else if(ang_vel > 2.) { // max in mine is 1.6
        if (log_enabled && !f->speed_warning) fprintf(stderr, "High angular velocity warning\n");
        f->speed_warning = true;
        f->speed_warning_time = time;
    }
    //if(f->speed_warning && filter_converged(f) < 1.) f->speed_failed = true;
    if(time - f->speed_warning_time > 1000000) f->speed_warning = false;

    //if (log_enabled) fprintf(stderr, "%d [%f %f %f] [%f %f %f]\n", time, output[0], output[1], output[2], output[3], output[4], output[5]); 
}

void process_observation_queue(struct filter *f, uint64_t time)
{
    f->last_time = time;
    if(!f->observations.process(f->s, time)) {
        f->numeric_failed = true;
        f->calibration_bad = true;
    }
    filter_update_outputs(f, time);
}

void filter_compute_gravity(struct filter *f, double latitude, double altitude)
{
    assert(f); f->s.compute_gravity(latitude, altitude);
}

static bool check_packet_time(struct filter *f, uint64_t t, int type)
{
    if(t < f->last_packet_time) {
        if (log_enabled) fprintf(stderr, "Warning: received packets out of order: %d at %lld came first, then %d at %lld. delta %lld\n", f->last_packet_type, f->last_packet_time, type, t, f->last_packet_time - t);
        return false;
    }
    f->last_packet_time = t;
    f->last_packet_type = type;
    return true;
}

extern "C" void filter_imu_packet(void *_f, packet_t *p)
{
    if(p->header.type != packet_imu) return;
    struct filter *f = (struct filter *)_f;
    filter_accelerometer_measurement(f, (float *)&p->data, p->header.time);
    filter_gyroscope_measurement(f, (float *)&p->data + 3, p->header.time);
}

extern "C" void filter_accelerometer_packet(void *_f, packet_t *p)
{
    if(p->header.type != packet_accelerometer) return;
    filter_accelerometer_measurement((struct filter *)_f, (float *)&p->data, p->header.time);
}

void update_static_calibration(struct filter *f)
{
    if(f->accel_stability.count < calibration_converge_samples) return;
    v4 var = f->accel_stability.variance;
    f->a_variance = (var[0] + var[1] + var[2]) / 3.;
    var = f->gyro_stability.variance;
    f->w_variance = (var[0] + var[1] + var[2]) / 3.;
    //this updates even the one dof that can't converge in the filter for this orientation (since we were static)
    f->s.w_bias.v = f->gyro_stability.mean;
    f->s.w_bias.set_initial_variance(f->gyro_stability.variance[0], f->gyro_stability.variance[1], f->gyro_stability.variance[2]); //Even though the one dof won't have converged in the filter, we know that this is a good value (average across stable meas).
    f->s.w_bias.reset_covariance(f->s.cov);
}

static void reset_stability(struct filter *f)
{
    f->accel_stability = stdev_vector();
    f->gyro_stability = stdev_vector();
    f->stable_start = 0;
}

uint64_t steady_time(struct filter *f, stdev_vector &stdev, v4 meas, f_t variance, f_t sigma, uint64_t time, v4 orientation, bool use_orientation)
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
        if(!f->s.orientation_initialized) return 0;
        v4 local_up = transpose(to_rotation_matrix(f->s.W.v)) * v4(0., 0., 1., 0.);
        //face up -> (0, 0, 1)
        //portrait -> (0, 1, 0)
        //landscape -> (1, 0, 0)
        f_t costheta = sum(orientation * local_up);
        if(fabs(costheta) < .71) return 0; //don't start since we aren't in orientation +/- 6 deg
    }
    stdev.data(meas);
    
    return time - f->stable_start;
}

static void print_calibration(struct filter *f)
{
    fprintf(stderr, "w bias is: "); f->s.w_bias.v.print(); fprintf(stderr, "\n");
    fprintf(stderr, "w bias var is: "); f->s.w_bias.variance().print(); fprintf(stderr, "\n");
    fprintf(stderr, "a bias is: "); f->s.a_bias.v.print(); fprintf(stderr, "\n");
    fprintf(stderr, "a bias var is: "); f->s.a_bias.variance().print(); fprintf(stderr, "\n");
}

static float var_bounds_to_std_percent(f_t current, f_t begin, f_t end)
{
    return current < end ? 1. : (log(begin) - log(current)) / (log(begin) - log(end)); //log here seems to give smoother progress
}

static f_t get_bias_convergence(struct filter *f, int dir)
{
    f_t max_pct = var_bounds_to_std_percent(f->s.a_bias.variance()[dir], f->a_bias_start[dir], min_a_bias_var);
    f_t pct = f->accel_stability.count / (f_t)calibration_converge_samples;
    if(f->last_time - f->stable_start < min_steady_time) pct = 0.;
    if(pct > max_pct) max_pct = pct;
    if(max_pct < 0.) max_pct = 0.;
    if(max_pct > 1.) max_pct = 1.;
    return max_pct;
}

static f_t get_accelerometer_variance_for_run_state(struct filter *f, v4 meas, uint64_t time)
{
    if(!f->s.orientation_initialized) return accelerometer_inertial_var; //first measurement is not used, so this doesn't actually matter
    switch(f->run_state)
    {
        case RCSensorFusionRunStateRunning:
        case RCSensorFusionRunStateInactive: //shouldn't happen
            return f->a_variance;
        case RCSensorFusionRunStateDynamicInitialization:
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
#if log_enabled
                    fprintf(stderr, "When finishing static calibration:\n");
                    print_calibration(f);
#endif
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
            if(steady_time(f, f->accel_stability, meas, accelerometer_steady_var, steady_sigma, time, v4(0, 1, 0, 0), true) > min_steady_time)
            {
                f->s.enable_bias_estimation();
                if(get_bias_convergence(f, 1) >= 1.)
                {
                    f->run_state = RCSensorFusionRunStateLandscapeCalibration;
                    f->a_bias_start = f->s.a_bias.variance();
                    f->w_bias_start = f->s.w_bias.variance();
                    reset_stability(f);
                    f->s.disable_bias_estimation();
#if log_enabled
                    fprintf(stderr, "When finishing portrait calibration:\n");
                    print_calibration(f);
#endif
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
            if(steady_time(f, f->accel_stability, meas, accelerometer_steady_var, steady_sigma, time, v4(1, 0, 0, 0), true) > min_steady_time)
            {
                f->s.enable_bias_estimation();
                if(get_bias_convergence(f, 0) >= 1.)
                {
                    f->run_state = RCSensorFusionRunStateInactive;
                    reset_stability(f);
                    f->s.disable_bias_estimation();
#if log_enabled
                    fprintf(stderr, "When finishing landscape calibration:\n");
                    print_calibration(f);
#endif
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
            uint64_t steady = steady_time(f, f->accel_stability, meas, accelerometer_steady_var, steady_sigma, time, v4(), false);
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

void filter_accelerometer_measurement(struct filter *f, float data[3], uint64_t time)
{
    v4 meas(data[0], data[1], data[2], 0.);
    v4 accel_delta = meas - f->last_accel_meas;
    f->last_accel_meas = meas;
    //This will throw away both the outlier measurement and the next measurement, because we update last every time. This prevents setting last to an outlier and never recovering.
    if(f->run_state == RCSensorFusionRunStateInactive) return;
    if(!check_packet_time(f, time, packet_accelerometer)) return;
    if(!f->got_accelerometer) { //skip first packet - has been crap from gyro
        f->got_accelerometer = true;
        return;
    }
    if(!f->got_gyroscope) return;

    if(fabs(accel_delta[0]) > max_accel_delta || fabs(accel_delta[1]) > max_accel_delta || fabs(accel_delta[2]) > max_accel_delta)
    {
        if(log_enabled) fprintf(stderr, "Rejecting an accel sample due to extreme jump %f %f %f\n", accel_delta[0], accel_delta[1], accel_delta[2]);
        return;
    }
    
    if(!f->gravity_init) {
        f->gravity_init = true;
        //set up plots
        if(f->visbuf) {
            packet_plot_setup(f->visbuf, time, packet_plot_meas_a, "Meas-alpha", sqrt(f->a_variance));
            packet_plot_setup(f->visbuf, time, packet_plot_meas_w, "Meas-omega", sqrt(f->w_variance));
            packet_plot_setup(f->visbuf, time, packet_plot_inn_a, "Inn-alpha", sqrt(f->a_variance));
            packet_plot_setup(f->visbuf, time, packet_plot_inn_w, "Inn-omega", sqrt(f->w_variance));
        }
        
        //fix up groups and features that have already been added
/*        for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
            state_vision_group *g = *giter;
            g->Wr.v = f->s.W.v;
        }
        
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            i->initial = i->current;
            i->Wr = f->s.W.v;
        }*/
    }
    
    observation_spatial *obs_a;
    obs_a = new observation_accelerometer(f->s, time, time);
    
    for(int i = 0; i < 3; ++i) {
        obs_a->meas[i] = data[i];
    }
    
    f->observations.observations.push_back(obs_a);
    
    obs_a->variance = get_accelerometer_variance_for_run_state(f, meas, time);

    if(show_tuning) fprintf(stderr, "accelerometer:\n");
    process_observation_queue(f, time);
    if(show_tuning) {
        fprintf(stderr, " actual innov stdev is:\n");
        observation_accelerometer::inn_stdev.print();
        fprintf(stderr, " signal stdev is:\n");
        observation_accelerometer::stdev.print();
        fprintf(stderr, " bias is:\n");
        f->s.a_bias.v.print(); v4(f->s.a_bias.variance()).print();
    }
}

extern "C" void filter_gyroscope_packet(void *_f, packet_t *p)
{
    if(p->header.type != packet_gyroscope) return;
    filter_gyroscope_measurement((struct filter *)_f, (float *)&p->data, p->header.time);
}

void filter_gyroscope_measurement(struct filter *f, float data[3], uint64_t time)
{
    v4 meas(data[0], data[1], data[2], 0.);
    v4 gyro_delta = meas - f->last_gyro_meas;
    f->last_gyro_meas = meas;
    //This will throw away both the outlier measurement and the next measurement, because we update last every time. This prevents setting last to an outlier and never recovering.
    if(f->run_state == RCSensorFusionRunStateInactive) return;
    if(!check_packet_time(f, time, packet_gyroscope)) return;
    if(!f->got_gyroscope) { //skip the first piece of data as it seems to be crap
        f->got_gyroscope = true;
        return;
    }
    if(!f->gravity_init) return;

    if(fabs(gyro_delta[0]) > max_gyro_delta || fabs(gyro_delta[1]) > max_gyro_delta || fabs(gyro_delta[2]) > max_gyro_delta)
    {
        if(log_enabled) fprintf(stderr, "Rejecting a gyro sample due to extreme jump %f %f %f\n", gyro_delta[0], gyro_delta[1], gyro_delta[2]);
        return;
    }

    observation_gyroscope *obs_w = new observation_gyroscope(f->s, time, time);
    for(int i = 0; i < 3; ++i) {
        obs_w->meas[i] = data[i];
    }
    obs_w->variance = f->w_variance;
    f->observations.observations.push_back(obs_w);
    
    if(f->run_state == RCSensorFusionRunStateStaticCalibration) {
        f->gyro_stability.data(meas);
    }

    if(show_tuning) fprintf(stderr, "gyroscope:\n");
    process_observation_queue(f, time);
    if(show_tuning) {
        fprintf(stderr, " actual innov stdev is:\n");
        observation_gyroscope::inn_stdev.print();
        fprintf(stderr, " signal stdev is:\n");
        observation_gyroscope::stdev.print();
        fprintf(stderr, " bias is:\n");
        f->s.w_bias.v.print(); v4(f->s.w_bias.variance()).print();
        fprintf(stderr, "\n");
    }
}

static int filter_process_features(struct filter *f, uint64_t time)
{
    int useful_drops = 0;
    int total_feats = 0;
    int outliers = 0;
    int track_fail = 0;
    int toobig = f->s.statesize - f->s.maxstatesize;
    //TODO: revisit this - should check after dropping other features, make this more intelligent
    if(toobig > 0) {
        int dropped = 0;
        vector<f_t> vars;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            vars.push_back((*fiter)->variance());
        }
        std::sort(vars.begin(), vars.end());
        if(vars.size() > toobig) {
            f_t min = vars[vars.size() - toobig];
            for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
                if((*fiter)->variance() >= min) {
                    (*fiter)->status = feature_empty;
                    ++dropped;
                    if(dropped >= toobig) break;
                }
            }
            if (log_enabled) fprintf(stderr, "state is %d too big, dropped %d features, min variance %f\n",toobig, dropped, min);
        }
    }
    for(list<state_vision_feature *>::iterator fi = f->s.features.begin(); fi != f->s.features.end(); ++fi) {
        state_vision_feature *i = *fi;
        if(i->current[0] == INFINITY) {
            ++track_fail;
            if(i->is_good()) ++useful_drops;
            i->drop();
        } else {
            if(i->status == feature_normal || i->status == feature_reject) ++total_feats;
            if(i->outlier > i->outlier_reject || i->status == feature_reject) {
                i->status = feature_empty;
                ++outliers;
            }
        }
    }
    if(track_fail && !total_feats && log_enabled) fprintf(stderr, "Tracker failed! %d features dropped.\n", track_fail);
    //    if (log_enabled) fprintf(stderr, "outliers: %d/%d (%f%%)\n", outliers, total_feats, outliers * 100. / total_feats);
    if(useful_drops && f->output) {
        packet_t *sp = mapbuffer_alloc(f->output, packet_filter_reconstruction, useful_drops * 3 * sizeof(float));
        sp->header.user = useful_drops;
        packet_filter_feature_id_association_t *association = (packet_filter_feature_id_association_t *)mapbuffer_alloc(f->output, packet_filter_feature_id_association, useful_drops * sizeof(uint64_t));
        association->header.user = useful_drops;
        packet_feature_intensity_t *intensity = (packet_feature_intensity_t *)mapbuffer_alloc(f->output, packet_feature_intensity, useful_drops);
        intensity->header.user = useful_drops;
        float (*world)[3] = (float (*)[3])sp->data;
        int nfeats = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            if(i->status == feature_gooddrop) {
                world[nfeats][0] = i->world[0];
                world[nfeats][1] = i->world[1];
                world[nfeats][2] = i->world[2];
                association->feature_id[nfeats] = i->id;
                intensity->intensity[nfeats] = i->intensity;
                ++nfeats;
            }
        }
        mapbuffer_enqueue(f->output, sp, time);
        mapbuffer_enqueue(f->output, (packet_t *)association, time);
        mapbuffer_enqueue(f->output, (packet_t *)intensity, time);
    }

    int features_used = f->s.process_features(time);
    if(!features_used)
    {
        //Lost all features - reset convergence
        f->has_converged = false;
        f->max_velocity = 0.;
        f->median_depth_variance = 1.;
    }

    //clean up dropped features and groups
    list<state_vision_feature *>::iterator fi = f->s.features.begin();
    while(fi != f->s.features.end()) {
        state_vision_feature *i = *fi;
        if(i->status == feature_gooddrop) {
            if(f->recognition_buffer) {
                packet_recognition_feature_t *rp = (packet_recognition_feature_t *)mapbuffer_alloc(f->recognition_buffer, packet_recognition_feature, sizeof(packet_recognition_feature_t));
                rp->groupid = i->groupid;
                rp->id = i->id;
                rp->ix = i->initial[0];
                rp->iy = i->initial[1];
                rp->x = i->relative[0];
                rp->y = i->relative[1];
                rp->z = i->relative[2];
                rp->depth = i->v.depth();
                f_t var = i->measurement_var < i->variance() ? i->variance() : i->measurement_var;
                //for measurement var, the values are simply scaled by depth, so variance multiplies by depth^2
                //for depth variance, d/dx = e^x, and the variance is v*(d/dx)^2
                rp->variance = var * rp->depth * rp->depth;
                mapbuffer_enqueue(f->recognition_buffer, (packet_t *)rp, time);
            }
        }
        if(i->status == feature_gooddrop) i->status = feature_empty;
        if(i->status == feature_reject) i->status = feature_empty;
        if(i->status == feature_empty) {
            delete i;
            fi = f->s.features.erase(fi);
        } else ++fi;
    }

    list<state_vision_group *>::iterator giter = f->s.groups.children.begin();
    while(giter != f->s.groups.children.end()) {
        state_vision_group *g = *giter;
        if(g->status == group_empty) {
            if(f->recognition_buffer) {
                packet_recognition_group_t *pg = (packet_recognition_group_t *)mapbuffer_alloc(f->recognition_buffer, packet_recognition_group, sizeof(packet_recognition_group_t));
                pg->id = g->id;
                pg->header.user = 1;
                mapbuffer_enqueue(f->recognition_buffer, (packet_t *)pg, time);
            }
            delete g;
            giter = f->s.groups.children.erase(giter);
        } else {
            if(g->status == group_initializing) {
                for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
                    if (log_enabled) fprintf(stderr, "calling triangulate feature from process\n");
                    assert(0);
                    //triangulate_feature(&(f->s), i);
                }
            }
            ++giter;
        }
    }

    f->s.remap();

    return features_used;
}

bool feature_variance_comp(state_vision_feature *p1, state_vision_feature *p2) {
    return p1->variance() < p2->variance();
}

void filter_setup_next_frame(struct filter *f, uint64_t time)
{
    size_t feats_used = f->s.features.size();

    if(f->run_state != RCSensorFusionRunStateRunning) return;

    if(feats_used) {
        int fi = 0;
        for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
            state_vision_group *g = *giter;
            if(!g->status || g->status == group_initializing) continue;
            for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
                state_vision_feature *i = *fiter;
                uint64_t extra_time = f->shutter_delay + i->current[1]/f->image_height * f->shutter_period;
                observation_vision_feature *obs = new observation_vision_feature(f->s, time + extra_time, time);
                obs->state_group = g;
                obs->feature = i;
                obs->meas[0] = i->current[0];
                obs->meas[1] = i->current[1];
                obs->im1 = f->track.im1;
                obs->im2 = f->track.im2;
                obs->tracker = f->track;

                f->observations.observations.push_back(obs);

                fi += 2;
            }
        }
    }
    //TODO: implement feature_single ?
}

void filter_send_output(struct filter *f, uint64_t time)
{
    if(!f->output) return;
    size_t nfeats = f->s.features.size();
    packet_filter_current_t *cp = (packet_filter_current_t *)mapbuffer_alloc(f->output, packet_filter_current, (uint32_t)(sizeof(packet_filter_current) - 16 + nfeats * 3 * sizeof(float)));
    int n_good_feats = 0;
    for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
        state_vision_group *g = *giter;
        if(g->status == group_initializing) continue;
        for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            if(!i->status || i->status == feature_reject) continue;
            cp->points[n_good_feats][0] = i->world[0];
            cp->points[n_good_feats][1] = i->world[1];
            cp->points[n_good_feats][2] = i->world[2];
            ++n_good_feats;
        }
    }
    cp->header.user = n_good_feats;
    mapbuffer_enqueue(f->output, (packet_t*)cp, time);
    
    packet_filter_feature_id_visible_t *visible = (packet_filter_feature_id_visible_t *)mapbuffer_alloc(f->output, packet_filter_feature_id_visible, (uint32_t)(sizeof(packet_filter_feature_id_visible_t) - 16 + nfeats * sizeof(uint64_t)));
    for(int i = 0; i < 3; ++i) {
        visible->T[i] = f->s.T.v[i];
        visible->W[i] = f->s.W.v.raw_vector()[i];
    }
    n_good_feats = 0;
    for(list<state_vision_group *>::iterator giter = f->s.groups.children.begin(); giter != f->s.groups.children.end(); ++giter) {
        state_vision_group *g = *giter;
        if(g->status == group_initializing) continue;
        for(list<state_vision_feature *>::iterator fiter = g->features.children.begin(); fiter != g->features.children.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            if(!i->status || i->status == feature_reject) continue;
            visible->feature_id[n_good_feats] = i->id;
            ++n_good_feats;
        }
    }
    visible->header.user = n_good_feats;
    mapbuffer_enqueue(f->output, (packet_t*)visible, time);
}

//features are added to the state immediately upon detection - handled with triangulation in observation_vision_feature::predict - but what is happening with the empty row of the covariance matrix during that time?
static void addfeatures(struct filter *f, size_t newfeats, unsigned char *img, unsigned int width, int height, uint64_t time)
{
#ifdef TEST_POSDEF
    if(!test_posdef(f->s.cov.cov)) fprintf(stderr, "not pos def before adding features\n");
#endif
    // Filter out features which we already have by masking where
    // existing features are located
    if(!f->scaled_mask) f->scaled_mask = new scaled_mask(width, height);
    f->scaled_mask->initialize();
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        f->scaled_mask->clear((*fiter)->current[0], (*fiter)->current[1]);
    }

    // Run detector
    vector<xy> &kp = f->track.detect(img, f->scaled_mask, (int)newfeats, 0, 0, width, height);

    // Check that the detected features don't collide with the mask
    // and add them to the filter
    if(kp.size() < newfeats) newfeats = kp.size();
    if(newfeats < state_vision_group::min_feats) return;
    state_vision_group *g = f->s.add_group(time);

    int found_feats = 0;
    for(int i = 0; i < kp.size(); ++i) {
        int x = kp[i].x;
        int y = kp[i].y;
        if(x > 0 && y > 0 && x < width-1 && y < height-1 && f->scaled_mask->test(x, y)) {
            f->scaled_mask->clear(x, y);
            state_vision_feature *feat = f->s.add_feature(x, y);
            int lx = floor(x);
            int ly = floor(y);
            feat->intensity = (((unsigned int)img[lx + ly*width]) + img[lx + 1 + ly * width] + img[lx + width + ly * width] + img[lx + 1 + width + ly * width]) >> 2;
            int half_patch = f->track.half_patch_width;
            int full_patch = 2 * half_patch + 1;
            for(int py = 0; py < full_patch; ++py)
            {
                for(int px = 0; px <= full_patch; ++px)
                {
                    feat->patch[py * full_patch + px] = img[lx + px - half_patch + (ly + py - half_patch) * width];
                }
            }
            g->features.children.push_back(feat);
            feat->groupid = g->id;
            feat->found_time = time;
            
            found_feats++;
            if(found_feats == newfeats) break;
        }
    }
    g->status = group_initializing;
    g->make_normal();
    f->s.remap();
#ifdef TEST_POSDEF
    if(!test_posdef(f->s.cov.cov)) fprintf(stderr, "not pos def after adding features\n");
#endif
}

void send_current_features_packet(struct filter *f, uint64_t time)
{
    const static f_t chi_square_95 = 5.991;
    //const static f_t chi_suqare_99 = 9.210;

    if(!f->track.sink) return;
    if(f->visbuf) {
        packet_feature_prediction_variance_t *predicted = (packet_feature_prediction_variance_t *)mapbuffer_alloc(f->track.sink, packet_feature_prediction_variance, (uint32_t)(f->s.features.size() * sizeof(feature_covariance_t)));
        int nfeats = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            //http://math.stackexchange.com/questions/8672/eigenvalues-and-eigenvectors-of-2-times-2-matrix
            f_t x = (*fiter)->innovation_variance_x;
            f_t y = (*fiter)->innovation_variance_y;
            f_t xy = (*fiter)->innovation_variance_xy;
            f_t tau = (y - x) / xy / 2.;
            f_t t = (tau >= 0.) ? (1. / (fabs(tau) + sqrt(1. + tau * tau))) : (-1. / (fabs(tau) + sqrt(1. + tau * tau)));
            f_t c = 1. / sqrt(1 + tau * tau);
            f_t s = c * t;
            f_t l1 = x - t * xy;
            f_t l2 = y + t * xy;
            f_t theta = atan2(-s, c);
            
            predicted->covariance[nfeats].x = (*fiter)->prediction.x;
            predicted->covariance[nfeats].y = (*fiter)->prediction.y;
            predicted->covariance[nfeats].cx = 2. * sqrt(l1 * chi_square_95);
            predicted->covariance[nfeats].cy = 2. * sqrt(l2 * chi_square_95);
            predicted->covariance[nfeats].cxy = theta;
            ++nfeats;
        }
        predicted->header.user = f->s.features.size();
        mapbuffer_enqueue(f->track.sink, (packet_t *)predicted, time);
        packet_t *status = mapbuffer_alloc(f->visbuf, packet_feature_status, (uint32_t)f->s.features.size());
        nfeats = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            if((*fiter)->outlier) status->data[nfeats] = 0;
            else if((*fiter)->is_initialized()) status->data[nfeats] = 1;
            else status->data[nfeats] = 2;
            ++nfeats;
        }
        status->header.user = f->s.features.size();
        mapbuffer_enqueue(f->visbuf, status, time);
    }
    
    packet_t *tracked = mapbuffer_alloc(f->track.sink, packet_feature_track, (uint32_t)(f->s.features.size() * sizeof(feature_t)));
    feature_t *trackedfeats = (feature_t *)tracked->data;
    int nfeats = 0;
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        trackedfeats[nfeats].x = (*fiter)->current[0];
        trackedfeats[nfeats].y = (*fiter)->current[1];
        ++nfeats;
    }
    tracked->header.user = f->s.features.size();
    mapbuffer_enqueue(f->track.sink, tracked, time);
}

void filter_set_reference(struct filter *f)
{
    f->s.reset_position();
}

extern "C" void filter_control_packet(void *_f, packet_t *p)
{
    if(p->header.type != packet_filter_control) return;
    struct filter *f = (struct filter *)_f;
    //ignore full filter reset - can't do from here and may not make sense anymore
    /*if(p->header.user == 2) {
        //full reset
        if (log_enabled) fprintf(stderr, "full filter reset\n");
        filter_reset_full(f);
    }*/
    if(p->header.user == 1) {
        //start measuring
        if (log_enabled) fprintf(stderr, "measurement starting\n");
        filter_set_reference(f);
    }
    if(p->header.user == 0) {
        //stop measuring
        if (log_enabled) fprintf(stderr, "measurement stopping\n");
        //ignore
    }
}

#include <mach/mach.h>

bool filter_image_measurement(struct filter *f, unsigned char *data, int width, int height, int stride, uint64_t time)
{
    if(f->run_state == RCSensorFusionRunStateInactive) return false;
    if(!check_packet_time(f, time, packet_camera)) return false;
    if(!f->got_accelerometer || !f->got_gyroscope) return false;
    
    if(!f->valid_time) {
        f->first_time = time;
        f->valid_time = true;
    }

    if(f->qr.running && (time - f->last_qr_time > qr_detect_period)) {
        f->last_qr_time = time;
        f->qr.process_frame(f, data, width, height);
    }

    f->got_image = true;
    if(f->run_state == RCSensorFusionRunStateDynamicInitialization) {
        if(f->want_start == 0) f->want_start = time;
        bool inertial_converged = (f->s.W.variance()[0] < dynamic_W_thresh_variance && f->s.W.variance()[1] < dynamic_W_thresh_variance);
        if(inertial_converged) {
            if(log_enabled) {
                if(inertial_converged) {
                    fprintf(stderr, "Inertial converged at time %lld\n", time - f->want_start);
                } else {
                    fprintf(stderr, "Inertial did not converge %f, %f\n", f->s.W.variance()[0], f->s.W.variance()[1]);
                }
            }
        } else return true;
    }
    if(f->run_state == RCSensorFusionRunStateSteadyInitialization) {
        if(f->stable_start == 0) return true;
        if(time - f->stable_start < steady_converge_time) return true;
    }
    if(f->run_state != RCSensorFusionRunStateRunning && f->run_state != RCSensorFusionRunStateDynamicInitialization && f->run_state != RCSensorFusionRunStateSteadyInitialization) return true; //frame was "processed" so that callbacks still get called
    if(width != f->track.width || height != f->track.height || stride != f->track.stride) {
        fprintf(stderr, "Image dimensions don't match what we expect!\n");
        abort();
    }
    
    if(!f->ignore_lateness) {
        /*thread_info_data_t thinfo;
        mach_msg_type_number_t thinfo_count;
        kern_return_t kr = thread_info(mach_thread_self(), THREAD_BASIC_INFO, thinfo, &thinfo_count);
        float cpu = ((thread_basic_info_t)thinfo)->cpu_usage / (float)TH_USAGE_SCALE;
        fprintf(stderr, "cpu usage is %f\n", cpu);*/
        
        int64_t current = cor_time();
        int64_t delta = current - (time - f->first_time);
        if(!f->valid_delta) {
            f->mindelta = delta;
            f->valid_delta = true;
        }
        if(delta < f->mindelta) {
            f->mindelta = delta;
        }
        int64_t lateness = delta - f->mindelta;
        int64_t period = time - f->last_arrival;
        f->last_arrival = time;
        
        if(lateness > period * 2) {
            if (log_enabled) fprintf(stderr, "old max_state_size was %d\n", f->s.maxstatesize);
            f->s.maxstatesize = f->s.statesize - 1;
            if(f->s.maxstatesize < MINSTATESIZE) f->s.maxstatesize = MINSTATESIZE;
            f->maxfeats = f->s.maxstatesize - 10;
            if (log_enabled) fprintf(stderr, "was %lld us late, new max state size is %d, current state size is %d\n", lateness, f->s.maxstatesize, f->s.statesize);
            if (log_enabled) fprintf(stderr, "dropping a frame!\n");
            return false;
        }
        if(lateness > period && f->s.maxstatesize > MINSTATESIZE && f->s.statesize < f->s.maxstatesize) {
            f->s.maxstatesize = f->s.statesize - 1;
            if(f->s.maxstatesize < MINSTATESIZE) f->s.maxstatesize = MINSTATESIZE;
            f->maxfeats = f->s.maxstatesize - 10;
            if (log_enabled) fprintf(stderr, "was %lld us late, new max state size is %d, current state size is %d\n", lateness, f->s.maxstatesize, f->s.statesize);
        }
        if(lateness < period / 4 && f->s.statesize > f->s.maxstatesize - f->min_group_add && f->s.maxstatesize < MAXSTATESIZE - 1) {
            ++f->s.maxstatesize;
            f->maxfeats = f->s.maxstatesize - 10;
            if (log_enabled) fprintf(stderr, "was %lld us late, new max state size is %d, current state size is %d\n", lateness, f->s.maxstatesize, f->s.statesize);
        }
    }

    f->track.im1 = f->track.im2;
    f->track.im2 = data;

    filter_setup_next_frame(f, time);

    if(show_tuning) {
        fprintf(stderr, "vision:\n");
    }
    process_observation_queue(f, time);
    if(show_tuning) {
        fprintf(stderr, " actual innov stdev is:\n");
        observation_vision_feature::inn_stdev[0].print();
        observation_vision_feature::inn_stdev[1].print();
    }

    filter_process_features(f, time);
    filter_send_output(f, time);
    send_current_features_packet(f, time);

    if(f->s.estimate_calibration && !f->estimating_Tc && time - f->active_time > 2000000)
    {
        //TODO: leaving Tc out of the state now. This gain scheduling is wrong (crash when adding tc back in if state is full).
        f->s.children.push_back(&f->s.Tc);
        f->s.remap();
        f->estimating_Tc = true;
    }

    int space = f->s.maxstatesize - f->s.statesize - 6;
    if(space > f->max_group_add) space = f->max_group_add;
    if(space >= f->min_group_add) {
        if(f->run_state == RCSensorFusionRunStateDynamicInitialization || f->run_state == RCSensorFusionRunStateSteadyInitialization) {
#ifdef TEST_POSDEF
            if(!test_posdef(f->s.cov.cov)) fprintf(stderr, "not pos def before disabling orient only\n");
#endif
            f->s.disable_orientation_only();
            if(f->s.estimate_calibration) {
                f->s.remove_child(&(f->s.Tc));
                f->s.remap();
                f->estimating_Tc = false;
            }
#ifdef TEST_POSDEF
            if(!test_posdef(f->s.cov.cov)) fprintf(stderr, "not pos def after disabling orient only\n");
#endif
        }
        addfeatures(f, space, data, f->track.width, f->track.height, time);
        if(f->s.features.size() < state_vision_group::min_feats) {
            if (log_enabled) fprintf(stderr, "detector failure: only %ld features after add\n", f->s.features.size());
            f->detector_failed = true;
            f->calibration_bad = true;
            if(f->run_state == RCSensorFusionRunStateDynamicInitialization || f->run_state == RCSensorFusionRunStateSteadyInitialization) f->s.enable_orientation_only();
        } else {
            //don't go active until we can successfully add features
            if(f->run_state == RCSensorFusionRunStateDynamicInitialization || f->run_state == RCSensorFusionRunStateSteadyInitialization) {
                f->run_state = RCSensorFusionRunStateRunning;
#if log_enabled
                fprintf(stderr, "When moving from steady init to running:\n");
                print_calibration(f);
#endif
                f->active_time = time;
            }
            f->detector_failed = false;
        }
    }
    
    vector<state_vision_feature *> useful_feats;
    for(auto i: f->s.features)
    {
        if(i->is_initialized()) useful_feats.push_back(i);
    }
    
    if(useful_feats.size())
    {
        sort(useful_feats.begin(), useful_feats.end(), [](state_vision_feature *a, state_vision_feature *b) { return a->variance() < b->variance(); });
        f->median_depth_variance = useful_feats[useful_feats.size() / 2]->variance();
    }
    else
    {
        f->median_depth_variance = 1.;
    }
    float velocity = norm(f->s.V.v);
    if(velocity > f->max_velocity) f->max_velocity = velocity;
    
    if(f->max_velocity > convergence_minimum_velocity && f->median_depth_variance < convergence_maximum_depth_variance) f->has_converged = true;
    
    return true;
}

extern "C" void filter_image_packet(void *_f, packet_t *p)
{
    if(p->header.type != packet_camera) return;
    struct filter *f = (struct filter *)_f;
    if(!f->track.width) {
        int width, height;
        sscanf((char *)p->data, "P5 %d %d", &width, &height);
        f->track.width = width;
        f->track.height = height;
        f->track.stride = width;
    }
    filter_image_measurement(f, p->data + 16, f->track.width, f->track.height, f->track.stride, p->header.time);
}

extern "C" void filter_features_added_packet(void *_f, packet_t *p)
{
    struct filter *f = (struct filter *)_f;
    if(p->header.type == packet_feature_select) {
        feature_t *initial = (feature_t*) p->data;
        for(int i = 0; i < p->header.user; ++i) {
            state_vision_feature *feat = f->s.add_feature(initial[i].x, initial[i].y);
            assert(initial[i].x != INFINITY);
            feat->status = feature_initializing;
            feat->current[0] = initial[i].x;
            feat->current[1] = initial[i].y;
        }
        f->s.remap();
    }
    if(p->header.type == packet_feature_intensity) {
        uint8_t *intensity = (uint8_t *)p->data;
        list<state_vision_feature *>::iterator fiter = f->s.features.end();
        --fiter;
        for(int i = p->header.user; i > 0; --i) {
            (*fiter)->intensity = intensity[i];
        }
        /*  
        int feature_base = f->s.features.size() - p->header.user;
        //        list<state_vision_feature *>::iterator fiter = f->s.featuresf->s.features.end()-p->header.user;
        for(int i = 0; i < p->header.user; ++i) {
            f->s.features[feature_base + i]->intensity = intensity[i];
            }*/
    }
}

/*static double a_bias_stdev = .02 * 9.8; //20 mg
static double BEGIN_ABIAS_VAR = a_bias_stdev * a_bias_stdev;
static double w_bias_stdev = 10. / 180. * M_PI; //10 dps
static double BEGIN_WBIAS_VAR = w_bias_stdev * w_bias_stdev;*/

#define BEGIN_FOCAL_VAR 10.
#define END_FOCAL_VAR .3
#define BEGIN_C_VAR 2.
#define END_C_VAR .16
#define BEGIN_ABIAS_VAR 1.e-5
#define END_ABIAS_VAR 1.e-6
#define BEGIN_WBIAS_VAR 1.e-7
#define END_WBIAS_VAR 1.e-8
#define BEGIN_K1_VAR 2.e-4
#define END_K1_VAR 1.e-5
#define BEGIN_K2_VAR 2.e-4
#define BEGIN_K3_VAR 1.e-4

//This should be called every time we want to initialize or reset the filter
extern "C" void filter_initialize(struct filter *f, struct corvis_device_parameters device)
{
    //changing these two doesn't affect much.
    f->min_group_add = 16;
    f->max_group_add = 40;
    
    f->shutter_delay = 0;
    f->shutter_period = 0;
    
    f->w_variance = device.w_meas_var;
    f->a_variance = device.a_meas_var;

    state_vision_feature::initial_depth_meters = M_E;
    state_vision_feature::initial_var = .75;
    state_vision_feature::initial_process_noise = 1.e-20;
    state_vision_feature::measurement_var = 1.5 * 1.5;
    state_vision_feature::outlier_thresh = 2;
    state_vision_feature::outlier_reject = 30.;
    state_vision_feature::max_variance = .10 * .10; //because of log-depth, the standard deviation is approximately a percentage (so .10 * .10 = 10%)
    state_vision_feature::min_add_vis_cov = .5;
    state_vision_group::ref_noise = 1.e-30;
    state_vision_group::min_feats = 1;
    
    state_vision_group::counter = 0;
    state_vision_feature::counter = 0;

    observation_vision_feature::stdev[0] = stdev_scalar();
    observation_vision_feature::stdev[1] = stdev_scalar();
    observation_vision_feature::inn_stdev[0] = stdev_scalar();
    observation_vision_feature::inn_stdev[1] = stdev_scalar();
    observation_accelerometer::stdev = stdev_vector();
    observation_accelerometer::inn_stdev = stdev_vector();
    observation_gyroscope::stdev = stdev_vector();
    observation_gyroscope::inn_stdev = stdev_vector();

    f->last_time = 0;
    f->last_packet_time = 0;
    f->last_packet_type = 0;
    f->gravity_init = false;
    f->want_start = 0;
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
    f->speed_warning_time = 0;
    f->gyro_stability = stdev_vector();
    f->accel_stability = stdev_vector();
    
    f->stable_start = 0;
    f->calibration_bad = false;
    
    f->valid_time = 0;
    f->first_time = 0;
    
    f->mindelta = 0;
    f->valid_delta = false;
    
    f->last_arrival = 0;
    f->active_time = 0;
    f->estimating_Tc = false;
    
    f->image_height = 0;
    f->image_width = 0;
    
    if(f->scaled_mask)
    {
        delete f->scaled_mask;
        f->scaled_mask = 0;
    }
    
    f->observations.clear();

    f->s.reset();
    f->s.maxstatesize = 120;
    f->maxfeats = 70;

    f->s.Tc.v = v4(device.Tc[0], device.Tc[1], device.Tc[2], 0.);
    f->s.Wc.v = rotation_vector(device.Wc[0], device.Wc[1], device.Wc[2]);

    //TODO: This is wrong
    f->s.Wc.set_initial_variance(device.Wc_var[0], device.Wc_var[1], device.Wc_var[2]);
    f->s.Tc.set_initial_variance(device.Tc_var[0], device.Tc_var[1], device.Tc_var[2]);
    f->s.a_bias.v = v4(device.a_bias[0], device.a_bias[1], device.a_bias[2], 0.);
    f_t tmp[3];
    //TODO: figure out how much drift we need to worry about between runs
    for(int i = 0; i < 3; ++i) tmp[i] = device.a_bias_var[i] < min_a_bias_var ? min_a_bias_var : device.a_bias_var[i];
    f->s.a_bias.set_initial_variance(tmp[0], tmp[1], tmp[2]);
    f->s.w_bias.v = v4(device.w_bias[0], device.w_bias[1], device.w_bias[2], 0.);
    for(int i = 0; i < 3; ++i) tmp[i] = device.w_bias_var[i] < min_w_bias_var ? min_w_bias_var : device.w_bias_var[i];
    f->s.w_bias.set_initial_variance(tmp[0], tmp[1], tmp[2]);
    
    f->s.focal_length.v = device.Fx;
    f->s.center_x.v = device.Cx;
    f->s.center_y.v = device.Cy;
    f->s.k1.v = device.K[0];
    f->s.k2.v = device.K[1];
    f->s.k3.v = 0.; //device.K[2];
    
    f->s.g.set_initial_variance(1.e-7);
    
    f->s.T.set_process_noise(0.);
    f->s.W.set_process_noise(0.);
    f->s.V.set_process_noise(0.);
    f->s.w.set_process_noise(0.);
    f->s.dw.set_process_noise(35. * 35.); // this stabilizes dw.stdev around 5-6
    f->s.a.set_process_noise(.6*.6);
    f->s.g.set_process_noise(1.e-30);
    f->s.Wc.set_process_noise(1.e-30);
    f->s.Tc.set_process_noise(1.e-30);
    f->s.a_bias.set_process_noise(1.e-10);
    f->s.w_bias.set_process_noise(1.e-12);
    //TODO: check this process noise
    f->s.focal_length.set_process_noise(1.e-2);
    f->s.center_x.set_process_noise(1.e-5);
    f->s.center_y.set_process_noise(1.e-5);
    f->s.k1.set_process_noise(1.e-6);
    f->s.k2.set_process_noise(1.e-6);
    f->s.k3.set_process_noise(1.e-6);
    
    f->s.T.set_initial_variance(1.e-7); // to avoid not being positive definite
    //TODO: This might be wrong. changing this to 10 makes a very different (and not necessarily worse) result.
    f->s.W.set_initial_variance(10., 10., 1.e-7); // to avoid not being positive definite
    f->s.V.set_initial_variance(1. * 1.);
    f->s.w.set_initial_variance(1.e5);
    f->s.dw.set_initial_variance(1.e5); //observed range of variances in sequences is 1-6
    f->s.a.set_initial_variance(1.e5);

    f->s.focal_length.set_initial_variance(BEGIN_FOCAL_VAR);
    f->s.center_x.set_initial_variance(BEGIN_C_VAR);
    f->s.center_y.set_initial_variance(BEGIN_C_VAR);
    f->s.k1.set_initial_variance(BEGIN_K1_VAR);
    f->s.k2.set_initial_variance(BEGIN_K2_VAR);
    f->s.k3.set_initial_variance(BEGIN_K3_VAR);
    
    f->shutter_delay = device.shutter_delay;
    f->shutter_period = device.shutter_period;
    f->image_height = device.image_height;
    f->image_width = device.image_width;
    
    f->track.width = device.image_width;
    f->track.height = device.image_height;
    f->track.stride = f->track.width;
    f->track.init();

    f->last_qr_time = 0;
    f->qr.init();
    
    f->max_velocity = 0.;
    f->median_depth_variance = 1.;
    f->has_converged = false;
    
    state_node::statesize = 0;
    f->s.enable_orientation_only();
    f->s.remap();
}

float filter_converged(struct filter *f)
{
    if(f->run_state == RCSensorFusionRunStateSteadyInitialization) {
        if(f->stable_start == 0) return 0.;
        return (f->last_time - f->stable_start) / (f_t)steady_converge_time;
    } else if(f->run_state == RCSensorFusionRunStatePortraitCalibration) {
        return get_bias_convergence(f, 1);
    } else if(f->run_state == RCSensorFusionRunStateLandscapeCalibration) {
        return get_bias_convergence(f, 0);
    } else if(f->run_state == RCSensorFusionRunStateStaticCalibration) {
        return min(f->accel_stability.count / (f_t)calibration_converge_samples, get_bias_convergence(f, 2));
    } else if(f->run_state == RCSensorFusionRunStateRunning || f->run_state == RCSensorFusionRunStateDynamicInitialization) { // TODO: proper progress for dynamic init, if needed.
        return 1.;
    } else return 0.;
}

bool filter_is_steady(struct filter *f)
{
    return
        norm(f->s.V.v) < .1 &&
        norm(f->s.w.v) < .1;
}

int filter_get_features(struct filter *f, struct corvis_feature_info *features, int max)
{
    int index = 0;
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        if(index >= max) break;
        features[index].id = (*fiter)->id;
        features[index].x = (*fiter)->current[0];
        features[index].y = (*fiter)->current[1];
        features[index].wx = (*fiter)->world[0];
        features[index].wy = (*fiter)->world[1];
        features[index].wz = (*fiter)->world[2];
        features[index].depth = (*fiter)->depth;
        features[index].stdev = (*fiter)->v.stdev_meters(sqrt((*fiter)->variance()));
        ++index;
    }
    return index;
}

void filter_get_camera_parameters(struct filter *f, float matrix[16], float focal_center_radial[5])
{
    focal_center_radial[0] = f->s.focal_length.v;
    focal_center_radial[1] = f->s.center_x.v;
    focal_center_radial[2] = f->s.center_y.v;
    focal_center_radial[3] = f->s.k1.v;
    focal_center_radial[4] = f->s.k2.v;

    //transpose for opengl
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            matrix[j * 4 + i] = f->s.camera_matrix[i][j];
        }
    }
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

void filter_select_feature(struct filter *f, float x, float y)
{
    //first, see if we already have a feature there
    float mydist = 8; // 8 pixel radius max
    state_vision_feature *myfeat = 0;
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        state_vision_feature *feat = *fiter;
        if(feat->status != feature_normal && feat->status != feature_ready && feat->status != feature_initializing) continue;
        float dx = fabs(feat->current[0] - x);
        float dy = fabs(feat->current[1] - y);
        if(dx <= mydist && dy <= mydist) { //<= so we get full 8 pixel range and we default to younger features
            myfeat = feat;
            mydist = (dx < dy) ? dy : dx;
        }
    }
    if(!myfeat) {
        //didn't find an existing feature - select a new one
        //f->maxfeats is not necessarily a hard limit, so don't worry if we don't have room for a feature
        vector<xy> kp = f->track.detect(f->track.im2, NULL, 1, x - 8, y - 8, 17, 17);
        if(kp.size() > 0) {
            myfeat = f->s.add_feature(kp[0].x, kp[0].y);
            int lx = floor(kp[0].x);
            int ly = floor(kp[0].y);
            myfeat->intensity = (((unsigned int)f->track.im2[lx + ly*f->track.width]) + f->track.im2[lx + 1 + ly * f->track.width] + f->track.im2[lx + f->track.width + ly * f->track.width] + f->track.im2[lx + 1 + f->track.width + ly * f->track.width]) >> 2;
        }
    }
    if(!myfeat) return; //couldn't find anything
    myfeat->user = true;
    f->s.remap();
}

void filter_start_qr_detection(struct filter *f, const char * data, float dimension, bool use_gravity)
{
    f->qr.start(data, dimension, use_gravity);
    f->last_qr_time = 0;
}

void filter_stop_qr_detection(struct filter *f)
{
    f->qr.stop();
}
