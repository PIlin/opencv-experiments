#ifndef IDETECTION_HPP__
#define IDETECTION_HPP__

#include <opencv2/opencv.hpp>


typedef uint32_t TrackID;

class IDetector
{
public:
	virtual cv::Point2f getTrackPos(TrackID id) const = 0;

protected:
	virtual ~IDetector() {}
};

class IDetectionConsumer
{
public:

	virtual void trackMoved(TrackID id, double x, double y) = 0;

	virtual void trackLost(TrackID id) = 0;

protected:

	virtual ~IDetectionConsumer() {}
};


#endif