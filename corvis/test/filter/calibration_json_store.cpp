#include "gtest/gtest.h"
#include "calibration_json_store.h"
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

TEST(calibration_json_store_tests, SerializeDeserialize)
{
    corvis_device_parameters cal, calDeserialized;
    EXPECT_TRUE(calibration_load_defaults(DEVICE_TYPE_UNKNOWN, cal));

    try
    {
        std::string jsonString;
        EXPECT_TRUE(calibration_serialize(cal, jsonString));
        EXPECT_TRUE(jsonString.length()); // expect non-zero length

        EXPECT_TRUE(calibration_deserialize(jsonString, calDeserialized));
    }
    catch (runtime_error)
    {
        FAIL();
    }

    // just do some spot checking
    EXPECT_EQ(cal.version, calDeserialized.version);
    EXPECT_FLOAT_EQ(cal.Fx, calDeserialized.Fx);
    EXPECT_FLOAT_EQ(cal.Cx, calDeserialized.Cx);
}

TEST(calibration_json_store_tests, DeserializeCalibration)
{
    corvis_device_parameters calDeserialized;

    try
    {
        std::string jsonString;
        EXPECT_FALSE(calibration_deserialize(jsonString, calDeserialized));
    }
    catch (...)
    {
        FAIL();
    }
}
