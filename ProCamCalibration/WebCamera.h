//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This is camera class for PointGrey camera
// If you use other camera (ex: usb camera), you should change this class, espetially "initCam" and "AsaCapture"
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


#pragma once

#include <iostream>

// opencv include & liblary
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/videoio/videoio.hpp>

class WebCamera
{
	private:
		cv::VideoCapture *cap;

	public:
		WebCamera(void){};
		~WebCamera(void){};

		// initialize
		void Initialize()
		{
			cap = new cv::VideoCapture();
			cap->open(0);
			if (!cap->isOpened())
			{
				std::cout << "Error: we can't open webcam" << std::endl;
				return;
			}

			cv::Mat frame;
			for (int ii = 0; ii < 5; ii++)
			{
				cap->operator>>(frame);
			}
		}

		// capture
		cv::Mat Capture()
		{
			cv::Mat frame;
			//cap->operator>>(frame);
			for (int ii = 0; ii < 5; ii++)
			{
				cap->operator>>(frame);
			}
			return frame;
		}
};