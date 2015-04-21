#include "pch.h"
#include "IMUManager.h"
#include "Debug.h"

using namespace Windows::Devices::Sensors;
using namespace Windows::Foundation;
using namespace Platform;
using namespace RealityCap;
using namespace Windows::UI::Xaml;

#define USE_WIN32_AMETER_API false
static const int DESIRED_INTERVAL = 16; // desired sensor reporting interval in milliseconds

IMUManager::IMUManager()
{

	if (!USE_WIN32_AMETER_API)
	{
		accelerometer = Accelerometer::GetDefault();
		if (accelerometer == nullptr) Debug::Log(L"ERROR: No accelerometer found");
	}
	
	gyro = Gyrometer::GetDefault();
	if (gyro == nullptr) Debug::Log(L"ERROR: No gyro found");
}

//IMUManager^ IMUManager::GetSharedInstance()
//{
//	static IMUManager^ _sharedInstance;
//	if (_sharedInstance == nullptr)
//	{
//		Debug::Log(L"Instantiating shared IMUManager instance");
//		_sharedInstance = ref new IMUManager();
//	}
//	return _sharedInstance;
//}

bool IMUManager::StartSensors()
{
	bool result = true;
	
	/*if (USE_WIN32_AMETER_API)
	{
		accelMan.Initialize();
		accelMan.SetChangeSensitivity(0);
	}
	else if (accelerometer != nullptr)
	{*/
		accelerometer->ReportInterval = accelerometer->MinimumReportInterval > DESIRED_INTERVAL ? accelerometer->MinimumReportInterval : DESIRED_INTERVAL;  // milliseconds
		Debug::Log(L"accel reporting interval: %ims", accelerometer->ReportInterval);
		accelToken = accelerometer->ReadingChanged::add(ref new TypedEventHandler<Accelerometer^, AccelerometerReadingChangedEventArgs^>(this, &IMUManager::AccelReadingChanged));
	/*}
	else
		result = false;*/

	if (gyro != nullptr)
	{
		gyro->ReportInterval = gyro->MinimumReportInterval > DESIRED_INTERVAL ? gyro->MinimumReportInterval : DESIRED_INTERVAL;  // milliseconds
		Debug::Log(L"gyro reporting interval: %ims", gyro->ReportInterval);
		gyroToken = gyro->ReadingChanged::add(ref new TypedEventHandler<Gyrometer^, GyrometerReadingChangedEventArgs^>(this, &IMUManager::GyroReadingChanged));
	}
	else
		result = false;

	return result;
}

void IMUManager::StopSensors()
{
	if (accelerometer != nullptr)
	{
		accelerometer->ReadingChanged::remove(accelToken);
		accelerometer->ReportInterval = 0; // Restore the default report interval to release resources while the sensor is not in use
	}

	if (gyro != nullptr)
	{
		gyro->ReadingChanged::remove(gyroToken);
		gyro->ReportInterval = 0; // Restore the default report interval to release resources while the sensor is not in use
	}
}

// polling
void IMUManager::ReadAccelerometerData(Object^ sender, Object^ e)
{
	AccelerometerReading^ reading = accelerometer->GetCurrentReading();
	long long millisec = reading->Timestamp.UniversalTime / 10000;
	Debug::Log(L"%lld\taccel\tx: %1.3f\ty: %1.3f\tz: %1.3f", millisec, reading->AccelerationX, reading->AccelerationY, reading->AccelerationZ);
}

// subscription
void IMUManager::AccelReadingChanged(Accelerometer^ sender, AccelerometerReadingChangedEventArgs^ e)
{
	AccelerometerReading^ reading = e->Reading;
	long long millisec = reading->Timestamp.UniversalTime / 10000;
	Debug::Log(L"%lld\taccel\tx: %1.3f\ty: %1.3f\tz: %1.3f", millisec, reading->AccelerationX, reading->AccelerationY, reading->AccelerationZ);
}

void IMUManager::GyroReadingChanged(Gyrometer^ sender, GyrometerReadingChangedEventArgs^ e)
{
	GyrometerReading^ reading = e->Reading;
	long long millisec = reading->Timestamp.UniversalTime / 10000;
	Debug::Log(L"%lld\tgyro\tx: %3.3f\ty: %3.3f\tz: %3.3f", millisec, reading->AngularVelocityX, reading->AngularVelocityY, reading->AngularVelocityZ); // gyro data is in degrees/sec

	//if (USE_WIN32_AMETER_API) // instead of setting up a timer for polling the ameter, we can use this event handler to poll in sync with the gyro
	//{
	//	AccelSample sample = accelMan.GetSample();
	//	Debug::Log(L"%i:%i:%i\taccel\tx: %1.3f\ty: %1.3f\tz: %1.3f", sample.timestamp.wMinute, sample.timestamp.wSecond, sample.timestamp.wMilliseconds, sample.x, sample.y, sample.z);
	//}
}