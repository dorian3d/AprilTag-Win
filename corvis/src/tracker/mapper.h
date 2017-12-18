// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#ifndef __MAPPER_H
#define __MAPPER_H

#include <list>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <stdbool.h>

#include "transformation.h"
#include "vec4.h"
#include <spdlog/spdlog.h>
#include "spdlog/sinks/null_sink.h"
#include "fast_tracker.h"
#include "sensor.h"
#include "state.h"
#include "rc_tracker.h"
#include "orb_descriptor.h"
#include "rapidjson/document.h"
#include "DBoW2/TemplatedVocabulary.h"
#include "descriptor.h"
#ifdef RELOCALIZATION_DEBUG
#include <opencv2/core/mat.hpp>
#endif

#define MIN_FEATURE_TRACKS 3
#define MIN_FEATURE_PARALLAX 5.0f/180*M_PI

typedef DBoW2::TemplatedVocabulary<orb_descriptor::raw, DBoW2::L1_NORM> orb_vocabulary;

class state_vision_intrinsics;
class log_depth;
struct frame_t;
struct camera_frame_t;

struct map_edge {
    bool loop_closure = false;
    transformation G;
    void serialize(rapidjson::Value &json, rapidjson::Document::AllocatorType &allocator);
    static void deserialize(const rapidjson::Value &json, map_edge &node);
};

enum class feature_type { tracked, triangulated };

struct map_feature {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    std::shared_ptr<log_depth> v;
    feature_type type;
    std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature;
    void serialize(rapidjson::Value &json, rapidjson::Document::AllocatorType &allocator);
    static bool deserialize(const rapidjson::Value &json, map_feature &feature, uint64_t &max_loaded_featid);
};

enum class node_status {normal, finished};

struct map_node {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    uint64_t id;
    std::map<uint64_t, map_edge> edges; // key is neighbor_id; we use a map structure to gurantee same order when traversing edges.
    map_edge &get_add_neighbor(uint64_t neighbor);
    void add_feature(std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature, std::shared_ptr<log_depth> v, const feature_type type);
    void set_feature_type(const uint64_t id, const feature_type type);

    transformation global_transformation;

    // relocalization
    uint64_t camera_id;
    std::shared_ptr<frame_t> frame;
    std::map<uint64_t,map_feature> features;
    node_status status{node_status::normal};
    void serialize(rapidjson::Value &json, rapidjson::Document::AllocatorType &allocator);
    static bool deserialize(const rapidjson::Value &json, map_node &node, uint64_t &max_loaded_featid);
};

struct map_relocalization_info {
    sensor_clock::time_point frame_timestamp;
    struct candidate {
        transformation G_node_frame;
        transformation G_world_node;
        sensor_clock::time_point node_timestamp;
        candidate() {}
        candidate(transformation g_node_frame, transformation g_world_node, sensor_clock::time_point node_ts)
            : G_node_frame(g_node_frame), G_world_node(g_world_node), node_timestamp(node_ts) {}
    };
    aligned_vector<candidate> candidates;
    void clear() { frame_timestamp = sensor_clock::micros_to_tp(0); candidates.clear(); }
    size_t size() const { return candidates.size(); }
};

class mapper {
 public:
    typedef uint64_t nodeid;
    typedef std::pair<nodeid, nodeid> match;
    typedef std::vector<match> matches;
    typedef std::pair<nodeid, transformation> node_path;
    typedef std::map<nodeid, transformation> nodes_path;

 private:
    std::unordered_map<nodeid, map_node> nodes;
    friend struct map_node;
    bool unlinked{false};
    uint64_t node_id_offset{0};
    uint64_t feature_id_offset{0};
    map_relocalization_info reloc_info;

 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    mapper();
    ~mapper();
    void reset();

