#include "calibration_defaults.h"
#include "calibration_json.h"

static const char *CALIBRATION_DEFAULT_GIGABYTES11 = R"(
{
  "device": "gigabytes11",
  "calibrationVersion": 7,
  "Fx": 635,
  "Fy": 635,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0,
  "K1": 0,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": 0,
  "Tc1": 0,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 0.000025,
  "aMeasVar": 0.0009,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPAD2 = R"(
{
  "device": "ipad2",
  "calibrationVersion": 7,
  "Fx": 782,
  "Fy": 782,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.029999999329447746,
  "K1": -0.20999999344348907,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": -0.029999999329447746,
  "Tc1": 0.11800000071525574,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPAD3 = R"(
{
  "device": "ipad3",
  "calibrationVersion": 7,
  "Fx": 627,
  "Fy": 627,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.17000000178813934,
  "K1": -0.37999999523162842,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": 0.064000003039836884,
  "Tc1": -0.017000000923871994,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPAD4 = R"(
{
  "device": "ipad4",
  "calibrationVersion": 7,
  "Fx": 594,
  "Fy": 594,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.17000000178813934,
  "K1": -0.4699999988079071,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": 0.0099999997764825821,
  "Tc1": 0.05000000074505806,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPADAIR = R"(
{
  "device": "ipadair",
  "calibrationVersion": 7,
  "Fx": 582,
  "Fy": 582,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.11999999731779099,
  "K1": -0.25,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": -0.012000000104308128,
  "Tc1": 0.064999997615814209,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPADAIR2 = R"(
{
  "device": "ipadair2",
  "calibrationVersion": 7,
  "Fx": 573,
  "Fy": 573,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.12999999523162842,
  "K1": -0.25999999046325684,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": -0.0030000000260770321,
  "Tc1": 0.068000003695487976,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPADMINI = R"(
{
  "device": "ipadmini",
  "calibrationVersion": 7,
  "Fx": 583,
  "Fy": 583,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.12999999523162842,
  "K1": -0.20999999344348907,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": -0.014000000432133675,
  "Tc1": 0.074000000953674316,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPADMINIRETINA = R"(
{
  "device": "ipadminiretina",
  "calibrationVersion": 7,
  "Fx": 580,
  "Fy": 580,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.14000000059604645,
  "K1": -0.33000001311302185,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": -0.0030000000260770321,
  "Tc1": 0.070000000298023224,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPADMINIRETINA2 = R"(
{
  "device": "ipadminiretina2",
  "calibrationVersion": 7,
  "Fx": 580,
  "Fy": 580,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.14000000059604645,
  "K1": -0.33000001311302185,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": -0.0030000000260770321,
  "Tc1": 0.070000000298023224,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPHONE4S = R"(
{
  "device": "iphone4s",
  "calibrationVersion": 7,
  "Fx": 606,
  "Fy": 606,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.20999999344348907,
  "K1": -0.44999998807907104,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": 0.0099999997764825821,
  "Tc1": 0.0060000000521540642,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPHONE5 = R"(
{
  "device": "iphone5",
  "calibrationVersion": 7,
  "Fx": 596,
  "Fy": 596,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.11999999731779099,
  "K1": -0.20000000298023224,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": 0.0099999997764825821,
  "Tc1": 0.0080000003799796104,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPHONE5C = R"(
{
  "device": "iphone5c",
  "calibrationVersion": 7,
  "Fx": 596,
  "Fy": 596,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.11999999731779099,
  "K1": -0.20000000298023224,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": 0.004999999888241291,
  "Tc1": 0.02500000037252903,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPHONE5S = R"(
{
  "device": "iphone5s",
  "calibrationVersion": 7,
  "Fx": 547,
  "Fy": 547,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.090000003576278687,
  "K1": -0.15000000596046448,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": 0.014999999664723873,
  "Tc1": 0.05000000074505806,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPHONE6 = R"(
{
  "device": "iphone6",
  "calibrationVersion": 7,
  "Fx": 548,
  "Fy": 548,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.10000000149011612,
  "K1": -0.15000000596046448,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": 0.014999999664723873,
  "Tc1": 0.064999997615814209,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPHONE6PLUS = R"(
{
  "device": "iphone6plus",
  "calibrationVersion": 7,
  "Fx": 548,
  "Fy": 548,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.10000000149011612,
  "K1": -0.15000000596046448,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": 0.0080000003799796104,
  "Tc1": 0.075000002980232239,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_IPOD5 = R"(
{
  "device": "ipodtouch",
  "calibrationVersion": 7,
  "Fx": 591,
  "Fy": 591,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.18000000715255737,
  "K1": -0.37000000476837158,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": 0.043000001460313797,
  "Tc1": 0.019999999552965164,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

static const char *CALIBRATION_DEFAULT_UNKNOWN = R"(
{
  "device": "unknown",
  "calibrationVersion": 7,
  "Fx": 600,
  "Fy": 600,
  "Cx": 319.5,
  "Cy": 239.5,
  "px": 0,
  "py": 0,
  "K0": 0.20000000298023224,
  "K1": -0.20000000298023224,
  "K2": 0,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "Tc0": 0.0099999997764825821,
  "Tc1": 0.029999999329447746,
  "Tc2": 0,
  "Wc0": 2.2214415073394775,
  "Wc1": -2.2214415073394775,
  "Wc2": 0,
  "abiasvar0": 0.0096039995551109314,
  "abiasvar1": 0.0096039995551109314,
  "abiasvar2": 0.0096039995551109314,
  "wbiasvar0": 0.0076154354028403759,
  "wbiasvar1": 0.0076154354028403759,
  "wbiasvar2": 0.0076154354028403759,
  "TcVar0": 9.9999999747524271e-07,
  "TcVar1": 9.9999999747524271e-07,
  "TcVar2": 1.000000013351432e-10,
  "WcVar0": 9.9999999747524271e-07,
  "WcVar1": 9.9999999747524271e-07,
  "WcVar2": 9.9999999747524271e-07,
  "wMeasVar": 1.3707783182326239e-05,
  "aMeasVar": 0.00022821025049779564,
  "imageWidth": 640,
  "imageHeight": 480,
  "shutterDelay": 0,
  "shutterPeriod": 33333
}
)";

const char *calibration_default_json_for_device_type(corvis_device_type device)
{
    switch(device) {
    case DEVICE_TYPE_IPOD5:
            return CALIBRATION_DEFAULT_IPOD5;
    case DEVICE_TYPE_IPHONE4S:
            return CALIBRATION_DEFAULT_IPHONE4S;
    case DEVICE_TYPE_IPHONE5C:
            return CALIBRATION_DEFAULT_IPHONE5C;
    case DEVICE_TYPE_IPHONE5S:
            return CALIBRATION_DEFAULT_IPHONE5S;
    case DEVICE_TYPE_IPHONE5:
            return CALIBRATION_DEFAULT_IPHONE5;
    case DEVICE_TYPE_IPHONE6PLUS:
            return CALIBRATION_DEFAULT_IPHONE6PLUS;
    case DEVICE_TYPE_IPHONE6:
            return CALIBRATION_DEFAULT_IPHONE6;
    case DEVICE_TYPE_IPAD2:
            return CALIBRATION_DEFAULT_IPAD2;
    case DEVICE_TYPE_IPAD3:
            return CALIBRATION_DEFAULT_IPAD3;
    case DEVICE_TYPE_IPAD4:
            return CALIBRATION_DEFAULT_IPAD4;
    case DEVICE_TYPE_IPADAIR2:
            return CALIBRATION_DEFAULT_IPADAIR2;
    case DEVICE_TYPE_IPADAIR:
            return CALIBRATION_DEFAULT_IPADAIR;
    case DEVICE_TYPE_IPADMINIRETINA2:
            return CALIBRATION_DEFAULT_IPADMINIRETINA2;
    case DEVICE_TYPE_IPADMINIRETINA:
            return CALIBRATION_DEFAULT_IPADMINIRETINA;
    case DEVICE_TYPE_IPADMINI:
            return CALIBRATION_DEFAULT_IPADMINI;
    case DEVICE_TYPE_GIGABYTES11:
            return CALIBRATION_DEFAULT_GIGABYTES11;
    case DEVICE_TYPE_UNKNOWN:
    default:
            return CALIBRATION_DEFAULT_UNKNOWN;
    }
}

bool calibration_load_defaults(const corvis_device_type deviceType, device_parameters &cal)
{
    const rcCalibration def = {};
    bool result = calibration_deserialize(calibration_default_json_for_device_type(deviceType), cal, &def);
    if (result) cal.calibrationVersion = CALIBRATION_VERSION; // need this here to override possibly outdated value in default JSON
    return result;
}
