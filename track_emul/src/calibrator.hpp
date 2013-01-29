#ifndef CALIBRATOR_HPP__
#define CALIBRAOTR_HPP__

#include "track.hpp"


void fsm_test();

class LightControl
{
public:
	virtual void Enable() = 0;
	virtual void Disable() = 0;

	virtual bool IsEnabled() = 0;
	virtual bool IsDisabled() = 0;

protected:
	virtual ~LightControl() {}
};

class CameraControl
{
public:
	virtual void Lock() = 0;
	virtual void Unlock() = 0;
	virtual void UpdateFrame() = 0;

protected:
	virtual ~CameraControl() {}
};

class CameraTrackerControl
{
public:
	virtual void UDC() = 0;

protected:
	virtual ~CameraTrackerControl() {}
};

class Calibrator
{
public:
	Calibrator(LightControl& lc, Tracker& tr, CameraControl& cc, CameraTrackerControl& ctc);

	~Calibrator();

	void begin();

	bool step();
	bool is_done();


	bool search_light();
	void light_found();

	LightControl& lc;
	Tracker& tracker;
	CameraControl& cc;
	CameraTrackerControl& ctc;

	class FSM;
	std::unique_ptr<FSM> fsm_;
	FSM& fsm();

	int wait_counter;
	int wait_max;

	int searches_counter;
	int searches_max;

	bool done;

	std::vector<uint32_t> found_ids;
};


#endif