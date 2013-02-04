
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <list>
#include <memory>

#include <opencv2/opencv.hpp>

#include <cvblob.h>

#include "Camera.hpp"
#include "LightController.hpp"
#include "StateController.hpp"
#include "track.hpp"
#include "utils.hpp"

using namespace cv;
using namespace std;

const int scale = 1;

const string main_win_name = "main";
const string prev_win_name = "prev";
const string controls_win_name = "con_win";
const string hsv_names[] = {"hsv 0", "hsv 1", "hsv 2"};
const string hsv_tres[] = {"hsv tres", "hsv tres dil", "hsv tres hough"};
struct win
{
	string name;
	int x;
	int y;
	int flags;
};

const vector<win> wins = {
	{main_win_name, 10, 10, WINDOW_AUTOSIZE},
	{prev_win_name, 660, 500, WINDOW_AUTOSIZE},
	// {hsv_names[1], 660, 10, WINDOW_AUTOSIZE},
	// {hsv_names[2], 660, 500, WINDOW_AUTOSIZE},
	{hsv_tres[0], 660, 10, WINDOW_AUTOSIZE},
	{hsv_tres[1], 1300, 10, WINDOW_AUTOSIZE},
	{hsv_tres[2], 1700, 10, WINDOW_AUTOSIZE},
	{controls_win_name, 10, 1000, WINDOW_NORMAL}
};

struct track
{
	static int track_num;

	track() {}

	track(int val) : track(val, "track_" + to_string(track_num++))
	{
	}

	track(int val, string name) :
		name(new string(name)),
		value(new int(val))
	{
		cout << name << endl;
		cv::createTrackbar(name, controls_win_name, value.get(), 255, &track::on_track, this->name.get());
	}


	~track()
	{
	}

	static void on_track(int val, void* ud)
	{
		const string& name = *(const string*)ud;

		std::cout << name << " " << val << endl;
	}

	shared_ptr<string> name;
	shared_ptr<int> value;
};
int track::track_num = 0;

std::map<string, track> trackbars;

void hsvInRange(Mat src, std::vector<int> low, std::vector<int> hi, OutputArray dst)
{
	if (low[0] < hi[0])
	{
		cv::inRange(src, low, hi, dst);
	}
	else
	{
		std::vector<Mat> hsv;
		split(src, hsv);

		int tmphi = hi[0];
		hi[0] = 180;

		cv::inRange(hsv[0], low[0], hi[0], dst);

		Mat d;
		low[0] = 0;
		hi[0] = tmphi;
		cv::inRange(hsv[0], low[0], hi[0], d);

		cv::bitwise_or(dst, d, dst);

		cv::inRange(hsv[1], low[1], hi[1], d);
		cv::bitwise_and(dst, d, dst);
		cv::inRange(hsv[2], low[2], hi[2], d);
		cv::bitwise_and(dst, d, dst);

	}
}

Point click(0,0);

void on_mouse(int event, int x, int y, int, void* pf)
{
	Mat& frame = *(Mat*)pf;

	if (CV_EVENT_LBUTTONDOWN != event)
		return;

	click = Point(x, y);
}



struct CTC : public CameraTrackerControl
{
	virtual void UDC()
	{
		prevFrame = lastFrame.clone();
		lastFrame = camera->getCamFrame();
		if (scale != 1)
		{
			Size s (lastFrame.size().width / 2, lastFrame.size().height / 2);
			cv::resize(lastFrame, lastFrame, s);
		}

		applyBlur(lastFrame);

		lastFiltered = filterFromHSV(lastFrame);

		tracker->update_tracks(lastFiltered);

		draw();
	}

	virtual std::vector<uint32_t> detect(int const inactive_time_min, int const inactive_time_max) const
	{
		return tracker->detect(inactive_time_min, inactive_time_max);
	}

	virtual void save_detected(std::vector<uint32_t> ids)
	{
		return tracker->save_detected(ids);
	}

	/////////////////


	void applyBlur(Mat & frame)
	{
		int s = *trackbars["blur"].value;
		if (s > 0)
		{
			// cv::blur(frame, frame, Size(s,s));
			// cv::GaussianBlur(frame, frame, Size(s,s), 0);
			// cv::medianBlur(frame, frame, s%2 ? s : s - 1);
			Mat m = frame.clone();
			cv::bilateralFilter(frame, m, s, s*2, s/2);
			frame = m;
		}
	}

	void applyErode(Mat & tres)
	{
		int s = *trackbars["erode"].value;
		if (s > 0)
		{
			const Mat dil_elem = cv::getStructuringElement(MORPH_ELLIPSE, Size(s,s));
			erode(tres, tres, dil_elem);
		}
	}

	void applyPrevAnd(Mat& tres)
	{
		int s = *trackbars["prev_and"].value;
		if (s > 0)
		{
			prev_and_frames.push_back(tres.clone());
			while (prev_and_frames.size() > s)
				prev_and_frames.pop_front();

			for (auto& f : prev_and_frames)
			{
				cv::bitwise_and(tres, f, tres);
			}
		}
	}

