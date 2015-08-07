package com.realitycap.android.rcutility;

import android.content.Context;
import android.hardware.Sensor;
import android.util.Log;

import com.intel.camera.toolkit.depth.Camera;
import com.intel.camera.toolkit.depth.Image;
import com.intel.camera.toolkit.depth.ImageSet;
import com.intel.camera.toolkit.depth.OnSenseManagerHandler;
import com.intel.camera.toolkit.depth.RSPixelFormat;
import com.intel.camera.toolkit.depth.StreamProfile;
import com.intel.camera.toolkit.depth.StreamProfileSet;
import com.intel.camera.toolkit.depth.StreamType;
import com.intel.camera.toolkit.depth.StreamTypeSet;
import com.intel.camera.toolkit.depth.sensemanager.IMUCaptureManager;
import com.intel.camera.toolkit.depth.sensemanager.SenseManager;
import com.intel.camera.toolkit.depth.sensemanager.SensorSample;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.concurrent.CountDownLatch;

/**
 * Created by benhirashima on 7/2/15.
 */
public class RealSenseManager
{
    private SenseManager mSenseManager;
    private IMUCaptureManager mIMUManager;
    private boolean mIsCamRunning = false;
    private boolean enablePlayback = false;
    private IRealSenseSensorReceiver receiver;

    private StreamTypeSet userStreamTypes = new StreamTypeSet(StreamType.COLOR, StreamType.DEPTH, StreamType.UVMAP);
    private Camera.Desc playbackCamDesc = new Camera.Desc(Camera.Type.PLAYBACK, Camera.Facing.ANY, userStreamTypes);

    protected Camera.Calibration.Intrinsics mColorParams; //intrinsics param of color camera
    private CountDownLatch startupLatch;

    RealSenseManager(Context context, IRealSenseSensorReceiver receiver)
    {
        mSenseManager = new SenseManager(context);
        mIMUManager = IMUCaptureManager.instance(context);
        this.receiver = receiver;
    }

    public boolean startCameras()
    {
        Log.d(MyApplication.TAG, "startCameras");

        if (false == mIsCamRunning)
        {
            try
            {
                startupLatch = new CountDownLatch(1);
                if (enablePlayback)
                {
                    mSenseManager.enableStreams(mSenseEventHandler, playbackCamDesc);
                }
                else
                {
                    mSenseManager.enableStreams(mSenseEventHandler, getUserProfiles(), null);
                }
                startupLatch.await();
                mIsCamRunning = true;
            }
            catch (Exception e)
            {
                Log.e(MyApplication.TAG, "Exception:" + e.getMessage());
                e.printStackTrace();
            }
        }
        return mIsCamRunning;
    }

    public void stopCameras()
    {
        Log.d(MyApplication.TAG, "stopCameras");

        if (true == mIsCamRunning)
        {
            try
            {
                mSenseManager.close();
            } catch (Exception e)
            {
                Log.e(MyApplication.TAG, "Exception:" + e.getMessage());
                e.printStackTrace();
            }

            mIsCamRunning = false;
        }
    }

    public boolean startImu()
    {
        if (mIMUManager == null) return false;

        try
        {
            if (!mIMUManager.enableSensor(Sensor.TYPE_ACCELEROMETER))
            {
                Log.e(MyApplication.TAG, "Failed to enable accelerometer");
                return false;
            }
            if (!mIMUManager.enableSensor(Sensor.TYPE_GYROSCOPE_UNCALIBRATED))
            {
                Log.e(MyApplication.TAG, "Failed to enable gyro");
                return false;
            }
            return true;
        }
        catch (Exception e)
        {
            Log.e(MyApplication.TAG, "Error starting IMU: " + e.getMessage());
            return false;
        }
    }

    public void stopImu()
    {
        try
        {
            if (mIMUManager != null) mIMUManager.close();
        }
        catch (Exception e)
        {
            Log.e(MyApplication.TAG, "Error closing IMUManager: " + e.getMessage());
        }
    }

