# Projector-Camera Calibration using a basic checkerboard

In this project, we are using well-known checkerboard method to calibrate the projector and camera system. 
Our basic idea is to project the checker pattern on the printed yellow pattern. 
The system automatically captured the projection image and the printed image and matching them. 
Then, obtain the intrinsic and extrinsic parameters. 

## Dependencies

This project verified to run with MSVC2017 and it is only need some libraries to complete complie:

> GLUT for Windows. Please install [freeglut](https://www.transmissionzero.co.uk/software/freeglut-devel/), the documents [how to install](http://home.ku.edu.tr/~yyemez/Comp410/GLUT%20for%20Windows.html) follows the documents.

> [OpenCV 3.4.2 or later](https://opencv.org/releases.html)

> This project using PointGrey camera, so that it is also requires PointGrey SDK. 

We will provide a full document very soon.