#include "Configuration.h"
#ifndef WEBCAM_MODE
#include "PointGreyCamera.h"
#else
#include "WebCamera.h"
#endif
#include "StructurePattern.h"

#ifndef WEBCAM_MODE
PointGreyCamera *pointGreyCamera;
#else
WebCamera *webCamera;
#endif

int CalibrationMode = -1;
std::vector<cv::Point2f> CameraPoints;

namespace
{
	static const std::string WINDOWNAME_CAP = "Captured Image";
	static const std::string WINDOWNAME_SRC = "Source Image";
	static const std::string WINDOWNAME_PRJ = "Projection";

	static const std::string C2P_SELECTED_PT = "Data\\C2P_selected.txt";
	static const std::string CAM_CALIB_PATH = "Data\\Calibration\\cam_";
	static const std::string PRO_CALIB_PATH = "Data\\Calibration\\pro_";
	static const std::string C2P_CALIB_PATH = "Data\\Calibration\\P2C";
	static const std::string C2P_TOTEXT_PATH = "Data\\Calibration\\P2C\\C2P_lookup.txt";
	static const std::string EXT_CAM_PRO_PATH = "Data\\Calibration\\ProCam_ExtrinsicParameter.txt";
	static const std::string PNP_OBJ_PT_PATH = "Data\\Calibration\\Object_3D_Points.txt";
	static const std::string PNP_CALIB_PATH = "Data\\Calibration\\PNP";
	static const std::string IMAGE_EXT = ".png";

	static const std::string XMLTAG_CAMERAMAT = "CameraMatrix";
	static const std::string XMLTAG_DISTCOEFFS = "DistortCoeffs";

	enum eCheck
	{
		CHESS,
		CIRCLES
	};

	static eCheck checkPattern = CHESS;
}

void Quit()
{
	// Stop camera (and free its memory)
	#ifndef WEBCAM_MODE
	pointGreyCamera->Stop();
	if (pointGreyCamera) delete pointGreyCamera;
	#else
	delete webCamera;
	#endif	

	exit(0);
}

void MouseCallback(int event, int x, int y, int flags, void* param)
{
	//cv::Mat* image = static_cast<cv::Mat*>(param);
	cv::Point2f SelectImagePoint = cv::Point2f(0.0, 0.0);

	switch (event)
	{
	case cv::EVENT_LBUTTONDOWN:
		SelectImagePoint = cv::Point2f(x, y);
		CameraPoints.push_back(SelectImagePoint);
		std::cout << "Image(camera) corrdinate: " << x << "," << y << " ** RECORDED." << std::endl;
		break;
	}
}

int CaptureImage(cv::Mat Frame, int* ImgNumber)
{
	cv::Mat temp;
	cv::resize(Frame, temp, cv::Size(), 0.80, 0.80);
	cv::imshow(WINDOWNAME_CAP, temp);

	int key = cv::waitKey(30);

	// C: CAPTURE
	if (key == 'c')
	{
		std::stringstream stream;
		stream << CAM_CALIB_PATH << CAM_ID << "\\source" << *ImgNumber << IMAGE_EXT;
		std::string  fileName = stream.str();
		cv::imwrite(fileName, Frame);
		std::cout << "Save Captured image: " << fileName << std::endl;
		(*ImgNumber)++;
	}
	// Q: FINISH CAPTURE
	else if (key == 'q')
	{
		std::cout << "Captured End" << std::endl;
		return 1;
	}
	// ESC: Exit program
	else if (key == '\x1b')
	{
		return -1;
	}
	return 0;
}

