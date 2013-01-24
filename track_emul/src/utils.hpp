
#ifndef UTILS_HPP__
#define UTILS_HPP__

#include <opencv2/opencv.hpp>

inline cv::Mat zeroed(cv::Mat& m)
{
	return cv::Mat::zeros(m.size(), m.type());
}

inline cv::Mat zeroed(cv::Mat&& m)
{
	return cv::Mat::zeros(m.size(), m.type());
}



#endif