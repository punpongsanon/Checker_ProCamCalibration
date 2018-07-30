#include <opencv2/imgproc/imgproc.hpp>

#include "PointGreyCamera.h"
#include "Configuration.h"

using namespace cv;
using namespace std;
using namespace FlyCapture2;

PointGreyCamera::PointGreyCamera()
{
	// Allocate cpu mat
	frame.create(CAM_H, CAM_W, CV_8UC1);
	// Allocate buffer
	iplBuffer = cvCreateImage(Size(CAM_W, CAM_H), IPL_DEPTH_8U, IMG_CHANNEL);
	matBuffer.create(CAM_H, CAM_W, CV_8UC1);

	// Initial the interest region in full frame camera resolusion
	cameraROI.x = 0;
	cameraROI.y = 0;
	cameraROI.width = PROC_CAM_W;
	cameraROI.height = PROC_CAM_H;

	char press;

	cout << "<CAMERA-SETUP>" << endl;
	// Get FlyCapture Version
	Utilities::GetLibraryVersion(&fc2Version);
	char version[128];
	sprintf(version,
			"FlyCapture2 library version: %d.%d.%d.%d\n",
			fc2Version.major, 
			fc2Version.minor, 
			fc2Version.type, 
			fc2Version.build);
	printf(version);
	// Get the camera, Set the connection
	error = busMgr.GetCameraFromIndex(0, &guid);
	cout << "<camera> GetCameraFromIndex: " << error.GetDescription() << endl;
	fle3camera.Connect(&guid);
	// Set Video mode and frame rate
	//error = fle3camera.SetVideoModeAndFrameRate(FlyCapture2::VIDEOMODE_1280x960RGB, FlyCapture2::FRAMERATE_60);
	cout << "<camera> SetVideoModeAndFrameRate: " << error.GetDescription() << endl;
	prop.absControl = true;
	prop.onePush = false;
	prop.onOff = true;
	prop.autoManualMode = false;
	// Set shutter speed
	prop.type = FlyCapture2::SHUTTER;
	prop.absControl = true;
	prop.absValue = ((float)(1.0 / 33.0*1000.0));
	error = fle3camera.SetProperty(&prop);
	cout << "<camera> [Set] Shutter Speed: " << error.GetDescription() << endl;
	// Set gain
	prop.type = FlyCapture2::GAIN;
	prop.absControl = true;
	prop.absValue = 0.0;
	error = fle3camera.SetProperty(&prop);
	cout << "<camera> [Set] Gain: " << error.GetDescription() << endl;
	// Set Brightness
	prop.type = FlyCapture2::BRIGHTNESS;
	prop.absControl = true;
	prop.absValue = 0.0;
	error = fle3camera.SetProperty(&prop);
	cout << "<camera> [Set] Brightness: " << error.GetDescription() << endl;
	// Set White Balance
	prop.absControl = false;
	prop.type = FlyCapture2::WHITE_BALANCE;
	prop.valueA = 760;
	prop.valueB = 600;
	error = fle3camera.SetProperty(&prop);
	cout << "<camera> [Set] W/B: " << error.GetDescription() << endl;
	// Set Exposure
	prop.type = FlyCapture2::AUTO_EXPOSURE;
	prop.onOff = true;
	prop.autoManualMode = false;
	prop.absControl = true;
	prop.absValue = 0.0;
	error = fle3camera.SetProperty(&prop);
	cout << "<camera> [Set] Exposure: " << error.GetDescription() << endl;
	// Set image gamma
	prop.type = FlyCapture2::GAMMA;
	prop.onOff = true;
	prop.absControl = true;
	prop.absValue = 1.1;
	error = fle3camera.SetProperty(&prop);
	cout << "<camera> [Set] Gamma: " << error.GetDescription() << endl;

	// Start Capturing
	error = fle3camera.StartCapture();
	cout << "<camera> Capture Status: " << error.GetDescription() << endl;
}

PointGreyCamera::~PointGreyCamera()
{
	// Release captured storage buffer
	frame.release();
	matBuffer.release();
	cvReleaseImage(&iplBuffer);

	// Release the camera frame buffer
	rawImage.ReleaseBuffer();
	bgrImage.ReleaseBuffer();

	char press;
	system("cls");
	// Stop Capturing
	//error = fle3camera.StopCapture();
	//cout << "<camera> StopCapture Status: " << error.GetDescription() << endl;
	//if (error != PGRERROR_OK) cin >> press;

	// Disconnect camera
	fle3camera.Disconnect();
	if (error != PGRERROR_OK) cin >> press;
	cout << "<camera> Disconnect Status: " << error.GetDescription() << endl;
}

// ----------------------------------------------------------------
// Sharpen image using "unsharp mask" algorithm (unused, 20131015)
// ----------------------------------------------------------------
// Mat blurred; double sigma = 1, threshold = 5, amount = 1;
// GaussianBlur(img, blurred, Size(), sigma, sigma);
// Mat lowConstrastMask = abs(img - blurred) < threshold;
// Mat sharpened = img*(1+amount) + blurred*(-amount);
// img.copyTo(sharpened, lowContrastMask);
// ----------------------------------------------------------------

cv::Mat PointGreyCamera::Capture()
{
	// Retrive current frame from the camera buffer (camera capture every timestep)
	error = fle3camera.RetrieveBuffer(&rawImage);
	if (error != PGRERROR_OK) cout << "<Camera> RetrieveBuffer Status: " << error.GetDescription() << endl;
	rawImage.Convert(FlyCapture2::PIXEL_FORMAT_BGR, &bgrImage);

	return cv::Mat(bgrImage.GetRows(), bgrImage.GetCols(), CV_8UC3, bgrImage.GetData());

	// Convert the Y8 format code to OpenCV likes (Y8/Y16/RGB -> GRAY/BGR)
	// NOTE: For mono camera (Fle3-u3-13s2m), please used ** MONO8 ** (No RAW8!)
	//size_t imageByte = CAM_W * CAM_H * IMG_CHANNEL;
	//memcpy(iplBuffer->imageData, rawImage.GetData(), imageByte);

	// Store frame buffer to image buffer AND Resize camera frame to processing frame
	//resize(cv::cvarrToMat(iplBuffer), matBuffer, Size(PROC_CAM_W, PROC_CAM_H));
	//frame = matBuffer(cameraROI);
	//return frame;
}

void PointGreyCamera::Stop()
{
	char press;

	// Stop Capturing
	error = fle3camera.StopCapture();
	cout << "<camera> StopCapture Status: " << error.GetDescription() << endl;
	if (error != PGRERROR_OK) cin >> press;

	return;
}