#include <iostream>

#include "Camera.hpp"
#include "ARPipeline.hpp"
#include "ARDrawingContext.hpp"
#include "utils.hpp"

void processVideo(Camera& camera, ARPipeline& pipeline,
		ARDrawingContext& drawing_context) {
	{
		cv::Mat frame = camera.getCamFrame();
		cv::Size frameSize(frame.cols, frame.rows);

		drawing_context.resize(frameSize);
	}

	bool manual = false;
	bool need_step = false;
	bool dump_images = false;

	while (true) {
		auto frame = camera.getCamFrame();

		drawing_context.updateBackground(frame);

		drawing_context.draw();

		char kp = cv::waitKey(20);
		if (kp == 27) {
			break;
		}

		switch (kp) {
		case 'm':
		case 'M': {
			manual = !manual;
			break;
		}
		case ' ': {
			need_step = true;
			break;
		}
		case 'd': {
			dump_images = true;
			break;
		}
		}
	}

}

int main()
try {

	auto camera = make_unique<WebCamera>();

	auto pipeline = make_unique<ARPipeline>();

	auto draw = make_unique<ARDrawingContext>();

	processVideo(*camera.get(), *pipeline.get(), *draw.get());

	return 0;
}
catch (std::exception& ex) {
	std::cerr << ex.what() << std::endl;
	return 1;
}
catch (...) {
	std::cerr << "something awful" << std::endl;
	return 1;

}

