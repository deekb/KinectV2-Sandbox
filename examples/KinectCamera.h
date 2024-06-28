#ifndef KINECTCAMERA_H
#define KINECTCAMERA_H

#include <opencv2/opencv.hpp>
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener.hpp>
#include <libfreenect2/registration.h>
#include <libfreenect2/packet_pipeline.h>
#include "libfreenect2/frame_listener_impl.h"

class KinectCamera {
public:
    KinectCamera();
    ~KinectCamera();

    bool initialize();
    void shutdown();
    bool captureFrame(cv::Mat& frame);

private:
    libfreenect2::Freenect2 freenect2;
    libfreenect2::Freenect2Device* device;
    libfreenect2::PacketPipeline* pipeline;
    libfreenect2::Registration* registration;
    libfreenect2::Frame* rgbFrame;
    libfreenect2::Frame* depthFrame;
    libfreenect2::FrameMap frameMap;
    libfreenect2::SyncMultiFrameListener* listener;

    bool configureDevice();
};

#endif // KINECTCAMERA_H
