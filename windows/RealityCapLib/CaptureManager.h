#pragma once

#include "SensorManager.h"
#include "..\..\corvis\src\filter\capture.h"

namespace RealityCap
{
    class CaptureManager : virtual public SensorManager
    {
    public:
        CaptureManager(PXCSenseManager* senseMan);
        bool StartCapture();
        void StopCapture();
        bool isCapturing();

    protected:
        virtual void OnColorFrame(PXCImage* colorImage) override;
        virtual void OnAmeterSample(struct imu_sample* sample) override;
        virtual void OnGyroSample(struct imu_sample* sample) override;

    private:
        capture cp;
        bool _isCapturing;
        std::string _captureFilePath;

        std::string GetExePath();
    };
}