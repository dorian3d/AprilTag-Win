#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include "CaptureManager.h"
#include "Debug.h"
#include <string>
#include "libpxcimu_internal.h"
#include "SensorData.h"

using namespace RealityCap;
using namespace std;

CaptureManager::CaptureManager(PXCSenseManager* senseMan) : SensorManager(senseMan), _isCapturing(false)
{
    _captureFilePath = GetExePath() + TEXT("\\capture");
}

bool CaptureManager::StartCapture()
{
    if (isCapturing()) return true;
    if (!isVideoStreaming())
    {
        if (!StartSensors()) return false;
    }
    SetRecording(_captureFilePath.c_str());
    return isCapturing();
}

void CaptureManager::StopCapture()
{
    if (!isCapturing()) return;
    //cp.stop();
    _isCapturing = false;
    StopSensors();
}

bool RealityCap::CaptureManager::isCapturing()
{
    return _isCapturing;
}

wstring CaptureManager::GetExePath()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    string buf(buffer);
    string::size_type pos = buf.find_last_of("\\/");
    return wstring(buf.begin(), buf.begin() + pos);
}
