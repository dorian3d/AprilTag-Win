#ifndef __FILTER_SETUP_H
#define __FILTER_SETUP_H

#include "device_parameters.h"
#include "filter.h"

class filter_setup
{
public:
#ifdef SWIG
    %immutable;
#endif
    filter sfm;
#ifdef SWIG
    %mutable;
#endif
    struct corvis_device_parameters device;
    filter_setup(corvis_device_parameters *device_params);
    struct corvis_device_parameters get_device_parameters();
    int get_failure_code();
    bool get_speed_warning();
    bool get_vision_warning();
    bool get_vision_failure();
    bool get_speed_failure();
    bool get_other_failure();
    float get_filter_converged();
    bool get_device_steady();
    RCSensorFusionErrorCode get_error();
protected:
};

#endif
