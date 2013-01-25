#include "track.hpp"
#include "utils.hpp"


#include <cvblob.h>

using namespace cv;


cvb::CvBlobs blobs;
cvb::CvTracks tracks;


void do_track(cv::Mat frame)
{
	IplImage iframe = frame;



	Mat m = zeroed(Mat(frame.size(), CV_8UC3));
	IplImage d = m;
	IplImage* dst = &d;



	IplImage *labelImg = cvCreateImage(cvGetSize(&iframe), IPL_DEPTH_LABEL, 1);
	unsigned int result = cvb::cvLabel(&iframe, labelImg, blobs);
	cvb::cvRenderBlobs(labelImg, blobs, &iframe, dst);
	cvb::cvUpdateTracks(blobs, tracks, 200., 5);
	cvb::cvRenderTracks(tracks, dst, dst);


	imshow("track", m);


	cvReleaseImage(&labelImg);
}