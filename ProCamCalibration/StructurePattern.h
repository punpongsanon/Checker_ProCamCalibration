#pragma once
#include "Configuration.h"

class StructurePattern
{
private:

	int			g_bit;				// current bit depth
	int			CODE_DEPTH;
	int			CODE_RESOLUTION;
	IplImage	*diff;
	int			_cameraW;
	int			_cameraH;
	int			_projectorW;
	int			_projectorH;
	bool		_debug;
	char		_dirname[256];
	int			_thresh;

public:

	int			*C2P[2];
	IplImage	*_white;
	IplImage	*_black;
	IplImage	*_area;

	// program mode definition 
	// DISP_IDLE    -- no task 
	// DISP_LIVE_INPUT_IMAGE_ON_SUBWINDOW  -- display live input image on subwindow
	// DISP_ILLUMI		  -- display white image
	// DISP_GRAYCODE      -- display gray code 
	enum { DISP_IDLE, DISP_GRAYCODE, DISP_ILLUMI } dispMode;

	// NEGA-POSI Pattern Projectino
	enum { NEGATIVE, POSITIVE } NPMode;

	// horizontal/vertical pattern
	enum { VERT, HORI } HVMode;

	// Binaryze Mode
	// AVG_MODE  -- capture usual intensity image.
	//			each color channels are averaged 
	// DIFF_MODE -- for binarized images.
	//			captured image is compared with the other intensity image.
	//			it is used for NEGA/POSI binarizing method 
	enum { AVG_MODE, DIFF_MODE };

	StructurePattern(int cw, int ch, int pw, int ph, int thresh, std::string dir, bool debug = false)
	{
		// Convert directory to char type
		const char *dirname = dir.c_str();

		// CODE_DEPTH should be greater than projectior resolution
		int code_depth_w, code_depth_h;
		code_depth_w = log((double)pw) / log(2.0) + 1;
		code_depth_h = log((double)ph) / log(2.0) + 1;
		if (code_depth_w > code_depth_h)
			CODE_DEPTH = code_depth_w;
		else
			CODE_DEPTH = code_depth_h;

		CODE_RESOLUTION = 1;

		dispMode = DISP_IDLE;

		_cameraW = cw;
		_cameraH = ch;
		_projectorW = pw;
		_projectorH = ph;
		_debug = debug;
		_thresh = thresh;

		sprintf_s(_dirname, "%s", dirname);

		diff = cvCreateImage(cvSize(_cameraW, _cameraH), IPL_DEPTH_8U, 1);
		_white = cvCreateImage(cvSize(_cameraW, _cameraH), IPL_DEPTH_8U, 3);
		_black = cvCreateImage(cvSize(_cameraW, _cameraH), IPL_DEPTH_8U, 3);
		_area = cvCreateImage(cvSize(_cameraW, _cameraH), IPL_DEPTH_8U, 1);
		for (int i = 0; i<2; i++)
		{
			C2P[i] = new int[_cameraW * _cameraH];
		}
	};

	~StructurePattern()
	{
		cvReleaseImage(&diff);
		cvReleaseImage(&_white);
		cvReleaseImage(&_black);
		cvReleaseImage(&_area);
		for (int i = 0; i<2; i++)
		{
			delete[] C2P[i];
		}
	};

	/*
	*	initialization for displaying of graycode
	*/

	void initDispCode(int hv_mode)
	{
		NPMode = POSITIVE;
		dispMode = DISP_ILLUMI;

		if (hv_mode == HORI)	HVMode = HORI;
		else					HVMode = VERT;

		g_bit = CODE_DEPTH - 1;
		for (int i = 0; i<_cameraW*_cameraH; i++)	C2P[HVMode][i] = 0;
	};

	/*
	*	GrayCode -> Binary Code
	*/

