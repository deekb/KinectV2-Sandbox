#ifndef KINECT_CAMERA_H
#define KINECT_CAMERA_H

#include <iostream>
#include <opencv2/opencv.hpp>
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>
#include <vector>

class KinectCamera {
    libfreenect2::Freenect2 freenect2;
    libfreenect2::Freenect2Device *dev = nullptr;
    libfreenect2::SyncMultiFrameListener listener;
    libfreenect2::FrameMap frames;
    libfreenect2::Registration *registration = nullptr;
    bool application_shutdown = false;

public:
    KinectCamera();

    ~KinectCamera();

    bool initialize(const std::string &serial = "");

    std::vector<cv::Mat> getFrames();

    void shutdown();
};

#endif // KINECT_CAMERA_H
