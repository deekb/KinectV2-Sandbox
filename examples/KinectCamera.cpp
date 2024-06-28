#include "KinectCamera.h"


KinectCamera::KinectCamera() :
    device(nullptr),
    pipeline(nullptr),
    registration(nullptr),
    rgbFrame(nullptr),
    depthFrame(nullptr),
    listener(nullptr) {}

KinectCamera::~KinectCamera() {
    shutdown();
}

bool KinectCamera::initialize() {
    if (!configureDevice()) {
        return false;
    }

    listener = new libfreenect2::SyncMultiFrameListener(
        libfreenect2::Frame::Color | libfreenect2::Frame::Depth);
    device->setColorFrameListener(listener);
    device->setIrAndDepthFrameListener(listener);

    if (!device->start()) {
        return false;
    }

    return true;
}

void KinectCamera::shutdown() {
    if (device) {
        device->stop();
        device->close();
        delete device;
        device = nullptr;
    }

    if (listener) {
        delete listener;
        listener = nullptr;
    }

    if (registration) {
        delete registration;
        registration = nullptr;
    }

    if (rgbFrame) {
        delete rgbFrame;
        rgbFrame = nullptr;
    }

    if (depthFrame) {
        delete depthFrame;
        depthFrame = nullptr;
    }
}

bool KinectCamera::captureFrame(cv::Mat& frame) {
    if (!listener || !device) {
        return false;
    }

    if (!listener->waitForNewFrame(frameMap, 1000)) {
        return false;
    }

    rgbFrame = frameMap[libfreenect2::Frame::Color];
    const cv::Mat rgbMat(rgbFrame->height, rgbFrame->width, CV_8UC4, rgbFrame->data);
    cvtColor(rgbMat, frame, cv::COLOR_BGRA2BGR);

    listener->release(frameMap);
    return true;
}

bool KinectCamera::configureDevice() {
    const std::string serial = freenect2.getDefaultDeviceSerialNumber();
    if (serial.empty())
    {
        return false;
    }

    pipeline = new libfreenect2::OpenGLPacketPipeline();
    device = freenect2.openDevice(serial, pipeline);

    if (!device) {
        return false;
    }

    registration = new libfreenect2::Registration(device->getIrCameraParams(), device->getColorCameraParams());

    return true;
}