	unsigned int codeG2B(unsigned int g)
	{
		int i, b;

		b = (g >> 31);

		for (i = 30; i >= 0; i--)
		{
			b <<= 1;
			if (b & 0x02)	b |= 1 - ((g >> i) & 0x01);
			else 			b |= ((g >> i) & 0x01);
		}

		return b;
	};

	/*
	*	Binary Code -> GrayCode
	*/

	unsigned int codeB2G(unsigned int b)
	{
		int i, g;

		g = (b >> 31) << 31;

		for (i = 30; i >= 0; i--)
		{
			g <<= 1;
			if ((b >> (i + 1)) & 0x01)		g |= 1 - ((b >> i) & 0x01);
			else						g |= (b >> i) & 0x01;
		}

		return g;
	};

	/*
	*	Binarize for captured GrayCode
	*/

	void binarize(IplImage *color, int bin_mode)
	{
		IplImage *gray = cvCreateImage(cvGetSize(color), IPL_DEPTH_8U, 1);
		cvCvtColor(color, gray, CV_BGR2GRAY);

		for (int i = 0; i < _cameraW * _cameraH; i++)
		{
			int val = (unsigned char)gray->imageData[i];

			if (bin_mode == DIFF_MODE)
			{
				val = val - (unsigned char)diff->imageData[i];
				if (val < 0)	diff->imageData[i] = 255;
				else		diff->imageData[i] = 0;
			}
			else
			{
				diff->imageData[i] = val;
			}

			color += 3;
		}

		cvReleaseImage(&gray);
	};

	void computeProcessArea()
	{
		IplImage *gray_white = cvCreateImage(cvGetSize(_white), IPL_DEPTH_8U, 1);
		IplImage *gray_black = cvCreateImage(cvGetSize(_black), IPL_DEPTH_8U, 1);
		cvCvtColor(_white, gray_white, CV_BGR2GRAY);
		cvCvtColor(_black, gray_black, CV_BGR2GRAY);

		for (int i = 0; i < _cameraW * _cameraH; i++)
		{
			int wval = (unsigned char)gray_white->imageData[i];
			int bval = (unsigned char)gray_black->imageData[i];
			_area->imageData[i] = abs(wval - bval);
		}
		cvReleaseImage(&gray_white);
		cvReleaseImage(&gray_black);
		cvThreshold(_area, _area, (double)_thresh, 255, CV_THRESH_BINARY);
	};

	/*
	*	display graycode
	*/

	void dispCode(void)
	{
		int i, b, g, on;

		glColor3d(0.0, 0.0, 0.0);

		glBegin(GL_POLYGON);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(_projectorW, 0.0, 0.0);
		glVertex3f(_projectorW, _projectorH, 0.0);
		glVertex3f(0, _projectorH, 0.0);
		glEnd();

		glColor3d(1.0, 1.0, 1.0);

		switch (dispMode)
		{
		case DISP_GRAYCODE:
			switch (HVMode)
			{
			case HORI:
				for (i = 0; i < _projectorH; i += CODE_RESOLUTION)
				{
					b = i / CODE_RESOLUTION;
					g = codeB2G(b);
					on = (g >> g_bit) & 0x01;
					if (NPMode == NEGATIVE)
					{
						on = 1 - on;
					}
					if (on)
					{
						glBegin(GL_POLYGON);
						glVertex3f(0.0, i, 0.0);
						glVertex3f(_projectorW, i, 0.0);
						glVertex3f(_projectorW, i + CODE_RESOLUTION, 0.0);
						glVertex3f(0, i + CODE_RESOLUTION, 0.0);
						glEnd();
					}
				}
				break;
			case VERT:
				for (i = 0; i < _projectorW; i += CODE_RESOLUTION)
				{
					b = i / CODE_RESOLUTION;
					g = codeB2G(b);
					on = (g >> g_bit) & 0x01;
					if (NPMode == NEGATIVE)
					{
						on = 1 - on;
					}
					if (on)
					{
						glBegin(GL_POLYGON);
						glVertex3f(i, 0.0, 0.0);
						glVertex3f(i, _projectorH, 0.0);
						glVertex3f(i + CODE_RESOLUTION, _projectorH, 0.0);
						glVertex3f(i + CODE_RESOLUTION, 0.0, 0.0);
						glEnd();
					}
				}
				break;
			}
			break;

		case DISP_ILLUMI:
			if (NPMode == POSITIVE)
			{
				glBegin(GL_POLYGON);
				glVertex3f(0.0, 0.0, 0.0);
				glVertex3f(_projectorW, 0.0, 0.0);
				glVertex3f(_projectorW, _projectorH, 0.0);
				glVertex3f(0.0, _projectorH, 0.0);
				glEnd();
			}
			break;

		}
	};

