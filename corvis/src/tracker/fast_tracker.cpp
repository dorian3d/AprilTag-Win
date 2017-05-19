#include "fast_tracker.h"
#include "fast_constants.h"

#include "cor_types.h"

using namespace std;

vector<tracker::point> &fast_tracker::detect(const image &image, const std::vector<point> &features, int number_desired)
{
    if (!mask)
        mask = std::make_unique<scaled_mask>(image.width_px, image.height_px);
    mask->initialize();
    for (auto &f : features)
        mask->clear((int)f.x, (int)f.y);

    fast.init(image.width_px, image.height_px, image.stride_px, full_patch_width, half_patch_width);

    feature_points.clear();
    feature_points.reserve(number_desired);
    for(const auto &d : fast.detect(image.image, mask.get(), number_desired, fast_detect_threshold, 0, 0, image.width_px, image.height_px)) {
        if(!is_trackable((int)d.x, (int)d.y, image.width_px, image.height_px) || !mask->test((int)d.x, (int)d.y))
            continue;
        mask->clear((int)d.x, (int)d.y);
        feature_points.emplace_back(make_shared<fast_feature>(d.x, d.y, image.image, image.stride_px), d.x, d.y, d.score);
        if (feature_points.size() == number_desired)
            break;
    }
    return feature_points;
}

void fast_tracker::track(const image &image, vector<feature_track> &tracks)
{
    for(auto &t : tracks) {
        fast_feature &f = *static_cast<fast_feature *>(t.feature.get());

        xy bestkp = fast.track(f.patch, image.image,
                half_patch_width, half_patch_width,
                t.x + f.dx, t.y + f.dy, fast_track_radius,
                fast_track_threshold, fast_min_match);

        // Not a good enough match, try the filter prediction
        if(bestkp.score < fast_good_match) {
            xy bestkp2 = fast.track(f.patch, image.image,
                    half_patch_width, half_patch_width,
                    t.pred_x, t.pred_y, fast_track_radius,
                    fast_track_threshold, bestkp.score);
            if(bestkp2.score > bestkp.score)
                bestkp = bestkp2;
        }

        if(bestkp.x != INFINITY) {
            f.dx = bestkp.x - t.x;
            f.dy = bestkp.y - t.y;
            t.x = bestkp.x;
            t.y = bestkp.y;
            t.score = bestkp.score;
            t.found = true;
        }
    }
}
