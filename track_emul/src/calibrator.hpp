#ifndef CALIBRATOR_HPP__
#define CALIBRAOTR_HPP__

#include "track.hpp"


void fsm_test();

class LightControl
{
public:
	virtual void Enable() = 0;
	virtual void Disable() = 0;

protected:
	virtual ~LightControl() {}
};


class Calibrator
{
public:
	Calibrator(LightControl& lc, Tracker& tr);

	void begin();

	bool step();
	bool is_done();


	bool search_light();
	void light_found();

	LightControl& lc;
	Tracker& tracker;

	class FSM;
	FSM&& fsm;

	int wait_counter;
	int wait_max;

	int searches_counter;
	int searches_max;

	bool done;

	std::vector<uint32_t> found_ids;
};


#endif