	Mat filterFromHSV(Mat & frame_bgr)
	{
		Mat frame_hsv;
		cvtColor(frame_bgr, frame_hsv, CV_BGR2HSV);

		std::vector<int> low = { *trackbars["Hmin"].value, *trackbars["Smin"].value, *trackbars["Vmin"].value};
		std::vector<int> hi = { *trackbars["Hmax"].value, *trackbars["Smax"].value, *trackbars["Vmax"].value};

		Mat tres;
		//cv::inRange(frame, low, hi, tres);
		hsvInRange(frame_hsv, low, hi, tres);

		applyErode(tres);
		applyPrevAnd(tres);

		return tres;
	}


	void draw() const
	{
		imshow(main_win_name, lastFrame);
		imshow(prev_win_name, prevFrame);
		imshow(hsv_tres[0], lastFiltered);

		Mat m = zeroed(lastFrame);
		tracker->draw_tracks(m);
		imshow(hsv_tres[1], m);

		m = zeroed(lastFrame);
		tracker->draw_detected(m);
		imshow(hsv_tres[2], m);
	}


	CTC (std::unique_ptr<Camera> cam, std::shared_ptr<Tracker> tracker) :
		camera(std::move(cam)),
		tracker(std::move(tracker)),
		lastFrame(Mat::zeros(Size(1,1), CV_8UC3))
	{
	}

	std::unique_ptr<Camera> camera;
	std::shared_ptr<Tracker> tracker;
	Mat lastFrame;
	Mat prevFrame;
	Mat lastFiltered;

	int scale = 1;
	std::list<cv::Mat> prev_and_frames;
};

struct CDO : public CalibrationDelayOptions
{
public:
	virtual int min_delay() const { return *trackbars["calib_delay_min"].value; }
	virtual int max_delay() const { return *trackbars["calib_delay_max"].value; }
	virtual int search_attempts() const { return *trackbars["calib_search_attempts"].value; }
	virtual int search_possible_skip_frames() const { return *trackbars["calib_pos_skip_frames"].value; }
};


int main ( int argc, char **argv )  try
{
	CTC ctc(make_unique<WebCamera>(), std::make_shared<Tracker>());
	// CTC ctc(make_unique<KinectCamera>());

	auto lc = std::make_shared<LightController>();

	ctc.tracker->setDetectionConsumer(lc);
	lc->setDetector(ctc.tracker);

	CDO cdo;

	StateController sc(*lc, ctc, cdo);

	for (auto& w : wins)
	{
		cv::namedWindow(w.name, w.flags);
		cv::moveWindow(w.name, w.x, w.y);
	}

	{
		auto t = [](int v, string n) {return std::make_pair(n, track(v, n)); };

		trackbars = {
			t(0, "Hmin"), t(/*100*/255, "Hmax"),
			t(0, "Smin"), t(/*4*/255, "Smax"),
			t(235, "Vmin"), t(255, "Vmax"),
			t(3, "erode"),
			t(0, "blur"),
			t(0, "prev_and"),
			t(4, "calib_delay_min"), t(5, "calib_delay_max"),
			t(0, "calib_search_attempts"),
			t(1, "calib_pos_skip_frames"),
		};
	}

	cv::Mat frame;
	// cv::setMouseCallback(main_win_name, on_mouse, &frame);


	bool manual = false;
	bool need_step = true;
	bool allow_autocalibrate = true;
	bool allow_beacon_request = true;

	while (true)
	{
		lc->poll();

		if (!manual || need_step)
		{
			sc.step();

			need_step = !manual;
		}

		if (allow_autocalibrate)
		{
			if (lc->have_undetected())
			{
				auto lid = lc->get_undetected();

				sc.begin_calibration(make_shared<LightID>(lid));
			}
		}

		if (allow_beacon_request)
		{
			lc->do_discovery();
		}

		char kp = cv::waitKey(20);
		if (kp == 27)
		{
			break;
		}

		switch (kp)
		{
		case 'm':
		case 'M':
			{
				manual = !manual;
				break;
			}
		case 'a':
			{
				allow_autocalibrate = !allow_autocalibrate;
				PPFX("allow_autocalibrate = " << allow_autocalibrate);
				break;
			}
		case 'c':
			{
				sc.begin_calibration(make_shared<LightID>(0));
				break;
			}
		case 'f':
			{
				ctc.tracker->forget_detected();
				break;
			}
		case ' ':
			{
				need_step = true;
				break;
			}
		}

	}

	return 0;
}
catch(std::exception& e)
{
	std::cerr << "Got exception\n" << e.what() << std::endl;
	return 1;
}
catch(...)
{
	fputs("Got unknow exception. Something awful. Exiting", stderr);
	return 1;
}