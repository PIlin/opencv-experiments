#ifndef TRACK_HPP__
#define TRACK_HPP__

#include <opencv2/opencv.hpp>

#include <memory>
#include <vector>

#include "IDetection.hpp"

struct DtBlob
{
	TrackID id;
	cv::Point2f centroid;
};


class Tracker : public IDetector
{
public:

	virtual cv::Point2f getTrackPos(TrackID id) const;

public:

	void update_tracks(cv::Mat const& frame);

	std::vector<TrackID> detect(int const inactive_time_min, int const inactive_time_max) const;

	void save_detected(std::vector<TrackID> ids);

	std::vector<DtBlob> get_all_detected() const;

	void draw_detected(cv::Mat & frame) const;
	void draw_tracks(cv::Mat & frame) const;

	void forget_detected();

	Tracker();

	void setDetectionConsumer(std::weak_ptr<IDetectionConsumer> dc) { det_cons = dc; }

protected:

	class Tracker_impl;
	friend class Tracker_impl;
	std::shared_ptr<Tracker_impl> impl;

	std::weak_ptr<IDetectionConsumer> det_cons;


};


// std::vector<DtBlob> do_track(cv::Mat frame);

#endif // TRACK_HPP__