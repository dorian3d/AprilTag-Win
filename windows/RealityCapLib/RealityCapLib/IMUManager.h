#pragma once
#ifndef IMUMANAGER_H
#define IMUMANAGER_H

namespace RealityCap
{
	public ref class IMUManager sealed
	{
	public:
		IMUManager();
		//static IMUManager^ GetSharedInstance();
		bool StartSensors(); // returns true if all sensors started successfully
		void StopSensors();

	private:
		Windows::Devices::Sensors::Accelerometer^ accelerometer;
		Windows::Devices::Sensors::Gyrometer^ gyro;
		Windows::Foundation::EventRegistrationToken accelToken;
		Windows::Foundation::EventRegistrationToken gyroToken;
		//AccelerometerManager accelMan;

		void ReadAccelerometerData(Object^ sender, Object^ e);
		void AccelReadingChanged(Windows::Devices::Sensors::Accelerometer^ sender, Windows::Devices::Sensors::AccelerometerReadingChangedEventArgs^ e);
		void GyroReadingChanged(Windows::Devices::Sensors::Gyrometer^ sender, Windows::Devices::Sensors::GyrometerReadingChangedEventArgs^ e);
	};
}

#endif
