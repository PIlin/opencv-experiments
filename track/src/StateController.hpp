#ifndef StateController_HPP__
#define StateController_HPP__

#include "track.hpp"
#include "IDetection.hpp"

void fsm_test();

struct LightID;

class LightControl
{
public:
	virtual void Enable(LightID const& id, std::function<void()> on_error) = 0;
	virtual void Disable(LightID const& id, std::function<void()> on_error) = 0;

	virtual bool IsEnabled(LightID const& id) = 0;
	virtual bool IsDisabled(LightID const& id) = 0;

	virtual void SetDetectedID(LightID const& id, TrackID track_id) = 0;
	virtual void DetectionFailed(LightID const& id) = 0;

protected:
	virtual ~LightControl() {}
};

class CameraTrackerControl
{
public:
	virtual void UDC() = 0;

	virtual std::vector<uint32_t> detect(int const inactive_time_min, int const inactive_time_max) const = 0;
	virtual void save_detected(std::vector<uint32_t> ids) = 0;

protected:
	virtual ~CameraTrackerControl() {}
};

class CalibrationDelayOptions
{
public:
	virtual int min_delay() const = 0;
	virtual int max_delay() const = 0;
	virtual int search_attempts() const = 0;
	virtual int search_possible_skip_frames() const = 0;
protected:
	virtual ~CalibrationDelayOptions() {}
};

class StateController
{
public:
	StateController(LightControl& lc, CameraTrackerControl& ctc, CalibrationDelayOptions& cdo);

	~StateController();

	bool begin_calibration(std::shared_ptr<LightID> id);

	bool step();

	bool search_light();
	void light_found();
	void light_not_found();

	LightControl& lc;
	CameraTrackerControl& ctc;
	CalibrationDelayOptions& cdo;

	class FSM;
	std::unique_ptr<FSM> fsm_;
	FSM& fsm();

	int wait_counter;
	//int wait_max;

	int searches_counter;
	//int searches_max;
	int searches_skip_frame_counter;

	bool allow_calibration;
	std::shared_ptr<LightID> light_id_calibration;

	std::vector<TrackID> found_ids;
};


#endif