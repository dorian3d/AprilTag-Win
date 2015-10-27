#include <string.h>
#include <jni.h>
#include <android/log.h>
#include <stdlib.h>
#include <rc_intel_interface.h>
#include <visualization.h>
#include "rc_replay_threaded.h"

#include "render_data.h"

#define TAG "tracker_wrapper"
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))

#define MY_JNI_VERSION JNI_VERSION_1_6

static JavaVM *javaVM;
static rc_Tracker *tracker;
static render_data render_data;
static visualization vis(&render_data);
static rc::replay_threaded *replayer;
static jobject trackerProxyObj;
static jclass trackerProxyClass;
static jmethodID trackerProxy_onStatusUpdated;
static jobject dataUpdateObj;
static jclass dataUpdateClass;
static jmethodID dataUpdate_setTimestamp;
static jmethodID dataUpdate_setPose;
static jmethodID dataUpdate_clearFeaturePoints;
static jmethodID dataUpdate_addFeaturePoint;
static jmethodID trackerProxy_onDataUpdated;
static jclass imageClass;
static jmethodID imageClass_close;

static float gOffsetX, gOffsetY, gOffsetZ;

typedef struct ZIntrinsics
{
    int rw, rh;
    float rpx, rpy;
    float rfx, rfy;
} ZIntrinsics;

static ZIntrinsics gZIntrinsics;

typedef struct RGBIntrinsics
{
    int rh, rw;
    float rpx, rpy;
    float rfx, rfy;
} RGBIntrinsics;

static RGBIntrinsics gRGBIntrinsics;

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    LOGI("JNI_OnLoad");
    javaVM = vm;
    return MY_JNI_VERSION;
}

#pragma mark - utility functions

bool RunExceptionCheck(JNIEnv *env)
{
    if (env->ExceptionCheck())
    {
#ifndef NDEBUG
        env->ExceptionDescribe();
        LOGE("JNI ERROR");
#endif
        env->ExceptionClear();
        return true;
    }
    else
    {
        return false;
    }
}

void initJavaObject(JNIEnv *env, const char *path, jobject *objptr)
{
    jclass cls = env->FindClass(path);
    if (!cls)
    {
        LOGE("initJavaObject: failed to get %s class reference", path);
        return;
    }
    jmethodID constr = env->GetMethodID(cls, "<init>", "()V");
    if (!constr)
    {
        LOGE("initJavaObject: failed to get %s constructor", path);
        return;
    }
    jobject obj = env->NewObject(cls, constr);
    if (!obj)
    {
        LOGE("initJavaObject: failed to create a %s object", path);
        return;
    }
    (*objptr) = env->NewGlobalRef(obj);
}

bool isThreadAttached()
{
    void* env;
    int status = javaVM->GetEnv(&env, MY_JNI_VERSION);
    return !(status == JNI_EDETACHED);
}

#pragma mark - callbacks up into java land

static void status_callback(void *handle, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence, float progress)
{
    if (!trackerProxyObj)
    {
        LOGE("status_callback: Tracker proxy object is null.");
        return;
    }

    JNIEnv *env;
    bool wasOriginallyAttached = isThreadAttached();
    javaVM->AttachCurrentThread(&env, NULL); // if thread was attached, we still need this to get the ref to JNIEnv, but don't need to detach later.
    if (RunExceptionCheck(env)) return;

    ((JNIEnv*)env)->CallVoidMethod(trackerProxyObj, trackerProxy_onStatusUpdated, (int)state, (int)error, (int)confidence, progress);
    RunExceptionCheck(((JNIEnv*)env));

    if (!wasOriginallyAttached) javaVM->DetachCurrentThread();
}

