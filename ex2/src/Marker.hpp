#ifndef MARKER_HPP__
#define MARKER_HPP__


#include <vector>

#include <opencv2/opencv.hpp>

#include "GeometryTypes.hpp"

class Marker
{
public:
  Marker();

  friend bool operator<(const Marker &M1,const Marker&M2);
  friend std::ostream & operator<<(std::ostream &str,const Marker &M);

  static cv::Mat rotate(cv::Mat in);
  static int hammDistMarker(cv::Mat bits);
  static int mat2id(const cv::Mat &bits);
  static int getMarkerId(cv::Mat &in,int &nRotations);

public:

  //id of the marker
  int id;

  //marker transformation wrt to the camera
  Transformation transformation;

  std::vector<cv::Point2f> points;
};

#endif