void Display(void)
{
	// 1. Setup parameter
	int imageNumber = 0;
	// Define checker pattern size and number of inner corner
	float checkSize;
	int checkPointX, checkPointY;
	if (CalibrationMode == CAM_CALIBRATE_MODE)
	{
		checkSize = 22.0f;
		checkPointX = 7, checkPointY = 9;
	}
	else if (CalibrationMode == PRO_CALIBRATE_MODE)
	{
		//checkSize = 114.0f / 5.0f;
		checkSize = 16.1f;
		checkPointX = 7, checkPointY = 9;
	}
	int pattern = CHESS;
	if (pattern == CHESS) checkPattern = CHESS;
	else if (pattern == CIRCLES) checkPattern = CIRCLES;

	// 3. Capturing process
	if (CalibrationMode == CAM_CALIBRATE_MODE)
	{
		// Set capturing screen
		cv::namedWindow(WINDOWNAME_CAP, CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);
		while (true)
		{
			#ifndef WEBCAM_MODE
			cv::Mat frame = pointGreyCamera->Capture();
			#else
			cv::Mat frame = webCamera->Capture();
			#endif
			//cv::flip(capture_img, capture_img, 1);

			// Show temporary image (with resize)
			cv::Mat displayFrame;
			cv::resize(frame, displayFrame, cv::Size(DISC_CAM_W, DISC_CAM_H), 640.0f / DISC_CAM_W, 480.0f / DISC_CAM_H);
			cv::imshow(WINDOWNAME_CAP, displayFrame);
			int key = cv::waitKey(30);

			// C: CAPTURE
			if (key == 'c')
			{
				std::stringstream stream;
				stream << CAM_CALIB_PATH << CAM_ID << "\\source" << imageNumber << IMAGE_EXT;
				std::string  fileName = stream.str();
				cv::imwrite(fileName, frame);
				std::cout << "Save Captured image: " << fileName << std::endl;
				imageNumber++;
			}
			// Q: FINISH CAPTURE
			else if (key == 'q')
			{
				std::cout << "Captured End" << std::endl;
				break;
			}
			// ESC: Exit program
			else if (key == '\x1b') Quit();
		}
		cv::destroyWindow(WINDOWNAME_CAP);
	}
	else if (CalibrationMode == PRO_CALIBRATE_MODE)
	{
		// Set capturing screen
		cv::namedWindow(WINDOWNAME_CAP, CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);

		// Board projection parameter
		int x = checkPointY + 1, y = checkPointX + 1;
		int xZoom = 50, yZoom = 50;
		int xTrans = 200, yTrans = 200;

		// Capture parameter
		std::cout << " Board Controller [z,x] = scale (zoom) : [a,w,s,d] = translate" << std::endl;
		while (true)
		{
			#ifndef WEBCAM_MODE
			cv::Mat frame = pointGreyCamera->Capture();
			#else
			cv::Mat frame = webCamera->Capture();
			#endif
			//cv::flip(capture_img, capture_img, 1);

			// Setup projection board
			cv::Mat projectionBoard(WIN_H, WIN_W, CV_8UC3);
			projectionBoard = cv::Scalar(BACKGROUND_PROC, BACKGROUND_PROC, BACKGROUND_PROC);
			if ((x*xZoom + x + xTrans) < WIN_W && (y*yZoom + y + yTrans) < WIN_H)
			{
				bool tm = true;
				for (int ii = 1; ii <= x; ii++)
				{
					bool fff;
					if (tm) fff = true;
					else fff = false;
					tm = !tm;
					for (int jj = 1; jj <= y; jj++)
					{
						if (fff)
						{
							cv::rectangle(projectionBoard,
								cv::Point((ii - 1) * xZoom + ii + xTrans, (jj - 1) * yZoom + jj + yTrans),
								cv::Point(ii * xZoom + ii + xTrans, jj * yZoom + jj + yTrans),
								cv::Scalar(0, 0, 0),
								-1,
								4);
						}
						fff = !fff;
					}
				}
			}
			cv::cvtColor(projectionBoard, projectionBoard, CV_BGR2RGB);

			// Set projection
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0, 0, 0, 0);
			glViewport(0, 0, WIN_W, WIN_H);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0, WIN_W, 0, WIN_H);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glDrawPixels(projectionBoard.cols, projectionBoard.rows, GL_RGB, GL_UNSIGNED_BYTE, projectionBoard.data);
			glutSwapBuffers();

			// Show temporary image (with resize)
			cv::Mat displayFrame;
			cv::resize(frame, displayFrame, cv::Size(DISC_CAM_W, DISC_CAM_H), 640.0f / DISC_CAM_W, 480.0f / DISC_CAM_H);
			cv::imshow(WINDOWNAME_CAP, displayFrame);
			int key = cv::waitKey(30);

			// C: Capture
			if (key == 'c')
			{
				// Capture image with pojected chessboard pattern
				{
					std::stringstream    stream;
					stream << PRO_CALIB_PATH << PRO_ID << "\\source2_" << imageNumber << IMAGE_EXT;
					std::string  fileName = stream.str();
					cv::imwrite(fileName, frame);
					std::cout << "Save Captured image: " << fileName << std::endl;
				}

				// Save the reference projected chessboard pattern (in the rendered w/o projection)
				{
					cv::cvtColor(projectionBoard, projectionBoard, CV_RGB2BGR);
					std::stringstream stream;
					stream << PRO_CALIB_PATH << PRO_ID << "\\project_" << imageNumber << IMAGE_EXT;
					std::string fileName = stream.str();
					cv::imwrite(fileName, projectionBoard);
					std::cout << "Save ProjectionBoard: " << fileName << std::endl;
				}

				// Capture image w/o projected chessboard pattern (appears only board with yellow marker)
				{
					cv::Mat noboard(WIN_H, WIN_W, CV_8UC3);
					noboard = cv::Scalar(BACKGROUND_PROC, BACKGROUND_PROC, BACKGROUND_PROC);

					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					glClearColor(0.0, 0.0, 0.0, 0.0);
					glViewport(0, 0, WIN_W, WIN_H);
					glMatrixMode(GL_PROJECTION);
					glLoadIdentity();
					gluOrtho2D(0, WIN_W, 0, WIN_H);
					glMatrixMode(GL_MODELVIEW);
					glLoadIdentity();
					glDrawPixels(noboard.cols, noboard.rows, GL_RGB, GL_UNSIGNED_BYTE, noboard.data);
					glutSwapBuffers();
					// Wait for 1 sec = 1000)
					Sleep(DELAYED_MSEC);

					#ifndef WEBCAM_MODE
					cv::Mat frame = pointGreyCamera->Capture();
					#else
					cv::Mat frame = webCamera->Capture();
					#endif
					std::stringstream stream;
					stream << PRO_CALIB_PATH << PRO_ID << "\\source1_" << imageNumber << IMAGE_EXT;
					std::string fileName = stream.str();
					cv::imwrite(fileName, frame);
					std::cout << "Save NoProjectionBoard image:" << fileName << std::endl;
				}

				imageNumber++;
			}
			// Q: Finish capture
			else if (key == 'q') 
			{
				std::cout << "Capture End" << std::endl;
				break;
			}
			// ESC: Exit program immerdately
			else if (key == '\x1b') 
			{
				Quit();
			}
			// Z: Zoom out chessboard
			else if (key == 'z')
			{
				xZoom -= 5; yZoom -= 5;
			}
			// X: Zoom in chessboard
			else if (key == 'x')
			{
				xZoom += 5; yZoom += 5;
			}
			// A: Move chessboard left (10 pixels)
			else if (key == 'a') xTrans -= 10;
			// D: Move chessboard right (10 pixels)
			else if (key == 'd') xTrans += 10;
			// W: Move chessboard up (10 pixels)
			else if (key == 'w') yTrans += 10;
			// S: Move chessboard down (10 pixels)
			else if (key == 's') yTrans -= 10;
		}
		cv::destroyWindow(WINDOWNAME_CAP);
	}

	// [selection] Camera Calibration
	if (CalibrationMode == CAM_CALIBRATE_MODE)
	{
		std::vector<cv::Mat> checkerImgs;
		const cv::Size patternSize(checkPointX, checkPointY);
		std::vector<std::vector<cv::Point2f>> imagePoints(imageNumber);
		std::vector<std::vector<cv::Point3f>> worldPoints(imageNumber);

		// 4.1 Load captured image
		for (int i = 0; i < imageNumber; i++)
		{
			std::stringstream stream;
			stream << CAM_CALIB_PATH << CAM_ID << "\\source" << i << IMAGE_EXT;
			std::string  fileName = stream.str();
			checkerImgs.push_back(cv::imread(fileName));
			std::cout << "Load Captured image: " << fileName << std::endl;
		}

		// 4.2 Dectect corners
		std::cout << "DETECT CORNERS" << std::endl;
		cv::namedWindow(WINDOWNAME_SRC, CV_WINDOW_AUTOSIZE);
		std::vector<int> tnk;
		int flag = 0;
		for (int i = 0; i < imageNumber; i++)
		{
			if (checkPattern == CHESS)
			{
				imagePoints[i].clear();
				if (cv::findChessboardCorners(checkerImgs[i], patternSize, imagePoints[i], CV_CALIB_CB_ADAPTIVE_THRESH)) {
					//std::cout << "********************" << std::endl;
					//std::cout << "ImageID: " << i << " [SUCCESS]" << std::endl;

					// Subpixel
					cv::Mat gray;
					cv::cvtColor(checkerImgs[i], gray, CV_BGR2GRAY);
					cv::cornerSubPix(gray, imagePoints[i], cv::Size(2, 2), cv::Size(-1, -1), cv::TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 10000, 0.001));

					flag++;
					cv::drawChessboardCorners(checkerImgs[i], patternSize, (cv::Mat)(imagePoints[i]), true);
					cv::imshow(WINDOWNAME_SRC, checkerImgs[i]);
					cv::waitKey(30);

					// Save to image
					std::stringstream stream;
					stream << CAM_CALIB_PATH << PRO_ID << "\\source_match_" << i << IMAGE_EXT;
					std::string fileName = stream.str();
					cv::imwrite(fileName, checkerImgs[i]);
				}
				else
				{
					std::cout << "ImageID: " << i << " [FAIL]" << std::endl;
					tnk.push_back(i);
				}
			}
		}
		// Incase of no any image can detect features, then quit
		if (flag == 0) Quit();

		// 4.3 Estimate world point from checker pattern (opject point)
		std::cout << "ESTIMATE WORLD POINTS" << std::endl;
		for (int i = 0; i < imageNumber; i++)
		{
			worldPoints[i].clear();
			for (int k = 0; k < patternSize.height; k++)
			{
				for (int j = 0; j < patternSize.width; j++)
				{
					worldPoints[i].push_back(cv::Point3f((float)(k * checkSize), (float)(j * checkSize), 0.0f));
				}
			}
		}

		// 4.4 Average world point from several images (for accuracy)
		for (int ii = tnk.size() - 1; ii >= 0; ii--)
		{
			int are = tnk[ii];
			std::vector<std::vector<cv::Point2f>>::iterator unkite = imagePoints.begin() + are;
			imagePoints.erase(unkite);
		}
		for (int ii = tnk.size() - 1; ii >= 0; ii--)
		{
			int are = tnk[ii];
			std::vector<std::vector<cv::Point3f>>::iterator unkite = worldPoints.begin() + are;
			worldPoints.erase(unkite);
		}
		imageNumber -= tnk.size();

		// 5. Calibration process
		std::cout << "CAMERA CALIBRATION" << std::endl;
		cv::Mat CameraIntrinsicMatrix;
		cv::Mat DistCoeffs;
		std::vector<cv::Mat> RotationVectors;
		std::vector<cv::Mat> TranslationVectors;
		for (int i = 0; i < imageNumber; i++)
		{
			std::cout << i << ": World points (Object points) : " << worldPoints[i].size() << " Image points : " << imagePoints[i].size() << std::endl;
		}
		std::cout << "Number of image : " << checkerImgs[0].size() << std::endl;

		// 5.1 Set output file
		std::stringstream  stream;
		stream << CAM_CALIB_PATH << CAM_ID << "\\CAM_PARAM_MAT.xml";
		std::string xmlName = stream.str();
		cv::FileStorage rfs(xmlName, cv::FileStorage::READ);
		bool fileExists = rfs.isOpened();

		// 5.2 Calibrate
		double a = cv::calibrateCamera(worldPoints,
									   imagePoints,
									   checkerImgs[0].size(),
									   CameraIntrinsicMatrix,
									   DistCoeffs,
									   RotationVectors,
									   TranslationVectors);
		std::cout << "Average re-projection error (0=good): " << a << std::endl;
		cv::waitKey(DELAYED_MSEC);

		// 5.3 Write to a file
		// Computes useful camera characteristics from the camera matrix
		std::cout << "CALCULATE CAMERA PARAMETER" << std::endl;
		double apertureWidth = 0, apertureHeight = 0;	// Physical width in mm of the sensor
		double fovx = 0, fovy = 0;						// Field of view in degrees
		double focalLength = 0;							// Focal length of the lens in mm
		cv::Point2d principalPoint = (0, 0);			// Principal point in mm
		double aspectRatio = 0;							// Fy/fx

		cv::calibrationMatrixValues(CameraIntrinsicMatrix,
			checkerImgs[0].size(),
			apertureWidth,
			apertureHeight,
			fovx,
			fovy,
			focalLength,
			principalPoint,
			aspectRatio);
		std::cout << "Write a output file: " << xmlName << std::endl;
		cv::FileStorage wfs(xmlName, cv::FileStorage::WRITE);
		// Calibration matrix
		wfs << XMLTAG_CAMERAMAT << CameraIntrinsicMatrix;
		wfs << XMLTAG_DISTCOEFFS << DistCoeffs;

		// Camera parameter
		wfs << "aperture_Width" << apertureWidth;
		wfs << "aperture_Height" << apertureHeight;
		wfs << "fov_x" << fovx;
		wfs << "fov_y" << fovy;
		wfs << "focal_Length" << focalLength;
		wfs << "principal_Point" << principalPoint;
		wfs << "aspect_Ratio" << aspectRatio;
		wfs << "avg_re-projection" << a;
		wfs.release();
		std::cout << "Finish XML and calibration process" << std::endl;
	}
	// [selection] ProjectorCalibration (and CamProCalibration)
	else if (CalibrationMode == PRO_CALIBRATE_MODE)
	{
		// 4.1 Setup parameters
		std::vector<cv::Mat> checkerImgs1;
		std::vector<cv::Mat> checkerImgs2;
		std::vector<cv::Mat> projectImgs;

		const cv::Size patternSize(checkPointX, checkPointY);
		std::vector<std::vector<cv::Point2f>> imagePoints1(imageNumber);
		std::vector<std::vector<cv::Point3f>> worldPoints1(imageNumber);
		std::vector<std::vector<cv::Point2f>> imagePoints2(imageNumber);
		std::vector<std::vector<cv::Point3f>> worldPoints2(imageNumber);
		std::vector<std::vector<cv::Point2f>> projectPoints(imageNumber);

		// 4.2 Get camera distortion matrix
		cv::Mat CameraIntrinsicMatrix, CameraDistortionCoeffs;
		std::stringstream stream;
		stream << CAM_CALIB_PATH << CAM_ID << "\\CAM_PARAM_MAT.xml";
		std::string  fileName = stream.str();
		std::cout << "Get camera parameter: " << fileName << std::endl;
		cv::FileStorage cvfs(fileName, cv::FileStorage::READ);
		cv::FileNode param1(cvfs.fs, NULL);
		cv::read(cvfs["CameraMatrix"], CameraIntrinsicMatrix);
		cv::read(cvfs["DistortCoeffs"], CameraDistortionCoeffs);

		// 4.3 Read captured images
		cv::Mat newmatrix;
		for (int i = 0; i < imageNumber; i++)
		{
			std::stringstream stream;
			stream << PRO_CALIB_PATH << PRO_ID << "\\source1_" << i << IMAGE_EXT;
			std::string fileName = stream.str();
			cv::Mat image = cv::imread(fileName);
			cv::Mat dst;
			// Undistortion image (from camera parameter)
			cv::undistort(image, dst, CameraIntrinsicMatrix, CameraDistortionCoeffs);
			checkerImgs1.push_back(dst);

			std::cout << "Load Captured image (w projection): " << fileName << std::endl;
		}
		for (int i = 0; i < imageNumber; i++)
		{
			std::stringstream stream;
			stream << PRO_CALIB_PATH << PRO_ID << "\\source2_" << i << IMAGE_EXT;
			std::string  fileName = stream.str();
			cv::Mat image = cv::imread(fileName);
			cv::Mat dst;
			// Undistortion image (from camera parameter)
			cv::undistort(image, dst, CameraIntrinsicMatrix, CameraDistortionCoeffs);
			checkerImgs2.push_back(dst);

			std::cout << "Load Captured image (w/o projection): " << fileName << std::endl;
		}
		for (int i = 0; i < imageNumber; i++)
		{
			std::stringstream stream;
			stream << PRO_CALIB_PATH << PRO_ID << "\\project_" << i << IMAGE_EXT;
			std::string  fileName = stream.str();
			cv::Mat image = cv::imread(fileName);
			// Require flip since the image is use for OpenGL (top-down flip)
			cv::flip(image, image, 0);
			projectImgs.push_back(image);
			std::cout << "Load Projction image: " << fileName << std::endl;
		}

		// 5. Detect corner
		std::cout << "DETECTION CORNERS" << std::endl;

		// Source 1 (ImagePoint1)
		int flag = 0;
		int rowrow = checkerImgs1[0].rows, colcol = checkerImgs1[0].cols;
		cv::namedWindow(WINDOWNAME_SRC, CV_WINDOW_AUTOSIZE);

		for (int i = 0; i < imageNumber; i++)
		{
			if (checkPattern == CHESS)
			{
				imagePoints1[i].clear();
				for (int ii = 0; ii < rowrow*colcol; ii++)
				{
					int are = ii * 3;
					checkerImgs1[i].data[are + 1] = 0;
					checkerImgs1[i].data[are + 2] = 0;
				}
				if (cv::findChessboardCorners(checkerImgs1[i], patternSize, imagePoints1[i], CV_CALIB_CB_ADAPTIVE_THRESH))
				{
					//std::cout << "******************" << std::endl;
					//std::cout << "ImageID: " << i << " [SUCCESS]" << std::endl;

					// Subpixel
					cv::Mat gray;
					cv::cvtColor(checkerImgs1[i], gray, CV_BGR2GRAY);
					cv::cornerSubPix(gray,
						imagePoints1[i],
						cv::Size(5, 5),
						cv::Size(-1, -1),
						cv::TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 10000, 0.001));

					// Draw chessboard
					cv::drawChessboardCorners(checkerImgs1[i], patternSize, (cv::Mat)(imagePoints1[i]), true);
					cv::imshow(WINDOWNAME_SRC, checkerImgs1[i]);
					cv::waitKey(30);

					// Save to image
					std::stringstream stream;
					stream << PRO_CALIB_PATH << PRO_ID << "\\Zsource1_" << i << IMAGE_EXT;
					std::string fileName = stream.str();
					cv::imwrite(fileName, checkerImgs1[i]);

					flag++;
				}
				else
				{
					std::cout << "ImageID: " << i << " [FAIL]" << std::endl;
				}
			}
		}
		if (flag == 0) Quit();

		// Source 2 (ImagePoint 2)
		flag = 0;
		rowrow = checkerImgs2[0].rows, colcol = checkerImgs2[0].cols;
		cv::namedWindow(WINDOWNAME_SRC, CV_WINDOW_AUTOSIZE);

		for (int i = 0; i < imageNumber; i++)
		{
			if (checkPattern == CHESS)
			{
				imagePoints2[i].clear();
				for (int ii = 0; ii < rowrow*colcol; ii++)
				{
					int are = ii * 3;
					checkerImgs2[i].data[are + 1] = 0;
					checkerImgs2[i].data[are + 0] = 0;
				}
				if (cv::findChessboardCorners(checkerImgs2[i], patternSize, imagePoints2[i], CV_CALIB_CB_ADAPTIVE_THRESH))
				{
					//std::cout << "******************" << std::endl;
					//std::cout << "ImageID: " << i << " [SUCCESS]" << std::endl;

					// Subpixel
					cv::Mat gray;
					cv::cvtColor(checkerImgs2[i], gray, CV_BGR2GRAY);
					cv::cornerSubPix(gray,
						imagePoints2[i],
						cv::Size(5, 5),
						cv::Size(-1, -1),
						cv::TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 10000, 0.001));

					// Draw chessboard
					cv::drawChessboardCorners(checkerImgs2[i], patternSize, (cv::Mat)(imagePoints2[i]), true);
					cv::imshow(WINDOWNAME_SRC, checkerImgs2[i]);
					cv::waitKey(30);

					// Save image
					std::stringstream stream;
					stream << PRO_CALIB_PATH << PRO_ID << "\\Zsource2_" << i << IMAGE_EXT;
					std::string fileName = stream.str();
					cv::imwrite(fileName, checkerImgs2[i]);

					flag++;
				}
				else
				{
					std::cout << "ImageID: " << i << " [FAIL]" << std::endl;
				}
			}
		}
		if (flag == 0) Quit();

		// Projection reference (ProjectPoint)
		flag = 0;
		rowrow = projectImgs[0].rows, colcol = projectImgs[0].cols;
		cv::namedWindow(WINDOWNAME_SRC, CV_WINDOW_AUTOSIZE);

		for (int i = 0; i < imageNumber; i++)
		{
			if (checkPattern == CHESS)
			{
				projectPoints[i].clear();
				if (cv::findChessboardCorners(projectImgs[i], patternSize, projectPoints[i], CV_CALIB_CB_ADAPTIVE_THRESH))
				{
					//std::cout << "******************" << std::endl;
					//std::cout << "ImageID: " << i << " [SUCCESS]" << std::endl;

					// Subpixel
					cv::Mat gray;
					cv::cvtColor(projectImgs[i], gray, CV_BGR2GRAY);
					cv::cornerSubPix(gray,
						projectPoints[i],
						cv::Size(2, 2),
						cv::Size(-1, -1),
						cv::TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 10000, 0.001));


					// Draw Chessboard
					cv::drawChessboardCorners(projectImgs[i], patternSize, (cv::Mat)(projectPoints[i]), true);
					cv::imshow(WINDOWNAME_SRC, projectImgs[i]);
					cv::waitKey(30);

					// Save image
					std::stringstream stream;
					stream << PRO_CALIB_PATH << PRO_ID << "\\Zproject_" << i << IMAGE_EXT;
					std::string fileName = stream.str();
					cv::imwrite(fileName, projectImgs[i]);

					flag++;
				}
				else
				{
					std::cout << "ImageID: " << i << " [FAIL]" << std::endl;
				}
			}
		}
		if (flag == 0) Quit();

		// 6.  Extimate world point from checker pattern (opject point)
		for (int i = 0; i < imageNumber; i++)
		{
			worldPoints1[i].clear();
			for (int k = 0; k < patternSize.height; k++)
			{
				for (int j = 0; j < patternSize.width; j++)
				{
					worldPoints1[i].push_back(cv::Point3f((float)((float)k * checkSize), (float)((float)j * checkSize), 0.0f));
				}
			}
		}

		// 7. Confirm the camera parameter by re-calculate the camera matrix again 
		//    from the image of chessboard w/o projection (source 1)
		//    ** Original load from XML and this calculation should be similar or same (in theory)
		cv::Mat ReCamIntrinsicMatrix;
		cv::Mat ReCamDistortionCoeffs;
		std::vector<cv::Mat> RotationVectors;
		std::vector<cv::Mat> TranslationVectors;

		double a = cv::calibrateCamera(worldPoints1,
			imagePoints1,
			checkerImgs1[0].size(),
			ReCamIntrinsicMatrix,
			ReCamDistortionCoeffs,
			RotationVectors,
			TranslationVectors);
		std::cout << "Average re-projection error (0=good): " << a << std::endl;
		// In theory this two matrix (re-calculate and original) should closed to each other or same
		std::cout << "Re-calculate Camera Matrix from yellow chessboard w/o projection" << std::endl;
		std::cout << "[Re-calculation: Intrinsic]" << std::endl;
		std::cout << ReCamIntrinsicMatrix << std::endl;
		std::cout << "[Original (in XML): Intrinsic]" << std::endl;
		std::cout << CameraIntrinsicMatrix << std::endl;
		std::cout << "CONFIRM?" << std::endl;
		cv::waitKey(50);

		std::cout << "[Re-calculation: Distortion]" << std::endl;
		std::cout << ReCamDistortionCoeffs << std::endl;
		std::cout << "[Original (in XML): Distortion]" << std::endl;
		std::cout << CameraDistortionCoeffs << std::endl;
		std::cout << "CONFIRM?" << std::endl;
		cv::waitKey(50);

		// 8. Calculate extrinsic parameter of yellow board point without projected chessboard
		//    Calculate camera Extrinsic parameter (of projection viewpoint)
		// 8.1 Reset distortion coefficient (assume no distortion)
		CameraDistortionCoeffs = cv::Scalar(0);
		for (int i = 0; i < imageNumber; i++)
		{
			cv::Mat rvec, tvec, R;
			// 8.2 Calculate the tranformation of 3D-2D correspondence (for each image)
			cv::solvePnP(worldPoints1[i],
				imagePoints1[i],
				CameraIntrinsicMatrix,
				CameraDistortionCoeffs,
				rvec,
				tvec);
			// 8.3 Transform rotation vector to rotation matrix
			cv::Rodrigues(rvec, R);

			cv::Mat RT = cv::Mat::zeros(3, 3, CV_64F);
			RT.at<double>(0, 0) = R.at<double>(0, 0);
			RT.at<double>(0, 1) = R.at<double>(0, 1);
			RT.at<double>(0, 2) = tvec.at<double>(0, 0);
			RT.at<double>(1, 0) = R.at<double>(1, 0);
			RT.at<double>(1, 1) = R.at<double>(1, 1);
			RT.at<double>(1, 2) = tvec.at<double>(1, 0);
			RT.at<double>(2, 0) = R.at<double>(2, 0);
			RT.at<double>(2, 1) = R.at<double>(2, 1);
			RT.at<double>(2, 2) = tvec.at<double>(2, 0);

			// 8.4 Estimate projector relationship from projected chessboard pattern
			//     ** Get average world point (object point) of projector
			for (int ii = 0; ii < imagePoints2[i].size(); ii++)
			{
				cv::Mat a = cv::Mat::zeros(3, 1, CV_64F);
				a.at<double>(0, 0) = (double)imagePoints2[i][ii].x;
				a.at<double>(1, 0) = (double)imagePoints2[i][ii].y;
				a.at<double>(2, 0) = 1.0;

				cv::Mat b = RT.inv() * CameraIntrinsicMatrix.inv() * a;
				b.at<double>(0, 0) /= b.at<double>(2, 0);
				b.at<double>(1, 0) /= b.at<double>(2, 0);

				worldPoints2[i].push_back(cv::Point3f((float)b.at<double>(0, 0), (float)b.at<double>(1, 0), 0.0));
			}
		}

		// After preparing the world point (object point) of projector (from camera capture) -- coz projector no eyes
		// At this point we know the world point (object point) from projector (similar to the one we know from camera
		// in the cameraCalibration process
		// The next step is to calibrate between world point (object point that projector project chessboard on)
		// and the projection image (projection reference)
		// 9. Calibration 
		cv::Mat projectorIntrinsicMatrix;
		cv::Mat projectorDistCoeffs;
		std::vector<cv::Mat> ProRotationVectors;
		std::vector<cv::Mat> ProTranslationVectors;

		std::cout << "START CALIBRATION" << std::endl;

		// 9.1 Preparing xml file output
		stream << PRO_CALIB_PATH << PRO_ID << "\\PRO_PARAM_MAT.xml";
		std::string  xmlName = stream.str();
		cv::FileStorage rfs(xmlName, cv::FileStorage::READ);
		bool FileExists = rfs.isOpened();

		// 9.2 Calibrate Intrinsic parameter of projector (from projection reference as ProjctorView and projected image as 3D point)
		a = cv::calibrateCamera(worldPoints2,
			projectPoints,
			projectImgs[0].size(),
			projectorIntrinsicMatrix,
			projectorDistCoeffs,
			ProRotationVectors,
			ProTranslationVectors);
		std::cout << "Average re-projection error (0=good): " << a << std::endl;

		// 9.3 Calculate Projector Parameter
		// Computes useful projector characteristics from the camera matrix
		std::cout << "CALCULATE PROJECTOR PARAMETER" << std::endl;
		double apertureWidth = 0, apertureHeight = 0;	// Physical width in mm of the sensor
		double fovx = 0, fovy = 0;						// Field of view in degrees
		double focalLength = 0;							// Focal length of the lens in mm
		cv::Point2d principalPoint = (0, 0);			// Principal point in mm
		double aspectRatio = 0;							// Fy/fx

		cv::calibrationMatrixValues(projectorIntrinsicMatrix,
			projectImgs[0].size(),
			apertureWidth,
			apertureHeight,
			fovx,
			fovy,
			focalLength,
			principalPoint,
			aspectRatio);
		std::cout << "Write a output file: " << xmlName << std::endl;
		cv::FileStorage wfs(xmlName, cv::FileStorage::WRITE);
		wfs << XMLTAG_CAMERAMAT << projectorIntrinsicMatrix;
		wfs << XMLTAG_DISTCOEFFS << projectorDistCoeffs;
		wfs << "aperture_Width" << apertureWidth;
		wfs << "aperture_Height" << apertureHeight;
		wfs << "fov_x" << fovx;
		wfs << "fov_y" << fovy;
		wfs << "focal_Length" << focalLength;
		wfs << "principal_Point" << principalPoint;
		wfs << "aspect_Ratio" << aspectRatio;
		wfs.release();
		std::cout << "Finish XML" << std::endl;

		// 9.4 Calculate Extrinsic parameter between Projector-Camera
		//     Assume Camera is a First Camera (that observed 3D point of Projector) and Projector is a Second Camera
		//     (as the projection reference depend on the size of projection)
		cv::Mat R, T, E, F;
		cv::TermCriteria criteria{ 10000, 10000, 0.0001 };
		double tms = cv::stereoCalibrate(worldPoints2,
			imagePoints2,
			projectPoints,
			CameraIntrinsicMatrix,
			CameraDistortionCoeffs,
			projectorIntrinsicMatrix,
			projectorDistCoeffs,
			checkerImgs1[0].size(),
			R,
			T,
			E,
			F,
			CV_CALIB_FIX_INTRINSIC/*CV_CALIB_USE_INTRINSIC_GUESS*//*CV_CALIB_FIX_ASPECT_RATIO*/,
			criteria);
		std::cout << "Average re-projection error (0=good): " << tms << std::endl;
		std::cout << "Founded [Rotation matrix] between Pro/Cam" << std::endl;
		std::cout << R << std::endl;
		std::cout << "Founded [Translation matrix] between Pro/Cam" << std::endl;
		std::cout << T << std::endl;

		// 9.5 Write to file
		FILE *fp;
		// Convert directory to char type
		const char *dirname = EXT_CAM_PRO_PATH.c_str();
		fopen_s(&fp, dirname, "w");
		for (int jj = 0; jj < 3; jj++) {
			fprintf_s(fp, "%f,%f,%f,%f\n", R.at<double>(jj, 0), R.at<double>(jj, 1), R.at<double>(jj, 2), T.at<double>(jj, 0));
		}
		fclose(fp);

		std::cout << "Write file success and finished calibration" << std::endl;
	}
	// [selection] P2C/C2P
	else if (CalibrationMode == P2C_CALIBRATE_MODE)
	{
		// Setup OpenGL viewport
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0, 0, 0, 0);

		glViewport(0, 0, WIN_W, WIN_H);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, WIN_W, 0, WIN_H);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// Initial structure pattern
		StructurePattern *structurePattern = new StructurePattern(CAM_W, CAM_H, WIN_W, WIN_H, 10, C2P_CALIB_PATH, true);
		// Set initial line (vertical or horizontal)
		structurePattern->initDispCode(structurePattern->VERT);

		bool startup = true;
		while (true)
		{
			glLoadIdentity();
			glTranslatef(0.0f, 0.0f, 0.0f);
			structurePattern->dispCode();
			glutSwapBuffers();
			if (startup)
			{
				Sleep(DELAYED_MSEC);
				startup = false;
			}
			else
				Sleep(500);

			#ifndef WEBCAM_MODE
			cv::Mat frame = pointGreyCamera->Capture();
			#else
			cv::Mat frame = webCamera->Capture();
			#endif
			IplImage* iplFrame = new IplImage(frame);
			structurePattern->captureCode(iplFrame);

			if (structurePattern->dispMode == structurePattern->DISP_IDLE) break;
		}

		// Save text-based lookup table (from camera to projector)
		structurePattern->saveLookup(C2P_TOTEXT_PATH);
		// Release structure pattern
		delete structurePattern;
	}

	// [option] Find out the specific set of correspondence points
	if (CalibrationMode == P2C_POINT_MODE)
	{
		// 1. Read C2P mapping file
		cv::Mat cameraCoordinate = cv::Mat(480, 640, CV_32FC2);
		cv::Mat projectorCoordinate_x = cv::Mat(480, 640, CV_32FC1);
		cv::Mat projectorCoordinate_y = cv::Mat(480, 640, CV_32FC1);

		std::ifstream input(C2P_TOTEXT_PATH);
		int cx, cy, px, py;
		if (input.is_open())
		{
			int nline = 0;
			while (true)
			{
				input >> cx;
				input.ignore();
				input >> cy;
				input.ignore();
				input >> px;
				input.ignore();
				input >> py;

				cameraCoordinate.at<cv::Point2f>(cy, cx) = cv::Point2f(cx, cy);
				projectorCoordinate_x.at<float>(cy, cx) = (float)px;
				projectorCoordinate_y.at<float>(cy, cx) = (float)py;

				nline++;
				if (input.eof()) break;
			}
			std::cout << "End reading line: " << nline << std::endl;
		}

		// 2. Set mouse callback
		cv::Mat temp;
		cv::namedWindow(WINDOWNAME_CAP, CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);
		cv::setMouseCallback(WINDOWNAME_CAP, MouseCallback);

		// 3. Get camera distortion matrix
		cv::Mat CameraIntrinsic, CameraDistortion;
		std::stringstream CamStream;
		CamStream << CAM_CALIB_PATH << CAM_ID << "\\CAM_PARAM_MAT.xml";
		std::string  fileName = CamStream.str();
		std::cout << "Get camera intrinsic parameter: " << fileName << std::endl;
		cv::FileStorage cvfsp(fileName, cv::FileStorage::READ);
		cv::FileNode param1(cvfsp.fs, NULL);
		cv::read(cvfsp["CameraMatrix"], CameraIntrinsic);
		cv::read(cvfsp["DistortCoeffs"], CameraDistortion);

		// 4. Capture image (with undistortion and select the object point)
		cv::Mat undistortFrame;
		std::cout << "Please select the image (camera) <> object correspondence point(s)!" << std::endl;
		while (true)
		{
			#ifndef WEBCAM_MODE
			cv::Mat frame = pointGreyCamera->Capture();
			#else
			cv::Mat frame = webCamera->Capture();
			#endif

			// Undistortion image (from camera parameter)
			cv::undistort(frame, undistortFrame, CameraIntrinsic, CameraDistortion);
			cv::imshow(WINDOWNAME_CAP, undistortFrame);
			int key = cv::waitKey(30);
			if (key == 'q')
			{
				std::cout << "Captured End" << std::endl;
				break;
			}
		}

		// 5. Get the point from C2P mapping and input to array
		float p_x, p_y;
		std::vector<cv::Point2f> ProjectorPoints;

		for (int a = 0; a < CameraPoints.size(); a++)
		{
			p_x = projectorCoordinate_x.at<float>(CameraPoints[a].y, CameraPoints[a].x);
			p_y = projectorCoordinate_y.at<float>(CameraPoints[a].y, CameraPoints[a].x);
			ProjectorPoints.push_back(cv::Point2f(p_x, p_y));
		}

		// 6.1 Test print projector point on the projection screen
		cv::Mat projection = cv::Mat::zeros(cv::Size(WIN_W, WIN_H), CV_8UC1);
		for (int b = 0; b < ProjectorPoints.size(); b++)
		{
			cv::circle(projection, ProjectorPoints[b], 3, cv::Scalar(128, 128, 0), 1, 8, 0);
		}

		// 6.2 Projection on the screen
		cv::namedWindow(WINDOWNAME_PRJ, CV_WINDOW_AUTOSIZE | CV_GUI_NORMAL);
		cv::moveWindow(WINDOWNAME_PRJ, INT_WIN_POS_X, INT_WIN_POS_Y);
		//cv::setWindowProperty(WINDOWNAME_PRJ, CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
		while (true)
		{
			cv::imshow(WINDOWNAME_PRJ, projection);
			int key = cv::waitKey(30);
			if (key == 'q') break;
		}

		// 7. Record the position to file
		std::stringstream PointRecorded;
		PointRecorded << C2P_SELECTED_PT;
		fileName = PointRecorded.str();
		std::cout << "Recorded correspondence of C2P: " << fileName << std::endl;
		cv::FileStorage cvfs(fileName, cv::FileStorage::WRITE);

		// 7. Record the position to file
		FILE *pr;
		const char *dirname = C2P_SELECTED_PT.c_str();
		fopen_s(&pr, dirname, "w");
		for (int h = 0; h < CameraPoints.size(); h++)
		{
			fprintf_s(pr, "%f\t%ft%f\t%f\n", CameraPoints[h].x, CameraPoints[h].y, ProjectorPoints[h].x, ProjectorPoints[h].y);
		}
		fclose(pr);

		// 7. Get projector distortion matrix
		/*
		cv::Mat ProjectorIntrinsic, ProjectorDistortion;
		std::stringstream ProStream;
		ProStream << PRO_CALIB_PATH << CAM_ID << "\\CAM_PARAM_MAT.xml";
		fileName = ProStream.str();
		std::cout << "Get projector intrinsic parameter: " << fileName << std::endl;
		cv::FileStorage cvfs(fileName, cv::FileStorage::READ);
		cv::FileNode parameters(cvfs.fs, NULL);
		cv::read(cvfs["CameraMatrix"], ProjectorIntrinsic);
		cv::read(cvfs["DistortCoeffs"], ProjectorDistortion);
		*/
	}

	// [option] Calibration using 3D point and 2D correspondence position
	if (CalibrationMode == CAM_PNP_CALIBRATE_MODE)
	{
		// 1. Read OBJ position file
		std::vector<cv::Point3f> Obj_3D_Points;
		std::ifstream input(PNP_OBJ_PT_PATH);

		float ox, oy, oz;
		if (input.is_open())
		{
			int nline = 0;
			while (true)
			{
				input >> ox;
				input.ignore();
				input >> oy;
				input.ignore();
				input >> oz;

				Obj_3D_Points.push_back(cv::Point3f(ox, oy, oz));

				nline++;
				if (input.eof()) break;
			}
			std::cout << "End reading line: " << nline << std::endl;
		}

		// 2. Set mouse callback
		cv::Mat temp;
		cv::namedWindow(WINDOWNAME_CAP, CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);
		cv::setMouseCallback(WINDOWNAME_CAP, MouseCallback);

		// 3. Get camera distortion matrix
		cv::Mat CameraIntrinsic, CameraDistortion;
		std::stringstream CamStream;
		CamStream << CAM_CALIB_PATH << CAM_ID << "\\CAM_PARAM_MAT.xml";
		std::string  fileName = CamStream.str();
		std::cout << "Get camera intrinsic parameter: " << fileName << std::endl;
		cv::FileStorage cvfsp(fileName, cv::FileStorage::READ);
		cv::FileNode param1(cvfsp.fs, NULL);
		cv::read(cvfsp["CameraMatrix"], CameraIntrinsic);
		cv::read(cvfsp["DistortCoeffs"], CameraDistortion);

		// 3. Capture image (with undistortion and select the object point)
		cv::Mat undistortFrame;
		std::cout << "Please select the image (camera) <> object correspondence point(s)!" << std::endl;
		for (int i = 0; i < Obj_3D_Points.size(); i++)
		{
			std::cout << i << "| " << "Object point: " << Obj_3D_Points[i] << std::endl;
		}
		CameraPoints.clear();
		while (true)
		{
			#ifndef WEBCAM_MODE
			cv::Mat frame = pointGreyCamera->Capture();
			#else
			cv::Mat frame = webCamera->Capture();
			#endif

			// Undistortion image (from camera parameter)
			cv::undistort(frame, undistortFrame, CameraIntrinsic, CameraDistortion);
			cv::imshow(WINDOWNAME_CAP, undistortFrame);
			int key = cv::waitKey(30);
			if (key == 'q')
			{
				std::cout << "Captured End" << std::endl;
				break;
			}
		}
		cv::Mat Obj_3D = cv::Mat(Obj_3D_Points, true);
		cv::Mat Cam_2D = cv::Mat(CameraPoints, true);		

		// 4. Calculate SolvePnP
		cv::Mat translation_vec, rotation_vec;
		bool status = cv::solvePnP(Obj_3D, 
								   Cam_2D, 
								   CameraIntrinsic, 
								   CameraDistortion, 
								   rotation_vec, 
								   translation_vec, 
								   false, 
								   CV_EPNP);

		if (status)
		{
			// 5. Packed the rotation and translation vectors
			cv::Mat R;
			cv::Rodrigues(rotation_vec, R); // R is 3 x 3
			// Inverse the rotation marix so that it will in object coordinate
			// Ref. http://stackoverflow.com/questions/18637494/camera-position-in-world-coordinate-from-cvsolvepnp
			R = R.t();
			// Translation of inverse
			translation_vec = -R * translation_vec;

			// 6. Transformation matrix
			cv::Mat T(4, 4, R.type()); // T is 4 x 4
			T(cv::Range(0, 3), cv::Range(0, 3)) = R * 1; // Copies R into T
			T(cv::Range(0, 3), cv::Range(3, 4)) = translation_vec * 1; // Copies translation vector into T

			// Fill the last row of T
			// Note: T is a 4x4 matrix with the pose of the camera in the object frame
			float *p = T.ptr<float>(3);
			p[0] = p[1] = p[2] = 0; p[3] = 1;

			// 7. Write output file
			std::stringstream  stream;
			stream << PNP_CALIB_PATH << "\\CAM_OBJ_Transformation.xml";
			std::string xmlName = stream.str();
			std::cout << "Write a output file: " << xmlName << std::endl;
			cv::FileStorage wfs(xmlName, cv::FileStorage::WRITE);
			// Calibration matrix
			wfs << XMLTAG_CAMERAMAT << T;

			wfs.release();
			std::cout << "Finish XML and calibration process" << std::endl;
		}
		else
			std::cout << "PNP Error !" << std::endl;
	}
	cv::waitKey(0);
	Quit();
}

