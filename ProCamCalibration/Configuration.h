// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
#pragma once

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <gl/glut.h>

#ifdef WIN32
#include <tchar.h>
#include <Windows.h>
#include <conio.h>
#include <process.h>
#include <math.h>
#else
#define _tmain(a,b) main(a,b)
#define _TCHAR char
#define _getch()
#endif

// Reference additional headers requires
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#define WIN_W 1024
#define WIN_H 768
#define INT_WIN_POS_X 1920
#define INT_WIN_POS_Y 68

#define CAM_ID 1
#define PRO_ID 1

#define CAM_W 1280
#define CAM_H 960
#define PROC_CAM_W 640
#define PROC_CAM_H 480
#define DISC_CAM_W 640
#define DISC_CAM_H 480

#define IMG_CHANNEL 3
#define DELAYED_MSEC 1000
#define BACKGROUND_PROC 60

// Program definitions
#define CAM_CALIBRATE_MODE 1
#define PRO_CALIBRATE_MODE 2
#define P2C_CALIBRATE_MODE 3
#define P2C_POINT_MODE 4
#define CAM_PNP_CALIBRATE_MODE 5
#define PROGRAM_EXIT 0

//#define WEBCAM_MODE