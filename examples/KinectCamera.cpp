#include "KinectCamera.h"

KinectCamera::KinectCamera() : listener(
    libfreenect2::Frame::Color | libfreenect2::Frame::Ir | libfreenect2::Frame::Depth) {
}

KinectCamera::~KinectCamera() {
    shutdown();
}

bool KinectCamera::initialize(const std::string &serial) {
    if (freenect2.enumerateDevices() == 0) {
        std::cout << "No device connected!" << std::endl;
        return false;
    }

    if (serial.empty()) {
        const std::string defaultSerial = freenect2.getDefaultDeviceSerialNumber();
        dev = freenect2.openDevice(defaultSerial, new libfreenect2::OpenGLPacketPipeline());
    } else {
        dev = freenect2.openDevice(serial, new libfreenect2::OpenGLPacketPipeline());
    }

    if (dev == nullptr) {
        std::cout << "Failure opening device!" << std::endl;
        return false;
    }

    dev->setColorFrameListener(&listener);
    dev->setIrAndDepthFrameListener(&listener);

    if (!dev->start()) {
        std::cout << "Failed to start device!" << std::endl;
        return false;
    }

    registration = new libfreenect2::Registration(dev->getIrCameraParams(), dev->getColorCameraParams());

    return true;
}


std::vector<cv::Mat> KinectCamera::getFrames() {
    std::vector<cv::Mat> outputFrames;

    if (!listener.waitForNewFrame(frames, 1000)) {
        std::cout << "Timeout waiting for new frame!" << std::endl;
        return outputFrames;  // Return empty vector if frame retrieval fails
    }

    auto RGBFrame = frames[libfreenect2::Frame::Color];
    auto depthFrame = frames[libfreenect2::Frame::Depth];
    auto IRFrame = frames[libfreenect2::Frame::Ir];

    cv::Mat RGBMat(static_cast<int>(RGBFrame->height), static_cast<int>(RGBFrame->width), CV_8UC4, RGBFrame->data);
    cv::Mat IRMat(static_cast<int>(IRFrame->height), static_cast<int>(IRFrame->width), CV_32FC1, IRFrame->data);
    cv::Mat depthMat(static_cast<int>(depthFrame->height), static_cast<int>(depthFrame->width), CV_32FC1, depthFrame->data);

    flip(RGBMat, RGBMat, 1);
    flip(IRMat, IRMat, 1);
    flip(depthMat, depthMat, 1);


    // Normalize IR and Depth frames
    cv::Mat IRMatNormalized, depthMatNormalized;
    IRMat.convertTo(IRMatNormalized, CV_32F, 1.0 / 65535.0); // Scale to 0.0 - 1.0 range
    depthMat.convertTo(depthMatNormalized, CV_32F, 1.0 / 4499.92); // Scale to 0.0 - 1.0 range

    // Apply square root operation to IRMatNormalized
    cv::Mat IRMatNormalizedSquareroot;
    cv::sqrt(IRMatNormalized, IRMatNormalizedSquareroot);

    // Resize the RGB image
    cv::Size newSize(640, 480); // Specify the new size here
    cv::Mat RGBMatNormalized;
    cv::resize(RGBMat, RGBMatNormalized, newSize);

    // Store all frames in the vector
    outputFrames.push_back(RGBMatNormalized);
    outputFrames.push_back(IRMatNormalized);
    outputFrames.push_back(IRMatNormalizedSquareroot);
    outputFrames.push_back(depthMatNormalized);

    listener.release(frames);

    return outputFrames;
}


void KinectCamera::shutdown() {
    application_shutdown = true;
    dev->stop();
    dev->close();
    delete registration;
}
