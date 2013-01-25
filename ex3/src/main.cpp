
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <list>

#include <opencv2/opencv.hpp>

#include <cvblob.h>

using namespace cv;
using namespace std;

const int scale = 1;

const string main_win_name = "cc";
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
	{hsv_names[0], 10, 500, WINDOW_AUTOSIZE},
	{hsv_names[1], 660, 10, WINDOW_AUTOSIZE},
	{hsv_names[2], 660, 500, WINDOW_AUTOSIZE},
	{hsv_tres[0], 1300, 10, WINDOW_AUTOSIZE},
	{hsv_tres[1], 1300, 500, WINDOW_AUTOSIZE},
	{hsv_tres[2], 1500, 500, WINDOW_AUTOSIZE},
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


std::tuple<Mat, Mat> disp_hsv(cv::Mat frame_bgr)
{
	Mat frame;
	cvtColor(frame_bgr, frame, CV_BGR2HSV);


	std::vector<int> low = { *trackbars["Hmin"].value, *trackbars["Smin"].value, *trackbars["Vmin"].value};
	std::vector<int> hi = { *trackbars["Hmax"].value, *trackbars["Smax"].value, *trackbars["Vmax"].value};

	Mat tres;
	//cv::inRange(frame, low, hi, tres);
	hsvInRange(frame, low, hi, tres);

	std::vector<Mat> hsv;
	split(frame, hsv);

	for (int i = 0; i < hsv.size(); ++i)
	{
		Mat res;
		if (i == 0)
			res = (hsv[i] / 360.f * 255.f);
		// else if (i == 1)
			// res = Mat(hsv[i]  *  255.0f, CV_8UC1);
		else
			res = hsv[i];

		// cout << i << "  ch: " << hsv[i].channels() << " " <<hsv[i].depth()  << endl;

		imshow("hsv " + std::to_string(i), res);
	}



	//imshow("hsv all", frame);
	imshow("hsv tres", tres);

	{
		int s = *trackbars["erode"].value;
		if (s > 0)
		{
			const Mat dil_elem = cv::getStructuringElement(MORPH_ELLIPSE, Size(s,s));
			erode(tres, tres, dil_elem);
		}
	}

	{
		static std::list<cv::Mat> prev_frames;

		int s = *trackbars["prev_and"].value;
		if (s > 0)
		{
			prev_frames.push_back(tres.clone());
			while (prev_frames.size() > s)
				prev_frames.pop_front();

			for (auto& f : prev_frames)
			{
				cv::bitwise_and(tres, f, tres);
			}
		}
	}

	imshow("hsv tres dil", tres);

	{
		Mat tr = tres.clone();
		cvb::CvBlobs blobs;
		IplImage img = tr;
		Mat mbgr =  frame_bgr.clone();
		IplImage bgr = mbgr;

		static cvb::CvTracks tracks;

		IplImage *labelImg = cvCreateImage(cvGetSize(&bgr), IPL_DEPTH_LABEL, 1);
		unsigned int result = cvb::cvLabel(&img, labelImg, blobs);
		cvb::cvRenderBlobs(labelImg, blobs, &bgr, &bgr, CV_BLOB_RENDER_BOUNDING_BOX);
		cvb::cvUpdateTracks(blobs, tracks, 200., 5);
		cvb::cvRenderTracks(tracks, &bgr, &bgr, CV_TRACK_RENDER_ID|CV_TRACK_RENDER_BOUNDING_BOX);

		imshow("hsv tres hough", mbgr);

		cvReleaseImage(&labelImg);
	}

	return make_tuple(frame, tres);
	// return frame;
}

Point click(0,0);

void on_mouse(int event, int x, int y, int, void* pf)
{
	Mat& frame = *(Mat*)pf;

	if (CV_EVENT_LBUTTONDOWN != event)
		return;

	click = Point(x, y);
}


struct Camera
{
	void checkCam()
	{
		if (!camera.isOpened())
		{
			std::cerr << "ERROR: could not open camera " << std::endl;
			exit(1);
		}
	}

	Mat getCamFrame()
	{
		checkCam();
		camera.grab();

		Mat frame;
		camera.retrieve(frame, CV_CAP_OPENNI_BGR_IMAGE);

		if (frame.empty())
		{
			std::cerr << "empty frame" << std::endl;
			exit(2);
		}

		return frame;
	}

protected:
	cv::VideoCapture camera;
	Camera() : Camera(0) {}
	Camera(int device) : camera(device) { checkCam(); }
};

struct WebCamera : public Camera
{
	WebCamera() : Camera() {
		camera.set(CV_CAP_PROP_FRAME_WIDTH, 640);
		camera.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	}
};

struct KinectCamera : public Camera
{
	KinectCamera() :
		Camera(CV_CAP_OPENNI)
	{
		// camera.set( CV_CAP_OPENNI_IMAGE_GENERATOR_OUTPUT_MODE, CV_CAP_OPENNI_VGA_30HZ );
		// cout << "FPS    " << camera.get( CV_CAP_OPENNI_IMAGE_GENERATOR+CV_CAP_PROP_FPS ) << endl;
	}
};


int main ( int argc, char **argv )
{
	// Camera camera = WebCamera();
	Camera camera = KinectCamera();

	for (auto& w : wins)
	{
		cv::namedWindow(w.name, w.flags);
		cv::moveWindow(w.name, w.x, w.y);
	}

	{
		auto t = [](int v, string n) {return std::make_pair(n, track(v, n)); };

		trackbars = {
			t(0, "Hmin"), t(100, "Hmax"),
			t(0, "Smin"), t(4, "Smax"),
			t(235, "Vmin"), t(255, "Vmax"),
			t(3, "erode"),
			t(0, "blur"),
			t(1, "prev_and")
		};
	}

	cv::Mat frame;
	cv::setMouseCallback(main_win_name, on_mouse, &frame);

	while (true)
	{
		frame = camera.getCamFrame();

		if (scale != 1)
		{
			Size s (frame.size().width / 2, frame.size().height / 2);
			cv::resize(frame, frame, s);
		}


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

		auto hsv_res = disp_hsv(frame);
		auto& hsv = get<0>(hsv_res);

		{

			auto p = hsv.at<Vec3b>(click);
			auto ts = [](Vec3b v) -> string {
				char buf[40];
				// sprintf(buf,"%.3f;%.3f;%.3f", v[0], v[1], v[2]);
				sprintf(buf,"%u;%u;%u", v[0], v[1], v[2]);
				return buf;
			};
			putText(frame, ts(p), click, FONT_HERSHEY_DUPLEX, 0.8, 250);
		}

		imshow(main_win_name,frame);

		char kp = cv::waitKey(20);
		if (kp == 27)
		{
			break;
		}


	}

	return 0;
}
