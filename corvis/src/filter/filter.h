#ifndef __FILTER_H
#define __FILTER_H

#include "model.h"
#include "observation.h"
#include "device_parameters.h"
#include "feature_info.h"
#include "tracker.h"

struct filter {
filter(bool estimate_calibration): min_feats_per_group(0), output(0), control(0), visbuf(0), last_time(0), last_packet_time(0), last_packet_type(0), s(estimate_calibration, cov), gravity_init(0), frame(0), status(ST_STOP), want_start(0), got_accelerometer(0), got_gyroscope(0), got_image(0), recognition_buffer(0), detector_failed(false), tracker_failed(false), tracker_warned(false), speed_failed(false), speed_warning(false), numeric_failed(false), speed_warning_time(0), ignore_lateness(false), calibration_bad(false), scaled_mask(0), image_packets(0), valid_time(false), first_time(0), mindelta(0), valid_delta(false), last_arrival(0)
    {
        track.sink = 0;
        s.g.v = 9.8065;
    }
    ~filter() {
        if(scaled_mask) delete[] scaled_mask;
    }
    int min_feats_per_group;
    int min_group_add;
    int max_group_add;

    struct mapbuffer * output;
    struct mapbuffer * control;
    struct mapbuffer * visbuf;

    uint64_t last_time;
    uint64_t last_packet_time;
    int last_packet_type;
#ifdef SWIG
    %readonly
#endif
    state s;
#ifdef SWIG
    %readwrite
#endif
    
    covariance cov;

#ifndef SWIG
#endif
    f_t vis_cov;
    f_t init_vis_cov;
    f_t max_add_vis_cov;
    f_t min_add_vis_cov;
    f_t vis_ref_noise;
    f_t vis_noise;
    f_t w_variance;
    f_t a_variance;

    bool gravity_init;
    int frame;

    enum { ST_STOP, ST_INERTIAL, ST_STATIC, ST_STEADY, ST_WANTVIDEO, ST_VIDEO, ST_ANY } status;
    uint64_t want_start;
    
    bool got_accelerometer, got_gyroscope, got_image;
    f_t max_feature_std_percent;
    f_t outlier_thresh;
    f_t outlier_reject;
    int image_height, image_width;
    uint64_t shutter_delay;
    uint64_t shutter_period;
    mapbuffer *recognition_buffer;
    bool detector_failed, tracker_failed, tracker_warned;
    bool speed_failed, speed_warning;
    bool numeric_failed;
    uint64_t speed_warning_time;
    bool ignore_lateness;
    tracker track;
    struct corvis_device_parameters device;
    stdev_vector gyro_stability, accel_stability;
    uint64_t stable_start;
    bool calibration_bad;
    uint8_t *scaled_mask;
    
    int image_packets;
    bool valid_time;
    uint64_t first_time;
    
    int64_t mindelta;
    bool valid_delta;
    uint64_t last_arrival;
    
    uint64_t active_time;
    
    bool estimating_Tc;
    
    observation_queue observations;
};

bool filter_image_measurement(struct filter *f, unsigned char *data, int width, int height, int stride, uint64_t time);
void filter_accelerometer_measurement(struct filter *f, float data[3], uint64_t time);
void filter_gyroscope_measurement(struct filter *f, float data[3], uint64_t time);
void filter_set_reference(struct filter *f);
void filter_compute_gravity(struct filter *f, double latitude, double altitude);
void filter_start_static_calibration(struct filter *f);
void filter_stop_static_calibration(struct filter *f);
void filter_start_hold_steady(struct filter *f);
void filter_start_processing_video(struct filter *f);
void filter_stop_processing_video(struct filter *f);

#ifdef SWIG
%callback("%s_cb");
#endif
extern "C" void filter_image_packet(void *f, packet_t *p);
extern "C" void filter_imu_packet(void *f, packet_t *p);
extern "C" void filter_accelerometer_packet(void *f, packet_t *p);
extern "C" void filter_gyroscope_packet(void *f, packet_t *p);
extern "C" void filter_features_added_packet(void *f, packet_t *p);
extern "C" void filter_control_packet(void *_f, packet_t *p);
#ifdef SWIG
%nocallback;
#endif

extern "C" void filter_init(struct filter *f, corvis_device_parameters device);
extern "C" void filter_reset_full(struct filter *f);
extern "C" void filter_reset_for_inertial(struct filter *f);
extern "C" void filter_reset_position(struct filter *f);
float filter_converged(struct filter *f);
bool filter_is_steady(struct filter *f);
int filter_get_features(struct filter *f, struct corvis_feature_info *features, int max);
void filter_get_camera_parameters(struct filter *f, float matrix[16], float focal_center_radial[5]);
void filter_select_feature(struct filter *f, float x, float y);

#endif
