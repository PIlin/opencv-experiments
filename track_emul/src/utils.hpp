
#ifndef UTILS_HPP__
#define UTILS_HPP__

#include <opencv2/opencv.hpp>

#include <memory>

inline cv::Mat zeroed(cv::Mat const & m)
{
	return cv::Mat::zeros(m.size(), m.type());
}

inline cv::Mat zeroed(cv::Mat const&& m)
{
	return cv::Mat::zeros(m.size(), m.type());
}

// http://herbsutter.com/gotw/_102/
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args )
{
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}

#define PF() do { std::cout << __FUNCTION__ << endl; } while(0)
#define PPF() do { std::cout << __PRETTY_FUNCTION__ << endl; } while(0)
#define PPFX(x) do { std::cout << __PRETTY_FUNCTION__ << x << endl; } while(0)


#endif