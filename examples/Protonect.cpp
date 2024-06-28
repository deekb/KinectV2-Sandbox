#include <iostream>
#include <cstdlib>
#include <csignal>
#include <opencv2/opencv.hpp>
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>
#include <libfreenect2/packet_pipeline.h>
#include <fstream>

// Constants for denoising parameters
constexpr float DENOISE_H = 2.0;                // Filter strength (lower preserves more details)
constexpr float DENOISE_H_COLOR = 0.0;          // Strength of color components
constexpr int DENOISE_TEMPLATE_WINDOW_SIZE = 5;// Size of the pixel neighborhood
constexpr int DENOISE_SEARCH_WINDOW_SIZE = 15;  // Size of the window for finding similar patches

bool application_shutdown = false;

void sigint_handler(int s)
{
    application_shutdown = true;
}

int main(int argc, char *argv[])
{
    std::string program_path(argv[0]);
    std::cout << "Version: " << LIBFREENECT2_VERSION << std::endl;
    std::cout << "Usage: " << program_path << " [<device serial>] [-help] [-version]" << std::endl;

    libfreenect2::Freenect2 freenect2;
    libfreenect2::Freenect2Device *dev = nullptr;
    libfreenect2::PacketPipeline *pipeline = nullptr;

    std::string serial;


    for(int argI = 1; argI < argc; ++argI)
    {
        const std::string arg(argv[argI]);

        if(arg == "-help" || arg == "--help" || arg == "-h" || arg == "-v" || arg == "--version" || arg == "-version")
        {
            return 0;
        }
        if(arg.find_first_not_of("0123456789") == std::string::npos)
        {
            serial = arg;
        }
        else
        {
            std::cout << "Unknown argument: " << arg << std::endl;
        }
    }

    if(freenect2.enumerateDevices() == 0)
    {
        std::cout << "No device connected!" << std::endl;
        return -1;
    }

    if (serial.empty())
    {
        serial = freenect2.getDefaultDeviceSerialNumber();
    }

    if(pipeline)
    {
        dev = freenect2.openDevice(serial, pipeline);
    }
    else
    {
        dev = freenect2.openDevice(serial);
    }

    if(dev == nullptr)
    {
        std::cout << "Failure opening device!" << std::endl;
        return -1;
    }

    application_shutdown = false;

    int types = 0;
    types |= libfreenect2::Frame::Color;
    types |= libfreenect2::Frame::Ir | libfreenect2::Frame::Depth;
    libfreenect2::SyncMultiFrameListener listener(types);
    libfreenect2::FrameMap frames;

    dev->setColorFrameListener(&listener);
    dev->setIrAndDepthFrameListener(&listener);

    if (!dev->start())
        return -1;

    std::cout << "device SN: " << dev->getSerialNumber() << std::endl;
    std::cout << "device FW: " << dev->getFirmwareVersion() << std::endl;

    auto* registration = new libfreenect2::Registration(dev->getIrCameraParams(), dev->getColorCameraParams());
    libfreenect2::Frame undistorted(512, 424, 4), registered(512, 424, 4);

    size_t framecount = 0;

    while(!application_shutdown)
    {
        if (!listener.waitForNewFrame(frames, 1000))
        {
            std::cout << "Timeout!" << std::endl;
            return -1;
        }

        // Convert frames to OpenCV Mat
        auto rgbFrame = frames[libfreenect2::Frame::Color];
        cv::Mat rgbMat(rgbFrame->height, rgbFrame->width, CV_8UC4, rgbFrame->data);        auto irMat = cv::Mat(frames[libfreenect2::Frame::Ir]->height, frames[libfreenect2::Frame::Ir]->width, CV_32FC1, frames[libfreenect2::Frame::Ir]->data);
        auto depthMat = cv::Mat(frames[libfreenect2::Frame::Depth]->height, frames[libfreenect2::Frame::Depth]->width, CV_32FC1, frames[libfreenect2::Frame::Depth]->data);

        // Calculate min, max, and average for depth and IR frames
        double minVal, maxVal;
        cv::Scalar meanVal;

        cv::minMaxLoc(depthMat, &minVal, &maxVal);
        meanVal = cv::mean(depthMat);
        std::cout << "Depth - Min: " << minVal << " Max: " << maxVal << " Avg: " << meanVal[0] << std::endl;

        cv::minMaxLoc(irMat, &minVal, &maxVal);
        meanVal = cv::mean(irMat);
        std::cout << "IR - Min: " << minVal << " Max: " << maxVal << " Avg: " << meanVal[0] << std::endl;

        // Normalize IR and Depth frames
        cv::Mat irMatNormalized, depthMatNormalized;
        irMat.convertTo(irMatNormalized, CV_8UC1, 255.0 / 65535.0);
        depthMat.convertTo(depthMatNormalized, CV_8UC1, 255.0 / 4499.92);

        // Perform any necessary operations on the frames (e.g., depth mapping, registration)
        // registration->apply(rgb, depth, &undistorted, &registered);

        // Resize the image
        cv::Size newSize(640, 480); // Specify the new size here (e.g., 640x480)
        cv::Mat resizedMat;
        cv::resize(rgbMat, resizedMat, newSize);


        // Apply Gaussian blur for noise reduction
        cv::Mat filteredMat;
        cv::GaussianBlur(resizedMat, filteredMat, cv::Size(5, 5), 0);

        // Apply Non-local Means Denoising with adjusted parameters
        cv::Mat denoisedMat;
        cv::fastNlMeansDenoisingColored(resizedMat, denoisedMat,
                                        DENOISE_H, DENOISE_H_COLOR,
                                        DENOISE_TEMPLATE_WINDOW_SIZE,
                                        DENOISE_SEARCH_WINDOW_SIZE);
        // Display the denoised image
        imshow("Denoised RGB", denoisedMat);

        // Display the denoised image
        imshow("Denoised RGB", denoisedMat);

        // Display the filtered image
        imshow("Filtered RGB", filteredMat);

        // Display the resized image
        imshow("Resized RGB", resizedMat);

        // Display frames
        imshow("RGB", rgbMat);
        imshow("IR", irMatNormalized);
        imshow("Depth", depthMatNormalized);

        int key = cv::waitKey(1);
        if (key == 27) // ESC key to exit
            application_shutdown = true;

        listener.release(frames);
        framecount++;
    }

    dev->stop();
    dev->close();

    delete registration;

    return 0;
}
