/*
 * ARDrawingContext.cpp
 *
 *  Created on: Feb 18, 2013
 *      Author: pavel
 */

#include "/Users/pavel/code/opencv-experiments/pattern_search/engine/src/ARDrawingContext.hpp"

void ARDrawingContextDrawCallback(void* param)
{
	ARDrawingContext * ctx = static_cast<ARDrawingContext*>(param);
	if (ctx)
	{
		ctx->draw();
	}
}

ARDrawingContext::ARDrawingContext() :
		windowName("main"),
		isTextureInitialized(false)
{
	cv::namedWindow(windowName, cv::WINDOW_OPENGL);

	cv::setOpenGlContext(windowName);
	cv::setOpenGlDrawCallback(windowName, ARDrawingContextDrawCallback, this);
}

ARDrawingContext::~ARDrawingContext()
{
	cv::setOpenGlDrawCallback(windowName, nullptr, nullptr);
}

void ARDrawingContext::resize(cv::Size new_size)
{
	cv::resizeWindow(windowName, new_size.width, new_size.height);
}

void ARDrawingContext::updateBackground(const cv::Mat& frame)
{
	frame.copyTo(background);
}

void ARDrawingContext::drawAll()
{
	cv::updateWindow(windowName);
}

void ARDrawingContext::draw()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	drawCameraFrame();
	drawAugmentedScene();
	glFlush();
}

void ARDrawingContext::drawCameraFrame()
{
	initTexture();
	uploadTexture(background, backgroundTextureId);

	const float w = background.cols;
	const float h = background.rows;

	const GLfloat bgTextureVertices[] =
		{ 0, 0, w, 0, 0, h, w, h };
	const GLfloat bgTextureCoords[] =
		{ 1, 0, 1, 1, 0, 0, 0, 1 };
	const GLfloat proj[] =
		{ 0, -2.f / w, 0, 0, -2.f / h, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1 };

	glMatrixMode (GL_PROJECTION);
	glLoadMatrixf(proj);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, backgroundTextureId);

	// Update attribute values.
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, bgTextureVertices);
	glTexCoordPointer(2, GL_FLOAT, 0, bgTextureCoords);

	glColor4f(1, 1, 1, 1);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void ARDrawingContext::drawAugmentedScene()
{
}

void ARDrawingContext::initTexture()
{
	if (!isTextureInitialized)
	{
		glGenTextures(1, &backgroundTextureId);
		glBindTexture(GL_TEXTURE_2D, backgroundTextureId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		isTextureInitialized = true;
	}
}

void ARDrawingContext::uploadTexture(const cv::Mat& frame, GLuint texture)
{
	const int w = frame.cols;
	const int h = frame.rows;

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, texture);

	// todo internal format must match format


	// Upload new texture data:
	if (frame.channels() == 3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_BGR_EXT,
				GL_UNSIGNED_BYTE, frame.data);
	else if (frame.channels() == 4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_BGRA_EXT,
				GL_UNSIGNED_BYTE, frame.data);
	else if (frame.channels() == 1)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_LUMINANCE,
				GL_UNSIGNED_BYTE, frame.data);
}

void ARDrawingContext::killTexture()
{
	if (isTextureInitialized)
	{
		glDeleteTextures(1, &backgroundTextureId);
		isTextureInitialized = false;
	}
}