static void data_callback(void *handle, rc_Timestamp time, rc_Pose pose, rc_Feature *features, size_t feature_count)
{
    render_data.update_data(time, pose, features, feature_count);

    JNIEnv *env;

    if (!trackerProxyObj)
    {
        LOGE("data_callback: Tracker proxy object is null.");
        return;
    }

    bool wasOriginallyAttached = isThreadAttached();
    javaVM->AttachCurrentThread(&env, NULL);
    if (RunExceptionCheck(env)) return;

    // set properties on the SensorFusionData instance
    env->CallVoidMethod(dataUpdateObj, dataUpdate_setTimestamp, (long) time);
    if (RunExceptionCheck(env)) return;

    env->CallVoidMethod(dataUpdateObj, dataUpdate_setPose, pose[0], pose[1], pose[2], pose[3], pose[4], pose[5], pose[6], pose[7], pose[8], pose[9], pose[10], pose[11]);
    if (RunExceptionCheck(env)) return;

    env->CallVoidMethod(dataUpdateObj, dataUpdate_clearFeaturePoints); // necessary because we are reusing the cached instance of SensorFusionData
    if (RunExceptionCheck(env)) return;

    // add features to SensorFusionData instance
    for (int i = 0; i < feature_count; ++i)
    {
        rc_Feature feat = features[i];
        env->CallVoidMethod(dataUpdateObj, dataUpdate_addFeaturePoint, feat.id, feat.world.x, feat.world.y, feat.world.z, feat.image_x, feat.image_y);
        if (RunExceptionCheck(env)) return;
    }

    env->CallVoidMethod(trackerProxyObj, trackerProxy_onDataUpdated, dataUpdateObj);
    if (RunExceptionCheck(env)) return;

    if (!wasOriginallyAttached) javaVM->DetachCurrentThread();
}

static void release_image(void *handle)
{
//    LOGV("release_image");
    JNIEnv *env;

    bool wasOriginallyAttached = isThreadAttached();
    javaVM->AttachCurrentThread(&env, NULL);
    if (RunExceptionCheck(env)) return;

    env->CallVoidMethod((jobject)handle, imageClass_close);
    if (RunExceptionCheck(env)) return;

    env->DeleteGlobalRef((jobject)handle);

    if (!wasOriginallyAttached) javaVM->DetachCurrentThread();
}

#pragma mark - functions that get called from java land

