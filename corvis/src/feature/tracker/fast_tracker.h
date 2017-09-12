#ifndef __FAST_TRACKER_H__
#define __FAST_TRACKER_H__

#include "tracker.h"
#include "fast_constants.h"

class fast_tracker : public tracker
{
public:
    template<int border_size>
    static bool is_trackable(int x, int y, int width, int height) {
        return 0 <= (x-border_size) && (x+border_size) < width
            && 0 <= (y-border_size) && (y+border_size) < height;
    }

    template<typename Descriptor>
    struct fast_feature: public tracker::feature
    {
        fast_feature(float x_, float y_, const tracker::image& image) : x(x_), y(y_),
                                                                        descriptor(x_, y_, image) {}
        float x, y;
        Descriptor descriptor;
    };


    typedef struct { float x, y, score, reserved; } xy;

private:
    static bool xy_comp(const xy &first, const xy &second) { return first.score > second.score; }
    int pixel[16];
    int xsize, ysize, stride, patch_stride;
    void init(const int x, const int y, const int s, const int ps);
    std::vector<xy> features;

public:
    fast_tracker() {}
    virtual ~fast_tracker() {}
    virtual std::vector<feature_track> &detect(const image &image, const std::vector<feature_track *> &current, size_t number_desired) override;
    virtual void track(const image &image, std::vector<feature_track *> &tracks) override;
};

#endif