	/*
	*	capture graycode
	*/

	void captureCode(IplImage* image)
	{
		char filename[256];

		switch (dispMode)
		{
		case DISP_GRAYCODE:

			if (_debug)
			{
				if (HVMode == VERT)
				{
					if (NPMode == POSITIVE)
						sprintf_s(filename, "%s\\vert_color%d_posi.png", _dirname, g_bit);
					else	// NPMode == NEGATIVE
						sprintf_s(filename, "%s\\vert_color%d_nega.png", _dirname, g_bit);
				}
				else
				{	// HVMode == HORI
					if (NPMode == POSITIVE)
						sprintf_s(filename, "%s\\hori_color%d_posi.png", _dirname, g_bit);
					else	// NPMode == NEGATIVE
						sprintf_s(filename, "%s\\hori_color%d_nega.png", _dirname, g_bit);
				}
				cvSaveImage(filename, image);
			}

			if (NPMode == POSITIVE)
			{
				binarize(image, AVG_MODE);
				NPMode = NEGATIVE;
			}
			else
			{
				binarize(image, DIFF_MODE);
				if (_debug)
				{
					if (HVMode == VERT)
						sprintf_s(filename, "%s\\vert_bin%d.png", _dirname, g_bit);
					else
						sprintf_s(filename, "%s\\hori_bin%d.png", _dirname, g_bit);
					cvSaveImage(filename, diff);
				}

				for (int i = 0; i<_cameraW*_cameraH; i++)
				{
					if ((unsigned char)diff->imageData[i])	C2P[HVMode][i] |= (1 << g_bit);
				}

				NPMode = POSITIVE;
				g_bit--;

				if (g_bit < 0)
				{
					for (int i = 0; i<_cameraW*_cameraH; i++)	C2P[HVMode][i] = codeG2B(C2P[HVMode][i]);
					if (HVMode == VERT)
					{
						initDispCode(HORI);
					}
					else
					{
						dispMode = DISP_IDLE;
						dispC2P();
						saveC2P();
						saveP2C();
					}
				}
			}//else : if( NPMode == POSITIVE)
			break;	// DISP_GRAYCODE

		case DISP_ILLUMI:
			if (NPMode == POSITIVE)
			{
				cvCopy(image, _white);
				NPMode = NEGATIVE;
			}
			else
			{
				cvCopy(image, _black);
				computeProcessArea();
				NPMode = POSITIVE;
				dispMode = DISP_GRAYCODE;
			}
			break;	// DISP_ILLUMI

		}
	};

	void dispC2P(void)
	{
		IplImage *C2PImage = cvCreateImage(cvSize(_cameraW, _cameraH), IPL_DEPTH_8U, 3);

		for (int i = 0; i<_cameraW * _cameraH; i++)
		{
			if (_area->imageData[i])
			{
				C2PImage->imageData[i * 3 + 2] = (float)C2P[VERT][i] / (float)pow(2.0, CODE_DEPTH) * 255.0;
				C2PImage->imageData[i * 3 + 1] = (float)C2P[HORI][i] / (float)pow(2.0, CODE_DEPTH) * 255.0;
				C2PImage->imageData[i * 3 + 0] = 0;
			}
		}

		/* ウインドウを準備して画像を表示 */
		cv::namedWindow("result (gray code)", CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);
		cv::moveWindow("result (gray code)", 50, 50);
		cvShowImage("result (gray code)", C2PImage);

		/* キー入力があるまで待つ */
		cvWaitKey(0);
		cvDestroyAllWindows();

		/* クリア */
		cvReleaseImage(&C2PImage);
	};