extern "C"
{
    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_createTracker(JNIEnv *env, jobject thiz)
    {
        LOGD("createTracker");
        tracker = rc_create();
        if (!tracker) return (JNI_FALSE);

        // save this stuff for the callbacks.
        trackerProxyObj = env->NewGlobalRef(thiz);
        trackerProxyClass = (jclass)env->NewGlobalRef(env->GetObjectClass(trackerProxyObj));
        trackerProxy_onStatusUpdated = env->GetMethodID(trackerProxyClass, "onStatusUpdated", "(IIIF)V");
        trackerProxy_onDataUpdated = env->GetMethodID(trackerProxyClass, "onDataUpdated", "(Lcom/realitycap/android/rcutility/SensorFusionData;)V");

        dataUpdateClass = (jclass)env->NewGlobalRef(env->FindClass("com/realitycap/android/rcutility/SensorFusionData"));
        dataUpdate_setTimestamp = env->GetMethodID(dataUpdateClass, "setTimestamp", "(J)V");
        dataUpdate_setPose = env->GetMethodID(dataUpdateClass, "setPose", "(FFFFFFFFFFFF)V");
        dataUpdate_clearFeaturePoints = env->GetMethodID(dataUpdateClass, "clearFeaturePoints", "()V");
        dataUpdate_addFeaturePoint = env->GetMethodID(dataUpdateClass, "addFeaturePoint", "(JFFFFF)V"); // takes a long and 5 floats

        // init a SensorFusionData instance
        initJavaObject(env, "com/realitycap/android/rcutility/SensorFusionData", &dataUpdateObj);

        imageClass = (jclass)env->NewGlobalRef(env->FindClass("android/media/Image"));
        imageClass_close = env->GetMethodID(imageClass, "close", "()V");

        rc_setStatusCallback(tracker, status_callback, NULL);
        rc_setDataCallback(tracker, data_callback, NULL);

        return (JNI_TRUE);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_destroyTracker(JNIEnv *env, jobject thiz)
    {
        LOGD("destroyTracker");
        if (!tracker) return (JNI_FALSE);
        rc_destroy(tracker);
        tracker = NULL;
        trackerProxyObj = NULL;
        env->DeleteGlobalRef(trackerProxyObj);
        env->DeleteGlobalRef(trackerProxyClass);
        env->DeleteGlobalRef(dataUpdateObj);
        env->DeleteGlobalRef(dataUpdateClass);
        env->DeleteGlobalRef(imageClass);
        return (JNI_TRUE);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_startTracker(JNIEnv *env, jobject thiz)
    {
        LOGD("startTracker");
        if (!tracker) return (JNI_FALSE);

        rc_startTracker(tracker, rc_E_ASYNCRONOUS);

        return (JNI_TRUE);
    }

    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_stopTracker(JNIEnv *env, jobject thiz)
    {
        LOGD("stopTracker");
        if (!tracker) return;
        rc_stopTracker(tracker);
        rc_reset(tracker, 0, rc_POSE_IDENTITY);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_startCalibration(JNIEnv *env, jobject thiz)
    {
        LOGD("startCalibration");
        if (!tracker) return (JNI_FALSE);

        rc_startCalibration(tracker, rc_E_ASYNCRONOUS);

        return (JNI_TRUE);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_startReplay(JNIEnv *env, jobject thiz, jstring absFilePath)
    {
        if (tracker) rc_stopTracker(tracker); // Stop the live tracker
        if (replayer) return JNI_FALSE;

        const char *cString = env->GetStringUTFChars(absFilePath, 0);
        LOGD("startReplay: %s", cString);

        replayer = new rc::replay_threaded;
        rc_setStatusCallback(replayer->tracker, status_callback, NULL);
        rc_setDataCallback(replayer->tracker, data_callback, NULL);
        render_data.reset();

        auto result = (jboolean)replayer->open(cString);

        env->ReleaseStringUTFChars(absFilePath, cString);
        return result;
    }
    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_stopReplay(JNIEnv *env, jobject thiz)
    {
        if (!replayer) return JNI_FALSE;
        LOGD("stoppingReplay...");
        jboolean result = replayer->close();
        delete replayer; replayer = nullptr;
        return result;
    }

    JNIEXPORT jstring JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_getCalibration(JNIEnv *env, jobject thiz)
    {
        LOGD("getCalibration");
        if (!tracker) return env->NewStringUTF("");
        const char *cal;
        return env->NewStringUTF(rc_getCalibration(tracker, &cal) ? cal : "");
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_setCalibration(JNIEnv *env, jobject thiz, jstring calString)
    {
        LOGD("setCalibration");
        if (!tracker) return (JNI_FALSE);
        const char *cString = env->GetStringUTFChars(calString, 0);
        jboolean result = (jboolean) rc_setCalibration(tracker, cString);
        env->ReleaseStringUTFChars(calString, cString);
        return (result);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_setOutputLog(JNIEnv *env, jobject thiz, jstring filename)
    {
        LOGD("setOutputLog");
        if (!tracker) return (JNI_FALSE);
        const char *cFilename = env->GetStringUTFChars(filename, 0);
        rc_setOutputLog(tracker, cFilename);
        env->ReleaseStringUTFChars(filename, cFilename);
        return (JNI_TRUE);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_configureCamera(JNIEnv *env, jobject thiz,
                                                                                                  jint width_px, jint height_px, jfloat center_x_px, jfloat center_y_px, jfloat focal_length_x_px, jfloat focal_length_y_px,
                                                                                                  jint depth_width, jint depth_height, jfloat depth_px, jfloat depth_py, jfloat depth_fx, jfloat depth_fy,
                                                                                                  jfloat offsetX, jfloat offsetY, jfloat offsetZ,
                                                                                                  jfloat skew, jboolean fisheye, jfloat fisheye_fov_radians)
    {
        if (!tracker) return (JNI_FALSE);

        //cache intrinsics
        gZIntrinsics.rw = depth_width;
        gZIntrinsics.rh = depth_height;
        gZIntrinsics.rfx = depth_fx;
        gZIntrinsics.rfy = depth_fy;
        gZIntrinsics.rpx = depth_px;
        gZIntrinsics.rpy = depth_py;

        gRGBIntrinsics.rw = width_px;
        gRGBIntrinsics.rh = height_px;
        gRGBIntrinsics.rfx = focal_length_x_px;
        gRGBIntrinsics.rfy = focal_length_y_px;
        gRGBIntrinsics.rpx = center_x_px;
        gRGBIntrinsics.rpy = center_y_px;

        gOffsetX = offsetX;
        gOffsetY = offsetY;
        gOffsetZ = offsetZ;

        const rc_Pose pose =    {0, -1, 0, 0,
                                -1, 0, 0, 0,
                                0, 0, -1, 0};

        rc_configureCamera(tracker, rc_Camera::rc_EGRAY8 , pose, width_px, height_px, center_x_px, center_y_px, focal_length_x_px, focal_length_y_px, skew, fisheye, fisheye_fov_radians);
        return (JNI_TRUE);
    }

    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_receiveAccelerometer(JNIEnv *env, jobject thiz, jfloat x, jfloat y, jfloat z, jlong time_ns)
    {
        if (!tracker) return;
        rc_Vector vec = {x, y, z};
        rc_Timestamp ts = time_ns / 1000; // convert ns to us
        rc_receiveAccelerometer(tracker, ts, vec);
    //	    LOGV("%li accel %f, %f, %f", (long)timestamp, x, y, z);
    }

    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_receiveGyro(JNIEnv *env, jobject thiz, jfloat x, jfloat y, jfloat z, jlong time_ns)
    {
        if (!tracker) return;
        rc_Vector vec = {x, y, z};
        rc_Timestamp ts = time_ns / 1000; // convert ns to us
        rc_receiveGyro(tracker, ts, vec);
    //	    LOGV("%li gyro %f, %f, %f", (long)timestamp, x, y, z);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_receiveImageWithDepth(JNIEnv *env, jobject thiz, jlong time_ns, jlong shutter_time_ns, jboolean force_recognition,
                                                                                                        jint width, jint height, jint stride, jobject colorData, jobject colorImage,
                                                                                                        jint depthWidth, jint depthHeight, jint depthStride, jobject depthData, jobject depthImage)
    {
        if (!tracker) return (JNI_FALSE);

        // cache these refs so we can close them in the callbacks
        jobject colorImageCached = env->NewGlobalRef(colorImage);
        jobject depthImageCached = env->NewGlobalRef(depthImage);

        void *colorPtr = env->GetDirectBufferAddress(colorData);
        if (RunExceptionCheck(env)) return (JNI_FALSE);

        void *depthPtr = env->GetDirectBufferAddress(depthData);
        if (RunExceptionCheck(env)) return (JNI_FALSE);

//        LOGV(">>>>>>>>>>> Synced camera frames received <<<<<<<<<<<<<");

        if (false) {
            rc_receiveImageWithDepth(tracker, rc_EGRAY8, time_ns / 1000, shutter_time_ns / 1000, NULL, false,
                                     width, height, stride, colorPtr, release_image, colorImageCached,
                                     depthWidth, depthHeight, depthStride, depthPtr, release_image, depthImageCached);
        } else {
            rc_receiveImage(tracker, rc_EGRAY8, time_ns / 1000, shutter_time_ns / 1000, NULL, false,
                            width, height, stride, colorPtr, release_image, colorImageCached);
            release_image(depthImageCached);
        }

        return (JNI_TRUE);
    }

    JNIEXPORT jint JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_alignDepth(JNIEnv *env, jobject thisObj, jobject jInputDepthImg, jobject jOutputDepthImg)
    {
        unsigned short *inDepth = nullptr;
        unsigned short *alignedZ = nullptr;
        float pGravity[3] = {0};

        if (jInputDepthImg != NULL)
        {
            inDepth = reinterpret_cast<unsigned short *>(env->GetDirectBufferAddress(jInputDepthImg));
        }

        if (jOutputDepthImg != NULL)
        {
            alignedZ = reinterpret_cast<unsigned short *>(env->GetDirectBufferAddress(jOutputDepthImg));
        }

        if ((inDepth == nullptr) ||
            (alignedZ == nullptr))
        {
            return 2;//SP_STATUS::SP_STATUS_INVALIDARG;
        }

        bool fillHoles = true;

        float invZFocalX = 1.0f / gZIntrinsics.rfx, invZFocalY = 1.0f / gZIntrinsics.rfy;

        memset(alignedZ, 0, gZIntrinsics.rw * gZIntrinsics.rh * 2);

        for (unsigned int y = 0; y < gZIntrinsics.rh; ++y)
        {
            const float tempy = (y - gZIntrinsics.rpy) * invZFocalY;
            for (unsigned int x = 0; x < gZIntrinsics.rw; ++x)
            {
                auto depth = *inDepth++;

                // DSTransformFromZImageToZCamera(gZIntrinsics, zImage, zCamera); // Move from image coordinates to 3D coordinates
                float zCamZ = static_cast<float>(depth);
                float zCamX = zCamZ * (x - gZIntrinsics.rpx) * invZFocalX;
                float zCamY = zCamZ * tempy;


                // DSTransformFromZCameraToRectThirdCamera(translation, zCamera, thirdCamera); // Move from Z to Third
                float thirdCamX = zCamX + gOffsetX;
                float thirdCamY = zCamY + gOffsetY;
                float thirdCamZ = zCamZ + gOffsetZ;

                // DSTransformFromThirdCameraToRectThirdImage(gRGBIntrinsics, thirdCamera, thirdImage); // Move from 3D coordinates back to image coordinates
                int thirdImageX = static_cast<int>(gRGBIntrinsics.rfx * (thirdCamX / thirdCamZ) + gRGBIntrinsics.rpx + 0.5f);
                int thirdImageY = static_cast<int>(gRGBIntrinsics.rfy * (thirdCamY / thirdCamZ) + gRGBIntrinsics.rpy + 0.5f);

                // The aligned image is the same size as the original depth image
                int alignedImageX = thirdImageX * gZIntrinsics.rw / gRGBIntrinsics.rw;
                int alignedImageY = thirdImageY * gZIntrinsics.rh / gRGBIntrinsics.rh;

                // Clip anything that falls outside the boundaries of the aligned image
                if (alignedImageX < 0 || alignedImageY < 0 || alignedImageX >= static_cast<int>(gZIntrinsics.rw) || alignedImageY >= static_cast<int>(gZIntrinsics.rh))
                {
                    continue;
                }

                // Write the current pixel to the aligned image
                auto & outDepth = alignedZ[alignedImageY * gZIntrinsics.rw + alignedImageX];
                auto minDepth = (depth > outDepth)? outDepth : depth;
                outDepth = outDepth ? minDepth : depth;
            }
        }

        // OPTIONAL: This does a very simple hole-filling by propagating each pixel into holes to its immediate left and below
        if(fillHoles)
        {
            auto out = alignedZ;
            for (unsigned int y = 0; y < gZIntrinsics.rh; ++y)
            {
                for(unsigned int x = 0; x < gZIntrinsics.rw; ++x)
                {
                    if(!*out)
                    {
                        if (x + 1 < gZIntrinsics.rw && out[1])
                        {
                            *out = out[1];
                        }
                        else
                        {
                            if (y + 1 < gZIntrinsics.rh && out[gZIntrinsics.rw])
                            {
                                *out = out[gZIntrinsics.rw];
                            }
                            else
                            {
                                if (x + 1 < gZIntrinsics.rw && y + 1 < gZIntrinsics.rh && out[gZIntrinsics.rw + 1])
                                {
                                    *out = out[gZIntrinsics.rw + 1];
                                }
                            }
                        }
                    }
                    ++out;
                }
            }
        }
        return 0;//SP_STATUS::SP_STATUS_SUCCESS;
    }

    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_MyRenderer_setup(JNIEnv *env, jobject thiz, jint width, jint height)
    {
        if (!tracker) return;
//        LOGD("setup(%d, %d)", width, height);
        vis.setup(width, height);
    }

    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_MyRenderer_render(JNIEnv *env, jobject thiz, jint width, jint height)
    {
        if (!tracker) return;
//        LOGV("render(%d, %d)", width, height);
        vis.render(width, height);
    }

    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_MyRenderer_teardown(JNIEnv *env, jobject thiz)
    {
        if (!tracker) return;
//        LOGD("teardown()");
        vis.teardown();
    }

    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_MyGLSurfaceView_handleDrag(JNIEnv *env, jobject thiz, jfloat x, jfloat y)
    {
        if (!tracker) return;
//        LOGV("handleDrag(%f,%f)", x, y);
        vis.mouse_move(x, y);
    }

    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_MyGLSurfaceView_handleDragEnd(JNIEnv *env, jobject thiz)
    {
        if (!tracker) return;
//        LOGV("handleDragEnd()");
        vis.mouse_up();
    }

    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_MyGLSurfaceView_handlePinch(JNIEnv *env, jobject thiz, jdouble pixelDist)
    {
        if (!tracker) return;
//        LOGV("handlePinch(%f)", pixelDist);
        vis.scroll(pixelDist);
    }

    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_MyGLSurfaceView_handlePinchEnd(JNIEnv *env, jobject thiz)
    {
        if (!tracker) return;
        LOGV("handlePinchEnd()");
        vis.scroll_done();
    }
}
