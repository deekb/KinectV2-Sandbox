#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/viz.hpp>
#include "KinectCamera.h"
#include <fstream>
#include <ctime>

bool application_shutdown = false;

// Function to export point cloud to a PLY file
void exportPointCloudToPLY(const std::vector<cv::Point3f>& pointCloud) {
    // Generate file name with current date-time
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "pointcloud-%Y-%m-%d-%H-%M-%S.ply", timeinfo);
    std::string filename(buffer);

    // Write PLY header and data
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing: " << filename << std::endl;
        return;
    }

    // Write PLY header
    outFile << "ply" << std::endl;
    outFile << "format ascii 1.0" << std::endl;
    outFile << "element vertex " << pointCloud.size() << std::endl;
    outFile << "property float x" << std::endl;
    outFile << "property float y" << std::endl;
    outFile << "property float z" << std::endl;
    outFile << "end_header" << std::endl;

    // Write point cloud data
    for (const auto& pt : pointCloud) {
        outFile << pt.x << " " << pt.y << " " << pt.z << std::endl;
    }

    outFile.close();
    std::cout << "Point cloud saved to " << filename << std::endl;
}

void visualizePointCloud(cv::viz::Viz3d& visualizer, const std::vector<cv::Point3f>& pointCloud) {
    cv::viz::WCloud cloudWidget(pointCloud, cv::viz::Color::green());
    visualizer.setRepresentation(cv::viz::REPRESENTATION_WIREFRAME);
    // Create camera pose
    cv::Vec3d cam_position(0, 0, -10); // Camera position in world coordinates
    cv::Vec3d cam_focal_point(0, 0, 0); // Look at the origin
    cv::Vec3d cam_y_dir(0.0, 1, 0); // Up direction (Y-axis)

    cv::Affine3d cam_pose = cv::viz::makeCameraPose(cam_position, cam_focal_point, cam_y_dir);
    visualizer.showWidget("Point Cloud", cloudWidget);
    visualizer.setViewerPose(cam_pose);
    visualizer.spinOnce(1, true);
}

void createPointCloud(const cv::Mat& depthMatNormalized, std::vector<cv::Point3f>& pointCloud) {
    // Clear existing point cloud data
    pointCloud.clear();

    // Assuming depthMat is already normalized and scaled to real-world values
    for (int y = 0; y < depthMatNormalized.rows; ++y) {
        for (int x = 0; x < depthMatNormalized.cols; ++x) {
            float depthValue = -depthMatNormalized.at<float>(y, x);

            float X = (x - 509.901) * depthValue / 711.629;
            float Y = depthValue;
            float Z = (y - 403.607) * depthValue / 710.944;

            cv::Point3f point(X, Y, Z);
            pointCloud.push_back(point);
        }
    }
}

int main(int argc, char *argv[]) {
    std::string serial;
    cv::Mat RGBMatNormalized, IRMatNormalized, IRMatNormalizedSquareroot, depthMatNormalized;

    if (argc > 1) {
        serial = argv[1];
    }

    KinectCamera kinect;
    if (!kinect.initialize(serial)) {
        std::cerr << "Failed to initialize Kinect camera!" << std::endl;
        return -1;
    }

    // Create Viz3d object for point cloud visualization
    cv::viz::Viz3d visualizer("Point Cloud");

    std::vector<cv::Point3f> pointCloud;

    while (!application_shutdown) {
        std::vector<cv::Mat> matVector = kinect.getFrames();
        RGBMatNormalized = matVector[0];
        IRMatNormalized = matVector[1];
        IRMatNormalizedSquareroot = matVector[2];
        depthMatNormalized = matVector[3];

        // Display images
        cv::imshow("Resized RGB", RGBMatNormalized);
        cv::imshow("IR", IRMatNormalized);
        cv::imshow("IR-SQRT", IRMatNormalizedSquareroot);
        cv::imshow("Depth", depthMatNormalized);

        // Create or update point cloud visualization
        createPointCloud(depthMatNormalized, pointCloud);
        visualizePointCloud(visualizer, pointCloud);

        // Wait for key press
        int keyCode = cv::waitKey(1);

        if (keyCode == 'p' || keyCode == 'P') {
            // Export point cloud to PLY file
            exportPointCloudToPLY(pointCloud);
        } else if (keyCode == 27) { // ESC key to exit
            application_shutdown = true;
        }
    }

    return 0;
}
