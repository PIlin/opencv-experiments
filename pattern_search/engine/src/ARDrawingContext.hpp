/*
 * ARDrawingContext.hpp
 *
 *  Created on: Feb 18, 2013
 *      Author: pavel
 */

#ifndef ARDRAWINGCONTEXT_HPP_
#define ARDRAWINGCONTEXT_HPP_


#include <opencv2/opencv.hpp>


#include "gl.hpp"

class ARDrawingContext {
public:
	ARDrawingContext();
	virtual ~ARDrawingContext();


public:

	void resize(cv::Size new_size);

	void updateBackground(const cv::Mat& frame);

	void drawAll();

protected:

    friend void ARDrawingContextDrawCallback(void* param);
    void draw();

private:

    void drawCameraFrame();
    void drawAugmentedScene();

    void initTexture();
    static void uploadTexture(cv::Mat const& frame, GLuint texture);
    void killTexture();

private:
	std::string windowName;

	cv::Mat background;

	bool isTextureInitialized;
	GLuint backgroundTextureId;
};

#endif /* ARDRAWINGCONTEXT_HPP_ */
