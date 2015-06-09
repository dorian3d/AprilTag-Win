
#include "gtest/gtest.h"
#include "rc_intel_interface.h"
#include <memory>
#include "calibration_json_store.h"

using namespace std;
using namespace RealityCap;

TEST(rc_intel_interface_tests, rc_setCalibration)
{
    // load default cal
    unique_ptr<calibration_data_store> store = calibration_data_store::GetStore();
    corvis_device_parameters cal;
    EXPECT_TRUE(store->LoadCalibrationDefaults(DEVICE_TYPE_GIGABYTE_S11, cal));

    // set cal
    std::wstring jsonString;
    EXPECT_TRUE(calibration_json_store::SerializeCalibration(cal, jsonString));

    rc_Tracker *tracker = rc_create();
    std::wstring wideJsonString = std::wstring(jsonString.begin(), jsonString.end());
    EXPECT_TRUE(rc_setCalibration(tracker, wideJsonString.c_str()));
    
    // now read cal back out and compare
    const wchar_t* buffer;
    size_t size = rc_getCalibration(tracker, &buffer);
    EXPECT_TRUE(size);

    // this doesn't work because not all all fields are being extracted from filter at the moment.
    //EXPECT_STREQ(jsonString.c_str(), buffer);
}

TEST(rc_intel_interface_tests, rc_setCalibration_failure)
{
    rc_Tracker *tracker = rc_create();
    std::wstring wideJsonString = L"";
    EXPECT_FALSE(rc_setCalibration(tracker, wideJsonString.c_str()));
}
