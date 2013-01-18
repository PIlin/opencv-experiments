/*
 * cartoon.cpp
 *
 *  Created on: Jan 18, 2013
 *      Author: pavel
 */

#include "cartoon.hpp"

using namespace cv;

void cartoonifyImage(cv::Mat const& from, cv::Mat& to)
{
	//to = from;
	Mat gray;
	cvtColor(from, gray, CV_BGR2GRAY);
	const int MBFS = 7;
	medianBlur(gray, gray, MBFS);

	Mat edges;
	const int LFS = 5;
	Laplacian(gray, edges, CV_8U, LFS);

	Mat mask;
	const int ET = 80;
	threshold(edges, mask, 80, 255, THRESH_BINARY_INV);

	Size size = from.size();
	Size smallSize(size.width/2, size.height/2);

	Mat small(smallSize, CV_8UC3);
	resize(from, small, smallSize, 0, 0, INTER_LINEAR);

	Mat tmp(smallSize, CV_8UC3);
	const int ksize = 9;
	const double sigmaCol = 9;
	const double sigmaSpace = 7;
	const int rep = 7;
	for (int i = 0; i < rep; ++i)
	{
		bilateralFilter(small, tmp, ksize, sigmaCol, sigmaSpace);
		bilateralFilter(tmp, small, ksize, sigmaCol, sigmaSpace);
	}

	Mat big;
	resize(small, big, size, 0, 0, INTER_LINEAR);
	to.setTo(0);
	big.copyTo(to, mask);

//	to = small;
}
