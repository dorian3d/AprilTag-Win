#include "stdafx.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "TrackerManager.h"
#include "Debug.h"
#include <string>
#include <fstream>
#include <streambuf>
#include "libpxcimu_internal.h"
#include "rc_intel_interface.h"
#include "rc_pxc_util.h"

using namespace RealityCap;
using namespace std;

TrackerManager::TrackerManager(PXCSenseManager* senseMan) : SensorManager(senseMan), _isRunning(false), _delegate(NULL)
{
    _tracker = rc_create();
}

RealityCap::TrackerManager::~TrackerManager()
{
    rc_destroy(_tracker);
}

static void status_callback(void *handle, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence, float progress)
{
    ((TrackerManager *)handle)->UpdateStatus(state, error, confidence, progress);
}

const rc_Pose camera_pose = { 0, -1, 0, 0,
                             -1, 0, 0, 0,
                              0, 0, -1, 0 };

bool TrackerManager::StartTracking()
{
    if (isRunning()) return true;
    if (!StartSensors()) return false;

    std::wifstream t("gigabyte_s11.json");
    std::wstring calibrationJSON((std::istreambuf_iterator<wchar_t>(t)),
    std::istreambuf_iterator<wchar_t>());
    
    bool result = rc_setCalibration(_tracker, calibrationJSON.c_str());
    if (!result) return false;

    PXCCaptureManager *capMan = GetSenseManager()->QueryCaptureManager();
    PXCCapture::Device *pDevice = capMan->QueryDevice();
    PXCPointF32 focal = pDevice->QueryColorFocalLength();
    PXCPointF32 principal = pDevice->QueryColorPrincipalPoint();
    pxcI32 exposure = pDevice->QueryColorExposure();
    rc_configureCamera(_tracker, rc_EGRAY8, camera_pose, 640, 480, principal.x, principal.y, focal.x, 0, focal.y);

    _trackerState = rc_E_INACTIVE;
    
    rc_startCalibration(_tracker);

    _isRunning = true;

    return isRunning();
}

void TrackerManager::StopTracking()
{
    if (!isRunning()) return;

    const wchar_t* buffer;
    size_t size = rc_getCalibration(_tracker, &buffer);
    std::wofstream t("calibration_result.json");
    t << std::wstring(buffer);
    t.close();
    rc_stopTracker(_tracker);
    StopSensors();
    _isRunning = false;
}

bool TrackerManager::isRunning()
{
    return _isRunning;
}

void TrackerManager::SetDelegate(TrackerManagerDelegate* del)
{
    _delegate = del;
}

void TrackerManager::OnColorFrame(PXCImage* colorSample)
{
    // video not needed for calibration
}

void TrackerManager::OnAmeterSample(imu_sample_t* sample)
{
    if (!isRunning()) return;
    const rc_Vector vec = rc_convertAcceleration(sample);
    rc_receiveAccelerometer(_tracker, sample->coordinatedUniversalTime100ns / 10, vec);
    UpdateStatus(rc_getState(_tracker), rc_getError(_tracker), rc_getConfidence(_tracker), rc_getProgress(_tracker));
}

void TrackerManager::OnGyroSample(imu_sample_t* sample)
{
    if (!isRunning()) return;
    const rc_Vector vec = rc_convertAngularVelocity(sample);
    rc_receiveGyro(_tracker, sample->coordinatedUniversalTime100ns / 10, vec);
}

void TrackerManager::UpdateStatus(rc_TrackerState newState, rc_TrackerError errorCode, rc_TrackerConfidence confidence, float progress)
{
    // check for errors
    if (errorCode && _delegate) _delegate->OnError(errorCode);

    // check for status changes
    if (newState != _trackerState)
    {
        if (_delegate) _delegate->OnStatusUpdated(newState);
            _trackerState = newState;
    }
}