int main(int argc, char **argv)
{
	#ifndef WEBCAM_MODE
	pointGreyCamera = new PointGreyCamera();
	#else
	webCamera = new WebCamera();
	webCamera->Initialize();
	#endif
	
	CameraPoints.clear();

	// 1. Set calibration mode
	std::cout << "Select Calibration mode: " << std::endl;
	std::cout << "1: Camera Calibration" << std::endl;
	std::cout << "2: Projector Calibration/Camera-To-Projector Calibration" << std::endl;
	std::cout << "3: Pro/Cam Pixel correspondence (Greycode)" << std::endl;
	std::cout << "4: FIND C-to-P correspondence point" << std::endl;
	std::cout << "5: 3D(object)-2D(screen) PnP Calibration [For Camera]" << std::endl;
	std::cout << "0: Exit" << std::endl;
	std::cout << "-------------------------" << std::endl;
	std::cin >> CalibrationMode;

	// 2. Initialize OpenGL
	if (CalibrationMode == P2C_POINT_MODE)
	{
		Display();
	}
	else if (CalibrationMode == PROGRAM_EXIT)
	{
		Quit();
	}
	else
	{
		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
		glutInitWindowPosition(INT_WIN_POS_X, INT_WIN_POS_Y);
		glutInitWindowSize(WIN_W, WIN_H);
		glutCreateWindow(argv[0]);
		std::cout << "CAM Position: " << CAM_W << "x" << CAM_H << std::endl;
		std::cout << "PRO Position: " << WIN_W << "x" << WIN_H << std::endl;
		printf("Screen Position: %d %d %d %d\n", CAM_W, CAM_H, WIN_W, WIN_H);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glutDisplayFunc(Display);

		// main loop
		glutMainLoop();
	}

	return 0;
}