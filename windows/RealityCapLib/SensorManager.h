#pragma once

#include "stdafx.h"
#include <thread>

class PXCSenseManager;
class PXCImage;
struct imu_sample;

namespace RealityCap
{
	class SensorManager
	{
	public:
		SensorManager();
		~SensorManager();
		
		bool StartSensors(); // returns true if video started successfully
		void StopSensors();
		bool isVideoStreaming();

	protected:
		virtual void OnColorFrame(PXCImage* colorImage);
		virtual void OnAmeterSample(struct imu_sample* sample);
		virtual void OnGyroSample(struct imu_sample* sample);

	private:		
		PXCSenseManager* senseMan;
		std::thread videoThread;
		void PollForFrames();
		bool _isVideoStreaming;
	};
}