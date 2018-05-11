// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#ifndef __MODEL_H
#define __MODEL_H

#include "cor_types.h"
#include "packet.h"
#include "vec4.h"
#include "matrix.h"
#include <vector>
#include <list>
#include <future>
#include <cinttypes>
#include "state.h"
#include "state_motion.h"
#include "tracker.h"
#include "platform/sensor_clock.h"
#include "feature_descriptor.h"
#include "mapper.h"
#include "sensor_data.h"
#include "orb_descriptor.h"

#ifdef MYRIAD2
    #include "platform_defines.h"
#endif

enum group_flag {
    group_empty = 0,
    group_normal,
    group_reference
};

enum feature_flag {
    feature_empty = 0,
    feature_gooddrop,
    feature_normal,
    feature_initializing,
    feature_lost
};

class log_depth
{
public:
    f_t v;
    v2 initial;
    f_t depth() { return exp(v); }
    f_t invdepth() { return exp(-v); }
    f_t invdepth_jacobian() { return -exp(-v); }
    f_t stdev_meters(f_t stdev) { return exp(v) * stdev; }
    void set_depth_meters(f_t initial_depth) { v = (initial_depth > 0) ? log(initial_depth) : 0; }
    log_depth(): v(0) {}
};

class state_vision_intrinsics: public state_branch<state_node *>
{
public:
    state_scalar    focal_length { "focal_length", constant };
    state_vector<2> center       { "center", constant };
    state_vector<4> k            { "k", constant };
    rc_CalibrationType type {rc_CALIBRATION_TYPE_UNKNOWN};
    int image_width, image_height;

    state_vision_intrinsics(bool estimate_)
    {
        estimate = estimate_;
        children.push_back(&focal_length);
        children.push_back(&center);
        children.push_back(&k);
    }

    feature_t image_size() const { return feature_t {(f_t)image_width, (f_t)image_height}; }
    feature_t undistort_feature(const feature_t &feat_d) const;
    feature_t distort_feature(const feature_t &featu_u) const;
    f_t get_undistortion_factor(const feature_t &feat_d, m<1,2> *dku_d_dfeat_d = nullptr, m<1,4> *dku_d_dk = nullptr) const;
    f_t   get_distortion_factor(const feature_t &feat_u, m<1,2> *dkd_u_dfeat_u = nullptr, m<1,4> *dkd_u_dk = nullptr) const;
    feature_t normalize_feature(const feature_t &feat) const;
    feature_t unnormalize_feature(const feature_t &feat) const;
};

class state_vision_group;
class state_vision_track;
struct state_camera;

class state_vision_feature: public state_leaf<1> {
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    std::shared_ptr<log_depth> v;
    std::shared_ptr<tracker::feature> feature;
    state_vision_group &group;
    std::vector<state_vision_track *> tracks;
    size_t tracks_found {0};
    v3 body = v3(0, 0, 0);

    struct descriptor descriptor;
    bool is_in_map{false};

    bool depth_measured{false};
    bool recovered{false};

    static f_t initial_depth_meters;
    static f_t initial_var;
    static f_t initial_process_noise;
    static f_t max_variance;

    state_vision_feature(const tracker::feature_track &track, state_vision_group &group);
    bool should_drop() const;
    bool is_valid() const;
    bool is_good() const;
    void drop();
    void make_lost();
    bool is_initialized() const { return status == feature_normal; }
    bool force_initialize();
//private:
    enum feature_flag status = feature_initializing;
    
    void reset() {
        set_initial_variance(initial_var);
        v->set_depth_meters(initial_depth_meters);
        set_process_noise(initial_process_noise);
    }

    using state_leaf::from_row;
    using state_leaf::to_col;

    void perturb_variance() {
        if(index < 0 || index >= cov->size()) return;
        cov->cov(index, index) *= PERTURB_FACTOR;
    }
    
    f_t variance() const {
        if(index < 0 || index >= cov->size()) return initial_covariance(0, 0);
        return (*cov)(index, index);
    }
    
    void copy_state_to_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        state[index] = v->v;
    }
    
    virtual void copy_state_from_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        v->v = state[index];
    }
    
    virtual void print_matrix_with_state_labels(matrix &state, node_type nt) const {
        if(type != nt) return;
        fprintf(stderr, "feature[%" PRIu64 "]: ", feature->id); state.row(index+0).print();
    }

    virtual std::ostream &print_to(std::ostream & s) const
    {
        return s << "f" << feature->id << ": " << v->v << "±" << std::sqrt(variance());
    }
};

class state_vision_track {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    state_vision_feature &feature;
    tracker::feature_track track;
    f_t outlier = 0;
    f_t innovation_variance_x = 0, innovation_variance_y = 0, innovation_variance_xy = 0;
    static f_t outlier_thresh;
    static f_t outlier_reject;
    static f_t outlier_lost_reject;
    