	void saveC2P(void)
	{
		char filename[256];
		sprintf_s(filename, "%s/C2P.pfm", _dirname);
		std::cout << "save PFM file to: " << filename << std::endl;
		std::ofstream os(filename, std::ofstream::binary);
		os << "PF" << ' ' << std::endl;
		os << _cameraW << " " << _cameraH << std::endl;
		// we assume little endian (until now we don't have any big endian system to add here
		os << "-1.0" << std::endl;
		// PFM raw data
		float val;
		for (int i = 0; i < _cameraW*_cameraH; i++)
		{
			if (_area->imageData[i]) {
				val = (float)C2P[VERT][i];
				os.write((char*)&val, sizeof(float));
				val = (float)C2P[HORI][i];
				os.write((char*)&val, sizeof(float));
				val = (float)0.0f;
				os.write((char*)&val, sizeof(float));
			}
			else {
				val = -1.0f;
				os.write((char*)&val, sizeof(float));
				os.write((char*)&val, sizeof(float));
				val = (float)0.0f;
				os.write((char*)&val, sizeof(float));
			}
		}
		// error checking
		if ((os.rdstate() & std::ifstream::failbit) != 0)
		{
			os.close();
			std::cout << "Error writing to file";
			exit(0);
		}
		os.close();
	};

	void saveP2C(void)
	{
		int *p2c = new int[_projectorW*_projectorH * 2];
		for (int i = 0; i<_projectorW*_projectorH * 2; i++) {
			p2c[i] = -1;
		}
		for (int c = 0; c<_cameraW*_cameraH; c++) {
			if (_area->imageData[c]) {
				int p = C2P[VERT][c] + C2P[HORI][c] * _projectorW;
				p2c[p * 2 + 0] = c%_cameraW;
				p2c[p * 2 + 1] = c / _cameraW;
			}
		}
		int filterSize = 5;
		std::vector<int> datax, datay;
		for (int x = filterSize; x<_projectorW - filterSize; x++)
			for (int y = filterSize; y<_projectorH - filterSize; y++) {
				int p = x + y*_projectorW;
				if (p2c[p * 2 + 0] == -1 && p2c[p * 2 + 1] == -1) {
					for (int fx = x - filterSize; fx<x + filterSize; fx++)
						for (int fy = y - filterSize; fy<y + filterSize; fy++) {
							int fp = fx + fy*_projectorW;
							if (p2c[fp * 2 + 0] != -1 && p2c[fp * 2 + 1] != -1) {
								datax.push_back(p2c[fp * 2 + 0]);
								datay.push_back(p2c[fp * 2 + 1]);
							}
						}
					if (datax.size() && datay.size()) {
						std::sort(datax.begin(), datax.end());
						std::sort(datay.begin(), datay.end());
						p2c[p * 2 + 0] = datax[datax.size() / 2];
						p2c[p * 2 + 1] = datay[datay.size() / 2];
					}
				}
				datax.clear();
				datay.clear();
			}

		char filename[256];
		sprintf_s(filename, "%s/P2C.pfm", _dirname);
		std::cout << "save PFM file to: " << filename << std::endl;
		std::ofstream os2(filename, std::ofstream::binary);
		os2 << "PF" << ' ' << std::endl;
		os2 << _projectorW << " " << _projectorH << std::endl;
		// we assume little endian (until now we don't have any big endian system to add here
		os2 << "-1.0" << std::endl;
		// PFM raw data
		for (int p = 0; p < _projectorW*_projectorH; p++) {
			float val = p2c[p * 2 + 0];
			os2.write((char*)&val, sizeof(float));
			val = p2c[p * 2 + 1];
			os2.write((char*)&val, sizeof(float));
			val = (float)0.0f;
			os2.write((char*)&val, sizeof(float));
		}

		// error checking
		if ((os2.rdstate() & std::ifstream::failbit) != 0)
		{
			os2.close();
			std::cout << "Error writing to file";
			exit(0);
		}
		os2.close();

		delete p2c;

	};

