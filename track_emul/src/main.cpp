#include <opencv2/opencv.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "calibrator.hpp"
#include "track.hpp"
#include "utils.hpp"

using namespace std;
using namespace cv;

struct win
{
	string name;
	int x;
	int y;
	int flags;
};
const vector<win> wins = {
	{"main", 10, 10, WINDOW_AUTOSIZE},
	{"tres", 415, 10, WINDOW_AUTOSIZE},
	{"track", 820, 10, WINDOW_AUTOSIZE},
	{"detected", 1225, 10, WINDOW_AUTOSIZE}
	// {controls_win_name, 10, 1000, WINDOW_NORMAL}
};

template <typename T>
auto center(T r) -> decltype(r.tl())
{
	return r.tl() + decltype(r.tl())(r.size().width / 2, r.size().height / 2);
}

struct RealBlob
{
	bool selected;
	Rect pos;
	bool enabled;

	RealBlob(Rect r):
		selected(false),
		pos(r),
		enabled(true)
	{

	}

	template <typename ostream>
	friend ostream& operator << (ostream& os, const RealBlob &base)
	{
	    os << "RealBlob " << base.pos;
	    return os;
	}
};

struct MouseCtrl
{
	Point2i pos;
	struct pressed_t {
		bool now;
		bool prev;
		void set(bool b) { prev = now; now = b; }
		bool is_down2up() const { return !now && prev; }
		bool is_up2down() const { return now && !prev; }
	};
	std::array<pressed_t, 3> pressed;

	std::vector<shared_ptr<RealBlob>> blobs;
	decltype(blobs)::iterator current;
	// bool no_current;
	void add_blob(shared_ptr<RealBlob> b) { set_current(blobs.end()); blobs.push_back(b); current = blobs.end(); }

	std::function<void()> on_invalidate;

	MouseCtrl() :
		current(blobs.end())
	{}

	void set_current(decltype(current) c)
	{
		if (current != blobs.end())
			(*current)->selected = false;
		if (c != blobs.end())
			(*c)->selected = true;

		current = c;
	}

	void find_next_pressed()
	{
		auto contains = [&](shared_ptr<RealBlob> sb) -> bool {
			return sb->pos.contains(this->pos);
		};

		// cout << "find_next_pressed" << endl;


		auto prev = current;

		if (current == blobs.end())
		{
			set_current(find_if(blobs.begin(), blobs.end(), contains));
		}
		else
		{
			auto it = current;
			auto r = find_if(++it, blobs.end(), contains);
			if (r != blobs.end())
				set_current(r);
			else
				set_current(find_if(blobs.begin(), current, contains));
		}




		if (prev != current)
		{
			if (current == blobs.end())
				cout << "no current" << endl;

			if (on_invalidate)
			 	on_invalidate();

		}
	}

	void update_blobs()
	{
		if (pressed[0].is_up2down())
		{
			find_next_pressed();
		}
		// else if (pressed[0].is_down2up())
		// {
		// }

		if (pressed[1].is_up2down())
		{
			cout << "no current" << endl;
			set_current(blobs.end());
			if (on_invalidate)
				on_invalidate();
		}

		if (pressed[2].is_up2down() && blobs.end() != current)
		{
			(*current)->enabled = !(*current)->enabled;
			if (on_invalidate)
				on_invalidate();
		}

		if (pressed[0].now && blobs.end() != current)
		{
			auto sb = *current;

			// cout << "current " << sb << " " << *sb << endl;

			sb->pos += pos - center(sb->pos);
			if (on_invalidate)
				on_invalidate();

		}

		for (auto& p : pressed)
			p.set(p.now);
	}


	void on_mouse(int event, int x, int y)
	{
		#define P << " - " << x << ":" << y

		pos = Point2d(x, y);

		switch (event)
		{
		case CV_EVENT_LBUTTONDOWN:
			{
				pressed[0].set(true);
				break;
			}
		case CV_EVENT_LBUTTONUP:
			{
				pressed[0].set(false);
				break;
			}
		case CV_EVENT_RBUTTONDOWN:
			{
				pressed[1].set(true);
				break;
			}
		case CV_EVENT_RBUTTONUP:
			{
				pressed[1].set(false);
				break;
			}
		case CV_EVENT_MBUTTONDOWN:
			{
				pressed[2].set(true);
				break;
			}
		case CV_EVENT_MBUTTONUP:
			{
				pressed[2].set(false);
				break;
			}
		default:
			{
				// cout << "default event " << event P<< endl;
				break;
			}
		}

		update_blobs();
	}

