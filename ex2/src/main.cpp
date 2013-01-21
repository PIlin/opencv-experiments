
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <opencv2/opencv.hpp>

// #include "cartoon.hpp"

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

  while (true)
  {
    cv::Mat frame;
    camera >> frame;
    if (frame.empty())
    {
      std::cerr << "empty frame" << std::endl;
      exit(2);
    }

    cv::Mat disp_frame(frame.size(), CV_8UC3);

    //cartoonifyImage(frame, disp_frame);
    disp_frame = frame;

    imshow("cc", disp_frame);

    char kp = cv::waitKey(20);
    if (kp == 27)
    {
      break;
    }


  }

  return 0;
}