    state_vision_track(size_t camera_id, state_vision_feature &f, tracker::feature_track &&t): feature(f), track(std::move(t)) {
        if(track.found()) ++feature.tracks_found;
        if(f.tracks.size() <= camera_id) f.tracks.resize(camera_id + 1);
        assert(f.tracks[camera_id] == nullptr);
        f.tracks[camera_id] = this;
    }
};

typedef nodeid groupid;
class state_vision_group: public state_branch<state_node *> {
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    std::shared_ptr<transformation> Gr;
    state_vector_ref<3>  Tr;
    state_quaternion_ref Qr;

    state_camera &camera;
    state_branch<std::unique_ptr<state_vision_feature>> features;
    std::list<std::unique_ptr<state_vision_feature>> lost_features;
    int health = 0;
    enum group_flag status = group_normal;
    groupid id;
    uint64_t frames_active = 0;
    bool reused = false;

    state_vision_group(const transformation &G, state_camera &camera, groupid group_id);
    void make_reference();
    static f_t ref_noise;
    static f_t min_feats;

    virtual std::ostream &print_to(std::ostream & s) const
    {
        s << "g" << id << ": "; return state_branch<state_node*>::print_to(s);
    }
};

struct camera_frame_t;

struct state_camera: state_branch<state_node*> {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    state_extrinsics extrinsics;
    state_vision_intrinsics intrinsics;
    std::unique_ptr<tracker> feature_tracker;
    std::list<tracker::feature_track> standby_tracks;
    size_t id;
    std::future<bool> detection_future; // true if detected_features is valid
    std::future<std::unique_ptr<camera_frame_t>> orb_future;
    bool orb_future_for_relocalization;

    std::list<state_vision_track> tracks;
    void update_feature_tracks(const sensor_data &data);
    void update_map_tracks(const sensor_data &data, mapper *map, const size_t min_group_map_add, const groupid closest_group_id, const transformation &G_Bclosest_Bnow);
    size_t track_count() const;
    void process_tracks(mapper *map, spdlog::logger &log);

    size_t detecting_space = 0;
    size_t detected_features = 0;
    int detector_failed = false;

    stdev<1> detect_stats, orb_stats, track_stats, map_track_stats;

    void clear() {
        tracks.clear();
        if (detection_future.valid())
            detection_future.wait();
        standby_tracks.clear();
    }

    void reset() {
        clear();
        detecting_space = 0;

        detect_stats    = stdev<1>();
        orb_stats       = stdev<1>();
        track_stats     = stdev<1>();
        map_track_stats = stdev<1>();

        state_branch<state_node*>::reset();
    }

    state_camera(size_t id_) : extrinsics("Qc", "Tc", false, false), intrinsics(false), id(id_) {
        reset();
        children.push_back(&extrinsics);
        children.push_back(&intrinsics);
    }
};

struct stereo_match
{
    struct view {
        state_camera &camera;
        std::list<tracker::feature_track>::iterator track;
        f_t depth_m;
    };
    std::array<view,2> views;
    f_t error_percent;
    stereo_match(state_camera &c0, std::list<tracker::feature_track>::iterator &t0, f_t d0,
                 state_camera &c1, std::list<tracker::feature_track>::iterator &t1, f_t d1, f_t e)
                     : views({{{c0, t0, d0}, {c1, t1, d1}}}), error_percent(e) {}
};

class state_vision: public state_motion {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    state_branch<std::unique_ptr<state_camera>, std::vector<std::unique_ptr<state_camera>>> cameras;
    state_branch<std::unique_ptr<state_vision_group>> groups;
    std::map<featureid,stereo_match> stereo_matches;
    state_vision(covariance &c, matrix &FP) : state_motion(c, FP) {
        non_orientation.children.push_back(&cameras);
        non_orientation.children.push_back(&groups);
    }
    ~state_vision();
    groupid group_counter = 0;

    void clear_features_and_groups();
    int process_features(mapper *map);
    state_vision_group *add_group(const rc_Sensor camera_id, mapper *map);
    transformation get_transformation() const;
    bool get_closest_group_transformation(groupid &group_id, transformation& G) const;
    bool get_group_transformation(const groupid group_id, transformation& G) const;

    void update_map(mapper *map);

    float median_depth_variance();
    
    virtual void enable_orientation_only(bool remap = true);
    virtual void reset();
protected:
    template<int N> int project_new_group_covariance(const state_vision_group &g, int i);
    void project_new_group_covariance(const state_vision_group &g);
#ifdef ENABLE_SHAVE_PROJECT_MOTION_COVARIANCE
    virtual void project_motion_covariance(matrix &dst, const matrix &src, f_t dt) const;
#endif
};

typedef state_vision state;

#endif
