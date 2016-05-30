#include "stdafx.h"
#include "PhCon.h"
#include "PhInt.h"
#include "PhFile.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>

using namespace cv;
using namespace std;

UINT imgSizeInBytes;
PBYTE m_pImageBuffer;

extern "C" {

	_declspec (dllexport) double imageCalibrate_Def(CINEHANDLE cineHandle, int imageN, int imageH, int imageW, 
		                                            int YM, int wall, 
													int X0_def, int Y0_def, int Xlength_def, int Ylength_def);
}

Mat Morphology_Operations(Mat dst_binary, int morph_operator, int morph_elem, int morph_size);

double GetMedian(double daArray[], int iSize);

double Deform(CINEHANDLE cineHandle, int imageN, int imageH, int imageW,
	          int YM, int wall,
			  int X0_def, int Y0_def, int Xlength_def, int Ylength_def);


_declspec (dllexport) double imageCalibrate_Def(CINEHANDLE cineHandle, int imageN, int imageH, int imageW,
	                                            int YM, int wall,
												int X0_def, int Y0_def, int Xlength_def, int Ylength_def)
{
	double deform;
	
	PhGetCineInfo(cineHandle, GCI_MAXIMGSIZE, (PVOID)&imgSizeInBytes);
	m_pImageBuffer = (PBYTE)_aligned_malloc(imgSizeInBytes, 16);
	deform = Deform(cineHandle, imageN, imageH, imageW, YM, wall, X0_def, Y0_def, Xlength_def, Ylength_def);
	
	return deform;
}



//
// Get median of intensity
double GetMedian(double daArray[], int iSize) {
	// Allocate an array of the same size and sort it.
	double* dpSorted = new double[iSize];
	for (int i = 0; i < iSize; ++i) {
		dpSorted[i] = daArray[i];
	}
	for (int i = iSize - 1; i > 0; --i) {
		for (int j = 0; j < i; ++j) {
			if (dpSorted[j] > dpSorted[j + 1]) {
				double dTemp = dpSorted[j];
				dpSorted[j] = dpSorted[j + 1];
				dpSorted[j + 1] = dTemp;
			}
		}
	}

	// Middle or average of middle values in the sorted array.
	double dMedian = 0.0;
	if ((iSize % 2) == 0) {
		dMedian = (dpSorted[iSize / 2] + dpSorted[(iSize / 2) - 1]) / 2.0;
	}
	else {
		dMedian = dpSorted[iSize / 2];
	}
	delete[] dpSorted;
	return dMedian;
}



