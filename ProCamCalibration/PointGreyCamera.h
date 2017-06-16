#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <FlyCapture2.h>

class PointGreyCamera
{
private:
	FlyCapture2::FC2Version fc2Version;
	FlyCapture2::Camera fle3camera;
	FlyCapture2::PGRGuid guid;
	FlyCapture2::BusManager busMgr;
	FlyCapture2::Property prop;
	// PointGrey Camera frame buffer
	FlyCapture2::Image rawImage;
	FlyCapture2::Image bgrImage;

	// Homography region of full captured frame
	cv::Rect cameraROI;
	// OpenCV Image buffer
	IplImage* iplBuffer;
	cv::Mat matBuffer;

public:
	FlyCapture2::Error error;
	cv::Mat frame;

	PointGreyCamera();
	~PointGreyCamera();

	cv::Mat Capture();
	void Stop();
};