	float *readC2P(void)
	{
		std::string str;
		float *c2p_data;

		char filename[256];
		sprintf_s(filename, "%s/C2P.pfm", _dirname);
		std::cout << "read PFM file from: " << filename << std::endl;
		std::ifstream is(filename, std::ifstream::binary);

		std::getline(is, str);	// PF
		std::getline(is, str);	// camera size
		int w, h;
		sscanf_s(str.data(), "%d %d", &w, &h);
		std::cout << "map size: " << w << " " << h << std::endl;
		c2p_data = new float[w*h * 2];
		std::getline(is, str);	// endian information
								// PFM raw data
		float bin_data;
		for (int i = 0; i < w*h; i++)
		{
			is.read((char*)&bin_data, sizeof(float));
			c2p_data[i * 2 + 0] = bin_data;
			is.read((char*)&bin_data, sizeof(float));
			c2p_data[i * 2 + 1] = bin_data;
			is.read((char*)&bin_data, sizeof(float));
			if (c2p_data[i * 2 + 0] >= _projectorW || c2p_data[i * 2 + 1] >= _projectorH)
			{
				c2p_data[i * 2 + 0] = -1;
				c2p_data[i * 2 + 1] = -1;
			}
		}
		is.close();
		printf(" -> done\n");
		return c2p_data;
	};

	float *readP2C(void)
	{
		std::string str;
		float *p2c_data;

		char filename[256];
		sprintf_s(filename, "%s/P2C.pfm", _dirname);
		std::cout << "read PFM file from: " << filename << std::endl;
		std::ifstream is(filename, std::ifstream::binary);

		std::getline(is, str);	// PF
		std::getline(is, str);	// camera size
		int w, h;
		sscanf_s(str.data(), "%d %d", &w, &h);
		std::cout << "map size: " << w << " " << h << std::endl;
		p2c_data = new float[w*h * 2];
		std::getline(is, str);	// endian information
								// PFM raw data
		float bin_data;
		for (int i = 0; i < w*h; i++)
		{
			is.read((char*)&bin_data, sizeof(float));
			p2c_data[i * 2 + 0] = bin_data;
			is.read((char*)&bin_data, sizeof(float));
			p2c_data[i * 2 + 1] = bin_data;
			is.read((char*)&bin_data, sizeof(float));
			if (p2c_data[i * 2 + 0] >= _cameraW || p2c_data[i * 2 + 1] >= _cameraH) {
				p2c_data[i * 2 + 0] = -1;
				p2c_data[i * 2 + 1] = -1;
			}
		}
		is.close();
		printf(" -> done\n");
		return p2c_data;
	};

	void saveLookup(std::string dir)
	{
		// Convert directory to char type
		const char *dirname = dir.c_str();

		FILE *fp;
		fopen_s(&fp, dirname, "w");

		// Inverse projector y (because the openGL screen has different coordinate with OpenCV)
		for (int y = 0; y<_cameraH; y++)
		{
			for (int x = 0; x<_cameraW; x++)
			{
				fprintf(fp, "%d\t%d\t%d\t%d\n", x, y, C2P[VERT][y*_cameraW + x], WIN_H - C2P[HORI][y*_cameraW + x]);
			}
		}
		fclose(fp);
	};
};