/* @Deformation */
double Deform(CINEHANDLE cineHandle, int imageN, int imageH, int imageW,
	          int YM, int wall,
	          int X0_def, int Y0_def, int Xlength_def, int Ylength_def)
{
	/// Variables
	//Mat image;
	Mat dst_crop;
	Mat dst_binary;
	Mat dst_morph;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	IH imgHeader;
	IMRANGE imrange;
	//vector<unsigned char> cineData(imageH * imageW);

	/// Read images
	imrange.First = imageN;
	imrange.Cnt = 1;
	PhGetCineImage(cineHandle, (PIMRANGE)&imrange, m_pImageBuffer, imgSizeInBytes, (PIH)&imgHeader);
	Mat image = Mat(imageH, imageW, CV_8U, m_pImageBuffer);

	Mat imcrop(image, Rect(X0_def, Y0_def, Xlength_def, Ylength_def));
	Mat dimage;
	equalizeHist(imcrop, dimage);
	imshow("deform", dimage);

	int ym = YM;  // window coordinate in y_middle
	const int msize_b = 6; // length of intensity to get median of background intensity
	const int msize_c = 3; // length of intensity to get median of cell intensity

	double I_b[msize_b] = { 0 };

	for (int i = 0; i < 6; ++i)
	{
		Scalar intensity = dimage.at<uchar>(ym, i);
		I_b[i] = intensity.val[0];
	}

	int backIntensity;  // backgroud intensity
	backIntensity = GetMedian(I_b, msize_b);

	int cutI = backIntensity * 0.3;
	int upI = backIntensity * 0.7;

	double upX = 0; // position with upI
	double loX = 0; // postion with lower Intensity
	double cutX = 0; // position with cut Intensity

	double th_t = 0; // temp threshold to find boundary
	double th1 = 0; // large threshold to find boundary
	double th2 = 0; // small threshold to find boundary
	double I_c[msize_c] = { 0 }; // intensity to find cells

	for (int x = 0; x <= Xlength_def; x++)
	{
		I_c[0] = dimage.at<uchar>(ym, x);
		I_c[1] = dimage.at<uchar>(ym - 1, x);
		I_c[2] = dimage.at<uchar>(ym + 1, x);
		th_t = GetMedian(I_c, msize_c);

		if (th_t < cutI)
		{
			loX = x;
			th2 = th_t;

			I_c[0] = dimage.at<uchar>(ym, x - 1);
			I_c[1] = dimage.at<uchar>(ym - 1, x - 1);
			I_c[2] = dimage.at<uchar>(ym + 1, x - 1);
			th1 = GetMedian(I_c, msize_c);

			break;
		}
	}

	double deltaX = 0; // delta x
	deltaX = (cutI - th2) / (th1 - th2);

	if ((th1 - th2) == 0)
	{
		return 0;
	}

	double realX = loX - deltaX; // real X
	//cout << "realX = " << realX << endl;

	int wall_x = wall; // wall position
	int xm = (wall_x + loX) / 2;

	double th_tY1 = 0.0; // temp threshold to find boundary
	double th1Y1 = 0.0; // large threshold to find boundary
	double th2Y1 = 0.0; // small threshold to find boundary

	double upY1 = 0.0; // position with upI
	double loY1 = 0.0; // postion with lower Intensity
	double cutY1 = 0.0; // position with cut Intensity

	for (int y = ym + 10; y >= ym; y--)
	{
		I_c[0] = dimage.at<uchar>(y, xm);
		I_c[1] = dimage.at<uchar>(y, xm + 1);
		I_c[2] = dimage.at<uchar>(y, xm + 2);
		th_tY1 = GetMedian(I_c, msize_c);

		if (th_tY1 < cutI)
		{
			loY1 = y;
			th2Y1 = th_tY1;

			I_c[0] = dimage.at<uchar>(y + 1, xm);
			I_c[1] = dimage.at<uchar>(y + 1, xm + 1);
			I_c[2] = dimage.at<uchar>(y + 1, xm - 1);
			th1Y1 = GetMedian(I_c, msize_c);

			break;
		}
	}

	double deltaY1 = 0.0; // delta y
	deltaY1 = (cutI - th2Y1) / (th1Y1 - th2Y1);

	if ((th1Y1 - th2Y1) == 0)
	{
		return 0;
	}

	double realY1 = loY1 + deltaY1; // real Y
	double th_tY2 = 0; // temp threshold to find boundary
	double th1Y2 = 0; // large threshold to find boundary
	double th2Y2 = 0; // small threshold to find boundary

	double upY2 = 0; // position with upI
	double loY2 = 0; // postion with lower Intensity
	double cutY2 = 0; // position with cut Intensity

	for (int y = ym - 10; y <= ym; y++)
	{
		I_c[0] = dimage.at<uchar>(y, xm);
		I_c[1] = dimage.at<uchar>(y, xm + 1);
		I_c[2] = dimage.at<uchar>(y, xm + 2);
		th_tY2 = GetMedian(I_c, msize_c);

		if (th_tY2 < cutI)
		{
			loY2 = y;
			th2Y2 = th_tY2;

			I_c[0] = dimage.at<uchar>(y - 1, xm);
			I_c[1] = dimage.at<uchar>(y - 1, xm + 1);
			I_c[2] = dimage.at<uchar>(y - 1, xm - 1);
			th1Y2 = GetMedian(I_c, msize_c);

			break;
		}

	}

	double deltaY2 = 0; // delta x
	deltaY2 = (cutI - th2Y2) / (th1Y2 - th2Y2);

	if ((th1Y2 - th2Y2) == 0)
	{
		return 0;
	}

	double realY2 = loY2 - deltaY2; // real X

	if ((realY1 - realY2)< 1 || (wall_x - realX) < 1)
	{
		return 0;
	}

	double deform = 0.0;
	deform = (realY1 - realY2) / (wall_x - realX);

	if (deform > 3 || deform < 1)
	{

		return 0;
	}

	return deform;

	///
}