    bool is_unlinked(nodeid node_id) const { return (unlinked && node_id < node_id_offset); }
    void add_node(nodeid node_id, const rc_Sensor camera_id);
    void remove_node(nodeid node_id);
    void remove_node_features(nodeid node_id);
    void add_edge(nodeid node_id1, nodeid node_id2, const transformation &G12, bool loop_closure = false);
    void remove_edge(nodeid node_id1, nodeid node_id2);
    void add_loop_closure_edge(nodeid node_id1, nodeid node_id2, const transformation &G12);
    void add_feature(nodeid node_id, std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature,
                     std::shared_ptr<log_depth> v, const feature_type type = feature_type::tracked);
    void set_feature_type(nodeid node_id, uint64_t feature_id, const feature_type type = feature_type::tracked);
    void initialize_track_triangulation(const tracker::feature_track &track, const nodeid node_id);
    void finish_lost_tracks(const tracker::feature_track &track);
    void add_triangulated_feature_to_group(const nodeid group_id, std::shared_ptr<fast_tracker::fast_feature<DESCRIPTOR>> feature, std::shared_ptr<log_depth> &v);
    void update_3d_feature(const tracker::feature_track &track, const transformation &&G_Bcurrent_Bnow, const rc_Sensor camera_id_now);
    nodes_path breadth_first_search(nodeid start, int maxdepth = 1);
    nodes_path breadth_first_search(nodeid start, const f_t maxdistance, const size_t N = 5); // maxdistance in meters, max number of nodes
    nodes_path breadth_first_search(nodeid start, std::set<nodeid>&& searched_nodes, bool expect_graph_connected=true);
    v3 get_feature3D(nodeid node_id, uint64_t feature_id); // returns feature wrt node body frame

    const std::unordered_map<nodeid, map_node> & get_nodes() const { return nodes; }
    map_node& get_node(nodeid id) { return nodes.at(id); }
    bool node_in_map(nodeid id) { return nodes.find(id) != nodes.end(); }
    uint64_t get_node_id_offset() { return node_id_offset; }
    uint64_t get_feature_id_offset() { return feature_id_offset; }

    void finish_node(nodeid node_id, bool compute_dbow_inverted_index);
    void set_node_transformation(nodeid id, const transformation & G);

    void serialize(rapidjson::Value &json, rapidjson::Document::AllocatorType &allocator);
    static bool deserialize(const rapidjson::Value &json, mapper &map);

    std::unique_ptr<orb_vocabulary> create_vocabulary_from_map(int branching_factor, int depth_levels) const;

    std::unique_ptr<spdlog::logger> log = std::make_unique<spdlog::logger>("mapper",  std::make_shared<spdlog::sinks::null_sink_st> ());

    /// fetch the vocabulary file from resource and create orb vocabulary
    std::unique_ptr<orb_vocabulary> orb_voc;

    std::map<unsigned int, std::vector<nodeid>> dbow_inverted_index; // given a word it stores the nodes in which it was observed

    // temporary point to current node
    map_node* current_node = nullptr;

    // for a feature id we associate the corresponding node in which it was detected
    std::map<uint64_t, nodeid> features_dbow;

    //we need the camera intrinsics
    std::vector<state_vision_intrinsics*> camera_intrinsics;
    std::vector<state_extrinsics*> camera_extrinsics;

    bool relocalize(const camera_frame_t& camera_frame);
    void estimate_pose(const aligned_vector<v3>& points_3d, const aligned_vector<v2>& points_2d, const rc_Sensor camera_id, transformation& G_candidateB_nowB, std::set<size_t>& inliers_set);
    const map_relocalization_info& get_relocalization_info() const { return reloc_info; }

    // reuse map features in filter
    struct map_feature_track {
        tracker::feature_track track;
        std::shared_ptr<log_depth> v;
        map_feature_track(tracker::feature_track &&track_, std::shared_ptr<log_depth> v_)
            : track(track_), v(v_) {}
    };

    struct node_feature_track {
        nodeid group_id;
        transformation G_neighbor_now;
        std::vector<map_feature_track> tracks;
        size_t found = 0;
        node_feature_track(nodeid id_, const transformation &G, std::vector<map_feature_track> &&tracks_)
            : group_id(id_), G_neighbor_now(G), tracks(tracks_) {}
    };
    std::vector<node_feature_track> map_feature_tracks;
    void predict_map_features(const uint64_t camera_id_now, const transformation& G_Bcurrent_Bnow);

// triangulated tracks
    struct triangulated_track
    {
        nodeid reference_nodeid = std::numeric_limits<uint64_t>::max();
        std::shared_ptr<log_depth> state; // pixel coordinates are constant
        float cov = 0.75f;
        float parallax = 0;
        size_t track_count = 0;
        triangulated_track(const nodeid id,
                           std::shared_ptr<log_depth> s) :
            reference_nodeid(id), state(s) {}
    };
    std::unordered_map<uint64_t, triangulated_track> triangulated_tracks;


#ifdef RELOCALIZATION_DEBUG
    std::function<void(cv::Mat &&image, const uint64_t image_id, const std::string &message, const bool pause)> debug;
#endif
};

#endif
