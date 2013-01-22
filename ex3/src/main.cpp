
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <opencv2/opencv.hpp>


using namespace cv;
using namespace std;

const string main_win_name = "cc";
const string controls_win_name = "con_win";
const string hsv_names[] = {"hsv 0", "hsv 1", "hsv 2"};
const string hsv_tres[] = {"hsv tres", "hsv tres dil"};
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

Mat disp_hsv(cv::Mat frame_bgr)
{
	Mat frame;
	cvtColor(frame_bgr, frame, CV_BGR2HSV);


	std::vector<int> low = { *trackbars["Hmin"].value, *trackbars["Smin"].value, *trackbars["Vmin"].value};
	std::vector<int> hi = { *trackbars["Hmax"].value, *trackbars["Smax"].value, *trackbars["Vmax"].value};

	Mat tres;
	cv::inRange(frame, low, hi, tres);

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

	int s = *trackbars["erode"].value;
	const Mat dil_elem = cv::getStructuringElement(MORPH_ELLIPSE, Size(s,s));

	//imshow("hsv all", frame);
	imshow("hsv tres", tres);
	erode(tres, tres, dil_elem);
	imshow("hsv tres dil", tres);

	return frame;
}

Point click(0,0);

void on_mouse(int event, int x, int y, int, void* pf)
{
	Mat& frame = *(Mat*)pf;

	if (CV_EVENT_LBUTTONDOWN != event)
		return;

	click = Point(x, y);
}

int main ( int argc, char **argv )
{
	int cameraNumber = 0;
	if (argc > 1)
		cameraNumber = atoi(argv[1]);

	cv::VideoCapture camera;
	camera.open(cameraNumber);
	if (!camera.isOpened())
	{
		std::cerr << "ERROR: could not open camera " << cameraNumber << std::endl;
		exit(1);
	}

	camera.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	camera.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

	for (auto& w : wins)
	{
		cv::namedWindow(w.name, w.flags);
		cv::moveWindow(w.name, w.x, w.y);
	}

	{
		auto t = [](int v, string n) {return std::make_pair(n, track(v, n)); };

		trackbars = {
			t(0, "Hmin"), t(10, "Hmax"),
			t(0, "Smin"), t(10, "Smax"),
			t(220, "Vmin"), t(255, "Vmax"),
			t(3, "erode"),
			t(3, "blur")
		};
	}

	cv::Mat frame;
	cv::setMouseCallback(main_win_name, on_mouse, &frame);

	while (true)
	{

		camera >> frame;
		if (frame.empty())
		{
			std::cerr << "empty frame" << std::endl;
			exit(2);
		}

		// cv::Mat disp_frame(frame.size(), CV_8UC3);

		//cartoonifyImage(frame, disp_frame);

		Mat hsv = disp_hsv(frame);

		auto p =hsv.at<Vec3b>(click);
		auto ts = [](Vec3b v) -> string {
			char buf[40];
			// sprintf(buf,"%.3f;%.3f;%.3f", v[0], v[1], v[2]);
			sprintf(buf,"%u;%u;%u", v[0], v[1], v[2]);
			return buf;
		};
		putText(frame, ts(p), click, FONT_HERSHEY_DUPLEX, 0.8, 250);


		imshow(main_win_name,frame);



		char kp = cv::waitKey(20);
		if (kp == 27)
		{
			break;
		}


	}

	return 0;
}
