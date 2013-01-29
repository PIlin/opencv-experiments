#ifndef StateController_HPP__
#define CALIBRAOTR_HPP__

#include "track.hpp"


void fsm_test();

struct LightID;

class LightControl
{
public:
	virtual void Enable(LightID const& id) = 0;
	virtual void Disable(LightID const& id) = 0;

	virtual bool IsEnabled(LightID const& id) = 0;
	virtual bool IsDisabled(LightID const& id) = 0;

	virtual void SetDetectedID(LightID const& id, uint32_t track_id) = 0;
	virtual void DetectionFailed(LightID const& id) = 0;

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



class StateController
{
public:
	StateController(LightControl& lc, Tracker& tr, CameraControl& cc, CameraTrackerControl& ctc);

	~StateController();

	bool begin_calibration(std::shared_ptr<LightID> id);

	bool step();

	bool search_light();
	void light_found();
	void light_not_found();

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

	bool allow_calibration;
	std::shared_ptr<LightID> light_id_calibration;

	std::vector<uint32_t> found_ids;
};


#endif