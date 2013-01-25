#ifndef TRACK_HPP__
#define TRACK_HPP__

#include <opencv2/opencv.hpp>

#include <vector>

struct DtBlob
{
	uint32_t id;
	cv::Point2f centroid;
};


std::vector<DtBlob> do_track(cv::Mat frame);

#endif // TRACK_HPP__