// 07/02/2015
// This code is to calibrate cell deformability

// 08/17/2015
// This code will read the image from the camera RAM

// 11/22/2015
// This code is to obtain intensity of the image

// 12/01/2015
// This code is calibrate the deformation


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
		                                            int mIntensity, int YM, int wall, 
													int X0_def, int Y0_def, int Xlength_def, int Ylength_def);
}

Mat Morphology_Operations(Mat dst_binary, int morph_operator, int morph_elem, int morph_size);

double Deform(CINEHANDLE cineHandle, int imageN, int imageH, int imageW,
	          int mIntensity, int YM, int wall,
			  int X0_def, int Y0_def, int Xlength_def, int Ylength_def);


_declspec (dllexport) double imageCalibrate_Def(CINEHANDLE cineHandle, int imageN, int imageH, int imageW,
	                                            int mIntensity, int YM, int wall,
												int X0_def, int Y0_def, int Xlength_def, int Ylength_def)
{
	double deform;
	
	PhGetCineInfo(cineHandle, GCI_MAXIMGSIZE, (PVOID)&imgSizeInBytes);
	m_pImageBuffer = (PBYTE)_aligned_malloc(imgSizeInBytes, 16);
	deform = Deform(cineHandle, imageN, imageH, imageW, mIntensity, YM, wall, X0_def, Y0_def, Xlength_def, Ylength_def);
	
	return deform;
}



