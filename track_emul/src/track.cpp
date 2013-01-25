#include "track.hpp"
#include "utils.hpp"


#include <cvblob.h>

#include <map>
#include <set>

using namespace cv;
using namespace std;

cvb::CvBlobs blobs;
cvb::CvTracks tracks;

std::map<cvb::CvID, std::shared_ptr<cvb::CvTrack>> old_tracks;

template<typename K, typename V>
std::map<K, std::shared_ptr<V>> deep_copy(std::map<K, V*> const& in)
{
	std::map<K, std::shared_ptr<V>> out;

	for (auto& it : in)
	{
		out.emplace(std::make_pair(it.first, std::make_shared<V>(*it.second)));
	}

	return out;
}


std::set<cvb::CvID> detected;

std::vector<DtBlob> do_track(cv::Mat frame)
{
	IplImage iframe = frame;



	Mat m = Mat::ones(frame.size(), CV_8UC3);
	IplImage d = m;
	IplImage* dst = &d;

	Mat mdet = Mat::zeros(frame.size(), CV_8UC3);
	IplImage ddet = mdet;


	IplImage *labelImg = cvCreateImage(cvGetSize(&iframe), IPL_DEPTH_LABEL, 1);
	unsigned int nLabeldPixels = cvb::cvLabel(&iframe, labelImg, blobs);
	cvb::cvRenderBlobs(labelImg, blobs, &iframe, dst);


	old_tracks = deep_copy(tracks);

	cvb::cvUpdateTracks(blobs, tracks, 20., 6);
	cvb::cvRenderTracks(tracks, dst, dst, CV_TRACK_RENDER_ID|CV_TRACK_RENDER_BOUNDING_BOX);

	{
		cout << "+++++++++++++++++" << endl;
		for (auto& it : old_tracks)
		{
			cout << it.first << " - " << *it.second << endl;
		}
		cout << "=================" << endl;
		for (auto& it : tracks)
		{
			cout << it.first << " - " << *it.second << endl;
		}
		cout << "=================" << endl;
	}

	// Mat label = *labelImg;
	imshow("track", m);

	const int inactivity_time = 5;
	for (auto& p : tracks)
	{
		if (p.second->inactive == 0)
		{
			cout << "inactive 0" << endl;
			auto old_p = old_tracks.find(p.first);
			if (old_p != old_tracks.end())
			{
				cout << "in old" << endl;
				if (old_p->second->inactive == inactivity_time)
				{
					cout << "old inactive " << old_p->second->inactive << endl;

					detected.insert(old_p->first);
				}
			}
		}

	}


	std::vector<DtBlob> result;

	{
		std::vector<cvb::CvID> removed;
		for (auto const& did : detected)
		{
			auto tit = tracks.find(did);
			if (tit != tracks.end())
			{
				auto bit = blobs.find(tit->second->label);
				if (bit != blobs.end())
				{
					auto b = bit->second;

					cvb::cvRenderBlob(labelImg, b, &ddet, &ddet, CV_BLOB_RENDER_COLOR|CV_BLOB_RENDER_CENTROID|CV_BLOB_RENDER_BOUNDING_BOX|CV_BLOB_RENDER_ANGLE | CV_BLOB_RENDER_TO_STD);

					result.push_back( { did, cv::Point2f(b->centroid.x, b->centroid.y) } );
				}
			}
			else
			{
				removed.push_back(did);
			}
		}

		for (auto& did : removed)
			detected.erase(did);

		imshow("detected", mdet);
	}


	cvReleaseImage(&labelImg);

	return result;
}