    protected ArrayList<SensorSample> getSamplesSince(int sensorType, long timestamp)
    {
        if (mIMUManager == null) return null;

        SensorSample[] allSamples = mIMUManager.querySensorSamples(sensorType); // The sensor samples are saved in reverse chronological order (so index 0, contains the most recent sample).
        ArrayList<SensorSample> newSamples = new ArrayList<>();
        if(allSamples != null)
        {
            for (SensorSample sample : allSamples)
            {
                if (sample.timestamp() > timestamp) newSamples.add(sample);
                else break;
            }
        }
        return newSamples;
    }

    public Camera.Calibration.Intrinsics getCameraIntrinsics()
    {
        return mColorParams;
    }

    OnSenseManagerHandler mSenseEventHandler = new OnSenseManagerHandler()
    {
        long lastAmeterTimestamp = 0;
        long lastGyroTimestamp = 0;

        @Override
        public void onSetProfile(Camera.CaptureInfo info)
        {
//            Log.i(MyApplication.TAG, "OnSetProfile");
            Camera.Calibration cal = info.getCalibrationData();
            if (cal != null) mColorParams = cal.colorIntrinsics;
            startupLatch.countDown();
        }

        @Override
        public void onNewSample(ImageSet images)
        {
            if (receiver == null) return; // no point in any of this if no one is receiving it

            Image color = images.acquireImage(StreamType.COLOR);
            Image depth = images.acquireImage(StreamType.DEPTH);

            if (color == null || depth == null)
            {
                if (color == null) Log.i(MyApplication.TAG, "color is null");
                if (depth == null) Log.i(MyApplication.TAG, "depth is null");
                return;
            }

            int colorStride = color.getInfo().DataSize / color.getHeight();
            int depthStride = depth.getInfo().DataSize / depth.getHeight();

//            Log.v(MyApplication.TAG, "RealSense camera sample received.");

            ByteBuffer colorData = color.acquireAccess();
            ByteBuffer depthData = depth.acquireAccess();

            receiver.onSyncedFrames(color.getTimeStamp(), 33333000, color.getWidth(), color.getHeight(), colorStride, colorData, depth.getWidth(), depth.getHeight(), depthStride, depthData);

            color.releaseAccess();
            depth.releaseAccess();

            // send IMU samples
            /*ArrayList<SensorSample> ameterSamples = getSamplesSince(Sensor.TYPE_ACCELEROMETER, lastAmeterTimestamp);
            if (ameterSamples != null && ameterSamples.size() > 0 && ameterSamples.get(0) != null)
            {
                lastAmeterTimestamp = ameterSamples.get(0).timestamp();
                receiver.onAccelerometerSamples(ameterSamples);
            }

            ArrayList<SensorSample> gyroSamples = getSamplesSince(Sensor.TYPE_GYROSCOPE_UNCALIBRATED, lastGyroTimestamp);
            if (gyroSamples != null && gyroSamples.size() > 0 && gyroSamples.get(0) != null)
            {
                lastGyroTimestamp = gyroSamples.get(0).timestamp();
                receiver.onGyroSamples(gyroSamples);
            }*/
        }

        @Override
        public void onError(StreamProfileSet profile, int error)
        {
            stopCameras();
            Log.e(MyApplication.TAG, "Error code " + error + ". The camera is not present or failed to initialize.");
        }
    };

    private StreamProfileSet getUserProfiles()
    {
        StreamProfileSet set = new StreamProfileSet();
        StreamProfile colorProfile = new StreamProfile(640, 480, RSPixelFormat.RGBA_8888, 30, StreamType.COLOR);
        StreamProfile depthProfile = new StreamProfile(320, 240, RSPixelFormat.Z16, 30, StreamType.DEPTH);
        set.set(StreamType.COLOR, colorProfile);
        set.set(StreamType.DEPTH, depthProfile);

        return set;
    }
}
