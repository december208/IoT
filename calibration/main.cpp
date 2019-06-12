#include "calibration.h"

int main(){ 
    CameraCalibrator myCameraCalibrator; 
    myCameraCalibrator.setFilename(); 
    myCameraCalibrator.setBorderSize(Size(9,6)); 
    myCameraCalibrator.addChessboardPoints();

    Mat src = imread("../picture/07.jpg",0);
    Mat dst;
    myCameraCalibrator.calibrate(src, dst);

    imshow("Original Image", src);
    imshow("Undistorted Image", dst);
    waitKey();
    return 0;
}