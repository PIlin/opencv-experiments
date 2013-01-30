#ifndef TRACK_HPP__
#define TRACK_HPP__

#include <opencv2/opencv.hpp>

#include <memory>
#include <vector>

struct DtBlob
{
	uint32_t id;
	cv::Point2f centroid;
};


class Tracker
{
public:

	void update_tracks(cv::Mat const& frame);

	std::vector<uint32_t> detect(int const inactive_time_min, int const inactive_time_max) const;

	void save_detected(std::vector<uint32_t> ids);

	std::vector<DtBlob> get_all_detected() const;

	void draw_detected(cv::Mat & frame) const;
	void draw_tracks(cv::Mat & frame) const;

	void forget_detected();

	Tracker();

protected:

	class Tracker_impl;
	friend class Tracker_impl;
	std::shared_ptr<Tracker_impl> impl;
};


// std::vector<DtBlob> do_track(cv::Mat frame);

#endif // TRACK_HPP__