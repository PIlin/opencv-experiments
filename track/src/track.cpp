#include "track.hpp"
#include "utils.hpp"


#include <cvblob.h>

#include <map>
#include <set>

using namespace cv;
using namespace std;

template<typename K, typename V>
std::map<K, std::shared_ptr<V>> deep_copy(std::map<K, V*> const& in)
{
	std::map<K, std::shared_ptr<V>> out;

	for (auto& it : in)
	{
		// out.emplace(std::make_pair(it.first, std::make_shared<V>(*it.second)));
		out.insert(std::make_pair(it.first, std::make_shared<V>(*it.second)));
	}

	return out;
}

class Tracker::Tracker_impl
{
public:
	Tracker & t;
	Tracker_impl(Tracker& t) :
		t(t)
	{
	}

	/////////////////

	cvb::CvBlobs blobs;
	cvb::CvTracks tracks;
	std::map<cvb::CvID, std::shared_ptr<cvb::CvTrack>> old_tracks;
	std::set<cvb::CvID> detected;
	IplImage *labelImg = NULL;

	void update_tracks(cv::Mat const& frame)
	{
		IplImage iframe = frame;

		if (labelImg) cvReleaseImage(&labelImg);
		labelImg = cvCreateImage(cvGetSize(&iframe), IPL_DEPTH_LABEL, 1);
		unsigned int nLabeldPixels = cvb::cvLabel(&iframe, labelImg, blobs);

		old_tracks = deep_copy(tracks);

		cvb::cvUpdateTracks(blobs, tracks, 40., 10);

		{
			cout << "+++++++++++++++++" << endl;
			for (auto const& it : old_tracks)
			{
				cout << it.first << " - " << *it.second << endl;
			}
			cout << "=================" << endl;
			for (auto const& it : tracks)
			{
				cout << it.first << " - " << *it.second << endl;
			}
			cout << "=================" << endl;
		}


		// remove old tracks from detected
		std::vector<cvb::CvID> removed;
		for (auto const& d : detected)
		{
			if (tracks.find(d) == tracks.end())
				removed.push_back(d);
		}
		for (auto const& d : removed)
		{
			cout << "removing track " << d << " from detected" << endl;
			detected.erase(d);
		}
	}

	std::vector<uint32_t> detect(int const inactive_time) const
	{
		PPF();

		std::vector<uint32_t> res;

		for (auto& p : tracks)
		{
			if (p.second->inactive == 0)
			{
				cout << "inactive 0" << endl;
				auto old_p = old_tracks.find(p.first);
				if (old_p != old_tracks.end())
				{
					cout << "in old" << endl;
					if (old_p->second->inactive == inactive_time)
					{
						cout << "old inactive " << old_p->second->inactive << endl;

						res.push_back(old_p->first);
					}
				}
			}

		}


		return res;
	}

	void save_detected(std::vector<uint32_t> ids)
	{
		PPF();

		for (auto& id : ids)
			detected.insert(id);
	}

	std::vector<DtBlob> get_all_detected() const
	{
		std::vector<DtBlob> res;
		for (auto const& did : detected)
		{
			auto tit = tracks.find(did);
			if (tit != tracks.end())
			{
				auto bit = blobs.find(tit->second->label);
				if (bit != blobs.end())
				{
					auto b = bit->second;
					res.push_back( { did, cv::Point2f(b->centroid.x, b->centroid.y) } );
				}
			}
		}

		return res;
	}


	void draw_detected(cv::Mat & frame) const
	{
		IplImage iframe = frame;
		for (auto const& did : detected)
		{
			auto tit = tracks.find(did);
			if (tit != tracks.end())
			{
				auto bit = blobs.find(tit->second->label);
				if (bit != blobs.end())
				{
					auto const& b = bit->second;
					cvb::cvRenderBlob(labelImg, b, &iframe, &iframe
						,CV_BLOB_RENDER_COLOR|CV_BLOB_RENDER_CENTROID|CV_BLOB_RENDER_BOUNDING_BOX|CV_BLOB_RENDER_ANGLE | CV_BLOB_RENDER_TO_STD
						);
				}
			}
		}

	}

	void draw_tracks(cv::Mat & frame) const
	{
		IplImage iframe = frame;
		cvb::cvRenderBlobs(labelImg, blobs, &iframe, &iframe);
		cvb::cvRenderTracks(tracks, &iframe, &iframe);
	}
};


void Tracker::update_tracks(cv::Mat const& frame) { return impl->update_tracks(frame); }
std::vector<uint32_t> Tracker::detect(int const inactive_time) const { return impl->detect(inactive_time); }
void Tracker::save_detected(std::vector<uint32_t> ids) { return impl->save_detected(ids); }
std::vector<DtBlob> Tracker::get_all_detected() const { return impl->get_all_detected(); }

void Tracker::draw_detected(cv::Mat & frame) const { return impl->draw_detected(frame); }
void Tracker::draw_tracks(cv::Mat & frame) const { return impl->draw_tracks(frame); }

Tracker::Tracker() :
	impl(make_shared<Tracker_impl>(*this))
{

}