	static void on_mouse(int event, int x, int y, int, void* pf)
	{
		assert(pf);
		MouseCtrl& m = *(MouseCtrl*)pf;
		m.on_mouse(event, x, y);
	}
} mouseCtrl;


struct BlobCtrl : public LightControl, public CameraControl
{
	virtual void Enable()
	{
		PPF();
		blobs[0]->enabled = true;
		invalidate();
	}
	virtual void Disable()
	{
		PPF();
		blobs[0]->enabled = false;
		invalidate();
	}

	///

	virtual void Lock()
	{
		locked = true;
	}

	virtual void Unlock()
	{
		locked = false;
	}

	virtual void UpdateFrame()
	{
		force_invalidate = true;
	}

	///
	std::vector<shared_ptr<RealBlob>> blobs;

	Mat frame;
	Mat control_frame;

	bool invalidated;
	bool locked;
	bool force_invalidate;


	BlobCtrl(Size mat_size) :
		frame(mat_size, CV_8UC1),
		control_frame(mat_size, CV_8UC3),
		invalidated(true),
		locked(false),
		force_invalidate(false)
	{
		blobs.push_back(make_shared<RealBlob>(Rect{10, 10, 15, 15}));
		blobs.push_back(make_shared<RealBlob>(Rect{37, 120, 20, 20}));

		for (auto& sb : blobs)
			mouseCtrl.add_blob(sb);

		mouseCtrl.on_invalidate = [=](){invalidate();};
	}

	void invalidate()
	{
		invalidated = true;
	}

	bool draw()
	{
		bool res = false;

		if (locked && !force_invalidate)
			return false;

		if (force_invalidate)
		{
			invalidated = true;
			force_invalidate = false;
		}

		if (invalidated)
		{
			control_frame = zeroed(control_frame);
			frame = zeroed(frame);

			for (auto cb : blobs)
			{
				cv::rectangle(control_frame,
					cb->pos,
					(cb->selected ? Scalar(128,255,128) : Scalar(255,255,255)),
					(cb->enabled ? CV_FILLED : 0)
				);

				if (cb->enabled)
				{
					cv::rectangle(frame,
						cb->pos,
						Scalar(255,255,255),
						CV_FILLED
					);
				}
			}


			imshow("main", control_frame);
			imshow("tres", frame);

			invalidated = false;
			res = true;
		}

		return res;
	}

} blobCtrl ({400, 300});

Tracker tracker;

void draw_tracker()
{
	Mat f;
	f = Mat::zeros(blobCtrl.frame.size(), CV_8UC3);
	tracker.draw_tracks(f);
	imshow("track", f);

	f = Mat::zeros(blobCtrl.frame.size(), CV_8UC3);
	tracker.draw_detected(f);
	imshow("detected", f);
}

int main()
{
	// fsm_test();
	// exit(0);

	// static int counter = 0;
	bool need_step = false;
	bool calibration = false;
	//Calibrator calib(blobCtrl, tracker);
	auto calib = std::make_shared<Calibrator>(blobCtrl, tracker, blobCtrl);

	// calib->begin();

	for (auto& w : wins)
	{
		cv::namedWindow(w.name, w.flags);
		cv::moveWindow(w.name, w.x, w.y);
	}

	cv::setMouseCallback("main", &MouseCtrl::on_mouse, &mouseCtrl);

	// init
	while (true)
	{
		if (calibration && need_step)
		{
			need_step = false;

			calib->step();

			if (calib->is_done())
			{
				calibration = false;
			}
		}

		if (blobCtrl.draw())
		{
			tracker.update_tracks(blobCtrl.frame);
			//auto dids = tracker.detect(5);
			//tracker.save_detected(dids);

			draw_tracker();

			auto dbv = tracker.get_all_detected();
			for (auto& db : dbv)
			{
				cout << "detected blob " << db.id << " " << db.centroid << endl;
			}
		}

		char kp = cv::waitKey(20);
		if (kp == 27)
		{
			break;
		}

		switch(kp)
		{
		case 'c':
			{
				calibration = true;
				calib->begin();
				break;
			}
		case ' ':
			{
				need_step = true;
				cout << "------------ need step -----------" << endl;
				break;
			}
		}

		// ++counter;
		// if (counter == 1)
		// 	need_step = true;
	}


	return 0;
}
