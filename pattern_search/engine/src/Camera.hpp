#ifndef CAMERA_HPP__
#define CAMERA_HPP__

#include "opencv2/opencv.hpp"

#include <stdexcept>


struct Camera
{
	void checkCam()
	{
		if (!camera.isOpened())
		{
			throw std::runtime_error("Could not open camera");
		}
	}

	cv::Mat getCamFrame()
	{
		checkCam();
		camera.grab();

		cv::Mat frame;
		camera.retrieve(frame, CV_CAP_OPENNI_BGR_IMAGE);

		if (frame.empty())
		{
			throw std::runtime_error("empty frame in Camera::getCamFrame()");
		}

		return frame;
	}

protected:
	cv::VideoCapture camera;
	Camera() : Camera(0) {}
	Camera(int device) : camera(device) { checkCam(); }
	virtual ~Camera() {}
};

struct WebCamera : public Camera
{
	WebCamera(int w = 640, int h = 480) : Camera() {
		camera.set(CV_CAP_PROP_FRAME_WIDTH, w);
		camera.set(CV_CAP_PROP_FRAME_HEIGHT, h);
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


#endif // CAMERA_HPP__