/* @Deformation */
double Deform(CINEHANDLE cineHandle, int imageN, int imageH, int imageW,
	          int mIntensity, int YM, int wall,
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

	Mat imcrop(image, Rect(X0_def, Y0_def, Xlength_def, Ylength_def));  // crop image (90, 0, 20, 32)
	imshow("dImCrop", imcrop);

	int xl = X0_def;   // cell position in x_left
	int xr = X0_def + Xlength_def;   // cell position in x_right
	int y = YM;    // cell position in y
	int backIntensity = mIntensity;  // backgroud intensity

	int x_el = 0;  // cell edge_left in x
	int y_el = 0;  // cell edge_left in y
	int x_er = wall;  // cell edge_right in x
	int y_er = 0;  // cell edge_right in y
	int x_mid = 0; // cell center in x
	int y_mid = 0; // cell center in y
	double y_eu = 0.0;  // cell edge_up in y
	double y_eb = 0.0;  // cell edge_bottom in y
	double edge_d_x = 0.0;   // cell edge width in x
	double edge_d_y1 = 0.0;   // cell edge width in y
	double edge_d_y2 = 0.0;   // cell edge width in y

	double major = 0.0;
	double minor = 0.0;
	double deform = 0.0;

	// Find cell left boundary
	for (int x = xl; x <= xr; x++)
	{
		Scalar intensity1 = image.at<uchar>(y, x);
		Scalar intensity2 = image.at<uchar>(y - 1, x);
		Scalar intensity3 = image.at<uchar>(y + 1, x);
		Scalar intensity4 = image.at<uchar>(y - 2, x);
		Scalar intensity5 = image.at<uchar>(y + 2, x);
		Scalar intensity6 = image.at<uchar>(y - 3, x);
		Scalar intensity7 = image.at<uchar>(y + 3, x);

		//cout << "BI = " << 0.8 * backIntensity << "x = " << x << ": " << intensity6.val[0] << ", " << intensity4.val[0] << ", " << intensity2.val[0]
		//	 << ", " << intensity1.val[0] << ", " << intensity3.val[0] << ", " << intensity5.val[0] << ", " << intensity7.val[0] << endl;

		if (intensity1.val[0] < 0.8 * backIntensity)
		{
			Scalar intensity = image.at<uchar>(y, x - 1);
			//edge_d_x = (intensity1.val[0] / intensity.val[0]);
			x_el = x;
			y_el = y;
			break;
		}
		if (intensity2.val[0] < 0.8 * backIntensity)
		{
			Scalar intensity = image.at<uchar>(y - 1, x - 1);
			//edge_d_x = (intensity2.val[0] / intensity.val[0]);
			x_el = x;
			y_el = y - 1;
			break;
		}
		if (intensity3.val[0] < 0.8 * backIntensity)
		{
			Scalar intensity = image.at<uchar>(y + 1, x - 1);
			//edge_d_x = (intensity3.val[0] / intensity.val[0]);
			x_el = x;
			y_el = y + 1;
			break;
		}
		if (intensity4.val[0] < 0.8 * backIntensity)
		{
			Scalar intensity = image.at<uchar>(y - 2, x - 1);
			//edge_d_x = (intensity4.val[0] / intensity.val[0]);
			x_el = x;
			y_el = y - 2;
			break;
		}
		if (intensity5.val[0] < 0.8 * backIntensity)
		{
			Scalar intensity = image.at<uchar>(y + 2, x - 1);
			//edge_d_x = (intensity5.val[0] / intensity.val[0]);
			x_el = x;
			y_el = y + 2;
			break;
		}
		if (intensity6.val[0] < 0.8 * backIntensity)
		{
			Scalar intensity = image.at<uchar>(y - 3, x - 1);
			//edge_d_x = (intensity6.val[0] / intensity.val[0]);
			x_el = x;
			y_el = y - 3;
			break;
		}
		if (intensity7.val[0] < 0.8 * backIntensity)
		{
			Scalar intensity = image.at<uchar>(y + 3, x - 1);
			//edge_d_x = (intensity7.val[0] / intensity.val[0]);
			x_el = x;
			y_el = y + 3;
			break;
		}
	}
	//cout << "x_el = " << x_el << endl; //
	//cout << "y_el = " << y_el << endl; //
	//cout << "edge_d_x = " << edge_d_x << endl; //

	if (x_el > wall - 2)
	{
		return 0;
	}

	// Find cell right boundary
	/*
	for (int x = x_el + 3; x <= xr; x++)
	{
	Scalar intensity = image.at<uchar>(y_el, x);
	if (intensity.val[0] < 0.75 * backIntensity)
	{
	x_er = x;
	break;
	}
	}
	*/
	//cout << "x_er = " << x_er << endl; //

	x_mid = (x_el + x_er) / 2 + 1;//(x_el + x_er) / 2;
	y_mid = y_el;

	//cout << "x_mid = " << x_mid << endl; //

	double ixu = 0.8;
	double ixb = ixu;

	// Find cell up boundary
	for (int i = 1; i <= 8; i++)
	{
		Scalar intensity = image.at<uchar>(y_mid - i, x_mid);
		Scalar intensity1 = image.at<uchar>(y_mid - i - 1, x_mid);
		Scalar intensity2 = image.at<uchar>(y_mid - i - 2, x_mid);
		Scalar intensity3 = image.at<uchar>(y_mid - i - 3, x_mid);

		//cout << "0.8*BI = " << ixu * backIntensity << "   ,i = " << i << "   ,intensity.val = " << intensity.val[0] << endl;

		if (intensity.val[0] < ixu * backIntensity)
		{
			//cout << "0.8*BI = " << ixu * backIntensity << "   ,i = " << i << "   ,intensity.val = " << intensity.val[0] << endl;

			if (intensity1.val[0] > backIntensity * 0.85)
			{
				//cout << "intensity1 = "  << intensity1.val[0] << endl;
				//edge_d_y1 = (intensity.val[0] / intensity1.val[0]);
				y_eu = y_mid - i - edge_d_y1;
				break;
			}
			if (intensity2.val[0] > backIntensity * 0.85)
			{
				//cout << "intensity2 = " << intensity2.val[0] << endl;
				//edge_d_y1 = (intensity1.val[0] / intensity2.val[0]);
				y_eu = y_mid - i - 1 - edge_d_y1;
				break;
			}
			if (intensity3.val[0] > backIntensity * 0.85)
			{
				//cout << "intensity3 = " << intensity3.val[0] << endl;
				//edge_d_y1 = (intensity2.val[0] / intensity3.val[0]);
				y_eu = y_mid - i - 2 - edge_d_y1;
				break;
			}

		}
	}

	if (y_eu == 0)
	{
		ixu = 0.85;
		for (int i = 1; i <= 8; i++)
		{
			Scalar intensity = image.at<uchar>(y_mid - i, x_mid);
			Scalar intensity1 = image.at<uchar>(y_mid - i - 1, x_mid);
			Scalar intensity2 = image.at<uchar>(y_mid - i - 2, x_mid);
			Scalar intensity3 = image.at<uchar>(y_mid - i - 3, x_mid);

			//cout << "0.85*BI = " << ixu * backIntensity << "   ,i = " << i << "   ,intensity.val = " << intensity.val[0] << endl;

			if (intensity.val[0] < ixu * backIntensity)
			{
				if (intensity1.val[0] > backIntensity * 0.85)
				{
					//edge_d_y1 = (intensity.val[0] / intensity1.val[0]);
					y_eu = y_mid - i - edge_d_y1;
					break;
				}
				if (intensity2.val[0] > backIntensity * 0.85)
				{
					//edge_d_y1 = (intensity1.val[0] / intensity2.val[0]);
					y_eu = y_mid - i - 1 - edge_d_y1;
					break;
				}
				if (intensity3.val[0] > backIntensity * 0.85)
				{
					//edge_d_y1 = (intensity2.val[0] / intensity3.val[0]);
					y_eu = y_mid - i - 2 - edge_d_y1;
					break;
				}

			}
		}
	}

	//cout << "y_eu = " << y_eu << endl; //
	//cout << "edge_d_y1 = " << edge_d_y1 << endl; //
	if (y_eu == 0)
	{
		return 0;
	}


	// Find cell bottom boundary
	for (int i = 1; i <= 8; i++)
	{
		Scalar intensity = image.at<uchar>(y_mid + i, x_mid);
		Scalar intensity1 = image.at<uchar>(y_mid + i + 1, x_mid);
		Scalar intensity2 = image.at<uchar>(y_mid + i + 2, x_mid);
		Scalar intensity3 = image.at<uchar>(y_mid + i + 3, x_mid);

		//cout << "0.8*BIL = " << ixb * backIntensity << "   ,i = " << i << "   ,intensityL.val = " << intensity.val[0] << endl;

		if (intensity.val[0] < ixb* backIntensity)
		{
			if (intensity1.val[0] > backIntensity * 0.85)
			{
				//edge_d_y2 = (intensity.val[0] / intensity1.val[0]);
				y_eb = y_mid + i + edge_d_y2;
				break;
			}
			if (intensity2.val[0] > backIntensity * 0.85)
			{
				//edge_d_y2 = (intensity1.val[0] / intensity2.val[0]);
				y_eb = y_mid + i + 1 + edge_d_y2;
				break;
			}
			if (intensity3.val[0] > backIntensity * 0.85)
			{
				//edge_d_y2 = (intensity2.val[0] / intensity3.val[0]);
				y_eb = y_mid + i + 2 + edge_d_y2;
				break;
			}

		}
	}

	if (y_eb == 0)
	{
		ixb = 0.85;
		for (int i = 1; i <= 8; i++)
		{
			Scalar intensity = image.at<uchar>(y_mid + i, x_mid);
			Scalar intensity1 = image.at<uchar>(y_mid + i + 1, x_mid);
			Scalar intensity2 = image.at<uchar>(y_mid + i + 2, x_mid);
			Scalar intensity3 = image.at<uchar>(y_mid + i + 3, x_mid);

			//cout << "0.8*BIL = " << ixb * backIntensity << "   ,i = " << i << "   ,intensityL.val = " << intensity.val[0] << endl;

			if (intensity.val[0] < ixb * backIntensity)
			{
				if (intensity1.val[0] > backIntensity * 0.85)
				{
					//edge_d_y2 = (intensity.val[0] / intensity1.val[0]);
					y_eb = y_mid + i + edge_d_y2;
					break;
				}
				if (intensity2.val[0] > backIntensity * 0.85)
				{
					//edge_d_y2 = (intensity1.val[0] / intensity2.val[0]);
					y_eb = y_mid + i + 1 + edge_d_y2;
					break;
				}
				if (intensity3.val[0] > backIntensity * 0.85)
				{
					//edge_d_y2 = (intensity2.val[0] / intensity3.val[0]);
					y_eb = y_mid + i + 2 + edge_d_y2;
					break;
				}

			}
		}
	}
	//cout << "y_eb = " << y_eb << endl; //
	//cout << "edge_d_y2 = " << edge_d_y2 << endl; //

	if (y_eb == 0)
	{
		return 0;
	}


	// Compute cell deformability

	double dh1;
	double dh2;
	dh1 = rand() / (RAND_MAX*2.0);
	dh2 = 0;// rand() / (RAND_MAX*2.0);


	major = (y_eb + dh1) - (y_eu - dh2) + 1;
	minor = wall - x_el; //x_er - x_el;  // + 1
	//cout << "major = " << major << endl; //
	//cout << "minor = " << minor << endl; //

	if (minor == 0)
	{
		return 0;
	}

	deform = (major) / (minor);

	//deform = major / minor;
	//cout << "deformability = " << deform << endl;   //

	if (deform > 3 || deform < 1)
	{

		return 0;
	}

	//cout << deform << endl;
	return deform;
	///

	///
}
